#include "pen.hpp"
#include <vector>
#include <linux/input-event-codes.h>
#include <linux/input.h>

// NOTE: The pen coordinate system is rotated 90 degrees anti-clockwise
// relative to the screen coordinate system. This fact is hard-coded in this
// class (by swapping the X and Y axes), since both the rM1 and the rM2 share
// this characteristic.

namespace rmioc
{

pen::pen(const char* device_path, bool flip_x, bool flip_y)
: input(device_path)
, flip_x(flip_x)
, flip_y(flip_y)
, x_limits(this->get_axis_limits(ABS_Y))
, y_limits(this->get_axis_limits(ABS_X))
, pressure_limits(this->get_axis_limits(ABS_PRESSURE))
, distance_limits(this->get_axis_limits(ABS_DISTANCE))
, tilt_x_limits(this->get_axis_limits(ABS_TILT_Y))
, tilt_y_limits(this->get_axis_limits(ABS_TILT_X))
{}

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
                case ABS_Y:
                    this->state.x = this->flip_x
                        ? (this->x_limits.second - event.value)
                        : (event.value - this->x_limits.first);
                    break;

                case ABS_X:
                    this->state.y = this->flip_y
                        ? (this->y_limits.second - event.value)
                        : (event.value - this->y_limits.first);
                    break;

                case ABS_PRESSURE:
                    this->state.pressure
                        = event.value - this->pressure_limits.first;
                    break;

                case ABS_DISTANCE:
                    this->state.distance
                        = event.value - this->distance_limits.first;
                    break;

                case ABS_TILT_Y:
                    this->state.tilt_x = event.value;
                    break;

                case ABS_TILT_X:
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

auto pen::get_xres() const -> int
{
    return this->x_limits.second - this->x_limits.first;
}

auto pen::get_yres() const -> int
{
    return this->y_limits.second - this->x_limits.first;
}

auto pen::get_pressure_res() const -> int
{
    return this->pressure_limits.second - this->pressure_limits.first;
}

auto pen::get_distance_res() const -> int
{
    return this->distance_limits.second - this->distance_limits.first;
}

auto pen::get_tilt_x_limits() const -> const std::pair<int, int>&
{
    return this->tilt_x_limits;
}

auto pen::get_tilt_y_limits() const -> const std::pair<int, int>&
{
    return this->tilt_y_limits;
}

} // namespace rmioc
