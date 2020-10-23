#ifndef APP_PEN_HPP
#define APP_PEN_HPP

#include "event_loop.hpp"

namespace rmioc
{

class pen;

}

namespace app
{

class screen;
class pen
{
public:
    pen(
        rmioc::pen& device,
        app::screen& screen_device,
        MouseCallback send_button_press
    );

    /** Process events from the pen digitizer. */
    event_loop_status process_events();

    /** Whether other forms of input should be inhibited. */
    bool is_inhibiting() const;

private:
    /** reMarkable pen digitizer device. */
    rmioc::pen& device;

    /** reMarkable screen. */
    app::screen& screen;

    /** Callback for sending mouse events. */
    MouseCallback send_button_press;

    /** Current state of the pen */
    MouseButton state;
};

} // namespace app

#endif // APP_PEN_HPP
