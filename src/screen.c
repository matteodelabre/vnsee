#include "mxcfb.h"
#include "screen.h"
#include "util.h"
#include <inttypes.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>

rm_screen rm_screen_init()
{
    rm_screen screen;

    screen.framebuf_fd = open("/dev/fb0", O_RDWR);

    if (screen.framebuf_fd == -1)
    {
        perror("rm_screen_init - open device framebuffer");
        exit(EXIT_FAILURE);
    }

    if (ioctl(
            screen.framebuf_fd,
            FBIOGET_VSCREENINFO,
            &screen.framebuf_varinfo
        ) == -1)
    {
        perror("rm_screen_init - ioctl framebuffer vscreeninfo");
        exit(EXIT_FAILURE);
    }

    if (ioctl(
            screen.framebuf_fd,
            FBIOGET_FSCREENINFO,
            &screen.framebuf_fixinfo
        ) == -1)
    {
        perror("rm_screen_init - ioctl framebuffer fscreeninfo");
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

void rm_screen_update(rm_screen* screen, int x, int y, int w, int h)
{
    // Clip update region to screen bounds
    if (x < 0)
    {
        w += x;
        x = 0;
    }

    if (y < 0)
    {
        h += y;
        y = 0;
    }

    if (x + w > screen->framebuf_varinfo.xres)
    {
        w = screen->framebuf_varinfo.xres - x;
    }

    if (y + h > screen->framebuf_varinfo.yres)
    {
        h = screen->framebuf_varinfo.yres - y;
    }

    // Ignore out of bounds or null updates
    if (x > screen->framebuf_varinfo.xres
        || y > screen->framebuf_varinfo.yres
        || w == 0 || h == 0)
    {
        return;
    }

#ifdef TRACE
    print_log("Screen Update");
    fprintf(stderr, "%dx%d+%d+%d\n", w, h, x, y);
#endif // TRACE

    struct mxcfb_update_data update;
    update.update_region.left = x;
    update.update_region.top = y;
    update.update_region.width = w;
    update.update_region.height = h;
    update.waveform_mode = MXCFB_WAVEFORM_MODE_GC16;
    update.temp = 0x18;
    update.update_mode = MXCFB_UPDATE_MODE_PARTIAL;
    update.flags = 0;
    update.update_marker = screen->next_update_marker;

    if (ioctl(screen->framebuf_fd, MXCFB_SEND_UPDATE, &update) == -1)
    {
        perror("rm_screen_update - ioctl send update");
        exit(EXIT_FAILURE);
    }

    struct mxcfb_update_marker_data wait;
    wait.update_marker = screen->next_update_marker;
    wait.collision_test = 0;

    if (ioctl(screen->framebuf_fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &wait) == -1)
    {
        perror("rm_screen_update - ioctl wait for update complete");
        exit(EXIT_FAILURE);
    }

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
