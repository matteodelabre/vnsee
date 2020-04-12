#include "client.hpp"
#include "log.hpp"
#include "rmioc/screen.hpp"
#include "rmioc/touch.hpp"
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <ostream>
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
constexpr double scroll_speed = 0.013;

auto client::event_loop_input() -> client::event_loop_status
{
    if (this->rm_touch.process_events())
    {
        auto touch_state = this->rm_touch.get_state();

        if (!touch_state.empty())
        {
            // Compute the mean touch position
            int summed_x = 0;
            int summed_y = 0;
            int total_points = touch_state.size();

            for (const auto& [id, slot] : touch_state)
            {
                summed_x += slot.x;
                summed_y += slot.y;
            }

            summed_x /= total_points;
            summed_y /= total_points;

            // Convert to screen coordinates
            int xres = static_cast<int>(this->rm_screen.get_xres());
            int yres = static_cast<int>(this->rm_screen.get_yres());

            int screen_x = (
                xres - xres * summed_x
                / rmioc::touch::touchpoint_state::x_max
            );

            int screen_y = (
                yres - yres * summed_y
                / rmioc::touch::touchpoint_state::y_max
            );

            this->on_touch_update(screen_x, screen_y);
        }
        else
        {
            this->on_touch_end();
        }
    }

    return {false, -1};
}

void client::on_touch_update(int x, int y)
{
    if (this->touch_state == TouchState::Inactive)
    {
        this->touch_state = TouchState::Tap;
        this->touch_x_initial = x;
        this->touch_y_initial = y;
        this->touch_x_scroll_events = 0;
        this->touch_y_scroll_events = 0;
    }

    this->touch_x = x;
    this->touch_y = y;

    // Initiate scrolling if the touchpoint has travelled enough
    if (this->touch_state == TouchState::Tap)
    {
        if (std::abs(this->touch_x - this->touch_x_initial) >= scroll_delta)
        {
            this->touch_state = TouchState::ScrollX;
        }
        else if (std::abs(this->touch_y - this->touch_y_initial)
                    >= scroll_delta)
        {
            this->touch_state = TouchState::ScrollY;
        }
    }

    // Send discrete scroll events to reflect travelled distance
    if (this->touch_state == TouchState::ScrollX)
    {
        int x_units = static_cast<int>(
            (this->touch_x - this->touch_x_initial) * scroll_speed);

        for (; x_units > this->touch_x_scroll_events;
               ++this->touch_x_scroll_events)
        {
            this->send_button_press(
                this->touch_x_initial,
                this->touch_y_initial,
                MouseButton::ScrollRight
            );
        }

        for (; x_units < this->touch_x_scroll_events;
               --this->touch_x_scroll_events)
        {
            this->send_button_press(
                this->touch_x_initial,
                this->touch_y_initial,
                MouseButton::ScrollLeft
            );
        }
    }

    if (this->touch_state == TouchState::ScrollY)
    {
        int y_units = static_cast<int>(
            (this->touch_y - this->touch_y_initial) * scroll_speed);

        for (; y_units > this->touch_y_scroll_events;
               ++this->touch_y_scroll_events)
        {
            this->send_button_press(
                this->touch_x_initial,
                this->touch_y_initial,
                MouseButton::ScrollDown
            );
        }

        for (; y_units < this->touch_y_scroll_events;
               --this->touch_y_scroll_events)
        {
            this->send_button_press(
                this->touch_x_initial,
                this->touch_y_initial,
                MouseButton::ScrollUp
            );
        }
    }
}

void client::on_touch_end()
{
    // Perform tap action if the touchpoint was not used for scrolling
    if (this->touch_state == TouchState::Tap)
    {
        this->send_button_press(
            this->touch_x_initial,
            this->touch_y_initial,
            MouseButton::Left
        );
    }

    this->touch_state = TouchState::Inactive;
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
