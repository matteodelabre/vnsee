#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <chrono>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include <rfb/rfbproto.h>

namespace rm
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
        rm::screen& rm_screen,
        rm::input& rm_input
    );

    ~client();

    /**
     * Start the client event loop.
     */
    void start();

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

        /** Last time an update was registered (in microseconds). */
        std::chrono::steady_clock::time_point last_update_time;
    };

private:
    /** VNC connection. */
    rfbClient* vnc_client;

    /** reMarkable screen. */
    rm::screen& rm_screen;

    /** reMarkable input device. */
    rm::input& rm_input;

    update_info_struct update_info;
}; // class client

#endif // CLIENT_HPP
