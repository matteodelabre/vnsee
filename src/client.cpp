#include "client.hpp"
#include "log.hpp"
#include "rmioc/screen.hpp"
#include "rmioc/touch.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

client::client(
    const char* ip, int port,
    rmioc::screen& rm_screen,
    rmioc::touch& rm_touch
)
: vnc_client(rfbGetClient(0, 0, 0))
, update_info{}
, rm_screen(rm_screen)
, rm_touch(rm_touch)
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
    this->vnc_client->frameBuffer = this->rm_screen.get_data();
    this->vnc_client->format.bitsPerPixel = this->rm_screen.get_bits_per_pixel();
    this->vnc_client->format.redShift = this->rm_screen.get_red_offset();
    this->vnc_client->format.redMax = this->rm_screen.get_red_max();
    this->vnc_client->format.greenShift = this->rm_screen.get_green_offset();
    this->vnc_client->format.greenMax = this->rm_screen.get_green_max();
    this->vnc_client->format.blueShift = this->rm_screen.get_blue_offset();
    this->vnc_client->format.blueMax = this->rm_screen.get_blue_max();

    if (rfbInitClient(this->vnc_client, nullptr, nullptr) == 0)
    {
        throw std::runtime_error{"Failed to initialize VNC connection"};
    }

    // Make sure the server gives us a compatible format
    if (this->vnc_client->width < 0
        || this->vnc_client->height < 0
        || static_cast<unsigned int>(this->vnc_client->width)
            != this->rm_screen.get_xres_memory()
        || static_cast<unsigned int>(this->vnc_client->height)
            > this->rm_screen.get_yres_memory())
    {
        std::stringstream msg;
        msg << "Server uses an unsupported resolution ("
            << this->vnc_client->width << 'x' << this->vnc_client->height
            << "). This client can only cope with a screen width of exactly "
            << this->rm_screen.get_xres_memory() << " pixels and a screen "
            "height of " << this->rm_screen.get_yres() << " pixels";
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
    this->rm_touch.setup_poll(polled_fds[poll_touch]);

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
            handle_status(this->event_loop_input());
        }
    }
}

auto client::event_loop_vnc() -> client::event_loop_status
{
    if (HandleRFBServerMessage(this->vnc_client) == 0)
    {
        return {true, -1};
    }

    return {false, -1};
}
