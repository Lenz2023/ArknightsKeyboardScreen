#pragma once

#include <stddef.h>
#include <stdint.h>

// User supplied artwork, uploaded through the built-in web server.
//
// Backgrounds follow the same weather/time buckets the built-in scenery uses, so
// a custom picture only replaces the condition it was uploaded for:
//   0 clear   1 cloudy   2 rain   3 snow   4 dusk   5 night
// Home artwork is a single slot drawn on the left of the home screen.
//
// Everything is decoded once at upload time into a ready-to-draw bitmap, so the
// GUI never touches a PNG while it holds the LVGL lock.
namespace artstore
{
    constexpr int backgroundWidth = 320;
    constexpr int backgroundHeight = 240;
    constexpr int homeWidth = 176;
    constexpr int homeHeight = 226;

    constexpr size_t backgroundBytes = size_t(backgroundWidth) * backgroundHeight * 2;
    constexpr size_t homeBytes = size_t(homeWidth) * homeHeight * 3;
    constexpr int backgroundSlots = 6;

    // Weather code + time band -> background slot, matching the scenery switch.
    int backgroundSlotFor(int weatherCode, int hour);
    // Short name used by the UI and the upload endpoints ("clear", "night", ...).
    const char *backgroundSlotName(int slot);
    // Accepts the short name, returns -1 when unknown.
    int backgroundSlotFromName(const char *name);

    // Where the home artwork sits and how big it is drawn. `zoom` uses the LVGL
    // scale where 256 is 100%, so the panel can be nudged without re-uploading.
    struct HomeLayout
    {
        int16_t x;
        int16_t y;
        uint16_t zoom;
    };

    HomeLayout homeLayout();
    bool saveHomeLayout(int x, int y, int zoom);
    // Bumped when only the placement changed, so the screen can move the image
    // without decoding the file again.
    uint32_t homeLayoutRevision();

    bool saveBackground(int slot, const uint8_t *png, size_t size, char *error, size_t errorSize);
    bool saveHomeImage(const uint8_t *png, size_t size, char *error, size_t errorSize);

    bool hasBackground(int slot);
    bool hasHomeImage();
    void clearBackground(int slot);
    void clearHomeImage();

    // Caller owns the returned PSRAM buffer (free() it) or gets nullptr.
    uint8_t *loadBackground(int slot);
    uint8_t *loadHomeImage();

    // Bumped on every change so screens can pick up new artwork when convenient.
    uint32_t revision();
}
