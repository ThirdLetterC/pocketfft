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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pocketfft/pocketfft.h"

constexpr size_t MAX_LEN = 8'192;

static void fill_pattern(double *data, size_t length) {
  for (size_t m = 0; m < length; ++m) {
    double x = (double)(m + 1);
    data[m] = sin(0.125 * x) + cos(0.375 * x);
  }
}

[[nodiscard]] static bool arrays_match(const double *lhs, const double *rhs,
                                       size_t length) {
  return memcmp(lhs, rhs, length * sizeof(*lhs)) == 0;
}

static double errcalc(double *data, double *odata, size_t length) {
  double sum = 0, errsum = 0;
  for (size_t m = 0; m < length; ++m) {
    errsum += (data[m] - odata[m]) * (data[m] - odata[m]);
    sum += odata[m] * odata[m];
  }
  if (sum == 0) return errsum == 0 ? 0 : INFINITY;
  return sqrt(errsum / sum);
}

static int test_api_contract(void) {
  auto ret = 0;
  double data[4] = {1., 2., 3., 4.};
  constexpr double original_real_data[4] = {1., 2., 3., 4.};
  constexpr double original_complex_data[4] = {1., 2., 3., 4.};

  if (make_rfft_plan(0) != nullptr) {
    printf("make_rfft_plan(0) did not fail closed\n");
    ret = 1;
  }
  if (make_cfft_plan(0) != nullptr) {
    printf("make_cfft_plan(0) did not fail closed\n");
    ret = 1;
  }
  if (make_rfft_plan(SIZE_MAX) != nullptr) {
    printf("make_rfft_plan(SIZE_MAX) did not fail closed\n");
    ret = 1;
  }
  if (make_cfft_plan(SIZE_MAX) != nullptr) {
    printf("make_cfft_plan(SIZE_MAX) did not fail closed\n");
    ret = 1;
  }
  if (rfft_forward(nullptr, data, 1.) != -1) {
    printf("rfft_forward(nullptr, ...) did not reject null plan\n");
    ret = 1;
  }
  if (rfft_backward(nullptr, data, 1.) != -1) {
    printf("rfft_backward(nullptr, ...) did not reject null plan\n");
    ret = 1;
  }
  if (cfft_forward(nullptr, data, 1.) != -1) {
    printf("cfft_forward(nullptr, ...) did not reject null plan\n");
    ret = 1;
  }
  if (cfft_backward(nullptr, data, 1.) != -1) {
    printf("cfft_backward(nullptr, ...) did not reject null plan\n");
    ret = 1;
  }
  if (rfft_length(nullptr) != 0) {
    printf("rfft_length(nullptr) did not return 0\n");
    ret = 1;
  }
  if (cfft_length(nullptr) != 0) {
    printf("cfft_length(nullptr) did not return 0\n");
    ret = 1;
  }

  auto real_plan = make_rfft_plan(4);
  auto complex_plan = make_cfft_plan(2);
  if ((real_plan == nullptr) || (complex_plan == nullptr)) {
    printf("failed to create API contract test plans\n");
    destroy_rfft_plan(real_plan);
    destroy_cfft_plan(complex_plan);
    return 1;
  }
  if (rfft_forward(real_plan, nullptr, 1.) != -1) {
    printf("rfft_forward(plan, nullptr, ...) did not reject null data\n");
    ret = 1;
  }
  if (rfft_backward(real_plan, nullptr, 1.) != -1) {
    printf("rfft_backward(plan, nullptr, ...) did not reject null data\n");
    ret = 1;
  }
  if (cfft_forward(complex_plan, nullptr, 1.) != -1) {
    printf("cfft_forward(plan, nullptr, ...) did not reject null data\n");
    ret = 1;
  }
  if (cfft_backward(complex_plan, nullptr, 1.) != -1) {
    printf("cfft_backward(plan, nullptr, ...) did not reject null data\n");
    ret = 1;
  }
  memcpy(data, original_real_data, sizeof(data));
  if (rfft_forward(real_plan, data, INFINITY) != -1) {
    printf("rfft_forward(plan, ..., +Inf) did not reject non-finite scale\n");
    ret = 1;
  }
  if (!arrays_match(data, original_real_data, 4)) {
    printf("rfft_forward(plan, ..., +Inf) mutated data on rejection\n");
    ret = 1;
  }
  memcpy(data, original_real_data, sizeof(data));
  if (rfft_backward(real_plan, data, NAN) != -1) {
    printf("rfft_backward(plan, ..., NaN) did not reject non-finite scale\n");
    ret = 1;
  }
  if (!arrays_match(data, original_real_data, 4)) {
    printf("rfft_backward(plan, ..., NaN) mutated data on rejection\n");
    ret = 1;
  }
  memcpy(data, original_complex_data, sizeof(data));
  if (cfft_forward(complex_plan, data, -INFINITY) != -1) {
    printf("cfft_forward(plan, ..., -Inf) did not reject non-finite scale\n");
    ret = 1;
  }
  if (!arrays_match(data, original_complex_data, 4)) {
    printf("cfft_forward(plan, ..., -Inf) mutated data on rejection\n");
    ret = 1;
  }
  memcpy(data, original_complex_data, sizeof(data));
  if (cfft_backward(complex_plan, data, NAN) != -1) {
    printf("cfft_backward(plan, ..., NaN) did not reject non-finite scale\n");
    ret = 1;
  }
  if (!arrays_match(data, original_complex_data, 4)) {
    printf("cfft_backward(plan, ..., NaN) mutated data on rejection\n");
    ret = 1;
  }

  destroy_rfft_plan(real_plan);
  destroy_cfft_plan(complex_plan);
  destroy_rfft_plan(nullptr);
  destroy_cfft_plan(nullptr);
  return ret;
}

static int test_real(void) {
  double data[MAX_LEN], odata[MAX_LEN];
  const double epsilon = 2e-15;
  auto ret = 0;
  fill_pattern(odata, MAX_LEN);
  double errsum = 0;
  for (size_t length = 1; length <= MAX_LEN; ++length) {
    memcpy(data, odata, length * sizeof(double));
    rfft_plan plan = make_rfft_plan(length);
    if (plan == nullptr) {
      printf("failed to create real fft plan at length %zu\n", length);
      ret = 1;
      break;
    }
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
  fill_pattern(odata, 2 * MAX_LEN);
  const double epsilon = 2e-15;
  auto ret = 0;
  double errsum = 0;
  for (size_t length = 1; length <= MAX_LEN; ++length) {
    memcpy(data, odata, 2 * length * sizeof(double));
    cfft_plan plan = make_cfft_plan(length);
    if (plan == nullptr) {
      printf("failed to create complex fft plan at length %zu\n", length);
      ret = 1;
      break;
    }
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
  auto ret = test_api_contract();
  ret += test_real();
  ret += test_complex();
  return ret;
}
