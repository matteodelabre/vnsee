#ifndef RMIOC_INPUT_HPP
#define RMIOC_INPUT_HPP

#include <vector>
#include <linux/input.h>

struct pollfd;

namespace rmioc
{

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

    /** Close the input device. */
    ~input();

    // Disallow copying input device handles
    input(const input& other) = delete;
    input& operator=(const input& other) = delete;

    // Transfer handle ownership
    input(input&& other) noexcept;
    input& operator=(input&& other) noexcept;

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
    int input_fd;

    /** List of queued events. */
    std::vector<input_event> queued_events;
}; // class input

} // namespace rmioc

#endif // RMIOC_INPUT_HPP
