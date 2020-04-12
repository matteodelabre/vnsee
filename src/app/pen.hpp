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

    /** Process events from the pen digitizer. */
    event_loop_status process_events();

    /** Whether other forms of input should be inhibited. */
    bool is_inhibiting() const;

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
