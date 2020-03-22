#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

/** Width of the reMarkable screen. */
#define SCREEN_COLS 1404

/** Number of padding pixels at the end of each line. */
#define SCREEN_COL_PAD 4

/** Height of the reMarkable screen. */
#define SCREEN_ROWS 1872

typedef struct {
    uint32_t top;
    uint32_t left;
    uint32_t width;
    uint32_t height;
} mxcfb_rect;

typedef struct {
    uint32_t phys_addr;
    uint32_t width; /* width of entire buffer */
    uint32_t height; /* height of entire buffer */
    mxcfb_rect alt_update_region; /* region within buffer to update */
} mxcfb_alt_buffer_data;

typedef struct {
    mxcfb_rect update_region;
    uint32_t waveform_mode;
    uint32_t update_mode;
    uint32_t update_marker;
    int temp;
    unsigned int flags;
    int dither_mode;
    int quant_bit;
    mxcfb_alt_buffer_data alt_buffer_data;
} mxcfb_update_data;

/** Ioctl for requesting screen refresh. */
#define SEND_UPDATE 0x4048462e

#endif // DEFS_H
