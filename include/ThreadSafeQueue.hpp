/*
 * Copyright 2017 Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadSafeQueue class.
 * Provides a wrapper around a basic queue to provide thread safety.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace MARC {

  template <typename T>
  class ThreadSafeQueue{
    public:

      /*
       * Attempt to get the first value in the queue.
       * Returns true if a value was successfully written to the out parameter, false otherwise.
       */
      bool tryPop (T& out);

      /*
       * Get the first value in the queue.
       * Will block until a value is available unless clear is called or the instance is destructed.
       * Returns true if a value was successfully written to the out parameter, false otherwise.
       */
      bool waitPop (T& out);
      bool waitPop (void);

      /*
       * Push a new value onto the queue.
       */
      void push (T value);

      /*
       * Push a new value onto the queue if the queue size is less than maxSize.
       * Otherwise, wait for it to happen and then push the new value.
       */
      bool waitPush (T value, int64_t maxSize);

      /*
       * Clear all items from the queue.
       */
      void clear (void);

      /*
       * Invalidate the queue.
       * Used to ensure no conditions are being waited on in waitPop when
       * a thread or the application is trying to exit.
       * The queue is invalid after calling this method and it is an error
       * to continue using a queue after this method has been called.
       */
      void invalidate(void);

      /*
       * Check whether or not the queue is empty.
       */
      bool empty (void) const;

      /*
       * Return the number of elements in the queue.
       */
      int64_t size (void) const;

      /*
       * Returns whether or not this queue is valid.
       */
      bool isValid(void) const;
      
      /*
       * Destructor.
       */
      ~ThreadSafeQueue(void);

      /*
       * Default constructor.
       */
      ThreadSafeQueue (void) = default;

      /*
       * Not copyable.
       */
      ThreadSafeQueue (const ThreadSafeQueue & other) = delete;
      ThreadSafeQueue & operator= (const ThreadSafeQueue & other) = delete;

      /*
       * Not assignable.
       */
      ThreadSafeQueue (const ThreadSafeQueue && other) = delete;
      ThreadSafeQueue & operator= (const ThreadSafeQueue && other) = delete;

    private:
      std::queue<T> m_queue;
      std::atomic_bool m_valid{true};
      mutable std::mutex m_mutex;
      std::condition_variable empty_condition;
      std::condition_variable full_condition;

      /*
       * Private methods.
       */
      void internal_push (T& value);
      void internal_pop (T& out);
  };
}

template <typename T>
bool MARC::ThreadSafeQueue<T>::tryPop (T& out){
  std::lock_guard<std::mutex> lock{m_mutex};
  if(m_queue.empty() || !m_valid){
    return false;
  }

  internal_pop(out);

  return true;
}

template <typename T>
bool MARC::ThreadSafeQueue<T>::waitPop (T& out){
  std::unique_lock<std::mutex> lock{m_mutex};

  /*
   * Check if the queue is not valid anymore.
   */
  if(!m_valid) {
    return false;
  }

  /*
   * We need to wait until the queue is not empty.
   *
   * Check if the queue is empty.
   */
  if (m_queue.empty()){

    /*
     * Wait until the queue will be in a valid state and it will be not empty.
     */
    empty_condition.wait(lock, 
      [this]()
      {
        return !m_queue.empty() || !m_valid;
      }
    );
  }

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!m_valid) {
    return false;
  }

  internal_pop(out);

  return true;
}

template <typename T>
bool MARC::ThreadSafeQueue<T>::waitPop (void){
  std::unique_lock<std::mutex> lock{m_mutex};

  /*
   * Check if the queue is not valid anymore.
   */
  if(!m_valid) {
    return false;
  }

  /*
   * We need to wait until the queue is not empty.
   *
   * Check if the queue is empty.
   */
  if (m_queue.empty()){

    /*
     * Wait until the queue will be in a valid state and it will be not empty.
     */
    empty_condition.wait(lock, 
      [this]()
      {
        return !m_queue.empty() || !m_valid;
      }
    );
  }

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!m_valid) {
    return false;
  }

  /*
   * Pop the top element from the queue.
   */
  this->m_queue.pop();

  /*
   * Notify about the fact that the queue might be not full now.
   */
  this->full_condition.notify_one();

  return true;
}

template <typename T>
void MARC::ThreadSafeQueue<T>::push (T value){
  std::lock_guard<std::mutex> lock{m_mutex};
  internal_push(value);

  return ;
}
 
template <typename T>
bool MARC::ThreadSafeQueue<T>::waitPush (T value, int64_t maxSize){
  std::unique_lock<std::mutex> lock{m_mutex};

  full_condition.wait(lock, 
    [this, maxSize]()
    {
      return (m_queue.size() < maxSize) || !m_valid;
    }
  );

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!m_valid) {
    return false;
  }

  internal_push(value);

  return true;
}

template <typename T>
bool MARC::ThreadSafeQueue<T>::empty (void) const {
  std::lock_guard<std::mutex> lock{m_mutex};

  return m_queue.empty();
}

template <typename T>
int64_t MARC::ThreadSafeQueue<T>::size (void) const {
  std::lock_guard<std::mutex> lock{m_mutex};

  return m_queue.size();
}

template <typename T>
void MARC::ThreadSafeQueue<T>::clear (void) {
  std::lock_guard<std::mutex> lock{m_mutex};
  while(!m_queue.empty()) {
    m_queue.pop();
  }
  full_condition.notify_all();

  return ;
}

template <typename T>
void MARC::ThreadSafeQueue<T>::invalidate (void) {
  std::lock_guard<std::mutex> lock{m_mutex};

  /*
   * Check if the queue has been already invalidated.
   */
  if (!m_valid){
    return ;
  }

  /*
   * Invalidate the queue.
   */
  m_valid = false;
  empty_condition.notify_all();
  full_condition.notify_all();

  return ;
}

template <typename T>
bool MARC::ThreadSafeQueue<T>::isValid (void) const {
  std::lock_guard<std::mutex> lock{m_mutex};
    
  return m_valid;
}

template <typename T>
MARC::ThreadSafeQueue<T>::~ThreadSafeQueue(void){
  this->invalidate();

  return ;
}

template <typename T>
void MARC::ThreadSafeQueue<T>::internal_push (T& value){

  /*
   * Push the value to the queue.
   */
  m_queue.push(std::move(value));

  /*
   * Notify that the queue is not empty.
   */
  empty_condition.notify_one();

  return ;
}

template <typename T>
void MARC::ThreadSafeQueue<T>::internal_pop (T& out){

  /*
   * Fetch the element on top of the queue.
   */
  out = std::move(m_queue.front());

  /*
   * Pop the top element from the queue.
   */
  this->m_queue.pop();

  /*
   * Notify about the fact that the queue might be not full now.
   */
  full_condition.notify_one();

  return ;
}
