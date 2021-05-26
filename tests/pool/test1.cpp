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

  /*
   * Return will force the main thread to wait for the completion of all tasks submitted.
   *
   * In more detail, return will call the destructor of the vector @results, which will call the @get API on all the futures stored inside.
   * Invoking @get forces the main thread to wait for the completion of the related task submitted.
   */
  return 0;
}
