#include "touch.hpp"
#include <vector>
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

touch::touch(touch&& other) noexcept
: input(std::move(other))
, flip_x(other.flip_x)
, flip_y(other.flip_y)
, state(std::move(other.state))
, current_id(other.current_id)
, x_limits(std::move(other.x_limits))
, y_limits(std::move(other.y_limits))
, pressure_limits(std::move(other.pressure_limits))
, orientation_limits(std::move(other.orientation_limits))
{}

auto touch::operator=(touch&& other) noexcept -> touch&
{
    this->flip_x = other.flip_x;
    this->flip_y = other.flip_y;
    this->state = std::move(other.state);
    this->current_id = other.current_id;
    this->x_limits = std::move(other.x_limits);
    this->y_limits = std::move(other.y_limits);
    this->pressure_limits = std::move(other.pressure_limits);
    this->orientation_limits = std::move(other.orientation_limits);
    input::operator=(std::move(other));
    return *this;
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
