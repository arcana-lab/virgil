/*
 * Copyright 2017 - 2019  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadSafeSpinLockQueue class.
 * Provides a wrapper around a basic queue to provide thread safety.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>
#include <pthread.h>
#include <thread>
#include <chrono>

#include <cstdlib>


#include "ThreadSafeQueue.hpp"

namespace MARC {

  template <typename T>
  class ThreadSafeSpinLockQueue final : public ThreadSafeQueue<T> {
    using Base = MARC::ThreadSafeQueue<T>;

    public:

      /*
       * Default constructor
       */
      ThreadSafeSpinLockQueue ();

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
      ~ThreadSafeSpinLockQueue(void);

      /*
       * Not copyable.
       */
      ThreadSafeSpinLockQueue (const ThreadSafeSpinLockQueue & other) = delete;
      ThreadSafeSpinLockQueue & operator= (const ThreadSafeSpinLockQueue & other) = delete;

      /*
       * Not assignable.
       */
      ThreadSafeSpinLockQueue (const ThreadSafeSpinLockQueue && other) = delete;
      ThreadSafeSpinLockQueue & operator= (const ThreadSafeSpinLockQueue && other) = delete;

    private:
      mutable pthread_spinlock_t spinLock;
  };
}

template <typename T>
MARC::ThreadSafeSpinLockQueue<T>::ThreadSafeSpinLockQueue(){
  pthread_spin_init(&this->spinLock, 0);

  return ;
}

template <typename T>
bool MARC::ThreadSafeSpinLockQueue<T>::tryPop (T& out){
  pthread_spin_lock(&this->spinLock);
  if(Base::m_queue.empty() || !Base::m_valid){
    pthread_spin_unlock(&this->spinLock);
    return false;
  }

  this->internal_pop(out);

  pthread_spin_unlock(&this->spinLock);
  return true;
}

template <typename T>
bool MARC::ThreadSafeSpinLockQueue<T>::waitPop (T& out){

  /*
   * Check if the queue is not valid anymore.
   */
  if(!Base::m_valid) {
    return false;
  }

  /*
   * Wait until the queue will be in a valid state and it will be not empty.
   */
  while (Base::m_valid && Base::m_queue.empty()){
  }

  pthread_spin_lock(&this->spinLock);
  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::m_valid) {
    pthread_spin_unlock(&this->spinLock);
    return false;
  }

  this->internal_pop(out);

  pthread_spin_unlock(&this->spinLock);
  return true;
}

template <typename T>
bool MARC::ThreadSafeSpinLockQueue<T>::waitPop (void){

  /*
   * Check if the queue is not valid anymore.
   */
  if(!Base::m_valid) {
    return false;
  }

  /*
   * Wait until the queue will be in a valid state and it will be not empty.
   */
  while (Base::m_valid && Base::m_queue.empty()){
  }

  pthread_spin_lock(&this->spinLock);
  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::m_valid) {
    pthread_spin_unlock(&this->spinLock);
    return false;
  }

  /*
   * Pop the top element from the queue.
   */
  this->Base::m_queue.pop();

  pthread_spin_unlock(&this->spinLock);
  return true;
}

template <typename T>
void MARC::ThreadSafeSpinLockQueue<T>::push (T value){
  pthread_spin_lock(&this->spinLock);
  this->internal_push(value);
  pthread_spin_unlock(&this->spinLock);

  return ;
}
 
template <typename T>
bool MARC::ThreadSafeSpinLockQueue<T>::waitPush (T value, int64_t maxSize){
  pthread_spin_lock(&this->spinLock);

  while (Base::m_queue.size() >= maxSize && Base::m_valid){
    pthread_spin_unlock(&this->spinLock);
    pthread_spin_lock(&this->spinLock);
  }

  /*
   * Using the condition in the predicate ensures that spurious wakeups with a valid
   * but empty queue will not proceed, so only need to check for validity before proceeding.
   */
  if(!Base::m_valid) {
    pthread_spin_unlock(&this->spinLock);
    return false;
  }

  this->internal_push(value);

  pthread_spin_unlock(&this->spinLock);
  return true;
}

template <typename T>
bool MARC::ThreadSafeSpinLockQueue<T>::empty (void) const {
  pthread_spin_lock(&this->spinLock);
  auto empty = Base::m_queue.empty();
  pthread_spin_unlock(&this->spinLock);

  return empty;
}

template <typename T>
int64_t MARC::ThreadSafeSpinLockQueue<T>::size (void) const {
  pthread_spin_lock(&this->spinLock);
  auto s = Base::m_queue.size();
  pthread_spin_unlock(&this->spinLock);

  return s;
}

template <typename T>
void MARC::ThreadSafeSpinLockQueue<T>::clear (void) {
  pthread_spin_lock(&this->spinLock);
  while(!Base::m_queue.empty()) {
    Base::m_queue.pop();
  }

  pthread_spin_unlock(&this->spinLock);
  return ;
}

template <typename T>
MARC::ThreadSafeSpinLockQueue<T>::~ThreadSafeSpinLockQueue(void){
  this->invalidate();

  return ;
}
