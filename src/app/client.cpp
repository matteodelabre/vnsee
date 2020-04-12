#include "client.hpp"
#include "../log.hpp"
#include "../rmioc/buttons.hpp"
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
    rmioc::buttons& buttons_device,
    rmioc::screen& screen_device,
    rmioc::touch& touch_device
)
: polled_fds{}
, vnc_client(rfbGetClient(0, 0, 0))
, buttons_handler(buttons_device, screen_device)
, screen_handler(screen_device, vnc_client)
, touch_handler(touch_device, screen_device,
                std::bind(&client::send_button_press, this, _1, _2, _3))
{
    rfbClientLog = vnc_client_log;
    rfbClientErr = vnc_client_log;

    // ↓ Use of C library
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
    free(this->vnc_client->serverHost);
    this->vnc_client->serverHost = strdup(ip);
    this->vnc_client->serverPort = port;

    if (rfbInitClient(this->vnc_client, nullptr, nullptr) == 0)
    {
        throw std::runtime_error{"Failed to initialize VNC connection"};
    }

    buttons_device.setup_poll(this->polled_fds[poll_buttons]);
    touch_device.setup_poll(this->polled_fds[poll_touch]);

    this->polled_fds[poll_vnc].fd = this->vnc_client->sock;
    this->polled_fds[poll_vnc].events = POLLIN;
}

client::~client()
{
    rfbClientCleanup(this->vnc_client);
}

void client::event_loop()
{
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
        while (poll(
                    this->polled_fds.data(),
                    this->polled_fds.size(),
                    timeout) == -1)
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

        handle_status(this->screen_handler.event_loop());

        // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
        if ((polled_fds[poll_buttons].revents & POLLIN) != 0)
        {
            handle_status(this->buttons_handler.event_loop());
        }

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
        return {/* quit = */ true, /* timeout = */ -1};
    }

    return {/* quit = */ false, /* timeout = */ -1};
}

void client::send_button_press(
    int x, int y,
    MouseButton button
)
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
