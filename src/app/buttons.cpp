#include "buttons.hpp"
#include "../rmioc/buttons.hpp"
#include "../rmioc/screen.hpp"

namespace app
{

buttons::buttons(
    rmioc::buttons& device,
    rmioc::screen& screen_device
)
: device(device)
, screen_device(screen_device)
, previous_state{}
{}

auto buttons::event_loop() -> event_loop_status
{
    if (this->device.process_events())
    {
        auto device_state = this->device.get_state();

        if (!device_state.power && this->previous_state.power)
        {
            // Quit application when pressing power
            return {/* quit = */ true, /* timeout = */ -1};
        }

        if (!device_state.home && this->previous_state.home)
        {
            // Full screen refresh when pressing home
            this->screen_device.update();
        }

        this->previous_state = device_state;
    }

    return {/* quit = */ false, /* timeout = */ -1};
}

} // namespace app
