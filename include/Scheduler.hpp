#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <iomanip>

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

  void printWorkHistories();

private:
  /// @param weight The weight of the task to submit
  /// @return the most ideal pu to submit a new task to given a task weight
  size_t find_best_pu(task_weight_t weight);

  struct PUWorkHistory {
    PU* pu;
    task_weight_t accumulated_work;
  };

  std::vector<PUWorkHistory> history_;
  std::vector<PUWorkHistory> trueHistory;

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
    trueHistory.emplace_back(PUWorkHistory{pu, 0});
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
  //std::cout << "SUBMITTING TASK WITH WEIGHT: " << weight << '\n';

  int i = 0;
  int j;

  for (auto& history_element : history_) {
    // std::cout << "PU" << history_element.pu->get_id() << " "
    //           << history_element.accumulated_work << " / " << lowest_work
    //           << "\n";
    auto total_work = history_element.accumulated_work + (weight * arch_.max_pu_strength / history_element.pu->get_power());
    //std::cout << std::setw(15) << "PU " << history_element.pu->get_id() << std::setw(25) << "CURR WORK: " << history_element.accumulated_work; 
    //std::cout << std::setw(25) << " TOTAL WORK : " << total_work  << std::setw(25) << '\n';
    if (total_work < lowest_work) {
      lowest_work = total_work;
      best_pu = history_element.pu->get_id();
      best_hist = &history_element;
      j = i;
    }
    i++;
  }
  best_hist->accumulated_work +=
      weight * arch_.max_pu_strength / best_hist->pu->get_power();

  //std::cout << "I am sending a task with weight : " << weight << " to : " << best_pu << "\n\n";
  //trueHistory[j].accumulated_work += weight;
  //return best_pu;

  static uint64_t last_cpu = 20;
  if (last_cpu == 28) {
    last_cpu = 20;
  }

  uint64_t ret = last_cpu;
  last_cpu += 2;
  //std::cout << "I am sending a task with weight : " << weight << " to : " << ret << "\n\n";
  trueHistory[(ret - 20)/2].accumulated_work += weight;
  return ret;
}

void Scheduler::printWorkHistories() {
  for (auto& history_element : trueHistory) {
    std::cout << "PU #" << history_element.pu->get_id() << " : "  << history_element.accumulated_work << '\n';
  }
}
} // namespace MARC