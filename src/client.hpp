#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <chrono>
#include <utility>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include <rfb/rfbproto.h>

namespace rmioc
{
    class screen;
    class input;
}

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
     * @param rm_screen Screen to update with data from the VNC server.
     * @param rm_input Input device to read mouse events from.
     */
    client(
        const char* ip, int port,
        rmioc::screen& rm_screen,
        rmioc::input& rm_input
    );

    ~client();

    /**
     * Start the client event loop.
     */
    void event_loop();

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
        short has_update;

        /** Last time an update was registered. */
        std::chrono::steady_clock::time_point last_update_time;
    };

private:
    /**
     * Informations returned by subroutines in the event loop.
     *
     */
    struct event_loop_status
    {
        /** True if the client must quit the event loop. */
        bool quit;

        /**
         * Timeout to use for the next poll call (in milliseconds).
         *
         * The minimum timeout among all called subroutines will be used.
         * Can be -1 if no more work is needed (wait indefinitely).
         */
        int timeout;
    };

    event_loop_status event_loop_vnc();
    event_loop_status event_loop_screen();
    event_loop_status event_loop_input();

    /** VNC connection. */
    rfbClient* vnc_client;

    /** reMarkable screen. */
    rmioc::screen& rm_screen;

    /** reMarkable input device. */
    rmioc::input& rm_input;

    update_info_struct update_info;
}; // class client

#endif // CLIENT_HPP
