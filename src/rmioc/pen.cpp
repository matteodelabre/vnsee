#include "pen.hpp"
#include <vector>
#include <linux/input-event-codes.h>
#include <linux/input.h>

namespace rmioc
{

pen::pen(const char* device_path)
: input(device_path)
, state{}
{}

pen::pen(pen&& other) noexcept
: input(std::move(other))
, state(other.state)
{}

auto pen::operator=(pen&& other) noexcept -> pen&
{
    this->state = other.state;
    input::operator=(std::move(other));
    return *this;
}

auto pen::process_events() -> bool
{
    auto events = this->fetch_events();

    if (!events.empty())
    {
        for (const input_event& event : events)
        {
            switch (event.type)
            {
            case EV_KEY:
                switch (event.code)
                {
                case BTN_TOOL_PEN:
                    this->state.tool_set.set_pen(event.value != 0);
                    break;

                case BTN_TOOL_RUBBER:
                    this->state.tool_set.set_rubber(event.value != 0);
                    break;
                }
                break;

            case EV_ABS:
                switch (event.code)
                {
                case ABS_X:
                    this->state.x = event.value;
                    break;

                case ABS_Y:
                    this->state.y = event.value;
                    break;

                case ABS_PRESSURE:
                    this->state.pressure = event.value;
                    break;

                case ABS_DISTANCE:
                    this->state.distance = event.value;
                    break;

                case ABS_TILT_X:
                    this->state.tilt_x = event.value;
                    break;

                case ABS_TILT_Y:
                    this->state.tilt_y = event.value;
                    break;
                }
                break;
            }
        }

        return true;
    }

    return false;
}

auto pen::get_state() const -> const pen::pen_state&
{
    return this->state;
}

} // namespace rmioc
