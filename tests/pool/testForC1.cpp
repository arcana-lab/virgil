#include <iostream>
#include <vector>
#include <math.h>

#include "ThreadPoolForC.hpp"
#include "workForC.hpp"

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
  MARC::ThreadPoolForC pool(false, threads);

  /*
   * Submit jobs.
   */
  for (auto i=0; i < tasks; i++){
    pool.submitAndDetach(myF, (void*)iters);
  }
  
  auto workload = pool.getWorkloads(); 
  for (int i = 0; i < workload.size(); i++) {
    std::cout << "WEIGHT OF QUEUE " << i << " : "  << workload[i] << '\n';
  }

  return 0;
}

