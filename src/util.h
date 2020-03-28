#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <time.h>

typedef uint64_t mono_time;

/**
 * Get the current monotonic time in microseconds.
 */
mono_time get_mono_time();

/**
 * Print the prefix for logging an event.
 *
 * @param type Type of event.
 */
void print_log(const char* type);

#endif // UTIL_H
