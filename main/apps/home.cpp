#include "A_Config.h"
#include "Atmosphere.h"
#include "ArtStore.h"
#include <stdio.h>
#include <stdlib.h>
extern "C" int weatherHomeTemperature();
extern "C" int weatherHomeCode();
extern "C" const char *weatherHomeCondition();
extern "C" const char *weatherHomeLocation();

extern "C" const lv_font_t font_rajdhani_16;
extern "C" const lv_font_t font_rajdhani_32;
extern "C" const lv_font_t font_rajdhani_40;
extern "C" uint32_t weatherHomeRevision();
extern void weatherHomeServiceSetActive(bool active);
extern const lv_img_dsc_t *weatherImages_s[];
extern const lv_img_dsc_t *weatherImages_l[];

#if 0 // Removed legacy selectable themes; keep only the unified default UI.
namespace theme_fox
{
    extern "C" const lv_img_dsc_t img_main1_bg;
    extern "C" const lv_img_dsc_t battery_frame_1;
    extern "C" const lv_img_dsc_t battery_charging_1;
    extern "C" const lv_img_dsc_t img_bt1_1;
    extern "C" const lv_img_dsc_t img_bt1_2;
    extern "C" const lv_img_dsc_t img_bt2_1;
    extern "C" const lv_img_dsc_t img_bt2_2;
    extern "C" const lv_img_dsc_t img_caplock1_1;
    extern "C" const lv_img_dsc_t img_caplock1_2;
    extern "C" const lv_img_dsc_t img_mac1_1;
    extern "C" const lv_img_dsc_t img_mac1_2;
    extern "C" const lv_img_dsc_t img_numlock1_1;
    extern "C" const lv_img_dsc_t img_numlock1_2;
    extern "C" const lv_img_dsc_t img_rf1_1;
    extern "C" const lv_img_dsc_t img_rf1_2;
    extern "C" const lv_img_dsc_t img_sllock1_1;
    extern "C" const lv_img_dsc_t img_sllock1_2;
    extern "C" const lv_img_dsc_t img_usb1_1;
    extern "C" const lv_img_dsc_t img_usb1_2;
    extern "C" const lv_img_dsc_t img_win1_1;
    extern "C" const lv_img_dsc_t img_win1_2;
    extern "C" const lv_img_dsc_t img_winlock1_1;
    extern "C" const lv_img_dsc_t img_winlock1_2;
    extern "C" const lv_img_dsc_t img_am1;
    extern "C" const lv_img_dsc_t img_pm1;
    lv_obj_t *img_battery_frame;
    lv_obj_t *img_battery_charging;
    lv_obj_t *obj_battery_cell;
    lv_obj_t *lbl_battery;
    lv_obj_t *img_1;
    lv_obj_t *img_2;
    lv_obj_t *img_3;
    lv_obj_t *img_4;
    lv_obj_t *img_5;
    lv_obj_t *img_6;
    lv_obj_t *img_am;
    lv_obj_t *lbl_time;
    lv_obj_t *lbl_date;
    bool toggle_connection;
    void setup(lv_obj_t *home_appScreen)
    {
        toggle_connection = true;
        lv_obj_set_style_bg_img_src(home_appScreen, &img_main1_bg, 0);
        img_1 = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_1, 115, 120);

        img_2 = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_2, 187, 120);

        img_3 = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_3, 259, 120);

        img_4 = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_4, 115, 177);

        img_5 = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_5, 187, 177);

        img_6 = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_6, 259, 177);

        lbl_time = lv_label_create(home_appScreen);
        lv_label_set_text(lbl_time, "");
        lv_obj_set_style_text_font(lbl_time, &font_main1_time, 0);
        lv_obj_set_pos(lbl_time, 150 + 8, 30);
        lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xf9c262), 0);

        lbl_date = lv_label_create(home_appScreen);
        lv_label_set_text(lbl_date, "");
        lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_26, 0);
        lv_obj_set_style_text_color(lbl_date, lv_color_hex(0xf9c262), 0);
        lv_obj_set_pos(lbl_date, 6, 110);
        lv_obj_set_width(lbl_date, 90);
        lv_obj_set_style_text_align(lbl_date, LV_TEXT_ALIGN_CENTER, 0);
        img_am = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_am, 118 + 8, 50);
        lv_obj_add_flag(img_am, LV_OBJ_FLAG_HIDDEN);
        // 电池
        img_battery_frame = lv_img_create(home_appScreen);
        lv_img_set_src(img_battery_frame, &battery_frame_1);
        lv_obj_set_pos(img_battery_frame, 195, 82);
        obj_battery_cell = lv_obj_create(home_appScreen); // 最宽38
        lv_obj_set_pos(obj_battery_cell, 199, 85);
        lv_obj_set_size(obj_battery_cell, 38, 10);
        lv_obj_set_style_radius(obj_battery_cell, 2, 0);
        lv_obj_clear_flag(obj_battery_cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_border_opa(obj_battery_cell, 0, 0);
        lv_obj_set_style_bg_color(obj_battery_cell, lv_color_hex(0xf8c163), 0);
        img_battery_charging = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_battery_charging, 185, 84);
        lv_img_set_src(img_battery_charging, &battery_charging_1);
        lv_obj_add_flag(img_battery_charging, LV_OBJ_FLAG_HIDDEN);
        lbl_battery = lv_label_create(home_appScreen);
        lv_obj_set_pos(lbl_battery, 173 - 9, 82);
        lv_obj_set_style_text_color(lbl_battery, lv_color_hex(0xf8c163), 0);
        lv_label_set_text(lbl_battery, "--%");
    }
    void blinker()
    {
        // img2_state
        if (hal.kb_status.chan_state == 1 || hal.kb_status.chan_state == 2)
        {
            if (toggle_connection)
            {
                toggle_connection = false;
                lv_obj_add_flag(img_2, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                toggle_connection = true;
                lv_obj_clear_flag(img_2, LV_OBJ_FLAG_HIDDEN);
            }
        }
        else
        {
            lv_obj_clear_flag(img_2, LV_OBJ_FLAG_HIDDEN);
        }
        if (hal.kb_status.chan_state & 1)
        {
            if (hal.kb_status.channel_current == 0)
                lv_img_set_src(img_2, &img_rf1_1);
            else if (hal.kb_status.channel_current == 1)
                lv_img_set_src(img_2, &img_bt1_1);
            else if (hal.kb_status.channel_current == 2)
                lv_img_set_src(img_2, &img_bt2_1);
            else if (hal.kb_status.channel_current == 3)
                lv_img_set_src(img_2, &img_usb1_1);
        }
        else
        {
            if (hal.kb_status.channel_current == 0)
                lv_img_set_src(img_2, &img_rf1_2);
            else if (hal.kb_status.channel_current == 1)
                lv_img_set_src(img_2, &img_bt1_2);
            else if (hal.kb_status.channel_current == 2)
                lv_img_set_src(img_2, &img_bt2_2);
            else if (hal.kb_status.channel_current == 3)
                lv_img_set_src(img_2, &img_usb1_2);
        }
    }
    void loop()
    {
        if (hal.battery_pct != 0)
        {
            if (hal.battery_status == BATTERY_STATUS_CHARGING)
            {
                lv_obj_clear_flag(img_battery_charging, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(lbl_battery, "");
                lv_obj_set_width(obj_battery_cell, 38);
            }
            else
            {
                lv_obj_add_flag(img_battery_charging, LV_OBJ_FLAG_HIDDEN);
                if (hal.battery_pct == 100)
                {
                    lv_obj_set_x(lbl_battery, 173 - 9 - 5);
                }
                else
                {
                    lv_obj_set_x(lbl_battery, 173 - 9);
                }
                lv_label_set_text_fmt(lbl_battery, "%d%%", hal.battery_pct);
                lv_obj_set_width(obj_battery_cell, hal.battery_pct * 38 / 100);
            }
        }
        if (hal.config_time_12hr)
        {
            int hour = hal.datetime.hour;
            if (hour > 12)
            {
                lv_img_set_src(img_am, &img_pm1);
                hour -= 12;
            }
            else
            {
                lv_img_set_src(img_am, &img_am1);
            }

            lv_obj_clear_flag(img_am, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(lbl_time, "%02d:%02d", hour, hal.datetime.minute);
        }
        else
        {
            lv_obj_add_flag(img_am, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(lbl_time, "%02d:%02d", hal.datetime.hour, hal.datetime.minute);
        }
        lv_label_set_text_fmt(lbl_date, "%02d/%02d", hal.datetime.month, hal.datetime.dayMonth);
        // img1
        if (hal.kb_status.system == 1)
            lv_img_set_src(img_1, &img_win1_1);
        else
            lv_img_set_src(img_1, &img_mac1_1);
        // img2
        blinker();
        // img3
        if (hal.kb_status.winlk && lv_img_get_src(img_3) != &img_winlock1_1)
            lv_img_set_src(img_3, &img_winlock1_1);
        else if (!hal.kb_status.winlk && lv_img_get_src(img_3) != &img_winlock1_2)
            lv_img_set_src(img_3, &img_winlock1_2);
        // img4
        if (hal.kb_status.capslock && lv_img_get_src(img_4) != &img_caplock1_1)
            lv_img_set_src(img_4, &img_caplock1_1);
        else if (!hal.kb_status.capslock && lv_img_get_src(img_4) != &img_caplock1_2)
            lv_img_set_src(img_4, &img_caplock1_2);
        // img5
        if (hal.kb_status.scrolllock && lv_img_get_src(img_5) != &img_sllock1_1)
            lv_img_set_src(img_5, &img_sllock1_1);
        else if (!hal.kb_status.scrolllock && lv_img_get_src(img_5) != &img_sllock1_2)
            lv_img_set_src(img_5, &img_sllock1_2);
        // img6
        if (hal.kb_status.numlock && lv_img_get_src(img_6) != &img_numlock1_1)
            lv_img_set_src(img_6, &img_numlock1_1);
        else if (!hal.kb_status.numlock && lv_img_get_src(img_6) != &img_numlock1_2)
            lv_img_set_src(img_6, &img_numlock1_2);
    }
}
namespace theme_spartan
{
    extern "C" const lv_img_dsc_t img_main2_bg;
    extern "C" const lv_img_dsc_t battery_frame_2;
    extern "C" const lv_img_dsc_t battery_indicator_2;
    extern "C" const lv_img_dsc_t img_bt1;
    extern "C" const lv_img_dsc_t img_bt2;
    extern "C" const lv_img_dsc_t img_caplock;
    extern "C" const lv_img_dsc_t img_mac;
    extern "C" const lv_img_dsc_t img_numlock;
    extern "C" const lv_img_dsc_t img_rf;
    extern "C" const lv_img_dsc_t img_sl;
    extern "C" const lv_img_dsc_t img_usb;
    extern "C" const lv_img_dsc_t img_win;
    extern "C" const lv_img_dsc_t img_winlock;
    extern "C" const lv_img_dsc_t img_am2;
    extern "C" const lv_img_dsc_t img_pm2;
    extern "C" const lv_img_dsc_t charge_bolt_spartan;
    lv_obj_t *img_battery_frame;
    lv_obj_t *img_battery_charging;
    lv_obj_t *obj_battery_cell;
    lv_obj_t *lbl_battery;
    lv_obj_t *img_battery_bolt;
    lv_obj_t *obj_img_1;
    lv_obj_t *obj_img_2;
    lv_obj_t *obj_img_3;
    lv_obj_t *obj_img_4;
    lv_obj_t *obj_img_5;
    lv_obj_t *obj_img_6;
    lv_obj_t *img_1;
    lv_obj_t *img_2;
    lv_obj_t *img_3;
    lv_obj_t *img_4;
    lv_obj_t *img_5;
    lv_obj_t *img_6;
    lv_obj_t *img_am;
    lv_obj_t *lbl_time;
    lv_obj_t *lbl_date;
    lv_obj_t *lbl_year;
    bool toggle_connection;
    const lv_img_dsc_t *img_2_src[4] = {&img_rf, &img_bt1, &img_bt2, &img_usb};
    void setup(lv_obj_t *home_appScreen)
    {
        toggle_connection = true;
        lv_obj_set_style_bg_img_src(home_appScreen, &img_main2_bg, 0);
        obj_img_1 = lv_obj_create(home_appScreen);
        lv_obj_set_size(obj_img_1, 45, 45);
        lv_obj_set_pos(obj_img_1, 138, 128);
        lv_obj_set_style_bg_opa(obj_img_1, LV_OPA_70, 0);
        lv_obj_set_style_border_opa(obj_img_1, 0, 0);
        lv_obj_set_style_radius(obj_img_1, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(obj_img_1, LV_OBJ_FLAG_SCROLLABLE);
        img_1 = lv_img_create(obj_img_1);
        lv_img_set_src(img_1, &img_win);
        lv_obj_center(img_1);

        obj_img_2 = lv_obj_create(home_appScreen);
        lv_obj_set_size(obj_img_2, 45, 45);
        lv_obj_set_pos(obj_img_2, 198, 128);
        lv_obj_set_style_bg_opa(obj_img_2, LV_OPA_70, 0);
        lv_obj_set_style_border_opa(obj_img_2, 0, 0);
        lv_obj_set_style_radius(obj_img_2, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(obj_img_2, LV_OBJ_FLAG_SCROLLABLE);
        img_2 = lv_img_create(obj_img_2);
        lv_obj_center(img_2);

        obj_img_3 = lv_obj_create(home_appScreen);
        lv_obj_set_size(obj_img_3, 45, 45);
        lv_obj_set_pos(obj_img_3, 258, 128);
        lv_obj_set_style_bg_opa(obj_img_3, LV_OPA_70, 0);
        lv_obj_set_style_border_opa(obj_img_3, 0, 0);
        lv_obj_set_style_radius(obj_img_3, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(obj_img_3, LV_OBJ_FLAG_SCROLLABLE);
        img_3 = lv_img_create(obj_img_3);
        lv_img_set_src(img_3, &img_winlock);
        lv_obj_center(img_3);

        obj_img_4 = lv_obj_create(home_appScreen);
        lv_obj_set_size(obj_img_4, 45, 45);
        lv_obj_set_pos(obj_img_4, 138, 182);
        lv_obj_set_style_bg_opa(obj_img_4, LV_OPA_70, 0);
        lv_obj_set_style_border_opa(obj_img_4, 0, 0);
        lv_obj_set_style_radius(obj_img_4, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(obj_img_4, LV_OBJ_FLAG_SCROLLABLE);
        img_4 = lv_img_create(obj_img_4);
        lv_img_set_src(img_4, &img_caplock);
        lv_obj_center(img_4);

        obj_img_5 = lv_obj_create(home_appScreen);
        lv_obj_set_size(obj_img_5, 45, 45);
        lv_obj_set_pos(obj_img_5, 198, 182);
        lv_obj_set_style_bg_opa(obj_img_5, LV_OPA_70, 0);
        lv_obj_set_style_border_opa(obj_img_5, 0, 0);
        lv_obj_set_style_radius(obj_img_5, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(obj_img_5, LV_OBJ_FLAG_SCROLLABLE);
        img_5 = lv_img_create(obj_img_5);
        lv_img_set_src(img_5, &img_sl);
        lv_obj_center(img_5);

        obj_img_6 = lv_obj_create(home_appScreen);
        lv_obj_set_size(obj_img_6, 45, 45);
        lv_obj_set_pos(obj_img_6, 258, 182);
        lv_obj_set_style_bg_opa(obj_img_6, LV_OPA_70, 0);
        lv_obj_set_style_border_opa(obj_img_6, 0, 0);
        lv_obj_set_style_radius(obj_img_6, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(obj_img_6, LV_OBJ_FLAG_SCROLLABLE);
        img_6 = lv_img_create(obj_img_6);
        lv_img_set_src(img_6, &img_numlock);
        lv_obj_center(img_6);

        lbl_time = lv_label_create(home_appScreen);
        lv_label_set_text(lbl_time, "");
        lv_obj_set_style_text_font(lbl_time, &font_main1_time, 0);
        lv_obj_set_pos(lbl_time, 136, 20);
        lv_obj_set_size(lbl_time, 171, 42);
        lv_obj_set_style_text_align(lbl_time, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(lbl_time, lv_color_hex(0x000000), 0);

        lbl_date = lv_label_create(home_appScreen);
        lv_label_set_text(lbl_date, "");
        lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(lbl_date, lv_color_hex(0x000000), 0);
        lv_obj_set_pos(lbl_date, 17, 41);
        lv_obj_set_width(lbl_date, 90);
        lv_obj_set_style_text_align(lbl_date, LV_TEXT_ALIGN_CENTER, 0);

        lbl_year = lv_label_create(home_appScreen);
        lv_label_set_text(lbl_year, "");
        lv_obj_set_style_text_font(lbl_year, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(lbl_year, lv_color_hex(0x3c3c3c), 0);
        lv_obj_set_pos(lbl_year, 12, 17);
        lv_obj_set_width(lbl_year, 100);
        lv_obj_set_style_text_align(lbl_year, LV_TEXT_ALIGN_CENTER, 0);
        img_am = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_am, 144, 29);
        img_battery_frame = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_battery_frame, 250 - 50, 58 + 5);
        lv_img_set_src(img_battery_frame, &battery_frame_2);
        obj_battery_cell = lv_obj_create(home_appScreen);
        lv_obj_set_pos(obj_battery_cell, 261 - 50, 63 + 5);
        lv_obj_set_size(obj_battery_cell, 29, 5);
        lv_obj_set_style_radius(obj_battery_cell, 0, 0);
        lv_obj_clear_flag(obj_battery_cell, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_border_opa(obj_battery_cell, 0, 0);
        lv_obj_set_style_bg_opa(obj_battery_cell, 0, 0);
        lv_obj_set_style_bg_img_src(obj_battery_cell, &battery_indicator_2, 0);
        lbl_battery = lv_label_create(home_appScreen);
        lv_obj_set_pos(lbl_battery, 250 - 50 - 22 - 9, 58 + 5);
        lv_obj_set_style_text_color(lbl_battery, lv_color_hex(0xFFFFFF), 0);
        lv_label_set_text(lbl_battery, "98%");
        img_battery_bolt = lv_img_create(home_appScreen);
        lv_obj_set_pos(img_battery_bolt, 250 - 50 - 10, 58 + 5);
        lv_img_set_src(img_battery_bolt, &charge_bolt_spartan);
        lv_obj_add_flag(img_battery_bolt, LV_OBJ_FLAG_HIDDEN);
    }
    void blinker()
    {
        lv_img_set_src(img_2, img_2_src[hal.kb_status.channel_current]);
        if (hal.kb_status.chan_state == 1 || hal.kb_status.chan_state == 2)
        {
            if (toggle_connection)
            {
                toggle_connection = false;
                lv_obj_add_flag(img_2, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                toggle_connection = true;
                lv_obj_clear_flag(img_2, LV_OBJ_FLAG_HIDDEN);
            }
        }
        else
        {
            lv_obj_clear_flag(img_2, LV_OBJ_FLAG_HIDDEN);
        }
        if (hal.kb_status.chan_state == 3)
        {
            lv_obj_set_style_bg_color(obj_img_2, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
        }
        else
        {
            lv_obj_set_style_bg_color(obj_img_2, lv_color_hex(0x282b30), 0);
        }
    }
    void loop()
    {
        if (hal.battery_pct != 0)
        {
            if (hal.battery_status != BATTERY_STATUS_CHARGING)
            {
                if (hal.battery_pct == 100)
                {
                    lv_obj_set_x(lbl_battery, 250 - 50 - 28 - 9);
                }
                else
                {
                    lv_obj_set_x(lbl_battery, 250 - 50 - 22 - 9);
                }
                lv_label_set_text_fmt(lbl_battery, "%d%%", hal.battery_pct);
                lv_obj_add_flag(img_battery_bolt, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_width(obj_battery_cell, hal.battery_pct * 29 / 100);
            }
            else
            {
                lv_obj_clear_flag(img_battery_bolt, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(lbl_battery, "");
                lv_obj_set_width(obj_battery_cell, 29);
            }
        }
        if (hal.config_time_12hr)
        {
            int hour = hal.datetime.hour;
            if (hour > 12)
            {
                lv_img_set_src(img_am, &img_pm2);
                lv_obj_set_pos(img_am, 144, 43);
                hour -= 12;
            }
            else
            {
                lv_img_set_src(img_am, &img_am2);
                lv_obj_set_pos(img_am, 144, 29);
            }

            lv_label_set_text_fmt(lbl_time, "%02d:%02d", hal.datetime.hour > 12 ? hal.datetime.hour - 12 : hal.datetime.hour, hal.datetime.minute);
        }
        else
        {
            lv_label_set_text_fmt(lbl_time, "%02d:%02d", hal.datetime.hour, hal.datetime.minute);
        }
        lv_label_set_text_fmt(lbl_date, "%02d/%02d", hal.datetime.month, hal.datetime.dayMonth);
        lv_label_set_text_fmt(lbl_year, "%04d", hal.datetime.year);
        blinker();
        if (hal.kb_status.capslock && lv_obj_get_style_bg_color(obj_img_4, 0).full != lv_palette_main(LV_PALETTE_BLUE_GREY).full)
            lv_obj_set_style_bg_color(obj_img_4, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
        else if (!hal.kb_status.capslock && lv_obj_get_style_bg_color(obj_img_4, 0).full != lv_color_hex(0x282b30).full)
            lv_obj_set_style_bg_color(obj_img_4, lv_color_hex(0x282b30), 0);

        if (hal.kb_status.numlock && lv_obj_get_style_bg_color(obj_img_6, 0).full != lv_palette_main(LV_PALETTE_BLUE_GREY).full)
            lv_obj_set_style_bg_color(obj_img_6, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
        else if (!hal.kb_status.numlock && lv_obj_get_style_bg_color(obj_img_6, 0).full != lv_color_hex(0x282b30).full)
            lv_obj_set_style_bg_color(obj_img_6, lv_color_hex(0x282b30), 0);

        if (hal.kb_status.scrolllock && lv_obj_get_style_bg_color(obj_img_5, 0).full != lv_palette_main(LV_PALETTE_BLUE_GREY).full)
            lv_obj_set_style_bg_color(obj_img_5, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
        else if (!hal.kb_status.scrolllock && lv_obj_get_style_bg_color(obj_img_5, 0).full != lv_color_hex(0x282b30).full)
            lv_obj_set_style_bg_color(obj_img_5, lv_color_hex(0x282b30), 0);
        if (hal.kb_status.system == 1 && lv_img_get_src(img_1) != &img_win)
            lv_img_set_src(img_1, &img_win);
        else if (hal.kb_status.system == 0 && lv_img_get_src(img_1) != &img_mac)
            lv_img_set_src(img_1, &img_mac);
        if (hal.kb_status.winlk && lv_obj_get_style_bg_color(obj_img_3, 0).full != lv_palette_main(LV_PALETTE_BLUE_GREY).full)
            lv_obj_set_style_bg_color(obj_img_3, lv_palette_main(LV_PALETTE_BLUE_GREY), 0);
        else if (!hal.kb_status.winlk && lv_obj_get_style_bg_color(obj_img_3, 0).full != lv_color_hex(0x282b30).full)
            lv_obj_set_style_bg_color(obj_img_3, lv_color_hex(0x282b30), 0);
    }

} // namespace theme_spartan
#endif
namespace theme_default
{
    lv_obj_t *lbl_time_hour, *lbl_time_colon, *lbl_time_minute, *lbl_date, *lbl_weather_city, *lbl_weather, *lbl_temp, *weather_icon;
    lv_obj_t *lbl_operator_title, *lbl_operator_subtitle, *operator_mark;
    lv_obj_t *lbl_battery_title;
    lv_obj_t *lbl_battery, *bar_battery, *lbl_connection, *lbl_system, *lbl_state, *status_dot;
    static lv_obj_t *separator_line;
    static const lv_point_t separator_points[] = {{166, 16}, {166, 226}};
    static lv_obj_t *sun_core, *sun_rays[8];
    static const lv_point_t sun_ray_points[8][2] = {
        {{190, 74}, {190, 81}}, {{190, 109}, {190, 116}},
        {{169, 95}, {176, 95}}, {{204, 95}, {211, 95}},
        {{175, 80}, {180, 85}}, {{200, 105}, {205, 110}},
        {{175, 110}, {180, 105}}, {{200, 85}, {205, 80}}
    };

    // The scenery (weather + time of day) comes from the shared atmosphere
    // layer, so the home page and every other page show the same backdrop.
    static const char *active_scene_label = nullptr;
    static void updateAtmosphere()
    {
        atmosphereRefresh();
        // atmosphereSceneLabel() returns string literals, so comparing pointers is
        // enough to tell whether the caption has to be rewritten.
        const char *sceneLabel = atmosphereSceneLabel();
        if (lbl_operator_title && sceneLabel != active_scene_label) {
            lv_label_set_text(lbl_operator_title, sceneLabel);
            active_scene_label = sceneLabel;
        }
    }

    static void style(lv_obj_t *label, uint32_t color, const lv_font_t *font)
    {
        lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
        lv_obj_set_style_text_font(label, font, 0);
    }

    static void updateClock()
    {
        char hour[8], minute[8];
        snprintf(hour, sizeof(hour), "%02d", hal.datetime.hour);
        snprintf(minute, sizeof(minute), "%02d", hal.datetime.minute);
        lv_label_set_text(lbl_time_hour, hour);
        lv_label_set_text(lbl_time_colon, ":");
        lv_label_set_text(lbl_time_minute, minute);

        const lv_font_t *font = &lv_font_montserrat_40;
        const lv_coord_t hourWidth = lv_txt_get_width(hour, 2, font, 0, LV_TEXT_FLAG_NONE);
        const lv_coord_t colonWidth = lv_txt_get_width(":", 1, font, 0, LV_TEXT_FLAG_NONE);
        const lv_coord_t start = 168;
        lv_obj_set_pos(lbl_time_hour, start, 1);
        lv_obj_set_pos(lbl_time_colon, start + hourWidth, 1);
        lv_obj_set_pos(lbl_time_minute, start + hourWidth + colonWidth, 1);
    }

    static void updateWeatherIcon(int code)
    {
        const bool sunny = code == 0 || code == 38;
        if (sunny) {
            lv_obj_add_flag(weather_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(sun_core, LV_OBJ_FLAG_HIDDEN);
            for (int i = 0; i < 8; ++i) lv_obj_clear_flag(sun_rays[i], LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_img_set_src(weather_icon, ::weatherImages_l[code >= 0 && code <= 38 ? code : 39]);
            lv_obj_clear_flag(weather_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(sun_core, LV_OBJ_FLAG_HIDDEN);
            for (int i = 0; i < 8; ++i) lv_obj_add_flag(sun_rays[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    static void updateKeyboardStatus()
    {
        const char *connection = "2.4G";
        switch (hal.kb_status.channel_current) {
        case 1: connection = "BT1"; break;
        case 2: connection = "BT2"; break;
        case 3: connection = "USB"; break;
        default: connection = "2.4G"; break;
        }
        lv_label_set_text(lbl_connection, connection);
        lv_label_set_text(lbl_system, hal.kb_status.system == 1 ? "WIN" : "MAC");
        lv_label_set_text(lbl_state, hal.kb_status.chan_state == 3 ? "ONLINE" : "WAIT");
        lv_obj_set_style_bg_color(status_dot,
                                  lv_color_hex(hal.kb_status.chan_state == 3 ? 0x8fd9ad : 0xe3bd70), 0);
    }

    static lv_obj_t *home_art = nullptr;
    static lv_img_dsc_t home_art_desc;
    static uint8_t *home_art_pixels = nullptr;
    static uint32_t home_art_revision = 0;
    static uint32_t home_art_layout_revision = 0;

    // Keeps the uploaded picture where the web page put it.
    static void applyHomeArtLayout()
    {
        if (!home_art) return;
        home_art_layout_revision = artstore::homeLayoutRevision();
        const artstore::HomeLayout layout = artstore::homeLayout();
        lv_img_set_zoom(home_art, layout.zoom);
        lv_obj_set_pos(home_art, layout.x, layout.y);
    }

    // Left-hand artwork for the home screen comes from the web upload page, so no
    // bundled character ships with the firmware.
    void reloadHomeArt(lv_obj_t *screen)
    {
        home_art_revision = artstore::revision();
        if (home_art) {
            lv_obj_del(home_art);
            home_art = nullptr;
        }
        if (home_art_pixels) {
            free(home_art_pixels);
            home_art_pixels = nullptr;
        }
        uint8_t *pixels = artstore::loadHomeImage();
        if (!pixels) return;
        home_art_pixels = pixels;
        memset(&home_art_desc, 0, sizeof(home_art_desc));
        home_art_desc.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        home_art_desc.header.w = artstore::homeWidth;
        home_art_desc.header.h = artstore::homeHeight;
        home_art_desc.data_size = artstore::homeBytes;
        home_art_desc.data = pixels;
        home_art = lv_img_create(screen);
        lv_img_set_src(home_art, &home_art_desc);
        lv_obj_clear_flag(home_art, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        applyHomeArtLayout();
        ESP_LOGI("HomeTheme", "home artwork loaded");
    }

    void releaseHomeArt()
    {
        if (home_art) {
            lv_obj_del(home_art);
            home_art = nullptr;
        }
        if (home_art_pixels) {
            free(home_art_pixels);
            home_art_pixels = nullptr;
        }
    }

    void setup(lv_obj_t *screen)
    {
        // Base colour plus a small image overscan prevents uncovered LCD edges.
        lv_obj_set_style_bg_color(screen, lv_color_hex(0x25303d), 0);
        lv_obj_set_style_border_width(screen, 0, 0);
        lv_obj_set_style_pad_all(screen, 0, 0);
        lv_obj_set_style_radius(screen, 0, 0);

        atmosphereAttach(screen, 0x0A1622, 90);
        reloadHomeArt(screen);

        updateAtmosphere();

        separator_line = lv_line_create(screen);
        lv_line_set_points(separator_line, separator_points, 2);
        lv_obj_set_style_line_color(separator_line, lv_color_hex(0x9a9a88), 0);
        lv_obj_set_style_line_opa(separator_line, LV_OPA_50, 0);
        lv_obj_set_style_line_width(separator_line, 1, 0);

        operator_mark = lv_obj_create(screen);
        lv_obj_set_pos(operator_mark, 12, 15);
        lv_obj_set_size(operator_mark, 18, 2);
        lv_obj_set_style_bg_color(operator_mark, lv_color_hex(0xe6b75d), 0);
        lv_obj_set_style_bg_opa(operator_mark, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(operator_mark, 0, 0);
        lv_obj_set_style_radius(operator_mark, 1, 0);
        lv_obj_clear_flag(operator_mark, LV_OBJ_FLAG_SCROLLABLE);

        lbl_operator_title = lv_label_create(screen);
        lv_label_set_text(lbl_operator_title, "HEAVY RAIN");
        lv_obj_set_pos(lbl_operator_title, 12, 22);
        style(lbl_operator_title, 0xefe7d8, &font_rajdhani_16);
        lv_obj_set_style_text_letter_space(lbl_operator_title, 1, 0);

        lbl_operator_subtitle = lv_label_create(screen);
        lv_label_set_text(lbl_operator_subtitle, "RHODES ISLAND");
        lv_obj_set_pos(lbl_operator_subtitle, 12, 38);
        style(lbl_operator_subtitle, 0xc2b99f, &font_rajdhani_16);
        lv_obj_set_style_text_letter_space(lbl_operator_subtitle, 1, 0);

        lbl_time_hour = lv_label_create(screen);
        style(lbl_time_hour, 0xefe7d8, &lv_font_montserrat_40);
        lbl_time_colon = lv_label_create(screen);
        style(lbl_time_colon, 0x42bde8, &lv_font_montserrat_40);
        lbl_time_minute = lv_label_create(screen);
        style(lbl_time_minute, 0xefe7d8, &lv_font_montserrat_40);
        updateClock();

        lbl_date = lv_label_create(screen);
        lv_obj_set_pos(lbl_date, 172, 44);
        style(lbl_date, 0xb8c4cc, &lv_font_montserrat_12);
        lv_obj_set_style_text_letter_space(lbl_date, 0, 0);

        lbl_weather_city = lv_label_create(screen);
        lv_obj_set_pos(lbl_weather_city, 172, 144);
        lv_obj_set_width(lbl_weather_city, 130);
        lv_label_set_long_mode(lbl_weather_city, LV_LABEL_LONG_DOT);
        style(lbl_weather_city, 0xe4e0d5, &lv_font_chinese_16);

        lbl_weather = lv_label_create(screen);
        lv_obj_set_pos(lbl_weather, 172, 122);
        style(lbl_weather, 0xe4e0d5, &lv_font_chinese_16);

        lbl_temp = lv_label_create(screen);
        lv_obj_set_pos(lbl_temp, 216, 77);
        style(lbl_temp, 0xe6b75d, &font_rajdhani_32);

        weather_icon = lv_img_create(screen);
        lv_img_set_src(weather_icon, ::weatherImages_l[39]);
        lv_obj_set_pos(weather_icon, 172, 79);
        // Draw sunny weather as a crisp vector mark; recolouring the source
        // image flattened its fine rays into a featureless disc.
        sun_core = lv_obj_create(screen);
        lv_obj_set_pos(sun_core, 183, 88);
        lv_obj_set_size(sun_core, 14, 14);
        lv_obj_set_style_bg_color(sun_core, lv_color_hex(0xe6b75d), 0);
        lv_obj_set_style_bg_opa(sun_core, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(sun_core, lv_color_hex(0x273745), 0);
        lv_obj_set_style_border_width(sun_core, 1, 0);
        lv_obj_set_style_radius(sun_core, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(sun_core, LV_OBJ_FLAG_SCROLLABLE);
        for (int i = 0; i < 8; ++i) {
            sun_rays[i] = lv_line_create(screen);
            lv_line_set_points(sun_rays[i], sun_ray_points[i], 2);
            lv_obj_set_style_line_color(sun_rays[i], lv_color_hex(0xe6b75d), 0);
            lv_obj_set_style_line_width(sun_rays[i], 2, 0);
            lv_obj_set_style_line_rounded(sun_rays[i], true, 0);
        }
        updateWeatherIcon(39);

        lbl_battery = lv_label_create(screen);
        lv_obj_set_pos(lbl_battery, 270, 165);
        style(lbl_battery, 0xefe7d8, &font_rajdhani_16);
        lbl_battery_title = lv_label_create(screen);
        lv_label_set_text(lbl_battery_title, "BATTERY");
        lv_obj_set_pos(lbl_battery_title, 172, 169);
        style(lbl_battery_title, 0xb8c4cc, &font_rajdhani_16);
        lv_obj_set_style_text_letter_space(lbl_battery_title, 1, 0);

        bar_battery = lv_bar_create(screen);
        lv_obj_set_pos(bar_battery, 172, 188);
        lv_obj_set_size(bar_battery, 132, 3);
        lv_bar_set_range(bar_battery, 0, 100);
        lv_obj_set_style_bg_color(bar_battery, lv_color_hex(0x4f5e69), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar_battery, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar_battery, lv_color_hex(0xe6b75d), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar_battery, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar_battery, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(bar_battery, 2, LV_PART_INDICATOR);

        status_dot = lv_obj_create(screen);
        lv_obj_set_pos(status_dot, 261, 214);
        lv_obj_set_size(status_dot, 4, 4);
        lv_obj_set_style_bg_color(status_dot, lv_color_hex(0xe6b75d), 0);
        lv_obj_set_style_bg_opa(status_dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(status_dot, 0, 0);
        lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_clear_flag(status_dot, LV_OBJ_FLAG_SCROLLABLE);
        lbl_connection = lv_label_create(screen);
        lv_obj_set_pos(lbl_connection, 172, 207);
        style(lbl_connection, 0xe6b75d, &font_rajdhani_16);
        lv_obj_set_style_text_letter_space(lbl_connection, 0, 0);
        lbl_system = lv_label_create(screen);
        lv_obj_set_pos(lbl_system, 220, 207);
        style(lbl_system, 0xdce4e6, &font_rajdhani_16);
        lv_obj_set_style_text_letter_space(lbl_system, 0, 0);
        lbl_state = lv_label_create(screen);
        lv_obj_set_pos(lbl_state, 270, 207);
        style(lbl_state, 0xdce4e6, &font_rajdhani_16);
        lv_obj_set_style_text_letter_space(lbl_state, 0, 0);
        updateKeyboardStatus();
    }

    void loop()
    {
        if (artstore::revision() != home_art_revision) {
            reloadHomeArt(lv_scr_act());
        } else if (artstore::homeLayoutRevision() != home_art_layout_revision) {
            applyHomeArtLayout();
        }
        updateAtmosphere();
        updateClock();
        static const char *weekdays[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
        const int weekday = hal.datetime.dayWeek >= 1 && hal.datetime.dayWeek <= 7 ? hal.datetime.dayWeek - 1 : 0;
        lv_label_set_text_fmt(lbl_date, "%02d-%02d %s", hal.datetime.month, hal.datetime.dayMonth, weekdays[weekday]);
        lv_label_set_text(lbl_weather_city, weatherHomeLocation());
        lv_label_set_text(lbl_weather, weatherHomeCondition());
        lv_label_set_text_fmt(lbl_temp, "%d°", weatherHomeTemperature());
        const int weatherCode = weatherHomeCode();
        updateWeatherIcon(weatherCode);
        lv_label_set_text_fmt(lbl_battery, "%d%%", hal.battery_pct);
        lv_bar_set_value(bar_battery, hal.battery_pct, LV_ANIM_OFF);
        updateKeyboardStatus();
    }

    void blinker() { updateKeyboardStatus(); }
} // namespace theme_default

uint8_t last_minute = -1;
static uint32_t last_home_weather_revision = 0;
void AppHome::setup()
{
    last_minute = -1;
    hal.LOCKLV();
    theme_default::setup(_appScreen);
    theme_default::loop();
    theme_default::blinker();
    hal.UNLOCKLV();
    last_home_weather_revision = weatherHomeRevision();
    weatherHomeServiceSetActive(true);
}
bool last_is_blinking = false;
extern SemaphoreHandle_t status_changed;
void AppHome::loop()
{
    hal.getTime();
    const uint32_t weatherRevision = weatherHomeRevision();
    if (hal.datetime.minute != last_minute || weatherRevision != last_home_weather_revision || xSemaphoreTake(status_changed, 500) == pdTRUE)
    {
        last_minute = hal.datetime.minute;
        last_home_weather_revision = weatherRevision;
        hal.LOCKLV();
        theme_default::loop();
        hal.UNLOCKLV();
        return;
    }
    if (hal.kb_status.chan_state == 1 || hal.kb_status.chan_state == 2)
    {
        hal.LOCKLV();
        theme_default::blinker();
        hal.UNLOCKLV();
    }
    delay(100);
}

void AppHome::destroy()
{
    weatherHomeServiceSetActive(false);
    hal.LOCKLV();
    theme_default::releaseHomeArt();
    hal.UNLOCKLV();
}
