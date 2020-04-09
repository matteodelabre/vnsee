#include "client.hpp"
#include "log.hpp"
#include "rmioc/input.hpp"
#include "rmioc/screen.hpp"
#include <algorithm>
#include <bitset>
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

/**
 * Minimal move in pixels to consider that a touchpoint has been dragged
 * enough to initiate scrolling.
 */
constexpr int scroll_delta = 10;

/**
 * Scroll speed factor.
 */
constexpr double scroll_speed = 0.05;

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

    log::print("VNC update") << w << 'x' << h << '+' << x << '+' << y << '\n';

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
: vnc_client(rfbGetClient(0, 0, 0))
, rm_screen(rm_screen)
, rm_input(rm_input)
, update_info{}
{
    rfbClientLog = vnc_client_log;

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
        "(client::event_loop) Wait for message"
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

            log::print("Screen update")
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

namespace MouseButton
{
    int Left = 1;

    int ScrollDown = 1 << 3;
    int ScrollUp = 1 << 4;
    int ScrollLeft = 1 << 5;
    int ScrollRight = 1 << 6;
}

client::touchpoint_state::touchpoint_state(client& parent, int x, int y)
: parent(parent)
, x(x), y(y)
, x_initial(x), y_initial(y)
{}

void client::touchpoint_state::update(int x, int y)
{
    this->x = x;
    this->y = y;

    // Initiate scrolling if the touchpoint has travelled enough
    if (!this->scrolling())
    {
        if (std::abs(this->x - this->x_initial) >= scroll_delta)
        {
            this->x_scrolling = true;
        }
        else if (std::abs(this->y - this->y_initial) >= scroll_delta)
        {
            this->y_scrolling = true;
        }
    }

    // Send discrete scroll events to reflect travelled distance
    if (this->x_scrolling)
    {
        int x_units = (this->x - this->x_initial) * scroll_speed;

        for (; x_units > this->x_sent_events; ++this->x_sent_events)
        {
            this->send_button_press(
                this->x_sensor_to_screen(this->x_initial),
                this->y_sensor_to_screen(this->y_initial),
                MouseButton::ScrollRight
            );
        }

        for (; x_units < this->x_sent_events; --this->x_sent_events)
        {
            this->send_button_press(
                this->x_sensor_to_screen(this->x_initial),
                this->y_sensor_to_screen(this->y_initial),
                MouseButton::ScrollLeft
            );
        }
    }

    if (this->y_scrolling)
    {
        int y_units = (this->y - this->y_initial) * scroll_speed;

        for (; y_units > this->y_sent_events; ++this->y_sent_events)
        {
            this->send_button_press(
                this->x_sensor_to_screen(this->x_initial),
                this->y_sensor_to_screen(this->y_initial),
                MouseButton::ScrollUp
            );
        }

        for (; y_units < this->y_sent_events; --this->y_sent_events)
        {
            this->send_button_press(
                this->x_sensor_to_screen(this->x_initial),
                this->y_sensor_to_screen(this->y_initial),
                MouseButton::ScrollDown
            );
        }
    }
}

void client::touchpoint_state::terminate()
{
    // Perform tap action if the touchpoint was not used for scrolling
    if (!this->scrolling())
    {
        this->send_button_press(
            this->x_sensor_to_screen(this->x),
            this->y_sensor_to_screen(this->y),
            MouseButton::Left
        );
    }
}

bool client::touchpoint_state::scrolling() const
{
    return this->x_scrolling || this->y_scrolling;
}

int client::touchpoint_state::x_sensor_to_screen(int x_value) const
{
    return this->parent.rm_screen.get_xres()
         - this->parent.rm_screen.get_xres()
         * x_value / rmioc::input::slot_state::x_max;
}

int client::touchpoint_state::y_sensor_to_screen(int y_value) const
{
    return this->parent.rm_screen.get_yres()
         - this->parent.rm_screen.get_yres()
         * y_value / rmioc::input::slot_state::y_max;
}

void client::touchpoint_state::send_button_press(int x, int y, int btn) const
{
    log::print("Button press")
        << x << 'x' << y
        << " (button mask: " << std::bitset<8>(btn) << ")\n";

    SendPointerEvent(this->parent.vnc_client, x, y, btn);
    SendPointerEvent(this->parent.vnc_client, x, y, 0);
}

client::event_loop_status client::event_loop_input()
{
    if (this->rm_input.fetch_events())
    {
        auto slots_state = this->rm_input.get_slots_state();

        // Initialize new touchpoints or update existing ones
        for (const auto& [id, slot] : slots_state)
        {
            auto touchpoint_it = this->touchpoints.find(id);

            if (touchpoint_it == std::end(this->touchpoints))
            {
                this->touchpoints.insert({
                    id, touchpoint_state{*this, slot.x, slot.y}
                });
            }
            else
            {
                touchpoint_it->second.update(slot.x, slot.y);
            }
        }

        // Remove stale touchpoints
        for (
            auto touchpoint_it = std::begin(this->touchpoints);
            touchpoint_it != std::end(this->touchpoints);
        )
        {
            int id = touchpoint_it->first;

            if (!slots_state.count(id))
            {
                touchpoint_it->second.terminate();
                touchpoint_it = this->touchpoints.erase(touchpoint_it);
            }
            else { ++touchpoint_it; }
        }
    }

    return {false, -1};
}
