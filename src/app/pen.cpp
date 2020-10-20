#include "pen.hpp"
#include "../rmioc/pen.hpp"
#include "../rmioc/screen.hpp"
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
{}

auto pen::process_events() -> event_loop_status
{
    if (this->device.process_events())
    {
        auto device_state = this->device.get_state();

        if (device_state.tool_set.pen())
        {
            // Convert to screen coordinates
            int xres = static_cast<int>(this->screen.get_xres());
            int yres = static_cast<int>(this->screen.get_yres());

            int screen_x = device_state.y * xres
                / rmioc::pen::pen_state::y_max;

            int screen_y = yres - yres * device_state.x
                / rmioc::pen::pen_state::x_max;

            // Move cursor to pen position, generate a click if the pen is
            // touching the screen
            this->send_button_press(
                screen_x, screen_y,
                device_state.pressure > 0
                    ? MouseButton::Left
                    : MouseButton::None
            );
        }
    }

    return {/* quit = */ false, /* timeout = */ -1};
}

auto pen::is_inhibiting() const -> bool
{
    // Inhibit other forms of inputs when the pen is active
    return this->device.get_state().tool_set.pen();
}

} // namespace app
