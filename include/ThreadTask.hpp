/*
 * Copyright 2017 - 2018  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadTask class.
 */
#pragma once

namespace MARC {

  /*
   * Thread task interface.
   */
  class IThreadTask {
    public:

      /*
       * Default constructor.
       */
      IThreadTask (void) = default;

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
      ThreadTask (Func&& func);

      /*
       * Default moving operation.
       */
      ThreadTask (ThreadTask&& other) = default;
      ThreadTask& operator= (ThreadTask&& other) = default;

      /*
       * Not copyable.
       */
      ThreadTask (const ThreadTask& rhs) = delete;
      ThreadTask& operator= (const ThreadTask& rhs) = delete;

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
}

template <typename Func>
MARC::ThreadTask<Func>::ThreadTask (Func&& func)
  :m_func{std::move(func)}
  {
  return ;
}

template <typename Func>
void MARC::ThreadTask<Func>::execute (void){
  this->m_func();

  return ;
}
