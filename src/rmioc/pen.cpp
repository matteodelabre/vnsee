#include "pen.hpp"
#include <vector>
#include <linux/input-event-codes.h>
#include <linux/input.h>

namespace rmioc
{

pen::pen()
: input("/dev/input/event0")
, state{}
{}

auto pen::pen_state::ToolSet::pen() const -> bool    { return this->set[0]; }
void pen::pen_state::ToolSet::pen(bool state)        { this->set[0] = state; }
auto pen::pen_state::ToolSet::rubber() const -> bool { return this->set[1]; }
void pen::pen_state::ToolSet::rubber(bool state)     { this->set[1] = state; }

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
                    this->state.tool_set.pen(event.value != 0);
                    break;

                case BTN_TOOL_RUBBER:
                    this->state.tool_set.rubber(event.value != 0);
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
