#include <iostream>
#include <vector>
#include <math.h>

#include "ThreadPoolForC.hpp"
#include "workForC.hpp"
#include "workForCLinear.hpp"
#include "Architecture.hpp"
#include "Scheduler.hpp"
#include "TaskDistribution.hpp"
#include "ThreadPoolForCSingleQueue.hpp"


int main (int argc, char *argv[]){

  /*
   * Fetch the inputs.
   */
  if (argc < 4){
    std::cerr << "USAGE: " << argv[0] << " TEST TASKS MAX_ITERS THREADS" << std::endl;
    return 1;
  }
  auto tasks = atoi(argv[2]);
  auto max_iters = atoi(argv[3]);
  auto threads = atoi(argv[4]);

  /*
   * Create the scheduler.
   */
  MARC::ThreadPoolForCMultiQueues pool(false, std::thread::hardware_concurrency());
  const MARC::Architecture arch;
  MARC::Scheduler scheduler(pool, arch);

  srand(0);

  /*
   * Get a distribution of iters for every task
   */ 
  std::vector<std::uint32_t> iterDistribution;
  switch (atoi(argv[1])) {
    case 0: {
      iterDistribution = getHomogeneousDistribution(tasks, max_iters / 2);
      break;
    }
    case 1: {
      iterDistribution = getUniformDistribution(tasks, max_iters);
      break;
    }
    case 2: {
      iterDistribution = getBimodalDistribution(tasks, max_iters * 1/4, max_iters * 3/4);
      break;
    }
    case 3: {
      iterDistribution = getNormalDistribution(tasks, max_iters / 2, max_iters / 5, max_iters);
      break;
    }
  }

  /*
   * Submit jobs with weight given by distribution
   */
  for (auto i=0; i < tasks; i++){
    auto iters = iterDistribution[i];
    scheduler.submitAndDetach(myF, (void*)iters, iters, 0);
    // pool.submitAndDetach(myF, (void*)iters);
  }

  return 0;
}
