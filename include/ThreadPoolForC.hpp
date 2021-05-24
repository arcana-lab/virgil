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
      ThreadPoolForC (void);

      /*
       * Constructor.
       */
      explicit ThreadPoolForC (
        const bool extendible,
        const std::uint32_t numThreads = std::max(std::thread::hardware_concurrency(), 2u) - 1u,
        std::function <void (void)> codeToExecuteAtDeconstructor = nullptr
        );

      /*
       * Submit a job to be run by the thread pool and detach it from the caller.
       */
      virtual void submitAndDetach (
        void (*f) (void *args),
        void *args
        ) = 0;

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

    protected:

      /*
       * Return a free task
       */
      ThreadCTask * getTask (void);

    private:
      std::vector<ThreadCTask *> memoryPool;
      mutable pthread_spinlock_t memoryPoolLock;
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
  std::function <void (void)> codeToExecuteAtDeconstructor
  ) : ThreadPoolInterface{extendible, numThreads, codeToExecuteAtDeconstructor}
  {
  pthread_spin_init(&this->memoryPoolLock, 0);
  return ;
}

MARC::ThreadCTask * MARC::ThreadPoolForC::getTask (void){

  /*
   * Fetch the memory.
   */
  ThreadCTask *cTask = nullptr;
  pthread_spin_lock(&this->memoryPoolLock);
  auto poolSize = this->memoryPool.size();
  for (auto i = 0; i < poolSize; i++){
    if (this->memoryPool[i]->getAvailability()){
      cTask = this->memoryPool[i];
      break ;
    }
  }
  if (cTask == nullptr){
    cTask = new ThreadCTask(poolSize);
    this->memoryPool.push_back(cTask);
  }
  pthread_spin_unlock(&this->memoryPoolLock);

  return cTask;
}

MARC::ThreadPoolForC::~ThreadPoolForC (void){

  /*
   * Join threads.
   */
  return ;
}
