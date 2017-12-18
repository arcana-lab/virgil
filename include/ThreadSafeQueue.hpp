/**
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
     * Destructor.
     */
    ~ThreadSafeQueue(void){
      invalidate();
    }

    /*
     * Attempt to get the first value in the queue.
     * Returns true if a value was successfully written to the out parameter, false otherwise.
     */
    bool tryPop (T& out){
      std::lock_guard<std::mutex> lock{m_mutex};
      if(m_queue.empty() || !m_valid){
        return false;
      }

      internal_pop(out);

      return true;
    }

    /*
     * Get the first value in the queue.
     * Will block until a value is available unless clear is called or the instance is destructed.
     * Returns true if a value was successfully written to the out parameter, false otherwise.
     */
    bool waitPop (T& out) {
      std::unique_lock<std::mutex> lock{m_mutex};

      empty_condition.wait(lock, 
        [this]()
        {
          return !m_queue.empty() || !m_valid;
        }
      );

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

    /*
     * Push a new value onto the queue.
     */
    void push (T value) {
      std::lock_guard<std::mutex> lock{m_mutex};
      internal_push(value);

      return ;
    }

    /*
     * Push a new value onto the queue if the queue size is less than maxSize.
     * Otherwise, wait for it to happen and then push the new value.
     */
    bool waitPush (T value, size_t maxSize) {
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

    /*
     * Check whether or not the queue is empty.
     */
    bool empty (void) const {
      std::lock_guard<std::mutex> lock{m_mutex};
      return m_queue.empty();
    }

    /*
     * Return the number of elements in the queue.
     */
    size_t size (void){
      std::lock_guard<std::mutex> lock{m_mutex};
      return m_queue.size();
    }

    /*
     * Clear all items from the queue.
     */
    void clear (void) {
      std::lock_guard<std::mutex> lock{m_mutex};
      while(!m_queue.empty()) {
        m_queue.pop();
      }
      full_condition.notify_all();

      return ;
    }

    /*
     * Invalidate the queue.
     * Used to ensure no conditions are being waited on in waitPop when
     * a thread or the application is trying to exit.
     * The queue is invalid after calling this method and it is an error
     * to continue using a queue after this method has been called.
     */
    void invalidate(void) {
      std::lock_guard<std::mutex> lock{m_mutex};
      m_valid = false;
      empty_condition.notify_all();
      full_condition.notify_all();

      return ;
    }

    /*
     * Returns whether or not this queue is valid.
     */
    bool isValid(void) const {
      std::lock_guard<std::mutex> lock{m_mutex};
      return m_valid;
    }

  private:
    std::atomic_bool m_valid{true};
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable empty_condition;
    std::condition_variable full_condition;

    void internal_push (T& value);
    void internal_pop (T& out);
  };
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
