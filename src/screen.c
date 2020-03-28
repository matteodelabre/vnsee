#include "screen.h"
#include "util.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>

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

void trigger_refresh(int fd)
{
    static uint32_t update_marker = 1;

    print_log("Refresh start");
    fprintf(stderr, "Marker = %" PRIu32 "\n", update_marker);

    struct mxcfb_update_data update;
    update.update_region.top = 0;
    update.update_region.left = 0;
    update.update_region.width = RM_SCREEN_COLS;
    update.update_region.height = RM_SCREEN_ROWS;
    update.waveform_mode = 0x0002;
    update.temp = 0;
    update.update_mode = 0;
    update.update_marker = update_marker;
    update.flags = 0;
    ioctl(fd, MXCFB_SEND_UPDATE, &update);

    struct mxcfb_update_marker_data wait;
    wait.update_marker = update_marker;
    wait.collision_test = 0;
    ioctl(fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &wait);

    print_log("Refresh end");
    fprintf(stderr, "Marker = %" PRIu32 "\n", update_marker);

    if (update_marker == 255)
    {
        update_marker = 1;
    }
    else
    {
        ++update_marker;
    }
}
