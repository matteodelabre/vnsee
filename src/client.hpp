#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <chrono>
#include <map>
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

    class touchpoint_state
    {
    public:
        touchpoint_state(client& parent, int x, int y);

        /** Register a move of the touch point on the sensor. */
        void update(int x, int y);

        /** Register the removal of a touch point. */
        void terminate();

        /** Whether this touchpoint is used for scrolling in any direction. */
        bool scrolling() const;

    private:
        /** Reference to the parent client instance. */
        client& parent;

        int x;
        int y;

        /** Initial X position of the touchpoint in the sensor frame. */
        int x_initial;

        /** Initial Y position of the touchpoint in the sensor frame. */
        int y_initial;

        /** True if this touchpoint is used for scrolling horizontally. */
        bool x_scrolling = false;

        /** True if this touchpoint is used for scrolling vertically. */
        bool y_scrolling = false;

        /**
         * Effective number of horizontal discrete scroll events that were
         * already sent to the server to reflect the dragging of this
         * touchpoint.
         *
         * Negative values are for leftward events, positive for rightward.
         */
        int x_sent_events = 0;

        /**
         * Effective number of vertical discrete scroll events that were
         * already sent to the server to reflect the dragging of this
         * touchpoint.
         *
         * Negative values are for upward scroll, positive for downward.
         */
        int y_sent_events = 0;

        /** Convert an X position on the sensor to its on-screen position. */
        int x_sensor_to_screen(int x_value) const;

        /** Convert an Y position on the sensor to its on-screen position. */
        int y_sensor_to_screen(int y_value) const;

        /**
         * Press and release the given button on the server.
         *
         * @param x Pointer X location on the screen.
         * @param y Pointer Y location on the screen.
         * @param button Button to press.
         */
        void send_button_press(int x, int y, int button) const;
    };

    std::map<int, touchpoint_state> touchpoints;

    update_info_struct update_info;
}; // class client

#endif // CLIENT_HPP
