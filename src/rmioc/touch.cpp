#include "touch.hpp"
#include <vector>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>

namespace rmioc
{

touch::touch(const char* device_path, bool flip_x, bool flip_y)
: input(device_path)
, flip_x(flip_x)
, flip_y(flip_y)
, x_limits(this->get_axis_limits(ABS_MT_POSITION_X))
, y_limits(this->get_axis_limits(ABS_MT_POSITION_Y))
, pressure_limits(this->get_axis_limits(ABS_MT_PRESSURE))
, orientation_limits(this->get_axis_limits(ABS_MT_ORIENTATION))
{}

auto touch::is(const char* device_path) -> bool
{
    file_descriptor input_fd{device_path, O_RDONLY};
    auto supp_events = supported_input_events(input_fd);

    if (!supp_events.has_abs())
    {
        return false;
    }

    auto supp_axes = supported_abs_types(input_fd);

    return (
        supp_axes.has_mt_slot()
        && supp_axes.has_mt_tracking_id()
        && supp_axes.has_mt_position_x()
        && supp_axes.has_mt_position_y()
        && supp_axes.has_mt_pressure()
        && supp_axes.has_mt_orientation()
    );
}

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
                this->state[this->current_id].x = this->flip_x
                    ? (this->x_limits.second - event.value)
                    : (event.value - this->x_limits.first);
                break;

            case ABS_MT_POSITION_Y:
                this->state[this->current_id].y = this->flip_y
                    ? (this->y_limits.second - event.value)
                    : (event.value - this->y_limits.first);
                break;

            case ABS_MT_PRESSURE:
                this->state[this->current_id].pressure
                    = event.value - this->pressure_limits.first;
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

auto touch::get_xres() const -> int
{
    return this->x_limits.second - this->x_limits.first;
}

auto touch::get_yres() const -> int
{
    return this->y_limits.second - this->x_limits.first;
}

auto touch::get_pressure_res() const -> int
{
    return this->pressure_limits.second - this->pressure_limits.first;
}

auto touch::get_orientation_limits() const -> const std::pair<int, int>&
{
    return this->orientation_limits;
}

} // namespace rmioc
