#include <iostream>
#include <vector>
#include <math.h>

#include "ThreadPoolForC.hpp"
#include "workForC.hpp"
#include "workForCLinear.hpp"
#include "Architecture.hpp"
#include "Scheduler.hpp"
#include "TaskDistribution.hpp"


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
   * Get a distribution of iters for every task
   */ 

  std::vector<std::uint32_t> iterDistribution = getUniformDistribution(tasks, max_iters);

  /*
   * Submit jobs with weight given by distribution
   */
  for (auto i=0; i < tasks; i++){
    auto iters = iterDistribution[i];
    scheduler.submitAndDetach(myF, (void*)iters, iters*iters*iters, 0);
  }

  return 0;
}
