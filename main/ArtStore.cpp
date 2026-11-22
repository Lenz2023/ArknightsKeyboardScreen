#include "A_Config.h"
#include "ArtStore.h"

#include <LittleFS.h>
#include <Preferences.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "esp_heap_caps.h"
#include "esp_log.h"

#define LODEPNG_NO_COMPILE_CPP
extern "C"
{
#include "src/extra/libs/png/lodepng.h"
}

namespace artstore
{
    namespace
    {
        const char *const slotNames[backgroundSlots] = {"clear", "cloudy", "rain", "snow", "dusk", "night"};

        // Decoding costs four bytes per source pixel in PSRAM; keep the ceiling low
        // enough that a large phone photo cannot starve the display buffers.
        constexpr size_t maxSourcePixels = 1000u * 1000u;
        constexpr size_t maxPngBytes = 6u * 1024u * 1024u;

        uint32_t s_revision = 1;
        uint32_t s_layout_revision = 1;
        constexpr int16_t defaultHomeX = -6;
        constexpr int16_t defaultHomeY = 2;
        constexpr uint16_t defaultHomeZoom = 248;

        Preferences &layoutPrefs()
        {
            static Preferences prefs;
            static bool opened = false;
            if (!opened)
            {
                prefs.begin("artstore", false);
                opened = true;
            }
            return prefs;
        }

        void fail(char *error, size_t errorSize, const char *text)
        {
            if (error && errorSize) snprintf(error, errorSize, "%s", text);
        }

        void backgroundPath(int slot, char *out, size_t size)
        {
            snprintf(out, size, "/user-bg-%s.bin", slotNames[std::clamp(slot, 0, backgroundSlots - 1)]);
        }

        bool decode(const uint8_t *png, size_t size, unsigned &width, unsigned &height,
                    uint8_t *&rgba, char *error, size_t errorSize)
        {
            rgba = nullptr;
            if (!png || size < 8)
            {
                fail(error, errorSize, "文件为空");
                return false;
            }
            if (size > maxPngBytes)
            {
                fail(error, errorSize, "文件过大（上限 6MB）");
                return false;
            }
            unsigned w = 0, h = 0;
            LodePNGState state;
            lodepng_state_init(&state);
            const unsigned head = lodepng_inspect(&w, &h, &state, png, size);
            lodepng_state_cleanup(&state);
            if (head != 0 || !w || !h)
            {
                fail(error, errorSize, "仅支持 PNG（无法解析）");
                return false;
            }
            if (size_t(w) * h > maxSourcePixels)
            {
                fail(error, errorSize, "图片太大，请先缩到 1280x720 以内");
                return false;
            }
            uint8_t *pixels = nullptr;
            const unsigned err = lodepng_decode32(&pixels, &w, &h, png, size);
            if (err != 0 || !pixels)
            {
                fail(error, errorSize, "PNG 解码失败（请用标准 PNG）");
                return false;
            }
            width = w;
            height = h;
            rgba = pixels;
            return true;
        }

        // Box-filtered scaling with premultiplied alpha, so a big photo lands softly
        // instead of aliasing the way nearest neighbour would.
        void sample(const uint8_t *rgba, unsigned sw, unsigned sh,
                    unsigned sx0, unsigned sy0, unsigned sx1, unsigned sy1,
                    uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a)
        {
            unsigned long long rr = 0, gg = 0, bb = 0, aa = 0, n = 0;
            sx1 = std::max(sx0 + 1, sx1);
            sy1 = std::max(sy0 + 1, sy1);
            for (unsigned y = sy0; y < sy1 && y < sh; ++y)
            {
                const uint8_t *row = rgba + 4u * size_t(y) * sw;
                for (unsigned x = sx0; x < sx1 && x < sw; ++x)
                {
                    const uint8_t *px = row + 4u * x;
                    const unsigned alpha = px[3];
                    rr += unsigned(px[0]) * alpha;
                    gg += unsigned(px[1]) * alpha;
                    bb += unsigned(px[2]) * alpha;
                    aa += alpha;
                    ++n;
                }
            }
            if (!n || !aa)
            {
                r = g = b = 0;
                a = 0;
                return;
            }
            r = uint8_t(rr / aa);
            g = uint8_t(gg / aa);
            b = uint8_t(bb / aa);
            a = uint8_t(aa / n);
        }

        bool writeFile(const char *temp, const char *target, const uint8_t *data, size_t size)
        {
            File f = LittleFS.open(temp, "w");
            if (!f) return false;
            const bool ok = f.write(data, size) == size;
            f.close();
            if (!ok)
            {
                LittleFS.remove(temp);
                return false;
            }
            LittleFS.remove(target);
            return LittleFS.rename(temp, target);
        }

        bool existsWithSize(const char *path, size_t size)
        {
            File f = LittleFS.open(path, "r");
            if (!f) return false;
            const bool ok = size_t(f.size()) == size;
            f.close();
            return ok;
        }
    } // namespace

    const char *backgroundSlotName(int slot)
    {
        if (slot < 0 || slot >= backgroundSlots) return "clear";
        return slotNames[slot];
    }

    int backgroundSlotFromName(const char *name)
    {
        if (!name) return -1;
        for (int i = 0; i < backgroundSlots; ++i)
            if (strcmp(name, slotNames[i]) == 0) return i;
        return -1;
    }

    int backgroundSlotFor(int weatherCode, int hour)
    {
        // Night and dusk win over the weather, the same order the scenery uses.
        if (hour >= 20 || hour < 5) return 5;
        if ((hour >= 17 && hour < 20) || (hour >= 5 && hour < 6)) return 4;
        if (weatherCode == 0 || weatherCode == 38) return 0;
        if ((weatherCode >= 10 && weatherCode <= 20) || (weatherCode >= 33 && weatherCode <= 36)) return 2;
        if (weatherCode >= 21 && weatherCode <= 25) return 3;
        return 1;
    }

    bool saveBackground(int slot, const uint8_t *png, size_t size, char *error, size_t errorSize)
    {
        if (slot < 0 || slot >= backgroundSlots)
        {
            fail(error, errorSize, "未知的背景槽位");
            return false;
        }
        unsigned w = 0, h = 0;
        uint8_t *rgba = nullptr;
        if (!decode(png, size, w, h, rgba, error, errorSize)) return false;

        uint8_t *out = (uint8_t *)heap_caps_malloc(backgroundBytes, MALLOC_CAP_SPIRAM);
        if (!out)
        {
            free(rgba);
            fail(error, errorSize, "内存不足");
            return false;
        }
        // Cover the panel: the page already crops to 4:3, so this normally maps 1:1
        // and only kicks in when a non-cropped picture is posted straight to the API.
        const float scale = std::max(float(backgroundWidth) / w, float(backgroundHeight) / h);
        const float inv = 1.0f / scale;
        const float offX = (float(w) - float(backgroundWidth) * inv) * 0.5f;
        const float offY = (float(h) - float(backgroundHeight) * inv) * 0.5f;
        for (int y = 0; y < backgroundHeight; ++y)
        {
            for (int x = 0; x < backgroundWidth; ++x)
            {
                const unsigned sx0 = unsigned(std::max(0.0f, offX + x * inv));
                const unsigned sy0 = unsigned(std::max(0.0f, offY + y * inv));
                const unsigned sx1 = unsigned(std::max(0.0f, offX + (x + 1) * inv));
                const unsigned sy1 = unsigned(std::max(0.0f, offY + (y + 1) * inv));
                uint8_t r, g, b, a;
                sample(rgba, w, h, sx0, sy0, sx1, sy1, r, g, b, a);
                // Transparent parts of a posted PNG fade into the dark scene colour
                // instead of turning into a black hole on the panel.
                const unsigned keep = a;
                const unsigned drop = 255u - a;
                r = uint8_t((unsigned(r) * keep + 14u * drop) / 255u);
                g = uint8_t((unsigned(g) * keep + 23u * drop) / 255u);
                b = uint8_t((unsigned(b) * keep + 32u * drop) / 255u);
                const lv_color_t color = lv_color_make(r, g, b);
                memcpy(out + 2 * (size_t(y) * backgroundWidth + x), &color, 2);
            }
        }
        free(rgba);

        char path[48], temp[64];
        backgroundPath(slot, path, sizeof(path));
        snprintf(temp, sizeof(temp), "%s.tmp", path);
        const bool ok = writeFile(temp, path, out, backgroundBytes);
        free(out);
        if (!ok)
        {
            fail(error, errorSize, "写入 Flash 失败");
            return false;
        }
        ++s_revision;
        ESP_LOGI("ArtStore", "background '%s' stored from %ux%u", slotNames[slot], w, h);
        return true;
    }

    bool saveHomeImage(const uint8_t *png, size_t size, char *error, size_t errorSize)
    {
        unsigned w = 0, h = 0;
        uint8_t *rgba = nullptr;
        if (!decode(png, size, w, h, rgba, error, errorSize)) return false;

        uint8_t *out = (uint8_t *)heap_caps_malloc(homeBytes, MALLOC_CAP_SPIRAM);
        if (!out)
        {
            free(rgba);
            fail(error, errorSize, "内存不足");
            return false;
        }
        memset(out, 0, homeBytes);
        // Fit inside the slot, keep the cut-out's own silhouette, sit on the bottom.
        const float scale = std::min(float(homeWidth) / w, float(homeHeight) / h);
        const unsigned dw = std::max(1u, std::min<unsigned>(homeWidth, unsigned(w * scale + 0.5f)));
        const unsigned dh = std::max(1u, std::min<unsigned>(homeHeight, unsigned(h * scale + 0.5f)));
        const unsigned ox = (homeWidth - dw) / 2;
        const unsigned oy = homeHeight - dh;
        const float stepX = float(w) / dw;
        const float stepY = float(h) / dh;
        for (unsigned y = 0; y < dh; ++y)
        {
            for (unsigned x = 0; x < dw; ++x)
            {
                uint8_t r, g, b, a;
                sample(rgba, w, h,
                       unsigned(x * stepX), unsigned(y * stepY),
                       unsigned((x + 1) * stepX), unsigned((y + 1) * stepY),
                       r, g, b, a);
                uint8_t *dst = out + 3 * (size_t(oy + y) * homeWidth + ox + x);
                const lv_color_t color = lv_color_make(r, g, b);
                memcpy(dst, &color, 2);
                dst[2] = a;
            }
        }
        free(rgba);
        const bool ok = writeFile("/user-home.tmp", "/user-home.bin", out, homeBytes);
        free(out);
        if (!ok)
        {
            fail(error, errorSize, "写入 Flash 失败");
            return false;
        }
        ++s_revision;
        ESP_LOGI("ArtStore", "home artwork stored from %ux%u", w, h);
        return true;
    }

    bool hasBackground(int slot)
    {
        char path[48];
        backgroundPath(slot, path, sizeof(path));
        return existsWithSize(path, backgroundBytes);
    }

    bool hasHomeImage() { return existsWithSize("/user-home.bin", homeBytes); }

    void clearBackground(int slot)
    {
        char path[48];
        backgroundPath(slot, path, sizeof(path));
        LittleFS.remove(path);
        ++s_revision;
    }

    void clearHomeImage()
    {
        LittleFS.remove("/user-home.bin");
        ++s_revision;
    }

    uint8_t *loadBackground(int slot)
    {
        if (!hasBackground(slot)) return nullptr;
        char path[48];
        backgroundPath(slot, path, sizeof(path));
        auto *out = (uint8_t *)heap_caps_malloc(backgroundBytes, MALLOC_CAP_SPIRAM);
        if (!out) return nullptr;
        File f = LittleFS.open(path, "r");
        if (!f || f.read(out, backgroundBytes) != backgroundBytes)
        {
            if (f) f.close();
            free(out);
            return nullptr;
        }
        f.close();
        return out;
    }

    uint8_t *loadHomeImage()
    {
        if (!hasHomeImage()) return nullptr;
        auto *out = (uint8_t *)heap_caps_malloc(homeBytes, MALLOC_CAP_SPIRAM);
        if (!out) return nullptr;
        File f = LittleFS.open("/user-home.bin", "r");
        if (!f || f.read(out, homeBytes) != homeBytes)
        {
            if (f) f.close();
            free(out);
            return nullptr;
        }
        f.close();
        return out;
    }

    HomeLayout homeLayout()
    {
        Preferences &prefs = layoutPrefs();
        HomeLayout layout;
        layout.x = int16_t(prefs.getInt("homeX", defaultHomeX));
        layout.y = int16_t(prefs.getInt("homeY", defaultHomeY));
        layout.zoom = uint16_t(prefs.getInt("homeZoom", defaultHomeZoom));
        if (layout.zoom < 96) layout.zoom = 96;
        if (layout.zoom > 512) layout.zoom = 512;
        return layout;
    }

    bool saveHomeLayout(int x, int y, int zoom)
    {
        const int cx = std::clamp(x, -120, 120);
        const int cy = std::clamp(y, -120, 120);
        const int cz = std::clamp(zoom, 96, 512);
        Preferences &prefs = layoutPrefs();
        prefs.putInt("homeX", cx);
        prefs.putInt("homeY", cy);
        prefs.putInt("homeZoom", cz);
        ++s_layout_revision;
        ESP_LOGI("ArtStore", "home layout x=%d y=%d zoom=%d", cx, cy, cz);
        return true;
    }

    uint32_t homeLayoutRevision() { return s_layout_revision; }

    uint32_t revision() { return s_revision; }
}
