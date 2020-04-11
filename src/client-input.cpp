#include "client.hpp"
#include "log.hpp"
#include "rmioc/screen.hpp"
#include "rmioc/touch.hpp"
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <map>
#include <ostream>
#include <utility>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include <type_traits>
// IWYU pragma: no_include "rfb/rfbproto.h"

/**
 * Minimal move in pixels to consider that a touchpoint has been dragged
 * enough to initiate scrolling.
 */
constexpr int scroll_delta = 10;

/**
 * Scroll speed factor.
 */
constexpr double scroll_speed = 0.05;

auto client::event_loop_input() -> client::event_loop_status
{
    if (this->rm_touch.process_events())
    {
        auto slots_state = this->rm_touch.get_slots_state();

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

            if (slots_state.count(id) == 0U)
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
    constexpr std::uint8_t Left = 1;
    // constexpr std::uint8_t Right = 1U << 1U;
    // constexpr std::uint8_t Middle = 1U << 2U;
    constexpr std::uint8_t ScrollDown = 1U << 3U;
    constexpr std::uint8_t ScrollUp = 1U << 4U;
    constexpr std::uint8_t ScrollLeft = 1U << 5U;
    constexpr std::uint8_t ScrollRight = 1U << 6U;
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
        int x_units = static_cast<int>(
            (this->x - this->x_initial) * scroll_speed);

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
        int y_units = static_cast<int>(
            (this->y - this->y_initial) * scroll_speed);

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

auto client::touchpoint_state::scrolling() const -> bool
{
    return this->x_scrolling || this->y_scrolling;
}

auto client::touchpoint_state::x_sensor_to_screen(int x_value) const -> int
{
    int xres = static_cast<int>(this->parent.rm_screen.get_xres());
    return xres - xres * x_value / rmioc::touch::slot_state::x_max;
}

auto client::touchpoint_state::y_sensor_to_screen(int y_value) const -> int
{
    int yres = static_cast<int>(this->parent.rm_screen.get_yres());
    return yres - yres * y_value / rmioc::touch::slot_state::y_max;
}

void client::touchpoint_state::send_button_press(
    int x, int y,
    std::uint8_t btn
) const
{
    constexpr auto bits = 8 * sizeof(btn);

    log::print("Button press")
        << x << 'x' << y << " (button mask: "
        << std::setfill('0') << std::setw(bits)
        << std::bitset<bits>(btn) << ")\n";

    SendPointerEvent(this->parent.vnc_client, x, y, btn);
    SendPointerEvent(this->parent.vnc_client, x, y, 0);
}
