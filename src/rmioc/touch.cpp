#include "touch.hpp"
#include <vector>
#include <linux/input-event-codes.h>
#include <linux/input.h>

namespace rmioc
{

touch::touch()
: input("/dev/input/event1")
{}

auto touch::process_events() -> bool
{
    auto events = this->fetch_events();

    if (!events.empty())
    {
        for (const input_event& event : events)
        {
            switch (event.code)
            {
            case ABS_MT_SLOT:
                this->current_id = event.value;
                break;

            case ABS_MT_TRACKING_ID:
                if (event.value == -1)
                {
                    // Destroy current touch point
                    this->state.erase(this->current_id);
                }
                else
                {
                    // Create a new touch point
                    this->state[this->current_id] = {};
                }
                break;

            case ABS_MT_POSITION_X:
                this->state[this->current_id].x = event.value;
                break;

            case ABS_MT_POSITION_Y:
                this->state[this->current_id].y = event.value;
                break;

            case ABS_MT_PRESSURE:
                this->state[this->current_id].pressure = event.value;
                break;

            case ABS_MT_ORIENTATION:
                this->state[this->current_id].orientation = event.value;
                break;
            }
        }

        return true;
    }

    return false;
}

auto touch::get_state() const -> const touch::touchpoints_state&
{
    return this->state;
}

} // namespace rmioc
