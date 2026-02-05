/*
 * This file is part of pocketfft.
 * Licensed under a 3-clause BSD style license - see LICENSE.md
 */

/*
 *  Test codes for pocketfft.
 *
 *  Copyright (C) 2004-2018 Max-Planck-Society
 *  \author Martin Reinecke
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pocketfft/pocketfft.h"

constexpr size_t MAX_LEN = 8'192;

static void fill_random(double *data, size_t length) {
  for (size_t m = 0; m < length; ++m)
    data[m] = rand() / (RAND_MAX + 1.0) - 0.5;
}

static double errcalc(double *data, double *odata, size_t length) {
  double sum = 0, errsum = 0;
  for (size_t m = 0; m < length; ++m) {
    errsum += (data[m] - odata[m]) * (data[m] - odata[m]);
    sum += odata[m] * odata[m];
  }
  return sqrt(errsum / sum);
}

static int test_real(void) {
  double data[MAX_LEN], odata[MAX_LEN];
  const double epsilon = 2e-15;
  auto ret = 0;
  fill_random(odata, MAX_LEN);
  double errsum = 0;
  for (size_t length = 1; length <= MAX_LEN; ++length) {
    memcpy(data, odata, length * sizeof(double));
    rfft_plan plan = make_rfft_plan(length);
    auto status = rfft_forward(plan, data, 1.);
    if (status != 0) {
      printf("forward real fft failed at length %zu with code %i\n", length,
             status);
      ret = 1;
      destroy_rfft_plan(plan);
      break;
    }
    status = rfft_backward(plan, data, 1. / length);
    if (status != 0) {
      printf("backward real fft failed at length %zu with code %i\n", length,
             status);
      ret = 1;
      destroy_rfft_plan(plan);
      break;
    }
    destroy_rfft_plan(plan);
    double err = errcalc(data, odata, length);
    if (err > epsilon) {
      printf("problem at real length %zu: %e\n", length, err);
      ret = 1;
    }
    errsum += err;
  }
  printf("errsum: %e\n", errsum);
  return ret;
}

static int test_complex(void) {
  double data[2 * MAX_LEN], odata[2 * MAX_LEN];
  fill_random(odata, 2 * MAX_LEN);
  const double epsilon = 2e-15;
  auto ret = 0;
  double errsum = 0;
  for (size_t length = 1; length <= MAX_LEN; ++length) {
    memcpy(data, odata, 2 * length * sizeof(double));
    cfft_plan plan = make_cfft_plan(length);
    auto status = cfft_forward(plan, data, 1.);
    if (status != 0) {
      printf("forward complex fft failed at length %zu with code %i\n", length,
             status);
      ret = 1;
      destroy_cfft_plan(plan);
      break;
    }
    status = cfft_backward(plan, data, 1. / length);
    if (status != 0) {
      printf("backward complex fft failed at length %zu with code %i\n", length,
             status);
      ret = 1;
      destroy_cfft_plan(plan);
      break;
    }
    destroy_cfft_plan(plan);
    double err = errcalc(data, odata, 2 * length);
    if (err > epsilon) {
      printf("problem at complex length %zu: %e\n", length, err);
      ret = 1;
    }
    errsum += err;
  }
  printf("errsum: %e\n", errsum);
  return ret;
}

int main() {
  auto ret = test_real();
  ret += test_complex();
  return ret;
}
