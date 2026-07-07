// Clarke.cpp - Clarke Transform implementation
#include "Clarke.h"

AlphaBeta clarke(float ia, float ib, float ic) {
  (void)ic; // ic is not needed when ia + ib + ic = 0
  AlphaBeta ab;
  // Standard amplitude-invariant Clarke transform:
  // ialpha = ia
  // ibeta  = (ia + 2*ib) / sqrt(3)
  ab.alpha = ia;
  ab.beta  = (ia + 2.0f * ib) * 0.57735026919f; // 1/sqrt(3)
  return ab;
}
