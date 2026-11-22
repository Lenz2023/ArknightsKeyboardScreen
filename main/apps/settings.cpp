#include "A_Config.h"
#include "Atmosphere.h"
#include "nvs_flash.h"

lv_obj_t *create_settings_switch(lv_obj_t *parent, const char *title, const char *str, lv_event_cb_t cb)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_width(box, lv_pct(95));
    lv_obj_set_height(box, LV_SIZE_CONTENT);

    lv_obj_t *lbl_setting_title = lv_label_create(box);
    lv_label_set_text(lbl_setting_title, title);
    lv_obj_align(lbl_setting_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *lbl_setting_desc = lv_label_create(box);
    lv_label_set_text(lbl_setting_desc, str);
    lv_obj_align_to(lbl_setting_desc, lbl_setting_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_label_set_long_mode(lbl_setting_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_setting_desc, lv_pct(70));
    lv_obj_set_height(lbl_setting_desc, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(lbl_setting_desc, lv_palette_main(LV_PALETTE_GREY), 0);

    lv_obj_t *sw_btn = lv_switch_create(box);
    lv_obj_align(sw_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(sw_btn, cb, LV_EVENT_ALL, NULL);
    return sw_btn;
}

lv_obj_t *create_settings_list(lv_obj_t *parent, const char *title, const char *str, const char *dropdown, lv_event_cb_t cb)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_width(box, lv_pct(95));
    lv_obj_set_height(box, LV_SIZE_CONTENT);

    lv_obj_t *lbl_setting_title = lv_label_create(box);
    lv_label_set_text(lbl_setting_title, title);
    lv_obj_align(lbl_setting_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *lbl_setting_desc = lv_label_create(box);
    lv_label_set_text(lbl_setting_desc, str);
    lv_obj_align_to(lbl_setting_desc, lbl_setting_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_label_set_long_mode(lbl_setting_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_setting_desc, lv_pct(60));
    lv_obj_set_height(lbl_setting_desc, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(lbl_setting_desc, lv_palette_main(LV_PALETTE_GREY), 0);

    lv_obj_t *lst = lv_dropdown_create(box);
    lv_dropdown_set_options(lst, dropdown);
    lv_obj_set_width(lst, lv_pct(40));
    lv_obj_align(lst, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(lst, cb, LV_EVENT_ALL, NULL);
    return lst;
}

lv_obj_t *create_settings_button(lv_obj_t *parent, const char *title, const char *str, const char *btn_str, lv_event_cb_t cb)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_width(box, lv_pct(95));
    lv_obj_set_height(box, LV_SIZE_CONTENT);

    lv_obj_t *lbl_setting_title = lv_label_create(box);
    lv_label_set_text(lbl_setting_title, title);
    lv_obj_align(lbl_setting_title, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *lbl_setting_desc = lv_label_create(box);
    lv_label_set_text(lbl_setting_desc, str);
    lv_obj_align_to(lbl_setting_desc, lbl_setting_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_label_set_long_mode(lbl_setting_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl_setting_desc, lv_pct(70));
    lv_obj_set_height(lbl_setting_desc, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(lbl_setting_desc, lv_palette_main(LV_PALETTE_GREY), 0);

    lv_obj_t *btn = lv_btn_create(box);
    lv_obj_align(btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_ALL, NULL);
    lv_obj_t *lbl_btn = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl_btn, &lv_font_chinese_16, 0);
    lv_label_set_text(lbl_btn, btn_str);
    lv_obj_center(lbl_btn);
    return btn;
}

void create_settings_slider(lv_obj_t *_appScreen, const char *title, int32_t value, lv_event_cb_t cb)
{
    lv_obj_t *box = lv_obj_create(_appScreen);
    lv_obj_set_size(box, lv_pct(95), lv_pct(30));
    lv_obj_center(box);

    lv_obj_t *lbl_title = lv_label_create(box);
    lv_label_set_text(lbl_title, title);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *slider = lv_slider_create(box);
    lv_obj_set_width(slider, lv_pct(80));
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_slider_set_range(slider, 0, 9);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, cb, LV_EVENT_ALL, NULL);
}

static bool server_req = false;
static bool ntp_req = false;
static bool rechoose_wifi = false;
static bool skland_relogin = false;
static int factory_reset_count = 20;
static bool reboot_needed = false;
static uint32_t factory_reset_millis_last = 0;
static lv_obj_t *factory_reset_btn = NULL;
static lv_obj_t *btn_server = NULL;
// Auto power-off choices, 0 = never.
static const uint16_t kSleepMinutes[] = {10, 30, 60, 120, 240, 480, 0};
static const char *kSleepOptions = "10 分钟\n30 分钟\n1 小时\n2 小时\n4 小时\n8 小时\n不关机";
lv_obj_t *exit_btn = NULL;

void AppSettings::setup()
{
    hal.LOCKLV();
    lv_obj_t *o;
    // Shared weather scenery so settings sits in the same shell as the rest.
    // The veil is heavy because this page relies on the theme's grey captions.
    atmosphereAttach(_appScreen, 0x0A1A24, 190);
    lv_group_set_wrap(lv_group_get_default(), false);
    lv_obj_set_flex_flow(_appScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_appScreen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_text_font(_appScreen, &lv_font_chinese_16, 0);
    o = create_settings_list(_appScreen, "开机首页", "下次开机时显示", "卫戍协议\n时钟\n天气\n音乐\n博士档案\n日常数据", [](lv_event_t *e) {
        if (e->code == LV_EVENT_VALUE_CHANGED)
            hal.pref.putUChar("boot_page", lv_dropdown_get_selected(e->target));
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF);
    });
    lv_dropdown_set_selected(o, appManagerLite.homePage());
    lv_obj_set_style_text_font(lv_dropdown_get_list(o), &lv_font_chinese_16, 0);
    o = create_settings_list(_appScreen, "自动关机", "无键盘输入多久后熄屏", kSleepOptions, [](lv_event_t *e) {
        if (e->code == LV_EVENT_VALUE_CHANGED) {
            const uint16_t minutes = kSleepMinutes[lv_dropdown_get_selected(e->target)];
            hal.sleep_minutes = minutes;
            hal.pref.putUShort("sleep_min", minutes);
        }
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF);
    });
    {
        int selected = 6;
        for (int i = 0; i < 7; ++i)
            if (kSleepMinutes[i] == hal.sleep_minutes) { selected = i; break; }
        lv_dropdown_set_selected(o, selected);
    }
    lv_obj_set_style_text_font(lv_dropdown_get_list(o), &lv_font_chinese_16, 0);
    create_settings_button(_appScreen, "重新登录森空岛", "清除保存的登录信息并重新扫码", "重新登录", [](lv_event_t *e) {
        if (e->code == LV_EVENT_CLICKED) skland_relogin = true;
        if (e->code == LV_EVENT_FOCUSED) lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF);
    });

    o = create_settings_button(_appScreen, _tr(I18N_ID_RECHOOSE_WIFI), _tr(I18N_ID_RECHOOSE_WIFI_DESC), _tr(I18N_ID_RECHOOSE), [](lv_event_t *e)
                               {
        if (e->code == LV_EVENT_CLICKED)
            rechoose_wifi = true; 
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });

    o = create_settings_switch(_appScreen, _tr(I18N_ID_12HR), _tr(I18N_ID_12HR_DESC), [](lv_event_t *e)
                               {
        if (e->code == LV_EVENT_VALUE_CHANGED)
            hal.config_time_12hr = lv_obj_has_state((lv_obj_t *)lv_event_get_target(e), LV_STATE_CHECKED);
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });
    if (hal.config_time_12hr)
        lv_obj_add_state(o, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(o, LV_STATE_CHECKED);

    btn_server = create_settings_button(_appScreen, _tr(I18N_ID_START_WEBSERVER), _tr(I18N_ID_START_WEBSERVER_DESC), _tr(I18N_ID_START), [](lv_event_t *e)
    {
        if (e->code == LV_EVENT_VALUE_CHANGED)
        {
            if(lv_obj_has_state((lv_obj_t *)lv_event_get_target(e), LV_STATE_CHECKED))
            {
                lv_obj_add_state(btn_server, LV_STATE_DISABLED);
                server_req = true;
            }
            else
            {
                lv_label_set_text(lv_obj_get_child(lv_event_get_target(e), 0), _tr(I18N_ID_START));
                hal.send_sysctl(EVENT_SERVERCTL, 0);
            }
        }
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });
    lv_obj_add_flag(btn_server, LV_OBJ_FLAG_CHECKABLE);
    if (hal.server_started)
    {
        lv_obj_add_state(btn_server, LV_STATE_CHECKED);
    }

    create_settings_button(_appScreen, _tr(I18N_ID_SYNC_TIME), _tr(I18N_ID_SYNC_TIME_DESC), _tr(I18N_ID_SYNC), [](lv_event_t *e)
                           {
        if (e->code == LV_EVENT_CLICKED)
            ntp_req = true;
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });

    create_settings_slider(_appScreen, _tr(I18N_ID_BRIGHTNESS), hal._brightness, [](lv_event_t *e) {
        if (e->code == LV_EVENT_VALUE_CHANGED)
            hal.setBrightness(lv_slider_get_value((lv_obj_t*)lv_event_get_target(e))); 
        if(e->code == LV_EVENT_SCREEN_LOADED)
        {
            lv_slider_set_value((lv_obj_t*)lv_event_get_target(e), hal._brightness, LV_ANIM_OFF);
        }
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF);
    });

    o = create_settings_list(_appScreen, "Language", "修改语言", "中文\nEnglish", [](lv_event_t *e)
                             {
        if (e->code == LV_EVENT_VALUE_CHANGED)
            i18n::setLanguage(lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e)));
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });
    lv_dropdown_set_selected(o, i18n::getLanguage());

    o = create_settings_list(_appScreen, _tr(I18N_ID_TIMEZONE), _tr(I18N_ID_TIMEZONE_DESC), "UTC-12\nUTC-11\nUTC-10\nUTC-9\nUTC-8\nUTC-7\nUTC-6\nUTC-5\nUTC-4\nUTC-3\nUTC-2\nUTC-1\nUTC+0\nUTC+1\nUTC+2\nUTC+3\nUTC+4\nUTC+5\nUTC+6\nUTC+7\nUTC+8\nUTC+9\nUTC+10\nUTC+11\nUTC+12", [](lv_event_t *e)
                             {

        if (e->code == LV_EVENT_VALUE_CHANGED)
        {
            long t = lv_dropdown_get_selected((lv_obj_t*)lv_event_get_target(e));
            t -= 12;
            t *= 3600;
            i18n::setNTPOffset(t);
            ntp_req = true;
        }
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });
    {
        long t = i18n::getNTPOffset();
        t /= 3600;
        t += 12;
        lv_dropdown_set_selected(o, t);
    }

    if (i18n::getLanguage() == 0) {
        o = create_settings_switch(_appScreen, _tr(I18N_ID_ENABLE_WEATHER_APP), _tr(I18N_ID_ENABLE_WEATHER_APP_DESC), [](lv_event_t *e)
                                {
            if (e->code == LV_EVENT_VALUE_CHANGED){
                reboot_needed = true;
                hal.pref.putBool("weather_enable", lv_obj_has_state((lv_obj_t *)lv_event_get_target(e), LV_STATE_CHECKED));
            }
            if (e->code == LV_EVENT_FOCUSED)
                lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });
        if (hal.pref.getBool("weather_enable", true))
            lv_obj_add_state(o, LV_STATE_CHECKED);
        else
            lv_obj_clear_state(o, LV_STATE_CHECKED);
    }

    factory_reset_btn = create_settings_button(_appScreen, _tr(I18N_ID_FACTORY_RESET), _tr(I18N_ID_FACTORY_RESET_DESC), _tr(I18N_ID_FACTORY_RESET_BTN), [](lv_event_t *e)
                                               {
        if (e->code == LV_EVENT_CLICKED)
        {
            factory_reset_millis_last = millis();
            factory_reset_count--;
            if(factory_reset_count == 0)
            {
                lv_label_set_text(lv_obj_get_child(factory_reset_btn, 0), _tr(I18N_ID_FACTORY_RESETTING));
            }
            else if(factory_reset_count > 0)
            {
                lv_label_set_text_fmt(lv_obj_get_child(factory_reset_btn, 0), _tr(I18N_ID_FACTORY_RESETTING_COUNT), factory_reset_count);
            }
        }
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });
    lv_obj_add_state(factory_reset_btn, LV_STATE_CHECKED);

    exit_btn = create_settings_button(_appScreen, _tr(I18N_ID_EXIT_TITLE), _tr(I18N_ID_EXIT_DESC), _tr(I18N_ID_EXIT), [](lv_event_t *e)
                                      {
            if (e->code == LV_EVENT_CLICKED)
            {
                hal.forceExitSettings();
            }
        if (e->code == LV_EVENT_FOCUSED)
            lv_obj_scroll_to_view(e->target->parent, LV_ANIM_OFF); });
    lv_obj_add_state(exit_btn, LV_STATE_CHECKED);

    hal.UNLOCKLV();
}

uint32_t last_millis = 0;

void AppSettings::loop()
{
    if (millis() - last_millis > 300)
    {
        last_millis = millis();
        hal.LOCKLV();
        atmosphereRefresh();
        hal.UNLOCKLV();
        if (hal.server_started)
        {
            if (WiFi.getMode() == WIFI_AP)
                lv_label_set_text(lv_obj_get_child(btn_server, 0), "192.168.4.1");
            else if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED)
                lv_label_set_text(lv_obj_get_child(btn_server, 0), WiFi.localIP().toString().c_str());
            else
                lv_label_set_text(lv_obj_get_child(btn_server, 0), _tr(I18N_ID_STOP));
        }
        else
        {
            lv_label_set_text(lv_obj_get_child(btn_server, 0), _tr(I18N_ID_START));
        }
    }
    if (rechoose_wifi)
    {
        WiFi.disconnect(true);
        WiFiMgr.requireWiFi(true);
        rechoose_wifi = false;
    }
    if (skland_relogin)
    {
        skland_relogin = false;
        Preferences storage;
        if (storage.begin("skland", false)) {
            storage.clear();
            storage.end();
        }
        // Settings::loop runs while AppManagerLite owns its app mutex.  Request
        // the switch for after this loop returns to avoid locking the mutex twice.
        appManagerLite.requestApp(appManagerLite.appSklandLogin);
        return;
    }
    if (server_req)
    {
        WiFiMgr.requireWiFi();
        hal.send_sysctl(EVENT_SERVERCTL, 1);
        lv_obj_clear_state(btn_server, LV_STATE_DISABLED);
        server_req = false;
    }
    if (ntp_req)
    {
        if (WiFiMgr.requireWiFi())
        {
            if (hal.NTPSync()) {
                GUI::toast(_tr(I18N_ID_SYNC_SUCCESS));
                hal.time_sync = true;
            } else {
                GUI::toast(_tr(I18N_ID_SYNC_FAILED));
            }
        }
        ntp_req = false;
    }
    if (factory_reset_count != 20)
    {
        if (millis() - factory_reset_millis_last > 1000)
        {
            factory_reset_millis_last = 0;
            factory_reset_count = 20;
            lv_label_set_text(lv_obj_get_child(factory_reset_btn, 0), _tr(I18N_ID_FACTORY_RESET_BTN));
        }
    }
    if (factory_reset_count <= 0)
    {
        hal.forceExitSettings();
        hal.LOCKLV();
        hal.pref.end();
        LittleFS.end();
        nvs_flash_erase();
        LittleFS.format();
        nvs_flash_init();
        if (LittleFS.begin(false)) {
            hal.pref.begin("settings", false);
            hal.pref.putUInt("lang", i18n::getLanguage());
        }
        delay(1000);
        ESP.restart();
    }
}

void AppSettings::destroy()
{
    atmosphereDetach();
    if (hal.server_started)
    {
        GUI::toast(_tr(I18N_ID_WEBSERVER_RUNNING_BACKGROUND));
    }
    else
    {
        WiFi.disconnect(true);
    }
    hal.pref.putUInt("bright", hal._brightness);
    hal.pref.putBool("12hr", hal.config_time_12hr);
    hal.pref.putUInt("lang", i18n::getLanguage());
    hal.pref.putInt("ntp", i18n::getNTPOffset());
    if (reboot_needed)
    {
        hal.forceExitSettings();
        delay(2000);
        ESP.restart();
    }
}
