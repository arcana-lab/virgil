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
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <assert.h>

namespace arcana::virgil {

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

arcana::virgil::ThreadPoolInterface::ThreadPoolInterface(void)
  : ThreadPoolInterface{false} 
  {

  /*
   * Always create at least one thread.  If hardware_concurrency() returns 0,
   * subtracting one would turn it to UINT_MAX, so get the maximum of
   * hardware_concurrency() and 2 before subtracting 1.
   */

  return ;
}

arcana::virgil::ThreadPoolInterface::ThreadPoolInterface (
  const bool extendible,
  const std::uint32_t numThreads,
  std::function <void (void)> codeToExecuteAtDeconstructor)
  :
  m_done{false},
  m_threads{},
  codeToExecuteByTheDeconstructor{}
  {

  /*
   * Set whether or not the thread pool can dynamically change its number of threads.
   */
  this->extendible = extendible;

  if (codeToExecuteAtDeconstructor != nullptr){
    this->codeToExecuteByTheDeconstructor.push(codeToExecuteAtDeconstructor);
  }

  return ;
}

void arcana::virgil::ThreadPoolInterface::appendCodeToDeconstructor (std::function<void ()> codeToExecuteAtDeconstructor){
  this->codeToExecuteByTheDeconstructor.push(codeToExecuteAtDeconstructor);

  return ;
}

void arcana::virgil::ThreadPoolInterface::newThreads (std::uint32_t newThreadsToGenerate){
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
  }

  return ;
}

void arcana::virgil::ThreadPoolInterface::workerFunctionTrampoline (ThreadPoolInterface *p, std::atomic_bool *availability, std::uint32_t thread) {
  if (p->m_done){
    (*availability) = false;
    return ;
  }
  p->workerFunction(availability, thread);
  (*availability) = false;

  return ;
}

std::uint32_t arcana::virgil::ThreadPoolInterface::numberOfIdleThreads (void) const {
  std::uint32_t n = 0;

  for (auto i=0; i < this->m_threads.size(); i++){
    auto isThreadAvailable = this->threadAvailability[i];
    if (*isThreadAvailable){
      n++;
    }
  }

  return n;
}

void arcana::virgil::ThreadPoolInterface::expandPool (void) {
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
      
void arcana::virgil::ThreadPoolInterface::waitAllThreadsToBeUnavailable (void) {
  for (auto i=0; i < this->threadAvailability.size(); i++){
    while (*(this->threadAvailability[i]));
  }

  return ;
}

arcana::virgil::ThreadPoolInterface::~ThreadPoolInterface (void){
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

  return ;
}
