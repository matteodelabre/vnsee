#ifndef RMIOC_SCREEN_MXCFB_HPP
#define RMIOC_SCREEN_MXCFB_HPP

#include "screen.hpp"
#include <cstdint>
#include <linux/fb.h>

namespace mxcfb
{
    struct update_data;
}

namespace rmioc
{

/**
 * Access the mxcfb screen (reMarkable 1 only).
 *
 * On reMarkable 1, screen access is exposed through the mxcfb framebuffer
 * driver, usually available at `/dev/fb0`.
 */
class screen_mxcfb : public screen
{
public:
    /**
     * Open the screen mxcfb device.
     *
     * @param path Path to the device.
     */
    screen_mxcfb(const char* device_path);

    /** Close the screen device. */
    ~screen_mxcfb();

    // Disallow copying screen device handles
    screen_mxcfb(const screen_mxcfb& other) = delete;
    screen_mxcfb& operator=(const screen_mxcfb& other) = delete;

    // Transfer handle ownership
    screen_mxcfb(screen_mxcfb&& other) noexcept;
    screen_mxcfb& operator=(screen_mxcfb&& other) noexcept;

    void update(
        int x, int y, int w, int h,
        waveform_modes mode = waveform_modes::gc16,
        bool wait = false) override;

    void update(
        waveform_modes mode = waveform_modes::gc16,
        bool wait = true) override;

    std::uint8_t* get_data() override;

    int get_xres() const override;
    int get_xres_memory() const override;
    int get_yres() const override;
    int get_yres_memory() const override;

    unsigned short get_bits_per_pixel() const override;
    component_format get_red_format() const override;
    component_format get_green_format() const override;
    component_format get_blue_format() const override;

private:
    /** File descriptor for the device framebuffer. */
    int framebuf_fd = -1;

    /** Variable screen information from the device framebuffer. */
    fb_var_screeninfo framebuf_varinfo{};

    /** Fixed screen information from the device framebuffer. */
    fb_fix_screeninfo framebuf_fixinfo{};

    /** Pointer to the memory-mapped framebuffer. */
    std::uint8_t* framebuf_ptr = nullptr;

    /**
     * Send an update object to the mxcfb driver.
     *
     * @param update Update object to send.
     * @param wait True to wait until update is complete.
     */
    void send_update(mxcfb::update_data& update, bool wait);

    /** Next value to be used as an update marker. */
    std::uint32_t next_update_marker = 1;

    /** Maximum value to use for update markers. */
    static constexpr std::uint32_t max_update_marker = 255;
}; // class screen_mxcfb

} // namespace rmioc

#endif // RMIOC_SCREEN_MXCFB_HPP
