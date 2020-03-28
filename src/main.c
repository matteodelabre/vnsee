#include "screen.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <rfb/rfbclient.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Tag used for storing in/retrieving from the client object
 * the structure in which updates are accumulated.
 */
#define UPDATE_INFO_TAG ((void*) 0)

/**
 * Time to wait after the last update from VNC before updating the screen
 * (in microseconds).
 *
 * VNC servers tend to send a lot of small updates in a short period of time.
 * This delay allows grouping those small updates into a larger screen update.
 */
#define UPDATE_DELAY 100000 /* 100 ms */

/**
 * Accumulate updates received from the VNC server.
 */
typedef struct vnc_update_info
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
    mono_time last_update_time;
} vnc_update_info;

rfbBool create_framebuf(rfbClient* client)
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
    vnc_update_info* update = rfbClientGetClientData(client, UPDATE_INFO_TAG);

#ifdef TRACE
    print_log("VNC Update");
    fprintf(stderr, "%dx%d+%d+%d\n", w, h, x, y);
#endif // TRACE

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

    update->last_update_time = get_mono_time();
}

int main(int argc, char** argv)
{
    // Setup the VNC connection
    rfbClient* client = rfbGetClient(0, 0, 0);

    // Accumulator for updates received from the server
    vnc_update_info update_info;
    rfbClientSetClientData(client, UPDATE_INFO_TAG, &update_info);

    // Open and map the device framebuffer to memory
    rm_screen screen = rm_screen_init();
    client->frameBuffer = screen.framebuf_ptr;

    // Use the same format as expected by the device framebuffer
    client->format.bitsPerPixel = screen.framebuf_varinfo.bits_per_pixel;
    client->format.redShift = screen.framebuf_varinfo.red.offset;
    client->format.redMax = (1 << screen.framebuf_varinfo.red.length) - 1;
    client->format.greenShift = screen.framebuf_varinfo.green.offset;
    client->format.greenMax = (1 << screen.framebuf_varinfo.green.length) - 1;
    client->format.blueShift = screen.framebuf_varinfo.blue.offset;
    client->format.blueMax = (1 << screen.framebuf_varinfo.blue.length) - 1;

    client->MallocFrameBuffer = create_framebuf;
    client->GotFrameBufferUpdate = update_framebuf;

    // Connect to the server
    if (!rfbInitClient(client, &argc, argv))
    {
        return EXIT_FAILURE;
    }

    // Make sure the server gives us a compatible format
    assert(client->width == screen.framebuf_varinfo.xres_virtual);
    assert(client->height <= screen.framebuf_varinfo.yres_virtual);

    // RFB protocol message loop
    while (TRUE)
    {
        int result = WaitForMessage(client, /* timeout = */ UPDATE_DELAY);

        if (result < 0)
        {
            perror("main - WaitForMessage");
            break;
        }

        if (result > 0)
        {
            if (!HandleRFBServerMessage(client))
            {
                break;
            }
        }

        if (update_info.has_update
            && update_info.last_update_time + UPDATE_DELAY < get_mono_time())
        {
            update_info.has_update = 0;
            rm_screen_update(
                &screen,
                update_info.x,
                update_info.y,
                update_info.w,
                update_info.h
            );
        }
    }

    rfbClientCleanup(client);
    rm_screen_free(&screen);
    return EXIT_SUCCESS;
}
