#ifndef RMIOC_PEN_HPP
#define RMIOC_PEN_HPP

#include "flags.hpp"
#include "input.hpp"
#include <boost/preprocessor/arithmetic/limits/dec_256.hpp>
#include <boost/preprocessor/arithmetic/limits/inc_256.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/detail/limits/auto_rec_256.hpp>
#include <boost/preprocessor/logical/limits/bool_256.hpp>
#include <boost/preprocessor/repetition/detail/limits/for_256.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/seq/limits/elem_256.hpp>
#include <boost/preprocessor/seq/limits/size_256.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/limits/to_seq_64.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/variadic/limits/elem_64.hpp>
#include <utility>

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
     * @param flip_x True to flip coordinates horizontally.
     * @param flip_y True to flip coordinates vertically.
     */
    pen(const char* device_path, bool flip_x = false, bool flip_y = false);

    /**
     * Check for new events.
     *
     * @return True if the pen state changed since last call.
     */
    bool process_events();

    /**
     * Information about the pen state.
     */
    struct pen_state
    {
        /** Set of currently active tools. */
        pen_tools tool_set;

        /**
         * Horizontal position of the pen.
         *
         * Integer between 0 and `get_xres()`.
         */
        int x;


        /**
         * Vertical position of the pen.
         *
         * Integer between 0 and `get_yres()`.
         */
        int y;

        /**
         * Amount of pressure applied with the pen.
         *
         * Integer between 0 and `get_pressure_res()`.
         */
        int pressure;

        /**
         * Distance from the pen to the screen.
         *
         * Integer between 0 and `get_distance_res()`.
         */
        int distance;

        /**
         * Tilt of the pen along the X axis.
         *
         * Integer within the range indicated by `get_tilt_x_limits()`.
         * A positive value indicates clockwise rotation around the X axis,
         * a negative value indicates counter-clockwise rotation.
         */
        int tilt_x;

        /**
         * Tilt of the pen along the Y axis.
         *
         * Integer within the range indicated by `get_tilt_y_limits()`.
         * A positive value indicates clockwise rotation around the Y axis,
         * a negative value indicates counter-clockwise rotation.
         */
        int tilt_y;
    };

    /**
     * Get the current state of the stylus.
     */
    const pen_state& get_state() const;

    /** Get the maximum possible value on the X axis. */
    int get_xres() const;

    /** Get the maximum possible value on the Y axis. */
    int get_yres() const;

    /** Get the maximum pressure value. */
    int get_pressure_res() const;

    /** Get the maximum distance value. */
    int get_distance_res() const;

    /**
     * Get the minimum and maximum value of the rotation around the X axis.
     */
    const std::pair<int, int>& get_tilt_x_limits() const;

    /**
     * Get the minimum and maximum value of the rotation around the Y axis.
     */
    const std::pair<int, int>& get_tilt_y_limits() const;

private:
    /** Coordinate flipping state. */
    bool flip_x;
    bool flip_y;

    /** Current stylus state. */
    pen_state state;

    /** Minimum and maximum value of the X axis. */
    std::pair<int, int> x_limits;

    /** Minimum and maximum value of the Y axis. */
    std::pair<int, int> y_limits;

    /** Minimum and maximum value of the pressure axis. */
    std::pair<int, int> pressure_limits;

    /** Minimum and maximum value of the distance axis. */
    std::pair<int, int> distance_limits;

    /** Minimum and maximum value of the X tilt axis. */
    std::pair<int, int> tilt_x_limits;

    /** Minimum and maximum value of the Y tilt axis. */
    std::pair<int, int> tilt_y_limits;
}; // class pen

} // namespace rmioc

#endif // RMIOC_PEN_HPP
