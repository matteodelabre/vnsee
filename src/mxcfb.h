#ifndef MXCFB_H
#define MXCFB_H

#include <stdint.h>

/**
 * Adapted from include/uapi/linux/mxcfb.h in the reMarkable kernel tree.
 * github.com/reMarkable/linux/blob/zero-gravitas/include/uapi/linux/mxcfb.h
 *
 * Copyright (C) 2013-2015 Freescale Semiconductor, Inc. All Rights Reserved
 */

struct mxcfb_rect
{
    uint32_t top;
    uint32_t left;
    uint32_t width;
    uint32_t height;
};

struct mxcfb_alt_buffer_data
{
    uint32_t phys_addr;
    uint32_t width; /* width of entire buffer */
    uint32_t height; /* height of entire buffer */
    struct mxcfb_rect alt_update_region; /* region within buffer to update */
};

struct mxcfb_update_data
{
    struct mxcfb_rect update_region;
    uint32_t waveform_mode;
    uint32_t update_mode;
    uint32_t update_marker;
    int temp;
    unsigned int flags;
    int dither_mode;
    int quant_bit;
    struct mxcfb_alt_buffer_data alt_buffer_data;
};

#define MXCFB_SEND_UPDATE \
    _IOW('F', 0x2E, struct mxcfb_update_data)

struct mxcfb_update_marker_data
{
    uint32_t update_marker;
    uint32_t collision_test;
};

#define MXCFB_WAIT_FOR_UPDATE_COMPLETE \
    _IOWR('F', 0x2F, struct mxcfb_update_marker_data)

#endif // MXCFB_H
