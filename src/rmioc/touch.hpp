#ifndef RMIOC_TOUCH_HPP
#define RMIOC_TOUCH_HPP

#include "input.hpp"
#include <map>

namespace rmioc
{

/**
 * Access to the state of the device’s touchscreen.
 *
 * See the Linux documentation on multi-touch input:
 * https://www.kernel.org/doc/Documentation/input/multi-touch-protocol.txt
 */
class touch : public input
{
public:
    /**
     * Open the touchscreen device.
     *
     * @param path Path to the device.
     * @param flip_x True to flip coordinates horizontally.
     * @param flip_y True to flip coordinates vertically.
     */
    touch(const char* device_path, bool flip_x = false, bool flip_y = false);

    // Disallow copying touch device handles
    touch(const touch& other) = delete;
    touch& operator=(const touch& other) = delete;

    // Transfer handle ownership
    touch(touch&& other) noexcept;
    touch& operator=(touch&& other) noexcept;

    /**
     * Check for new events.
     *
     * @return True if touch state changed since last call.
     */
    bool process_events();

    /**
     * Information about a touch point on the screen.
     */
    struct touchpoint_state
    {
        /**
         * Horizontal position of the touch point.
         *
         * Integer between 0 and `get_xres()`.
         */
        int x;

        /**
         * Vertical position of the touch point.
         *
         * Integer between 0 and `get_yres()`.
         */
        int y;

        /**
         * Amount of pressure applied on the touch point.
         *
         * Integer between 0 and `get_pressure_res()`.
         */
        int pressure;

        /**
         * Orientation of the touch point.
         *
         * Integer within the range indicated by `get_orientation_limits()`.
         * A positive value indicates clockwise rotation from the
         * Y-axis-aligned default position, a negative one indicates
         * counter-clockwise rotation.
         */
        int orientation;
    };

    using touchpoints_state = std::map<int, touchpoint_state>;

    /**
     * Get the set of currently active touch points indexed by their ID.
     */
    const touchpoints_state& get_state() const;

    /** Get the maximum possible value on the X axis. */
    int get_xres() const;

    /** Get the maximum possible value on the Y axis. */
    int get_yres() const;

    /** Get the maximum pressure value. */
    int get_pressure_res() const;

    /** Get the minimum and maximum possible values of the orientation axis. */
    const std::pair<int, int>& get_orientation_limits() const;

private:
    /** Coordinate flipping state. */
    bool flip_x;
    bool flip_y;

    /** Currently active touch points by ID. */
    touchpoints_state state;

    /** Currently active touch point ID. */
    int current_id = 0;

    /** Minimum and maximum value of the X axis. */
    std::pair<int, int> x_limits;

    /** Minimum and maximum value of the Y axis. */
    std::pair<int, int> y_limits;

    /** Minimum and maximum value of the pressure axis. */
    std::pair<int, int> pressure_limits;

    /** Minimum and maximum value of the orientation axis. */
    std::pair<int, int> orientation_limits;
}; // class touch

} // namespace rmioc

#endif // RMIOC_TOUCH_HPP
