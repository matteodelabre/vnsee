#include "buttons.hpp"
#include <vector>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>

namespace rmioc
{

buttons::buttons(const char* device_path)
: input(device_path)
{}

auto buttons::is(const char* device_path) -> bool
{
    file_descriptor input_fd{device_path, O_RDONLY};
    auto supp_events = supported_input_events(input_fd);

    if (!supp_events.has_key())
    {
        return false;
    }

    auto supp_keys = supported_key_types(input_fd);
    return supp_keys.has_power();
}

auto buttons::process_events() -> bool
{
    auto events = this->fetch_events();

    if (!events.empty())
    {
        for (const input_event& event : events)
        {
            if (event.type == EV_KEY)
            {
                switch (event.code)
                {
                case KEY_LEFT:
                    this->state.left = (event.value != 0);
                    break;

                case KEY_HOME:
                    this->state.home = (event.value != 0);
                    break;

                case KEY_RIGHT:
                    this->state.right = (event.value != 0);
                    break;

                case KEY_POWER:
                    this->state.power = (event.value != 0);
                    break;
                }
            }
        }

        return true;
    }

    return false;
}

auto buttons::get_state() const -> const buttons::buttons_state&
{
    return this->state;
}

} // namespace rmioc
