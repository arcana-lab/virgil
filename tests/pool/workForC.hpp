#pragma once
#include <iostream>
#include <math.h>

double result[0x10000] = {0};

pthread_spinlock_t locks[0x10000] = {0};

struct myFargs {
  int iters;
  uint64_t task_id;
  pthread_spinlock_t* lock;
};

void myF(void* args) {
  uint64_t task_id = ((struct myFargs*)args)->task_id;
  int iters = ((struct myFargs*)args)->iters;
  double v = static_cast<double>(iters);
  for (auto i = 0; i < iters; i++) {
    for (auto i = 0; i < iters; i++) {
      v = sqrt(v);
    }
  }

  result[task_id * 8] = v;

  free(args);

  pthread_spin_unlock(((struct myFargs*)args)->lock);

  //std::cout << "RESULT IS: " << v << '\n';
  return;
}
