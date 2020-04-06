#include "screen.hpp"
#include "log.hpp"
#include <cerrno>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <rfb/rfbclient.h>
// IWYU pragma: no_include <rfb/rfbproto.h>
#include <system_error>

namespace chrono = std::chrono;

/**
 * Tag used for storing in/retrieving from the client object
 * the structure in which updates are accumulated.
 */
constexpr auto update_info_tag = 1;

/**
 * Time to wait after the last update from VNC before updating the screen
 * (in microseconds).
 *
 * VNC servers tend to send a lot of small updates in a short period of time.
 * This delay allows grouping those small updates into a larger screen update.
 */
constexpr chrono::milliseconds update_delay{100};

/**
 * Accumulate updates received from the VNC server.
 */
struct vnc_update_info
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
    chrono::steady_clock::time_point last_update_time;
};

rfbBool create_framebuf(rfbClient*)
{
    // No-op: Write directly to the memory-mapped framebuffer
    return TRUE;
}

/**
 * Register an update from the server.
 *
 * @param client Handle to the VNC client.
 * @param x Left bound of the updated rectangle (in pixels).
 * @param y Top bound of the updated rectangle (in pixels).
 * @param w Width of the updated rectangle (in pixels).
 * @param h Height of the updated rectangle (in pixels).
 */
void update_framebuf(rfbClient* client, int x, int y, int w, int h)
{
    // Register the region as pending update, potentially extending
    // an existing one
    vnc_update_info* update = reinterpret_cast<vnc_update_info*>(
        rfbClientGetClientData(client, reinterpret_cast<void*>(
            update_info_tag)));

    log::print("VNC Update") << w << 'x' << h << '+' << x << '+' << y << '\n';

    if (update->has_update)
    {
        int left_x = x < update->x ? x : update->x;
        int top_y = y < update->y ? y : update->y;

        int right_x_1 = x + w;
        int right_x_2 = update->x + update->w;
        int right_x = right_x_1 > right_x_2 ? right_x_1 : right_x_2;

        int bottom_y_1 = y + h;
        int bottom_y_2 = update->y + update->h;
        int bottom_y = bottom_y_1 > bottom_y_2 ? bottom_y_1 : bottom_y_2;

        update->x = left_x;
        update->y = top_y;
        update->w = right_x - left_x;
        update->h = bottom_y - top_y;
    }
    else
    {
        update->x = x;
        update->y = y;
        update->w = w;
        update->h = h;
        update->has_update = 1;
    }

    update->last_update_time = chrono::steady_clock::now();
}

int main(int argc, char** argv)
{
    // Setup the VNC connection
    rfbClient* client = rfbGetClient(0, 0, 0);

    // Accumulator for updates received from the server
    vnc_update_info update_info;
    rfbClientSetClientData(client, reinterpret_cast<void*>(update_info_tag),
        &update_info);

    // Open and map the device framebuffer to memory
    rm::screen screen;
    client->frameBuffer = screen.get_data();

    // Use the same format as expected by the device framebuffer
    client->format.bitsPerPixel = screen.get_bits_per_pixel();
    client->format.redShift = screen.get_red_offset();
    client->format.redMax = screen.get_red_max();
    client->format.greenShift = screen.get_green_offset();
    client->format.greenMax = screen.get_green_max();
    client->format.blueShift = screen.get_blue_offset();
    client->format.blueMax = screen.get_blue_max();

    client->MallocFrameBuffer = create_framebuf;
    client->GotFrameBufferUpdate = update_framebuf;

    // Connect to the server
    if (!rfbInitClient(client, &argc, argv))
    {
        return EXIT_FAILURE;
    }

    // Make sure the server gives us a compatible format
    if (client->width < 0 || client->height < 0
        || static_cast<unsigned int>(client->width)
            != screen.get_xres_memory()
        || static_cast<unsigned int>(client->height)
            > screen.get_yres_memory())
    {
        std::cerr << "\nError: Server uses an unsupported resolution ("
            << client->width << 'x' << client->height << "). This client can "
            "only cope with a screen width of exactly "
            << screen.get_xres_memory() << " pixels and a screen height no "
            "larger than " << screen.get_yres_memory() << " pixels.\n";
        return EXIT_FAILURE;
    }

    // RFB protocol message loop
    while (TRUE)
    {
        int result = WaitForMessage(
            client,
            /* timeout = */
            chrono::duration_cast<chrono::microseconds>(update_delay).count()
        );

        if (result < 0)
        {
            throw std::system_error(
                errno,
                std::generic_category(),
                "(main) Wait for message"
            );
        }

        if (result > 0)
        {
            if (!HandleRFBServerMessage(client))
            {
                break;
            }
        }

        if (update_info.has_update && update_info.last_update_time
                + update_delay < chrono::steady_clock::now())
        {
            update_info.has_update = 0;
            screen.update(
                update_info.x, update_info.y,
                update_info.w, update_info.h
            );
        }
    }

    rfbClientCleanup(client);
    return EXIT_SUCCESS;
}
