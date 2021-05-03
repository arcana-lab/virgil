/*
 * Copyright 2017 - 2021  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadPoolForC class.
 * Keeps a set of threads constantly waiting to execute incoming jobs.
 */
#pragma once

#include "ThreadSafeMutexQueue.hpp"
#include "ThreadSafeSpinLockQueue.hpp"
#include "ThreadCTask.hpp"
#include "TaskFuture.hpp"
#include "ThreadPoolInterface.hpp"
#include "ThreadSafeMutexQueueSleep.hpp"


#include <assert.h>
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

namespace MARC {

  /*
   * Thread pool.
   */
  class ThreadPoolForC : public ThreadPoolInterface {
    public:

      /*
       * Default constructor.
       *
       * By default, the thread pool is not extendible and it creates at least one thread.
       */
      ThreadPoolForC(void);

      /*
       * Constructor.
       */
      explicit ThreadPoolForC (
        const bool extendible,
        const std::uint32_t numThreads = std::max(std::thread::hardware_concurrency(), 2u) - 1u,
        std::function <void (void)> codeToExecuteAtDeconstructor = nullptr);

      /*
       * Submit a job to be run by the thread pool and detach it from the caller.
       */
      void submitAndDetach (
        void (*f) (void *args),
        void *args
        );

      /*
       * Return the number of tasks that did not start executing yet.
       */
      std::uint64_t numberOfTasksWaitingToBeProcessed (void) const override ;

      /*
       * Destructor.
       */
      ~ThreadPoolForC(void);

      /*
       * Non-copyable.
       */
      ThreadPoolForC(const ThreadPoolForC& rhs) = delete;

      /*
       * Non-assignable.
       */
      ThreadPoolForC& operator=(const ThreadPoolForC& rhs) = delete;

    private:
      std::vector<ThreadCTask *> memoryPool;
      std::vector<bool> memoryPoolAvailability;
      mutable pthread_spinlock_t memoryPoolLock;

      /*
       * Object fields.
       */
      ThreadSafeMutexQueue<ThreadCTask *> cWorkQueue;

      /*
       * Constantly running function each thread uses to acquire work items from the queue.
       */
      void worker (std::atomic_bool *availability) override ;

      /*
       * Invalidates the queue and joins all running threads.
       */
      void destroy (void) override ;
  };

}

MARC::ThreadPoolForC::ThreadPoolForC(void) 
  : ThreadPoolForC{false} 
  {

  /*
   * Always create at least one thread.  If hardware_concurrency() returns 0,
   * subtracting one would turn it to UINT_MAX, so get the maximum of
   * hardware_concurrency() and 2 before subtracting 1.
   */

  return ;
}

MARC::ThreadPoolForC::ThreadPoolForC (
  const bool extendible,
  const std::uint32_t numThreads,
  std::function <void (void)> codeToExecuteAtDeconstructor)
  :
    cWorkQueue{}
  {
  pthread_spin_init(&this->memoryPoolLock, 0);

  /*
   * Start threads.
   */
  try {
    this->newThreads(numThreads);

  } catch(...) {
    destroy();
    throw;
  }

  return ;
}

void MARC::ThreadPoolForC::submitAndDetach (
  void (*f) (void *args),
  void *args
  ){

  /*
   * Fetch the memory.
   */
  ThreadCTask *cTask = nullptr;
  pthread_spin_lock(&this->memoryPoolLock);
  auto poolSize = this->memoryPoolAvailability.size();
  for (auto i = 0; i < poolSize; i++){
    if (this->memoryPoolAvailability[i]){
      cTask = this->memoryPool[i];
      this->memoryPoolAvailability[i] = false;
    }
  }
  if (cTask == nullptr){
    cTask = new ThreadCTask(poolSize);
    this->memoryPool.push_back(cTask);
    this->memoryPoolAvailability.push_back(false);
  }
  pthread_spin_unlock(&this->memoryPoolLock);
  cTask->setFunction(f, args);

  /*
   * Submit the task.
   */
  this->cWorkQueue.push(cTask);

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return ;
}

void MARC::ThreadPoolForC::worker (std::atomic_bool *availability){

  while(!m_done) {
    (*availability) = true;
    ThreadCTask *pTask = nullptr;
    if(this->cWorkQueue.waitPop(pTask)) {
      (*availability) = false;
      pTask->execute();
    }
    if (pTask){

      /*
       * Fetch the task ID.
       */
      auto taskID = pTask->getID();

      /*
       * Set the task memory as available.
       */
      pthread_spin_lock(&this->memoryPoolLock);
      assert(taskID < this->memoryPool.size());
      assert(!this->memoryPoolAvailability[taskID]);
      this->memoryPoolAvailability[taskID] = true;
      pthread_spin_unlock(&this->memoryPoolLock);
    }
  }

  return ;
}

void MARC::ThreadPoolForC::destroy (void){
  MARC::ThreadPoolInterface::destroy();

  /*
   * Signal threads to quite.
   */
  m_done = true;
  this->cWorkQueue.invalidate();

  /*
   * Join threads.
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

std::uint64_t MARC::ThreadPoolForC::numberOfTasksWaitingToBeProcessed (void) const {
  auto s = this->cWorkQueue.size();

  return s;
}

MARC::ThreadPoolForC::~ThreadPoolForC (void){
  destroy();

  return ;
}
