#include "pen.hpp"
#include "../rmioc/pen.hpp"
#include "../rmioc/screen.hpp"
#include <iostream>
#include <utility>
// IWYU pragma: no_include <type_traits>

namespace app
{

pen::pen(
    rmioc::pen& device,
    const rmioc::screen& screen_device,
    MouseCallback send_button_press
)
: device(device)
, screen_device(screen_device)
, send_button_press(std::move(send_button_press))
{}

auto pen::event_loop() -> event_loop_status
{
    if (this->device.process_events())
    {
        auto device_state = this->device.get_state();

        // Convert to screen coordinates
        int xres = static_cast<int>(this->screen_device.get_xres());
        int yres = static_cast<int>(this->screen_device.get_yres());

        int screen_x = device_state.y * xres / rmioc::pen::pen_state::y_max;
        int screen_y = (
            yres - yres * device_state.x
            / rmioc::pen::pen_state::x_max
        );

        // Move cursor to pen position, click if pen is touching screen
        this->send_button_press(
            screen_x, screen_y,
            device_state.pressure > 0 ? MouseButton::Left : MouseButton::None
        );
    }

    return {/* quit = */ false, /* timeout = */ -1};
}

} // namespace app
