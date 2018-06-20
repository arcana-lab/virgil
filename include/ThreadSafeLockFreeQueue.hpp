/*
 * Copyright 2017 - 2018  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadSafeLockFreeQueue class.
 * Provides a wrapper around a basic queue to provide thread safety.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
#include <pthread.h>

#include <ThreadSafeQueue.hpp>
#include <readerwriterqueue/readerwriterqueue.h>

using namespace moodycamel ;

namespace MARC {

  template <typename T>
  class ThreadSafeLockFreeQueue final : public ThreadSafeQueue<T> {
    using Base = MARC::ThreadSafeQueue<T>;

    public:

      /*
       * Default constructor
       */
      ThreadSafeLockFreeQueue ();

      /*
       * Attempt to get the first value in the queue.
       * Returns true if a value was successfully written to the out parameter, false otherwise.
       */
      bool tryPop (T& out) override ;

      /*
       * Get the first value in the queue.
       * Will block until a value is available unless clear is called or the instance is destructed.
       * Returns true if a value was successfully written to the out parameter, false otherwise.
       */
      bool waitPop (T& out) override ;
      bool waitPop (void) override ;

      /*
       * Push a new value onto the queue.
       */
      void push (T value) override ;

      /*
       * Push a new value onto the queue if the queue size is less than maxSize.
       * Otherwise, wait for it to happen and then push the new value.
       */
      bool waitPush (T value, int64_t maxSize) override ;

      /*
       * Clear all items from the queue.
       */
      void clear (void) override ;

      /*
       * Check whether or not the queue is empty.
       */
      bool empty (void) const override ;

      /*
       * Return the number of elements in the queue.
       */
      int64_t size (void) const override ;

      /*
       * Destructor.
       */
      ~ThreadSafeLockFreeQueue(void);

      /*
       * Not copyable.
       */
      ThreadSafeLockFreeQueue (const ThreadSafeLockFreeQueue & other) = delete;
      ThreadSafeLockFreeQueue & operator= (const ThreadSafeLockFreeQueue & other) = delete;

      /*
       * Not assignable.
       */
      ThreadSafeLockFreeQueue (const ThreadSafeLockFreeQueue && other) = delete;
      ThreadSafeLockFreeQueue & operator= (const ThreadSafeLockFreeQueue && other) = delete;

    private:
      mutable BlockingReaderWriterQueue<T> queue;
  };
}

template <typename T>
MARC::ThreadSafeLockFreeQueue<T>::ThreadSafeLockFreeQueue(){

  return ;
}

template <typename T>
bool MARC::ThreadSafeLockFreeQueue<T>::tryPop (T& out){

  /*
   * Check if the queue is not valid anymore.
   */
  if (!Base::m_valid){
    return false;
  }

  /*
   * Try to pop.
   */
  auto success = queue.try_dequeue(out);

  return success;
}

template <typename T>
bool MARC::ThreadSafeLockFreeQueue<T>::waitPop (T& out){

  /*
   * Check if the queue is not valid anymore.
   */
  if(!Base::m_valid) {
    return false;
  }

  /*
   * Wait until the queue will be in a valid state and it will be not empty.
   */
  while (Base::m_valid){
    if (queue.wait_dequeue_timed(out, std::chrono::milliseconds(5))){
      break ;
    }
  }

  /*
   * If the queue is still valid, then a value has been popped and stored in out.
   */
  return Base::m_valid;
}

template <typename T>
bool MARC::ThreadSafeLockFreeQueue<T>::waitPop (void){
  T out;
  return waitPop(out);
}

template <typename T>
void MARC::ThreadSafeLockFreeQueue<T>::push (T value){
  queue.enqueue(value);

  return ;
}
 
template <typename T>
bool MARC::ThreadSafeLockFreeQueue<T>::waitPush (T value, int64_t maxSize){

  /*
   * Wait until the queue has less elements than maxSize
   */
  while (queue.size_approx() > maxSize){
    if (!Base::m_valid){
      return false;
    }
  }

  /*
   * Push
   */
  push(value);

  return true;
}

template <typename T>
bool MARC::ThreadSafeLockFreeQueue<T>::empty (void) const {
  auto empty = (queue.size_approx() == 0);

  return empty;
}

template <typename T>
int64_t MARC::ThreadSafeLockFreeQueue<T>::size (void) const {
  auto s = queue.size_approx();
 
  return s;
}

template <typename T>
void MARC::ThreadSafeLockFreeQueue<T>::clear (void) {
  while(!empty()){
    waitPop();
  }

  return ;
}

template <typename T>
MARC::ThreadSafeLockFreeQueue<T>::~ThreadSafeLockFreeQueue(void){
  this->invalidate();

  return ;
}
