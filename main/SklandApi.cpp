#include "A_Config.h"
#include "SklandApi.h"
#include <cJSON.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp32-hal-psram.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <mbedtls/md5.h>

namespace SklandApi {
namespace {
String credValue, accountValue, uidValue, nameValue, apiToken, errorValue;
uint32_t apiTimestamp = 0;
constexpr char kHost[] = "https://zonai.skland.com/";
constexpr char kVersion[] = "1.21.0";
constexpr char kAppCode[] = "4ca99fa6b56cc2ba";

const char *skipSpace(const char *p, const char *end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p;
    return p;
}

const char *skipJsonValue(const char *p, const char *end) {
    p = skipSpace(p, end);
    if (p >= end) return nullptr;
    if (*p == '"') {
        for (++p; p < end; ++p) {
            if (*p == '\\') { if (++p >= end) return nullptr; continue; }
            if (*p == '"') return p + 1;
        }
        return nullptr;
    }
    if (*p == '{' || *p == '[') {
        int depth = 0;
        bool inString = false;
        for (; p < end; ++p) {
            if (inString) {
                if (*p == '\\') { if (++p >= end) return nullptr; }
                else if (*p == '"') inString = false;
                continue;
            }
            if (*p == '"') inString = true;
            else if (*p == '{' || *p == '[') ++depth;
            else if (*p == '}' || *p == ']') { if (--depth == 0) return p + 1; }
        }
        return nullptr;
    }
    while (p < end && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') ++p;
    return p;
}

bool findObjectValue(const char *object, const char *end, const char *key, const char **valueStart, const char **valueEnd) {
    if (!object || object >= end || *object != '{') return false;
    const char *p = skipSpace(object + 1, end);
    while (p < end && *p != '}') {
        if (*p != '"') return false;
        const char *keyStart = p + 1;
        const char *keyEnd = keyStart;
        while (keyEnd < end) {
            if (*keyEnd == '\\') { keyEnd += 2; continue; }
            if (*keyEnd == '"') break;
            ++keyEnd;
        }
        if (keyEnd >= end) return false;
        p = skipSpace(keyEnd + 1, end);
        if (p >= end || *p++ != ':') return false;
        p = skipSpace(p, end);
        const char *start = p;
        const char *finish = skipJsonValue(start, end);
        if (!finish) return false;
        if (static_cast<size_t>(keyEnd - keyStart) == strlen(key) && memcmp(keyStart, key, strlen(key)) == 0) {
            *valueStart = start; *valueEnd = finish; return true;
        }
        p = skipSpace(finish, end);
        if (p < end && *p == ',') p = skipSpace(p + 1, end);
        else break;
    }
    return false;
}

bool appendRawField(String &out, const char *object, const char *end, const char *key, bool &first) {
    const char *start = nullptr, *finish = nullptr;
    if (!findObjectValue(object, end, key, &start, &finish)) return false;
    if (!first) out += ',';
    out += '"'; out += key; out += "\":";
    out.concat(start, static_cast<unsigned int>(finish - start));
    first = false;
    return true;
}

const char *findNamedObject(const char *from, const char *end, const char *key) {
    const size_t keyLength = strlen(key);
    const char *p = from;
    while (p < end) {
        const char *match = strstr(p, "\"");
        if (!match || match >= end) return nullptr;
        const char *keyStart = match + 1;
        const char *keyEnd = keyStart + keyLength;
        if (keyEnd < end && memcmp(keyStart, key, keyLength) == 0 && *keyEnd == '"') {
            const char *value = skipSpace(keyEnd + 1, end);
            if (value < end && *value++ == ':') {
                value = skipSpace(value, end);
                if (value < end && *value == '{') return value;
            }
        }
        p = match + 1;
    }
    return nullptr;
}

cJSON *parsePlayerSummary(const char *json, size_t size, bool requireAllFields) {
    const char *end = json + size;
    const char *begin = skipSpace(json, end);
    const char *data = findNamedObject(begin, end, "data");
    if (!data) return nullptr;
    const char *status = findNamedObject(data, end, "status");
    const char *routine = findNamedObject(data, end, "routine");
    const char *campaign = findNamedObject(data, end, "campaign");
    String summary; summary.reserve(4096);
    summary = "{\"data\":{\"status\":{";
    bool first = true;
    const bool hasName = appendRawField(summary, status, end, "name", first);
    const bool hasLevel = appendRawField(summary, status, end, "level", first);
    const bool hasAp = appendRawField(summary, status, end, "ap", first);
    appendRawField(summary, status, end, "storeTs", first);
    appendRawField(summary, status, end, "avatar", first);
    appendRawField(summary, status, end, "secretary", first);
    appendRawField(summary, status, end, "charCnt", first);
    appendRawField(summary, status, end, "mainStageProgress", first);
    summary += "},\"routine\":{";
    bool routineFirst = true;
    const bool hasDaily = appendRawField(summary, routine, end, "daily", routineFirst);
    const bool hasWeekly = appendRawField(summary, routine, end, "weekly", routineFirst);
    summary += "},\"campaign\":{";
    bool campaignFirst = true;
    const bool hasReward = appendRawField(summary, campaign, end, "reward", campaignFirst);
    summary += "},\"building\":{";
    const char *building = findNamedObject(data, end, "building");
    bool buildingFirst = true;
    appendRawField(summary, building, end, "labor", buildingFirst);
    const char *rogue = findNamedObject(data, end, "rogue");
    summary += "},\"rogue\":{";
    bool rogueFirst = true;
    appendRawField(summary, rogue, end, "records", rogueFirst);
    summary += "}}}";
    if (!(hasName && hasLevel && hasAp) || (requireAllFields && !(hasDaily && hasWeekly && hasReward))) return nullptr;
    return cJSON_ParseWithLength(summary.c_str(), summary.length());
}

cJSON *parseGarrisonResourceSummary(const char *json, size_t size) {
    const char *end = json + size;
    const char *begin = skipSpace(json, end);
    const char *data = findNamedObject(begin, end, "data");
    if (!data) return nullptr;
    // Do not walk every data property to find versionInfo: the seasons/assets
    // fields before it can be hundreds of KB. Find the object key directly,
    // then copy only the tiny season/version fields needed by the display.
    const char *version = findNamedObject(data, end, "versionInfo");
    if (!version) return nullptr;
    const char *seasonStart = nullptr, *seasonEnd = nullptr;
    if (!findObjectValue(version, end, "seasonId", &seasonStart, &seasonEnd)) return nullptr;
    String summary; summary.reserve(256);
    summary = "{\"data\":{\"versionInfo\":{\"seasonId\":";
    summary.concat(seasonStart, static_cast<unsigned int>(seasonEnd - seasonStart));
    const char *versionStart = nullptr, *versionEnd = nullptr;
    if (findObjectValue(version, end, "versionId", &versionStart, &versionEnd)) {
        summary += ",\"versionId\":";
        summary.concat(versionStart, static_cast<unsigned int>(versionEnd - versionStart));
    }
    const char *nameStart = nullptr, *nameEnd = nullptr;
    if (findObjectValue(version, end, "name", &nameStart, &nameEnd)) {
        summary += ",\"name\":";
        summary.concat(nameStart, static_cast<unsigned int>(nameEnd - nameStart));
    }
    summary += "}}}";
    return cJSON_ParseWithLength(summary.c_str(), summary.length());
}

const char *findNamedArray(const char *from, const char *end, const char *key) {
    const size_t keyLength = strlen(key);
    const char *p = from;
    while (p < end) {
        const char *match = strstr(p, "\"");
        if (!match || match >= end) return nullptr;
        const char *keyStart = match + 1;
        const char *keyEnd = keyStart + keyLength;
        if (keyEnd < end && memcmp(keyStart, key, keyLength) == 0 && *keyEnd == '\"') {
            const char *value = skipSpace(keyEnd + 1, end);
            if (value < end && *value++ == ':') {
                value = skipSpace(value, end);
                if (value < end && *value == '[') return value;
            }
        }
        p = match + 1;
    }
    return nullptr;
}

int jsonIntField(cJSON *obj, const char *key, int fallback = 0) {
    cJSON *value = obj ? cJSON_GetObjectItem(obj, key) : nullptr;
    if (cJSON_IsNumber(value)) return value->valueint;
    if (cJSON_IsString(value) && value->valuestring[0]) return atoi(value->valuestring);
    return fallback;
}

cJSON *parseGarrisonHistorySummary(const char *json, size_t size) {
    const char *end = json + size;
    const char *begin = skipSpace(json, end);
    const char *data = findNamedObject(begin, end, "data");
    if (!data) return nullptr;
    const char *records = findNamedArray(data, end, "records");
    if (!records) return nullptr;
    String summary; summary.reserve(8192);
    summary = "{\"data\":{\"records\":[";
    bool first = true;
    const char *p = skipSpace(records + 1, end);
    while (p < end && *p != ']') {
        const char *finish = skipJsonValue(p, end);
        if (!finish) return nullptr;
        if (*p == '{') {
            cJSON *record = cJSON_ParseWithLength(p, static_cast<size_t>(finish - p));
            if (!record) return nullptr;
            const int complete = cJSON_IsTrue(cJSON_GetObjectItem(record, "completeState")) ? 1 :
                                 jsonIntField(record, "completeState");
            const int trophy = jsonIntField(record, "trophyAfter");
            const int round = jsonIntField(record, "passRound");
            if (!first) summary += ',';
            summary += "{\"completeState\":" + String(complete) +
                       ",\"trophyAfter\":" + String(trophy) +
                       ",\"passRound\":" + String(round) + "}";
            first = false;
            cJSON_Delete(record);
        }
        p = skipSpace(finish, end);
        if (p < end && *p == ',') p = skipSpace(p + 1, end);
        else if (p >= end || *p != ']') return nullptr;
    }
    if (p >= end || *p != ']') return nullptr;
    summary += "]}}";
    return cJSON_ParseWithLength(summary.c_str(), summary.length());
}

String hex(const unsigned char *data, size_t size) {
    static const char chars[] = "0123456789abcdef";
    String out; out.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) { out += chars[data[i] >> 4]; out += chars[data[i] & 15]; }
    return out;
}

String signature(const String &pathQuery, const String &payload, const String &token, uint32_t ts) {
    const String timestamp = String(ts);
    const String signHeaders = String("{\"platform\":\"1\",\"timestamp\":\"") + timestamp +
                               "\",\"dId\":\"\",\"vName\":\"" + kVersion + "\"}";
    const String source = pathQuery + payload + timestamp + signHeaders;
    unsigned char hmac[32] = {};
    mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                    reinterpret_cast<const unsigned char *>(token.c_str()), token.length(),
                    reinterpret_cast<const unsigned char *>(source.c_str()), source.length(), hmac);
    const String hmacHex = hex(hmac, sizeof(hmac));
    unsigned char digest[16] = {};
    mbedtls_md5(reinterpret_cast<const unsigned char *>(hmacHex.c_str()), hmacHex.length(), digest);
    return hex(digest, sizeof(digest));
}

cJSON *plainJsonRequest(const char *url, const String &payload) {
    esp_http_client_config_t config = {};
    config.url = url;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 12000;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return nullptr;
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json;charset=UTF-8");
    esp_http_client_set_post_field(client, payload.c_str(), payload.length());

    cJSON *result = nullptr;
    if (esp_http_client_open(client, payload.length()) == ESP_OK &&
        esp_http_client_write(client, payload.c_str(), payload.length()) == payload.length() &&
        esp_http_client_fetch_headers(client) >= 0) {
        const int status = esp_http_client_get_status_code(client);
        char *buffer = static_cast<char *>(malloc(8192));
        if (buffer) {
            const int received = esp_http_client_read_response(client, buffer, 8191);
            if (received > 0) {
                buffer[received] = 0;
                result = cJSON_ParseWithLength(buffer, received);
            }
            free(buffer);
        }
        if (status < 200 || status >= 300) {
            if (result) { cJSON_Delete(result); result = nullptr; }
            errorValue = "登录续期 HTTP " + String(status);
        }
    } else {
        errorValue = "登录续期网络失败";
    }
    esp_http_client_cleanup(client);
    return result;
}

bool renewCredFromAccount() {
    if (accountValue.isEmpty()) {
        errorValue = "登录已过期，请重新扫码";
        return false;
    }

    const String grantBody = String("{\"token\":\"") + accountValue +
                             "\",\"appCode\":\"" + kAppCode + "\",\"type\":0}";
    cJSON *grantRoot = plainJsonRequest("https://as.hypergryph.com/user/oauth2/v2/grant", grantBody);
    cJSON *grantData = grantRoot ? cJSON_GetObjectItem(grantRoot, "data") : nullptr;
    cJSON *code = grantData ? cJSON_GetObjectItem(grantData, "code") : nullptr;
    if (!cJSON_IsString(code) || !code->valuestring[0]) {
        if (grantRoot) cJSON_Delete(grantRoot);
        if (errorValue.isEmpty()) errorValue = "长期登录已失效，请重新扫码";
        return false;
    }

    const String credBody = String("{\"kind\":1,\"code\":\"") + code->valuestring + "\"}";
    cJSON *credRoot = plainJsonRequest("https://zonai.skland.com/web/v1/user/auth/generate_cred_by_code", credBody);
    cJSON *credData = credRoot ? cJSON_GetObjectItem(credRoot, "data") : nullptr;
    cJSON *cred = credData ? cJSON_GetObjectItem(credData, "cred") : nullptr;
    const bool ok = cJSON_IsString(cred) && cred->valuestring[0];
    if (ok) {
        credValue = cred->valuestring;
        apiToken = "";
        apiTimestamp = 0;
        Preferences storage;
        if (storage.begin("skland", false)) {
            storage.putString("cred", credValue);
            storage.end();
        }
        errorValue = "";
    } else if (errorValue.isEmpty()) {
        errorValue = "登录凭证续期失败";
    }
    if (credRoot) cJSON_Delete(credRoot);
    if (grantRoot) cJSON_Delete(grantRoot);
    return ok;
}

bool refresh();

// The season-career answer packs the entire season (every match record plus the
// title lists) into one base64 blob. Handing that to cJSON together with its
// decoded twin costs more heap than the screen has, and a failed allocation there
// starves the WiFi timer and reboots. So decode it, then keep only the six title
// counters and rebuild a tiny tree with the shape the caller expects.
cJSON *parseGarrisonCareerSummary(const char *json, size_t size) {
    const char *key = nullptr;
    for (size_t i = 0; i + 9 < size; ++i) {
        if (memcmp(json + i, "\"content\"", 9) == 0) {
            key = json + i + 9;
            break;
        }
    }
    if (!key) return nullptr;
    while (key < json + size && (*key == ' ' || *key == ':' || *key == '\t')) ++key;
    if (key >= json + size || *key != '"') return nullptr;
    const char *start = key + 1;
    const char *end = start;
    while (end < json + size && *end != '"') ++end;
    if (end >= json + size) return nullptr;
    const size_t encoded = static_cast<size_t>(end - start);
    if (encoded < 16 || encoded > 1024 * 1024) return nullptr;

    const size_t capacity = encoded / 4 * 3 + 8;
    auto *raw = static_cast<unsigned char *>(ps_malloc(capacity));
    if (!raw) return nullptr;
    size_t decoded = 0;
    const bool ok = mbedtls_base64_decode(raw, capacity - 1, &decoded,
                                          reinterpret_cast<const unsigned char *>(start), encoded) == 0 &&
                    decoded > 1;
    if (!ok) {
        free(raw);
        return nullptr;
    }
    raw[decoded] = 0;

    const char *hit = strstr(reinterpret_cast<const char *>(raw), "\"titleTypeCnt\"");
    const char *brace = hit ? strchr(hit, '{') : nullptr;
    if (!brace) {
        free(raw);
        return nullptr;
    }
    int depth = 0;
    bool quoted = false;
    const char *stop = brace;
    for (; *stop; ++stop) {
        const char c = *stop;
        if (quoted) {
            if (c == '"') quoted = false;
            continue;
        }
        if (c == '"') quoted = true;
        else if (c == '{') ++depth;
        else if (c == '}' && --depth == 0) {
            ++stop;
            break;
        }
    }
    const size_t slice = static_cast<size_t>(stop - brace);
    if (depth != 0 || slice < 2 || slice > 4096) {
        free(raw);
        return nullptr;
    }
    char *copy = static_cast<char *>(malloc(slice + 1));
    if (!copy) {
        free(raw);
        return nullptr;
    }
    memcpy(copy, brace, slice);
    copy[slice] = 0;
    cJSON *counts = cJSON_ParseWithLength(copy, slice);
    free(copy);
    free(raw);
    if (!counts) return nullptr;

    cJSON *root = cJSON_CreateObject();
    cJSON *data = root ? cJSON_AddObjectToObject(root, "data") : nullptr;
    cJSON *career = data ? cJSON_AddObjectToObject(data, "career") : nullptr;
    if (!career) {
        cJSON_Delete(root);
        cJSON_Delete(counts);
        return nullptr;
    }
    cJSON_AddItemToObject(career, "titleTypeCnt", counts);
    return root;
}

cJSON *request(const String &pathQuery, const char *method, const String &payload, bool isRefresh, const String &role, bool retryAuth = true, bool transformed = false) {
    esp_http_client_config_t config = {};
    String url = String(kHost) + pathQuery;
    config.url = url.c_str(); config.crt_bundle_attach = esp_crt_bundle_attach; config.timeout_ms = 12000;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return nullptr;
    // Refresh requests must use the current clock; reusing the previous server
    // timestamp makes the next refresh signature expire after a few minutes.
    const uint32_t ts = (!isRefresh && apiTimestamp) ? apiTimestamp : static_cast<uint32_t>(time(nullptr));
    const String rawPath = pathQuery.substring(0, pathQuery.indexOf('?') >= 0 ? pathQuery.indexOf('?') : pathQuery.length());
    // The Skland signature is based on URL.pathname, which always begins with '/'.
    // Signing "api/v1/..." produces a valid HTTP request but an invalid signature.
    const String pathOnly = rawPath.startsWith("/") ? rawPath : "/" + rawPath;
    esp_http_client_set_method(client, strcmp(method, "POST") == 0 ? HTTP_METHOD_POST : HTTP_METHOD_GET);
    esp_http_client_set_header(client, "User-Agent", "Skland/2.2.0 (com.hypergryph.skland; build:200200057; Android; )");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept-Language", "zh-Hans-CN;q=1.0");
    esp_http_client_set_header(client, "platform", "1");
    esp_http_client_set_header(client, "timestamp", String(ts).c_str());
    esp_http_client_set_header(client, "dId", "");
    esp_http_client_set_header(client, "vName", kVersion);
    esp_http_client_set_header(client, "language", "zh-hans-CN");
    const int queryAt = pathQuery.indexOf('?');
    const String signBody = (queryAt >= 0 ? pathQuery.substring(queryAt + 1) : String("")) + payload;
    esp_http_client_set_header(client, "sign", signature(pathOnly, signBody, isRefresh ? String("") : apiToken, ts).c_str());
    if (!credValue.isEmpty()) esp_http_client_set_header(client, "cred", credValue.c_str());
    if (!isRefresh && !apiToken.isEmpty()) esp_http_client_set_header(client, "token", apiToken.c_str());
    if (!role.isEmpty()) esp_http_client_set_header(client, "sk-game-role", role.c_str());
    // Asking for base64 keeps big seasonal payloads (career/titles) intact.
    if (transformed) esp_http_client_set_header(client, "x-transformed", "base64");
    if (!payload.isEmpty()) esp_http_client_set_post_field(client, payload.c_str(), payload.length());
    cJSON *result = nullptr;
    int status = 0;
    errorValue = "";
    if (esp_http_client_open(client, payload.length()) == ESP_OK &&
        (payload.isEmpty() || esp_http_client_write(client, payload.c_str(), payload.length()) == payload.length()) &&
        esp_http_client_fetch_headers(client) >= 0) {
        status = esp_http_client_get_status_code(client);
        if (status >= 200 && status < 300) {
            ESP_LOGI("SklandApi", "HTTP %d: %s", status, pathOnly.c_str());
            const int64_t announced = esp_http_client_get_content_length(client);
            const bool isPlayerInfo = pathOnly == "/api/v1/game/player/info";
            const bool isGarrisonResource = pathOnly == "/api/v1/game/garrison/guide/resource";
            const bool isGarrisonHistory = pathOnly == "/api/v1/game/garrison/guide/history-log";
            // The career answer packs the whole season history as base64, so it is
            // far past the 64KB default.
            const bool isGarrisonCareer = pathOnly == "/api/ms/game/arknights/garrison/info";
            // Player-info can contain large operator inventories. Keep reading
            // until the compact identity, resource and mission fields are present.
            size_t capacity = announced > 0 ? static_cast<size_t>(announced) + 1 :
                              ((isGarrisonResource || isGarrisonHistory || isGarrisonCareer) ? 2 * 1024 * 1024
                                                                                                 : isPlayerInfo ? 2 * 1024 * 1024
                                                                                                                : 65536);
            if (capacity > 2 * 1024 * 1024) capacity = 2 * 1024 * 1024;
            char *buffer = static_cast<char *>(ps_malloc(capacity));
            if (!buffer) buffer = static_cast<char *>(malloc(capacity));
            if (buffer) {
                size_t used = 0;
                while (used + 1 < capacity) {
                    const int n = esp_http_client_read(client, buffer + used, capacity - used - 1);
                    if (n <= 0) break;
                    used += static_cast<size_t>(n);
                    buffer[used] = 0;
                    if (isPlayerInfo) {
                        cJSON *candidate = parsePlayerSummary(buffer, used, false);
                        if (candidate) {
                            cJSON *data = cJSON_GetObjectItem(candidate, "data");
                            cJSON *routine = cJSON_GetObjectItem(data, "routine");
                            cJSON *campaign = cJSON_GetObjectItem(data, "campaign");
                            cJSON *daily = cJSON_GetObjectItem(routine, "daily");
                            cJSON *weekly = cJSON_GetObjectItem(routine, "weekly");
                            cJSON *reward = cJSON_GetObjectItem(campaign, "reward");
                            cJSON *status = cJSON_GetObjectItem(data, "status");
                            cJSON *building = cJSON_GetObjectItem(data, "building");
                            cJSON *labor = building ? cJSON_GetObjectItem(building, "labor") : nullptr;
                            cJSON *rogue = cJSON_GetObjectItem(data, "rogue");
                            cJSON *records = rogue ? cJSON_GetObjectItem(rogue, "records") : nullptr;
                            const bool hasProfile = cJSON_GetObjectItem(status, "avatar") && cJSON_GetObjectItem(status, "secretary") &&
                                cJSON_GetObjectItem(status, "charCnt") && cJSON_GetObjectItem(status, "mainStageProgress");
                            const bool hasTasks = daily && weekly && reward;
                            const bool hasResources = labor && cJSON_IsArray(records);
                            if ((hasProfile && hasTasks && hasResources)) { result = candidate; break; }
                            cJSON_Delete(candidate);
                        }
                    } else if (isGarrisonResource) {
                        result = parseGarrisonResourceSummary(buffer, used);
                        if (result) break;
                    }
                }
                if (used > 0) {
                    buffer[used] = 0;
                    if (isGarrisonHistory) ESP_LOGI("SklandApi", "history response bytes=%u complete=%d announced=%lld",
                                                     static_cast<unsigned>(used), esp_http_client_is_complete_data_received(client), announced);
                    if (!result && isPlayerInfo) result = parsePlayerSummary(buffer, used, false);
                    else if (!result && isGarrisonResource) result = parseGarrisonResourceSummary(buffer, used);
                    else if (!result && isGarrisonHistory) result = parseGarrisonHistorySummary(buffer, used);
                    else if (!result && isGarrisonCareer) result = parseGarrisonCareerSummary(buffer, used);
                    else if (!isPlayerInfo && !isGarrisonResource && !isGarrisonHistory && !isGarrisonCareer)
                        result = cJSON_ParseWithLength(buffer, used);
                    if (!result) {
                        ESP_LOGW("SklandApi", "JSON parse failed: %s, bytes=%u, announced=%lld",
                                 pathQuery.c_str(), static_cast<unsigned>(used), announced);
                    }
                    if (!result) errorValue = "博士信息解析失败";
                }
                if (!result && used + 1 >= capacity && !esp_http_client_is_complete_data_received(client)) {
                    ESP_LOGW("SklandApi", "response truncated: %s, capacity=%u",
                             pathOnly.c_str(), static_cast<unsigned>(capacity));
                    errorValue = "博士信息字段未在响应前段找到";
                }
                free(buffer);
            }
        } else {
            errorValue = "HTTP " + String(status);
            char errorBody[384] = {};
            const int received = esp_http_client_read_response(client, errorBody, sizeof(errorBody) - 1);
            if (received > 0) {
                errorBody[received] = 0;
                ESP_LOGW("SklandApi", "HTTP %d: %s body=%s", status, pathQuery.c_str(), errorBody);
            } else {
                ESP_LOGW("SklandApi", "HTTP %d: %s (empty response)", status, pathQuery.c_str());
            }
        }
    } else errorValue = "网络请求失败";
    esp_http_client_cleanup(client);
    if (status == 401 && !isRefresh && retryAuth && renewCredFromAccount() && refresh()) {
        return request(pathQuery, method, payload, false, role, false);
    }
    return result;
}

bool refresh() {
    if (credValue.isEmpty()) { errorValue = "登录凭证为空"; return false; }
    cJSON *root = request("api/v1/auth/refresh", "GET", "", true, "");
    cJSON *data = root ? cJSON_GetObjectItem(root, "data") : nullptr;
    cJSON *token = data ? cJSON_GetObjectItem(data, "token") : nullptr;
    cJSON *timestamp = root ? cJSON_GetObjectItem(root, "timestamp") : nullptr;
    const bool ok = cJSON_IsString(token) && token->valuestring[0];
    if (ok) {
        apiToken = token->valuestring;
        if (cJSON_IsNumber(timestamp) && timestamp->valuedouble > 0) apiTimestamp = static_cast<uint32_t>(timestamp->valuedouble);
        else if (cJSON_IsString(timestamp) && timestamp->valuestring[0]) apiTimestamp = static_cast<uint32_t>(strtoul(timestamp->valuestring, nullptr, 10));
        else apiTimestamp = static_cast<uint32_t>(time(nullptr));
        ESP_LOGI("SklandApi", "refresh timestamp=%u local=%lld type=%s", static_cast<unsigned>(apiTimestamp),
                 static_cast<long long>(time(nullptr)), cJSON_IsString(timestamp) ? "string" : cJSON_IsNumber(timestamp) ? "number" : "fallback");
    }
    if (!ok && root) {
        cJSON *message = cJSON_GetObjectItem(root, "message");
        cJSON *code = cJSON_GetObjectItem(root, "code");
        if (cJSON_IsString(message)) errorValue = message->valuestring;
        else if (cJSON_IsNumber(code)) errorValue = "接口错误 " + String(code->valueint);
    }
    if (root) cJSON_Delete(root);
    return ok;
}

bool ensureRole() {
    if (!uidValue.isEmpty()) return true;
    cJSON *root = request("api/v1/game/player/binding", "GET", "", false, "");
    cJSON *data = root ? cJSON_GetObjectItem(root, "data") : nullptr;
    cJSON *list = data ? cJSON_GetObjectItem(data, "list") : nullptr;
    if (cJSON_IsArray(list)) {
        cJSON *item = nullptr;
        cJSON_ArrayForEach(item, list) {
            cJSON *appCode = cJSON_GetObjectItem(item, "appCode");
            if (!cJSON_IsString(appCode) || strcmp(appCode->valuestring, "arknights") != 0) continue;
            cJSON *defaultUid = cJSON_GetObjectItem(item, "defaultUid");
            if (cJSON_IsString(defaultUid)) uidValue = defaultUid->valuestring;
            else if (cJSON_IsNumber(defaultUid)) uidValue = String(static_cast<uint32_t>(defaultUid->valuedouble));
            cJSON *bindingList = cJSON_GetObjectItem(item, "bindingList");
            cJSON *first = cJSON_GetArrayItem(bindingList, 0);
            cJSON *nickname = first ? cJSON_GetObjectItem(first, "nickName") : nullptr;
            if (cJSON_IsString(nickname)) nameValue = nickname->valuestring;
            break;
        }
    }
    if (root) cJSON_Delete(root);
    if (!uidValue.isEmpty()) {
        Preferences storage;
        if (storage.begin("skland", false)) {
            storage.putString("uid", uidValue);
            if (!nameValue.isEmpty()) storage.putString("name", nameValue);
            storage.end();
        }
    }
    if (uidValue.isEmpty() && errorValue.isEmpty()) errorValue = "未找到明日方舟角色";
    return !uidValue.isEmpty();
}
}

bool loadCredentials() {
    Preferences storage;
    if (!storage.begin("skland", true)) return false;
    credValue = storage.getString("cred", "");
    accountValue = storage.getString("account", "");
    uidValue = storage.getString("uid", "");
    nameValue = storage.getString("name", "");
    storage.end();
    return !credValue.isEmpty();
}

bool hasSavedAccount() {
    Preferences storage;
    if (!storage.begin("skland", true)) return false;
    const bool saved = !storage.getString("account", "").isEmpty();
    storage.end();
    return saved;
}

void saveCredentials(const String &cred, const String &accountToken) {
    if (!credValue.isEmpty() && credValue != cred) { uidValue = ""; nameValue = ""; }
    credValue = cred; accountValue = accountToken;
    Preferences storage;
    if (storage.begin("skland", false)) {
        storage.putString("cred", credValue);
        if (!accountToken.isEmpty()) storage.putString("account", accountToken);
        storage.end();
    }
}

cJSON *get(const char *path, const String &query, const String &role, bool transformed) {
    if (credValue.isEmpty() && !loadCredentials()) { errorValue = "未保存登录信息"; return nullptr; }
    if (!refresh()) {
        // cred is short-lived. Keep the long-lived account token in NVS and
        // transparently exchange it for a new cred instead of asking the user
        // to scan again after every reboot or expiration.
        if (!renewCredFromAccount() || !refresh()) return nullptr;
    }
    if (!ensureRole()) return nullptr;
    String effectiveQuery = query;
    if (strcmp(path, "api/v1/game/player/info") == 0) effectiveQuery = "uid=" + uidValue;
    const String pathQuery = String(path) + (effectiveQuery.isEmpty() ? "" : "?" + effectiveQuery);
    cJSON *root = request(pathQuery, "GET", "", false, role, true, transformed);
    cJSON *code = root ? cJSON_GetObjectItem(root, "code") : nullptr;
    if (cJSON_IsNumber(code) && code->valueint != 0) {
        cJSON *message = cJSON_GetObjectItem(root, "message");
        errorValue = cJSON_IsString(message) ? String(message->valuestring) : "接口错误 " + String(code->valueint);
        cJSON_Delete(root);
        return nullptr;
    }
    return root;
}

// The body is already reduced by parseGarrisonCareerSummary(), so asking for the
// transformed variant only has to set the request header.
cJSON *getTransformed(const char *path, const String &query, const String &role) {
    return get(path, query, role, true);
}

String uid() { return uidValue; }
String playerName() { return nameValue; }
String lastError() { return errorValue; }
uint32_t serverTimestamp() { return apiTimestamp; }

bool downloadPortrait(const String &url, uint8_t **buffer, size_t *size) {
    if (!buffer || !size || (!url.startsWith("https://web.hycdn.cn/") && !url.startsWith("https://bbs.hycdn.cn/"))) return false;
    *buffer = nullptr; *size = 0;
    esp_http_client_config_t config = {};
    config.url = url.c_str(); config.crt_bundle_attach = esp_crt_bundle_attach; config.timeout_ms = 15000;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;
    bool ok = false;
    if (esp_http_client_open(client, 0) == ESP_OK && esp_http_client_fetch_headers(client) >= 0 &&
        esp_http_client_get_status_code(client) == 200) {
        constexpr size_t limit = 512 * 1024;
        const int64_t announced = esp_http_client_get_content_length(client);
        if (announced <= static_cast<int64_t>(limit)) {
            const size_t capacity = announced > 0 ? static_cast<size_t>(announced) : limit;
            uint8_t *out = static_cast<uint8_t *>(ps_malloc(capacity));
            if (!out) out = static_cast<uint8_t *>(malloc(capacity));
            if (out) {
                size_t used = 0;
                while (used < capacity) {
                    int n = esp_http_client_read(client, reinterpret_cast<char *>(out + used), static_cast<int>(capacity - used));
                    if (n <= 0) break;
                    used += static_cast<size_t>(n);
                }
                if (used > 8 && (announced <= 0 || used == static_cast<size_t>(announced))) { *buffer = out; *size = used; ok = true; }
                else free(out);
            }
        }
    }
    esp_http_client_cleanup(client);
    return ok;
}
}
