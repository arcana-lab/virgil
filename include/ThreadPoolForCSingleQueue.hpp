/*
 * Copyright 2020 - 2021  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadPoolForCSingleQueue class.
 * Keeps a set of threads constantly waiting to execute incoming jobs.
 */
#pragma once

#include "ThreadSafeMutexQueue.hpp"
#include "ThreadSafeSpinLockQueue.hpp"
#include "ThreadCTask.hpp"
#include "TaskFuture.hpp"
#include "ThreadPoolForC.hpp"

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
  class ThreadPoolForCSingleQueue : public ThreadPoolForC {
    public:

      /*
       * Default constructor.
       *
       * By default, the thread pool is not extendible and it creates at least one thread.
       */
      ThreadPoolForCSingleQueue(void);

      /*
       * Constructor.
       */
      explicit ThreadPoolForCSingleQueue (
        const bool extendible,
        const std::uint32_t numThreads = std::max(std::thread::hardware_concurrency(), 2u) - 1u,
        std::function <void (void)> codeToExecuteAtDeconstructor = nullptr
        );

      /*
       * Submit a job to be run by the thread pool and detach it from the caller.
       */
      void submitAndDetach (
        void (*f) (void *args),
        void *args
        ) override;

      /*
       * Return the number of tasks that did not start executing yet.
       */
      std::uint64_t numberOfTasksWaitingToBeProcessed (void) const override ;

      /*
       * Destructor.
       */
      ~ThreadPoolForCSingleQueue(void);

      /*
       * Non-copyable.
       */
      ThreadPoolForCSingleQueue(const ThreadPoolForCSingleQueue& rhs) = delete;

      /*
       * Non-assignable.
       */
      ThreadPoolForCSingleQueue& operator=(const ThreadPoolForCSingleQueue& rhs) = delete;

    protected:

      /*
       * Object fields.
       */
      ThreadSafeMutexQueue<ThreadCTask *> cWorkQueue;

      /*
       * Constantly running function each thread uses to acquire work items from the queue.
       */
      void workerFunction (std::atomic_bool *availability, std::uint32_t thread) override ;
  };

}

MARC::ThreadPoolForCSingleQueue::ThreadPoolForCSingleQueue(void) 
  : ThreadPoolForCSingleQueue{false} 
  {

  /*
   * Always create at least one thread.  If hardware_concurrency() returns 0,
   * subtracting one would turn it to UINT_MAX, so get the maximum of
   * hardware_concurrency() and 2 before subtracting 1.
   */

  return ;
}

MARC::ThreadPoolForCSingleQueue::ThreadPoolForCSingleQueue (
  const bool extendible,
  const std::uint32_t numThreads,
  std::function <void (void)> codeToExecuteAtDeconstructor)
  :
      cWorkQueue{}
    , ThreadPoolForC{extendible, numThreads, codeToExecuteAtDeconstructor}
  {

  /*
   * Start threads.
   */
  try {
    this->newThreads(numThreads);

  } catch(...) {
    throw;
  }

  return ;
}

void MARC::ThreadPoolForCSingleQueue::submitAndDetach (
  void (*f) (void *args),
  void *args
  ){

  /*
   * Fetch the memory.
   */
  auto cTask = this->getTask();
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

void MARC::ThreadPoolForCSingleQueue::workerFunction (std::atomic_bool *availability, std::uint32_t thread) {
  while(!m_done) {
    (*availability) = true;
    ThreadCTask *pTask = nullptr;
    if(this->cWorkQueue.waitPop(pTask)) {
      (*availability) = false;
      pTask->execute();
    }
    if (pTask){
      pTask->setAvailable();
    }
  }

  return ;
}

std::uint64_t MARC::ThreadPoolForCSingleQueue::numberOfTasksWaitingToBeProcessed (void) const {
  auto s = this->cWorkQueue.size();

  return s;
}

MARC::ThreadPoolForCSingleQueue::~ThreadPoolForCSingleQueue (void){

  /*
   * Signal threads to quite.
   */
  m_done = true;
  this->cWorkQueue.invalidate();

  /*
   * Join threads.
   */
  return ;
}
