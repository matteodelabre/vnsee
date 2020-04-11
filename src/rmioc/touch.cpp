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
                this->current_slot = event.value;
                break;

            case ABS_MT_TRACKING_ID:
                if (event.value == -1)
                {
                    // Destroy current touch point slot
                    this->slots_state.erase(this->current_slot);
                }
                else
                {
                    // Create touch point slot
                    this->slots_state[this->current_slot] = {};
                }
                break;

            case ABS_MT_POSITION_X:
                this->slots_state[this->current_slot].x = event.value;
                break;

            case ABS_MT_POSITION_Y:
                this->slots_state[this->current_slot].y = event.value;
                break;

            case ABS_MT_PRESSURE:
                this->slots_state[this->current_slot].pressure
                    = event.value;
                break;

            case ABS_MT_ORIENTATION:
                this->slots_state[this->current_slot].orientation
                    = event.value;
                break;
            }
        }

        return true;
    }

    return false;
}

auto touch::get_slots_state() const -> const touch::slots_state_t&
{
    return this->slots_state;
}

} // namespace rmioc
