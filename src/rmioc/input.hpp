#ifndef RMIOC_INPUT_HPP
#define RMIOC_INPUT_HPP

#include "flags.hpp"
#include "file.hpp"
#include <utility>
#include <vector>
#include <linux/input.h>

struct pollfd;

namespace rmioc
{

/** Available Linux input events. */
RMIOC_FLAGS_DEFINE(
    input_events,
    syn, key, rel, abs
);

/** Get the set of input events that can be emitted by a device. */
input_events supported_input_events(int input_fd);

/** Available Linux key types. */
RMIOC_FLAGS_DEFINE(
    key_types,
    power, tool_pen, tool_rubber
);

/** Get the set of key codes that can be emitted by a device. */
key_types supported_key_types(int input_fd);

/** Available Linux absolute axes. */
RMIOC_FLAGS_DEFINE(
    abs_types,
    x, y, pressure, distance, tilt_x, tilt_y,
    mt_slot, mt_tracking_id, mt_position_x, mt_position_y,
    mt_pressure, mt_orientation
);

/** Get the set of absolute axes that are supported by a device. */
abs_types supported_abs_types(int input_fd);

/**
 * Generic class for reading Linux input devices.
 *
 * See the Linux documentation on input:
 * https://www.kernel.org/doc/Documentation/input/input.txt
 */
class input
{
public:
    /**
     * Open an input device.
     *
     * @param device_path Path to the device.
     */
    input(const char* device_path);

    /**
     * Setup a pollfd structure to wait for events on the device.
     *
     * @param in_pollfd Structure to modify.
     */
    void setup_poll(pollfd& in_pollfd) const;

protected:
    /**
     * Fetch the next set of events from the device.
     *
     * If no EV_SYN event is available, queue existing events and return an
     * empty set. This function will not block if no events are available on
     * the device.
     *
     * @return Next set of available events.
     */
    std::vector<input_event> fetch_events();

    /**
     * Get the minimum and maximum value of an absolute axis of the device.
     *
     * @param type Type of axis to query (see <linux/input.h> for a list)
     * @return Axis limits.
     */
    std::pair<int, int> get_axis_limits(unsigned int type) const;

private:
    /** File descriptor for the input device. */
    file_descriptor input_fd;

    /** List of queued events. */
    std::vector<input_event> queued_events;
}; // class input

} // namespace rmioc

#endif // RMIOC_INPUT_HPP
