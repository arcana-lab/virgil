#include <iostream>
#include <vector>

#include "ThreadPool.hpp"
#include "ThreadSafeSpinLockQueue.hpp"

void pushFunction (int64_t pushes, MARC::ThreadSafeSpinLockQueue<int64_t> *queue){
  for (auto i=0; i < pushes; i++){
    queue->push(i);
  }

  return ;
}

void pullFunction (int64_t pushes, MARC::ThreadSafeSpinLockQueue<int64_t> *queue){
  int64_t finalSum = 0;
  for (auto i=0; i < pushes; i++){
    int64_t tmpValue;
    queue->waitPop(tmpValue);
    finalSum += tmpValue;
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
  MARC::ThreadSafeSpinLockQueue<int64_t> queue;

  /*
   * Work
   */
  std::vector<MARC::TaskFuture<void>> results;
  results.push_back(pool.submit(pushFunction, pushes, &queue));
  results.push_back(pool.submit(pullFunction, pushes, &queue));

  return 0;
}
