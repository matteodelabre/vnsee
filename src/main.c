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

#define FRAMEBUF_FP_TAG ((void*) 0)
#define UPDATE_DATA_TAG ((void*) 1)

/**
 * Maximum time (in microseconds) between two updates to join them in
 * the same batch.
 */
#define BATCH_WINDOW_US 500000

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

    /** Time of the latest update. */
    uint64_t update_time;

    /** Whether an update is pending. */
    short needs_flush;
};

/**
 * Initialize the client framebuffer, a buffer in which screen updates from
 * the server are stored.
 *
 * @param client Handle to the VNC client.
 * @return True if the new buffer was successfully allocated.
 */
rfbBool create_framebuf(rfbClient* client)
{
    uint8_t* data = malloc(
        client->width
        * client->height
        * client->format.bitsPerPixel / 8
    );

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
    if (x > RM_SCREEN_COLS || y > RM_SCREEN_ROWS)
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

    if (update->needs_flush)
    {
        fprintf(
            stderr,
            "%d x %d + %d x %d -> ",
            update->w, update->h, update->x, update->y
        );

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
        fprintf(stderr, "0 x 0 + 0 x 0 -> ");

        update->x = x;
        update->y = y;
        update->w = w;
        update->h = h;
        update->needs_flush = 1;
    }

    fprintf(
        stderr,
        "%d x %d + %d x %d\n",
        update->w, update->h, update->x, update->y
    );

    update->update_time = get_time_us();
}

/**
 * Flush pending updates from the client framebuffer to the device framebuffer.
 *
 * @param client Handle to the VNC client.
 * @param update Information about pending updates.
 */
void flush_framebuf(rfbClient* client, struct update_data* update)
{
    if (!update->needs_flush)
    {
        return;
    }

    // Batch sequences of updates together
    uint64_t cur_time = get_time_us();

    if (update->update_time + BATCH_WINDOW_US >= cur_time)
    {
        return;
    }

    update->needs_flush = 0;

    int x = update->x;
    int y = update->y;
    int w = update->w;
    int h = update->h;

    const size_t framebuf_client_depth = client->format.bitsPerPixel / 8;
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
        perror("update_framebuf");
        exit(EXIT_FAILURE);
    }

    // Seek to the first pixel in the client framebuffer
    uint8_t* framebuf_client = client->frameBuffer
        + (y * client->width + x) * framebuf_client_depth;

    uint16_t* linebuf = malloc(w * sizeof(*linebuf));

    if (!linebuf)
    {
        perror("update_framebuf");
        exit(EXIT_FAILURE);
    }

    for (int row = 0; row < h; ++row)
    {
        // Convert RGB triplet to grayscale
        for (int col = 0; col < w; ++col)
        {
            linebuf[col] = (
                21 * framebuf_client[0]
                + 72 * framebuf_client[1]
                + 7 * framebuf_client[2]
            ) * 257 / 100;

            framebuf_client += framebuf_client_depth;
        }

        // Write line buffer to device
        size_t linebuf_size = w * sizeof(*linebuf);
        uint16_t* linebuf_ptr = linebuf;

        while (linebuf_size)
        {
            ssize_t bytes_written = write(
                framebuf_fp,
                linebuf_ptr,
                linebuf_size
            );

            if (bytes_written == -1)
            {
                perror("update_framebuf");
                exit(EXIT_FAILURE);
            }

            linebuf_size -= bytes_written;
            linebuf_ptr += bytes_written;
        }

        // Seek to the next line
        new_position = lseek(
            framebuf_fp,
            (RM_SCREEN_COLS + RM_SCREEN_COL_PAD - w) * RM_SCREEN_DEPTH,
            SEEK_CUR
        );

        if (new_position == -1)
        {
            perror("update_framebuf");
            exit(EXIT_FAILURE);
        }

        framebuf_client += (client->width - w) * framebuf_client_depth;
    }

    free(linebuf);
    trigger_refresh(framebuf_fp, x, y, w, h);
}

int main(int argc, char** argv)
{
    rfbClient* client = rfbGetClient(
        /* bitsPerSample = */ 8,
        /* samplesPerPixel = */ 3,
        /* bytesPerPixel = */ 4
    );

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

    struct update_data update = {0};

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

        flush_framebuf(client, &update);
    }

    rfbClientCleanup(client);
    close(framebuf_fp);
    return EXIT_SUCCESS;
}
