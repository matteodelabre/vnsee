#ifndef REFRESH_H
#define REFRESH_H

/** Width of the reMarkable screen. */
#define RM_SCREEN_COLS 1404

/** Number of padding pixels at the end of each line. */
#define RM_SCREEN_COL_PAD 4

/** Height of the reMarkable screen. */
#define RM_SCREEN_ROWS 1872

/** Depth of each screen pixel in bytes. */
#define RM_SCREEN_DEPTH 2

void trigger_refresh(int fb);

#endif // REFRESH_H
