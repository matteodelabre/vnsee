#include "touch.hpp"
#include "../rmioc/screen.hpp"
#include "../rmioc/touch.hpp"
#include <cstdlib>
#include <functional>
#include <map>
#include <utility>
// IWYU pragma: no_include <ratio>
// IWYU pragma: no_include <type_traits>

namespace app
{

/**
 * Minimal move in pixels to consider that a touchpoint has been dragged
 * enough to initiate scrolling.
 */
constexpr int scroll_delta = 10;

/**
 * Duration of a tap to make it a right click action.
 */
constexpr auto right_click_time = std::chrono::milliseconds{500};

/**
 * Number of scroll events to send per screen pixel that is dragged.
 */
constexpr double scroll_speed = 0.013;

touch::touch(
    rmioc::touch& device,
    const rmioc::screen& screen_device,
    MouseCallback send_button_press
)
: device(device)
, screen_device(screen_device)
, send_button_press(std::move(send_button_press))
{}

auto touch::process_events(bool inhibit) -> event_loop_status
{
    if (this->device.process_events())
    {
        if (inhibit)
        {
            this->state = TouchState::Inactive;
        }
        else
        {
            auto device_state = this->device.get_state();

            if (!device_state.empty())
            {
                // Compute the mean touch position
                int summed_x = 0;
                int summed_y = 0;
                int total_points = device_state.size();

                for (const auto& [id, slot] : device_state)
                {
                    summed_x += slot.x;
                    summed_y += slot.y;
                }

                summed_x /= total_points;
                summed_y /= total_points;

                // Convert to screen coordinates
                int xres = static_cast<int>(this->screen_device.get_xres());
                int yres = static_cast<int>(this->screen_device.get_yres());

                int screen_x = (
                    xres - xres * summed_x
                    / rmioc::touch::touchpoint_state::x_max
                );

                int screen_y = (
                    yres - yres * summed_y
                    / rmioc::touch::touchpoint_state::y_max
                );

                this->on_update(screen_x, screen_y);
            }
            else
            {
                this->on_end();
            }
        }
    }

    return {/* quit = */ false, /* timeout = */ -1};
}

void touch::on_update(int x, int y)
{
    if (this->state == TouchState::Inactive)
    {
        this->state = TouchState::Tap;
        this->touch_start = std::chrono::steady_clock::now();
        this->x_initial = x;
        this->y_initial = y;
        this->x_scroll_events = 0;
        this->y_scroll_events = 0;
    }

    this->x = x;
    this->y = y;

    // Initiate scrolling if the touchpoint has travelled enough
    if (this->state == TouchState::Tap)
    {
        if (std::abs(this->x - this->x_initial) >= scroll_delta)
        {
            this->state = TouchState::ScrollX;
        }
        else if (std::abs(this->y - this->y_initial) >= scroll_delta)
        {
            this->state = TouchState::ScrollY;
        }
    }

    // Send discrete scroll events to reflect travelled distance
    if (this->state == TouchState::ScrollX)
    {
        int x_units = static_cast<int>(
            (this->x - this->x_initial) * scroll_speed);

        for (; x_units > this->x_scroll_events; ++this->x_scroll_events)
        {
            this->send_button_press(
                this->x_initial, this->y_initial,
                MouseButton::ScrollRight
            );

            this->send_button_press(
                this->x_initial, this->y_initial,
                MouseButton::None
            );
        }

        for (; x_units < this->x_scroll_events; --this->x_scroll_events)
        {
            this->send_button_press(
                this->x_initial, this->y_initial,
                MouseButton::ScrollLeft
            );

            this->send_button_press(
                this->x_initial, this->y_initial,
                MouseButton::None
            );
        }
    }

    if (this->state == TouchState::ScrollY)
    {
        int y_units = static_cast<int>(
            (this->y - this->y_initial) * scroll_speed);

        for (; y_units > this->y_scroll_events; ++this->y_scroll_events)
        {
            this->send_button_press(
                this->x_initial, this->y_initial,
                MouseButton::ScrollDown
            );

            this->send_button_press(
                this->x_initial, this->y_initial,
                MouseButton::None
            );
        }

        for (; y_units < this->y_scroll_events; --this->y_scroll_events)
        {
            this->send_button_press(
                this->x_initial, this->y_initial,
                MouseButton::ScrollUp
            );

            this->send_button_press(
                this->x_initial, this->y_initial,
                MouseButton::None
            );
        }
    }
}

void touch::on_end()
{
    // Perform tap action if the touchpoint was not used for scrolling
    if (this->state == TouchState::Tap)
    {
        auto touch_duration
            = std::chrono::steady_clock::now().time_since_epoch()
            - this->touch_start.time_since_epoch();

        this->send_button_press(
            this->x_initial, this->y_initial,
            touch_duration < right_click_time
                ? MouseButton::Left
                : MouseButton::Right
        );

        this->send_button_press(
            this->x_initial, this->y_initial,
            MouseButton::None
        );
    }

    this->state = TouchState::Inactive;
}

} // namespace app
