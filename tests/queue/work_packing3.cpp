#include <iostream>
#include <vector>

#include "ThreadPool.hpp"
#include "ThreadSafeLockFreeQueue.hpp"

#define PACKAGE_LENGTH 8

typedef struct {
  int64_t values[PACKAGE_LENGTH];
} package_t;

void pushFunction (int64_t pushes, MARC::ThreadSafeQueue<package_t> *queue){
  auto packageIndex = 0;

  package_t package;
  for (auto i=0; i < pushes; i++){
    package.values[packageIndex] = i;
    packageIndex++;

    if (packageIndex == PACKAGE_LENGTH){
      queue->push(package);
      packageIndex = 0;
    }
  }
  if (packageIndex != 0){
    abort();
  }

  return ;
}

void pullFunction (int64_t pushes, MARC::ThreadSafeQueue<package_t> *queue){
  int64_t finalSum = 0;
  for (auto i=0; i < pushes; i += PACKAGE_LENGTH){
    package_t tmpValues;
    queue->waitPop(tmpValues);
    for (auto packageIndex=0; packageIndex < PACKAGE_LENGTH; packageIndex++){
      finalSum += tmpValues.values[packageIndex];
    }
  }
  std::cout << finalSum << std::endl;

  return ;
}

int main (int argc, char *argv[]){

  /*
   * Fetch the inputs.
   */
  if (argc < 2){
    std::cerr << "USAGE: " << argv[0] << " NUMBER_OF_PUSHES" << std::endl;
    return 1;
  }
  auto pushes = atoll(argv[1]);

  /*
   * Create a thread pool.
   */
  MARC::ThreadPool pool(2);
  
  /*
   * Create the queue.
   */
  MARC::ThreadSafeLockFreeQueue<package_t> queue;

  /*
   * Work
   */
  std::vector<MARC::TaskFuture<void>> results;
  results.push_back(pool.submit(pushFunction, pushes, &queue));
  results.push_back(pool.submit(pullFunction, pushes, &queue));

  return 0;
}
