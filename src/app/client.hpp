#ifndef APP_CLIENT_HPP
#define APP_CLIENT_HPP

#include "event_loop.hpp"
#include "buttons.hpp"
// IWYU pragma: no_forward_declare app::buttons
#include "pen.hpp"
// IWYU pragma: no_forward_declare app::pen
#include "screen.hpp"
#include "touch.hpp"
// IWYU pragma: no_forward_declare app::touch
#include <iosfwd>
#include <optional>
#include <poll.h> // IWYU pragma: keep
#include <rfb/rfbclient.h>
// IWYU pragma: no_include "rfb/rfbproto.h"
#include <vector>

namespace rmioc
{
    class screen;
    class buttons;
    class pen;
    class touch;
}

namespace app
{

/**
 * VNC client for the reMarkable tablet.
 */
class client
{
public:
    /**
     * Create a VNC client.
     *
     * @param ip IP address of the VNC server to connect to.
     * @param port Port of the VNC server to connect to.
     * @param screen_device Screen to update with data from the VNC server.
     * @param buttons_device Buttons device to read input events from, or
     * nullptr to ignore input events from the buttons.
     * @param pen_device Pen digitizer to read input events from, or
     * nullptr to ignore input events from the pen.
     * @param touch_device Touchscreen device to read input events from, or
     * nullptr to ignore input events from the touchscreen.
     */
    client(
        const char* ip, int port,
        rmioc::screen& screen_device,
        rmioc::buttons* buttons_device,
        rmioc::pen* pen_device,
        rmioc::touch* touch_device
    );

    ~client();

    /** Start the client event loop.  */
    void event_loop();

private:
    /** List of file descriptors to watch in the event loop. */
    std::vector<pollfd> polled_fds;

    /** Index of the buttons file descriptor in the poll structure. */
    std::size_t poll_buttons = -1;

    /** Index of the pen file descriptor in the poll structure. */
    std::size_t poll_pen = -1;

    /** Index of the touch file descriptor in the poll structure. */
    std::size_t poll_touch = -1;

    /** Index of the VNC socket file descriptor in the poll structure. */
    std::size_t poll_vnc = -1;

    /** VNC connection. */
    rfbClient* vnc_client;

    /** Event handler for the screen device. */
    screen screen_handler;

    /** Event handler for the buttons device. */
    std::optional<buttons> buttons_handler;

    /** Event handler for the pen device. */
    std::optional<pen> pen_handler;

    /** Event handler for the touch device. */
    std::optional<touch> touch_handler;

    /**
     * Send a pointer event to the VNC server.
     *
     * @param x Pointer X location on the screen.
     * @param y Pointer Y location on the screen.
     * @param button Button to press.
     */
    void send_button_press(int x, int y, MouseButton button);
}; // class client

} // namespace app

#endif // APP_CLIENT_HPP
