#ifndef APP_CLIENT_HPP
#define APP_CLIENT_HPP

#include "event_loop.hpp"
#include "touch.hpp"
#include <chrono>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include "rfb/rfbproto.h"

namespace rmioc
{
    class screen;
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
     * @param touch_device Touchscreen device to read input events from.
     */
    client(
        const char* ip, int port,
        rmioc::screen& screen_device,
        rmioc::touch& touch_device
    );

    ~client();

    /**
     * Start the client event loop.
     */
    void event_loop();

private:
    /** Tag used for accessing the update accumulator from the C callbacks. */
    static constexpr auto update_info_tag = 1;

    /** Subroutine for handling VNC events from the server. */
    event_loop_status event_loop_vnc();

    /** VNC connection. */
    rfbClient* vnc_client;

    /** Accumulator for updates received from the VNC server. */
    struct update_info_struct
    {
        /** Left bound of the overall updated rectangle (in pixels). */
        int x;

        /** Top bound of the overall updated rectangle (in pixels). */
        int y;

        /** Width of the overall updated rectangle (in pixels). */
        int w;

        /** Height of the overall updated rectangle (in pixels). */
        int h;

        /** Whether at least one update has been registered. */
        bool has_update;

        /** Last time an update was registered. */
        std::chrono::steady_clock::time_point last_update_time;
    } update_info;

    /** Subroutine for updating the e-ink screen. */
    event_loop_status event_loop_screen();

    /** reMarkable screen. */
    rmioc::screen& screen_device;

    /**
     * Called by the VNC client library to initialize our local framebuffer.
     *
     * @param client Handle to the VNC client.
     */
    static rfbBool create_framebuf(rfbClient* client);

    /**
     * Called by the VNC client library to register an update from the server.
     *
     * @param client Handle to the VNC client.
     * @param x Left bound of the updated rectangle (in pixels).
     * @param y Top bound of the updated rectangle (in pixels).
     * @param w Width of the updated rectangle (in pixels).
     * @param h Height of the updated rectangle (in pixels).
     */
    static void update_framebuf(
        rfbClient* client,
        int x, int y, int w, int h
    );

    rmioc::touch& touch_device;
    touch touch_handler;

    /**
     * Send press and release events for the given button to VNC.
     *
     * @param x Pointer X location on the screen.
     * @param y Pointer Y location on the screen.
     * @param button Button to press.
     */
    void send_button_press(int x, int y, MouseButton button) const;
}; // class client

} // namespace app

#endif // APP_CLIENT_HPP
