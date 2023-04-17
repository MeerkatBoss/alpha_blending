/**
 * @file test_macros.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 * 
 * @brief Helper macros for running tests
 *
 * @version 0.1
 * @date 2023-04-06
 *
 * @copyright Copyright MeerkatBoss (c) 2023
 */
#ifndef __TEST_MACROS_H
#define __TEST_MACROS_H

// TODO: Add documentation

#include <time.h>
#include <stdint.h>
#include <math.h>

#define SEQUENCE(...) __VA_ARGS__

#define RUN_TEST(function, data, cnt_repeat, result) do                     \
{                                                                           \
    clock_t start_time = clock();                                           \
    for (size_t r = 0; r < cnt_repeat; r++)                                 \
        function(data);                                                     \
    clock_t end_time = clock();                                             \
    (result) = (size_t)((end_time - start_time) / (CLOCKS_PER_SEC / 1000)); \
} while (0)

#define COLLECT_DATA(function, data, cnt_repeat, buffer, buffer_size) do    \
{                                                                           \
    for (size_t i = 0; i < buffer_size; i++)                                \
        RUN_TEST(function, data, cnt_repeat, buffer[i]);                    \
} while (0)

inline double get_average_time(const size_t* test_data, size_t test_size)
{
    double sum = 0;
    for (size_t i = 0; i < test_size; ++i)
        sum += test_data[i];
    return sum / test_size;
}

inline double get_time_standard_deviation(double average,
                                          const size_t* test_data,
                                          size_t test_size)
{
    double dispersion = 0;
    for (size_t i = 0; i < test_size; ++i)
    {
        double delta = test_data[i] - average;
        dispersion += delta*delta;
    }
    return sqrt(dispersion / (test_size - 1));
}

#endif /* test_macros.h */
