#include "util.h"
#include <inttypes.h>
#include <stdio.h>

uint64_t get_time_us()
{
    struct timespec cur_time_st;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time_st);
    return cur_time_st.tv_sec * 1000000 + cur_time_st.tv_nsec / 1000;
}

void print_log(const char* type)
{
    uint64_t time_us = get_time_us();

    fprintf(
        stderr,
        "%" PRIu64 ".%06" PRIu64 " [%s] ",
        time_us / 1000000,
        time_us % 1000000,
        type
    );
}
