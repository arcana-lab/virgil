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
#include "ThreadPoolForC.hpp"
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
#include <sched.h>

typedef int LocalityIsland;

namespace MARC {

  /*
   * Thread pool.
   */
  class ThreadPoolForCMultiQueues : public ThreadPoolForC {
    public:

      /*
       * Default constructor.
       *
       * By default, the thread pool is not extendible and it creates at least one thread.
       */
      ThreadPoolForCMultiQueues(void);

      /*
       * Constructor.
       */
      explicit ThreadPoolForCMultiQueues (
        const bool extendible,
        const std::uint32_t numThreads = std::max(std::thread::hardware_concurrency(), 2u) - 1u,
        std::function <void (void)> codeToExecuteAtDeconstructor = nullptr);

      /*
       * Submit a job to be run by the thread pool and detach it from the caller.
       */
      void submitAndDetach (
        void (*f) (void *args),
        void *args
        ) override;

      /*
       * Submit a job to be run by the thread pool and detach it from the caller.
       */
      void submitAndDetach (
        void (*f) (void *args),
        void *args,
        LocalityIsland li
        );

      /*
       * Return the number of tasks that did not start executing yet.
       */
      std::uint64_t numberOfTasksWaitingToBeProcessed (void) const override ;

      /*
       * Destructor.
       */
      ~ThreadPoolForCMultiQueues(void);

      /*
       * Non-copyable.
       */
      ThreadPoolForCMultiQueues(const ThreadPoolForCMultiQueues& rhs) = delete;

      /*
       * Non-assignable.
       */
      ThreadPoolForCMultiQueues& operator=(const ThreadPoolForCMultiQueues& rhs) = delete;

    private:

      /*
       * Object fields.
       */
      std::vector<ThreadSafeMutexQueue<ThreadCTask *>*> cWorkQueues;
      mutable pthread_spinlock_t cWorkQueuesLock;

      /*
       * Constantly running function each thread uses to acquire work items from the queue.
       */
      void workerFunction (std::atomic_bool *availability, std::uint32_t thread) override ;
  };

}

MARC::ThreadPoolForCMultiQueues::ThreadPoolForCMultiQueues(void) 
  : ThreadPoolForCMultiQueues{false} 
  {

  /*
   * Always create at least one thread.  If hardware_concurrency() returns 0,
   * subtracting one would turn it to UINT_MAX, so get the maximum of
   * hardware_concurrency() and 2 before subtracting 1.
   */

  return ;
}

MARC::ThreadPoolForCMultiQueues::ThreadPoolForCMultiQueues (
  const bool extendible,
  const std::uint32_t numThreads,
  std::function <void (void)> codeToExecuteAtDeconstructor)
  :   
      ThreadPoolForC{extendible, numThreads, codeToExecuteAtDeconstructor}
  {
  pthread_spin_init(&this->cWorkQueuesLock, 0);

  /*
   * Create 1 queue per thread
   */
  for (auto i = 0; i < numThreads; i++){
    cWorkQueues.push_back(new ThreadSafeMutexQueue<ThreadCTask *>);
  }

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

void MARC::ThreadPoolForCMultiQueues::submitAndDetach (
  void (*f) (void *args),
  void *args
  ){
  static std::uint32_t nextLocality = 0;
  this->submitAndDetach(f, args, nextLocality);
  nextLocality++;
}

void MARC::ThreadPoolForCMultiQueues::submitAndDetach (
  void (*f) (void *args),
  void *args,
  LocalityIsland li
  ){

  /*
   * Fetch the memory.
   */
  auto cTask = this->getTask();
  cTask->setFunction(f, args);

  /*
   * Submit the task.
   */
  if (this->extendible){
    pthread_spin_lock(&this->cWorkQueuesLock);
  }

  auto queueID = li % this->cWorkQueues.size();
  this->cWorkQueues.at(queueID)->push(cTask);

  if (this->extendible){
    pthread_spin_unlock(&this->cWorkQueuesLock);
  }

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return ;
}

void MARC::ThreadPoolForCMultiQueues::workerFunction (std::atomic_bool *availability, std::uint32_t thread){

  /*
   * Fetch the current core the thread is running on
   */
  auto cpu = thread;
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  /*
   * Pin the thread to the current core is running on
   */
  pthread_t current_thread = pthread_self();
  pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);

  /*
   * Fetch the queue of the thread
   */
  pthread_spin_lock(&this->cWorkQueuesLock);
  auto threadQueue = this->cWorkQueues.at(thread);
/*  std::cout << "Worker started with cpu " << cpu << std::endl
            << "It's thread id is " << thread << std::endl
            << "cWorkQueues Size = " << cWorkQueues.size() << std::endl
            << "threadQ = " << threadQueue << std::endl;*/
  pthread_spin_unlock(&this->cWorkQueuesLock);

  while(!m_done) {
    (*availability) = true;
    ThreadCTask *pTask = nullptr;
    if(threadQueue->waitPop(pTask)) {
      (*availability) = false;
      pTask->execute();
    }
    if (pTask){
      pTask->setAvailable();
    }
  }

  return ;
}

std::uint64_t MARC::ThreadPoolForCMultiQueues::numberOfTasksWaitingToBeProcessed (void) const {
  std::uint64_t s = 0;
  pthread_spin_lock(&this->cWorkQueuesLock);
  for (auto queue : this->cWorkQueues) {
    s += queue->size();
  }
  pthread_spin_unlock(&this->cWorkQueuesLock);

  return s;
}

MARC::ThreadPoolForCMultiQueues::~ThreadPoolForCMultiQueues (void){

  /*
   * Signal threads to quite.
   */
  m_done = true;
  pthread_spin_lock(&this->cWorkQueuesLock);
  for (auto queue : this->cWorkQueues) {
    queue->invalidate();
  }
  pthread_spin_unlock(&this->cWorkQueuesLock);

  /*
   * Wait for all threads to start or avoid to start.
   */
  this->waitAllThreadsToBeUnavailable();

  /*
   * Join threads.
   */
  return ;
}
