#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdlib.h>

/** Width of the reMarkable screen. */
#define SCREEN_COLS 1404

/** Number of padding pixels at the end of each line. */
#define SCREEN_COL_PAD 4

/** Height of the reMarkable screen. */
#define SCREEN_ROWS 1872

typedef struct line_update
{
    /** Index of the line to update. */
    uint16_t line_idx;

    /** Contents of the line to update. */
    uint16_t buffer[SCREEN_COLS];
} line_update;

#endif // DEFS_H
