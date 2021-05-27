#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
// #include <optional>

#include "Architecture.hpp"
#include "ThreadPoolForCMultiQueues.hpp"

typedef size_t task_weight_t;

namespace MARC {
class ThreadPoolForC;
class Architecture;

class Scheduler {
public:
  Scheduler(MARC::ThreadPoolForCMultiQueues& pool, const Architecture& arch);

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
  /// @return the most ideal pu to submit a new task to given a task weight
  size_t find_best_pu(task_weight_t weight);

  struct PUWorkHistory {
    PU* pu;
    task_weight_t accumulated_work;
  };

  std::vector<PUWorkHistory> history_;

  /// Architecture
  const Architecture& arch_;

  /// VIRGIL ThreadPool
  MARC::ThreadPoolForCMultiQueues& pool_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Implementation below here */
////////////////////////////////////////////////////////////////////////////////////////////////////////////

Scheduler::Scheduler(MARC::ThreadPoolForCMultiQueues& pool,
                     const Architecture& arch)
    : arch_(arch)
    , pool_(pool) {

  // Initialize the work history
  for (auto* pu : arch_.get_pus()) {
    history_.emplace_back(PUWorkHistory{pu, 0});
  }
}

size_t Scheduler::submitAndDetach(void (*f)(void*), void* args,
                                  task_weight_t weight,
                                  size_t locality_island) {

  const size_t best_core = find_best_pu(
      1000 * weight); /* Multiply by 1000 to allow more granularity */
  pool_.submitAndDetach(f, args, best_core);
  return best_core;
}

#include <iostream>

size_t Scheduler::find_best_pu(task_weight_t weight) {
  PUWorkHistory* best_hist = nullptr;
  pu_id_t best_pu = 0;
  task_weight_t lowest_work = std::numeric_limits<task_weight_t>::max();

  for (auto& history_element : history_) {
    // std::cout << "PU" << history_element.pu->get_id() << " "
    //           << history_element.accumulated_work << " / " << lowest_work
    //           << "\n";
    if (history_element.accumulated_work < lowest_work) {
      lowest_work = history_element.accumulated_work;
      best_pu = history_element.pu->get_id();
      best_hist = &history_element;
    }
  }
  best_hist->accumulated_work +=
      weight * arch_.max_pu_strength / best_hist->pu->get_power();
  // std::cout << "I am sending a task to pu " << best_pu << "\n";
  return best_pu;

  static uint64_t last_cpu = 20;
  if (last_cpu == 28) {
    last_cpu = 20;
  }

  uint64_t ret = last_cpu;
  last_cpu += 2;
  return ret;
}
} // namespace MARC