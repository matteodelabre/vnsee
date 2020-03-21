#include "defs.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int create_server_socket(short port)
{
    int srv = socket(AF_INET, SOCK_DGRAM, 0);

    if (srv == -1)
    {
        perror("Unable to create server socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr = {0};
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (bind(srv, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    {
        perror("Unable to bind server socket");
        exit(EXIT_FAILURE);
    }

    return srv;
}

typedef struct {
    uint32_t top;
    uint32_t left;
    uint32_t width;
    uint32_t height;
} mxcfb_rect;

typedef struct {
    uint32_t phys_addr;
    uint32_t width; /* width of entire buffer */
    uint32_t height; /* height of entire buffer */
    mxcfb_rect alt_update_region; /* region within buffer to update */
} mxcfb_alt_buffer_data;

typedef struct {
    mxcfb_rect update_region;
    uint32_t waveform_mode;
    uint32_t update_mode;
    uint32_t update_marker;
    int temp;
    unsigned int flags;
    int dither_mode;
    int quant_bit;
    mxcfb_alt_buffer_data alt_buffer_data;
} mxcfb_update_data;

const int SEND_UPDATE = 0x4048462e;

void write_frame(int fb, void* buffer, size_t buffer_size)
{
    lseek(fb, 0, SEEK_SET);

    while (buffer_size > 0)
    {
        ssize_t bytes_written = write(fb, buffer, buffer_size);

        if (bytes_written == -1)
        {
            perror("Could not write to the framebuffer");
            exit(EXIT_FAILURE);
        }

        buffer += bytes_written;
        buffer_size -= bytes_written;
    }

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
    ioctl(fb, SEND_UPDATE, &data);
}

int main(int argc, char** argv)
{
    int srv = create_server_socket(/* port = */ 7777);
    int fb = open("/dev/fb0", O_WRONLY);

    uint16_t screen_buffer[(SCREEN_COLS + SCREEN_COL_PAD) * SCREEN_ROWS];
    line_update message;

    while (1)
    {
        ssize_t bytes_read = recvfrom(
            srv, &message, sizeof(message),
            /* flags = */ 0,
            /* address = */ NULL,
            /* address_len = */ NULL
        );

        if (message.line_idx >= SCREEN_ROWS)
        {
            fprintf(stderr, "Line index too big (%d)\n", message.line_idx);
            continue;
        }

        memcpy(
            screen_buffer + (SCREEN_COLS + SCREEN_COL_PAD) * message.line_idx,
            message.buffer,
            2 * SCREEN_COLS
        );

        if (message.line_idx + 1 == SCREEN_ROWS)
        {
            printf("Flushing buffer\n");
            write_frame(fb, screen_buffer, sizeof(screen_buffer));
        }
    }

    close(srv);
    close(fb);
}
