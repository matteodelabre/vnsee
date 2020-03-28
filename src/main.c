#include "screen.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <rfb/rfbclient.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>

/**
 * Tag used for storing in/retrieving from the client object
 * the device framebuffer’s file descriptor.
 */
#define FRAMEBUF_FD_TAG ((void*) 0)

/**
 * Tag used for storing in/retrieving from the client object
 * the flag indicating whether a screen refresh is needed.
 */
#define NEEDS_REFRESH_TAG ((void*) 1)

/**
 * Maximum time in microseconds to wait between two screen refreshes.
 */
#define REFRESH_RATE 500000

/**
 * Map the device framebuffer to memory and make it available for
 * the VNC client to write in.
 *
 * @param client Handle to the VNC client.
 * @return True if the framebuffer is successfully mapped to memory.
 */
rfbBool create_framebuf(rfbClient* client)
{
    // Make sure the server streams the right color format
    assert(client->format.bitsPerPixel == 8 * RM_SCREEN_DEPTH);
    assert(client->width == RM_SCREEN_COLS + RM_SCREEN_COL_PAD);
    assert(client->height == RM_SCREEN_ROWS);

    if (client->frameBuffer)
    {
        // Already allocated
        return TRUE;
    }

    int framebuf_fd = *(int*) rfbClientGetClientData(
        client, FRAMEBUF_FD_TAG);

    uint8_t* data = mmap(
        /* addr = */ NULL,
        /* len = */ (RM_SCREEN_COLS + RM_SCREEN_COL_PAD)
                    * RM_SCREEN_ROWS * RM_SCREEN_DEPTH,
        /* prot = */ PROT_READ | PROT_WRITE,
        /* flags = */ MAP_SHARED,
        /* fd = */ framebuf_fd,
        /* off = */ 0
    );

    if (data == MAP_FAILED)
    {
        perror("create_framebuf:mmap");
        return FALSE;
    }

    client->frameBuffer = data;
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

    // Mark that screen as needing a refresh
    short* needs_refresh = rfbClientGetClientData(client, NEEDS_REFRESH_TAG);
    *needs_refresh = 1;
}

int main(int argc, char** argv)
{
    // Open the system framebuffer
    int framebuf_fd = open("/dev/fb0", O_RDWR);

    if (framebuf_fd == -1)
    {
        perror("open:/dev/fb0");
        return EXIT_FAILURE;
    }

    // Flag activated when the screen needs a refresh
    short needs_refresh = 0;

    // Last time the screen was refreshed
    uint64_t last_refresh = 0;

    // Setup the VNC connection
    rfbClient* client = rfbGetClient(
        /* bitsPerSample = */ 5,
        /* samplesPerPixel = */ 3,
        /* bytesPerPixel = */ 2
    );

    // Setup for RGB565 color mode
    client->format.redShift=11;
    client->format.redMax=31;
    client->format.greenShift=5;
    client->format.greenMax=63;
    client->format.blueShift=0;
    client->format.blueMax=31;

    client->MallocFrameBuffer = create_framebuf;
    client->GotFrameBufferUpdate = update_framebuf;

    rfbClientSetClientData(client, FRAMEBUF_FD_TAG, &framebuf_fd);
    rfbClientSetClientData(client, NEEDS_REFRESH_TAG, &needs_refresh);

    // Connect to the VNC server
    if (!rfbInitClient(client, &argc, argv))
    {
        return EXIT_FAILURE;
    }

    // RFB protocol message loop
    while (TRUE)
    {
        int result = WaitForMessage(client, /* timeout μs = */ 100000);

        if (result < 0)
        {
            perror("WaitForMessage");
            break;
        }

        if (result > 0)
        {
            if (!HandleRFBServerMessage(client))
            {
                break;
            }
        }

        uint64_t cur_time = get_time_us();

        if (needs_refresh && last_refresh + REFRESH_RATE < cur_time)
        {
            needs_refresh = 0;
            last_refresh = cur_time;

            trigger_refresh(framebuf_fd);
        }
    }

    rfbClientCleanup(client);
    close(framebuf_fd);
    return EXIT_SUCCESS;
}
