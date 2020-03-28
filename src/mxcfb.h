#ifndef MXCFB_H
#define MXCFB_H

#include <stdint.h>

/**
 * Adapted from include/uapi/linux/mxcfb.h in the reMarkable kernel tree.
 * See https://github.com/reMarkable/linux/blob/zero-gravitas/include/uapi/linux/mxcfb.h
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
    /** Specify the region to update (when using MXCFB_UPDATE_MODE_PARTIAL). */
    struct mxcfb_rect update_region;

    /** E-ink display update mode. */
    uint32_t waveform_mode;

    /** Choose between full or partial update. */
    uint32_t update_mode;

    /**
     * Sequence number for this update.
     *
     * Used when waiting for update completion using
     * MXCFB_WAIT_FOR_UPDATE_COMPLETE, where the same number must be used.
     */
    uint32_t update_marker;

    /** TODO: Find out what this is used for. */
    int temp;

    /** TODO: Find out what this is used for. */
    unsigned int flags;

    /** TODO: Find out what this is used for. */
    int dither_mode;

    /** TODO: Find out what this is used for. */
    int quant_bit;

    /** TODO: Find out what this is used for. */
    struct mxcfb_alt_buffer_data alt_buffer_data;
};

/**
 * Waveform modes.
 *
 * E-ink display update modes, which offer various tradeoffs between
 * fidelity and speed.
 *
 * See:
 *
 * - http://www.waveshare.net/w/upload/c/c4/E-paper-mode-declaration.pdf
 * - https://github.com/koreader/koreader-base/blob/master/ffi-cdecl/include/mxcfb-remarkable.h
 */

/**
 * Initialization mode.
 *
 * Completely erase the display to white.
 * Must be used with MXCFB_UPDATE_MODE_FULL.
 *
 * Update time: 2 s.
 * Ghosting: None.
 */
#define MXCFB_WAVEFORM_MODE_INIT 0

/**
 * Direct update mode.
 *
 * For fast response to user input.
 * Can only set cells to full black or full white (grays will be ignored).
 *
 * Update time: 260 ms.
 * Ghosting: Low.
 */
#define MXCFB_WAVEFORM_MODE_DU 1

/**
 * Grayscale clearing mode.
 *
 * For high quality images.
 * Will flash the screen when used with MXCFB_UPDATE_MODE_FULL.
 *
 * Update time: 450 ms.
 * Ghosting: Very low.
 */
#define MXCFB_WAVEFORM_MODE_GC16 2

/**
 * Low-fidelity grayscale clearing mode.
 *
 * For text on white background.
 *
 * Update time: 450 ms.
 * Ghosting: Medium.
 */
#define MXCFB_WAVEFORM_MODE_GL16 3

/**
 * A2 mode.
 *
 * For page turning or simple black and white animation.
 * Can only set cells from full black/white to full black/white.
 *
 * Update time: 120 ms.
 * Ghosting: High.
 */
#define MXCFB_WAVEFORM_MODE_A2 4

/**
 * Update modes.
 */

/**
 * Partial mode.
 *
 * Only update the requested rectangle.
 */
#define MXCFB_UPDATE_MODE_PARTIAL 0

/**
 * Full mode.
 *
 * Update the whole screen.
 */
#define MXCFB_UPDATE_MODE_FULL 1

/** Ioctl request updating the screen. */
#define MXCFB_SEND_UPDATE \
    _IOW('F', 0x2E, struct mxcfb_update_data)

struct mxcfb_update_marker_data
{
    /** Sequence number of the update to wait for completion. */
    uint32_t update_marker;

    /** TODO: Find out what this is used for. */
    uint32_t collision_test;
};

/** Ioctl request for blocking until an update has been performed. */
#define MXCFB_WAIT_FOR_UPDATE_COMPLETE \
    _IOWR('F', 0x2F, struct mxcfb_update_marker_data)

#endif // MXCFB_H
