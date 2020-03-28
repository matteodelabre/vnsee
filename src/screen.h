#ifndef SCREEN_H
#define SCREEN_H

#include <linux/fb.h>
#include <rfb/rfbclient.h>
#include <stdint.h>

/**
 * Information and resources for using the device screen.
 */
typedef struct rm_screen
{
    /** File descriptor for the device framebuffer. */
    int framebuf_fd;

    /** Variable screen information from the device framebuffer. */
    struct fb_var_screeninfo framebuf_varinfo;

    /** Fixed screen information from the device framebuffer. */
    struct fb_fix_screeninfo framebuf_fixinfo;

    /** Pointer to the memory-mapped framebuffer. */
    uint8_t* framebuf_ptr;

    /** Length of the memory-mapped framebuffer. */
    size_t framebuf_len;

    /** Next value to be used as an update marker. */
    uint32_t next_update_marker;
} rm_screen;

/**
 * Initialize the screen structure.
 */
rm_screen rm_screen_init();

/**
 * Update the given region of the screen.
 *
 * @param x Left bound of the region to update (in pixels).
 * @param y Top bound of the region to update (in pixels).
 * @param w Width of the region to update (in pixels).
 * @param h Height of the region to update (in pixels).
 */
void rm_screen_update(rm_screen* screen, int x, int y, int w, int h);

/**
 * Free resources held by the screen structure.
 */
void rm_screen_free(rm_screen* screen);

#endif // SCREEN_H
