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
 * the flag indicating whether a screen refresh is needed.
 */
#define NEEDS_REFRESH_TAG ((void*) 0)

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
    print_log("Update");
    fprintf(stderr, "%dx%d+%d+%d\n", w, h, x, y);

    short* needs_refresh = rfbClientGetClientData(client, NEEDS_REFRESH_TAG);
    *needs_refresh = 1;
}

int main(int argc, char** argv)
{
    rm_screen screen = rm_screen_init();

    // Flag activated when the screen needs a refresh
    short needs_refresh = 0;

    // Setup the VNC connection
    rfbClient* client = rfbGetClient(0, 0, 0);

    // Use the same format as expected by the device framebuffer
    client->format.bitsPerPixel = screen.framebuf_varinfo.bits_per_pixel;
    client->format.redShift = screen.framebuf_varinfo.red.offset;
    client->format.redMax = (1 << screen.framebuf_varinfo.red.length) - 1;
    client->format.greenShift = screen.framebuf_varinfo.green.offset;
    client->format.greenMax = (1 << screen.framebuf_varinfo.green.length) - 1;
    client->format.blueShift = screen.framebuf_varinfo.blue.offset;
    client->format.blueMax = (1 << screen.framebuf_varinfo.blue.length) - 1;

    client->frameBuffer = screen.framebuf_ptr;

    client->MallocFrameBuffer = create_framebuf;
    client->GotFrameBufferUpdate = update_framebuf;

    rfbClientSetClientData(client, NEEDS_REFRESH_TAG, &needs_refresh);

    // Connect to the VNC server
    if (!rfbInitClient(client, &argc, argv))
    {
        return EXIT_FAILURE;
    }

    // Make sure the VNC server gives us a compatible format
    assert(client->width == screen.framebuf_varinfo.xres_virtual);
    assert(client->height <= screen.framebuf_varinfo.yres_virtual);

    // RFB protocol message loop
    while (TRUE)
    {
        int result = WaitForMessage(client, /* timeout μs = */ 100000);

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

        if (needs_refresh)
        {
            needs_refresh = 0;
            rm_screen_update(&screen);
        }
    }

    rfbClientCleanup(client);
    rm_screen_free(&screen);
    return EXIT_SUCCESS;
}
