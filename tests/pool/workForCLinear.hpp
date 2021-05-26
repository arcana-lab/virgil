#pragma once
#include <iostream>
#include <math.h>

void myFLinear (void* iters_){
  int64_t iters = (int64_t)(iters_);
  double v = static_cast<double>(iters);
  for (auto i=0; i < iters; i++){
    v = sqrt(v);
  }

  return;
}