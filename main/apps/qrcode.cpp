#include "A_Config.h"
#include "Atmosphere.h"
#include <stdio.h>
#include <stdlib.h>

static lv_point_t qrcode_line_points[] = {{63, 3}, {256, 3}, {256, 196}, {63, 196}, {63, 3}};

namespace {
// Bringing up the radio and syncing the clock costs seconds; doing it in setup()
// would freeze the page switch, because AppManagerLite::loop() holds its mutex
// while an app is inside setup()/loop(). The probe runs on its own task and the
// UI is built when the answer is known.
portMUX_TYPE qrMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool qrNetworkBusy = false;
volatile bool qrNetworkDone = false;
volatile uint32_t qrGeneration = 0;
lv_obj_t *qrPrompt = nullptr;
bool qrUiBuilt = false;
uint32_t qrLastRefresh = 0;

constexpr const char *kApName = "SKYLOONG 4.0 Screen";

void qrNetworkTask(void *arg) {
    const uint32_t generation = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg));
    WiFi.begin();
    if (WiFi.waitForConnectResult(3000) == WL_CONNECTED) {
        if (hal.NTPSync()) {
            hal.time_sync = true;
        }
    }
    if (WiFi.getMode() == WIFI_OFF || (WiFi.getMode() == WIFI_STA && WiFi.isConnected() == false)) {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(kApName);
    }
    portENTER_CRITICAL(&qrMux);
    const bool live = (generation == qrGeneration);
    if (live) qrNetworkDone = true;
    qrNetworkBusy = false;
    portEXIT_CRITICAL(&qrMux);
    vTaskDelete(nullptr);
}

void buildQrUi(lv_obj_t *screen) {
    char data[100];
    hal.LOCKLV();
    if (qrPrompt) {
        lv_obj_del(qrPrompt);
        qrPrompt = nullptr;
    }
    if (WiFi.getMode() == WIFI_AP) {
        qrPrompt = lv_label_create(screen);
        lv_obj_set_style_text_align(qrPrompt, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(qrPrompt, &lv_font_chinese_16, false);
        lv_obj_set_style_text_color(qrPrompt, lv_color_white(), 0);
        lv_obj_set_size(qrPrompt, 300, 30);
        snprintf(data, sizeof(data), _tr(I18N_ID_CONNECT_SCREEN_AP), kApName);
        lv_label_set_text(qrPrompt, data);
        lv_obj_set_pos(qrPrompt, 10, 100);
    } else {
        lv_obj_t *line = lv_line_create(screen);
        lv_obj_set_style_line_width(line, 1, 0);
        lv_obj_set_style_line_color(line, lv_color_white(), 0);
        lv_line_set_points(line, qrcode_line_points, 5);

        lv_obj_t *qrcode_obj = lv_qrcode_create(screen, 190, lv_color_black(), lv_color_white());
        lv_obj_set_pos(qrcode_obj, 65, 5);
        snprintf(data, sizeof(data), "http://%s", WiFi.localIP().toString().c_str());
        lv_qrcode_update(qrcode_obj, data, strlen(data));

        lv_obj_t *lbl_prompt = lv_label_create(screen);
        lv_obj_set_style_text_align(lbl_prompt, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(lbl_prompt, &lv_font_chinese_16, false);
        lv_obj_set_style_text_color(lbl_prompt, lv_color_white(), 0);
        lv_obj_set_size(lbl_prompt, 300, 20);
        snprintf(data, sizeof(data), _tr(I18N_ID_CONNECTED_WIFI), WiFi.SSID().c_str());
        lv_label_set_text(lbl_prompt, data);
        lv_obj_set_pos(lbl_prompt, 10, 197);

        lv_obj_t *lbl_url = lv_label_create(screen);
        lv_obj_set_style_text_align(lbl_url, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(lbl_url, &lv_font_chinese_16, false);
        lv_obj_set_style_text_color(lbl_url, lv_color_white(), 0);
        lv_obj_set_size(lbl_url, 300, 20);
        snprintf(data, sizeof(data), "IP: %s", WiFi.localIP().toString().c_str());
        lv_label_set_text(lbl_url, data);
        lv_obj_set_pos(lbl_url, 10, 215);
    }
    hal.UNLOCKLV();
}
}  // namespace

void AppQRCode::setup() {
    hal.LOCKLV();
    atmosphereAttach(_appScreen, 0x0A1A24, 170);
    qrPrompt = lv_label_create(_appScreen);
    lv_obj_set_style_text_align(qrPrompt, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(qrPrompt, &lv_font_chinese_16, false);
    lv_obj_set_style_text_color(qrPrompt, lv_color_white(), 0);
    lv_obj_set_size(qrPrompt, 300, 30);
    lv_label_set_text(qrPrompt, "正在连接网络...");
    lv_obj_center(qrPrompt);
    hal.UNLOCKLV();

    uint32_t generation;
    portENTER_CRITICAL(&qrMux);
    qrUiBuilt = false;
    qrNetworkDone = false;
    qrNetworkBusy = true;
    generation = qrGeneration;
    portEXIT_CRITICAL(&qrMux);
    // A fresh page entry invalidates nothing; the generation only guards teardown.
    xTaskCreate(qrNetworkTask, "qr_net", 6144,
                reinterpret_cast<void *>(static_cast<uintptr_t>(generation)), 2, nullptr);
}

void AppQRCode::loop() {
    if (!qrUiBuilt && qrNetworkDone) {
        qrUiBuilt = true;
        hal.start_webserver();
        buildQrUi(_appScreen);
    }
    if (qrUiBuilt && millis() - qrLastRefresh > 30000) {
        qrLastRefresh = millis();
        hal.LOCKLV();
        atmosphereRefresh();
        hal.UNLOCKLV();
    }
    if (hal.config_wifi) {
        WiFiMgr.add(hal.ssid, hal.password);
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.begin(hal.ssid, hal.password);
        GUI::toast(_tr(I18N_ID_CONNECTING));
        if (WiFi.waitForConnectResult(4000) == WL_CONNECTED) {
            if (hal.NTPSync()) {
                hal.time_sync = true;
            }
            hal.send_sysctl(EVENT_EXIT_QRCODE);
        } else {
            GUI::toast(_tr(I18N_ID_CONNECT_FAILED));
            WiFiMgr.remove(hal.ssid);
            WiFi.mode(WIFI_AP);
            WiFi.softAP(kApName);
        }
        hal.config_wifi = false;
    }
}

void AppQRCode::destroy() {
    portENTER_CRITICAL(&qrMux);
    qrGeneration = qrGeneration + 1;  // discard a probe that is still in flight
    portEXIT_CRITICAL(&qrMux);
    atmosphereDetach();
    qrPrompt = nullptr;
    qrUiBuilt = false;
    qrNetworkDone = false;
}
