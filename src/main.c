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
 * Tag used for storing/retrieving the device framebuffer’s
 * file descriptor in the client object.
 */
#define FRAMEBUF_FP_TAG ((void*) 0)

/**
 * Tag used for storing/retrieving update information in the client object.
 */
#define UPDATE_DATA_TAG ((void*) 1)

/**
 * Maximum time in microseconds to wait between two updates are flushed
 * to the device framebuffer.
 */
#define FLUSH_RATE 500000

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
 * Print the current monotonic time up to the microsecond.
 *
 * @param header Header string to print.
 */
void print_time(const char* header)
{
    struct timespec cur_time_st;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time_st);

    fprintf(
        stderr,
        "\n[%s @ %ld.%ld]\n",
        header,
        cur_time_st.tv_sec,
        cur_time_st.tv_nsec / 1000
    );
}

/**
 * Information about pending updates.
 */
struct update_data
{
    /** Left bound of the updated rectangle (in pixels). */
    int x;

    /** Top bound of the updated rectangle (in pixels). */
    int y;

    /** Width of the updated rectangle (in pixels) */
    int w;

    /** Height of the updated rectangle (in pixels). */
    int h;

    /** Whether an update is stored. */
    short has_update;
};

/**
 * Initialize the client framebuffer, a buffer in which screen data from
 * the server is stored before being flushed to the device framebuffer.
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

    // Register the region as pending update, potentially extending
    // an existing one
    struct update_data* update = rfbClientGetClientData(
        client, UPDATE_DATA_TAG);

    print_time("Update");
    fprintf(stderr, "%d x %d + %d x %d\n", w, h, x, y);

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
}

/**
 * Flush pending updates from the client framebuffer to the device framebuffer.
 *
 * @param client Handle to the VNC client.
 * @param update Information about pending updates.
 */
void flush_framebuf(rfbClient* client, struct update_data* update)
{
    if (!update->has_update)
    {
        return;
    }

    update->has_update = 0;

    int x = update->x;
    int y = update->y;
    int w = update->w;
    int h = update->h;

    print_time("Flush ");
    fprintf(stderr, "%d x %d + %d x %d\n", w, h, x, y);

    // Seek to the first pixel in the device framebuffer
    int framebuf_fp = *(int*) rfbClientGetClientData(client, FRAMEBUF_FP_TAG);
    off_t new_position = lseek(
        framebuf_fp,
        (y * (RM_SCREEN_COLS + RM_SCREEN_COL_PAD) + x) * RM_SCREEN_DEPTH,
        SEEK_SET
    );

    if (new_position == -1)
    {
        perror("flush_framebuf:initial lseek");
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
                perror("flush_framebuf:write");
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
            perror("flush_framebuf:line lseek");
            exit(EXIT_FAILURE);
        }

        framebuf_client += (client->width - w) * RM_SCREEN_DEPTH;
    }

    trigger_refresh(framebuf_fp, 0, 0, RM_SCREEN_COLS, RM_SCREEN_ROWS);
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

    // Information about pending updates
    struct update_data update = {0};

    // Last time the client framebuffer was flushed to the device framebuffer
    uint64_t last_flush = 0;

    rfbClientSetClientData(client, FRAMEBUF_FP_TAG, &framebuf_fp);
    rfbClientSetClientData(client, UPDATE_DATA_TAG, &update);

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

        // Batch sequences of updates together
        uint64_t cur_time = get_time_us();

        if (last_flush + FLUSH_RATE < cur_time)
        {
            flush_framebuf(client, &update);
            last_flush = cur_time;
        }
    }

    rfbClientCleanup(client);
    close(framebuf_fp);
    return EXIT_SUCCESS;
}
