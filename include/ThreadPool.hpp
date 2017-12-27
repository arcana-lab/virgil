/*
 * Copyright 2017 Simone Campanoni
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

#include "ThreadSafeQueue.hpp"

#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace MARC {

  /*
   * Thread task interface.
   */
  class IThreadTask {
    public:

      /*
       * Default constructor.
       */
      IThreadTask(void) = default;

      /*
       * Run the task.
       */
      virtual void execute() = 0;

      /*
       * Default moving operation.
       */
      IThreadTask(IThreadTask&& other) = default;
      IThreadTask& operator=(IThreadTask&& other) = default;

      /*
       * Not copyable.
       */
      IThreadTask(const IThreadTask& rhs) = delete;
      IThreadTask& operator=(const IThreadTask& rhs) = delete;

      /*
       * Default deconstructor.
       */
      virtual ~IThreadTask(void) = default;
  };

  /*
   * An implementation of the thread task interface.
   */
  template <typename Func>
  class ThreadTask: public IThreadTask {
    public:

      /*
       * Constructor.
       */
      ThreadTask(Func&& func);

      /*
       * Default moving operation.
       */
      ThreadTask(ThreadTask&& other) = default;
      ThreadTask& operator=(ThreadTask&& other) = default;

      /*
       * Not copyable.
       */
      ThreadTask(const ThreadTask& rhs) = delete;
      ThreadTask& operator=(const ThreadTask& rhs) = delete;

      /*
       * Default deconstructor.
       */
      ~ThreadTask(void) override = default;

      /*
       * Run the task.
       */
      void execute() override ;

    private:
      Func m_func;
  };

  /*
   * Thread pool.
   */
  class ThreadPool {
  public:

    /*
     * A wrapper around a std::future that adds the behavior of futures returned from std::async.
     * Specifically, this object will block and wait for execution to finish before going out of scope.
     */
    template <typename T>
    class TaskFuture {
    public:
      TaskFuture(std::future<T>&& future)
        :m_future{std::move(future)}
        {
        return ;
      }

      TaskFuture(const TaskFuture& rhs) = delete;
      TaskFuture& operator=(const TaskFuture& rhs) = delete;
      TaskFuture(TaskFuture&& other) = default;
      TaskFuture& operator=(TaskFuture&& other) = default;

      ~TaskFuture(void) {

        /*
         * Check if we have a result to wait.
         */
        if(!m_future.valid()) {
          return ;
        }

        /*
         * Wait for the result.
         */
        m_future.get();

        return ;
      }

      auto get(void) {
        return m_future.get();
      }

    private:
      std::future<T> m_future;
    };

  public:

    /*
     * Default constructor.
     */
    ThreadPool(void)
      :ThreadPool{std::max(std::thread::hardware_concurrency(), 2u) - 1u}
    {
      /*
       * Always create at least one thread.  If hardware_concurrency() returns 0,
       * subtracting one would turn it to UINT_MAX, so get the maximum of
       * hardware_concurrency() and 2 before subtracting 1.
       */
    }

    /*
     * Constructor.
     */
    explicit ThreadPool (const std::uint32_t numThreads)
      :
      m_done{false},
      m_workQueue{},
      m_threads{}
    {
      try {

        /*
         * Initialize the per-thread flags.
         */
        this->threadAvailability = new std::atomic_bool[numThreads];
        for(auto i = 0u; i < numThreads; ++i) {
          this->threadAvailability[i] = true;
        }

        /*
         * Start threads.
         */
        for(auto i = 0u; i < numThreads; ++i) {
          m_threads.emplace_back(&ThreadPool::worker, this, &(this->threadAvailability[i]));
        }

      } catch(...) {
        destroy();
        throw;
      }
    }

    /*
     * Non-copyable.
     */
    ThreadPool(const ThreadPool& rhs) = delete;

    /*
     * Non-assignable.
     */
    ThreadPool& operator=(const ThreadPool& rhs) = delete;

    /*
     * Destructor.
     */
    ~ThreadPool(void) {
      destroy();

      return ;
    }

    /*
     * Submit a job to be run by the thread pool.
     */
    template <typename Func, typename... Args>
    auto submit (Func&& func, Args&&... args) {

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
    
      return result;
    }

    /*
     * Submit a job to be run by the thread pool and detach it from the caller.
     */
    template <typename Func, typename... Args>
    void submitAndDetach (Func&& func, Args&&... args) {

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

      return ;
    }

    /*
     * Submit a job to the normal queue or to an alternative one.
     */
    template <typename Func, typename... Args>
    auto submitToAlternativeQueueIfNecessary (
      std::function<bool ()> submitToAlternativeQueue,
      ThreadSafeQueue<std::unique_ptr<IThreadTask>> &alternativeQueue,
      Func&& func, 
      Args&&... args
      ) {

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
      if (submitToAlternativeQueue()){
        alternativeQueue.push(std::make_unique<TaskType>(std::move(task)));

      } else {
        m_workQueue.push(std::make_unique<TaskType>(std::move(task)));
      }
    
      return result;
    }

    std::uint32_t numberOfIdleThreads (void) const {
      std::uint32_t n = 0;

      for (auto i=0; i < this->m_threads.size(); i++){
        if (this->threadAvailability[i]){
          n++;
        }
      }

      return n;
    }

    std::uint64_t numberOfTasksWaitingToBeProcessed (void) const {
      return this->m_workQueue.size();
    }

  private:

    /*
     * Constantly running function each thread uses to acquire work items from the queue.
     */
    void worker (std::atomic_bool *availability){
      while(!m_done) {
        *availability = true;
        std::unique_ptr<IThreadTask> pTask{nullptr};
        if(m_workQueue.waitPop(pTask)) {
          *availability = false;
          pTask->execute();
        }
      }

      return ;
    }

    /*
     * Invalidates the queue and joins all running threads.
     */
    void destroy(void) {

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
      delete[] this->threadAvailability;

      return ;
    }

  private:
    std::atomic_bool m_done;
    ThreadSafeQueue<std::unique_ptr<IThreadTask>> m_workQueue;
    std::vector<std::thread> m_threads;
    std::atomic_bool *threadAvailability;

  };
}

/*
 * Thread task
 */
template <typename Func>
MARC::ThreadTask<Func>::ThreadTask(Func&& func)
  :m_func{std::move(func)}
  {
  return ;
}

template <typename Func>
void MARC::ThreadTask<Func>::execute (void){
  this->m_func();

  return ;
}
