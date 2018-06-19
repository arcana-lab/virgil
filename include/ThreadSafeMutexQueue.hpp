/*
 * Copyright 2017 - 2018  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadSafeMutexQueue class.
 * Provides a wrapper around a basic queue to provide thread safety.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

#include "ThreadSafeQueue.hpp"

namespace MARC {

  template <typename T>
  class ThreadSafeMutexQueue final : public ThreadSafeQueue<T> {
    using Base = MARC::ThreadSafeQueue<T>;

    public:

      /*
       * Default constructor
       */
      ThreadSafeMutexQueue ();

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
       * Invalidate the queue.
       * Used to ensure no conditions are being waited on in waitPop when
       * a thread or the application is trying to exit.
       * The queue is invalid after calling this method and it is an error
       * to continue using a queue after this method has been called.
       */
      void invalidate(void) override ;

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
      ~ThreadSafeMutexQueue(void);

      /*
       * Not copyable.
       */
      ThreadSafeMutexQueue (const ThreadSafeMutexQueue & other) = delete;
      ThreadSafeMutexQueue & operator= (const ThreadSafeMutexQueue & other) = delete;

      /*
       * Not assignable.
       */
      ThreadSafeMutexQueue (const ThreadSafeMutexQueue && other) = delete;
      ThreadSafeMutexQueue & operator= (const ThreadSafeMutexQueue && other) = delete;

    private:

      /*
       * Fields
       */
      mutable std::mutex m_mutex;
      std::condition_variable empty_condition;
      std::condition_variable full_condition;

      /*
       * Methods.
       */
      void internal_pushAndNotify(T& value);
      void internal_popAndNotify(T& out);

  };
}

template <typename T>
MARC::ThreadSafeMutexQueue<T>::ThreadSafeMutexQueue(){

  return ;
}

template <typename T>
bool MARC::ThreadSafeMutexQueue<T>::tryPop (T& out){
  std::lock_guard<std::mutex> lock{m_mutex};
  if(Base::m_queue.empty() || !Base::m_valid){
    return false;
  }

  this->internal_popAndNotify(out);

  return true;
}

template <typename T>
bool MARC::ThreadSafeMutexQueue<T>::waitPop (T& out){
  std::unique_lock<std::mutex> lock{m_mutex};

  /*
   * Check if the queue is not valid anymore.
   */
  if(!Base::m_valid) {
    return false;
  }

  /*
   * We need to wait until the queue is not empty.
   *
   * Check if the queue is empty.
   */
  if (Base::m_queue.empty()){

    /*
     * Wait until the queue will be in a valid state and it will be not empty.
     */
    empty_condition.wait(lock, 
      [this]()
      {
        return !Base::m_queue.empty() || !Base::m_valid;
      }
    );
  }

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::m_valid) {
    return false;
  }

  this->internal_popAndNotify(out);

  return true;
}

template <typename T>
bool MARC::ThreadSafeMutexQueue<T>::waitPop (void){
  std::unique_lock<std::mutex> lock{m_mutex};

  /*
   * Check if the queue is not valid anymore.
   */
  if(!Base::m_valid) {
    return false;
  }

  /*
   * We need to wait until the queue is not empty.
   *
   * Check if the queue is empty.
   */
  if (Base::m_queue.empty()){

    /*
     * Wait until the queue will be in a valid state and it will be not empty.
     */
    empty_condition.wait(lock, 
      [this]()
      {
        return !Base::m_queue.empty() || !Base::m_valid;
      }
    );
  }

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::m_valid) {
    return false;
  }

  /*
   * Pop the top element from the queue.
   */
  this->Base::m_queue.pop();

  /*
   * Notify about the fact that the queue might be not full now.
   */
  this->full_condition.notify_one();

  return true;
}

template <typename T>
void MARC::ThreadSafeMutexQueue<T>::push (T value){
  std::lock_guard<std::mutex> lock{m_mutex};
  internal_pushAndNotify(value);

  return ;
}
 
template <typename T>
bool MARC::ThreadSafeMutexQueue<T>::waitPush (T value, int64_t maxSize){
  std::unique_lock<std::mutex> lock{m_mutex};

  full_condition.wait(lock, 
    [this, maxSize]()
    {
      return (Base::m_queue.size() < maxSize) || !Base::m_valid;
    }
  );

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::m_valid) {
    return false;
  }

  internal_pushAndNotify(value);

  return true;
}

template <typename T>
bool MARC::ThreadSafeMutexQueue<T>::empty (void) const {
  std::lock_guard<std::mutex> lock{m_mutex};

  return Base::m_queue.empty();
}

template <typename T>
int64_t MARC::ThreadSafeMutexQueue<T>::size (void) const {
  std::lock_guard<std::mutex> lock{m_mutex};

  return Base::m_queue.size();
}

template <typename T>
void MARC::ThreadSafeMutexQueue<T>::clear (void) {
  std::lock_guard<std::mutex> lock{m_mutex};

  while(!Base::m_queue.empty()) {
    Base::m_queue.pop();
  }

  /*
   * Notify
   */
  full_condition.notify_all();

  return ;
}

template <typename T>
void MARC::ThreadSafeMutexQueue<T>::invalidate (void) {
  std::lock_guard<std::mutex> lock{m_mutex};

  /*
   * Check if the queue has been already invalidated.
   */
  if (!Base::m_valid){
    return ;
  }

  /*
   * Invalidate the queue.
   */
  Base::m_valid = false;

  /*
   * Notify
   */
  empty_condition.notify_all();
  full_condition.notify_all();

  return ;
}

template <typename T>
MARC::ThreadSafeMutexQueue<T>::~ThreadSafeMutexQueue(void){
  this->invalidate();

  return ;
}

template <typename T>
void MARC::ThreadSafeMutexQueue<T>::internal_pushAndNotify (T& value){

  /*
   * Push the value to the queue.
   */
  this->internal_push(value);

  /*
   * Notify that the queue is not empty.
   */
  empty_condition.notify_one();

  return ;
}

template <typename T>
void MARC::ThreadSafeMutexQueue<T>::internal_popAndNotify (T& out){

  /*
   * Pop the top element from the queue.
   */
  this->internal_pop(out);

  /*
   * Notify about the fact that the queue might be not full now.
   */
  full_condition.notify_one();

  return ;
}
