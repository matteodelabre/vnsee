#include "defs.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <rfb/rfbclient.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define FRAMEBUF_FP_TAG 0

rfbBool create_framebuf(rfbClient* client)
{
    uint8_t* data = malloc(
        client->width
        * client->height
        * client->format.bitsPerPixel
    );

    assert(client->width == SCREEN_COLS + SCREEN_COL_PAD);
    assert(client->height == SCREEN_ROWS);
    assert(client->format.bitsPerPixel == 16);

    if (!data)
    {
        perror("create_framebuf");
        return FALSE;
    }

    free(client->frameBuffer);
    client->frameBuffer = data;
    return TRUE;
}

void update_framebuf(rfbClient* client, int x, int y, int w, int h)
{
    int shift = (y * (SCREEN_COLS + SCREEN_COL_PAD) + x) * 2;

    // Write received framebuffer to client framebuffer
    int framebuf_fp = *(int*) rfbClientGetClientData(client, FRAMEBUF_FP_TAG);
    lseek(framebuf_fp, shift, SEEK_SET);

    uint8_t* framebuf_client = client->frameBuffer + shift;

    for (int line = 0; line < h; ++line)
    {
        write(framebuf_fp, framebuf_client, w * 2);
        lseek(framebuf_fp, (SCREEN_COLS + SCREEN_COL_PAD - w) * 2, SEEK_CUR);
        framebuf_client += (SCREEN_COLS + SCREEN_COL_PAD) * 2;
    }

    // TODO: Only refresh updated zone
    // Ask for refresh
    mxcfb_update_data data;
    data.update_region.top = 0;
    data.update_region.left = 0;
    data.update_region.width = SCREEN_COLS;
    data.update_region.height = SCREEN_ROWS;
    data.waveform_mode = 0x0002;
    data.temp = 0;
    data.update_mode = 0;
    data.update_marker = 0x002a;
    data.flags = 0;
    ioctl(framebuf_fp, SEND_UPDATE, &data);
}

int main(int argc, char** argv)
{
    rfbClient* client = rfbGetClient(
        /* bitsPerSample = */ 16,
        /* samplesPerPixel = */ 1,
        /* bytesPerPixel = */ 2
    );

    client->MallocFrameBuffer = create_framebuf;
    client->GotFrameBufferUpdate = update_framebuf;

    if (!rfbInitClient(client, &argc, argv))
    {
        return EXIT_FAILURE;
    }

    // Open system framebuffer
    int framebuf_fp = open("/dev/fb0", O_WRONLY);

    if (framebuf_fp == -1)
    {
        perror("open /dev/fb0");
        return EXIT_FAILURE;
    }

    rfbClientSetClientData(client, FRAMEBUF_FP_TAG, &framebuf_fp);

    // RFB protocol message loop
    while (TRUE)
    {
        int result = WaitForMessage(client, /* timeout μs = */ 1000000);

        if (result < 0)
        {
            perror("WaitForMessage");
            break;
        }

        if (result > 0)
        {
            if (!HandleRFBServerMessage(client))
            {
                fprintf(stderr, "Unable to handle message\n");
                break;
            }
        }
    }

    rfbClientCleanup(client);
    close(framebuf_fp);
    return EXIT_SUCCESS;
}
