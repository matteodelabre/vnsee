#ifndef RMIOC_SCREEN_HPP
#define RMIOC_SCREEN_HPP

#include <cstdint>

namespace rmioc
{

/**
 * Information about the location of a RGB component in a packed pixel.
 */
struct component_format
{
    /** Offset of the first bit of the component. */
    unsigned short offset;

    /** Number of contiguous bits used to represent the component. */
    unsigned short length;

    /** Maximum value. */
    std::uint32_t max() const;
}; // struct component_format

/**
 * E-ink waveform modes.
 *
 * E-ink display update modes, which offer various trade-offs between
 * fidelity, ghosting and speed.
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
 * Abstract class for accessing the device screen.
 */
class screen
{
public:
    virtual ~screen() = default;

    /**
     * Update a partial region of the screen.
     *
     * @param x Left bound of the region to update (in pixels).
     * @param y Top bound of the region to update (in pixels).
     * @param w Width of the region to update (in pixels).
     * @param h Height of the region to update (in pixels).
     * @param mode Update mode to use (default GC16).
     * @param wait True to block until the update is complete.
     */
    virtual void update(
        int x, int y, int w, int h,
        waveform_modes mode = waveform_modes::gc16,
        bool wait = false) = 0;

    /**
     * Perform a full update of the screen.
     *
     * @param mode Update mode to use (default GC16).
     * @param wait True to block until the update is complete.
     */
    virtual void update(
        waveform_modes mode = waveform_modes::gc16,
        bool wait = true) = 0;

    /**
     * Access the screen data buffer.
     *
     * This is a contiguous row-major-order array of pixels.
     *
     * Each row of this array contains `get_xres_memory()` pixels, which
     * may be greater than the actual `get_xres()` number of visible pixels.
     *
     * There are `get_yres_memory()` rows, which again may be greater than
     * the actual `get_yres()` number of visible rows.
     *
     * Each pixel is represented by `get_bits_per_pixel()` bits split among
     * red, blue and green components as specified by the
     * `get_{red|green|blue}_format()` values.
     */
    virtual std::uint8_t* get_data() = 0;

    /** Number of visible pixels in each row of the screen. */
    virtual int get_xres() const = 0;

    /** Number of in-memory pixels in each row of the screen. */
    virtual int get_xres_memory() const = 0;

    /** Number of visible rows in the screen. */
    virtual int get_yres() const = 0;

    /** Number of in-memory rows in the screen. */
    virtual int get_yres_memory() const = 0;

    /** Get the number of bits in a packed pixel. */
    virtual unsigned short get_bits_per_pixel() const = 0;

    /** Get packing information about the red pixel component. */
    virtual component_format get_red_format() const = 0;

    /** Get packing information about the green pixel component. */
    virtual component_format get_green_format() const = 0;

    /** Get packing information about the blue pixel component. */
    virtual component_format get_blue_format() const = 0;
}; // class screen

} // namespace rmioc

#endif // RMIOC_SCREEN_HPP
