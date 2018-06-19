#include <iostream>
#include <vector>
#include <math.h>

#include "ThreadPool.hpp"
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
  auto threads = (std::uint32_t) atoi(argv[3]);

  /*
   * Create a thread pool.
   */
  MARC::ThreadPool pool{false, threads};

  /*
   * Submit jobs.
   */
  std::vector<MARC::TaskFuture<double>> results;
  for (auto i=0; i < tasks; i++){
    results.push_back(pool.submit(myF, iters));
  }

  return 0;
}
