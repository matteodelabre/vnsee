#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <time.h>

/**
 * Get the current monotonic time in microseconds.
 */
uint64_t get_time_us();

/**
 * Print the prefix for logging an event.
 *
 * @param type Type of event.
 */
void print_log(const char* type);

#endif // UTIL_H
