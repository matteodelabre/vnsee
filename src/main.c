#include "defs.h"
#include "refresh.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <rfb/rfbclient.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

/**
 * Tag used for storing in/retrieving from the client object
 * the device framebuffer’s file descriptor.
 */
#define FRAMEBUF_FP_TAG ((void*) 0)

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
 * Get the current monotonic time in microseconds.
 */
uint64_t get_time_us()
{
    struct timespec cur_time_st;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time_st);
    return cur_time_st.tv_sec * 1000000 + cur_time_st.tv_nsec / 1000;
}

/**
 * Print the prefix for logging an event.
 *
 * @param type Type of event.
 */
void print_log(const char* type)
{
    struct timespec cur_time_st;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time_st);

    fprintf(
        stderr,
        "%ld.%06ld [%s] ",
        cur_time_st.tv_sec,
        cur_time_st.tv_nsec / 1000,
        type
    );
}

/**
 * Initialize the client framebuffer, a buffer in which screen data from
 * the server is stored before being copied to the device framebuffer.
 *
 * @param client Handle to the VNC client.
 * @return True if the new buffer was successfully allocated.
 */
rfbBool create_framebuf(rfbClient* client)
{
    // Make sure the server streams the right color format
    assert(client->format.bitsPerPixel == 8 * RM_SCREEN_DEPTH);

    uint8_t* data = malloc(client->width * client->height * RM_SCREEN_DEPTH);

    if (!data)
    {
        perror("create_framebuf");
        return FALSE;
    }

    free(client->frameBuffer);
    client->frameBuffer = data;
    return TRUE;
}

/**
 * Register an update from the server in the client framebuffer.
 *
 * @param client Handle to the VNC client.
 * @param x Left bound of the updated rectangle (in pixels).
 * @param y Top bound of the updated rectangle (in pixels).
 * @param w Width of the updated rectangle (in pixels).
 * @param h Height of the updated rectangle (in pixels).
 */
void update_framebuf(rfbClient* client, int x, int y, int w, int h)
{
    // Ignore off-screen updates
    if (x < 0 || y < 0 || x > RM_SCREEN_COLS || y > RM_SCREEN_ROWS)
    {
        return;
    }

    // Clip overflowing updates
    if (x + w > RM_SCREEN_COLS)
    {
        w = RM_SCREEN_COLS - x;
    }

    if (y + h > RM_SCREEN_ROWS)
    {
        h = RM_SCREEN_ROWS - y;
    }

    print_log("Update");
    fprintf(stderr, "%dx%d+%d+%d\n", w, h, x, y);

    // Copy the changed region from the client to the device framebuffer
    // (note that this does not cause a screen refresh)
    int framebuf_fp = *(int*) rfbClientGetClientData(client, FRAMEBUF_FP_TAG);
    off_t new_position = lseek(
        framebuf_fp,
        (y * (RM_SCREEN_COLS + RM_SCREEN_COL_PAD) + x) * RM_SCREEN_DEPTH,
        SEEK_SET
    );

    if (new_position == -1)
    {
        perror("update_framebuf:initial lseek");
        exit(EXIT_FAILURE);
    }

    // Seek to the first pixel in the client framebuffer
    uint8_t* framebuf_client = client->frameBuffer
        + (y * client->width + x) * RM_SCREEN_DEPTH;

    for (int row = 0; row < h; ++row)
    {
        // Write line buffer to device
        size_t length = w * RM_SCREEN_DEPTH;

        while (length)
        {
            ssize_t bytes_written = write(
                framebuf_fp,
                framebuf_client,
                length
            );

            if (bytes_written == -1)
            {
                perror("update_framebuf:write");
                exit(EXIT_FAILURE);
            }

            length -= bytes_written;
            framebuf_client += bytes_written;
        }

        // Seek to the next line
        new_position = lseek(
            framebuf_fp,
            (RM_SCREEN_COLS + RM_SCREEN_COL_PAD - w) * RM_SCREEN_DEPTH,
            SEEK_CUR
        );

        if (new_position == -1)
        {
            perror("update_framebuf:line lseek");
            exit(EXIT_FAILURE);
        }

        framebuf_client += (client->width - w) * RM_SCREEN_DEPTH;
    }

    // Mark that screen as needing a refresh
    short* needs_refresh = rfbClientGetClientData(client, NEEDS_REFRESH_TAG);
    *needs_refresh = 1;
}

int main(int argc, char** argv)
{
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

    if (!rfbInitClient(client, &argc, argv))
    {
        return EXIT_FAILURE;
    }

    // Open the system framebuffer
    int framebuf_fp = open("/dev/fb0", O_WRONLY);

    if (framebuf_fp == -1)
    {
        perror("open /dev/fb0");
        return EXIT_FAILURE;
    }

    // Flag activated when the screen needs a refresh
    short needs_refresh = 0;

    // Last time the screen was refreshed
    uint64_t last_refresh = 0;

    rfbClientSetClientData(client, FRAMEBUF_FP_TAG, &framebuf_fp);
    rfbClientSetClientData(client, NEEDS_REFRESH_TAG, &needs_refresh);

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

            print_log("Refresh");
            fprintf(stderr, "\n");

            trigger_refresh(framebuf_fp, 0, 0, RM_SCREEN_COLS, RM_SCREEN_ROWS);
        }
    }

    rfbClientCleanup(client);
    close(framebuf_fp);
    return EXIT_SUCCESS;
}
