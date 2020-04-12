#ifndef RMIOC_TOUCH_HPP
#define RMIOC_TOUCH_HPP

#include "input.hpp"
#include <map>

namespace rmioc
{

/**
 * Access to the state of the device’s touchscreen.
 */
class touch : public input
{
public:
    touch();

    /**
     * Check for new events.
     *
     * @return True if touch state changed since last call.
     */
    bool process_events();

    /**
     * Information about a touch point on the screen.
     *
     * Coordinates are in the sensor’s frame, which has its origin on the
     * bottom right of the screen and increases in the upper and left
     * direction.
     *
     *     (767, 1023) ← (0, 1023)
     *     |                     |
     *     ↑                     ↑
     *     |                     |
     *     (767, 0) ———←——— (0, 0)
     */
    struct touchpoint_state
    {
        /** Horizontal position of the touch point. */
        int x;

        static constexpr int x_min = 0;
        static constexpr int x_max = 767;

        /** Vertical position of the touch point. */
        int y;

        static constexpr int y_min = 0;
        static constexpr int y_max = 1023;

        /** Amount of pressure applied on the touch point. */
        int pressure;

        static constexpr int pressure_min = 0;
        static constexpr int pressure_max = 255;

        /**
         * Orientation of the touch point.
         *
         * A signed value indicating clockwise rotation from the Y-axis-aligned
         * default position.
         */
        int orientation;

        static constexpr int orientation_min = -127;
        static constexpr int orientation_max = 127;
    };

    using touchpoints_state = std::map<int, touchpoint_state>;

    /**
     * Get the set of currently active touch points indexed by their ID.
     */
    const touchpoints_state& get_state() const;

private:
    touchpoints_state state;

    /** Currently active touch point ID. */
    int current_id = 0;
}; // class touch

} // namespace rmioc

#endif // RMIOC_TOUCH_HPP
