#include "screen.hpp"
#include "event_loop.hpp"
#include "../log.hpp"
#include "../rmioc/screen.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include <type_traits>

namespace chrono = std::chrono;

/**
 * Time to wait between two standard repaints.
 *
 * VNC servers tend to send a lot of small updates in a short period of time.
 * This delay allows grouping those small updates into a larger screen update.
 */
constexpr chrono::milliseconds standard_repaint_delay{400};

/**
 * Time to wait between two fast repaints.
 *
 * This mode is used when the pen is active so that the user can have a quicker
 * feedback on what they are drawing. When the pen is lifted, a high fidelity
 * update is performed.
 */
constexpr chrono::milliseconds fast_repaint_delay{50};

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

    // Ask the server to send pixels in the same format as the screen buffer
    this->vnc_client->format.bitsPerPixel = this->device.get_bits_per_pixel();
    this->vnc_client->format.depth = this->device.get_bits_per_pixel();
    this->vnc_client->format.redShift = this->device.get_red_format().offset;
    this->vnc_client->format.redMax = this->device.get_red_format().max();
    this->vnc_client->format.greenShift = this->device.get_green_format().offset;
    this->vnc_client->format.greenMax = this->device.get_green_format().max();
    this->vnc_client->format.blueShift = this->device.get_blue_format().offset;
    this->vnc_client->format.blueMax = this->device.get_blue_format().max();

    // Force the raw encoding and override the rect reception methods
    this->vnc_client->appData.encodingsString = "raw";
    this->vnc_client->MallocFrameBuffer = screen::create_framebuf;
    this->vnc_client->GotBitmap = screen::recv_update;
    this->vnc_client->GotFrameBufferUpdate = screen::commit_updates;
}

void screen::repaint()
{
    // Clear the has_update flag only in standard repaint mode
    // In fast mode, a clean update will be needed in the future
    if (this->repaint_mode == repaint_modes::standard)
    {
        this->update_info.has_update = false;
    }

    this->last_repaint = chrono::steady_clock::now();

    log::print("Screen update")
        << this->update_info.w << 'x' << this->update_info.h << '+'
        << this->update_info.x << '+' << this->update_info.y << '\n';

    this->device.update(
        this->update_info.x, this->update_info.y,
        this->update_info.w, this->update_info.h,
        this->repaint_mode == repaint_modes::standard
            ? rmioc::waveform_modes::gl16
            : rmioc::waveform_modes::du
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
        auto next_update_time = this->last_repaint + (
            this->repaint_mode == repaint_modes::standard
            ? standard_repaint_delay
            : fast_repaint_delay
        );

        auto now = chrono::steady_clock::now();
        int wait_time = chrono::duration_cast<chrono::milliseconds>(
            next_update_time - now
        ).count();

        if (wait_time <= 0)
        {
            this->repaint();
        }
        else
        {
            // Wait until the next update is due
            return {
                /* quit = */ false,
                /* timeout = */ wait_time
            };
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

    int xres = static_cast<int>(that->device.get_xres());
    int yres = static_cast<int>(that->device.get_yres());

    if (vnc_client->width < 0 || vnc_client->height < 0)
    {
        std::stringstream msg;
        msg << "Invalid server resolution ("
            << vnc_client->width << 'x' << vnc_client->height;
        throw std::runtime_error{msg.str()};
    }

    if (vnc_client->width > xres || vnc_client->height > yres)
    {
        std::cerr << "Warning: The server resolution ("
            << vnc_client->width << 'x' << vnc_client->height
            << ") does not fit in the screen ("
            << xres << 'x' << yres << ")\nThe image will be cropped to fit\n";
    }

    return TRUE;
}

void screen::recv_update(
    rfbClient* vnc_client, const uint8_t* buffer,
    int x, int y, int w, int h
)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* that = reinterpret_cast<screen*>(
        rfbClientGetClientData(
            vnc_client,
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<void*>(screen::instance_tag)
        ));

    if (x < 0 || y < 0
        || x >= that->device.get_xres_memory()
        || y >= that->device.get_yres_memory())
    {
        return;
    }

    std::size_t pixel_size = that->device.get_bits_per_pixel() / 8;

    std::size_t dest_stride = that->device.get_xres_memory() * pixel_size;
    std::size_t dest_size = dest_stride * that->device.get_yres_memory();
    uint8_t* const dest = that->device.get_data();
    uint8_t* dest_line = dest + y * dest_stride + x * pixel_size;

    std::size_t buffer_stride = w * pixel_size;
    std::size_t buffer_size = buffer_stride * h;
    const uint8_t* buffer_line = buffer;

    while (dest_line < dest + dest_size && buffer_line < buffer + buffer_size)
    {
        std::memcpy(
            dest_line,
            buffer_line,
            std::min(dest_stride - x * pixel_size, buffer_stride)
        );

        dest_line += dest_stride;
        buffer_line += buffer_stride;
    }
}

void screen::commit_updates(rfbClient* vnc_client, int x, int y, int w, int h)
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
