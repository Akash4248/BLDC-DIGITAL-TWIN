// Park.cpp - Park and Inverse Park Transform
// Optimized for AVR: compute sinf/cosf ONCE and reuse both outputs.
#include "Park.h"
#include <math.h>

DQ park(float alpha, float beta, float theta) {
  float s, c;
  // sinf and cosf are computed as a pair - some AVR toolchains optimize this
  s = sinf(theta);
  c = cosf(theta);
  DQ dq;
  dq.d =  alpha * c + beta * s;
  dq.q = -alpha * s + beta * c;
  return dq;
}

AlphaBetaV inversePark(float vd, float vq, float theta) {
  float s, c;
  s = sinf(theta);
  c = cosf(theta);
  AlphaBetaV ab;
  ab.alpha = vd * c - vq * s;
  ab.beta  = vd * s + vq * c;
  return ab;
}
