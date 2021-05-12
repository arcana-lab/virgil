#pragma once

#include <cstdint>

typedef std::size_t task_weight_t;

namespace MARC {
class ThreadPoolForC;

class Architechture;

class Scheduler {
public:
  Scheduler(MARC::ThreadPoolForC& pool, const Architechture& arch);

  /// Submit a C function for execution on the most appropriate core.
  /// @param f Function to execute
  /// @param args Args to f
  /// @param weight Estimate for the execution time (unitless, just needs to be
  /// relatively consistent)
  /// @param locality_island ID corresponding to a group of tasks with similar
  /// locality
  /// @return The CPU ID which the task was dispatched to
  std::size_t submitAndDetach(void (*f)(void*), void* args, task_weight_t weight,
                           std::size_t locality_island);

private:
  /// @param weight The weight of the task to submit
  /// @return the most ideal core to submit a new task to given a task weight
  std::size_t find_best_core(task_weight_t weight);

  /// Architechture
  const Architechture& arch_;

  /// VIRGIL ThreadPool
  MARC::ThreadPoolForC& pool_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Implementation below here */
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ThreadPoolForC.hpp"
#include "Architechture.hpp"

Scheduler::Scheduler(MARC::ThreadPoolForC& pool, const Architechture& arch)
    : arch_(arch)
    , pool_(pool) {}

std::size_t Scheduler::submitAndDetach(void (*f)(void*), void* args,
                                    task_weight_t weight,
                                    std::size_t locality_island) {

  const std::size_t best_core = find_best_core(weight);
  pool_.submitToCore(best_core, f, args);
}

std::size_t Scheduler::find_best_core(task_weight_t weight) {
  static std::size_t last_cpu = 0;
  last_cpu %= arch_.num_cpus();
  return last_cpu;
}
} // namespace MARC