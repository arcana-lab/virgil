#include <iostream>
#include <vector>
#include <math.h>

#include "ThreadPools.hpp"
#include "work.hpp"

int main (int argc, char *argv[]){

  /*
   * Fetch the inputs.
   */
  if (argc < 4){
    std::cerr << "USAGE: " << argv[0] << " TASKS ITERS_PER_TASK THREADS" << std::endl;
    return 1;
  }
  auto tasks = atoi(argv[1]);
  auto iters = atoi(argv[2]);
  auto threads = atoi(argv[3]);

  /*
   * Create a thread pool.
   */
  arcana::virgil::ThreadPool pool(threads);

  /*
   * Submit jobs.
   */
  for (auto i=0; i < tasks; i++){
    pool.submitAndDetach(myF, iters);
  }

  return 0;
}
