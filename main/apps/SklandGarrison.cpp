#include "A_Config.h"
#include "Atmosphere.h"
#include "SklandApi.h"

#include <cstdio>
#include <cstring>

extern "C" const lv_font_t font_rajdhani_32;

namespace {

// What the history-log endpoint actually sends per record is just three fields:
// completeState (win/lose), trophyAfter (running trophy total) and passRound.
// There is no killCnt, difficulty, mode or timestamp, so this page only shows
// what really exists and derives the rest (streaks, averages, the win tape).
#include "GarrisonEmblem.inc"

constexpr int recentStripCount = 20;
constexpr uint32_t snapshotMagic = 0x47563431;  // 'GV41'

struct StatPack {
    uint16_t total;
    uint16_t wins;
    uint16_t winRate;
    uint16_t bestTrophy;
    uint16_t bestRound;
    uint32_t totalRound;
    uint16_t avgRound10;  // average rounds x10, so 11.4 is stored as 114
    uint8_t streakLongest;
    uint8_t streakCurrent;
    uint8_t streakWinFirst;  // 1 when the current streak is a winning one
    uint8_t recentCount;     // how many entries the tape covers
    uint8_t recentWins;
    uint32_t recentMask;  // bit i set when the i-th newest entry was a win
};
// The whole page state is small and flat, so it is stored as raw bytes.
struct SnapshotBlob {
    uint32_t magic;
    char uid[32];
    char season[64];
    uint8_t careerValid;   // six title axes below were parsed at least once
    uint16_t career[6];    // COIN, KILL, DAMAGE, BOND, CHAR, EQUIP
    StatPack pack;
};

SnapshotBlob snapshot{};
String activeUid;
uint32_t lastFetch = 0;
bool fetched = false;
bool attempted = false;
bool busy = false;

lv_obj_t *root = nullptr;
lv_obj_t *seasonLabel = nullptr;
lv_obj_t *statusLabel = nullptr;
lv_obj_t *roundTotalLabel = nullptr;
lv_obj_t *statTitles[4] = {};
lv_obj_t *statValues[4] = {};
lv_obj_t *tapeSummaryLabel = nullptr;
lv_obj_t *winBarFill = nullptr;
lv_obj_t *tapeCells[recentStripCount] = {};
lv_obj_t *chipValues[3] = {};
// The content area holds two boards: the season summary and the six title axes.
// They take turns so the small panel never has to show eight numbers at once.
lv_obj_t *statsView = nullptr;
lv_obj_t *careerView = nullptr;
lv_obj_t *careerTotalLabel = nullptr;
lv_obj_t *careerValues[6] = {};
lv_obj_t *careerBars[6] = {};
bool careerReady = false;
uint16_t careerCounts[6] = {0, 0, 0, 0, 0, 0};
uint32_t careerSwitchAt = 0;
bool careerBoardVisible = false;
constexpr uint32_t careerHoldMs = 9000;
constexpr int careerAxisCount = 6;
constexpr int careerBarWidth = 178;
constexpr int winBarWidth = 58;

const lv_color_t cyan = lv_color_hex(0x39D8F2);
const lv_color_t pale = lv_color_hex(0xD8F6FF);
const lv_color_t lime = lv_color_hex(0xD9F56A);
const lv_color_t dim = lv_color_hex(0x7AA2B0);
const lv_color_t rule = lv_color_hex(0x2A5A70);
const lv_color_t grey = lv_color_hex(0x5A6E78);
const lv_color_t faint = lv_color_hex(0x55747F);
const lv_color_t seasonInk = lv_color_hex(0x6E9CAC);

int getInt(cJSON *item, const char *key, int fallback = 0) {
    cJSON *value = item ? cJSON_GetObjectItem(item, key) : nullptr;
    return cJSON_IsNumber(value) ? static_cast<int>(value->valuedouble) : fallback;
}
bool isTrueish(cJSON *item, const char *key) {
    cJSON *value = item ? cJSON_GetObjectItem(item, key) : nullptr;
    return cJSON_IsTrue(value) || (cJSON_IsNumber(value) && getInt(item, key) != 0);
}
String getString(cJSON *item, const char *key, const char *fallback = "") {
    cJSON *value = item ? cJSON_GetObjectItem(item, key) : nullptr;
    if (cJSON_IsString(value) && value->valuestring) return value->valuestring;
    if (cJSON_IsNumber(value)) return String(static_cast<int>(value->valuedouble));
    return fallback;
}

void copyText(char *dst, size_t size, const char *src) {
    if (size) strlcpy(dst, src ? src : "", size);
}

void restoreSnapshot() {
    String uid = SklandApi::loadCredentials() ? SklandApi::uid() : String();
    if (uid == activeUid) return;
    activeUid = uid;
    snapshot = SnapshotBlob{};
    fetched = false;
    lastFetch = 0;
    if (uid.isEmpty()) return;
    Preferences store;
    if (!store.begin("garrison-view", true)) return;
    SnapshotBlob saved{};
    if (store.getBytesLength("snapshot3") == sizeof(saved) &&
        store.getBytes("snapshot3", &saved, sizeof(saved)) == sizeof(saved) &&
        saved.magic == snapshotMagic && saved.uid[sizeof(saved.uid) - 1] == 0 && uid == saved.uid)
        snapshot = saved;
}

void storeSnapshot() {
    Preferences store;
    if (!store.begin("garrison-view", false)) return;
    store.putBytes("snapshot3", &snapshot, sizeof(snapshot));
}

// ---- painting -------------------------------------------------------------
void paintStats() {
    const StatPack &pack = snapshot.pack;
    if (statValues[0]) lv_label_set_text_fmt(statValues[0], "%u", static_cast<unsigned>(pack.total));
    if (statValues[1]) lv_label_set_text_fmt(statValues[1], "%u%%", static_cast<unsigned>(pack.winRate));
    if (statValues[2]) lv_label_set_text_fmt(statValues[2], "%u", static_cast<unsigned>(pack.bestTrophy));
    if (statValues[3]) lv_label_set_text_fmt(statValues[3], "%u", static_cast<unsigned>(pack.bestRound));
    if (seasonLabel) lv_label_set_text(seasonLabel, snapshot.season[0] ? snapshot.season : "赛季未同步");
    if (roundTotalLabel) {
        if (pack.total) lv_label_set_text_fmt(roundTotalLabel, "%u", static_cast<unsigned>(pack.totalRound));
        else lv_label_set_text(roundTotalLabel, "--");
    }
    if (winBarFill) {
        const int fill = static_cast<int>(winBarWidth) * static_cast<int>(pack.winRate) / 100;
        lv_obj_set_width(winBarFill, pack.total ? fill : 0);
    }
    if (tapeSummaryLabel) {
        if (pack.recentCount) {
            const char *mark = pack.streakWinFirst ? "胜" : "负";
            lv_label_set_text_fmt(tapeSummaryLabel, "%u \u80dc %u \u8d1f \u00b7 %u \u8fde%s",
                                  static_cast<unsigned>(pack.recentWins),
                                  static_cast<unsigned>(pack.recentCount - pack.recentWins),
                                  static_cast<unsigned>(pack.streakCurrent), mark);
        } else {
            lv_label_set_text(tapeSummaryLabel, "");
        }
    }
    if (chipValues[0]) {
        if (pack.total)
            lv_label_set_text_fmt(chipValues[0], "%u.%u", static_cast<unsigned>(pack.avgRound10 / 10),
                                  static_cast<unsigned>(pack.avgRound10 % 10));
        else lv_label_set_text(chipValues[0], "--");
    }
    if (chipValues[1])
        lv_label_set_text_fmt(chipValues[1], "%u", static_cast<unsigned>(pack.streakLongest));
    if (chipValues[2]) {
        if (pack.total)
            lv_label_set_text_fmt(chipValues[2], "%u", static_cast<unsigned>(pack.streakCurrent));
        else lv_label_set_text(chipValues[2], "--");
    }
    for (int i = 0; i < recentStripCount; ++i) {
        lv_obj_t *cell = tapeCells[i];
        if (!cell) continue;
        if (i < pack.recentCount) {
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_HIDDEN);
            const bool win = (pack.recentMask >> i) & 1u;
            lv_obj_set_style_bg_color(cell, win ? lime : grey, 0);
            lv_obj_set_style_bg_opa(cell, win ? LV_OPA_COVER : static_cast<lv_opa_t>(150), 0);
        } else {
            lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void paintCareer() {
    if (!careerValues[0]) return;
    uint16_t peak = 1;
    uint32_t sum = 0;
    for (int i = 0; i < careerAxisCount; ++i) {
        if (careerCounts[i] > peak) peak = careerCounts[i];
        sum += careerCounts[i];
    }
    for (int i = 0; i < careerAxisCount; ++i) {
        lv_label_set_text_fmt(careerValues[i], "%u", static_cast<unsigned>(careerCounts[i]));
        if (careerBars[i]) {
            const int width = static_cast<int>(careerBarWidth) * static_cast<int>(careerCounts[i]) / peak;
            lv_obj_set_width(careerBars[i], width);
        }
    }
    if (careerTotalLabel)
        lv_label_set_text_fmt(careerTotalLabel, "合计 %u", static_cast<unsigned>(sum));
}

// The network lives in its own task. Calling it from loop() would hold the app
// manager mutex for the whole request, and every page switch would block on it.
struct FetchRequest {
    char uid[32];
    bool hasCached;
    SnapshotBlob cached;
};
struct FetchOutcome {
    char uid[32];
    bool valid;    // the history request itself succeeded
    bool hasPack;  // pack/season carry usable data
    char season[64];
    char notice[128];
    uint8_t careerValid;
    uint16_t career[6];
    StatPack pack;
};
portMUX_TYPE mailMux = portMUX_INITIALIZER_UNLOCKED;
FetchRequest pendingRequest{};
FetchOutcome pendingOutcome{};
volatile bool fetchWanted = false;
volatile bool incomingReady = false;
volatile bool workerBusy = false;
TaskHandle_t workerTask = nullptr;

void runFetch(const FetchRequest &req, FetchOutcome &out) {
    memset(&out, 0, sizeof(out));
    // Never start a TLS handshake before the link is up: a DNS lookup on a down
    // netif trips an lwIP assert and reboots the screen.
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin();
        copyText(out.notice, sizeof(out.notice), "等待 WiFi 连接\u2026");
        return;
    }
    const String playerUid = req.uid[0] ? String(req.uid) : SklandApi::uid();
    const String gameRole = playerUid.isEmpty() ? String() : String("1_") + playerUid + "_0";
    cJSON *resourceRoot = SklandApi::get("api/v1/game/garrison/guide/resource", "", gameRole);
    cJSON *resource = resourceRoot ? cJSON_GetObjectItem(resourceRoot, "data") : nullptr;
    cJSON *version = resource ? cJSON_GetObjectItem(resource, "versionInfo") : nullptr;
    const String seasonId = version ? getString(version, "seasonId") : String();
    String seasonName = seasonId;
    cJSON *seasons = resource ? cJSON_GetObjectItem(resource, "seasons") : nullptr;
    if (cJSON_IsArray(seasons)) {
        cJSON *entry = nullptr;
        cJSON_ArrayForEach(entry, seasons) {
            if (seasonId == getString(entry, "id")) {
                const String name = getString(entry, "name");
                if (!name.isEmpty()) seasonName = name;
                break;
            }
        }
    } else if (cJSON_IsObject(seasons)) {
        // The guide resource keys seasons by id instead of using an array.
        cJSON *entry = cJSON_GetObjectItem(seasons, seasonId.c_str());
        if (cJSON_IsObject(entry)) seasonName = getString(entry, "name", seasonId.c_str());
    }
    // This endpoint ships no display name for the running season, so never leak
    // the raw internal id onto the screen.
    if (seasonName == seasonId) seasonName = "本期赛季";
    if (seasonId.isEmpty()) {
        const String error = SklandApi::lastError();
        if (!resourceRoot) copyText(out.notice, sizeof(out.notice), error.isEmpty() ? "赛季接口请求失败" : error.c_str());
        else if (!cJSON_IsObject(resource)) copyText(out.notice, sizeof(out.notice), "赛季接口未返回资料");
        else copyText(out.notice, sizeof(out.notice), "接口未提供当前赛季");
        if (resourceRoot) cJSON_Delete(resourceRoot);
        return;
    }

    cJSON *historyRoot = SklandApi::get("api/v1/game/garrison/guide/history-log", "seasonId=" + seasonId, gameRole);
    cJSON *history = historyRoot ? cJSON_GetObjectItem(historyRoot, "data") : nullptr;
    cJSON *records = history ? cJSON_GetObjectItem(history, "records") : nullptr;
    StatPack pack{};
    if (cJSON_IsArray(records)) {
        pack.total = static_cast<uint16_t>(cJSON_GetArraySize(records));
        // Records arrive newest first, which is what the streak and the tape
        // both rely on.
        int previous = -1;
        uint8_t runLength = 0;
        bool firstGroup = true;
        int index = 0;
        cJSON *item = nullptr;
        cJSON_ArrayForEach(item, records) {
            const bool win = isTrueish(item, "completeState");
            const int trophy = getInt(item, "trophyAfter");
            const int round = getInt(item, "passRound");
            if (win) ++pack.wins;
            if (trophy > pack.bestTrophy) pack.bestTrophy = static_cast<uint16_t>(trophy);
            if (round > pack.bestRound) pack.bestRound = static_cast<uint16_t>(round);
            if (round > 0) pack.totalRound += static_cast<uint32_t>(round);

            const int result = win ? 1 : 0;
            if (result == previous) {
                if (runLength < 255) ++runLength;
            } else {
                runLength = 1;
                previous = result;
                // Everything after the very first streak belongs to history, not
                // to the "current" streak.
                if (index > 0) firstGroup = false;
            }
            if (runLength > pack.streakLongest) pack.streakLongest = runLength;
            if (index == 0) pack.streakWinFirst = win ? 1 : 0;
            if (firstGroup) pack.streakCurrent = runLength;

            if (index < recentStripCount) {
                ++pack.recentCount;
                if (win) {
                    ++pack.recentWins;
                    pack.recentMask |= 1u << index;
                }
            }
            ++index;
        }
        pack.winRate = pack.total ? static_cast<uint16_t>((pack.wins * 200u + pack.total) / (pack.total * 2u)) : 0;
        pack.avgRound10 = pack.total
                              ? static_cast<uint16_t>((pack.totalRound * 10u + pack.total / 2u) / pack.total)
                              : 0;
    }
    if (cJSON_IsArray(records)) {
        char order[32] = {0};
        for (int i = 0; i < 8 && i < cJSON_GetArraySize(records); ++i) {
            order[i] = isTrueish(cJSON_GetArrayItem(records, i), "completeState") ? 'W' : 'L';
        }
        ESP_LOGI("SklandGarrison", "order=%s mask=%08x", order, static_cast<unsigned>(pack.recentMask));
    }
    const bool valid = cJSON_IsArray(records);
    out.valid = valid;
    copyText(out.season, sizeof(out.season), seasonName.c_str());
    out.pack = pack;
    out.hasPack = valid;
    if (valid) {
        ESP_LOGI("SklandGarrison",
                 "season='%s' total=%u wins=%u rate=%u%% trophy=%u round=%u roundSum=%u avg=%u.%u longest=%u current=%u%s",
                 seasonName.c_str(), pack.total, pack.wins, pack.winRate, pack.bestTrophy, pack.bestRound,
                 static_cast<unsigned>(pack.totalRound), static_cast<unsigned>(pack.avgRound10 / 10),
                 static_cast<unsigned>(pack.avgRound10 % 10), pack.streakLongest, pack.streakCurrent,
                 pack.streakWinFirst ? "W" : "L");
        copyText(out.notice, sizeof(out.notice), pack.total ? "历史战绩已同步" : "暂无历史战绩");
    } else {
        const String error = SklandApi::lastError();
        copyText(out.notice, sizeof(out.notice),
                 req.hasCached ? "刷新失败，显示缓存" : (error.isEmpty() ? "战绩获取失败" : error.c_str()));
    }
    // Season career: six title axes. This endpoint answers with a base64 body,
    // which SklandApi::getTransformed() decodes before handing it over.
    if (valid && !seasonId.isEmpty()) {
        cJSON *careerRoot = SklandApi::getTransformed("api/ms/game/arknights/garrison/info",
                                                    "uid=" + playerUid + "&activityId=" + seasonId, "");
        if (careerRoot) {
            cJSON *careerData = cJSON_GetObjectItem(careerRoot, "data");
            cJSON *career = careerData ? cJSON_GetObjectItem(careerData, "career") : nullptr;
            cJSON *counts = career ? cJSON_GetObjectItem(career, "titleTypeCnt") : nullptr;
            if (cJSON_IsObject(counts)) {
                static const char *kAxisKeys[6] = {"COIN", "KILL", "DAMAGE", "BOND", "CHAR", "EQUIP"};
                bool any = false;
                for (int i = 0; i < 6; ++i) {
                    cJSON *axis = cJSON_GetObjectItem(counts, kAxisKeys[i]);
                    int value = cJSON_IsNumber(axis) ? axis->valueint : 0;
                    if (value < 0) value = 0;
                    if (value > 9999) value = 9999;
                    out.career[i] = static_cast<uint16_t>(value);
                    if (value) any = true;
                }
                out.careerValid = any ? 1 : 0;
                if (any)
                    ESP_LOGI("SklandGarrison", "career %u/%u/%u/%u/%u/%u", out.career[0], out.career[1],
                             out.career[2], out.career[3], out.career[4], out.career[5]);
            } else {
                ESP_LOGW("SklandGarrison", "career: no titleTypeCnt");
            }
            cJSON_Delete(careerRoot);
        } else {
            ESP_LOGW("SklandGarrison", "career request failed: %s", SklandApi::lastError().c_str());
        }
    }
    if (resourceRoot) cJSON_Delete(resourceRoot);
    if (historyRoot) cJSON_Delete(historyRoot);
}

void queueFetch() {
    portENTER_CRITICAL(&mailMux);
    copyText(pendingRequest.uid, sizeof(pendingRequest.uid), activeUid.c_str());
    pendingRequest.hasCached = snapshot.season[0] != 0;
    pendingRequest.cached = snapshot;
    fetchWanted = true;
    portEXIT_CRITICAL(&mailMux);
}

bool takeIncoming(FetchOutcome &out) {
    bool ready = false;
    portENTER_CRITICAL(&mailMux);
    if (incomingReady) {
        ready = true;
        out = pendingOutcome;
        incomingReady = false;
    }
    portEXIT_CRITICAL(&mailMux);
    return ready;
}

void workerEntry(void *) {
    for (;;) {
        FetchRequest req{};
        bool wanted = false;
        // Claim the request and flag the busy state in one critical section so
        // destroy() can never delete this task in the middle of a request.
        portENTER_CRITICAL(&mailMux);
        if (fetchWanted) {
            wanted = true;
            fetchWanted = false;
            req = pendingRequest;
            workerBusy = true;
        }
        portEXIT_CRITICAL(&mailMux);
        if (!wanted) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        FetchOutcome out;
        runFetch(req, out);
        portENTER_CRITICAL(&mailMux);
        pendingOutcome = out;
        incomingReady = true;
        workerBusy = false;
        portEXIT_CRITICAL(&mailMux);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
}  // namespace

// ---- widgets --------------------------------------------------------------
namespace {
lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, int x, int y, int width, lv_color_t color,
                    const lv_font_t *font, lv_text_align_t align = LV_TEXT_ALIGN_LEFT) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(label, align, 0);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    return label;
}
lv_obj_t *makeRect(lv_obj_t *parent, int x, int y, int width, int height, lv_color_t color,
                   lv_opa_t opa = LV_OPA_COVER) {
    lv_obj_t *object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    lv_obj_set_style_bg_color(object, color, 0);
    lv_obj_set_style_bg_opa(object, opa, 0);
    lv_obj_set_style_radius(object, 0, 0);
    return object;
}
lv_obj_t *makeGlyph(lv_obj_t *parent, const lv_img_dsc_t *source, int x, int y, lv_color_t color,
                    lv_opa_t opa) {
    lv_obj_t *image = lv_img_create(parent);
    lv_img_set_src(image, source);
    lv_obj_set_pos(image, x, y);
    lv_obj_set_style_img_recolor(image, color, 0);
    lv_obj_set_style_img_recolor_opa(image, LV_OPA_COVER, 0);
    lv_obj_set_style_img_opa(image, opa, 0);
    return image;
}
}  // namespace

void AppSklandGarrison::setup() {
    root = _appScreen;
    restoreSnapshot();
    hal.LOCKLV();
    lv_obj_set_style_bg_color(root, lv_color_hex(0x073652), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    // Same weather scenery and time tint the home page uses, veiled enough that
    // the pale stat type keeps its contrast over the art.
    atmosphereAttach(root, 0x05243A, 168);

    // Faint PRTS emblem bleeding out of the top right corner.
    makeGlyph(root, &garrisonEmblem, 246, -18, lv_color_hex(0x2E6E8C), 30);
    makeGlyph(root, &garrisonBadge, 8, 7, pale, 240);
    makeLabel(root, "卫戍协议", 35, 5, 120, pale, &lv_font_chinese_16);
    makeLabel(root, "总轮次", 152, 10, 60, dim, &lv_font_chinese_16, LV_TEXT_ALIGN_RIGHT);
    roundTotalLabel = makeLabel(root, "--", 214, 1, 98, pale, &font_rajdhani_32, LV_TEXT_ALIGN_RIGHT);
    makeRect(root, 0, 35, 320, 1, rule);
    makeRect(root, 0, 35, 58, 2, cyan);

    auto makeBoard = [](lv_obj_t *parent) {
        lv_obj_t *view = lv_obj_create(parent);
        lv_obj_remove_style_all(view);
        lv_obj_set_size(view, 320, 240);
        lv_obj_set_pos(view, 0, 0);
        lv_obj_clear_flag(view, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(view, LV_OBJ_FLAG_CLICKABLE);
        return view;
    };
    statsView = makeBoard(root);
    careerView = makeBoard(root);
    lv_obj_add_flag(careerView, LV_OBJ_FLAG_HIDDEN);
    careerSwitchAt = millis();

    const char *titles[] = {"场次", "胜率", "奖杯", "轮次"};
    const lv_color_t valueColors[] = {cyan, lime, cyan, cyan};
    for (int i = 0; i < 4; ++i) {
        const int x = 4 + 79 * i;
        if (i) makeRect(statsView, x - 4, 43, 1, 42, rule);
        statTitles[i] = makeLabel(statsView, titles[i], x + 4, 41, 72, dim, &lv_font_chinese_16);
        statValues[i] = makeLabel(statsView, "--", x + 4, 57, 72, valueColors[i], &font_rajdhani_32);
    }

    makeRect(statsView, 83, 88, winBarWidth, 3, rule);
    winBarFill = makeRect(statsView, 83, 88, 0, 3, lime);

    makeLabel(statsView, "近 20 场", 8, 94, 120, dim, &lv_font_chinese_16);
    tapeSummaryLabel = makeLabel(statsView, "", 140, 94, 172, faint, &lv_font_chinese_16,
                                 LV_TEXT_ALIGN_RIGHT);
    makeRect(statsView, 8, 114, 304, 1, rule);
    for (int i = 0; i < recentStripCount; ++i)
        tapeCells[i] = makeRect(statsView, 8 + 15 * i, 119, 14, 14, lime);

    const char *chipTitles[] = {"场均轮次", "最长连胜", "当前连胜"};
    const lv_color_t chipColors[] = {cyan, lime, lime};
    for (int i = 0; i < 3; ++i) {
        const int x = 8 + 104 * i;
        if (i) makeRect(statsView, x - 8, 145, 1, 40, rule);
        makeLabel(statsView, chipTitles[i], x, 142, 96, dim, &lv_font_chinese_16);
        chipValues[i] = makeLabel(statsView, "--", x, 158, 96, chipColors[i], &font_rajdhani_32);
    }

    makeRect(root, 8, 196, 304, 1, rule);
    statusLabel = makeLabel(root, snapshot.season[0] ? "已载入缓存" : "连接森空岛中…", 8, 202, 188, dim,
                            &lv_font_chinese_16);
    seasonLabel = makeLabel(root, "本期赛季", 198, 206, 114, seasonInk, &lv_font_chinese_16,
                            LV_TEXT_ALIGN_RIGHT);

    // Career board: the six title axes as labelled bars. A radar would have to be
    // tiny to fit six labels around it on 240 rows, and the numbers would vanish.
    makeLabel(careerView, "赛季称号", 8, 41, 120, dim, &lv_font_chinese_16);
    careerTotalLabel = makeLabel(careerView, "", 172, 41, 140, faint, &lv_font_chinese_16,
                                 LV_TEXT_ALIGN_RIGHT);
    static const char *kCareerNames[6] = {"资金管理", "补救能力", "领袖挑战",
                                          "盟约理解", "人力调度", "装备活用"};
    for (int i = 0; i < careerAxisCount; ++i) {
        const int y = 64 + 21 * i;
        makeLabel(careerView, kCareerNames[i], 8, y, 68, dim, &lv_font_chinese_16);
        careerValues[i] = makeLabel(careerView, "0", 78, y, 34, cyan, &lv_font_chinese_16,
                                    LV_TEXT_ALIGN_RIGHT);
        makeRect(careerView, 118, y + 6, careerBarWidth, 6, rule);
        careerBars[i] = makeRect(careerView, 118, y + 6, 0, 6, lime);
    }

    if (snapshot.careerValid) {
        careerReady = true;
        memcpy(careerCounts, snapshot.career, sizeof(careerCounts));
    }
    paintStats();
    paintCareer();
    hal.UNLOCKLV();
}

void AppSklandGarrison::loop() {
    FetchOutcome out;
    if (takeIncoming(out)) {
        busy = false;
        if (!out.uid[0] || activeUid == out.uid) {
            lastFetch = millis();
            fetched = out.valid;
            if (root) {
                hal.LOCKLV();
                if (out.hasPack) {
                    snapshot.pack = out.pack;
                    copyText(snapshot.season, sizeof(snapshot.season), out.season);
                    snapshot.magic = snapshotMagic;
                    copyText(snapshot.uid, sizeof(snapshot.uid), activeUid.c_str());
                    if (out.careerValid) {
                        careerReady = true;
                        memcpy(careerCounts, out.career, sizeof(careerCounts));
                        snapshot.careerValid = 1;
                        memcpy(snapshot.career, out.career, sizeof(snapshot.career));
                        paintCareer();
                    }
                    paintStats();
                    storeSnapshot();
                }
                if (out.notice[0] && statusLabel) lv_label_set_text(statusLabel, out.notice);
                hal.UNLOCKLV();
            }
        }
    }
    const uint32_t now = millis();
    if (root) {
        hal.LOCKLV();
        atmosphereRefresh();
        hal.UNLOCKLV();
    }
    // Two boards share one screen area: the season summary and the six title axes.
    // Flipping keeps both readable at 16px instead of cramming them together.
    if (root && careerReady && now - careerSwitchAt >= careerHoldMs) {
        careerSwitchAt = now;
        careerBoardVisible = !careerBoardVisible;
        hal.LOCKLV();
        if (careerBoardVisible) {
            lv_obj_add_flag(statsView, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(careerView, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(careerView, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(statsView, LV_OBJ_FLAG_HIDDEN);
        }
        hal.UNLOCKLV();
    }
    // A dropped answer must not block the page forever (two 12s request timeouts
    // are the worst case of a single fetch).
    if (busy && now - lastFetch > 45000u) busy = false;
    const bool due = (attempted ? (fetched ? now - lastFetch > 300000u : now - lastFetch > 10000u) : now > 4000u);
    if (!busy && due) {
        if (!workerTask)
            xTaskCreatePinnedToCore(workerEntry, "garrison_net", 10240, nullptr, 2, &workerTask, 0);
        if (workerTask) {
            attempted = true;
            busy = true;
            lastFetch = now;
            queueFetch();
        }
    }
    vTaskDelay(pdMS_TO_TICKS(40));
}

void AppSklandGarrison::destroy() {
    atmosphereDetach();
    root = nullptr;
    seasonLabel = nullptr;
    statusLabel = nullptr;
    roundTotalLabel = nullptr;
    tapeSummaryLabel = nullptr;
    winBarFill = nullptr;
    for (auto &label : statTitles) label = nullptr;
    for (auto &label : statValues) label = nullptr;
    for (auto &cell : tapeCells) cell = nullptr;
    for (auto &value : chipValues) value = nullptr;
    statsView = nullptr;
    careerView = nullptr;
    careerTotalLabel = nullptr;
    for (auto &value : careerValues) value = nullptr;
    for (auto &bar : careerBars) bar = nullptr;
    careerReady = false;
    careerBoardVisible = false;
    // Keep the worker task across page changes, but release it while it is idle so a
    // re-entry starts clean and the 10KB stack returns to the heap.
    TaskHandle_t idleWorker = nullptr;
    portENTER_CRITICAL(&mailMux);
    if (workerTask && !workerBusy) {
        idleWorker = workerTask;
        workerTask = nullptr;
        fetchWanted = false;
    }
    portEXIT_CRITICAL(&mailMux);
    if (idleWorker) vTaskDelete(idleWorker);
}
