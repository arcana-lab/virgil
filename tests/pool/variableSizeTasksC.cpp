#include <iostream>
#include <vector>
#include <math.h>

#include "ThreadPoolForC.hpp"
#include "workForC.hpp"
#include "workForCLinear.hpp"
#include "Architecture.hpp"
#include "Scheduler.hpp"


int main (int argc, char *argv[]){

  /*
   * Fetch the inputs.
   */
  if (argc < 4){
    std::cerr << "USAGE: " << argv[0] << " TASKS MAX_ITERS THREADS" << std::endl;
    return 1;
  }
  auto tasks = atoi(argv[1]);
  auto max_iters = atoi(argv[2]);
  auto threads = atoi(argv[3]);

  /*
   * Create the scheduler.
   */
  MARC::ThreadPoolForC pool(threads);
  const MARC::Architecture arch;
  MARC::Scheduler scheduler(pool, arch);

  /*
   * Submit jobs.
   * Number of iterations per task is *almost* uniformly chosen from [1, MAX_ITERS]
   */
  for (auto i=0; i < tasks; i++){
    auto iters = rand() % max_iters + 1;
    scheduler.submitAndDetach(myF, (void*)iters, iters*iters*iters, 0);
  }

  return 0;
}
