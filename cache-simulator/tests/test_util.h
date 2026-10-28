/* Assertions for standalone tests; each test main owns test_failures. */
#ifndef CACHE_SIMULATOR_TEST_UTIL_H
#define CACHE_SIMULATOR_TEST_UTIL_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#define CHECK(condition)                                                        \
    do {                                                                        \
        int check_result_ = !!(condition);                                       \
        if (!check_result_) {                                                   \
            (void)fprintf(stderr, "%s:%d: CHECK(%s) got %d, expected 1\n",     \
                          __FILE__, __LINE__, #condition, check_result_);       \
            ++test_failures;                                                    \
        }                                                                       \
    } while (0)

#define CHECK_EQ_U(actual, expected)                                             \
    do {                                                                        \
        uintmax_t actual_value_ = (uintmax_t)(actual);                          \
        uintmax_t expected_value_ = (uintmax_t)(expected);                      \
        if (actual_value_ != expected_value_) {                                 \
            (void)fprintf(stderr,                                               \
                          "%s:%d: CHECK_EQ_U(%s, %s) got %" PRIuMAX            \
                          ", expected %" PRIuMAX "\n",                         \
                          __FILE__, __LINE__, #actual, #expected,               \
                          actual_value_, expected_value_);                      \
            ++test_failures;                                                    \
        }                                                                       \
    } while (0)

#define CHECK_NEAR(actual, expected, tolerance)                                  \
    do {                                                                        \
        double actual_value_ = (double)(actual);                                \
        double expected_value_ = (double)(expected);                            \
        double tolerance_value_ = (double)(tolerance);                          \
        double difference_ = actual_value_ - expected_value_;                  \
        if (difference_ < 0.0) {                                                \
            difference_ = -difference_;                                        \
        }                                                                       \
        if (!(difference_ <= tolerance_value_)) {                               \
            (void)fprintf(stderr,                                               \
                          "%s:%d: CHECK_NEAR(%s, %s, %s) got %.17g, "          \
                          "expected %.17g (tolerance %.17g)\n",                 \
                          __FILE__, __LINE__, #actual, #expected,               \
                          #tolerance, actual_value_, expected_value_,           \
                          tolerance_value_);                                    \
            ++test_failures;                                                    \
        }                                                                       \
    } while (0)

#define TEST_SUMMARY()                                                          \
    do {                                                                        \
        (void)fprintf(test_failures == 0 ? stdout : stderr,                     \
                      "%s: %d failure(s)\n", __FILE__, test_failures);          \
        return test_failures == 0 ? 0 : 1;                                      \
    } while (0)

#endif
