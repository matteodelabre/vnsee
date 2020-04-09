#include "client.hpp"
#include "log.hpp"
#include "rmioc/input.hpp"
#include "rmioc/screen.hpp"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <fcntl.h>
#include <poll.h>
#include <rfb/rfbclient.h>
#include <unistd.h>
// IWYU pragma: no_include <type_traits>
// IWYU pragma: no_include <rfb/rfbproto.h>

namespace chrono = std::chrono;

/** Custom log printer for the VNC client library.  */
void vnc_client_log(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    ssize_t buffer_size = vsnprintf(NULL, 0, format, args);
    char* buffer = new char[buffer_size + 1];
    vsnprintf(buffer, buffer_size + 1, format, args);

    log::print("VNC message") << buffer;

    delete[] buffer;
    va_end(args);
}

client::client(
    const char* ip, int port,
    rmioc::screen& rm_screen,
    rmioc::input& rm_input
)
: update_info{}
, vnc_client(rfbGetClient(0, 0, 0))
, rm_screen(rm_screen)
, rm_input(rm_input)
{
    rfbClientLog = vnc_client_log;
    rfbClientErr = vnc_client_log;

    rfbClientSetClientData(
        this->vnc_client,
        reinterpret_cast<void*>(client::update_info_tag),
        &this->update_info
    );

    free(this->vnc_client->serverHost);
    this->vnc_client->serverHost = strdup(ip);
    this->vnc_client->serverPort = port;

    this->vnc_client->MallocFrameBuffer = client_create_framebuf;
    this->vnc_client->GotFrameBufferUpdate = client_update_framebuf;

    // Configure connection with device framebuffer settings
    this->vnc_client->frameBuffer = this->rm_screen.get_data();
    this->vnc_client->format.bitsPerPixel = this->rm_screen.get_bits_per_pixel();
    this->vnc_client->format.redShift = this->rm_screen.get_red_offset();
    this->vnc_client->format.redMax = this->rm_screen.get_red_max();
    this->vnc_client->format.greenShift = this->rm_screen.get_green_offset();
    this->vnc_client->format.greenMax = this->rm_screen.get_green_max();
    this->vnc_client->format.blueShift = this->rm_screen.get_blue_offset();
    this->vnc_client->format.blueMax = this->rm_screen.get_blue_max();

    if (!rfbInitClient(this->vnc_client, nullptr, nullptr))
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
    pollfd polled_fds[2];
    auto count_fds = sizeof(polled_fds) / sizeof(pollfd);

    int poll_vnc = 0;
    polled_fds[poll_vnc].fd = this->vnc_client->sock;
    polled_fds[poll_vnc].events = POLLIN;

    int poll_input = 1;
    polled_fds[poll_input].fd = this->rm_input.get_device_fd();
    polled_fds[poll_input].events = POLLIN;

    // Maximum time to wait before timeout in the next poll
    int timeout = -1;

    // Flag used for quitting the event loop
    bool quit = false;

    auto handle_status = [this, &timeout, &quit](const event_loop_status& st)
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
        while (poll(polled_fds, count_fds, timeout) == -1)
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

        if (polled_fds[poll_vnc].revents & POLLIN)
        {
            handle_status(this->event_loop_vnc());
        }

        handle_status(this->event_loop_screen());

        if (polled_fds[poll_input].revents & POLLIN)
        {
            handle_status(this->event_loop_input());
        }
    }
}

client::event_loop_status client::event_loop_vnc()
{
    if (!HandleRFBServerMessage(this->vnc_client))
    {
        return {true, -1};
    }

    return {false, -1};
}
