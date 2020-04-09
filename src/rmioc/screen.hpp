#ifndef RMIOC_SCREEN_HPP
#define RMIOC_SCREEN_HPP

#include <cstdint>
#include <linux/fb.h>

namespace rmioc
{

/**
 * Information and resources for using the device screen.
 */
class screen
{
public:
    screen();
    ~screen();

    /**
     * Update the given region of the screen.
     *
     * @param x Left bound of the region to update (in pixels).
     * @param y Top bound of the region to update (in pixels).
     * @param w Width of the region to update (in pixels).
     * @param h Height of the region to update (in pixels).
     */
    void update(int x, int y, int w, int h);

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
     * `get_{red|green|blue}_{offset|length}()` values.
     */
    std::uint8_t* get_data();

    std::uint32_t get_xres() const;
    std::uint32_t get_xres_memory() const;

    std::uint32_t get_yres() const;
    std::uint32_t get_yres_memory() const;

    std::uint32_t get_bits_per_pixel() const;

    std::uint32_t get_red_offset() const;
    std::uint32_t get_red_length() const;
    std::uint32_t get_red_max() const;

    std::uint32_t get_green_offset() const;
    std::uint32_t get_green_length() const;
    std::uint32_t get_green_max() const;

    std::uint32_t get_blue_offset() const;
    std::uint32_t get_blue_length() const;
    std::uint32_t get_blue_max() const;

private:
    /** File descriptor for the device framebuffer. */
    int framebuf_fd;

    /** Variable screen information from the device framebuffer. */
    fb_var_screeninfo framebuf_varinfo;

    /** Fixed screen information from the device framebuffer. */
    fb_fix_screeninfo framebuf_fixinfo;

    /** Pointer to the memory-mapped framebuffer. */
    uint8_t* framebuf_ptr;

    /** Next value to be used as an update marker. */
    uint32_t next_update_marker;
}; // class screen

} // namespace rmioc

#endif // RMIOC_SCREEN_HPP
