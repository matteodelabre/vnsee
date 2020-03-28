#include "screen.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>

/* Adapted from include/uapi/linux/mxcfb.h in the kernel tree */
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

struct mxcfb_update_marker_data
{
    uint32_t update_marker;
    uint32_t collision_test;
};

#define MXCFB_SEND_UPDATE \
    _IOW('F', 0x2E, struct mxcfb_update_data)

void trigger_refresh(int fb, int x, int y, int w, int h)
{
    static uint32_t update_marker = 0;

    struct mxcfb_update_data update;
    update.update_region.top = x;
    update.update_region.left = y;
    update.update_region.width = w;
    update.update_region.height = h;
    update.waveform_mode = 0x0002;
    update.temp = 0;
    update.update_mode = 0;
    update.update_marker = update_marker;
    update.flags = 0;
    ioctl(fb, MXCFB_SEND_UPDATE, &update);

    ++update_marker;
}
