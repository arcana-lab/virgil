/*
 * Copyright 2017 - 2019  Simone Campanoni
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

namespace MARC {

  /*
   * Thread pool.
   */
  class ThreadPool {
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
       * Add code to execute when the threadpool is destroyed.
       */
      void appendCodeToDeconstructor (std::function<void ()> codeToExecuteAtDeconstructor);

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
       * Submit a job to be run by the thread pool and detach it from the caller.
       */
      void submitAndDetachCFunction (
        void (*f) (void *args),
        void *args
        );

      /*
       * Return the number of threads that are currently idle.
       */
      std::uint32_t numberOfIdleThreads (void) const ;

      /*
       * Return the number of tasks that did not start executing yet.
       */
      std::uint64_t numberOfTasksWaitingToBeProcessed (void) const ;

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
      std::atomic_bool m_done;
      ThreadSafeMutexQueue<std::unique_ptr<IThreadTask>> m_workQueue;
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
       * Constantly running function each thread uses to acquire work items from the queue.
       */
      void worker (std::atomic_bool *availability);

      /*
       * Start new threads.
       */
      void newThreads (std::uint32_t newThreadsToGenerate);

      /*
       * Invalidates the queue and joins all running threads.
       */
      void destroy (void);
  };

}

MARC::ThreadPool::ThreadPool(void) 
  : ThreadPool{false} 
  {

  /*
   * Always create at least one thread.  If hardware_concurrency() returns 0,
   * subtracting one would turn it to UINT_MAX, so get the maximum of
   * hardware_concurrency() and 2 before subtracting 1.
   */

  return ;
}

MARC::ThreadPool::ThreadPool (
  const bool extendible,
  const std::uint32_t numThreads,
  std::function <void (void)> codeToExecuteAtDeconstructor)
  :
  m_done{false},
  m_workQueue{},
  m_threads{},
  codeToExecuteByTheDeconstructor{}
  {

  /*
   * Set whether or not the thread pool can dynamically change its number of threads.
   */
  this->extendible = extendible;

  /*
   * Start threads.
   */
  try {
    this->newThreads(numThreads);

  } catch(...) {
    destroy();
    throw;
  }

  if (codeToExecuteAtDeconstructor != nullptr){
    this->codeToExecuteByTheDeconstructor.push(codeToExecuteAtDeconstructor);
  }

  return ;
}

void MARC::ThreadPool::appendCodeToDeconstructor (std::function<void ()> codeToExecuteAtDeconstructor){
  this->codeToExecuteByTheDeconstructor.push(codeToExecuteAtDeconstructor);

  return ;
}

template <typename Func, typename... Args>
auto MARC::ThreadPool::submit (Func&& func, Args&&... args){

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
auto MARC::ThreadPool::submitToCores (const cpu_set_t& cores, Func&& func, Args&&... args){

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
auto MARC::ThreadPool::submitToCore (uint32_t core, Func&& func, Args&&... args){

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
void MARC::ThreadPool::submitAndDetach (Func&& func, Args&&... args){

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

void MARC::ThreadPool::submitAndDetachCFunction (
  void (*f) (void *args),
  void *args
  ){

  /*
   * Submit the task.
   */
  m_workQueue.push(std::make_unique<ThreadCTask>(f, args));

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return ;
}

void MARC::ThreadPool::worker (std::atomic_bool *availability){

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

void MARC::ThreadPool::newThreads (std::uint32_t newThreadsToGenerate){
  for (auto i = 0; i < newThreadsToGenerate; i++){

    /*
     * Create the availability flag.
     */
    auto flag = new std::atomic_bool(true);
    this->threadAvailability.push_back(flag);

    /*
     * Create a new thread.
     */
    this->m_threads.emplace_back(&ThreadPool::worker, this, flag);
  }

  return ;
}

void MARC::ThreadPool::destroy (void){

  /*
   * Execute the user code.
   */
  while (codeToExecuteByTheDeconstructor.size() > 0){
    std::function<void ()> code;
    codeToExecuteByTheDeconstructor.waitPop(code);
    code();
  }

  /*
   * Signal threads to quite.
   */
  m_done = true;
  m_workQueue.invalidate();

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

std::uint32_t MARC::ThreadPool::numberOfIdleThreads (void) const {
  std::uint32_t n = 0;

  for (auto i=0; i < this->m_threads.size(); i++){
    auto isThreadAvailable = this->threadAvailability[i];
    if (*isThreadAvailable){
      n++;
    }
  }

  return n;
}

std::uint64_t MARC::ThreadPool::numberOfTasksWaitingToBeProcessed (void) const {
  auto s = this->m_workQueue.size();

  return s;
}

void MARC::ThreadPool::expandPool (void) {

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
  if (this->numberOfIdleThreads() < this->m_workQueue.size()){

    /*
     * Spawn new threads.
     */
    std::lock_guard<std::mutex> lock{this->extendingMutex};
    this->newThreads(2);
  }

  return ;
}

MARC::ThreadPool::~ThreadPool (void){
  destroy();

  return ;
}
