#include "client.hpp"
#include "../log.hpp"
#include "../rmioc/buttons.hpp"
#include "../rmioc/device.hpp"
#include "../rmioc/pen.hpp"
#include "../rmioc/touch.hpp"
#include <algorithm>
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
#include <memory>
#include <stdexcept>
#include <system_error>
#include <vector>
#include <poll.h>
#include <rfb/rfbclient.h>
#include <unistd.h>
// IWYU pragma: no_include <type_traits>

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

client::client(const char* ip, int port, rmioc::device& device)
: vnc_client(rfbGetClient(0, 0, 0))
{
    if (device.get_screen() == nullptr)
    {
        throw std::runtime_error{"Missing screen device"};
    }

    auto& screen_device = *device.get_screen();
    this->screen_handler.emplace(screen_device, vnc_client);

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

    if (device.get_buttons() != nullptr)
    {
        auto& buttons_device = *device.get_buttons();
        this->buttons_handler.emplace(buttons_device, screen_device);
        this->poll_buttons = this->polled_fds.size();
        this->polled_fds.push_back(pollfd{});
        buttons_device.setup_poll(this->polled_fds[this->poll_buttons]);
    }

    auto button_callback = [this](int x, int y, MouseButton button)
    {
        this->send_button_press(x, y, button);
    };

    if (device.get_pen() != nullptr)
    {
        auto& pen_device = *device.get_pen();
        this->pen_handler.emplace(
            pen_device, *this->screen_handler,
            button_callback);
        this->poll_pen = this->polled_fds.size();
        this->polled_fds.push_back(pollfd{});
        pen_device.setup_poll(this->polled_fds[this->poll_pen]);
    }

    if (device.get_touch() != nullptr)
    {
        auto& touch_device = *device.get_touch();
        this->touch_handler.emplace(
            touch_device, screen_device,
            button_callback);
        this->poll_touch = this->polled_fds.size();
        this->polled_fds.push_back(pollfd{});
        touch_device.setup_poll(this->polled_fds[this->poll_touch]);
    }

    this->poll_vnc = this->polled_fds.size();
    this->polled_fds.push_back(pollfd{
        /* fd = */ this->vnc_client->sock,
        /* events = */ POLLIN,
        /* revents = */ 0
    });
}

client::~client()
{
    rfbClientCleanup(this->vnc_client);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto client::event_loop() -> bool
{
    // Maximum time to wait before timeout in the next poll
    long timeout = -1;

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
                    static_cast<int>(timeout)) == -1)
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
        if ((polled_fds[this->poll_vnc].revents & POLLIN) != 0)
        {
            if (HandleRFBServerMessage(this->vnc_client) == 0)
            {
                return false;
            }
        }

        handle_status(this->screen_handler->event_loop());

        if (this->pen_handler.has_value()
        // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
                && (polled_fds[this->poll_pen].revents & POLLIN) != 0)
        {
            handle_status(this->pen_handler->process_events());
        }

        bool inhibit = this->pen_handler.has_value()
            && this->pen_handler->is_inhibiting();

        if (this->buttons_handler.has_value()
        // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
                && (polled_fds[this->poll_buttons].revents & POLLIN) != 0)
        {
            handle_status(this->buttons_handler->process_events(inhibit));
        }

        if (this->touch_handler.has_value()
        // NOLINTNEXTLINE(hicpp-signed-bitwise): Use of C library
                && (polled_fds[this->poll_touch].revents & POLLIN) != 0)
        {
            handle_status(this->touch_handler->process_events(inhibit));
        }
    }

    return true;
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
}

} // namespace app
