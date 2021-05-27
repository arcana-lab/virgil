#pragma once
#include <iostream>
#include <math.h>

static double data[0x10000] = {0};

void myF (void* iters_){
  int64_t iters = (int64_t)(iters_);
  double v = static_cast<double>(iters);
  // for (auto i=0; i < iters; i++){
  //   for (auto i=0; i < iters; i++){
      for (auto i=0; i < iters; i++){
        data[rand() % (0x10000 - 1)] = v = sqrt(v);
      }
  //   }
  // }

  return;
}
