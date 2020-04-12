#ifndef APP_TOUCH_HPP
#define APP_TOUCH_HPP

#include "event_loop.hpp"

namespace rmioc
{
    class screen;
    class touch;
}

namespace app
{

class touch
{
public:
    touch(
        rmioc::touch& device,
        const rmioc::screen& screen_device,
        MouseCallback send_button_press
    );

    /** Subroutine for processing touch input. */
    event_loop_status event_loop();

private:
    /** reMarkable touchscreen device. */
    rmioc::touch& device;

    /** reMarkable screen device. */
    const rmioc::screen& screen_device;

    /** Callback for sending mouse events. */
    MouseCallback send_button_press;

    /** Current state of the touch interaction. */
    enum class TouchState
    {
        /** No touch point is active. */
        Inactive,

        /** Touch points are active but did not move enough to scroll. */
        Tap,

        /** Touch points are active and scrolling horizontally. */
        ScrollX,

        /** Touch points are active and scrolling vertically. */
        ScrollY,
    } state = TouchState::Inactive;

    /**
     * Called when the touch point position changes.
     *
     * @param x New X position of the touch point.
     * @param y New Y position of the touch point.
     */
    void on_update(int x, int y);

    /** Called when all touch points are removed from the screen. */
    void on_end();

    /** Current X position of the touch interaction, if not inactive. */
    int x = 0;

    /** Current Y position of the touch interaction, if not inactive. */
    int y = 0;

    /** Initial X position of the touch interaction, if not inactive. */
    int x_initial = 0;

    /** Initial Y position of the touch interaction, if not inactive. */
    int y_initial = 0;

    /**
     * Total number of horizontal scroll events that were sent in this
     * interaction, positive for scrolling right and negative for
     * scrolling left.
     */
    int x_scroll_events = 0;

    /**
     * Total number of vertical scroll events that were sent in this
     * interaction, positive for scrolling down and negative for
     * scrolling up.
     */
    int y_scroll_events = 0;
};

} // namespace app

#endif // APP_TOUCH_HPP
