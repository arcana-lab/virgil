#pragma once

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
