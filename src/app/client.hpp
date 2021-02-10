#ifndef APP_CLIENT_HPP
#define APP_CLIENT_HPP

#include "event_loop.hpp"
#include "buttons.hpp"
#include "pen.hpp"
#include "screen.hpp"
#include "touch.hpp"
#include <iosfwd>
#include <optional>
#include <poll.h> // IWYU pragma: keep
#include <rfb/rfbclient.h>
#include <vector>

namespace rmioc
{
    class device;
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
     * @param device Handle to opened devices.
     */
    client(const char* ip, int port, rmioc::device& device);

    /** Disconnect the VNC client. */
    ~client();

    /**
     * Start the client event loop.
     *
     * @return True if the loop was exited because of a user action, false if
     * it was because the server closed the connection.
     */
    bool event_loop();

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
    std::optional<screen> screen_handler;

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
