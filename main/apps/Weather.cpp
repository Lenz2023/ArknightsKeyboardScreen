#include "A_Config.h"
#include "Atmosphere.h"
#include "Bgm.h"
#include "ArtStore.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

extern "C" const lv_img_dsc_t weather_0_s;
extern "C" const lv_img_dsc_t weather_1_s;
extern "C" const lv_img_dsc_t weather_2_s;
extern "C" const lv_img_dsc_t weather_3_s;
extern "C" const lv_img_dsc_t weather_4_s;
extern "C" const lv_img_dsc_t weather_5_s;
extern "C" const lv_img_dsc_t weather_6_s;
extern "C" const lv_img_dsc_t weather_7_s;
extern "C" const lv_img_dsc_t weather_8_s;
extern "C" const lv_img_dsc_t weather_9_s;
extern "C" const lv_img_dsc_t weather_10_s;
extern "C" const lv_img_dsc_t weather_11_s;
extern "C" const lv_img_dsc_t weather_12_s;
extern "C" const lv_img_dsc_t weather_13_s;
extern "C" const lv_img_dsc_t weather_14_s;
extern "C" const lv_img_dsc_t weather_15_s;
extern "C" const lv_img_dsc_t weather_16_s;
extern "C" const lv_img_dsc_t weather_17_s;
extern "C" const lv_img_dsc_t weather_18_s;
extern "C" const lv_img_dsc_t weather_19_s;
extern "C" const lv_img_dsc_t weather_20_s;
extern "C" const lv_img_dsc_t weather_21_s;
extern "C" const lv_img_dsc_t weather_22_s;
extern "C" const lv_img_dsc_t weather_23_s;
extern "C" const lv_img_dsc_t weather_24_s;
extern "C" const lv_img_dsc_t weather_25_s;
extern "C" const lv_img_dsc_t weather_26_s;
extern "C" const lv_img_dsc_t weather_27_s;
extern "C" const lv_img_dsc_t weather_28_s;
extern "C" const lv_img_dsc_t weather_29_s;
extern "C" const lv_img_dsc_t weather_30_s;
extern "C" const lv_img_dsc_t weather_31_s;
extern "C" const lv_img_dsc_t weather_32_s;
extern "C" const lv_img_dsc_t weather_33_s;
extern "C" const lv_img_dsc_t weather_34_s;
extern "C" const lv_img_dsc_t weather_35_s;
extern "C" const lv_img_dsc_t weather_36_s;
extern "C" const lv_img_dsc_t weather_37_s;
extern "C" const lv_img_dsc_t weather_38_s;
extern "C" const lv_img_dsc_t weather_99_s;
extern "C" const lv_img_dsc_t weather_0_l;
extern "C" const lv_img_dsc_t weather_1_l;
extern "C" const lv_img_dsc_t weather_2_l;
extern "C" const lv_img_dsc_t weather_3_l;
extern "C" const lv_img_dsc_t weather_4_l;
extern "C" const lv_img_dsc_t weather_5_l;
extern "C" const lv_img_dsc_t weather_6_l;
extern "C" const lv_img_dsc_t weather_7_l;
extern "C" const lv_img_dsc_t weather_8_l;
extern "C" const lv_img_dsc_t weather_9_l;
extern "C" const lv_img_dsc_t weather_10_l;
extern "C" const lv_img_dsc_t weather_11_l;
extern "C" const lv_img_dsc_t weather_12_l;
extern "C" const lv_img_dsc_t weather_13_l;
extern "C" const lv_img_dsc_t weather_14_l;
extern "C" const lv_img_dsc_t weather_15_l;
extern "C" const lv_img_dsc_t weather_16_l;
extern "C" const lv_img_dsc_t weather_17_l;
extern "C" const lv_img_dsc_t weather_18_l;
extern "C" const lv_img_dsc_t weather_19_l;
extern "C" const lv_img_dsc_t weather_20_l;
extern "C" const lv_img_dsc_t weather_21_l;
extern "C" const lv_img_dsc_t weather_22_l;
extern "C" const lv_img_dsc_t weather_23_l;
extern "C" const lv_img_dsc_t weather_24_l;
extern "C" const lv_img_dsc_t weather_25_l;
extern "C" const lv_img_dsc_t weather_26_l;
extern "C" const lv_img_dsc_t weather_27_l;
extern "C" const lv_img_dsc_t weather_28_l;
extern "C" const lv_img_dsc_t weather_29_l;
extern "C" const lv_img_dsc_t weather_30_l;
extern "C" const lv_img_dsc_t weather_31_l;
extern "C" const lv_img_dsc_t weather_32_l;
extern "C" const lv_img_dsc_t weather_33_l;
extern "C" const lv_img_dsc_t weather_34_l;
extern "C" const lv_img_dsc_t weather_35_l;
extern "C" const lv_img_dsc_t weather_36_l;
extern "C" const lv_img_dsc_t weather_37_l;
extern "C" const lv_img_dsc_t weather_38_l;
extern "C" const lv_img_dsc_t weather_99_l;
const lv_img_dsc_t *weatherImages_s[] =
    {
        &weather_0_s,
        &weather_1_s,
        &weather_2_s,
        &weather_3_s,
        &weather_4_s,
        &weather_5_s,
        &weather_6_s,
        &weather_7_s,
        &weather_8_s,
        &weather_9_s,
        &weather_10_s,
        &weather_11_s,
        &weather_12_s,
        &weather_13_s,
        &weather_14_s,
        &weather_15_s,
        &weather_16_s,
        &weather_17_s,
        &weather_18_s,
        &weather_19_s,
        &weather_20_s,
        &weather_21_s,
        &weather_22_s,
        &weather_23_s,
        &weather_24_s,
        &weather_25_s,
        &weather_26_s,
        &weather_27_s,
        &weather_28_s,
        &weather_29_s,
        &weather_30_s,
        &weather_31_s,
        &weather_32_s,
        &weather_33_s,
        &weather_34_s,
        &weather_35_s,
        &weather_36_s,
        &weather_37_s,
        &weather_38_s,
        &weather_99_s,
};
const lv_img_dsc_t *weatherImages_l[] =
    {
        &weather_0_l,
        &weather_1_l,
        &weather_2_l,
        &weather_3_l,
        &weather_4_l,
        &weather_5_l,
        &weather_6_l,
        &weather_7_l,
        &weather_8_l,
        &weather_9_l,
        &weather_10_l,
        &weather_11_l,
        &weather_12_l,
        &weather_13_l,
        &weather_14_l,
        &weather_15_l,
        &weather_16_l,
        &weather_17_l,
        &weather_18_l,
        &weather_19_l,
        &weather_20_l,
        &weather_21_l,
        &weather_22_l,
        &weather_23_l,
        &weather_24_l,
        &weather_25_l,
        &weather_26_l,
        &weather_27_l,
        &weather_28_l,
        &weather_29_l,
        &weather_30_l,
        &weather_31_l,
        &weather_32_l,
        &weather_33_l,
        &weather_34_l,
        &weather_35_l,
        &weather_36_l,
        &weather_37_l,
        &weather_38_l,
        &weather_99_l,
};
const char *weatherNames[] = {
    "晴",
    "晴",
    "晴",
    "晴",
    "多云",
    "晴间多云",
    "晴间多云",
    "大部多云",
    "大部多云",
    "阴",
    "阵雨",
    "雷阵雨",
    "冰雹",
    "小雨",
    "中雨",
    "大雨",
    "暴雨",
    "大暴雨",
    "特大暴雨",
    "冻雨",
    "雨夹雪",
    "阵雪",
    "小雪",
    "中雪",
    "大雪",
    "暴雪",
    "浮尘",
    "扬沙",
    "沙尘暴",
    "强沙尘暴",
    "雾",
    "霾",
    "风",
    "大风",
    "飓风",
    "热带风暴",
    "龙卷风",
    "冷",
    "热",
    "未知",
};
extern "C" const lv_font_t font_rajdhani_16;
// Home scenery from the PRTS "首页场景一览" set, see scene_home_bg.c.
extern "C" const lv_img_dsc_t scene_clear_bg;
extern "C" const lv_img_dsc_t scene_cloudy_bg;
extern "C" const lv_img_dsc_t scene_rain_bg;
extern "C" const lv_img_dsc_t scene_snow_bg;
extern "C" const lv_img_dsc_t scene_dusk_bg;
extern "C" const lv_img_dsc_t scene_night_bg;

struct WeatherData
{
    char location[32];
    uint8_t weatherCode_today;
    uint8_t weatherCode_tomorrow;
    uint8_t weatherCode_tomorrow_1;
    int8_t temperature_now;
    int8_t hour_temperature[6];
    uint8_t hour_code[6];
    char hour_time[6][6];
    uint16_t windKmhPlusOne;
};
static WeatherData weatherData;
static portMUX_TYPE weatherDataMux = portMUX_INITIALIZER_UNLOCKED;
static char weatherHomeLocationBuffer[sizeof(weatherData.location)] = {};
static volatile uint32_t weatherHomeDataRevision = 0;
extern "C" int weatherHomeTemperature() {
    int value;
    portENTER_CRITICAL(&weatherDataMux);
    value = weatherData.temperature_now;
    portEXIT_CRITICAL(&weatherDataMux);
    return value;
}
extern "C" int weatherHomeCode() {
    int value;
    portENTER_CRITICAL(&weatherDataMux);
    value = weatherData.weatherCode_today;
    portEXIT_CRITICAL(&weatherDataMux);
    return value;
}
extern "C" const char *weatherHomeLocation() {
    portENTER_CRITICAL(&weatherDataMux);
    memcpy(weatherHomeLocationBuffer, weatherData.location, sizeof(weatherHomeLocationBuffer));
    portEXIT_CRITICAL(&weatherDataMux);
    return weatherHomeLocationBuffer[0] ? weatherHomeLocationBuffer : "北京";
}
extern "C" uint32_t weatherHomeRevision() { return weatherHomeDataRevision; }

// --- Shared weather / time backdrop (see Atmosphere.h) ----------------------
// Every page except the home and music pages used to invent its own backdrop:
// a flat blue, plain white, or an one-off interior scene. These layers give
// them the same scenery the home page shows, so the device reads as one shell.
namespace {
lv_obj_t *atmosphereBackdrop = nullptr;
lv_obj_t *atmosphereTint = nullptr;
const lv_img_dsc_t *atmosphereImage = nullptr;
int atmosphereBand = -1;

// Uploaded pictures replace one weather/time slot at a time. Only the slot that is
// on screen is kept in PSRAM, so six custom backgrounds cost one buffer.
lv_img_dsc_t atmosphereCustomDesc;
uint8_t *atmosphereCustomPixels = nullptr;
bool atmosphereCustomActive = false;
int atmosphereCustomSlot = -1;
// 记录上次尝试过的槽位：没有上传图时也要记住，避免每帧反复读 LittleFS。
int atmosphereProbedSlot = -1;
uint32_t atmosphereCustomRevision = 0;

void atmosphereDropCustom() {
    if (atmosphereCustomPixels) {
        free(atmosphereCustomPixels);
        atmosphereCustomPixels = nullptr;
    }
    atmosphereCustomActive = false;
    atmosphereCustomSlot = -1;
    atmosphereProbedSlot = -1;
}

// Loads the uploaded picture for `slot` when there is one; otherwise the built-in
// scenery for that condition stays in use.
void atmosphereUseSlot(int slot) {
    if (atmosphereCustomActive && atmosphereCustomSlot == slot) return;
    if (!atmosphereCustomActive && atmosphereProbedSlot == slot) return;
    atmosphereDropCustom();
    atmosphereProbedSlot = slot;
    uint8_t *pixels = artstore::loadBackground(slot);
    if (!pixels) return;
    atmosphereCustomPixels = pixels;
    memset(&atmosphereCustomDesc, 0, sizeof(atmosphereCustomDesc));
    atmosphereCustomDesc.header.cf = LV_IMG_CF_TRUE_COLOR;
    atmosphereCustomDesc.header.w = artstore::backgroundWidth;
    atmosphereCustomDesc.header.h = artstore::backgroundHeight;
    atmosphereCustomDesc.data_size = artstore::backgroundBytes;
    atmosphereCustomDesc.data = pixels;
    atmosphereCustomSlot = slot;
    atmosphereCustomActive = true;
    ESP_LOGI("Atmosphere", "custom background '%s'", artstore::backgroundSlotName(slot));
}

// Same buckets the home page and the music page use, kept in one place so the
// three implementations cannot drift apart.
int atmosphereSceneForCode(int code) {
    if (code == 0 || code == 38) return 0;
    if ((code >= 10 && code <= 20) || (code >= 33 && code <= 36)) return 2;
    if (code >= 21 && code <= 25) return 3;
    return 1;
}

// Night and dusk always win over the weather, the way the game cycles its own
// home scenes: only the daytime slot is picked by the sky right now.
const lv_img_dsc_t *atmosphereImageFor(int scene, int band) {
    if (atmosphereCustomActive) return &atmosphereCustomDesc;
    if (band == 2) return &scene_night_bg;
    if (band == 1) return &scene_dusk_bg;
    switch (scene) {
    case 0: return &scene_clear_bg;
    case 2: return &scene_rain_bg;
    case 3: return &scene_snow_bg;
    default: return &scene_cloudy_bg;
    }
}

int atmosphereBandForHour(int hour) {
    if (hour >= 6 && hour < 17) return 0;                                  // daylight
    if ((hour >= 17 && hour < 20) || (hour >= 5 && hour < 6)) return 1;    // dusk
    return 2;                                                             // night
}
}  // namespace

extern "C" void atmosphereRefresh(void) {
    if (!atmosphereBackdrop || !atmosphereTint) return;

    // Picking up a freshly uploaded picture does not need a page switch.
    if (artstore::revision() != atmosphereCustomRevision) {
        atmosphereCustomRevision = artstore::revision();
        atmosphereDropCustom();
        atmosphereImage = nullptr;
    }

    const int scene = atmosphereSceneForCode(weatherHomeCode());
    const int band = atmosphereBandForHour(hal.datetime.hour);
    atmosphereUseSlot(artstore::backgroundSlotFor(weatherHomeCode(), hal.datetime.hour));
    // 背景音乐跟着场景走：同一个 slot 表决定画面和曲子
    bgm::setSlot(artstore::backgroundSlotFor(weatherHomeCode(), hal.datetime.hour));

    const lv_img_dsc_t *image = atmosphereImageFor(scene, band);
    if (image != atmosphereImage) {
        lv_img_set_src(atmosphereBackdrop, image);
        atmosphereImage = image;
    }

    if (band != atmosphereBand) {
        lv_obj_set_style_bg_color(atmosphereTint,
                                  lv_color_hex(band == 0 ? 0x8494b0 : band == 1 ? 0xa77a78 : 0x101831), 0);
        lv_obj_set_style_bg_opa(atmosphereTint,
                                band == 0 ? LV_OPA_TRANSP : band == 1 ? LV_OPA_30 : LV_OPA_40, 0);
        atmosphereBand = band;
    }
}

extern "C" void atmosphereAttach(lv_obj_t *screen, uint32_t veilColor, uint8_t veilOpa) {
    atmosphereImage = nullptr;
    atmosphereBand = -1;
    if (artstore::revision() != atmosphereCustomRevision) {
        atmosphereCustomRevision = artstore::revision();
        atmosphereDropCustom();
    }

    // Overscan the scenery by the same amount the home page does: the panel's
    // rim is not perfectly aligned with the bezel, so 320x240 is scaled up.
    atmosphereBackdrop = lv_img_create(screen);
    lv_obj_add_flag(atmosphereBackdrop, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_pos(atmosphereBackdrop, -5, -4);
    lv_img_set_src(atmosphereBackdrop, &scene_clear_bg);
    lv_img_set_zoom(atmosphereBackdrop, 264);

    atmosphereTint = lv_obj_create(screen);
    lv_obj_remove_style_all(atmosphereTint);
    lv_obj_add_flag(atmosphereTint, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_pos(atmosphereTint, 0, 0);
    lv_obj_set_size(atmosphereTint, 320, 240);
    lv_obj_clear_flag(atmosphereTint, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    if (veilOpa) {
        lv_obj_t *veil = lv_obj_create(screen);
        lv_obj_remove_style_all(veil);
        lv_obj_add_flag(veil, LV_OBJ_FLAG_IGNORE_LAYOUT);
        lv_obj_set_pos(veil, 0, 0);
        lv_obj_set_size(veil, 320, 240);
        lv_obj_set_style_bg_color(veil, lv_color_hex(veilColor), 0);
        lv_obj_set_style_bg_opa(veil, (lv_opa_t)veilOpa, 0);
        lv_obj_clear_flag(veil, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    }

    atmosphereRefresh();
}

extern "C" const uint8_t *atmosphereCurrentPixels(int *width, int *height) {
    if (width) *width = artstore::backgroundWidth;
    if (height) *height = artstore::backgroundHeight;
    if (atmosphereCustomActive && atmosphereCustomPixels) return atmosphereCustomPixels;
    if (!atmosphereImage) return nullptr;
    return static_cast<const uint8_t *>(atmosphereImage->data);
}

extern "C" const char *atmosphereSceneLabel(void) {
    if (atmosphereImage == &scene_night_bg) return "NIGHT";
    if (atmosphereImage == &scene_dusk_bg) return "DUSK";
    if (atmosphereImage == &scene_rain_bg) return "RAIN";
    if (atmosphereImage == &scene_snow_bg) return "SNOW";
    if (atmosphereImage == &scene_cloudy_bg) return "CLOUDY";
    return "CLEAR";
}

extern "C" void atmosphereDetach(void) {
    atmosphereBackdrop = nullptr;
    atmosphereTint = nullptr;
    atmosphereImage = nullptr;
    atmosphereBand = -1;
}

extern "C" const char *weatherHomeCondition() {
    const unsigned code = (unsigned)weatherHomeCode();
    return code <= 38 ? weatherNames[code] : weatherNames[39];
}
static time_t weatherLastSuccess = 0;
static bool weatherLastSuccessEstimated = false;
bool loadWeather = false;
static constexpr time_t WEATHER_MIN_VALID_EPOCH = 1672531200;

static bool weatherFileExists(const char *path)
{
    char fullPath[64];
    snprintf(fullPath, sizeof(fullPath), "/littlefs%s", path);
    struct stat info = {};
    return stat(fullPath, &info) == 0;
}

static time_t weatherRtcEpoch()
{
    DS1302_DateTime rtcTime = {};
    if (!DS1302_getDateTime(&hal.rtc, &rtcTime) || rtcTime.year < 2023)
        return 0;

    // The DS1302 stores local time (NTP UTC plus the configured offset).
    // Convert the calendar fields directly so this does not depend on libc TZ.
    int year = rtcTime.year;
    const unsigned month = rtcTime.month;
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = (unsigned)(year - era * 400);
    const unsigned shiftedMonth = month > 2 ? month - 3 : month + 9;
    const unsigned dayOfYear = (153 * shiftedMonth + 2) / 5 + rtcTime.dayMonth - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    const int64_t daysSinceEpoch = (int64_t)era * 146097 + dayOfEra - 719468;
    const int64_t localSeconds = daysSinceEpoch * 86400 + rtcTime.hour * 3600 + rtcTime.minute * 60 + rtcTime.second;
    const int64_t utcSeconds = localSeconds - i18n::getNTPOffset();
    return utcSeconds >= WEATHER_MIN_VALID_EPOCH ? (time_t)utcSeconds : 0;
}

static time_t currentWeatherEpoch()
{
    const time_t rtcEpoch = weatherRtcEpoch();
    if (rtcEpoch >= WEATHER_MIN_VALID_EPOCH)
        return rtcEpoch;

    const time_t systemEpoch = time(nullptr);
    return systemEpoch >= WEATHER_MIN_VALID_EPOCH ? systemEpoch : 0;
}

static void saveWeatherTimestamp()
{
    if (weatherLastSuccess < WEATHER_MIN_VALID_EPOCH)
        return;

    File timeFile = LittleFS.open("/.weather_time", "w");
    if (timeFile) {
        timeFile.write((const uint8_t *)&weatherLastSuccess, sizeof(weatherLastSuccess));
        timeFile.close();
    }

    if (weatherLastSuccessEstimated) {
        const uint8_t estimated = 1;
        File marker = LittleFS.open("/.weather_time_estimated", "w");
        if (marker) {
            marker.write(&estimated, sizeof(estimated));
            marker.close();
        }
    } else if (weatherFileExists("/.weather_time_estimated")) {
        LittleFS.remove("/.weather_time_estimated");
    }
}

static void seedEstimatedWeatherTimestamp()
{
    if (!loadWeather || weatherLastSuccess >= WEATHER_MIN_VALID_EPOCH)
        return;

    const time_t now = currentWeatherEpoch();
    if (now < WEATHER_MIN_VALID_EPOCH)
        return;

    weatherLastSuccess = now;
    weatherLastSuccessEstimated = true;
    saveWeatherTimestamp();
}

static bool weatherCacheLoaded = false;
static void loadWeatherCache()
{
    if (weatherCacheLoaded)
        return;
    weatherCacheLoaded = true;

    WeatherData cached = {};
    File f = LittleFS.open("/.weather_hourly_v1", "r");
    if (f) {
        if (f.readBytes((char *)&cached, sizeof(cached)) == sizeof(cached)) {
            portENTER_CRITICAL(&weatherDataMux);
            weatherData = cached;
            ++weatherHomeDataRevision;
            portEXIT_CRITICAL(&weatherDataMux);
            loadWeather = true;
        }
        f.close();
    }

    if (weatherFileExists("/.weather_time")) {
        File timeFile = LittleFS.open("/.weather_time", "r");
        if (timeFile) {
            if (timeFile.size() == sizeof(weatherLastSuccess))
                timeFile.read((uint8_t *)&weatherLastSuccess, sizeof(weatherLastSuccess));
            timeFile.close();
        }
    }
    if (weatherLastSuccess < WEATHER_MIN_VALID_EPOCH)
        weatherLastSuccess = 0;

    weatherLastSuccessEstimated = false;
    if (weatherFileExists("/.weather_time_estimated")) {
        File estimatedMarker = LittleFS.open("/.weather_time_estimated", "r");
        if (estimatedMarker) {
            uint8_t estimated = 0;
            if (estimatedMarker.size() == sizeof(estimated))
                estimatedMarker.read(&estimated, sizeof(estimated));
            weatherLastSuccessEstimated = estimated == 1;
            estimatedMarker.close();
        }
    }
    seedEstimatedWeatherTimestamp();
}

lv_obj_t *obj_temperature;
lv_obj_t *obj_weatherIcon_big;
lv_obj_t *lbl_weather_big;
lv_obj_t *lbl_location;
lv_obj_t *obj_weatherIcon_tomorrow;
lv_obj_t *lbl_date_tomorrow;
lv_obj_t *lbl_weather_tomorrow;
lv_obj_t *obj_weatherIcon_tomorrow_1;
lv_obj_t *lbl_date_tomorrow_1;
lv_obj_t *lbl_weather_tomorrow_1;
static lv_obj_t *hour_icons[6];
static lv_obj_t *hour_labels[6];
static lv_obj_t *hour_temps[6];
static lv_obj_t *hour_interval;
static lv_obj_t *wind_label;
static lv_obj_t *lbl_weather_age;
static void updateWeatherAge() {
    if (weatherLastSuccess < WEATHER_MIN_VALID_EPOCH) {
        lv_label_set_text(lbl_weather_age, "等待更新");
        return;
    }
    const time_t now = currentWeatherEpoch();
    if (now < WEATHER_MIN_VALID_EPOCH || now < weatherLastSuccess) {
        lv_label_set_text(lbl_weather_age, "更新：等待校时");
        return;
    }
    const time_t ageSeconds = now - weatherLastSuccess;
    char ageText[32];
    if (ageSeconds < 60) {
        snprintf(ageText, sizeof(ageText), "刚刚");
    } else if (ageSeconds < 3600) {
        const unsigned long long ageMinutes = (unsigned long long)(ageSeconds / 60);
        snprintf(ageText, sizeof(ageText), "%llu分钟前", ageMinutes);
    } else {
        const unsigned long long ageHours = (unsigned long long)(ageSeconds / 3600);
        snprintf(ageText, sizeof(ageText), "%llu小时前", ageHours);
    }
    lv_label_set_text_fmt(lbl_weather_age, weatherLastSuccessEstimated ? "更新：约%s" : "更新：%s", ageText);
}
void updateWeather()
{
    WeatherData snapshot;
    portENTER_CRITICAL(&weatherDataMux);
    snapshot = weatherData;
    portEXIT_CRITICAL(&weatherDataMux);
    const unsigned todayCode = snapshot.weatherCode_today <= 38 ? snapshot.weatherCode_today : 39;
    // The scenery is owned by the shared atmosphere layer so this page tracks
    // the same weather scene and time tint as every other page.
    atmosphereRefresh();
    const unsigned tomorrowCode = snapshot.weatherCode_tomorrow <= 38 ? snapshot.weatherCode_tomorrow : 39;
    const unsigned tomorrow2Code = snapshot.weatherCode_tomorrow_1 <= 38 ? snapshot.weatherCode_tomorrow_1 : 39;
    lv_label_set_text_fmt(obj_temperature, "%d°", snapshot.temperature_now);
    lv_img_set_src(obj_weatherIcon_big, weatherImages_l[todayCode]);
    lv_label_set_text(lbl_weather_big, weatherNames[todayCode]);
    if (snapshot.windKmhPlusOne)
        lv_label_set_text_fmt(wind_label, "风速 %u km/h", (unsigned)snapshot.windKmhPlusOne - 1);
    else
        lv_label_set_text(wind_label, "风速 --");
    lv_label_set_text(lbl_location, snapshot.location);
    lv_img_set_src(obj_weatherIcon_tomorrow, weatherImages_s[tomorrowCode]);
    lv_label_set_text(lbl_weather_tomorrow, weatherNames[tomorrowCode]);
    lv_img_set_src(obj_weatherIcon_tomorrow_1, weatherImages_s[tomorrow2Code]);
    lv_label_set_text(lbl_weather_tomorrow_1, weatherNames[tomorrow2Code]);
    bool hasHourly = false;
    for (int i = 0; i < 6; ++i) {
        const unsigned rawCode = snapshot.hour_code[i] & 0x7f;
        const unsigned code = rawCode <= 38 ? rawCode : 39;
        const bool valid = snapshot.hour_time[i][0] != 0;
        hasHourly |= valid;
        lv_label_set_text(hour_labels[i], valid ? snapshot.hour_time[i] : "--:--");
        if (valid) {
            lv_label_set_text_fmt(hour_temps[i], "%d°", snapshot.hour_temperature[i]);
            lv_img_set_src(hour_icons[i], weatherImages_s[code]);
            lv_obj_clear_flag(hour_icons[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_label_set_text(hour_temps[i], "--");
            lv_obj_add_flag(hour_icons[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    lv_label_set_text(hour_interval, !hasHourly ? "暂无小时数据" :
                      (snapshot.hour_code[0] & 0x80) ? "Open-Meteo" : "每4小时");
}
#include <HTTPClient.h>
#include <cJSON.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>

static String city;

static cJSON *getForecastJson(const String &url)
{
    // The display RTC is maintained separately from the system clock used by TLS.
    if (time(nullptr) < WEATHER_MIN_VALID_EPOCH) {
        const time_t rtc = weatherRtcEpoch();
        if (rtc >= WEATHER_MIN_VALID_EPOCH) {
            const timeval clock = {rtc, 0};
            settimeofday(&clock, nullptr);
        }
    }
    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 5000;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return nullptr;
    cJSON *result = nullptr;
    const esp_err_t openErr = esp_http_client_open(client, 0);
    if (openErr != ESP_OK) {
        ESP_LOGW("WeatherOM", "open failed: %d (%s)", (int)openErr, url.c_str());
    }
    const int64_t headerLen = openErr == ESP_OK ? esp_http_client_fetch_headers(client) : -1;
    const int status = openErr == ESP_OK ? esp_http_client_get_status_code(client) : -1;
    if (openErr == ESP_OK && (headerLen < 0 || status != 200)) {
        ESP_LOGW("WeatherOM", "bad response: status=%d len=%lld %s", status, (long long)headerLen, url.c_str());
    }
    if (openErr == ESP_OK && headerLen >= 0 && status == 200) {
        constexpr size_t capacity = 8192;
        char *body = static_cast<char *>(malloc(capacity));
        if (body) {
            const int received = esp_http_client_read_response(client, body, capacity - 1);
            const bool complete = esp_http_client_is_complete_data_received(client);
            if (received > 0 && complete) {
                body[received] = 0;
                result = cJSON_Parse(body);
            }
            free(body);
        }
    }
    esp_http_client_cleanup(client);
    return result;
}

static uint8_t forecastWeatherCode(int code, bool night)
{
    switch (code) {
        case 0: case 1: return night ? 1 : 0;
        case 2: return night ? 6 : 5;
        case 3: return 9;
        case 45: case 48: return 30;
        case 51: case 53: case 55: case 61: return 13;
        case 63: return 14;
        case 65: return 15;
        case 56: case 57: case 66: case 67: return 19;
        case 71: case 77: return 22;
        case 73: return 23;
        case 75: return 24;
        case 80: case 81: case 82: return 10;
        case 85: case 86: return 21;
        case 95: case 96: case 99: return 11;
        default: return 39;
    }
}

static bool fetchOpenMeteo(WeatherData &data, const String &country)
{
    if (!data.location[0]) return false;
    static String resolvedLocation;
    static double latitude, longitude;
    const String locationKey = String(data.location) + "," + country;
    if (resolvedLocation != locationKey) {
        String query = "https://geocoding-api.open-meteo.com/v1/search?count=1&language=zh&name=";
        for (const unsigned char *p = reinterpret_cast<const unsigned char *>(data.location); *p; ++p) {
            char escaped[4];
            snprintf(escaped, sizeof(escaped), "%%%02X", (unsigned)*p);
            query += escaped;
        }
        if (country.length() == 2) query += "&countryCode=" + country;
        cJSON *root = getForecastJson(query);
        const cJSON *place = cJSON_GetArrayItem(cJSON_GetObjectItem(root, "results"), 0);
        const cJSON *lat = cJSON_GetObjectItem(place, "latitude");
        const cJSON *lon = cJSON_GetObjectItem(place, "longitude");
        const bool found = cJSON_IsNumber(lat) && cJSON_IsNumber(lon);
        if (found) {
            latitude = lat->valuedouble;
            longitude = lon->valuedouble;
            resolvedLocation = locationKey;
        }
        if (!found) {
            ESP_LOGW("WeatherOM", "geocode failed for '%s'", data.location);
        }
        cJSON_Delete(root);
        if (!found) return false;
    }

    const String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(latitude, 5) +
        "&longitude=" + String(longitude, 5) +
        "&timezone=auto&forecast_hours=24&forecast_days=3"
        "&current=temperature_2m,weather_code,is_day,wind_speed_10m"
        "&hourly=temperature_2m,weather_code,is_day&daily=weather_code";
    cJSON *root = getForecastJson(url);
    const cJSON *hourly = cJSON_GetObjectItem(root, "hourly");
    const cJSON *times = cJSON_GetObjectItem(hourly, "time");
    const cJSON *temperatures = cJSON_GetObjectItem(hourly, "temperature_2m");
    const cJSON *codes = cJSON_GetObjectItem(hourly, "weather_code");
    const cJSON *daylight = cJSON_GetObjectItem(hourly, "is_day");
    bool received = false;
    for (int slot = 0; slot < 6; ++slot) {
        const int index = slot * 4;
        const cJSON *time = cJSON_GetArrayItem(times, index);
        const cJSON *temp = cJSON_GetArrayItem(temperatures, index);
        const cJSON *code = cJSON_GetArrayItem(codes, index);
        if (!cJSON_IsString(time) || !cJSON_IsNumber(temp) || !cJSON_IsNumber(code))
            continue;
        const char *clock = strchr(time->valuestring, 'T');
        const double degrees = temp->valuedouble;
        if (!clock || strspn(clock + 1, "0123456789:") < 5 || clock[3] != ':' ||
            degrees < -100 || degrees > 70)
            continue;
        snprintf(data.hour_time[slot], sizeof(data.hour_time[slot]), "%.5s", clock + 1);
        data.hour_temperature[slot] = static_cast<int8_t>(degrees + (degrees < 0 ? -0.5 : 0.5));
        const cJSON *day = cJSON_GetArrayItem(daylight, index);
        data.hour_code[slot] = forecastWeatherCode(code->valueint, cJSON_IsNumber(day) && day->valueint == 0);
        received = true;
    }
    bool currentOk = false;
    const cJSON *current = cJSON_GetObjectItem(root, "current");
    const cJSON *nowTemp = cJSON_GetObjectItem(current, "temperature_2m");
    const cJSON *nowCode = cJSON_GetObjectItem(current, "weather_code");
    const cJSON *nowDay = cJSON_GetObjectItem(current, "is_day");
    if (cJSON_IsNumber(nowTemp) && cJSON_IsNumber(nowCode) &&
        nowTemp->valuedouble >= -100 && nowTemp->valuedouble <= 70) {
        const double degrees = nowTemp->valuedouble;
        data.temperature_now = static_cast<int8_t>(degrees < 0 ? -(int)(-degrees + 0.5) : (int)(degrees + 0.5));
        data.weatherCode_today = forecastWeatherCode(nowCode->valueint, cJSON_IsNumber(nowDay) && nowDay->valueint == 0);
        const cJSON *wind = cJSON_GetObjectItem(current, "wind_speed_10m");
        if (cJSON_IsNumber(wind) && wind->valuedouble >= 0 && wind->valuedouble <= 500)
            data.windKmhPlusOne = static_cast<uint16_t>(wind->valuedouble + 0.5) + 1;
        currentOk = true;
    }
    const cJSON *daily = cJSON_GetObjectItem(root, "daily");
    const cJSON *dailyCodes = cJSON_GetObjectItem(daily, "weather_code");
    if (cJSON_IsArray(dailyCodes) && cJSON_GetArraySize(dailyCodes) >= 3) {
        data.weatherCode_tomorrow = forecastWeatherCode(cJSON_GetArrayItem(dailyCodes, 1)->valueint, false);
        data.weatherCode_tomorrow_1 = forecastWeatherCode(cJSON_GetArrayItem(dailyCodes, 2)->valueint, false);
    }
    ESP_LOGI("WeatherOM", "'%s' %.4f,%.4f temp=%d code=%d hourly=%d current=%d",
             data.location, latitude, longitude,
             (int)data.temperature_now, (int)data.weatherCode_today, (int)received, (int)currentOk);
    cJSON_Delete(root);
    if (received) data.hour_code[0] |= 0x80;
    return received || currentOk;
}

bool getWeather()
{
    WeatherData candidate;
    portENTER_CRITICAL(&weatherDataMux);
    candidate = weatherData;
    portEXIT_CRITICAL(&weatherDataMux);
    const WeatherData previous = candidate;
    const String country = "CN";

    // Open-Meteo only: the request below carries current conditions, the 24h
    // strip and the two-day outlook, and it needs no API key or secret.
    if (city.length())
        snprintf(candidate.location, sizeof(candidate.location), "%s", city.c_str());
    const bool updated = fetchOpenMeteo(candidate, country);
    if (!updated) {
        if (strcmp(candidate.location, previous.location) == 0) {
            memcpy(candidate.hour_time, previous.hour_time, sizeof(candidate.hour_time));
            memcpy(candidate.hour_code, previous.hour_code, sizeof(candidate.hour_code));
            memcpy(candidate.hour_temperature, previous.hour_temperature, sizeof(candidate.hour_temperature));
        }
        return false;
    }
    portENTER_CRITICAL(&weatherDataMux);
    weatherData = candidate;
    ++weatherHomeDataRevision;
    portEXIT_CRITICAL(&weatherDataMux);
    return true;
}

static uint32_t last_millis = 0;
static uint32_t last_age_refresh = 0;
uint8_t weather_update_cnt = 0;
static volatile bool homeWeatherServiceActive = false;
static volatile bool homeWeatherFetchInProgress = false;
static TaskHandle_t homeWeatherServiceTask = nullptr;

static void saveWeatherCache()
{
    WeatherData snapshot;
    portENTER_CRITICAL(&weatherDataMux);
    snapshot = weatherData;
    portEXIT_CRITICAL(&weatherDataMux);
    File f = LittleFS.open("/.weather_hourly_v1", "w");
    if (f) {
        f.write((const uint8_t *)&snapshot, sizeof(snapshot));
        f.close();
    }
}

static void homeWeatherServiceTaskEntry(void *)
{
    uint16_t networkIndex = WiFiMgr.count() ? WiFiMgr.count() - 1 : 0;
    uint32_t wifiAttemptStarted = 0;
    uint32_t lastWifiRetry = 0;
    uint32_t lastWeatherAttempt = 0;
    uint32_t weatherRefreshInterval = 2UL * 60 * 1000;
    uint32_t lastNtpAttempt = 0;
    uint32_t lastNtpSuccess = 0;
    bool waitingForWiFi = false;

    for (;;) {
        if (!homeWeatherServiceActive) {
            waitingForWiFi = false;
            lastWifiRetry = 0;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        loadWeatherCache();
        if (WiFi.status() != WL_CONNECTED) {
            const uint32_t now = millis();
            if (waitingForWiFi && now - wifiAttemptStarted >= 15000) {
                WiFi.disconnect(false, false);
                waitingForWiFi = false;
                const uint16_t savedCount = WiFiMgr.count();
                if (savedCount > 0)
                    networkIndex = (networkIndex + 1) % savedCount;
            }
            if (!waitingForWiFi && (lastWifiRetry == 0 || now - lastWifiRetry >= 5000)) {
                bool started = false;
                const uint16_t savedCount = WiFiMgr.count();
                if (savedCount > 0) {
                    if (networkIndex >= savedCount)
                        networkIndex = savedCount - 1;
                    started = WiFiMgr.beginSaved(networkIndex);
                } else {
                    WiFi.mode(WIFI_STA);
                    WiFi.begin();
                    started = true;
                }
                waitingForWiFi = started;
                wifiAttemptStarted = now;
                lastWifiRetry = now;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        waitingForWiFi = false;
        const uint32_t now = millis();
        // Keep the screen reachable by the PC music bridge while on the home page.
        if (!hal.server_started)
            hal.start_webserver();
        const bool rtcInvalid = hal.datetime.year < 2023;
        if ((rtcInvalid || lastNtpSuccess == 0 || now - lastNtpSuccess >= 6UL * 60 * 60 * 1000) &&
            (lastNtpAttempt == 0 || now - lastNtpAttempt >= 60000)) {
            lastNtpAttempt = now;
            if (hal.NTPSync()) {
                lastNtpSuccess = now;
                hal.time_sync = true;
                hal.getTime();
            }
        }

        // Open-Meteo needs no API key, only a city to geocode.
        const bool hasWeatherSettings = app_settings_save.weather_city[0] != 0;
        const bool refreshDue = lastWeatherAttempt == 0 || now - lastWeatherAttempt >= weatherRefreshInterval;
        if (hal.weather_enable && hasWeatherSettings && refreshDue && !homeWeatherFetchInProgress) {
            lastWeatherAttempt = now;
            homeWeatherFetchInProgress = true;
            city = app_settings_save.weather_city;
            if (getWeather()) {
                const time_t weatherTime = currentWeatherEpoch();
                if (weatherTime >= WEATHER_MIN_VALID_EPOCH) {
                    weatherLastSuccess = weatherTime;
                    weatherLastSuccessEstimated = false;
                    saveWeatherTimestamp();
                }
                saveWeatherCache();
                loadWeather = true;
                weatherRefreshInterval = 10UL * 60 * 1000;
                last_millis = millis();
            }
            homeWeatherFetchInProgress = false;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void weatherHomeServiceSetActive(bool active)
{
    homeWeatherServiceActive = active;
    if (active && homeWeatherServiceTask == nullptr) {
        xTaskCreatePinnedToCore(homeWeatherServiceTaskEntry, "home_weather", 8192, nullptr, 1,
                                &homeWeatherServiceTask, 0);
    }
}

static uint32_t last_weather_page_revision = 0;

void AppWeather::setup()
{
    hal.LOCKLV();
    lv_obj_set_style_bg_color(_appScreen, lv_color_hex(0x182f40), 0);
    lv_obj_set_style_bg_opa(_appScreen, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(_appScreen, 0, 0);
    lv_obj_clear_flag(_appScreen, LV_OBJ_FLAG_SCROLLABLE);
    atmosphereAttach(_appScreen, 0, 0);

    constexpr uint32_t ink = 0xf3f2e9, muted = 0x9ed5df, gold = 0xe6b75d;
    const auto label = [&](int x, int y, int width, const char *text, uint32_t color,
                           bool center = false, const lv_font_t *font = &lv_font_chinese_16) {
        lv_obj_t *obj = lv_label_create(_appScreen);
        lv_obj_set_pos(obj, x, y);
        lv_obj_set_width(obj, width);
        lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(obj, font, 0);
        lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
        lv_obj_set_style_text_align(obj, center ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, 0);
        lv_label_set_text(obj, text);
        return obj;
    };
    const auto icon = [&](int x, int y, const lv_img_dsc_t *source) {
        lv_obj_t *obj = lv_img_create(_appScreen);
        lv_obj_set_pos(obj, x, y);
        lv_img_set_src(obj, source);
        return obj;
    };
    const auto rule = [&](int x, int y, int width, uint32_t color) {
        lv_obj_t *obj = lv_obj_create(_appScreen);
        lv_obj_remove_style_all(obj);
        lv_obj_set_pos(obj, x, y);
        lv_obj_set_size(obj, width, 2);
        lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    };

    rule(18, 13, 18, gold);
    lbl_location = label(18, 21, 160, "--", ink);
    obj_temperature = label(16, 34, 105, "--", ink, false, &lv_font_montserrat_48);
    obj_weatherIcon_big = icon(101, 44, weatherImages_l[39]);
    lbl_weather_big = label(18, 88, 145, "--", muted);
    lbl_weather_age = label(18, 109, 156, "等待更新", muted);
    wind_label = label(180, 109, 124, "风速 --", muted);
    lv_obj_set_style_text_align(wind_label, LV_TEXT_ALIGN_RIGHT, 0);

    lbl_date_tomorrow = label(182, 24, 60, "明天", muted, true);
    lbl_date_tomorrow_1 = label(246, 24, 60, "后天", muted, true);
    obj_weatherIcon_tomorrow = icon(200, 50, weatherImages_s[39]);
    obj_weatherIcon_tomorrow_1 = icon(264, 50, weatherImages_s[39]);
    lbl_weather_tomorrow = label(182, 83, 60, "--", ink, true);
    lbl_weather_tomorrow_1 = label(246, 83, 60, "--", ink, true);

    rule(18, 134, 284, 0x537987);
    rule(18, 134, 28, gold);
    label(18, 144, 170, "未来24小时", ink);
    hour_interval = label(202, 144, 100, "暂无小时数据", muted);
    lv_obj_set_style_text_align(hour_interval, LV_TEXT_ALIGN_RIGHT, 0);
    for (int i = 0; i < 6; ++i) {
        const int x = 16 + i * 48;
        hour_labels[i] = label(x + 3, 169, 42, "--:--", muted, true, &font_rajdhani_16);
        hour_icons[i] = icon(x + 12, 189, weatherImages_s[39]);
        lv_obj_add_flag(hour_icons[i], LV_OBJ_FLAG_HIDDEN);
        hour_temps[i] = label(x, 216, 48, "--", gold, true, &lv_font_montserrat_16);
    }
    hal.UNLOCKLV();
    loadWeatherCache();
    hal.LOCKLV();
    if (loadWeather) updateWeather();
    updateWeatherAge();
    hal.UNLOCKLV();
    last_weather_page_revision = weatherHomeRevision();
    // Keep the shared async weather service running while this page is shown,
    // so the loop above never needs to fetch synchronously.
    weatherHomeServiceSetActive(true);
}
static bool connectToWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        GUI::toast(_tr(I18N_ID_CONNECTING));
        WiFi.begin();
        if (WiFi.waitForConnectResult(6000) == WL_CONNECTED) {
            if (hal.NTPSync()) {
                hal.time_sync = true;
            }
        } else {
            GUI::toast(_tr(I18N_ID_CONNECT_FAILED));
            return false;
        }
    }
    return true;
}
void AppWeather::loop()
{
    if (lbl_weather_age && millis() - last_age_refresh >= 60000) {
        last_age_refresh = millis();
        hal.LOCKLV();
        atmosphereRefresh();  // the time-of-day band only moves once an hour
        updateWeatherAge();
        hal.UNLOCKLV();
    }
    if (i18n::getLanguage() == 1) {
        xSemaphoreGive(appManagerLite._binary_switchApp);
        return;
    }
    if (!hal.weather_enable) {
        xSemaphoreGive(appManagerLite._binary_switchApp);
        return;
    }

    // The async home weather service refreshes weatherData in the background.
    // This loop only repaints when the shared snapshot advances — never blocks.
    const uint32_t revision = weatherHomeRevision();
    if (revision != last_weather_page_revision) {
        last_weather_page_revision = revision;
        hal.LOCKLV();
        updateWeather();
        updateWeatherAge();
        hal.UNLOCKLV();
    }
}

void AppWeather::destroy()
{
    atmosphereDetach();
    if (hal.server_started == false)
        WiFi.disconnect(true);
}
