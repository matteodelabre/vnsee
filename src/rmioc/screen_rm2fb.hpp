#ifndef RMIOC_SCREEN_RM2FB_HPP
#define RMIOC_SCREEN_RM2FB_HPP

#include "screen.hpp"
#include "mxcfb.hpp"
#include <cstdint>
#include <linux/fb.h>

namespace rmioc
{

/**
 * Connect to a rm2fb server to access the reMarkable 2 screen.
 *
 * On reMarkable 2, screen cannot be accessed through a kernel driver but
 * must be controlled by software. This class connects to a running rm2fb
 * server that handles the driver logic.
 *
 * See <https://github.com/ddvk/remarkable2-framebuffer>.
 */
class screen_rm2fb : public screen
{
public:
    /**
     * Connect to the rm2fb server.
     *
     * @param framebuf_path Path to the shared memory framebuffer.
     * @param msgqueue_key Unique key for the update message queue.
     */
    screen_rm2fb(const char* shm_path, int msgqueue_key);

    /** Close the screen device. */
    ~screen_rm2fb();

    // Disallow copying screen device handles
    screen_rm2fb(const screen_rm2fb& other) = delete;
    screen_rm2fb& operator=(const screen_rm2fb& other) = delete;

    // Transfer handle ownership
    screen_rm2fb(screen_rm2fb&& other) noexcept;
    screen_rm2fb& operator=(screen_rm2fb&& other) noexcept;

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
    /** File descriptor for the shared memory framebuffer. */
    int framebuf_fd = -1;

    /** Identifier of the framebuffer message queue. */
    int msgqueue_id = -1;

    /** Pointer to the memory-mapped framebuffer. */
    std::uint8_t* framebuf_ptr = nullptr;

    /**
     * Send an update object to the mxcfb driver.
     *
     * @param update Update object to send.
     * @param wait True to wait until update is complete.
     */
    void send_update(mxcfb::update_data& update, bool wait) const;
}; // class screen_rm2fb

} // namespace rmioc

#endif // RMIOC_SCREEN_RM2FB_HPP
