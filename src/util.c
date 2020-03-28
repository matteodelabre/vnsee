#include "util.h"
#include <inttypes.h>
#include <stdio.h>

mono_time get_mono_time()
{
    struct timespec cur_time_st;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time_st);

    return (mono_time) cur_time_st.tv_sec * 1000000
         + (mono_time) cur_time_st.tv_nsec / 1000;
}

void print_log(const char* type)
{
    mono_time cur_time = get_mono_time();

    fprintf(
        stderr,
        "%" PRIu64 ".%06" PRIu64 " [%s] ",
        cur_time / 1000000,
        cur_time % 1000000,
        type
    );
}
