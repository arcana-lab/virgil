#include <iostream>
#include <vector>
#include <math.h>

#include "ThreadPool.hpp"

double myF (std::int64_t iters){
  double v = static_cast<double>(iters);
  for (auto i=0; i < iters; i++){
    for (auto i=0; i < iters; i++){
      for (auto i=0; i < iters; i++){
        v = sqrt(v);
      }
    }
  }

  return v;
}

int main (int argc, char *argv[]){

  /*
   * Create a thread pool.
   */
  MARC::ThreadPool pool(10);

  /*
   * Submit jobs.
   */
  std::cerr << "Start" << std::endl;
  std::vector<MARC::ThreadPool::TaskFuture<double>> results;
  for (auto i=0; i < 10; i++){
    std::cerr << "Submitting a job" << std::endl;
    results.push_back(pool.submit(myF, 100));
  }

  /*
   * Wait.
   */
  for (auto& result : results){
    std::cerr << "Wait for the job to be over" << std::endl;
    result.get();
  }
  std::cerr << "Exit" << std::endl;
  
  return 0;
}
