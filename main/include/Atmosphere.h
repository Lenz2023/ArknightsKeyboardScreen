#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Shared weather / time-of-day backdrop.
//
// Every page paints the same three layers so the device reads as one shell
// instead of a patchwork of flat colours, a white login screen and one-off
// interior scenes:
//
//   1. the home scenery image (clear / cloudy / rain / snow by weather, and
//      dusk / night when the clock says so),
//   2. a time-of-day tint (daytime clear, dusk warm, night blue),
//   3. an optional constant veil that keeps text legible over the art.
//
// The scenery comes from the game's own home-screen scenes, mirrored by PRTS
// (https://prts.wiki/w/首页场景一览) and baked into scene_home_bg.c.
//
// Call atmosphereAttach() first and create the page content afterwards, so the
// content stays above the backdrop. Both functions require the LVGL lock.
void atmosphereAttach(lv_obj_t *screen, uint32_t veilColor, uint8_t veilOpa);

// Re-evaluates the weather scene and the time band. Cheap when nothing changed,
// so it is safe to call from a page loop (expects the LVGL lock to be held).
void atmosphereRefresh(void);

// Short latin caption for the scenery currently shown, e.g. "NIGHT". Pages that
// print a banner can use it to name what the user is looking at.
const char *atmosphereSceneLabel(void);
// Pixels currently behind the pages (RGB565, 320x240) plus their size. Used by the
// web preview so it can show the real home screen; the buffer is owned elsewhere
// and must not be kept or freed.
const uint8_t *atmosphereCurrentPixels(int *width, int *height);

// Forgets the cached layer pointers. The screen still owns and deletes the
// objects, so pages must call this from destroy().
void atmosphereDetach(void);

#ifdef __cplusplus
}
#endif
