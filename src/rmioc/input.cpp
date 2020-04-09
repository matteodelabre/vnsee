#include "input.hpp"
#include <cerrno>
#include <system_error>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <unistd.h>

namespace rmioc
{

input::input()
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,hicpp-signed-bitwise)
: input_fd(open("/dev/input/event1", O_RDONLY | O_NONBLOCK))
{
    if (this->input_fd == -1)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::input) Open input device"
        );
    }
}

input::~input()
{
    close(this->input_fd);
}

auto input::get_device_fd() -> int
{
    return this->input_fd;
}

auto input::fetch_events() -> bool
{
    // See the Linux input protocol documentation
    // https://www.kernel.org/doc/Documentation/input/input.txt
    // https://www.kernel.org/doc/Documentation/input/multi-touch-protocol.txt

    bool has_changes = false;
    input_event current_event{};

    while (read(this->input_fd, &current_event, sizeof(current_event)) != -1)
    {
        if (current_event.type == EV_SYN)
        {
            has_changes = true;

            for (const input_event& event : this->pending_events)
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

            this->pending_events.clear();
        }
        else
        {
            this->pending_events.push_back(current_event);
        }
    }

    if (errno != EAGAIN)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "(rmioc::input) Open input device"
        );
    }

    return has_changes;
}

auto input::get_slots_state() const -> const input::slots_state_t&
{
    return this->slots_state;
}

} // namespace rmioc
