#include <iostream>
#include <vector>
#include <math.h>

#include "ThreadPool.hpp"
#include "work.hpp"

int main (int argc, char *argv[]){

  /*
   * Fetch the inputs.
   */
  if (argc < 3){
    std::cerr << "USAGE: " << argv[0] << " TASKS ITERS_PER_TASK" << std::endl;
    return 1;
  }
  auto tasks = atoi(argv[1]);
  auto iters = atoi(argv[2]);

  /*
   * Submit jobs.
   */
  for (auto i=0; i < tasks; i++){
    myF(iters);
  }

  return 0;
}
