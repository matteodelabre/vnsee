#ifndef APP_PEN_HPP
#define APP_PEN_HPP

#include "event_loop.hpp"

namespace rmioc
{
    class screen;
    class pen;
}

namespace app
{

class pen
{
public:
    pen(
        rmioc::pen& device,
        const rmioc::screen& screen_device,
        MouseCallback send_button_press
    );

    /** Subroutine for processing pen input. */
    event_loop_status event_loop();

private:
    /** reMarkable pen digitizer device. */
    rmioc::pen& device;

    /** reMarkable screen device. */
    const rmioc::screen& screen_device;

    /** Callback for sending mouse events. */
    MouseCallback send_button_press;
};

} // namespace app

#endif // APP_PEN_HPP
