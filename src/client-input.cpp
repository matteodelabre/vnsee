#include "client.hpp"
#include "log.hpp"
#include "rmioc/input.hpp"
#include "rmioc/screen.hpp"
#include <bitset>
#include <rfb/rfbclient.h>

/**
 * Minimal move in pixels to consider that a touchpoint has been dragged
 * enough to initiate scrolling.
 */
constexpr int scroll_delta = 10;

/**
 * Scroll speed factor.
 */
constexpr double scroll_speed = 0.05;

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

/** List of mouse button flags used by the VNC protocol. */
namespace MouseButton
{
    int Left = 1;
    int Right = 1 << 1;
    int Middle = 1 << 2;
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
