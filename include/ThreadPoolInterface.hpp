/*
 * Copyright 2021 Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadPoolInterface class.
 * Keeps a set of threads constantly waiting to execute incoming jobs.
 */
#pragma once

#include "ThreadSafeMutexQueue.hpp"
#include "ThreadTask.hpp"
#include "ThreadCTask.hpp"
#include "TaskFuture.hpp"

#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <assert.h>
#include <hwloc.h>

namespace MARC {

  /*
   * Thread pool.
   */
  class ThreadPoolInterface {
    public:

      /*
       * Default constructor.
       *
       * By default, the thread pool is not extendible and it creates at least one thread.
       */
      ThreadPoolInterface(void);

      /*
       * Constructor.
       */
      explicit ThreadPoolInterface (
        const bool extendible,
        const std::uint32_t numThreads = std::max(std::thread::hardware_concurrency(), 2u) - 1u,
        std::function <void (void)> codeToExecuteAtDeconstructor = nullptr
        );

      /*
       * Add code to execute when the threadpool is destroyed.
       */
      void appendCodeToDeconstructor (std::function<void ()> codeToExecuteAtDeconstructor);

      /*
       * Return the number of threads that are currently idle.
       */
      std::uint32_t numberOfIdleThreads (void) const ;

      /*
       * Return the number of tasks that did not start executing yet.
       */
      virtual std::uint64_t numberOfTasksWaitingToBeProcessed (void) const = 0;

      uint32_t getCurrentThreadQId();

      /*
       * Destructor.
       */
      ~ThreadPoolInterface(void);

      /*
       * Non-copyable.
       */
      ThreadPoolInterface(const ThreadPoolInterface& rhs) = delete;

      /*
       * Non-assignable.
       */
      ThreadPoolInterface& operator=(const ThreadPoolInterface& rhs) = delete;

    protected:

      /*
       * Object fields.
       */
      std::atomic_bool m_done;
      std::vector<std::thread> m_threads;
      std::vector<std::atomic_bool *> threadAvailability;
      ThreadSafeMutexQueue<std::function<void ()>> codeToExecuteByTheDeconstructor;
      bool extendible;
      mutable std::mutex extendingMutex;

      /*
       * state and topology for hwloc library
       * Core 0 is dedicated to main thread
       * nextCore value starts from 1
       */
      hwloc_topology_t topo;
      uint32_t nextCore;
      std::map<uint32_t, uint32_t> coreToQIdMap;

      /*
       * Expand the pool if possible and necessary.
       */
      void expandPool (void);

      /*
       * Start new threads.
       */
      void newThreads (std::uint32_t newThreadsToGenerate);

      /*
       * Wait for threads.
       */
      void waitAllThreadsToBeUnavailable (void) ;

      /*
       * Constantly running function each thread uses to acquire work items from the queue.
       */
      virtual void workerFunction (std::atomic_bool *availability, std::uint32_t thread) = 0;

    private:

      /*
       * Object fields
       */
      static void workerFunctionTrampoline (ThreadPoolInterface *p, std::atomic_bool *availability, std::uint32_t thread) ;
  };

}

MARC::ThreadPoolInterface::ThreadPoolInterface(void)
  : ThreadPoolInterface{false} 
  {

  /*
   * Always create at least one thread.  If hardware_concurrency() returns 0,
   * subtracting one would turn it to UINT_MAX, so get the maximum of
   * hardware_concurrency() and 2 before subtracting 1.
   */

  return ;
}

MARC::ThreadPoolInterface::ThreadPoolInterface (
  const bool extendible,
  const std::uint32_t numThreads,
  std::function <void (void)> codeToExecuteAtDeconstructor)
  :
  m_done{false},
  m_threads{},
  codeToExecuteByTheDeconstructor{},
  nextCore(1)
  {

  /*
   * Set whether or not the thread pool can dynamically change its number of threads.
   */
  this->extendible = extendible;

  if (codeToExecuteAtDeconstructor != nullptr){
    this->codeToExecuteByTheDeconstructor.push(codeToExecuteAtDeconstructor);
  }

  /*
   * Initialize hwloc variables
   */
  hwloc_topology_init(&this->topo);
  hwloc_topology_load(this->topo);

  /*
   * Bind main thread to core 0 here
   *
   */
  hwloc_obj_t coreObj = hwloc_get_obj_by_type(this->topo, HWLOC_OBJ_CORE, 0);
  hwloc_set_cpubind(topo, coreObj->children[0]->cpuset, 0);

  int coreId = hwloc_bitmap_first(coreObj->children[0]->cpuset); 

  assert(coreId == 0 && "main tread is not bound to core 0"); 
  coreToQIdMap[coreId] = 0;

  return ;
}

void MARC::ThreadPoolInterface::appendCodeToDeconstructor (std::function<void ()> codeToExecuteAtDeconstructor){
  this->codeToExecuteByTheDeconstructor.push(codeToExecuteAtDeconstructor);

  return ;
}

void MARC::ThreadPoolInterface::newThreads (std::uint32_t newThreadsToGenerate){
  assert(!this->m_done);

  for (auto i = 0; i < newThreadsToGenerate; i++){

    /*
     * Create the availability flag.
     */
    auto flag = new std::atomic_bool(true);
    this->threadAvailability.push_back(flag);

    /*
     * Create a new thread.
     */
    this->m_threads.emplace_back(&this->workerFunctionTrampoline, this, flag, i);
    auto &thread = this->m_threads.back();
    
    // Create a cpu_set_t object representing a set of CPUs. Clear it and mark
    // only CPU i as set.
    hwloc_obj_t coreObj = hwloc_get_obj_by_type(this->topo, HWLOC_OBJ_CORE, nextCore);

    int rc = hwloc_set_thread_cpubind(this->topo, thread.native_handle(),
            coreObj->children[0]->cpuset, 0); 
    
    int coreId = hwloc_bitmap_first(coreObj->children[0]->cpuset);

    coreToQIdMap[coreId] = nextCore;

    if (rc != 0) {
      std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }

    nextCore++;
  }

  return ;
}

void MARC::ThreadPoolInterface::workerFunctionTrampoline (ThreadPoolInterface *p, std::atomic_bool *availability, std::uint32_t thread) {
  if (p->m_done){
    (*availability) = false;
    return ;
  }
  p->workerFunction(availability, thread);
  (*availability) = false;

  return ;
}

std::uint32_t MARC::ThreadPoolInterface::numberOfIdleThreads (void) const {
  std::uint32_t n = 0;

  for (auto i=0; i < this->m_threads.size(); i++){
    auto isThreadAvailable = this->threadAvailability[i];
    if (*isThreadAvailable){
      n++;
    }
  }

  return n;
}

void MARC::ThreadPoolInterface::expandPool (void) {
  assert(!this->m_done);

  /*
   * Check whether we are allow to expand the pool or not.
   */
  if (!this->extendible){
     return ;
  }

  /*
   * We are allow to expand the pool.
   *
   * Check whether we should expand the pool.
   */
  auto totalWaitingTasks = this->numberOfTasksWaitingToBeProcessed();
  if (this->numberOfIdleThreads() < totalWaitingTasks){

    /*
     * Spawn new threads.
     */
    std::lock_guard<std::mutex> lock{this->extendingMutex};
    this->newThreads(2);
  }

  return ;
}
      
void MARC::ThreadPoolInterface::waitAllThreadsToBeUnavailable (void) {
  for (auto i=0; i < this->threadAvailability.size(); i++){
    while (*(this->threadAvailability[i]));
  }

  return ;
}

uint32_t MARC::ThreadPoolInterface::getCurrentThreadQId(){

  hwloc_bitmap_t set = hwloc_bitmap_alloc();
  int err = hwloc_get_cpubind(this->topo, set, 0);
  if(err){
     assert(false && "hwloc cpubind function failed\n"); 
  }
  
  int coreId = hwloc_bitmap_first(set);
  
  return coreToQIdMap[coreId];

}

MARC::ThreadPoolInterface::~ThreadPoolInterface (void){
  assert(this->m_done);

  /*
   * Execute the user code.
   */
  while (codeToExecuteByTheDeconstructor.size() > 0){
    std::function<void ()> code;
    codeToExecuteByTheDeconstructor.waitPop(code);
    code();
  }

  /*
   * Join the threads
   */
  for(auto& thread : m_threads) {
    if(!thread.joinable()) {
      continue ;
    }
    thread.join();
  }
  for (auto flag : this->threadAvailability){
    delete flag;
  }

  hwloc_topology_destroy(this->topo);
  return ;
}
