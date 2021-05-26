#pragma once

#include <pthread.h>
#include <math.h>

double myF (std::int64_t iters){
  double v = static_cast<double>(iters);
  for (auto i=0; i < iters; i++){
    for (auto i=0; i < iters; i++){
      for (auto i=0; i < iters; i++){
        v = sqrt(v);
      }
    }
  }

  return v;
}

void myFInC (void *d){
  pthread_spinlock_t *s = (pthread_spinlock_t *) d;
  pthread_spin_unlock(s);

  return ;
}
