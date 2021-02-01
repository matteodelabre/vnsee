#include "buttons.hpp"
#include <vector>
#include <linux/input-event-codes.h>
#include <linux/input.h>

namespace rmioc
{

buttons::buttons(const char* device_path)
: input(device_path)
{}

buttons::buttons(buttons&& other) noexcept
: input(std::move(other))
{}

auto buttons::operator=(buttons&& other) noexcept -> buttons&
{
    input::operator=(std::move(other));
    return *this;
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
