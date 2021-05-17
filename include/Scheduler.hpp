#pragma once

#include <cstdint>
#include <cstddef>
// #include <optional>

#include "ThreadPoolForC.hpp"
#include "Architecture.hpp"

typedef size_t task_weight_t;

namespace MARC {
class ThreadPoolForC;
class Architecture;

class Scheduler {
public:
  Scheduler(MARC::ThreadPoolForC& pool, const Architecture& arch);

  /// Submit a C function for execution on the most appropriate core.
  /// @param f Function to execute
  /// @param args Args to f
  /// @param weight Estimate for the execution time (unitless, just needs to be
  /// relatively consistent)
  /// @param locality_island ID corresponding to a group of tasks with similar
  /// locality
  /// @return The CPU ID which the task was dispatched to
  size_t submitAndDetach(void (*f)(void*), void* args, task_weight_t weight,
                           size_t locality_island);

private:
  /// @param weight The weight of the task to submit
  /// @return the most ideal core to submit a new task to given a task weight
  size_t find_best_core(task_weight_t weight);

  /// Architecture
  const Architecture& arch_;

  /// VIRGIL ThreadPool
  MARC::ThreadPoolForC& pool_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Implementation below here */
////////////////////////////////////////////////////////////////////////////////////////////////////////////

Scheduler::Scheduler(MARC::ThreadPoolForC& pool, const Architecture& arch)
    : arch_(arch)
    , pool_(pool) {}

size_t Scheduler::submitAndDetach(void (*f)(void*), void* args,
                                    task_weight_t weight,
                                    size_t locality_island) {

  const size_t best_core = find_best_core(weight);
  pool_.submitToCore(best_core, f, args);
  return best_core;
}

#include <iostream>

size_t Scheduler::find_best_core(task_weight_t weight) {
  static size_t last_cpu = 0;
  last_cpu += 1;
  last_cpu %= arch_.num_pus();
  return last_cpu;
}
} // namespace MARC