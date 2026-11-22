#include "A_Config.h"
#include "Atmosphere.h"

extern "C" const lv_font_t font_rajdhani_16;
extern "C" const lv_font_t lv_font_japanese_16;
extern "C" int weatherHomeCode();

static lv_obj_t *music_title;
static lv_obj_t *music_artist;
static lv_obj_t *music_lyric;
static lv_obj_t *music_next_lyric;
static lv_obj_t *music_status;
static lv_obj_t *music_progress;
static lv_obj_t *music_current_marker;
static lv_obj_t *music_next_marker;
static lv_font_t music_text_font;
static lv_font_t music_chinese_font;
struct LyricFade {
    lv_obj_t *outgoing = nullptr;
    char text[sizeof(hal.music.lyric)] = {};
    lv_coord_t base_y = 0;
    uint32_t started_at = 0;
    uint16_t frames = 0;
};
static LyricFade current_fade, next_fade;
static unsigned fade_reports = 0;
static uint32_t displayed_revision = UINT32_MAX;

static bool isAsciiAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool contractionSuffix(const char *text)
{
    static const char *suffixes[] = {"m", "t", "s", "d", "re", "ve", "ll"};
    for (const char *suffix : suffixes) {
        size_t n = strlen(suffix);
        bool match = true;
        for (size_t i = 0; i < n; ++i) {
            char c = text[i];
            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            if (c != suffix[i]) { match = false; break; }
        }
        if (match && (text[n] == 0 || text[n] == ' ' || text[n] == '\t' ||
                      text[n] == ',' || text[n] == '.' || text[n] == '!' || text[n] == '?'))
            return true;
    }
    return false;
}

static void setLyricText(lv_obj_t *label, const char *text)
{
    char normalized[sizeof(hal.music.lyric)];
    size_t in = 0, out = 0;
    while (text[in] && out + 1 < sizeof(normalized)) {
        // Some lyric parsers turn a straight apostrophe into an opening
        // parenthesis in English contractions (e.g. "I(m", "don(t").
        if (text[in] == '(' && out > 0 && isAsciiAlpha(normalized[out - 1]) &&
            contractionSuffix(text + in + 1)) {
            normalized[out++] = '\'';
            ++in;
            continue;
        }
        normalized[out++] = text[in++];
    }
    normalized[out] = '\0';
    lv_label_set_text(label, normalized);

    // Rajdhani draws ASCII punctuation cleanly; retain the CJK face whenever
    // the lyric contains translated Chinese text.
    bool ascii_only = true;
    for (const unsigned char *p = (const unsigned char *)normalized; *p; ++p) {
        if (*p >= 0x80) { ascii_only = false; break; }
    }
    lv_obj_set_style_text_font(label, ascii_only ? &font_rajdhani_16 : &music_text_font, 0);
}

static void fadeLyric(void *object, int32_t progress)
{
    auto *incoming = static_cast<lv_obj_t *>(object);
    auto &fade = incoming == music_lyric ? current_fade : next_fade;
    ++fade.frames;
    lv_obj_set_style_opa(incoming, (lv_opa_t)progress, 0);
    lv_obj_set_style_opa(fade.outgoing, (lv_opa_t)(LV_OPA_COVER - progress), 0);
    lv_obj_set_y(incoming, fade.base_y + (20 * (LV_OPA_COVER - progress)) / LV_OPA_COVER);
    lv_obj_set_y(fade.outgoing, fade.base_y - (20 * progress) / LV_OPA_COVER);
}

static void lyricFadeFinished(lv_anim_t *animation)
{
    auto *label = static_cast<lv_obj_t *>(animation->var);
    auto &fade = label == music_lyric ? current_fade : next_fade;
    lv_obj_set_y(label, fade.base_y);
    lv_obj_set_y(fade.outgoing, fade.base_y);
    lv_obj_set_style_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(fade.outgoing, LV_OPA_TRANSP, 0);
    if (label == music_lyric && fade_reports < 3) {
        ++fade_reports;
        ESP_LOGI("Music", "Lyric scroll complete: %u frames, %" PRIu32 " ms",
                 (unsigned)fade.frames, lv_tick_elaps(fade.started_at));
    }
}

static void updateLyric(lv_obj_t *label, LyricFade &fade, const char *text, bool animate)
{
    if (strcmp(fade.text, text) == 0) return;
    lv_anim_del(label, fadeLyric);
    if (animate) {
        setLyricText(fade.outgoing, fade.text);
        lv_obj_set_y(fade.outgoing, fade.base_y);
        lv_obj_set_style_opa(fade.outgoing, LV_OPA_COVER, 0);
        lv_obj_set_style_opa(label, LV_OPA_TRANSP, 0);
        lv_obj_set_y(label, fade.base_y + 20);
    } else {
        lv_obj_set_y(label, fade.base_y);
        lv_obj_set_style_opa(fade.outgoing, LV_OPA_TRANSP, 0);
        lv_obj_set_style_opa(label, LV_OPA_COVER, 0);
    }
    strlcpy(fade.text, text, sizeof(fade.text));
    setLyricText(label, text);
    if (!animate) return;

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, label);
    lv_anim_set_exec_cb(&anim, fadeLyric);
    lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
    fade.frames = 0;
    fade.started_at = lv_tick_get();
    lv_anim_set_time(&anim, 250);
    lv_anim_set_ready_cb(&anim, lyricFadeFinished);
    lv_anim_start(&anim);
}

static lv_obj_t *createLyricLabel(lv_obj_t *parent, lv_coord_t y, lv_coord_t h,
                                 uint32_t color, lv_coord_t line_space)
{
    auto *label = lv_label_create(parent);
    lv_label_set_text(label, "");
    lv_obj_set_size(label, 272, h);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(label, &music_text_font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_line_space(label, line_space, 0);
    lv_obj_set_pos(label, 30, y);
    return label;
}

static lv_obj_t *music_shape(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                             lv_coord_t w, lv_coord_t h, uint32_t color,
                             lv_opa_t opacity, lv_coord_t radius)
{
    lv_obj_t *shape = lv_obj_create(parent);
    lv_obj_remove_style_all(shape);
    lv_obj_set_size(shape, w, h);
    lv_obj_set_pos(shape, x, y);
    lv_obj_set_style_bg_color(shape, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(shape, opacity, 0);
    lv_obj_set_style_radius(shape, radius, 0);
    lv_obj_clear_flag(shape, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return shape;
}

static void updateMusicAtmosphere()
{
    // Same scenery as every other page; only the veil below differs.
    atmosphereRefresh();
}

static void updateMusicScreen()
{
    music_now_playing_t snapshot;
    hal.LOCKLV();
    snapshot = hal.music;
    if (snapshot.revision == displayed_revision) {
        hal.UNLOCKLV();
        return;
    }
    const bool animate = displayed_revision != UINT32_MAX && snapshot.playing &&
                         snapshot.lyric[0] && current_fade.text[0] &&
                         strcmp(current_fade.text, "♪  暂无同步歌词") != 0;
    displayed_revision = snapshot.revision;

    lv_label_set_text(music_title, snapshot.title[0] ? snapshot.title : "等待歌曲…");
    lv_label_set_text(music_artist, snapshot.artist[0] ? snapshot.artist : "网易云音乐  ·  连接待命");
    updateLyric(music_lyric, current_fade,
                snapshot.lyric[0] ? snapshot.lyric :
                snapshot.next_lyric[0] ? "" : "♪  暂无同步歌词", animate);
    updateLyric(music_next_lyric, next_fade, snapshot.next_lyric, animate);
    lv_label_set_text(music_status, snapshot.playing ? "NOW PLAYING" : "PAUSED");
    if (snapshot.duration_ms > 0) {
        uint32_t percent = (snapshot.position_ms * 100U) / snapshot.duration_ms;
        if (percent > 100) percent = 100;
        lv_bar_set_value(music_progress, percent, LV_ANIM_OFF);
    } else {
        lv_bar_set_value(music_progress, 0, LV_ANIM_OFF);
    }
    hal.UNLOCKLV();
}

void AppMusic::setup()
{
    displayed_revision = UINT32_MAX;
    // The CJK font has the Chinese lyric glyphs; Rajdhani supplies clean
    // ASCII punctuation that is missing from its compact glyph subset.
    music_chinese_font = lv_font_chinese_16;
    music_chinese_font.fallback = &font_rajdhani_16;
    music_text_font = lv_font_japanese_16;
    music_text_font.fallback = &music_chinese_font;

    hal.LOCKLV();
    lv_obj_set_style_bg_color(_appScreen, lv_color_hex(0x25303d), 0);
    lv_obj_set_style_bg_opa(_appScreen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_appScreen, 0, 0);
    lv_obj_set_style_pad_all(_appScreen, 0, 0);
    lv_obj_set_style_radius(_appScreen, 0, 0);

    // Shared weather/time scenery, with a veil so lyrics stay readable.
    atmosphereAttach(_appScreen, 0x08131C, 130);
    updateMusicAtmosphere();

    // Compact music header: track metadata takes the top rows; the status
    // remains in the upper-right corner, with no operator/title banner.
    music_shape(_appScreen, 18, 14, 18, 2, 0xe6b75d, LV_OPA_COVER, 1);
    music_status = lv_label_create(_appScreen);
    lv_obj_set_style_text_font(music_status, &font_rajdhani_16, 0);
    lv_obj_set_style_text_color(music_status, lv_color_hex(0x66d8e7), 0);
    lv_obj_set_pos(music_status, 216, 11);
    lv_label_set_text(music_status, "NOW PLAYING");

    music_title = lv_label_create(_appScreen);
    lv_label_set_text(music_title, "");
    lv_obj_set_size(music_title, 284, 19);
    lv_label_set_long_mode(music_title, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(music_title, &music_text_font, 0);
    lv_obj_set_style_text_color(music_title, lv_color_hex(0xf3f2e9), 0);
    lv_obj_set_pos(music_title, 18, 33);

    music_artist = lv_label_create(_appScreen);
    lv_label_set_text(music_artist, "");
    lv_obj_set_size(music_artist, 284, 19);
    lv_label_set_long_mode(music_artist, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(music_artist, &music_text_font, 0);
    lv_obj_set_style_text_color(music_artist, lv_color_hex(0x8fb9c5), 0);
    lv_obj_set_pos(music_artist, 18, 53);

    music_shape(_appScreen, 18, 77, 284, 1, 0xc2b99f, LV_OPA_50, 0);

    current_fade = {};
    current_fade.base_y = 94;
    fade_reports = 0;
    current_fade.outgoing = createLyricLabel(_appScreen, 94, 44, 0xf5e9bd, 2);
    music_lyric = createLyricLabel(_appScreen, 94, 44, 0xf5e9bd, 2);
    music_current_marker = music_shape(_appScreen, 18, 98, 3, 15, 0xe6b75d, LV_OPA_COVER, 1);

    next_fade = {};
    next_fade.base_y = 149;
    next_fade.outgoing = createLyricLabel(_appScreen, 149, 32, 0x91b6c1, 0);
    music_next_lyric = createLyricLabel(_appScreen, 149, 32, 0x91b6c1, 0);
    music_next_marker = music_shape(_appScreen, 19, 156, 4, 4, 0x68cbd9, LV_OPA_COVER, LV_RADIUS_CIRCLE);

    music_progress = lv_bar_create(_appScreen);
    lv_obj_set_size(music_progress, 284, 3);
    lv_obj_set_pos(music_progress, 18, 218);
    lv_obj_set_style_bg_color(music_progress, lv_color_hex(0x4f5e69), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(music_progress, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(music_progress, lv_color_hex(0xe6b75d), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(music_progress, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(music_progress, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(music_progress, 2, LV_PART_INDICATOR);

    hal.UNLOCKLV();
    updateMusicScreen();
}

void AppMusic::loop()
{
    updateMusicAtmosphere();
    updateMusicScreen();
    vTaskDelay(pdMS_TO_TICKS(100));
}

void AppMusic::destroy()
{
    hal.LOCKLV();
    if (music_lyric) lv_anim_del(music_lyric, fadeLyric);
    if (music_next_lyric) lv_anim_del(music_next_lyric, fadeLyric);
    music_title = nullptr;
    music_artist = nullptr;
    music_lyric = nullptr;
    music_next_lyric = nullptr;
    music_status = nullptr;
    music_progress = nullptr;
    music_current_marker = nullptr;
    music_next_marker = nullptr;
    atmosphereDetach();
    hal.UNLOCKLV();
}
