/*
 * Copyright 2017 Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The TaskFuture class.
 */
#pragma once

#include <future>

namespace MARC {

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

}
