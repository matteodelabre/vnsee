#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <chrono>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include "rfb/rfbproto.h"

namespace rmioc
{
    class screen;
    class touch;
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
     * @param rm_touch Touchscreen device to read input events from.
     */
    client(
        const char* ip, int port,
        rmioc::screen& rm_screen,
        rmioc::touch& rm_touch
    );

    ~client();

    /**
     * Start the client event loop.
     */
    void event_loop();

private:
    /** Tag used for accessing the update accumulator from the C callbacks. */
    static constexpr auto update_info_tag = 1;

    /**
     * Informations returned by subroutines of the event loop.
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
    rmioc::screen& rm_screen;

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

    /** Subroutine for processing user input. */
    event_loop_status event_loop_input();

    /** reMarkable touchscreen device. */
    rmioc::touch& rm_touch;

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
    } touch_state = TouchState::Inactive;

    void on_touch_update(int x, int y);
    void on_touch_end();

    /** Current X position of the touch interaction, if not inactive. */
    int touch_x = 0;

    /** Current Y position of the touch interaction, if not inactive. */
    int touch_y = 0;

    /** Initial X position of the touch interaction, if not inactive. */
    int touch_x_initial = 0;

    /** Initial Y position of the touch interaction, if not inactive. */
    int touch_y_initial = 0;

    /**
     * Total number of horizontal scroll events that were sent in this
     * interaction, positive for scrolling right and negative for
     * scrolling left.
     */
    int touch_x_scroll_events = 0;

    /**
     * Total number of vertical scroll events that were sent in this
     * interaction, positive for scrolling down and negative for
     * scrolling up.
     */
    int touch_y_scroll_events = 0;

    /** List of mouse button flags used by the VNC protocol. */
    enum class MouseButton : std::uint8_t
    {
        Left = 1,
        // Right = 1U << 1U,
        // Middle = 1U << 2U,
        ScrollDown = 1U << 3U,
        ScrollUp = 1U << 4U,
        ScrollLeft = 1U << 5U,
        ScrollRight = 1U << 6U,
    };

    /**
     * Send press and release events for the given button to VNC.
     *
     * @param x Pointer X location on the screen.
     * @param y Pointer Y location on the screen.
     * @param button Button to press.
     */
    void send_button_press(int x, int y, MouseButton button) const;
}; // class client

#endif // CLIENT_HPP
