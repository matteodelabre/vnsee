#ifndef RMIOC_MXCFB_HPP
#define RMIOC_MXCFB_HPP

#include <cstdint>
#include <linux/ioctl.h>

namespace mxcfb
{

/**
 * Adapted from the reMarkable kernel tree.
 * See https://github.com/reMarkable/linux/blob/zero-gravitas/include/uapi/linux/mxcfb.h
 *
 * Copyright (C) 2013-2015 Freescale Semiconductor, Inc. All Rights Reserved
 */

struct rect
{
    std::uint32_t top;
    std::uint32_t left;
    std::uint32_t width;
    std::uint32_t height;
};

struct alt_buffer_data
{
    std::uint32_t phys_addr;
    std::uint32_t width; /* width of entire buffer */
    std::uint32_t height; /* height of entire buffer */
    rect alt_update_region; /* region within buffer to update */
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
enum class waveform_modes : std::uint32_t
{
    /**
     * Initialization mode.
     *
     * Completely erase the display to white.
     * Must be used with MXCFB_UPDATE_MODE_FULL.
     *
     * Update time: 2 s.
     * Ghosting: None.
     */
    init = 0,

    /**
     * Direct update mode.
     *
     * For fast response to user input.
     * Can only set cells to full black or full white (grays will be ignored).
     *
     * Update time: 260 ms.
     * Ghosting: Low.
     */
    du = 1,

    /**
     * Grayscale clearing mode.
     *
     * For high quality images.
     * Will flash the screen when used with MXCFB_UPDATE_MODE_FULL.
     *
     * Update time: 450 ms.
     * Ghosting: Very low.
     */
    gc16 = 2,

    /**
     * Low-fidelity grayscale clearing mode.
     *
     * For text on white background.
     *
     * Update time: 450 ms.
     * Ghosting: Medium.
     */
    gl16 = 3,

    /**
     * A2 mode.
     *
     * For page turning or simple black and white animation.
     * Can only set cells from full black/white to full black/white.
     *
     * Update time: 120 ms.
     * Ghosting: High.
     */
    a2 = 4,
};

/**
 * Update modes.
 */
enum class update_modes : std::uint32_t
{
    /**
     * Partial mode.
     *
     * Only update the requested rectangle.
     */
    partial = 0,

    /**
     * Full mode.
     *
     * Update the whole screen.
     */
    full = 1,
};

/**
 * TODO: Figure out what this is used for.
 * Temps.
 */
enum class temps : std::uint32_t
{
    normal = 0x18,
};

/**
 * Screen update payload.
 */
struct update_data
{
    /** Specify the region to update (when using MXCFB_UPDATE_MODE_PARTIAL). */
    rect update_region;

    /** E-ink display update mode. */
    waveform_modes waveform_mode;

    /** Choose between full or partial update. */
    update_modes update_mode;

    /**
     * Sequence number for this update.
     *
     * Used when waiting for update completion using
     * MXCFB_WAIT_FOR_UPDATE_COMPLETE, where the same number must be used.
     */
    std::uint32_t update_marker;

    /** TODO: Find out what this is used for. */
    temps temp;

    /** TODO: Find out what this is used for. */
    unsigned int flags;

    /** TODO: Find out what this is used for. */
    int dither_mode;

    /** TODO: Find out what this is used for. */
    int quant_bit;

    /** TODO: Find out what this is used for. */
    alt_buffer_data alt_buffer_data_;
};

/** Ioctl request for updating the screen. */
constexpr auto send_update = _IOW('F', 0x2E, update_data);

struct update_marker_data
{
    /** Sequence number of the update to wait for completion. */
    std::uint32_t update_marker;

    /** TODO: Find out what this is used for. */
    std::uint32_t collision_test;
};

/** Ioctl request for blocking until an update has been performed. */
constexpr auto wait_for_update_complete = _IOWR('F', 0x2F, update_marker_data);

} // namespace mxcfb

#endif // RMIOC_MXCFB_HPP
