// Park.h - Park Transform and Inverse Park Transform
#pragma once

struct DQ {
  float d, q;
};

struct AlphaBetaV {
  float alpha, beta;
};

// Park Transform: rotates (alpha, beta) from stationary to rotating DQ frame.
// theta = electrical rotor angle (radians)
DQ park(float alpha, float beta, float theta);

// Inverse Park: rotates (vd, vq) from rotating DQ back to stationary alpha-beta.
// theta = electrical rotor angle (radians)
AlphaBetaV inversePark(float vd, float vq, float theta);
