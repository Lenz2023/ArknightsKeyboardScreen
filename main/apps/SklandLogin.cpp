#include "A_Config.h"
#include "Atmosphere.h"
#include <cJSON.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include "SklandApi.h"

namespace {
constexpr const char *kAppCode = "4ca99fa6b56cc2ba";
constexpr uint32_t kRefreshMs = 4UL * 60UL * 1000UL;
constexpr uint32_t kPollMs = 5000;

lv_obj_t *qr = nullptr;
lv_obj_t *qrCard = nullptr;
lv_obj_t *title = nullptr;
lv_obj_t *statusLabel = nullptr;
lv_obj_t *screenRoot = nullptr;
uint32_t lastQrAt = 0;
uint32_t lastPollAt = 0;
String scanId;         // main-loop owned
String savedToken;     // main-loop owned
bool waiting = false;  // main-loop owned
bool loggedIn = false; // main-loop owned
uint32_t lastRestoreAt = 0;
uint32_t lastAtmosphereRefresh = 0;

cJSON *requestJson(const char *url, const char *method, const char *body = nullptr) {
    esp_http_client_config_t config = {};
    config.url = url;
    config.crt_bundle_attach = esp_crt_bundle_attach;
    config.timeout_ms = 8000;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return nullptr;
    if (body) {
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json;charset=UTF-8");
        esp_http_client_set_post_field(client, body, strlen(body));
    } else if (strcmp(method, "GET") == 0) {
        esp_http_client_set_method(client, HTTP_METHOD_GET);
    }
    cJSON *result = nullptr;
    if (esp_http_client_open(client, body ? strlen(body) : 0) == ESP_OK &&
        (!body || esp_http_client_write(client, body, strlen(body)) == static_cast<int>(strlen(body))) &&
        esp_http_client_fetch_headers(client) >= 0 &&
        esp_http_client_get_status_code(client) == 200) {
        constexpr size_t capacity = 4096;
        char *buffer = static_cast<char *>(malloc(capacity));
        if (buffer) {
            const int received = esp_http_client_read_response(client, buffer, capacity - 1);
            if (received > 0) {
                buffer[received] = 0;
                result = cJSON_Parse(buffer);
            }
            free(buffer);
        }
    }
    esp_http_client_cleanup(client);
    return result;
}

void setStatus(const char *text) {
    if (!statusLabel) return;
    hal.LOCKLV();
    lv_label_set_text(statusLabel, text);
    hal.UNLOCKLV();
}

// ---- network-only helpers (run inside the worker task; no LVGL, no shared globals) ----

// Generate a login QR; fills outScanId/outScanUrl; returns success.
static bool requestQr(String &outScanId, String &outScanUrl) {
    cJSON *root = requestJson("https://as.hypergryph.com/general/v1/gen_scan/login", "POST",
                              "{\"appCode\":\"4ca99fa6b56cc2ba\"}");
    cJSON *data = root ? cJSON_GetObjectItem(root, "data") : nullptr;
    cJSON *id = data ? cJSON_GetObjectItem(data, "scanId") : nullptr;
    cJSON *url = data ? cJSON_GetObjectItem(data, "scanUrl") : nullptr;
    const bool ok = cJSON_IsString(id) && cJSON_IsString(url);
    if (ok) { outScanId = id->valuestring; outScanUrl = url->valuestring; }
    if (root) cJSON_Delete(root);
    return ok;
}

// Run the full credential exchange. `token` (account token) or `scanCode` is required.
// Persists the account token (Preferences) and cred (SklandApi) on success.
// Returns true and fills outToken with the account token on success.
static bool doExchange(const char *token, const char *scanCode, String &outToken) {
    String accountToken;
    if (token && token[0]) {
        accountToken = token;
    } else if (scanCode && scanCode[0]) {
        char body[512];
        snprintf(body, sizeof(body), "{\"scanCode\":\"%s\",\"appCode\":\"%s\"}", scanCode, kAppCode);
        cJSON *tokenRoot = requestJson("https://as.hypergryph.com/user/auth/v1/token_by_scan_code", "POST", body);
        cJSON *tokenData = tokenRoot ? cJSON_GetObjectItem(tokenRoot, "data") : nullptr;
        cJSON *returnedToken = tokenData ? cJSON_GetObjectItem(tokenData, "token") : nullptr;
        if (cJSON_IsString(returnedToken)) accountToken = returnedToken->valuestring;
        if (tokenRoot) cJSON_Delete(tokenRoot);
    }
    if (accountToken.isEmpty()) return false;

    Preferences storage;
    if (storage.begin("skland", false)) {
        if (storage.getString("account", "") != accountToken)
            storage.putString("account", accountToken);
        storage.end();
    }

    char grantBody[768];
    snprintf(grantBody, sizeof(grantBody), "{\"token\":\"%s\",\"appCode\":\"%s\",\"type\":0}",
             accountToken.c_str(), kAppCode);
    cJSON *grantRoot = requestJson("https://as.hypergryph.com/user/oauth2/v2/grant", "POST", grantBody);
    cJSON *grantData = grantRoot ? cJSON_GetObjectItem(grantRoot, "data") : nullptr;
    cJSON *code = grantData ? cJSON_GetObjectItem(grantData, "code") : nullptr;
    if (!cJSON_IsString(code)) {
        if (grantRoot) cJSON_Delete(grantRoot);
        return false;
    }

    char credBody[768];
    snprintf(credBody, sizeof(credBody), "{\"kind\":1,\"code\":\"%s\"}", code->valuestring);
    cJSON *credRoot = requestJson("https://zonai.skland.com/web/v1/user/auth/generate_cred_by_code", "POST", credBody);
    cJSON *credData = credRoot ? cJSON_GetObjectItem(credRoot, "data") : nullptr;
    cJSON *cred = credData ? cJSON_GetObjectItem(credData, "cred") : nullptr;
    cJSON *sign = credData ? cJSON_GetObjectItem(credData, "token") : nullptr;
    const bool ok = cJSON_IsString(cred) && cJSON_IsString(sign);
    if (ok) {
        outToken = accountToken;
        SklandApi::saveCredentials(cred->valuestring, accountToken);
    }
    if (credRoot) cJSON_Delete(credRoot);
    if (grantRoot) cJSON_Delete(grantRoot);
    return ok;
}

// Poll scan status. Returns 0 = still pending, 1 = logged in, 2 = exchange failed.
static int requestPoll(const String &sid, String &outToken) {
    if (sid.isEmpty()) return 0;
    String url = "https://as.hypergryph.com/general/v1/scan_status?scanId=" + sid;
    cJSON *root = requestJson(url.c_str(), "GET");
    cJSON *data = root ? cJSON_GetObjectItem(root, "data") : nullptr;
    cJSON *token = data ? cJSON_GetObjectItem(data, "token") : nullptr;
    cJSON *scanCode = data ? cJSON_GetObjectItem(data, "scanCode") : nullptr;
    int result = 0;
    if (cJSON_IsString(token) || cJSON_IsString(scanCode)) {
        const char *tok = cJSON_IsString(token) ? token->valuestring : nullptr;
        const char *code = cJSON_IsString(scanCode) ? scanCode->valuestring : nullptr;
        result = doExchange(tok, code, outToken) ? 1 : 2;
    }
    if (root) cJSON_Delete(root);
    return result;
}

// ---- worker + mailbox (mirrors the garrison/doctor async pattern) ----
enum LoginKind {
    kLoginNone = 0,
    kLoginQrReady = 1,
    kLoginQrFailed = 2,
    kLoginPollPending = 3,
    kLoginPollLoggedIn = 4,
    kLoginPollFailed = 5,
    kLoginRestoreLoggedIn = 6,
    kLoginRestoreFailed = 7,
};
enum LoginOp { kLoginCreateQr = 1, kLoginPoll = 2, kLoginRestore = 3 };
struct LoginRequest { uint8_t op; char scanId[64]; char token[256]; };
struct LoginResult { uint8_t kind; char scanId[64]; char scanUrl[512]; char accountToken[256]; };

portMUX_TYPE loginMux = portMUX_INITIALIZER_UNLOCKED;
LoginRequest loginReq{};
LoginResult loginResult{};
volatile bool loginReqReady = false;
volatile bool loginResultReady = false;
volatile bool loginWorkerBusy = false;
volatile uint32_t loginGeneration = 0;
TaskHandle_t loginWorker = nullptr;

static void queueLoginOp(uint8_t op, const char *sid = nullptr, const char *tok = nullptr) {
    portENTER_CRITICAL(&loginMux);
    loginReq.op = op;
    if (sid && *sid) strlcpy(loginReq.scanId, sid, sizeof(loginReq.scanId)); else loginReq.scanId[0] = 0;
    if (tok && *tok) strlcpy(loginReq.token, tok, sizeof(loginReq.token)); else loginReq.token[0] = 0;
    loginReqReady = true;
    portEXIT_CRITICAL(&loginMux);
}

static void loginWorkerEntry(void *) {
    for (;;) {
        portENTER_CRITICAL(&loginMux);
        if (!loginReqReady) { portEXIT_CRITICAL(&loginMux); vTaskDelay(pdMS_TO_TICKS(100)); continue; }
        LoginRequest req = loginReq;
        loginReqReady = false;
        loginWorkerBusy = true;
        const uint32_t gen = loginGeneration;
        portEXIT_CRITICAL(&loginMux);

        // Ensure a working Wi-Fi link before the HTTPS round-trip (blocking is fine here).
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.begin();
            WiFi.waitForConnectResult(5000);
        }

        LoginResult res{}; res.kind = kLoginNone;
        String outToken;
        switch (req.op) {
        case kLoginRestore:
            res.kind = doExchange(req.token, nullptr, outToken) ? kLoginRestoreLoggedIn : kLoginRestoreFailed;
            break;
        case kLoginCreateQr: {
            String id, url;
            if (requestQr(id, url)) {
                res.kind = kLoginQrReady;
                strlcpy(res.scanId, id.c_str(), sizeof(res.scanId));
                strlcpy(res.scanUrl, url.c_str(), sizeof(res.scanUrl));
            } else {
                res.kind = kLoginQrFailed;
            }
            break;
        }
        case kLoginPoll: {
            const int r = requestPoll(String(req.scanId), outToken);
            res.kind = r == 1 ? kLoginPollLoggedIn : r == 2 ? kLoginPollFailed : kLoginPollPending;
            break;
        }
        }
        if (outToken.length()) strlcpy(res.accountToken, outToken.c_str(), sizeof(res.accountToken));

        portENTER_CRITICAL(&loginMux);
        if (gen == loginGeneration) {   // discard results from a page that was torn down
            loginResult = res;
            loginResultReady = true;
        }
        loginWorkerBusy = false;
        portEXIT_CRITICAL(&loginMux);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static bool takeLoginResult(LoginResult &out) {
    portENTER_CRITICAL(&loginMux);
    const bool ready = loginResultReady;
    if (ready) { out = loginResult; loginResultReady = false; }
    portEXIT_CRITICAL(&loginMux);
    return ready;
}

static bool loginWorkerIdle() {
    portENTER_CRITICAL(&loginMux);
    const bool idle = !loginWorkerBusy;
    portEXIT_CRITICAL(&loginMux);
    return idle;
}

static void ensureLoginWorker() {
    if (loginWorker == nullptr)
        xTaskCreatePinnedToCore(loginWorkerEntry, "skland_login", 10240, nullptr, 2, &loginWorker, 0);
}

static void renderQr(const char *scanUrl) {
    hal.LOCKLV();
    if (qr) {
        lv_obj_del(qr);
        qr = nullptr;
    }
    if (qrCard) {
        lv_obj_del(qrCard);
        qrCard = nullptr;
    }
    // The scenery sits under a veil, so the code gets its own opaque card: the
    // widget has to stay on top of the atmosphere layers, never behind them.
    qrCard = lv_obj_create(screenRoot);
    lv_obj_remove_style_all(qrCard);
    lv_obj_set_pos(qrCard, 78, 32);
    lv_obj_set_size(qrCard, 164, 160);
    lv_obj_set_style_radius(qrCard, 8, 0);
    lv_obj_set_style_bg_color(qrCard, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(qrCard, LV_OPA_COVER, 0);
    lv_obj_clear_flag(qrCard, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(qrCard, LV_OBJ_FLAG_CLICKABLE);
    qr = lv_qrcode_create(screenRoot, 148, lv_color_black(), lv_color_white());
    lv_obj_set_pos(qr, 86, 38);
    lv_qrcode_update(qr, scanUrl, strlen(scanUrl));
    lv_obj_move_foreground(qr);
    hal.UNLOCKLV();
}
} // namespace

void AppSklandLogin::setup() {
    screenRoot = _appScreen;
    loggedIn = false;
    waiting = false;
    scanId = "";
    lastQrAt = 0;
    lastPollAt = 0;
    lastRestoreAt = millis() - 30000;   // force an immediate restore attempt
    hal.LOCKLV();
    lv_obj_set_style_bg_color(_appScreen, lv_color_hex(0x0F2230), 0);
    lv_obj_set_style_bg_opa(_appScreen, LV_OPA_COVER, 0);
    // Same weather scenery as the rest of the shell. The veil is stronger here
    // because the QR card is white and needs a calm field around it.
    atmosphereAttach(_appScreen, 0x0B1B26, 150);
    title = lv_label_create(_appScreen);
    lv_obj_set_style_text_font(title, &lv_font_chinese_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xE8F4F7), 0);
    lv_label_set_text(title, "森空岛  /  LOGIN");
    lv_obj_set_pos(title, 18, 10);
    statusLabel = lv_label_create(_appScreen);
    lv_obj_set_style_text_font(statusLabel, &lv_font_chinese_16, 0);
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xC3D3D8), 0);
    lv_obj_set_width(statusLabel, 284);
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(statusLabel, "正在获取二维码...");
    lv_obj_set_pos(statusLabel, 18, 200);
    hal.UNLOCKLV();

    Preferences storage;
    savedToken = "";
    if (storage.begin("skland", true)) {
        savedToken = storage.getString("account", "");
        storage.end();
    }

    portENTER_CRITICAL(&loginMux);
    ++loginGeneration;        // invalidate any in-flight result from a previous page instance
    loginReqReady = false;
    loginResultReady = false;
    portEXIT_CRITICAL(&loginMux);
    ensureLoginWorker();

    if (savedToken.isEmpty()) setStatus("正在获取二维码...");
    else setStatus("正在恢复登录...");
}

void AppSklandLogin::loop() {
    LoginResult res;
    if (takeLoginResult(res)) {
        switch (res.kind) {
        case kLoginQrReady:
            scanId = res.scanId;
            renderQr(res.scanUrl);
            waiting = true;
            lastQrAt = millis();
            lastPollAt = millis();
            setStatus("请用森空岛 App 扫码");
            break;
        case kLoginQrFailed:
            waiting = false;
            setStatus("二维码获取失败，稍后重试");
            break;
        case kLoginPollPending:
            break;
        case kLoginPollLoggedIn:
        case kLoginRestoreLoggedIn:
            if (res.accountToken[0]) savedToken = res.accountToken;
            loggedIn = true;
            setStatus("已登录森空岛");
            break;
        case kLoginPollFailed:
            setStatus("扫码成功，但凭证交换失败");
            break;
        case kLoginRestoreFailed:
            setStatus("恢复失败，将重试；可在设置重新登录");
            break;
        }
        if (loggedIn) { appManagerLite.openSklandGeneral(); return; }
    }

    const uint32_t now = millis();
    if (screenRoot && now - lastAtmosphereRefresh > 30000) {
        lastAtmosphereRefresh = now;
        hal.LOCKLV();
        atmosphereRefresh();
        hal.UNLOCKLV();
    }
    if (loginWorkerIdle()) {
        if (!loggedIn && !savedToken.isEmpty() && now - lastRestoreAt >= 30000) {
            lastRestoreAt = now;
            queueLoginOp(kLoginRestore, nullptr, savedToken.c_str());
        } else if (!loggedIn && savedToken.isEmpty() && (!waiting || now - lastQrAt >= kRefreshMs)) {
            lastQrAt = now;
            queueLoginOp(kLoginCreateQr);
        } else if (!loggedIn && waiting && now - lastPollAt >= kPollMs) {
            lastPollAt = now;
            queueLoginOp(kLoginPoll, scanId.c_str());
        }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
}

void AppSklandLogin::destroy() {
    atmosphereDetach();
    portENTER_CRITICAL(&loginMux);
    ++loginGeneration;        // discard any result still in flight
    const bool idle = !loginWorkerBusy;
    TaskHandle_t handle = loginWorker;
    if (idle) {
        loginWorker = nullptr;
        loginReqReady = false;
        loginResultReady = false;
    }
    portEXIT_CRITICAL(&loginMux);
    if (idle && handle) vTaskDelete(handle);

    qr = nullptr;
    qrCard = nullptr;
    title = nullptr;
    statusLabel = nullptr;
    scanId = "";
    waiting = false;
    loggedIn = false;
}
