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

/**
 * Tag used for storing in/retrieving from the client object
 * the structure in which updates are accumulated.
 */
constexpr auto update_info_tag = 1;

/**
 * Time to wait after the last update from VNC before updating the screen
 * (in microseconds).
 *
 * VNC servers tend to send a lot of small updates in a short period of time.
 * This delay allows grouping those small updates into a larger screen update.
 */
constexpr chrono::milliseconds update_delay{150};

rfbBool create_framebuf(rfbClient*)
{
    // No-op: Data is written directly to the memory-mapped framebuffer
    return true;
}

/**
 * Register an update from the server.
 *
 * @param client Handle to the VNC client.
 * @param x Left bound of the updated rectangle (in pixels).
 * @param y Top bound of the updated rectangle (in pixels).
 * @param w Width of the updated rectangle (in pixels).
 * @param h Height of the updated rectangle (in pixels).
 */
void update_framebuf(rfbClient* client, int x, int y, int w, int h)
{
    // Register the region as pending update, potentially extending
    // an existing one
    client::update_info_struct* update_info
        = reinterpret_cast<client::update_info_struct*>(
                rfbClientGetClientData(
                    client,
                    reinterpret_cast<void*>(update_info_tag)
                ));

    log::print("VNC Update") << w << 'x' << h << '+' << x << '+' << y << '\n';

    if (update_info->has_update)
    {
        // Merge new rectangle with existing one
        int left_x = std::min(x, update_info->x);
        int top_y = std::min(y, update_info->y);
        int right_x = std::max(x + w, update_info->x + update_info->w);
        int bottom_y = std::max(y + h, update_info->y + update_info->h);

        update_info->x = left_x;
        update_info->y = top_y;
        update_info->w = right_x - left_x;
        update_info->h = bottom_y - top_y;
    }
    else
    {
        update_info->x = x;
        update_info->y = y;
        update_info->w = w;
        update_info->h = h;
        update_info->has_update = 1;
    }

    update_info->last_update_time = chrono::steady_clock::now();
}

client::client(
    const char* ip, int port,
    rmioc::screen& rm_screen,
    rmioc::input& rm_input
)
: vnc_client(rfbGetClient(0, 0, 0))
, rm_screen(rm_screen)
, rm_input(rm_input)
, update_info{}
{
    rfbClientSetClientData(
        this->vnc_client,
        reinterpret_cast<void*>(update_info_tag),
        &this->update_info
    );

    free(this->vnc_client->serverHost);
    this->vnc_client->serverHost = strdup(ip);
    this->vnc_client->serverPort = port;

    this->vnc_client->MallocFrameBuffer = create_framebuf;
    this->vnc_client->GotFrameBufferUpdate = update_framebuf;

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
            "height no larger than " << this->rm_screen.get_yres_memory()
            << " pixels";
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
    while (!quit && poll(polled_fds, count_fds, timeout) != -1)
    {
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

    throw std::system_error(
        errno,
        std::generic_category(),
        "(client::start) Wait for message"
    );
}

client::event_loop_status client::event_loop_vnc()
{
    if (!HandleRFBServerMessage(this->vnc_client))
    {
        return {true, -1};
    }

    return {false, -1};
}

client::event_loop_status client::event_loop_screen()
{
    if (this->update_info.has_update)
    {
        int remaining_wait_time =
            chrono::duration_cast<chrono::milliseconds>(
                this->update_info.last_update_time + update_delay
                - chrono::steady_clock::now()
            ).count();

        if (remaining_wait_time <= 0)
        {
            this->update_info.has_update = 0;

            log::print("Screen Update")
                << this->update_info.w << 'x' << this->update_info.h << '+'
                << this->update_info.x << '+' << this->update_info.y << '\n';

            this->rm_screen.update(
                this->update_info.x, this->update_info.y,
                this->update_info.w, this->update_info.h
            );
        }
        else
        {
            // Wait until the next update is due
            return {false, remaining_wait_time};
        }
    }

    return {false, -1};
}

client::event_loop_status client::event_loop_input()
{
    if (this->rm_input.fetch_events())
    {
        auto prev_state = this->rm_input.get_previous_slots_state();
        auto cur_state = this->rm_input.get_slots_state();

        // Map touch coordinates to screen coordinates
        auto x_slot_to_screen = [this](int x)
        {
            return this->rm_screen.get_xres() - this->rm_screen.get_xres()
                * x / rmioc::input::slot_state::x_max;
        };

        auto y_slot_to_screen = [this](int y)
        {
            return this->rm_screen.get_yres() - this->rm_screen.get_yres()
                * y / rmioc::input::slot_state::y_max;
        };

        for (const auto& [id, cur_slot] : cur_state)
        {
            SendPointerEvent(
                this->vnc_client,
                x_slot_to_screen(cur_slot.x),
                y_slot_to_screen(cur_slot.y),
                0
            );
        }

        for (const auto& [id, prev_slot] : prev_state)
        {
            if (!cur_state.count(id))
            {
                SendPointerEvent(
                    this->vnc_client,
                    x_slot_to_screen(prev_slot.x),
                    y_slot_to_screen(prev_slot.y),
                    0
                );
            }
        }
    }

    return {false, -1};
}
