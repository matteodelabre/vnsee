#ifndef RMIOC_PEN_HPP
#define RMIOC_PEN_HPP

#include "flags.hpp"
#include "input.hpp"
#include <bitset>

namespace rmioc
{

/** Recognized pen tools. */
RMIOC_FLAGS_DEFINE(
    pen_tools,
    pen, rubber
);

/**
 * Access to the state of the device’s pen digitizer.
 */
class pen : public input
{
public:
    /**
     * Open the pen digitizer device.
     *
     * @param path Path to the device.
     */
    pen(const char* device_path);

    // Disallow copying pen device handles
    pen(const pen& other) = delete;
    pen& operator=(const pen& other) = delete;

    // Transfer handle ownership
    pen(pen&& other) noexcept;
    pen& operator=(pen&& other) noexcept;

    /**
     * Check for new events.
     *
     * @return True if the pen state changed since last call.
     */
    bool process_events();

    /**
     * Information about the pen state.
     *
     * Coordinates are in the pen digitizer’s frame, which has its origin on
     * the bottom left of the screen with the X axis increasing in the upper
     * direction and the Y axis increasing in the right direction.
     *
     *     (20967, 0) → (20967, 15725)
     *     |                         |
     *     ↑                         ↑
     *     |                         |
     *     (0, 0) ————→———— (0, 15725)
     */
    struct pen_state
    {
        /** Set of currently active tools. */
        pen_tools tool_set;

        /** Horizontal position of the pen. */
        int x;

        static constexpr int x_min = 0;
        static constexpr int x_max = 20967;

        /** Vertical position of the pen. */
        int y;

        static constexpr int y_min = 0;
        static constexpr int y_max = 15725;

        /** Amount of pressure applied with the pen. */
        int pressure;

        static constexpr int pressure_min = 0;
        static constexpr int pressure_max = 4095;

        /** Distance from the pen to the screen. */
        int distance;

        static constexpr int distance_min = 0;
        static constexpr int distance_max = 255;

        /**
         * Tilt of the pen along the X axis.
         *
         * A positive value indicates clockwise rotation around the X axis,
         * a negative value indicates counter-clockwise rotation.
         */
        int tilt_x;

        static constexpr int tilt_x_min = -9000;
        static constexpr int tilt_x_max = 9000;

        /**
         * Tilt of the pen along the Y axis.
         *
         * A positive value indicates clockwise rotation around the Y axis,
         * a negative value indicates counter-clockwise rotation.
         */
        int tilt_y;

        static constexpr int tilt_y_min = -9000;
        static constexpr int tilt_y_max = 9000;
    };

    const pen_state& get_state() const;

private:
    pen_state state;
}; // class pen

} // namespace rmioc

#endif // RMIOC_PEN_HPP
