#include "screen.hpp"
#include "event_loop.hpp"
#include "../log.hpp"
#include "../rmioc/screen.hpp"
#include "../rmioc/mxcfb.hpp"
#include <algorithm>
#include <chrono>
#include <ostream>
#include <stdexcept>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include <type_traits>
// IWYU pragma: no_include "rfb/rfbproto.h"

namespace chrono = std::chrono;

/**
 * Time to wait between two repaints.
 *
 * VNC servers tend to send a lot of small updates in a short period of time.
 * This delay allows grouping those small updates into a larger screen update.
 */
constexpr chrono::milliseconds repaint_delay{400};

namespace app
{

screen::screen(rmioc::screen& device, rfbClient* vnc_client)
: device(device)
, vnc_client(vnc_client)
, repaint_mode(repaint_modes::standard)
{
    rfbClientSetClientData(
        this->vnc_client,
        // ↓ Use of C library
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<void*>(screen::instance_tag),
        this
    );

    this->vnc_client->MallocFrameBuffer = screen::create_framebuf;
    this->vnc_client->GotFrameBufferUpdate = screen::recv_framebuf;
}

void screen::repaint()
{
    // Clear the has_update flag only in standard repaint mode
    // In fast mode, a clean update will be needed in the future
    if (this->repaint_mode == repaint_modes::standard)
    {
        this->update_info.has_update = false;
    }

    this->last_repaint_time = chrono::steady_clock::now();

    log::print("Screen update")
        << this->update_info.w << 'x' << this->update_info.h << '+'
        << this->update_info.x << '+' << this->update_info.y << '\n';

    this->device.update(
        this->update_info.x, this->update_info.y,
        this->update_info.w, this->update_info.h,
        this->repaint_mode == repaint_modes::standard
            ? mxcfb::waveform_modes::gc16
            : mxcfb::waveform_modes::du
    );
}

auto screen::get_xres() -> int
{
    return this->device.get_xres();
}

auto screen::get_yres() -> int
{
    return this->device.get_yres();
}

void screen::set_repaint_mode(repaint_modes mode)
{
    this->repaint_mode = mode;

    log::print("Screen update") << (mode == repaint_modes::standard
        ? "Switched to standard mode\n"
        : "Switched to fast mode\n");
}

auto screen::event_loop() -> event_loop_status
{
    if (this->update_info.has_update)
    {
        if (this->repaint_mode == repaint_modes::fast)
        {
            this->repaint();
        }
        else
        {
            auto now = chrono::steady_clock::now();
            int remaining_wait_time =
                chrono::duration_cast<chrono::milliseconds>(
                    this->last_repaint_time + repaint_delay - now
                ).count();

            if (remaining_wait_time <= 0)
            {
                this->repaint();
            }
            else
            {
                // Wait until the next update is due
                return {
                    /* quit = */ false,
                    /* timeout = */ remaining_wait_time
                };
            }
        }
    }

    return {/* quit = */ false, /* timeout = */ -1};
}

auto screen::create_framebuf(rfbClient* vnc_client) -> rfbBool
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* that = reinterpret_cast<screen*>(
        rfbClientGetClientData(
            vnc_client,
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<void*>(screen::instance_tag)
        ));

    // Make sure the server gives us a compatible format
    int xres_mem = static_cast<int>(that->device.get_xres_memory());
    int yres_mem = static_cast<int>(that->device.get_yres_memory());

    if (vnc_client->width < 0
        || vnc_client->height < 0
        || vnc_client->width != xres_mem
        || vnc_client->height > yres_mem)
    {
        std::stringstream msg;
        msg << "Server uses an unsupported resolution ("
            << that->vnc_client->width << 'x' << that->vnc_client->height
            << "). This client can only cope with a screen width of exactly "
            << xres_mem << " pixels and a screen height of "
            << yres_mem << " pixels";
        throw std::runtime_error{msg.str()};
    }

    // Configure connection with device framebuffer settings
    that->vnc_client->frameBuffer = that->device.get_data();
    that->vnc_client->format.bitsPerPixel = that->device.get_bits_per_pixel();
    that->vnc_client->format.depth = that->device.get_bits_per_pixel();
    that->vnc_client->format.redShift = that->device.get_red_format().offset;
    that->vnc_client->format.redMax = that->device.get_red_format().max();
    that->vnc_client->format.greenShift = that->device.get_green_format().offset;
    that->vnc_client->format.greenMax = that->device.get_green_format().max();
    that->vnc_client->format.blueShift = that->device.get_blue_format().offset;
    that->vnc_client->format.blueMax = that->device.get_blue_format().max();

    return TRUE;
}

void screen::recv_framebuf(rfbClient* vnc_client, int x, int y, int w, int h)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* that = reinterpret_cast<screen*>(
        rfbClientGetClientData(
            vnc_client,
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<void*>(screen::instance_tag)
        ));

    // Register the region as pending update, potentially extending
    // an existing one
    log::print("VNC update") << w << 'x' << h << '+' << x << '+' << y << '\n';

    if (that->update_info.has_update)
    {
        // Merge new rectangle with existing one
        int left_x = std::min(x, that->update_info.x);
        int top_y = std::min(y, that->update_info.y);
        int right_x = std::max(x + w,
                that->update_info.x + that->update_info.w);
        int bottom_y = std::max(y + h,
                that->update_info.y + that->update_info.h);

        that->update_info.x = left_x;
        that->update_info.y = top_y;
        that->update_info.w = right_x - left_x;
        that->update_info.h = bottom_y - top_y;
    }
    else
    {
        that->update_info.x = x;
        that->update_info.y = y;
        that->update_info.w = w;
        that->update_info.h = h;
        that->update_info.has_update = true;
    }
}

} // namespace app
