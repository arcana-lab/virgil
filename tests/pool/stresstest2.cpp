#include <iostream>
#include <vector>
#include <math.h>
#include <pthread.h>

#include "ThreadPools.hpp"
#include "work.hpp"

int main (int argc, char *argv[]){

  /*
   * Fetch the inputs.
   */
  if (argc < 4){
    std::cerr << "USAGE: " << argv[0] << " TASKS OUTERITERS THREADS" << std::endl;
    return 1;
  }
  auto tasks = atoi(argv[1]);
  auto outerIters = atoi(argv[2]);
  auto threads = (std::uint32_t) atoi(argv[3]);

  /*
   * Create a thread pool.
   */
  arcana::virgil::ThreadPoolForCMultiQueues pool{false, threads};

  /*
   * Create the locks
   */
  auto locks = new pthread_spinlock_t[tasks];
  for (auto i=0; i < tasks; i++){
    auto &lock = locks[i];
    pthread_spin_init(&lock, 0);
    pthread_spin_lock(&lock);
  }

  /*
   * Stress test
   */
  for (auto j=0; j < outerIters; j++){

    /*
     * Submit the tasks.
     */
    for (auto i=0; i < tasks; i++){
      auto &lock = locks[i];
      pool.submitAndDetach(myFInC, (void *)&lock);
    }

    /*
     * Wait for the tasks
     */
    for (auto i=0; i < tasks; i++){
      auto &lock = locks[i];
      pthread_spin_lock(&lock);
    }
  }

  return 0;
}
