#include "client.hpp"
#include "../log.hpp"
#include "../rmioc/screen.hpp"
#include "../rmioc/touch.hpp"
#include <algorithm>
#include <array>
#include <bitset>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <vector>
#include <poll.h>
#include <rfb/rfbclient.h>
#include <unistd.h>
// IWYU pragma: no_include <type_traits>
// IWYU pragma: no_include <rfb/rfbproto.h>

/** Custom log printer for the VNC client library.  */
// NOLINTNEXTLINE(cert-dcl50-cpp): Need to use a vararg function for C compat
void vnc_client_log(const char* format, ...)
{
    va_list args;

    // ↓ Use of C library
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-no-array-decay)
    va_start(args, format);

    // NOLINTNEXTLINE(hicpp-no-array-decay): Use of C library
    ssize_t buffer_size = vsnprintf(nullptr, 0, format, args);
    std::vector<char> buffer(buffer_size + 1);

    // NOLINTNEXTLINE(hicpp-no-array-decay): Use of C library
    vsnprintf(buffer.data(), buffer.size(), format, args);
    log::print("VNC message") << buffer.data();

    // NOLINTNEXTLINE(hicpp-no-array-decay): Use of C library
    va_end(args);
}

namespace app
{

using namespace std::placeholders;

client::client(
    const char* ip, int port,
    rmioc::screen& screen_device,
    rmioc::touch& touch_device
)
: vnc_client(rfbGetClient(0, 0, 0))
, update_info{}
, screen_device(screen_device)
, touch_device(touch_device)
, touch_handler(touch_device, screen_device,
                std::bind(&client::send_button_press, this, _1, _2, _3))
{
    rfbClientLog = vnc_client_log;
    rfbClientErr = vnc_client_log;

    rfbClientSetClientData(
        this->vnc_client,
        // ↓ Use of C library
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<void*>(client::update_info_tag),
        &this->update_info
    );

    // ↓ Use of C library
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
    free(this->vnc_client->serverHost);
    this->vnc_client->serverHost = strdup(ip);
    this->vnc_client->serverPort = port;

    this->vnc_client->MallocFrameBuffer = client::create_framebuf;
    this->vnc_client->GotFrameBufferUpdate = client::update_framebuf;

    // Configure connection with device framebuffer settings
    this->vnc_client->frameBuffer = this->screen_device.get_data();
    this->vnc_client->format.bitsPerPixel
        = this->screen_device.get_bits_per_pixel();
    this->vnc_client->format.redShift = this->screen_device.get_red_offset();
    this->vnc_client->format.redMax = this->screen_device.get_red_max();
    this->vnc_client->format.greenShift
        = this->screen_device.get_green_offset();
    this->vnc_client->format.greenMax = this->screen_device.get_green_max();
    this->vnc_client->format.blueShift = this->screen_device.get_blue_offset();
    this->vnc_client->format.blueMax = this->screen_device.get_blue_max();

    if (rfbInitClient(this->vnc_client, nullptr, nullptr) == 0)
    {
        throw std::runtime_error{"Failed to initialize VNC connection"};
    }

    // Make sure the server gives us a compatible format
    int xres_mem = static_cast<int>(this->screen_device.get_xres_memory());
    int yres_mem = static_cast<int>(this->screen_device.get_yres_memory());

    if (this->vnc_client->width < 0
        || this->vnc_client->height < 0
        || this->vnc_client->width != xres_mem
        || this->vnc_client->height > yres_mem)
    {
        std::stringstream msg;
        msg << "Server uses an unsupported resolution ("
            << this->vnc_client->width << 'x' << this->vnc_client->height
            << "). This client can only cope with a screen width of exactly "
            << xres_mem << " pixels and a screen height of "
            << yres_mem << " pixels";
        throw std::runtime_error{msg.str()};
    }
}

client::~client()
{
    rfbClientCleanup(this->vnc_client);
}

void client::event_loop()
{
    // List of file descriptors to keep an eye on
    std::array<pollfd, 2> polled_fds{};

    constexpr std::size_t poll_vnc = 0;
    polled_fds[poll_vnc].fd = this->vnc_client->sock;
    polled_fds[poll_vnc].events = POLLIN;

    constexpr std::size_t poll_touch = 1;
    this->touch_device.setup_poll(polled_fds[poll_touch]);

    // Maximum time to wait before timeout in the next poll
    int timeout = -1;

    // Flag used for quitting the event loop
    bool quit = false;

    auto handle_status = [&timeout, &quit](const event_loop_status& st)
    {
        if (st.quit)
        {
            quit = true;
        }
        else
        {
            if (timeout == -1)
            {
                timeout = st.timeout;
            }
            else if (st.timeout != -1)
            {
                timeout = std::min(timeout, st.timeout);
            }
        }
    };

    // Wait for events from the VNC server or from device inputs
    while (!quit)
    {
        while (poll(polled_fds.data(), polled_fds.size(), timeout) == -1)
        {
            if (errno != EAGAIN)
            {
                throw std::system_error(
                    errno,
                    std::generic_category(),
                    "(client::event_loop) Wait for message"
                );
            }
        }

        timeout = -1;

        // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
        if ((polled_fds[poll_vnc].revents & POLLIN) != 0)
        {
            handle_status(this->event_loop_vnc());
        }

        handle_status(this->event_loop_screen());

        // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
        if ((polled_fds[poll_touch].revents & POLLIN) != 0)
        {
            handle_status(this->touch_handler.event_loop());
        }
    }
}

auto client::event_loop_vnc() -> event_loop_status
{
    if (HandleRFBServerMessage(this->vnc_client) == 0)
    {
        return {true, -1};
    }

    return {false, -1};
}

void client::send_button_press(
    int x, int y,
    MouseButton button
) const
{
    auto button_flag = static_cast<std::uint8_t>(button);
    constexpr auto bits = 8 * sizeof(button_flag);

    log::print("Button press")
        << x << 'x' << y << " (button mask: "
        << std::setfill('0') << std::setw(bits)
        << std::bitset<bits>(button_flag) << ")\n";

    SendPointerEvent(this->vnc_client, x, y, button_flag);
    SendPointerEvent(this->vnc_client, x, y, 0);
}

} // namespace app
