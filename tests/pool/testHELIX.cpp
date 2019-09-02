#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <math.h>
#include <assert.h>

#include "ThreadPool.hpp"

#define CACHE_LINE_SIZE 64
//#define BASELINE
//#define PRINT

static pthread_spinlock_t globalArray;

static std::uint32_t pauses;

double work0 (double v, uint64_t innerIters){
  for (auto j=0; j < innerIters; j++){
    v = sqrt(v);
  }
  return v;
}

double parallelizedLoopBaseline (uint64_t startIV, uint64_t iters, volatile double v, uint64_t innerIters){
  auto v0 = v;
  auto v1 = v;
  for (auto i=startIV; i < iters; i++){
    v0 = work0(v0, innerIters);
    v1 = work0(v1, innerIters);
  }

  return v0 + v1;
}

void parallelizedLoop (void *ssArrayPast, void *ssArrayFuture, uint64_t startIV, uint64_t iters, uint64_t threads, volatile uint64_t *theLoopIsOver, double v[], uint64_t innerIters, std::uint32_t numberOfSequentialSegments){

  /*
   * Compute pointers to spinlocks.
   */
  auto preds = new pthread_spinlock_t *[numberOfSequentialSegments];
  auto succs = new pthread_spinlock_t *[numberOfSequentialSegments];
  for (auto ssID = 0; ssID < numberOfSequentialSegments; ssID++){
    preds[ssID] = (pthread_spinlock_t *)(std::uint64_t(ssArrayPast) + (ssID * CACHE_LINE_SIZE));
    succs[ssID] = (pthread_spinlock_t *)(std::uint64_t(ssArrayFuture) + (ssID * CACHE_LINE_SIZE));
  }
  #ifdef PRINT
  pthread_spin_lock(&globalArray);
  std::cerr << "Core " << startIV << ": Start" << std::endl;
  for (auto ssID = 0; ssID < numberOfSequentialSegments; ssID++){
    std::cerr << "Core " << startIV << ":   SS" << ssID << ": pred \"" << (void *)preds[ssID] << "\" succ \"" << (void *)succs[ssID] << "\"" << std::endl;
  }
  pthread_spin_unlock(&globalArray);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  #endif

  for (auto i=startIV; i < iters; i += threads){
    for (auto ssID = 0 ; ssID < numberOfSequentialSegments; ssID++){

      /*
       * Parallel segment.
       */
      v[ssID] = work0(v[ssID], innerIters);

      /*
       * Sequential segment.
       */
      pthread_spin_lock(preds[ssID]);
      #ifdef PRINT
      pthread_spin_lock(&globalArray);
      std::cerr << "Core " << startIV << ": " << i << " SS0: " << *((uint64_t *)pred0) << " " << *((uint64_t *)succ0) << std::endl;
      pthread_spin_unlock(&globalArray);
      #endif
      pthread_spin_unlock(succs[ssID]);
    }
  }
  (*theLoopIsOver) = 1;

  #ifdef PRINT
  pthread_spin_lock(&globalArray);
  std::cerr << "Core " << startIV << ": end" << std::endl;
  pthread_spin_unlock(&globalArray);
  #endif 

  return ;
}

void HELIX_helperThread (void *ssArray, std::uint32_t numOfsequentialSegments, volatile uint64_t *theLoopIsOver){
  #ifdef PRINT
  pthread_spin_lock(&globalArray);
  std::cerr << "Helper thread " << ssArray << ": start" << std::endl;
  pthread_spin_unlock(&globalArray);
  #endif

  while ((*theLoopIsOver) == 0){

    /*
     * Prefetch all sequential segment cache lines of the current loop iteration.
     */
    for (auto i = 0 ; ((*theLoopIsOver) == 0) && (i < numOfsequentialSegments); i++){

      /*
       * Fetch the pointer.
       */
      auto ptr = (uint64_t *)(((uint64_t)ssArray) + (i * CACHE_LINE_SIZE));

      /*
       * Prefetch the cache line for the current sequential segment.
       */
      while (((*theLoopIsOver) == 0) && ((*ptr) == 0)) {
        //std::this_thread::sleep_for(std::chrono::milliseconds(pauses));
        for (auto j=0; j < pauses; j++){
          _mm_pause();
          if (*theLoopIsOver) break ;
        }
      }
    }
  }

  #ifdef PRINT
  pthread_spin_lock(&globalArray);
  std::cerr << "Helper thread " << ssArray << ": end" << std::endl;
  pthread_spin_unlock(&globalArray);
  #endif

  return ;
} 

int main (int argc, char *argv[]){

  /*
   * Fetch the inputs.
   */
  if (argc < 5){
    std::cerr << "USAGE: " << argv[0] << " ITERS THREADS NUMBER_OF_SSS PAUSES_HELPER_THREADS" << std::endl;
    return 1;
  }
  auto iters = atoi(argv[1]);
  auto threads = (std::uint32_t) atoi(argv[2]);
  auto numOfsequentialSegments = std::uint32_t(atoi(argv[3]));
  pauses = (std::uint32_t) atoi(argv[4]);

  #ifdef BASELINE
  std::cerr << parallelizedLoopBaseline(0, iters, 3.5432, 100);
  return 0;
  #endif

  /*
   * Prepare the memory.
   */
  auto numOfSSArrays = threads;
  void *ssArrays = NULL;
  auto ssSize = CACHE_LINE_SIZE;
  auto ssArraySize = ssSize * numOfsequentialSegments;
  posix_memalign(&ssArrays, CACHE_LINE_SIZE, ssArraySize *  numOfSSArrays);

  /*
   * Initialize the sequential segment arrays.
   */
  for (auto i = 0; i < numOfSSArrays; i++){

    /*
     * Fetch the current sequential segment array.
     */
    auto ssArray = (void *)(((uint64_t)ssArrays) + (i * ssArraySize));

    /*
     * Initialize the locks.
     */
    for (auto lockID = 0; lockID < numOfsequentialSegments; lockID++){

      /*
       * Fetch the pointer to the current lock.
       */
      auto lock = (pthread_spinlock_t *)(((uint64_t)ssArray) + (lockID * ssSize));

      /*
       * Initialize the lock.
       */
      pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);

      /*
       * If the sequential segment is not for core 0, then we need to lock it.
       */
      if (i > 0){
        #ifdef PRINT
        std::cerr << "Lock " << (void *)(lock) << std::endl;
        #endif
        pthread_spin_lock(lock);
      }
    }
  }
  pthread_spin_init(&globalArray, PTHREAD_PROCESS_PRIVATE);

  /*
   * Create a thread pool.
   */
  MARC::ThreadPool pool{true, std::thread::hardware_concurrency()};

  /*
   * Set the initial values.
   */
  auto values = new double[numOfsequentialSegments];
  for (auto i=0; i < numOfsequentialSegments; i++){
    values[i] = 3.4514 * (rand() % 10);
  }

  /*
   * Submit jobs.
   */
  volatile uint64_t loopIsOverFlag = 0;
  cpu_set_t cores;
  std::vector<MARC::TaskFuture<void>> results;
  for (auto i=0; i < threads; i++){

    /*
     * Identify the past and future sequential segment arrays.
     */
    auto pastID = i;
    auto futureID = (i + 1) % threads;

    /*
     * Fetch the sequential segment array for the current thread.
     */
    auto ssArrayPast = (void *)(((uint64_t)ssArrays) + (pastID * ssArraySize));
    auto ssArrayFuture = (void *)(((uint64_t)ssArrays) + (futureID * ssArraySize));
    assert(ssArrayPast != ssArrayFuture);

    /*
     * Set the affinity for both the thread and its helper.
     */
    CPU_ZERO(&cores);
    auto physicalCore = i * 2;
    CPU_SET(physicalCore, &cores);

    /*
     * Launch the working thread.
     */
    results.push_back(pool.submitToCores(
      cores,
      parallelizedLoop,
      ssArrayPast, ssArrayFuture,
      (uint64_t)i, (uint64_t)iters, (uint64_t)threads,
      &loopIsOverFlag,
      values,
      100,
      numOfsequentialSegments
    ));

    /*
     * Launch the helper thread.
     */
    continue ;
    CPU_SET(physicalCore + 1, &cores);
    /*results.push_back(pool.submitToCores(
      cores,
      HELIX_helperThread, 
      ssArrayPast,
      numOfsequentialSegments,
      &loopIsOverFlag
    ));*/
  }

  return 0;
}
