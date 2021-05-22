/*
 * Copyright 2020 - 2021  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadCTask class.
 */
#pragma once

#include <sched.h>
#include <pthread.h>
#include <iostream>

#include <ThreadTask.hpp>

namespace MARC {

  /*
   * An implementation of the thread task interface for C functions
   */
  class ThreadCTask: public IThreadTask {
    public:

      /*
       * Constructors.
       */
      ThreadCTask (
        uint64_t ID
        );

      ThreadCTask (
        uint64_t ID,
        void (*f) (void *args),
        void *args
        );

      ThreadCTask (
        uint64_t ID,
        cpu_set_t coresToUse, 
        void (*f) (void *args),
        void *args
        );

      /*
       * Default moving operation.
       */
      ThreadCTask (ThreadCTask&& other) = default;
      ThreadCTask& operator= (ThreadCTask&& other) = default;

      /*
       * Not copyable.
       */
      ThreadCTask (const ThreadCTask& rhs) = delete;
      ThreadCTask& operator= (const ThreadCTask& rhs) = delete;

      /*
       * Default deconstructor.
       */
      ~ThreadCTask(void) override = default;

      /*
       * Run the task.
       */
      void execute (void) override ;

      std::uint64_t getID (void) const ;

      void setFunction (void (*f) (void *args), void *args);

      bool getAvailability();
      void setAvailable();

    private:
      void (*m_func) (void *args);
      void *args;
      cpu_set_t cores;
      bool useAffinity;
      bool available;
      uint64_t ID;
      mutable pthread_spinlock_t availabilityLock;
  };

}

MARC::ThreadCTask::ThreadCTask (uint64_t ID)
  : useAffinity{false}
  , ID{ID}
  , available{false}
{
  pthread_spin_init(&this->availabilityLock, 0);
  return ;
}

MARC::ThreadCTask::ThreadCTask (
  uint64_t ID,
  void (*f) (void *args),
  void *args
  )
  :
    m_func{f}
  , args{args}
  , useAffinity{false}
  , ID{ID}
  , available{false}
  {

  pthread_spin_init(&this->availabilityLock, 0);
  return ;
}

MARC::ThreadCTask::ThreadCTask (
  uint64_t ID,
  cpu_set_t coresToUse, 
  void (*f) (void *args),
  void *args
  )
  :
  m_func{f},
  args{args},
  cores{coresToUse},
  useAffinity{true}
  , ID{ID}
  , available{false}
  {
  pthread_spin_init(&this->availabilityLock, 0);
  return ;
}

void MARC::ThreadCTask::execute (void){

  /*
   * Check if we have been asked to set the affinity of the thread that will run the task.
   */
  if (this->useAffinity){

    /*
     * Set the thread affinity.
     */
    auto self = pthread_self();
    auto exitCode = pthread_setaffinity_np(self, sizeof(cpu_set_t), &(this->cores));
    if (exitCode != 0) {
      std::cerr << "ThreadPool: Error calling pthread_setaffinity_np: " << exitCode << std::endl;
      abort();
    }
  }

  /*
   * Run
   */
  (*this->m_func)(this->args);

  return ;
}

bool MARC::ThreadCTask::getAvailability() {
  auto ret = false;

  if(available) {
    pthread_spin_lock(&this->availabilityLock);
    if(available) {
      available = false;
      ret = true;
    }
    pthread_spin_unlock(&this->availabilityLock);
  }

  return ret;
}

void MARC::ThreadCTask::setAvailable() {
  pthread_spin_lock(&this->availabilityLock);
  available = true;
  pthread_spin_unlock(&this->availabilityLock);
}

std::uint64_t MARC::ThreadCTask::getID (void) const {
  return this->ID;
}
      
void MARC::ThreadCTask::setFunction (void (*f) (void *args), void *args){
  this->m_func = f;
  this->args = args;
}
