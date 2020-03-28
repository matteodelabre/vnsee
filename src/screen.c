#include "screen.h"
#include "util.h"
#include <inttypes.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

/* Adapted from include/uapi/linux/mxcfb.h in the reMarkable kernel tree */
struct mxcfb_rect
{
    uint32_t top;
    uint32_t left;
    uint32_t width;
    uint32_t height;
};

struct mxcfb_alt_buffer_data
{
    uint32_t phys_addr;
    uint32_t width; /* width of entire buffer */
    uint32_t height; /* height of entire buffer */
    struct mxcfb_rect alt_update_region; /* region within buffer to update */
};

struct mxcfb_update_data
{
    struct mxcfb_rect update_region;
    uint32_t waveform_mode;
    uint32_t update_mode;
    uint32_t update_marker;
    int temp;
    unsigned int flags;
    int dither_mode;
    int quant_bit;
    struct mxcfb_alt_buffer_data alt_buffer_data;
};

#define MXCFB_SEND_UPDATE \
    _IOW('F', 0x2E, struct mxcfb_update_data)

struct mxcfb_update_marker_data
{
    uint32_t update_marker;
    uint32_t collision_test;
};

#define MXCFB_WAIT_FOR_UPDATE_COMPLETE \
    _IOWR('F', 0x2F, struct mxcfb_update_marker_data)

rm_screen rm_screen_init()
{
    rm_screen screen;

    screen.framebuf_fd = open("/dev/fb0", O_RDWR);

    if (screen.framebuf_fd == -1)
    {
        perror("open device framebuffer");
        exit(EXIT_FAILURE);
    }

    if (ioctl(
            screen.framebuf_fd,
            FBIOGET_VSCREENINFO,
            &screen.framebuf_varinfo
        ) == -1)
    {
        perror("ioctl framebuffer vscreeninfo");
        exit(EXIT_FAILURE);
    }

    if (ioctl(
            screen.framebuf_fd,
            FBIOGET_FSCREENINFO,
            &screen.framebuf_fixinfo
        ) == -1)
    {
        perror("ioctl framebuffer fscreeninfo");
        exit(EXIT_FAILURE);
    }

    screen.framebuf_len = screen.framebuf_fixinfo.smem_len;
    screen.framebuf_ptr = mmap(
        /* addr = */ NULL,
        /* len = */ screen.framebuf_len,
        /* prot = */ PROT_READ | PROT_WRITE,
        /* flags = */ MAP_SHARED,
        /* fd = */ screen.framebuf_fd,
        /* off = */ 0
    );

    screen.next_update_marker = 1;
    return screen;
}

void rm_screen_update(rm_screen* screen)
{
    print_log("Refresh start");
    fprintf(stderr, "Marker = %" PRIu32 "\n", screen->next_update_marker);

    struct mxcfb_update_data update;
    update.update_region.top = 0;
    update.update_region.left = 0;
    update.update_region.width = screen->framebuf_varinfo.xres;
    update.update_region.height = screen->framebuf_varinfo.yres;
    update.waveform_mode = 0x0002;
    update.temp = 0;
    update.update_mode = 0;
    update.update_marker = screen->next_update_marker;
    update.flags = 0;
    ioctl(screen->framebuf_fd, MXCFB_SEND_UPDATE, &update);

    // TODO: Check for ioctl failure

    struct mxcfb_update_marker_data wait;
    wait.update_marker = screen->next_update_marker;
    wait.collision_test = 0;
    ioctl(screen->framebuf_fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &wait);

    print_log("Refresh end");
    fprintf(stderr, "Marker = %" PRIu32 "\n", screen->next_update_marker);

    if (screen->next_update_marker == 255)
    {
        screen->next_update_marker = 1;
    }
    else
    {
        ++screen->next_update_marker;
    }
}

void rm_screen_free(rm_screen* screen)
{
    munmap(screen->framebuf_ptr, screen->framebuf_len);
    screen->framebuf_ptr = NULL;
    screen->framebuf_len = 0;

    close(screen->framebuf_fd);
    screen->framebuf_fd = -1;
}
