// Clarke.h - Clarke Transform (3-phase to 2-phase stationary frame)
#pragma once

struct AlphaBeta {
  float alpha, beta;
};

// Performs the amplitude-invariant Clarke transform.
// Converts 3-phase currents (ia, ib, ic) to the 2-axis (alpha-beta) frame.
// Assumes ia + ib + ic = 0, so ic is redundant and not used.
AlphaBeta clarke(float ia, float ib, float ic);
