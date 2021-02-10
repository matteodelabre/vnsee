#include "pen.hpp"
#include "screen.hpp"
#include "../rmioc/pen.hpp"
#include <functional>
#include <utility>
// IWYU pragma: no_include <type_traits>

namespace app
{

pen::pen(
    rmioc::pen& device,
    app::screen& screen,
    MouseCallback send_button_press
)
: device(device)
, screen(screen)
, send_button_press(std::move(send_button_press))
, state(MouseButton::None)
{}

auto pen::process_events() -> event_loop_status
{
    if (this->device.process_events())
    {
        auto device_state = this->device.get_state();

        if (device_state.tool_set.has_pen())
        {
            // Convert to screen coordinates
            int screen_xres = static_cast<int>(this->screen.get_xres());
            int screen_yres = static_cast<int>(this->screen.get_yres());

            int pen_xres = this->device.get_xres();
            int pen_yres = this->device.get_yres();

            int screen_x = device_state.x * screen_xres / pen_xres;
            int screen_y = device_state.y * screen_yres / pen_yres;

            // Move the mouse cursor to the pen position and generate a click
            // if the pen is touching the screen
            MouseButton new_state = device_state.pressure > 0
                ? MouseButton::Left
                : MouseButton::None;

            this->send_button_press(screen_x, screen_y, new_state);

            // Switch to the fast update mode for as long as the pen
            // touches the screen
            if (this->state != new_state)
            {
                if (new_state == MouseButton::Left)
                {
                    this->screen.set_repaint_mode(screen::repaint_modes::fast);
                }
                else
                {
                    this->screen.set_repaint_mode(
                        screen::repaint_modes::standard);
                    this->screen.repaint();
                }
            }

            this->state = new_state;
        }
    }

    return {/* quit = */ false, /* timeout = */ -1};
}

auto pen::is_inhibiting() const -> bool
{
    // Inhibit other forms of inputs when the pen is active
    return this->device.get_state().tool_set.has_pen();
}

} // namespace app
