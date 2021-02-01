#ifndef RMIOC_DEVICE_HPP
#define RMIOC_DEVICE_HPP

#include "buttons.hpp"
#include "flags.hpp"
#include "pen.hpp"
#include "screen.hpp"
#include "touch.hpp"
#include <memory>

namespace rmioc
{

/** Devices that can be opened. */
RMIOC_FLAGS_DEFINE(
    device_request,
    buttons, touch, pen, screen
);

/**
 * Access input and output devices.
 *
 * This class enables access to the buttons, touch and pen inputs and
 * screen output while abstracting differences between rM1 and rM2.
 */
class device
{
public:
    /**
     * Automatically detect the current device type and open the requested
     * input and output devices.
     *
     * @param request Devices to open.
     */
    static device detect(device_request request);

    /**
     * Supported devices.
     */
    enum class types
    {
        reMarkable1 = 1,
        reMarkable2 = 2,
    };

    /** Get what revision of the reMarkable we’re running on. */
    types get_type() const;

    /** Access the buttons device, if possible. */
    buttons* get_buttons();

    /** Access the touch device, if possible. */
    touch* get_touch();

    /** Access the pen digitizer device, if possible. */
    pen* get_pen();

    /** Access the screen device, if possible. */
    screen* get_screen();

private:
    device(
        types type,
        std::unique_ptr<buttons>&& buttons_device,
        std::unique_ptr<touch>&& touch_device,
        std::unique_ptr<pen>&& pen_device,
        std::unique_ptr<screen>&& screen_device
    );

    types type;
    std::unique_ptr<buttons> buttons_device;
    std::unique_ptr<touch> touch_device;
    std::unique_ptr<pen> pen_device;
    std::unique_ptr<screen> screen_device;
};

} // namespace

#endif // RMIOC_DEVICE_HPP
