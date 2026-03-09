#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pocketfft/pocketfft.h"

static int failures = 0;
static constexpr size_t MAX_TEST_LENGTH = 97;

static void expect_true(bool condition, char const *expression,
                        char const *test_name) {
  if (condition) return;
  fprintf(stderr, "FAIL [%s]: %s\n", test_name, expression);
  ++failures;
}

static void expect_near(double actual, double expected, double tolerance,
                        char const *context, char const *test_name) {
  auto diff = fabs(actual - expected);
  if (diff <= tolerance) return;
  fprintf(
      stderr,
      "FAIL [%s]: %s expected %.17g but got %.17g (|diff|=%.3e, tol=%.3e)\n",
      test_name, context, expected, actual, diff, tolerance);
  ++failures;
}

#define EXPECT_TRUE(expr) expect_true((expr), #expr, __func__)

[[nodiscard]] static bool arrays_match(double const *lhs, double const *rhs,
                                       size_t length) {
  return memcmp(lhs, rhs, length * sizeof(*lhs)) == 0;
}

static void fill_real_pattern(double *data, size_t length) {
  for (size_t index = 0; index < length; ++index) {
    auto x = (double)(index + 1);
    data[index] = sin(0.125 * x) + cos(0.375 * x) + (0.03125 * x);
  }
}

static void fill_complex_pattern(double *data, size_t length) {
  for (size_t index = 0; index < length; ++index) {
    auto x = (double)(index + 1);
    data[2 * index] = sin(0.125 * x) + 0.25 * cos(0.5 * x);
    data[2 * index + 1] = cos(0.375 * x) - 0.125 * sin(0.75 * x);
  }
}

static void reference_complex_dft(double const *input, double *output,
                                  size_t length, double direction) {
  constexpr double tau = 6.2831853071795864769252867665590058;
  for (size_t k = 0; k < length; ++k) {
    double sum_real = 0.;
    double sum_imag = 0.;
    for (size_t n = 0; n < length; ++n) {
      auto angle = tau * (double)(k * n) / (double)length;
      auto cosine = cos(angle);
      auto sine = sin(angle);
      auto xr = input[2 * n];
      auto xi = input[2 * n + 1];
      sum_real += (xr * cosine) - (direction * xi * sine);
      sum_imag += (direction * xr * sine) + (xi * cosine);
    }
    output[2 * k] = sum_real;
    output[2 * k + 1] = sum_imag;
  }
}

static void reference_real_forward(double const *input, double *output,
                                   size_t length) {
  double complex_input[2 * MAX_TEST_LENGTH];
  double complex_output[2 * MAX_TEST_LENGTH];

  for (size_t index = 0; index < length; ++index) {
    complex_input[2 * index] = input[index];
    complex_input[2 * index + 1] = 0.;
  }

  reference_complex_dft(complex_input, complex_output, length, -1.);

  output[0] = complex_output[0];
  for (size_t k = 1; k < length / 2; ++k) {
    output[2 * k - 1] = complex_output[2 * k];
    output[2 * k] = complex_output[2 * k + 1];
  }
  if ((length & 1) == 0) {
    if (length > 1) output[length - 1] = complex_output[length];
  } else if (length > 1) {
    auto k = length / 2;
    output[2 * k - 1] = complex_output[2 * k];
    output[2 * k] = complex_output[2 * k + 1];
  }
}

static void test_api_contract() {
  double sample[4] = {1., 2., 3., 4.};
  constexpr double original_real_sample[4] = {1., 2., 3., 4.};
  constexpr double original_complex_sample[4] = {1., 2., 3., 4.};

  EXPECT_TRUE(make_rfft_plan(0) == nullptr);
  EXPECT_TRUE(make_cfft_plan(0) == nullptr);
  EXPECT_TRUE(make_rfft_plan(SIZE_MAX) == nullptr);
  EXPECT_TRUE(make_cfft_plan(SIZE_MAX) == nullptr);
  EXPECT_TRUE(rfft_length(nullptr) == 0);
  EXPECT_TRUE(cfft_length(nullptr) == 0);
  EXPECT_TRUE(rfft_forward(nullptr, sample, 1.) == -1);
  EXPECT_TRUE(rfft_backward(nullptr, sample, 1.) == -1);
  EXPECT_TRUE(cfft_forward(nullptr, sample, 1.) == -1);
  EXPECT_TRUE(cfft_backward(nullptr, sample, 1.) == -1);

  auto real_plan = make_rfft_plan(8);
  auto complex_plan = make_cfft_plan(8);

  EXPECT_TRUE(real_plan != nullptr);
  EXPECT_TRUE(complex_plan != nullptr);

  if ((real_plan != nullptr) && (complex_plan != nullptr)) {
    EXPECT_TRUE(rfft_length(real_plan) == 8);
    EXPECT_TRUE(cfft_length(complex_plan) == 8);
    EXPECT_TRUE(rfft_forward(real_plan, nullptr, 1.) == -1);
    EXPECT_TRUE(rfft_backward(real_plan, nullptr, 1.) == -1);
    EXPECT_TRUE(cfft_forward(complex_plan, nullptr, 1.) == -1);
    EXPECT_TRUE(cfft_backward(complex_plan, nullptr, 1.) == -1);

    memcpy(sample, original_real_sample, sizeof(sample));
    EXPECT_TRUE(rfft_forward(real_plan, sample, INFINITY) == -1);
    EXPECT_TRUE(arrays_match(sample, original_real_sample, 4));

    memcpy(sample, original_real_sample, sizeof(sample));
    EXPECT_TRUE(rfft_backward(real_plan, sample, NAN) == -1);
    EXPECT_TRUE(arrays_match(sample, original_real_sample, 4));

    memcpy(sample, original_complex_sample, sizeof(sample));
    EXPECT_TRUE(cfft_forward(complex_plan, sample, -INFINITY) == -1);
    EXPECT_TRUE(arrays_match(sample, original_complex_sample, 4));

    memcpy(sample, original_complex_sample, sizeof(sample));
    EXPECT_TRUE(cfft_backward(complex_plan, sample, NAN) == -1);
    EXPECT_TRUE(arrays_match(sample, original_complex_sample, 4));
  }

  destroy_rfft_plan(real_plan);
  destroy_cfft_plan(complex_plan);
  destroy_rfft_plan(nullptr);
  destroy_cfft_plan(nullptr);
}

static void test_complex_forward_matches_reference() {
  constexpr size_t lengths[] = {
      1, 2, 3, 4, 5, 6, 7, 8, 11, 16, 25, 32, 49, 64, 97,
  };
  constexpr double tolerance = 5e-12;

  for (size_t i = 0; i < (sizeof(lengths) / sizeof(lengths[0])); ++i) {
    auto length = lengths[i];
    double input[2 * MAX_TEST_LENGTH];
    double actual[2 * MAX_TEST_LENGTH];
    double expected[2 * MAX_TEST_LENGTH];

    fill_complex_pattern(input, length);
    memcpy(actual, input, 2 * length * sizeof(double));
    reference_complex_dft(input, expected, length, -1.);

    auto plan = make_cfft_plan(length);
    EXPECT_TRUE(plan != nullptr);
    if (plan == nullptr) continue;

    EXPECT_TRUE(cfft_forward(plan, actual, 1.) == 0);
    EXPECT_TRUE(cfft_length(plan) == length);

    for (size_t element = 0; element < 2 * length; ++element) {
      char context[64];
      (void)snprintf(context, sizeof(context), "length=%zu element=%zu", length,
                     element);
      expect_near(actual[element], expected[element], tolerance, context,
                  __func__);
    }

    destroy_cfft_plan(plan);
  }
}

static void test_complex_roundtrip() {
  constexpr size_t lengths[] = {
      1, 2, 3, 4, 5, 7, 8, 16, 31, 32, 64, 97,
  };
  constexpr double tolerance = 5e-12;

  for (size_t i = 0; i < (sizeof(lengths) / sizeof(lengths[0])); ++i) {
    auto length = lengths[i];
    double original[2 * MAX_TEST_LENGTH];
    double actual[2 * MAX_TEST_LENGTH];

    fill_complex_pattern(original, length);
    memcpy(actual, original, 2 * length * sizeof(double));

    auto plan = make_cfft_plan(length);
    EXPECT_TRUE(plan != nullptr);
    if (plan == nullptr) continue;

    EXPECT_TRUE(cfft_forward(plan, actual, 1.) == 0);
    EXPECT_TRUE(cfft_backward(plan, actual, 1. / (double)length) == 0);

    for (size_t element = 0; element < 2 * length; ++element) {
      char context[64];
      (void)snprintf(context, sizeof(context), "length=%zu element=%zu", length,
                     element);
      expect_near(actual[element], original[element], tolerance, context,
                  __func__);
    }

    destroy_cfft_plan(plan);
  }
}

static void test_real_forward_matches_reference() {
  constexpr size_t lengths[] = {
      1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 25, 32, 63, 64, 97,
  };
  constexpr double tolerance = 5e-12;

  for (size_t i = 0; i < (sizeof(lengths) / sizeof(lengths[0])); ++i) {
    auto length = lengths[i];
    double input[MAX_TEST_LENGTH];
    double actual[MAX_TEST_LENGTH];
    double expected[MAX_TEST_LENGTH];

    fill_real_pattern(input, length);
    memcpy(actual, input, length * sizeof(double));
    reference_real_forward(input, expected, length);

    auto plan = make_rfft_plan(length);
    EXPECT_TRUE(plan != nullptr);
    if (plan == nullptr) continue;

    EXPECT_TRUE(rfft_forward(plan, actual, 1.) == 0);
    EXPECT_TRUE(rfft_length(plan) == length);

    for (size_t element = 0; element < length; ++element) {
      char context[64];
      (void)snprintf(context, sizeof(context), "length=%zu element=%zu", length,
                     element);
      expect_near(actual[element], expected[element], tolerance, context,
                  __func__);
    }

    destroy_rfft_plan(plan);
  }
}

static void test_real_roundtrip() {
  constexpr size_t lengths[] = {
      1, 2, 3, 4, 5, 7, 8, 16, 25, 32, 63, 64, 97,
  };
  constexpr double tolerance = 5e-12;

  for (size_t i = 0; i < (sizeof(lengths) / sizeof(lengths[0])); ++i) {
    auto length = lengths[i];
    double original[MAX_TEST_LENGTH];
    double actual[MAX_TEST_LENGTH];

    fill_real_pattern(original, length);
    memcpy(actual, original, length * sizeof(double));

    auto plan = make_rfft_plan(length);
    EXPECT_TRUE(plan != nullptr);
    if (plan == nullptr) continue;

    EXPECT_TRUE(rfft_forward(plan, actual, 1.) == 0);
    EXPECT_TRUE(rfft_backward(plan, actual, 1. / (double)length) == 0);

    for (size_t element = 0; element < length; ++element) {
      char context[64];
      (void)snprintf(context, sizeof(context), "length=%zu element=%zu", length,
                     element);
      expect_near(actual[element], original[element], tolerance, context,
                  __func__);
    }

    destroy_rfft_plan(plan);
  }
}

int main() {
  test_api_contract();
  test_complex_forward_matches_reference();
  test_complex_roundtrip();
  test_real_forward_matches_reference();
  test_real_roundtrip();

  if (failures != 0) {
    fprintf(stderr, "%d test assertion(s) failed\n", failures);
    return 1;
  }

  puts("all tests passed");
  return 0;
}
