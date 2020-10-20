#ifndef APP_SCREEN_HPP
#define APP_SCREEN_HPP

#include "event_loop.hpp"
#include <chrono>
#include <iosfwd>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include "rfb/rfbproto.h"

namespace rmioc
{
    class screen;
}

namespace app
{

class screen
{
public:
    screen(
        rmioc::screen& device,
        rfbClient* vnc_client
    );

    event_loop_status event_loop();

    /** repaint the reMarkable screen. */
    void repaint();
private:
    /** reMarkable screen device. */
    rmioc::screen& device;

    /** VNC connection. */
    rfbClient* vnc_client;

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

    /** Tag used for accessing the instance from C callbacks. */
    static constexpr std::size_t instance_tag = 6803;
}; // class screen

} // namespace app

#endif // APP_SCREEN_HPP
