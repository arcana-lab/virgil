/*
 * Copyright 2017 - 2021  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadPool class.
 * Keeps a set of threads constantly waiting to execute incoming jobs.
 */
#pragma once

#include "ThreadSafeMutexQueue.hpp"
#include "ThreadTask.hpp"
#include "TaskFuture.hpp"
#include "ThreadPoolInterface.hpp"

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

namespace arcana::virgil {

  /*
   * Thread pool.
   */
  class ThreadPool : public ThreadPoolInterface {
    public:

      /*
       * Default constructor.
       *
       * By default, the thread pool is not extendible and it creates at least one thread.
       */
      ThreadPool(void);

      /*
       * Constructor.
       */
      explicit ThreadPool (
        const bool extendible,
        const std::uint32_t numThreads = std::max(std::thread::hardware_concurrency(), 2u) - 1u,
        std::function <void (void)> codeToExecuteAtDeconstructor = nullptr);

      /*
       * Submit a job to be run by the thread pool.
       */
      template <typename Func, typename... Args>
      auto submit (Func&& func, Args&&... args);

      /*
       * Submit a job to be run by the thread pool pinning the thread to one of the specified cores.
       */
      template <typename Func, typename... Args>
      auto submitToCores (const cpu_set_t& cores, Func&& func, Args&&... args);

      /*
       * Submit a job to be run by the thread pool pinning the thread to the specified core.
       */
      template <typename Func, typename... Args>
      auto submitToCore (uint32_t core, Func&& func, Args&&... args);

      /*
       * Submit a job to be run by the thread pool and detach it from the caller.
       */
      template <typename Func, typename... Args>
      void submitAndDetach (Func&& func, Args&&... args) ;

      /*
       * Return the number of tasks that did not start executing yet.
       */
      std::uint64_t numberOfTasksWaitingToBeProcessed (void) const override ;

      /*
       * Destructor.
       */
      ~ThreadPool(void);

      /*
       * Non-copyable.
       */
      ThreadPool(const ThreadPool& rhs) = delete;

      /*
       * Non-assignable.
       */
      ThreadPool& operator=(const ThreadPool& rhs) = delete;

    private:

      /*
       * Object fields.
       */
      ThreadSafeMutexQueue<std::unique_ptr<IThreadTask>> m_workQueue;

      /*
       * Constantly running function each thread uses to acquire work items from the queue.
       */
      void workerFunction (std::atomic_bool *availability, std::uint32_t thread) override ;
  };

}

arcana::virgil::ThreadPool::ThreadPool(void) 
  : ThreadPool{false} 
  {

  /*
   * Always create at least one thread.  If hardware_concurrency() returns 0,
   * subtracting one would turn it to UINT_MAX, so get the maximum of
   * hardware_concurrency() and 2 before subtracting 1.
   */

  return ;
}

arcana::virgil::ThreadPool::ThreadPool (
  const bool extendible,
  const std::uint32_t numThreads,
  std::function <void (void)> codeToExecuteAtDeconstructor)
  :
    m_workQueue{}
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

template <typename Func, typename... Args>
auto arcana::virgil::ThreadPool::submit (Func&& func, Args&&... args){

  /*
   * Making the task.
   */
  auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  using ResultType = std::result_of_t<decltype(boundTask)()>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;
  PackagedTask task{std::move(boundTask)};

  /*
   * Create the future.
   */
  TaskFuture<ResultType> result{task.get_future()};
  
  /*
   * Submit the task.
   */
  m_workQueue.push(std::make_unique<TaskType>(std::move(task)));

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return result;
}

template <typename Func, typename... Args>
auto arcana::virgil::ThreadPool::submitToCores (const cpu_set_t& cores, Func&& func, Args&&... args){

  /*
   * Making the task.
   */
  auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  using ResultType = std::result_of_t<decltype(boundTask)()>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;
  PackagedTask task{std::move(boundTask)};

  /*
   * Create the future.
   */
  TaskFuture<ResultType> result{task.get_future()};
  
  /*
   * Submit the task.
   */
  m_workQueue.push(std::make_unique<TaskType>(cores, std::move(task)));

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return result;
}

template <typename Func, typename... Args>
auto arcana::virgil::ThreadPool::submitToCore (uint32_t core, Func&& func, Args&&... args){

  /*
   * Making the task.
   */
  auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  using ResultType = std::result_of_t<decltype(boundTask)()>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;
  PackagedTask task{std::move(boundTask)};

  /*
   * Create the future.
   */
  TaskFuture<ResultType> result{task.get_future()};

  /*
   * Set the affinity.
   */
  cpu_set_t cores;
  CPU_ZERO(&cores);
  CPU_SET(core, &cores);

  /*
   * Submit the task.
   */
  m_workQueue.push(std::make_unique<TaskType>(cores, std::move(task)));

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return result;
}

template <typename Func, typename... Args>
void arcana::virgil::ThreadPool::submitAndDetach (Func&& func, Args&&... args){

  /*
   * Making the task.
   */
  auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  using ResultType = std::result_of_t<decltype(boundTask)()>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;
  PackagedTask task{std::move(boundTask)};

  /*
   * Submit the task.
   */
  m_workQueue.push(std::make_unique<TaskType>(std::move(task)));

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return ;
}

void arcana::virgil::ThreadPool::workerFunction (std::atomic_bool *availability, std::uint32_t thread) {
  while(!m_done) {
    (*availability) = true;
    std::unique_ptr<IThreadTask> pTask{nullptr};
    if(m_workQueue.waitPop(pTask)) {
      (*availability) = false;
      pTask->execute();
    }
  }

  return ;
}

std::uint64_t arcana::virgil::ThreadPool::numberOfTasksWaitingToBeProcessed (void) const {
  auto s = this->m_workQueue.size();

  return s;
}

arcana::virgil::ThreadPool::~ThreadPool (void){

  /*
   * Signal threads to quite.
   */
  m_done = true;
  m_workQueue.invalidate();

  /*
   * Wait for all threads to start or avoid to start.
   */
  this->waitAllThreadsToBeUnavailable();

  /*
   * Join threads.
   */
  return ;
}
