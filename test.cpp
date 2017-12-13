#include <iostream>
#include <vector>

#include "ThreadPool.hpp"

void myF (void){
  std::cerr << "RUNNING myF" << std::endl;
  while (true){

  }

  return ;
}

int main (){
  std::cerr << "Start" << std::endl;

  /*
   * Create a thread pool.
   */
  MARC::ThreadPool pool(10);

  /*
   * Submit jobs.
   */
  std::vector<MARC::ThreadPool::TaskFuture<void>> results;
  for (auto i=0; i < 10; i++){
    std::cerr << "Submitting a job" << std::endl;
    results.push_back(pool.submit(myF));
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
