// Park.cpp - Park and Inverse Park Transform implementation
#include "Park.h"
#include <math.h>

DQ park(float alpha, float beta, float theta) {
  float s = sinf(theta);
  float c = cosf(theta);
  DQ dq;
  // Standard Park transform:
  // id =  alpha*cos(theta) + beta*sin(theta)
  // iq = -alpha*sin(theta) + beta*cos(theta)
  dq.d =  alpha * c + beta * s;
  dq.q = -alpha * s + beta * c;
  return dq;
}

AlphaBetaV inversePark(float vd, float vq, float theta) {
  float s = sinf(theta);
  float c = cosf(theta);
  AlphaBetaV ab;
  // Inverse Park transform:
  // valpha = vd*cos(theta) - vq*sin(theta)
  // vbeta  = vd*sin(theta) + vq*cos(theta)
  ab.alpha = vd * c - vq * s;
  ab.beta  = vd * s + vq * c;
  return ab;
}
