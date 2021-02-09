#ifndef RMIOC_MXCFB_HPP
#define RMIOC_MXCFB_HPP

#include "screen.hpp"
#include <cstdint>
#include <sys/ioctl.h>

namespace mxcfb
{

/**
 * Interface to the MXC EPDC framebuffer available on reMarkable 1.
 *
 * See also:
 *
 * - Section 6.2.6.1 of https://www.nxp.com/docs/en/reference-manual/i.MX_Reference_Manual_Linux.pdf
 * - https://github.com/reMarkable/linux/blob/zero-gravitas/include/uapi/linux/mxcfb.h
 * - https://github.com/reMarkable/linux/blob/zero-gravitas/drivers/video/fbdev/mxc/mxc_epdc_v2_fb.c
 */

struct rect
{
    std::uint32_t top;
    std::uint32_t left;
    std::uint32_t width;
    std::uint32_t height;

    /**
     * Clip a signed rectangle to fit in the screen bounds.
     *
     * @param left First horizontal pixel of the rectangle.
     * @param top First vertical pixel of the rectangle.
     * @param width Number of horizontal pixels in the rectangle.
     * @param height Number of vertical pixels in the rectangle.
     * @param xres Horizontal screen resolution.
     * @param yres Vertical screen resolution.
     * @return Clipped rectangle.
     */
    static rect clip(
        int left, int top,
        int width, int height,
        int xres, int yres
    );

    /**
     * Check if a screen rectangle is not empty.
     *
     * @return True if the rectangle is not empty.
     */
    operator bool() const;
};

struct alt_buffer_data
{
    std::uint32_t phys_addr;
    std::uint32_t width; /* width of entire buffer */
    std::uint32_t height; /* height of entire buffer */
    rect alt_update_region; /* region within buffer to update */
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
    rmioc::waveform_modes waveform_mode;

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
