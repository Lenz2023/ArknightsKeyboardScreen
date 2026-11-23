#include "A_Config.h"
#include "ArtStore.h"
#include "Atmosphere.h"
#include "Bgm.h"

#include "es8311.h"
#include "driver/i2s_std.h"
#include <math.h>
#include <Wire.h>

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI(); 

HAL hal;
i2s_chan_handle_t i2s_tx = nullptr;
es8311_handle_t es_handle = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
#define DRAW_BUF_SIZE (2 * screenWidth * screenHeight)
lv_indev_t *indev_keypad;

static int first_refresh = 3;

void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h);
    tft.endWrite();

    if (first_refresh > 0)
    {
        first_refresh--;
        if (first_refresh == 0)
        {
            if(screen_is_on == true) {
                screen_is_sleep = false;
                hal.setBrightness(hal._brightness);
            }
            hal.time_sync = true;
            ESP_LOGW("HAL", "首次刷新完成");
            first_refresh = -1;
        }
    }
    lv_disp_flush_ready(disp_drv);
}

const char* getFileSuffix(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

bool hasFileSuffix(const char *filename, const char *suffix) {
    const char *file_suffix = getFileSuffix(filename);
    return strcmp(file_suffix, suffix) == 0;
}
size_t sizeFile(String path) {
  File file = LittleFS.open(path, "r", false); 
  size_t size = file.size();
  file.close();
  return size;
}

uint8_t * readFile(String path) {
  File file = LittleFS.open(path, "r", false); 
  uint8_t *buf = (uint8_t *)malloc(file.size());
  if (buf == NULL) {
    file.close();
    return NULL;
  }

  int bytesRead = file.readBytes((char *)buf, file.size());
  if (bytesRead != file.size()) {
    file.close();
    return NULL;
  }

  file.close();
  return buf;
}

static void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static uint8_t last_key = 0xff;
    static uint8_t act_key = 0;
    static bool last_pressed = false;
    data->state = LV_INDEV_STATE_REL;
    data->key = last_key;
    if (last_pressed)
    {
        last_pressed = false;
        return;
    }
    if (xQueueReceive(hal._queue_kb, &act_key, 0) != pdTRUE)
        return;
    data->key = act_key;
    last_pressed = true;
    data->state = LV_INDEV_STATE_PR;
    last_key = act_key;
}

static void task_systemctl(void *p)
{
    if (DS1302_isHalted(&hal.rtc))
    {
        DS1302_writeProtect(&hal.rtc, false);
        DS1302_halt(&hal.rtc, false);
        GUI::toast(_tr(I18N_ID_RTC_SHUTDOWN), true, 10000);
    }
    while (1)
    {
        sysctl_event_t event;
        xQueueReceive(hal._queue, &event, portMAX_DELAY);
        switch (event.type)
        {
        case EVENT_GET_TIME:
            DS1302_getDateTime(&hal.rtc, &hal.datetime);
            break;
        case EVENT_GOTO_SETTING:
            hal.setting_mode = true;
            appManagerLite.switchSetting();
            break;
        case EVENT_EXIT_SETTING:
            hal.setting_mode = false;
            appManagerLite.exitSetting();
            break;
        case EVENT_REQUIRE_SETTING:
            break;
        case EVENT_TOGGLE_SCREEN_ON:
            if (event.data == 0)
            {
                ledcWrite(PIN_DISPLAY_BL, 0);
            }
            else
            {
                hal.setBrightness(hal._brightness);
            }
            break;
        case EVENT_APM_CHANGED:
            hal.APM = event.data;
            hal.APMChanged = true;
            break;
        case EVENT_SERVERCTL:
            if (event.data == 1)
                hal.start_webserver();
            else
                hal.stop_webserver();
            break;
        case EVENT_GOSLEEP:
        {
            if (xSemaphoreTake(hal._mutex, 4000) == pdTRUE)
            {
                if (hal.server_started == false)
                {
                    hal.goSleep();
                }
                else
                {
                    screen_is_sleep = true;
                    ledcWrite(PIN_DISPLAY_BL, 0);
                    xSemaphoreGive(hal._mutex);
                }
            }
            else
            {
                ESP_LOGE("HAL", "无法获取锁");
                hal.goSleep();
            }
        }
        break;
        case EVENT_GOTO_QRCODE:
            hal.qrcode_mode = true;
            appManagerLite.switchQRCode();
            break;
        case EVENT_EXIT_QRCODE:
            hal.qrcode_mode = false;
            appManagerLite.exitQRCode();
            break;
        case EVENT_HOME_REFRESH:
            appManagerLite.switchApp(1);
            break;
        case EVENT_GOTO_MUSIC:
            appManagerLite.openMusic();
            break;
        case EVENT_EXIT_MUSIC:
            appManagerLite.exitMusic();
            break;
        default:
            break;
        }
    }
}

void lcd_init()
{
    pinMode(PIN_DISPLAY_PWR, OUTPUT);
    digitalWrite(PIN_DISPLAY_PWR, LOW);
    pinMode(PIN_DISPLAY_BL, OUTPUT);
    ledcAttach(PIN_DISPLAY_BL, 16000, 8);
    ledcWrite(PIN_DISPLAY_BL, 0);

    delay(1000);

    tft.init();

    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.invertDisplay(true);
}

void HAL::init()
{
    memset(&datetime, 0, sizeof(DS1302_DateTime));
    WiFi.setHostname("skyloong-screen");
    lcd_init();

    hal.audio_init();

    DS1302_begin(&rtc, PIN_RTC_SCLK, PIN_RTC_SDIO, PIN_RTC_RST);
    DS1302_writeClockRegister(&rtc, DS1302_REG_TC, 0xA5);
    if (LittleFS.begin(false) == false)
    {
        LittleFS.format();
        if (LittleFS.begin(false) == false)
        {
            ESP_LOGE("HAL", "LittleFS格式化失败");
            while (1)
                vTaskDelay(1000);
        }
    }

    loadAppSettings();
    pref.begin("settings", false);
    hal._brightness = pref.getUInt("bright", 6);
    // 音量首次启用时统一归零：早期测试可能把音量写进了 NVS
    if (!pref.getBool("audio_ready", false))
    {
        pref.putUInt("volume", 0);
        pref.putBool("audio_ready", true);
    }
    hal._volume = pref.getUInt("volume", 0);
    hal.setVolume(hal._volume);
    bgm::init();

    // 开机不再自动发声：音量由设置页控制，默认 0（静音）
    hal.config_time_12hr = pref.getBool("12hr", false);
    hal.weather_enable = pref.getBool("weather_enable", true);
    hal.sleep_minutes = pref.getUShort("sleep_min", 10);
    i18n::setLanguage(pref.getUInt("lang", 0));
    i18n::setNTPOffset(pref.getInt("ntp", 3600 * 8));
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t *screen_buf = NULL;
    static lv_color_t *screen_buf2 = NULL;
    static lv_disp_drv_t disp_drv;
    static lv_indev_drv_t indev_drv;

    _mutex = xSemaphoreCreateMutex();
    _queue = xQueueCreate(10, sizeof(sysctl_event_t));
    _queue_kb = xQueueCreate(10, 1);
    lv_init();

    screen_buf = (lv_color_t *)heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_SPIRAM);
    screen_buf2 = (lv_color_t *)heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_SPIRAM);

    lv_disp_draw_buf_init(&draw_buf, screen_buf, screen_buf2, DRAW_BUF_SIZE / 2);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv);

    lv_group_t *group = lv_group_create();
    lv_group_set_default(group);
    lv_indev_set_group(indev_keypad, group);

    xTaskCreatePinnedToCore(task_systemctl, "task_systemctl", 8192, NULL, 3, NULL, 0);
}

void HAL::getTime()
{
    send_sysctl(EVENT_GET_TIME);
}
uint8_t HAL::getDoW(uint16_t iYear, uint8_t iMonth, uint8_t iDay)
{
    int iWeek = 0;
    unsigned int y = 0, c = 0, m = 0, d = 0;

    if (iMonth == 1 || iMonth == 2)
    {
        c = (iYear - 1) / 100;
        y = (iYear - 1) % 100;
        m = iMonth + 12;
        d = iDay;
    }
    else
    {
        c = iYear / 100;
        y = iYear % 100;
        m = iMonth;
        d = iDay;
    }

    iWeek = y + y / 4 + c / 4 - 2 * c + 26 * (m + 1) / 10 + d - 1; // 蔡勒公式
    iWeek = iWeek >= 0 ? (iWeek % 7) : (iWeek % 7 + 7);            // iWeek为负时取模
    if (iWeek == 0)                                                // 星期日不作为一周的第一天
    {
        iWeek = 7;
    }

    return iWeek;
}
void HAL::setTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    DS1302_DateTime dt;
    DS1302_writeProtect(&rtc, false);
    DS1302_halt(&rtc, false);
    dt.second = second;
    dt.minute = minute;
    dt.hour = hour;
    dt.dayWeek = getDoW(year, month, day);
    dt.dayMonth = day;
    dt.month = month;
    dt.year = year;
    DS1302_setDateTime(&rtc, &dt);
}

/*-------- NTP ----------*/
#define CONFIG_NTP_ADDR "ntp.aliyun.com"
#include "WiFiUdp.h"
#include "hal.h"
WiFiUDP Udp;
static const int NTP_PACKET_SIZE = 48;        // NTP time is in the first 48 bytes of message
static byte NTPpacketBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing packets

// send an NTP request to the time server at the given address
static void sendNTPpacket(IPAddress &address)
{
    // set all bytes in the buffer to 0
    memset(NTPpacketBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    NTPpacketBuffer[0] = 0b11100011; // LI, Version, Mode
    NTPpacketBuffer[1] = 0;          // Stratum, or type of clock
    NTPpacketBuffer[2] = 6;          // Polling Interval
    NTPpacketBuffer[3] = 0xEC;       // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    NTPpacketBuffer[12] = 49;
    NTPpacketBuffer[13] = 0x4E;
    NTPpacketBuffer[14] = 49;
    NTPpacketBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); // NTP requests are to port 123
    Udp.write(NTPpacketBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}
static time_t getNtpTime()
{
    IPAddress ntpServerIP;
    while (Udp.parsePacket() > 0)
        ;
    WiFi.hostByName(CONFIG_NTP_ADDR, ntpServerIP);
    sendNTPpacket(ntpServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500)
    {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE)
        {
            Udp.read(NTPpacketBuffer, NTP_PACKET_SIZE);
            unsigned long secsSince1900;
            secsSince1900 = (unsigned long)NTPpacketBuffer[40] << 24;
            secsSince1900 |= (unsigned long)NTPpacketBuffer[41] << 16;
            secsSince1900 |= (unsigned long)NTPpacketBuffer[42] << 8;
            secsSince1900 |= (unsigned long)NTPpacketBuffer[43];
            return secsSince1900 - 2208988800UL;
        }
    }
    return 0;
}
bool HAL::NTPSync()
{
    time_t tm_now = 0;
    Udp.begin(8888);
    for (uint8_t i = 0; i < 5; ++i)
    {
        delay(10);
        tm_now = getNtpTime();
        if (tm_now != 0 && tm_now != 0xffffffff)
        {
            tm_now += i18n::getNTPOffset();
            tm time_now;
            localtime_r(&tm_now, &time_now);
            ESP_LOGI("NTP", "NTP同步成功: %d-%d-%d %d:%d:%d", time_now.tm_year + 1900, time_now.tm_mon + 1, time_now.tm_mday, time_now.tm_hour, time_now.tm_min, time_now.tm_sec);
            setTime(time_now.tm_year + 1900, time_now.tm_mon + 1, time_now.tm_mday, time_now.tm_hour, time_now.tm_min, time_now.tm_sec);
            break;
        }
        else
        {
            tm_now = 0;
        }
    }
    Udp.stop();
    if (tm_now == 0)
        return false;
    return true;
}

void HAL::setBrightness(int8_t brightness)
{
    static uint8_t brightness_lut[10] = {10, 15, 20, 30, 40, 60, 90, 130, 180, 255};
    if (brightness >= 0 && brightness <= 9)
    {
        _brightness = brightness;
        ledcWrite(PIN_DISPLAY_BL, brightness_lut[brightness]);
    }
    else
    {
        ESP_LOGE("HAL", "亮度设置错误: %d", brightness);
        ledcWrite(PIN_DISPLAY_BL, 130);
    }
}

esp_err_t HAL::audio_init()
{
    pinMode(AUDIO_AMP_CTRL, OUTPUT);
    digitalWrite(AUDIO_AMP_CTRL, LOW);   // 功放先静音，音量设好再开

    Wire.begin(AUDIO_CODEC_I2C_SDA_PIN, AUDIO_CODEC_I2C_SCL_PIN);

    ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, "HAL", "es8311 create failed");
    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = AUDIO_MCLK_FREQ_HZ,
        .sample_frequency = AUDIO_SAMPLE_RATE
    };
    esp_err_t err = es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    if (err != ESP_OK)
    {
        ESP_LOGE("HAL", "es8311 init failed: %s", esp_err_to_name(err));
        return err;
    }
    es8311_microphone_config(es_handle, false);
    es8311_voice_volume_set(es_handle, 0, NULL);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    err = i2s_new_channel(&chan_cfg, &i2s_tx, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE("HAL", "i2s_new_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = (gpio_num_t)AUDIO_I2S_GPIO_MCLK,
            .bclk = (gpio_num_t)AUDIO_I2S_GPIO_BCLK,
            .ws = (gpio_num_t)AUDIO_I2S_GPIO_WS,
            .dout = (gpio_num_t)AUDIO_I2S_GPIO_DOUT,
            .din = (gpio_num_t)AUDIO_I2S_GPIO_DIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    err = i2s_channel_init_std_mode(i2s_tx, &std_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE("HAL", "i2s_channel_init_std_mode failed: %s", esp_err_to_name(err));
        return err;
    }
    i2s_channel_enable(i2s_tx);
    ESP_LOGI("HAL", "audio ready: es8311 + i2s %d Hz (mclk=%d, bclk=%d, ws=%d, dout=%d)",
             AUDIO_SAMPLE_RATE, AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT);
    return ESP_OK;
}

size_t HAL::audio_write(const uint8_t *data, size_t len)
{
    if (!i2s_tx || !data || !len)
        return 0;
    size_t written = 0;
    if (i2s_channel_write(i2s_tx, data, len, &written, pdMS_TO_TICKS(1000)) != ESP_OK)
        return 0;
    return written;
}

void HAL::setVolume(int8_t volume)
{
    static const uint8_t volume_lut[10] = {0, 38, 46, 53, 59, 64, 68, 71, 73, 74};
    if (volume < 0) volume = 0;
    if (volume > 9) volume = 9;
    _volume = volume;
    if (!es_handle)
        return;
    es8311_voice_volume_set(es_handle, volume_lut[volume], NULL);
    digitalWrite(AUDIO_AMP_CTRL, volume > 0 ? HIGH : LOW);
}
void HAL::LOCKLV()
{
    xSemaphoreTake(_mutex, portMAX_DELAY);
}

void HAL::UNLOCKLV()
{
    xSemaphoreGive(_mutex);
}
#include <esp_sleep.h>
#include <driver/rtc_io.h>
void HAL::goSleep()
{
    xSemaphoreTake(appManagerLite._mutex, 2000);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_SERIAL1_RX, 0);
    rtc_gpio_pullup_en((gpio_num_t)PIN_SERIAL1_RX);
    rtc_gpio_hold_en((gpio_num_t)PIN_SERIAL1_RX);
    delay(2);
    esp_deep_sleep_start();
}

void HAL::send_sysctl(system_event_type_t type, uint8_t data)
{
    sysctl_event_t event;
    event.type = type;
    event.data = data;
    xQueueSend(_queue, &event, 20);
}
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

//////////////////////////////////网页服务器部分

#include <WebServer.h>
#include <ESPmDNS.h>

#define FILESYSTEM LittleFS
DNSServer dnsServer;
WebServer server(80);
// holds the current upload
File fsUploadFile;
#include <list>

String getContentType(String filename)
{
    if (server.hasArg("download"))
    {
        return "application/octet-stream";
    }
    else if (filename.endsWith(".htm"))
    {
        return "text/html";
    }
    else if (filename.endsWith(".html"))
    {
        return "text/html";
    }
    else if (filename.endsWith(".css"))
    {
        return "text/css";
    }
    else if (filename.endsWith(".js"))
    {
        return "application/javascript";
    }
    else if (filename.endsWith(".png"))
    {
        return "image/png";
    }
    else if (filename.endsWith(".gif"))
    {
        return "image/gif";
    }
    else if (filename.endsWith(".jpg"))
    {
        return "image/jpeg";
    }
    else if (filename.endsWith(".ico"))
    {
        return "image/x-icon";
    }
    else if (filename.endsWith(".xml"))
    {
        return "text/xml";
    }
    else if (filename.endsWith(".pdf"))
    {
        return "application/x-pdf";
    }
    else if (filename.endsWith(".zip"))
    {
        return "application/x-zip";
    }
    else if (filename.endsWith(".gz"))
    {
        return "application/x-gzip";
    }
    return "text/plain";
}

bool exists(String path)
{
    bool yes = false;
    File file = FILESYSTEM.open(path, "r", false);
    if (!file.isDirectory())
    {
        yes = true;
    }
    file.close();
    return yes;
}

bool handleFileRead(String path)
{
    ESP_LOGI("SERVER", "handleFileRead: %s", path.c_str());
    if (path.endsWith(".wifi.csv"))
        return false;
    String contentType = getContentType(path);
    if (exists(path))
    {
        File file = FILESYSTEM.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

void handleFileUpload()
{
    if (server.uri() != "/edit")
    {
        return;
    }
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if (!filename.startsWith("/"))
        {
            filename = "/" + filename;
        }
        ESP_LOGI("SERVER", "handleFileUpload Name: %s", filename.c_str());
        fsUploadFile = FILESYSTEM.open(filename, "w");
        filename = String();
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (fsUploadFile)
        {
            fsUploadFile.write(upload.buf, upload.currentSize);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (fsUploadFile)
        {
            fsUploadFile.close();
        }
        ESP_LOGI("SERVER", "handleFileUpload Size: %d", upload.totalSize);
    }
}

void handleFileDelete()
{
    if (server.args() == 0)
    {
        return server.send(500, "text/plain", "BAD ARGS");
    }
    String path = server.arg(0);
    ESP_LOGI("SERVER", "handleFileDelete: %s", path.c_str());
    if (path == "/")
    {
        return server.send(500, "text/plain", "BAD PATH");
    }
    if (!exists(path))
    {
        return server.send(404, "text/plain", "FileNotFound");
    }
    bool result = FILESYSTEM.remove(path);
    if (result) {
        server.send(200, "text/plain", "");
    } else {
        return server.send(500, "text/plain", "DELETE FAIL");  
    }
    path = String();
}

void handleFileCreate()
{
    if (server.args() == 0)
    {
        return server.send(500, "text/plain", "BAD ARGS");
    }
    String path = server.arg(0);
    ESP_LOGI("SERVER", "handleFileCreate: %s", path.c_str());
    if (path == "/")
    {
        return server.send(500, "text/plain", "BAD PATH");
    }
    if (exists(path))
    {
        return server.send(500, "text/plain", "FILE EXISTS");
    }
    File file = FILESYSTEM.open(path, "w");
    if (file)
    {
        file.close();
    }
    else
    {
        return server.send(500, "text/plain", "CREATE FAILED");
    }
    server.send(200, "text/plain", "");
    path = String();
}

void handleFileList()
{
    if (!server.hasArg("dir"))
    {
        server.send(500, "text/plain", "BAD ARGS");
        return;
    }

    String path = server.arg("dir");
    ESP_LOGI("SERVER", "handleFileList: %s", path.c_str());

    File root = FILESYSTEM.open(path);
    path = String();
    String output;
    bool first = true;
    output = "{\"size\":";
    output += String(FILESYSTEM.usedBytes());
    output += ",\"total\":";
    output += String(FILESYSTEM.totalBytes());
    output += ",\"data\":[";
    if (root.isDirectory())
    {
        File file = root.openNextFile();
        while (file)
        {
            if (file.name()[0] == '.')
            {
                file = root.openNextFile();
                continue;
            }
            if (first == false)
            {
                output += ',';
            }
            first = false;
            output += "{\"type\":\"";
            output += (file.isDirectory()) ? "dir" : "file";
            output += "\",\"name\":\"";
            output += String(file.name());
            output += "\",\"size\":\"";
            output += String(file.size());
            output += "\"}";
            file = root.openNextFile();
        }
    }
    output += "]}";
    server.send(200, "text/json", output);
}

void handleRMRF()
{
    server.send(500, "text/plain", "此功能已被移除");
}
void handleRename()
{
    server.send(500, "text/plain", "此功能已被移除");
}
void handleMkdir()
{
    server.send(500, "text/plain", "此功能已被移除");
}
void handleTime()
{
    if (server.hasArg("plain"))
    {
        String time = server.arg("plain");
        timeval tv;
        tm newtime;
        tv.tv_sec = atol(time.c_str());
        tv.tv_usec = 0;
        if (tv.tv_sec > 0)
        {
            tv.tv_sec += i18n::getNTPOffset();
            localtime_r(&tv.tv_sec, &newtime);
            hal.setTime(newtime.tm_year + 1900, newtime.tm_mon + 1, newtime.tm_mday, newtime.tm_hour, newtime.tm_min, newtime.tm_sec);
            server.send(200, "text/plain", "OK");
            return;
        }
    }
    server.send(500, "text/plain", "ERR 500");
}
//////////////////////////////////主页APP JSON处理
#include <cJSON.h>

static char jsonbuffer[1024];
const char default_app_setting[] = "{\"ip\":\"192.168.1.1\",\"port\":1648,\"weather\":\"SoC098cCa8Ih-GWTb\",\"city\":\"北京\",\"userdata\":\"请在网页端\n自定义文本\"}";

struct app_setting app_settings_save;

void parseAppSettings(const char *input)
{
    cJSON *root = cJSON_Parse(input);
    if (root == NULL)
    {
        ESP_LOGE("APP", "JSON解析失败");
        cJSON_Delete(root);
        root = cJSON_Parse(default_app_setting);
    }
    strncpy(app_settings_save.remote_ip, cJSON_GetObjectItem(root, "ip")->valuestring, sizeof(app_settings_save.remote_ip) - 1);
    app_settings_save.remote_port = cJSON_GetObjectItem(root, "port")->valueint;
    strncpy(app_settings_save.weather_secret, cJSON_GetObjectItem(root, "weather")->valuestring, sizeof(app_settings_save.weather_secret) - 1);
    strncpy(app_settings_save.weather_city, cJSON_GetObjectItem(root, "city")->valuestring, sizeof(app_settings_save.weather_city) - 1);
    strncpy(app_settings_save.userdata, cJSON_GetObjectItem(root, "userdata")->valuestring, sizeof(app_settings_save.userdata) - 1);
    cJSON_Delete(root);
}

void appSettingsToJson(char *result)
{
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "ip", app_settings_save.remote_ip);
    cJSON_AddNumberToObject(item, "port", app_settings_save.remote_port);
    cJSON_AddStringToObject(item, "weather", app_settings_save.weather_secret);
    cJSON_AddStringToObject(item, "city", app_settings_save.weather_city);
    cJSON_AddStringToObject(item, "userdata", app_settings_save.userdata);
    strncpy(result, cJSON_PrintUnformatted(item), 1024);
    cJSON_Delete(item);
}

void HAL::saveAppSettings()
{
    File file = LittleFS.open("/.cfg.bin", "w");
    file.write((uint8_t *)&app_settings_save, sizeof(app_settings_save));
    file.close();
}

void HAL::loadAppSettings()
{
    File file = LittleFS.open("/.cfg.bin", "r");
    if (file)
    {
        int sz = file.read((uint8_t *)&app_settings_save, sizeof(app_settings_save));
        if (sz != sizeof(app_settings_save))
        {
            file.close();
            goto reset;
        }
        if (file.read() != -1)
        {
            file.close();
            goto reset;
        }
        file.close();
    }
    else
    {
    reset:
        parseAppSettings(default_app_setting);
        file = LittleFS.open("/.cfg.bin", "w");
        file.write((uint8_t *)&app_settings_save, sizeof(app_settings_save));
        file.close();
    }
}
void handleJson()
{
    if (server.method() == HTTP_POST && server.hasArg("plain"))
    {
        parseAppSettings(server.arg("plain").c_str());
        hal.saveAppSettings();
        hal.weather_update = false;
        server.send(200, "text/plain", "OK");
    }
    else if (server.method() == HTTP_GET)
    {
        appSettingsToJson(jsonbuffer);
        server.send(200, "application/json", jsonbuffer);
    }
    {
        server.send(500, "text/plain", "ERR 500");
    }
}

#if 0 // Legacy media-manager SPA: replaced by the compact Wi-Fi setup page below.
#include "webserver/favicon.h"
#include "webserver/index.h"
#include "webserver/js.h"
#include "webserver/ffmpeg.h"
#include "webserver/css.h"
#include "webserver/menu.h"
#include "webserver/close_eye.h"
#include "webserver/i.h"
#include "webserver/local.h"
#include "webserver/open_eye.h"
#include "webserver/wifi.h"
#include "webserver/arrow_left.h"
#include "webserver/arrow_right.h"
#include "webserver/check.h"
#include "webserver/check2.h"
#include "webserver/cpu.h"
#include "webserver/demo.h"
#include "webserver/error_bg.h"
#include "webserver/error_m.h"
#include "webserver/ic_d.h"
#include "webserver/ic_del.h"
#include "webserver/image2.h"
#include "webserver/nothing.h"
#include "webserver/play.h"
#include "webserver/setting2.h"
#include "webserver/spead.h"
#include "webserver/stop.h"
#include "webserver/theme1.h"
#include "webserver/theme2.h"
#include "webserver/theme3.h"
#include "webserver/time_bg.h"
#include "webserver/time_ic.h"
#include "webserver/video2.h"
#include "webserver/weather.h"
#include "webserver/worker.h"
#endif

static const char kWifiSetupPage[] PROGMEM = R"HTML(<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SKYLOONG Screen</title><style>
body{margin:0;background:#10283a;color:#e8f7ff;font:16px system-ui,sans-serif;display:grid;place-items:center;min-height:100vh}
main{width:min(88vw,420px);padding:28px;background:#163a50;border-radius:18px}h1{margin:0 0 8px;color:#7fe8ff}
p{color:#a9c9d8}select,input,button{box-sizing:border-box;width:100%;margin-top:12px;padding:12px;border:0;border-radius:9px;font:inherit}
button{background:#42d6ed;color:#082333;font-weight:700}#status{min-height:24px}
</style></head><body><main><h1>SKYLOONG Screen</h1><p>连接 Wi-Fi</p>
<select id="ssid"><option>正在扫描网络…</option></select><input id="password" type="password" placeholder="Wi-Fi 密码">
<button onclick="save()">保存并连接</button><p id="status"></p></main><script>
const s=document.getElementById('ssid'),p=document.getElementById('password'),m=document.getElementById('status');
fetch('/scan_networks').then(r=>r.json()).then(x=>{s.innerHTML='';x.networks.forEach(n=>{const o=document.createElement('option');o.textContent=n.ssid+' ('+n.rssi+' dBm)';o.value=n.ssid;s.appendChild(o)});if(!s.options.length)s.innerHTML='<option>未发现网络</option>'}).catch(()=>m.textContent='扫描失败，请刷新重试');
function save(){m.textContent='正在保存…';fetch('/config_wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({ssid:s.value,password:p.value})}).then(r=>{if(!r.ok)throw 0;m.textContent='已保存，屏幕将尝试连接'}).catch(()=>m.textContent='保存失败')}</script></body></html>)HTML";


// --- user artwork upload ---------------------------------------------------
// PNG files posted to /art/background and /art/home are decoded into ready-to-draw
// bitmaps, so the GUI never decodes an image while holding the LVGL lock.
static File artUploadFile;
static size_t artUploadBytes = 0;
static bool artUploadOversize = false;
static constexpr size_t artUploadLimit = 6u * 1024u * 1024u;

static const char kArtPage[] PROGMEM = R"HTML(<!doctype html><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1"><title>屏幕图库</title>
<style>
body{font:15px/1.55 -apple-system,system-ui,sans-serif;margin:0;padding:16px;background:#0f141a;color:#e6edf3}
h1{font-size:18px;margin:0 0 6px}.muted{color:#8d9dab;font-size:12px;margin:0 0 14px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(240px,1fr));gap:14px}
.card{background:#1a222b;border-radius:12px;padding:12px}
.hd{display:flex;align-items:center;justify-content:space-between;margin:0 0 8px}
.hd b{font-size:14px}.tag{font-size:11px;color:#7d8c9a}
.dot{width:8px;height:8px;border-radius:50%;background:#3b4756}
.dot.on{background:#4ec9a0}
canvas{width:100%;height:auto;display:block;border-radius:8px;cursor:grab;touch-action:none;
background-color:#101820;
background-image:linear-gradient(45deg,#1b2530 25%,transparent 25%,transparent 75%,#1b2530 75%),
linear-gradient(45deg,#1b2530 25%,transparent 25%,transparent 75%,#1b2530 75%);
background-size:16px 16px;background-position:0 0,8px 8px}
canvas:active{cursor:grabbing}
.row{display:flex;gap:8px;align-items:center;margin:8px 0 0}
input[type=range]{flex:1}
button{border:0;border-radius:8px;padding:8px 14px;background:#2f81f7;color:#fff;font-size:13px}
button.ghost{background:#2b3542}
label.file{display:inline-block;background:#243040;color:#cfdae6;border-radius:8px;padding:7px 12px;font-size:12px;cursor:pointer}
label.file input{display:none}
.out{font-size:12px;color:#8d9dab;margin:6px 0 0;min-height:16px}
.lbl{font-size:11px;color:#8d9dab;min-width:26px}
.pv{width:100%;border-radius:8px;display:block;background:#0b1016;margin:0 0 8px}
</style>
<h1>屏幕图库</h1>
<p class=muted>拖动图片调整位置，滑杆缩放；上传前会按屏幕比例裁切，预览即最终效果。</p>
<div class=grid id=grid></div>
<p class=muted>仅支持 PNG，单张 ≤6MB。背景只在对应天气/时间段生效，点「恢复默认」回到内置场景。</p>
<script>
var SLOTS=[["clear","白天·晴",320,240],["cloudy","白天·多云",320,240],["rain","白天·雨",320,240],
["snow","白天·雪",320,240],["dusk","黄昏 17-20",320,240],["night","夜间 20-6",320,240],
["home","主页左侧图片",176,226]];
var ST={},GRID=document.getElementById('grid');
function card(id,label,w,h){
  var el=document.createElement('div');el.className='card';
  el.innerHTML='<div class=hd><b>'+label+'</b><span class=tag id=t_'+id+'>内置</span></div>'
   +'<canvas id=c_'+id+' width='+w+' height='+h+'></canvas>'
   +'<div class=row><label class=file>选择图片<input type=file accept="image/png" id=f_'+id+'></label>'
   +'<button id=u_'+id+'>上传</button><button class=ghost id=r_'+id+'>恢复默认</button></div>'
   +'<div class=row><span class=lbl>缩放</span><input type=range id=z_'+id+' min=40 max=400 value=100></div>'
   +(id==='home'?'<div class=row><span class=lbl>水平</span><input type=range id=lx min=-120 max=120 value=-6></div>'
                 +'<div class=row><span class=lbl>垂直</span><input type=range id=ly min=-120 max=120 value=2></div>'
                 +'<div class=row><span class=lbl>大小</span><input type=range id=lz min=96 max=512 value=248></div>'
                 +'<div class=row><button id=la>应用位置与大小</button></div>':'')
   +(id==='home'?'<div class=row><img id=pv_home class=pv alt="主界面效果" src="/art/preview?name=home-screen"></div>':'')
   +'<div class=out id=o_'+id+'></div>';
  GRID.appendChild(el);
  var c=document.getElementById('c_'+id),s=ST[id]={img:null,zoom:1,ox:0,oy:0,w:w,h:h};
  document.getElementById('f_'+id).onchange=function(e){
    var f=e.target.files[0];if(!f)return;
    var img=new Image();img.onload=function(){s.img=img;s.zoom=1;s.ox=0;s.oy=0;
      document.getElementById('z_'+id).value=100;draw(id);};img.src=URL.createObjectURL(f);};
  document.getElementById('z_'+id).oninput=function(e){s.zoom=e.target.value/100;clamp(id);draw(id);};
  c.onpointerdown=function(e){s.drag={x:e.offsetX,y:e.offsetY,ox:s.ox,oy:s.oy};
    var sc=c.width/c.clientWidth;s.drag.x*=sc;s.drag.y*=sc;c.setPointerCapture(e.pointerId);};
  c.onpointermove=function(e){if(!s.drag)return;var sc=c.width/c.clientWidth;
    s.ox=s.drag.ox+(e.offsetX*sc-s.drag.x);s.oy=s.drag.oy+(e.offsetY*sc-s.drag.y);clamp(id);draw(id);};
  c.onpointerup=function(){s.drag=null;};
  document.getElementById('u_'+id).onclick=function(){upload(id,label,w,h);};
  if(id==='home') document.getElementById('la').onclick=function(){applyLayout();};
  document.getElementById('r_'+id).onclick=function(){reset(id);};
  draw(id);
}
function geo(s){var g={};if(!s.img)return g;
  var base=Math.max(s.w/s.img.width,s.h/s.img.height),k=base*s.zoom;
  g.w=s.img.width*k;g.h=s.img.height*k;g.x=(s.w-g.w)/2+s.ox;g.y=(s.h-g.h)/2+s.oy;return g;}
function clamp(id){var s=ST[id],g=geo(s);if(!g.w)return;
  var mx=g.w>=s.w?(g.w-s.w)/2:s.w/2, my=g.h>=s.h?(g.h-s.h)/2:s.h/2;
  s.ox=Math.min(mx,Math.max(-mx,s.ox));s.oy=Math.min(my,Math.max(-my,s.oy));}
function draw(id){var s=ST[id],c=document.getElementById('c_'+id),x=c.getContext('2d');
  x.clearRect(0,0,s.w,s.h);
  if(!s.img){x.fillStyle='#7d8c9a';x.font='12px sans-serif';x.textAlign='center';
    x.fillText('未选择图片',s.w/2,s.h/2);return;}
  var g=geo(s);x.drawImage(s.img,g.x,g.y,g.w,g.h);}
function upload(id,label,w,h){
  var s=ST[id],o=document.getElementById('o_'+id);
  if(!s.img){o.textContent='请先选择图片';return;}
  o.textContent='处理中…';
  var cv=document.createElement('canvas');
  cv.width=w;cv.height=h;
  var g=geo(s);
  cv.getContext('2d').drawImage(s.img,g.x,g.y,g.w,g.h);
  cv.toBlob(function(b){
    var d=new FormData();d.append('file',b,id+'.png');
    var url=(id==='home')?'/art/home':'/art/bg/'+id;
    fetch(url,{method:'POST',body:d}).then(function(r){return r.json();}).then(function(j){
      o.textContent=j.ok?'已保存，屏幕立即生效':'失败：'+(j.error||'');mark(id,j.ok);
      if(j.ok&&id==='home') refreshPreview();
    }).catch(function(e){o.textContent='失败：'+e;});
  },'image/png');
}
function reset(id){
  var o=document.getElementById('o_'+id);
  var url=(id==='home')?'/art/clear-home':'/art/clear-bg/'+id;
  fetch(url,{method:'POST'}).then(function(r){return r.json();}).then(function(){
    o.textContent='已恢复默认';mark(id,false);
    if(id==='home') refreshPreview();}).catch(function(e){o.textContent='失败：'+e;});
}
function refreshPreview(){
  var im=document.getElementById('pv_home');
  if(im) im.src='/art/preview?name=home-screen&t='+Date.now();
}
function applyLayout(){
  var o=document.getElementById('o_home');
  var d=new URLSearchParams();
  d.append('x',document.getElementById('lx').value);
  d.append('y',document.getElementById('ly').value);
  d.append('zoom',document.getElementById('lz').value);
  o.textContent='应用中…';
  fetch('/art/home-layout',{method:'POST',body:d}).then(function(r){return r.json();}).then(function(j){
    o.textContent=j.ok?'位置与大小已应用到屏幕':'失败：'+(j.error||'');
    if(j.ok) refreshPreview();}).catch(function(e){o.textContent='失败：'+e;});
}
function mark(id,on){var t=document.getElementById('t_'+id);t.textContent=on?'已自定义':'内置';t.style.color=on?'#4ec9a0':'#7d8c9a';}
SLOTS.forEach(function(s){card(s[0],s[1],s[2],s[3]);});
fetch('/art/state').then(function(r){return r.json();}).then(function(j){
  ['clear','cloudy','rain','snow','dusk','night','home'].forEach(function(k){mark(k,!!j[k]);});
  if(j.homeX!=null){
    document.getElementById('lx').value=j.homeX;
    document.getElementById('ly').value=j.homeY;
    document.getElementById('lz').value=j.homeZoom;
  }}).catch(function(){});
</script>)HTML";

static void handleArtUpload()
{
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        artUploadBytes = 0;
        artUploadOversize = false;
        artUploadFile = LittleFS.open("/art-upload.tmp", "w");
        ESP_LOGI("SERVER", "art upload start: %s", upload.filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (artUploadFile && !artUploadOversize) {
            if (artUploadBytes + upload.currentSize > artUploadLimit) {
                artUploadOversize = true;
            } else {
                artUploadFile.write(upload.buf, upload.currentSize);
                artUploadBytes += upload.currentSize;
            }
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (artUploadFile) artUploadFile.close();
        ESP_LOGI("SERVER", "art upload end: %u bytes", unsigned(artUploadBytes));
    }
}

// Smallest possible encoder for the web previews: a 24-bit BMP. Rows are bottom-up
// and the panel only carries RGB565, so the low bits are rebuilt from the high ones.
static void sendBmpFromRgb565(const uint8_t *pixels, int width, int height)
{
    const size_t stride = size_t(width) * 3;
    const size_t padded = (stride + 3) & ~size_t(3);
    const size_t imageBytes = padded * size_t(height);
    const size_t total = 54 + imageBytes;
    auto *out = (uint8_t *)ps_malloc(total);
    if (!out) {
        server.send(500, "text/plain", "oom");
        return;
    }
    memset(out, 0, total);
    out[0] = 'B';
    out[1] = 'M';
    const uint32_t fileSize = uint32_t(total);
    const uint32_t dataOffset = 54;
    const uint32_t headerSize = 40;
    const uint32_t imageSize = uint32_t(imageBytes);
    const int32_t w32 = width, h32 = height;
    const uint16_t planes = 1, bits = 24;
    memcpy(out + 2, &fileSize, 4);
    memcpy(out + 10, &dataOffset, 4);
    memcpy(out + 14, &headerSize, 4);
    memcpy(out + 18, &w32, 4);
    memcpy(out + 22, &h32, 4);
    memcpy(out + 26, &planes, 2);
    memcpy(out + 28, &bits, 2);
    memcpy(out + 34, &imageSize, 4);
    for (int y = 0; y < height; ++y) {
        uint8_t *dst = out + 54 + padded * size_t(height - 1 - y);
        for (int x = 0; x < width; ++x) {
            const uint16_t v = uint16_t(pixels[2 * (size_t(y) * width + x)] |
                                        (pixels[2 * (size_t(y) * width + x) + 1] << 8));
            dst[3 * x + 0] = uint8_t(unsigned(v & 0x1F) * 255u / 31u);
            dst[3 * x + 1] = uint8_t(unsigned((v >> 5) & 0x3F) * 255u / 63u);
            dst[3 * x + 2] = uint8_t(unsigned((v >> 11) & 0x1F) * 255u / 31u);
        }
    }
    server.sendHeader("Cache-Control", "no-store");
    server.setContentLength(total);
    server.send(200, "image/bmp", "");
    server.sendContent(reinterpret_cast<const char *>(out), total);
    free(out);
}

// The plain cut-out, alpha composited on the page grey.
static void sendArtCutout(const uint8_t *pixels, int width, int height)
{
    auto *rgb = (uint8_t *)ps_malloc(size_t(width) * height * 2);
    if (!rgb) {
        server.send(500, "text/plain", "oom");
        return;
    }
    for (int i = 0; i < width * height; ++i) {
        const uint16_t v = uint16_t(pixels[3 * i] | (pixels[3 * i + 1] << 8));
        const unsigned a = pixels[3 * i + 2];
        const unsigned r = (unsigned((v >> 11) & 0x1F) * 255u / 31u * a + 15u * (255u - a)) / 255u;
        const unsigned g = (unsigned((v >> 5) & 0x3F) * 255u / 63u * a + 20u * (255u - a)) / 255u;
        const unsigned b = (unsigned(v & 0x1F) * 255u / 31u * a + 26u * (255u - a)) / 255u;
        const uint16_t out = uint16_t(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
        rgb[2 * i] = uint8_t(out & 0xFF);
        rgb[2 * i + 1] = uint8_t(out >> 8);
    }
    sendBmpFromRgb565(rgb, width, height);
    free(rgb);
}

// A mock of the home screen: current backdrop under the same veil the page uses,
// with the uploaded cut-out placed exactly where the panel draws it.
static void sendHomeScreenPreview()
{
    constexpr int kWidth = 320;
    constexpr int kHeight = 240;
    constexpr unsigned kVeilRed = 0x0A, kVeilGreen = 0x16, kVeilBlue = 0x22, kVeil = 90;

    int backdropWidth = 0, backdropHeight = 0;
    const uint8_t *backdrop = atmosphereCurrentPixels(&backdropWidth, &backdropHeight);
    uint8_t *art = artstore::loadHomeImage();

    auto *frame = (uint8_t *)ps_malloc(size_t(kWidth) * kHeight * 2);
    if (!frame) {
        free(art);
        server.send(500, "text/plain", "oom");
        return;
    }
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            unsigned r = kVeilRed, g = kVeilGreen, b = kVeilBlue;
            if (backdrop && x < backdropWidth && y < backdropHeight) {
                const size_t index = size_t(y) * size_t(backdropWidth) + size_t(x);
                const uint16_t v = uint16_t(backdrop[2 * index] | (backdrop[2 * index + 1] << 8));
                const unsigned br = unsigned((v >> 11) & 0x1F) * 255u / 31u;
                const unsigned bg = unsigned((v >> 5) & 0x3F) * 255u / 63u;
                const unsigned bb = unsigned(v & 0x1F) * 255u / 31u;
                r = (br * (255u - kVeil) + kVeilRed * kVeil) / 255u;
                g = (bg * (255u - kVeil) + kVeilGreen * kVeil) / 255u;
                b = (bb * (255u - kVeil) + kVeilBlue * kVeil) / 255u;
            }
            const uint16_t out = uint16_t(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
            const size_t index = size_t(y) * kWidth + x;
            frame[2 * index] = uint8_t(out & 0xFF);
            frame[2 * index + 1] = uint8_t(out >> 8);
        }
    }

    if (art) {
        const artstore::HomeLayout layout = artstore::homeLayout();
        const int zoom = layout.zoom > 0 ? layout.zoom : 256;
        const int drawWidth = artstore::homeWidth * zoom / 256;
        const int drawHeight = artstore::homeHeight * zoom / 256;
        for (int dy = 0; dy < drawHeight; ++dy) {
            const int sy = dy * 256 / zoom;
            const int py = layout.y + dy;
            if (sy < 0 || sy >= artstore::homeHeight || py < 0 || py >= kHeight) continue;
            for (int dx = 0; dx < drawWidth; ++dx) {
                const int sx = dx * 256 / zoom;
                const int px = layout.x + dx;
                if (sx < 0 || sx >= artstore::homeWidth || px < 0 || px >= kWidth) continue;
                const uint8_t *src = art + 3ull * (size_t(sy) * artstore::homeWidth + size_t(sx));
                const unsigned a = src[2];
                if (!a) continue;
                const uint16_t v = uint16_t(src[0] | (src[1] << 8));
                const unsigned sr = unsigned((v >> 11) & 0x1F) * 255u / 31u;
                const unsigned sg = unsigned((v >> 5) & 0x3F) * 255u / 63u;
                const unsigned sb = unsigned(v & 0x1F) * 255u / 31u;
                uint8_t *dst = frame + 2 * (size_t(py) * kWidth + size_t(px));
                const uint16_t d = uint16_t(dst[0] | (dst[1] << 8));
                const unsigned dr = unsigned((d >> 11) & 0x1F) * 255u / 31u;
                const unsigned dg = unsigned((d >> 5) & 0x3F) * 255u / 63u;
                const unsigned db = unsigned(d & 0x1F) * 255u / 31u;
                const unsigned r = (sr * a + dr * (255u - a)) / 255u;
                const unsigned g = (sg * a + dg * (255u - a)) / 255u;
                const unsigned b = (sb * a + db * (255u - a)) / 255u;
                const uint16_t out = uint16_t(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
                dst[0] = uint8_t(out & 0xFF);
                dst[1] = uint8_t(out >> 8);
            }
        }
        free(art);
    }
    sendBmpFromRgb565(frame, kWidth, kHeight);
    free(frame);
}

static void sendArtPreview()
{
    const String name = server.arg("name");
    if (name == "home-screen") {
        sendHomeScreenPreview();
        return;
    }
    if (name == "home") {
        uint8_t *art = artstore::loadHomeImage();
        if (!art) {
            server.send(404, "text/plain", "none");
            return;
        }
        sendArtCutout(art, artstore::homeWidth, artstore::homeHeight);
        free(art);
        return;
    }
    const int slot = artstore::backgroundSlotFromName(name.c_str());
    if (slot < 0) {
        server.send(404, "text/plain", "none");
        return;
    }
    uint8_t *pixels = artstore::loadBackground(slot);
    if (!pixels) {
        server.send(404, "text/plain", "none");
        return;
    }
    sendBmpFromRgb565(pixels, artstore::backgroundWidth, artstore::backgroundHeight);
    free(pixels);
}

static void finishArtUpload(bool background, int slot)
{
    char error[64] = {0};
    bool ok = false;
    if (artUploadOversize) {
        snprintf(error, sizeof(error), "文件过大（上限 6MB）");
    } else if (!artUploadBytes) {
        snprintf(error, sizeof(error), "没有收到文件");
    } else {
        File f = LittleFS.open("/art-upload.tmp", "r");
        auto *buf = (uint8_t *)ps_malloc(artUploadBytes);
        const bool opened = (bool)f;
        size_t got = 0;
        if (opened) {
            if (buf) got = f.read(buf, artUploadBytes);
            f.close();
        }
        if (!buf) {
            snprintf(error, sizeof(error), "内存不足");
        } else if (!opened || got != artUploadBytes) {
            snprintf(error, sizeof(error), "读取上传内容失败");
        } else {
            ok = background ? artstore::saveBackground(slot, buf, artUploadBytes, error, sizeof(error))
                            : artstore::saveHomeImage(buf, artUploadBytes, error, sizeof(error));
        }
        if (buf) free(buf);
    }
    LittleFS.remove("/art-upload.tmp");
    artUploadBytes = 0;
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "ok", ok);
    if (!ok) cJSON_AddStringToObject(json, "error", error);
    char *text = cJSON_PrintUnformatted(json);
    server.send(ok ? 200 : 400, "application/json", text);
    free(text);
    cJSON_Delete(json);
    if (background)
        ESP_LOGI("SERVER", "art '%s': %s", artstore::backgroundSlotName(slot), ok ? "saved" : error);
    else
        ESP_LOGI("SERVER", "art home: %s", ok ? "saved" : error);
}

void HAL::start_webserver()
{
    if (webserver_task != NULL)
        return;
    hal.server_started = true;

    if (WiFi.getMode() == WIFI_OFF || (WiFi.getMode() == WIFI_STA && WiFi.isConnected() == false))
    {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("SKYLOONG 4.0 Screen");
    }

    MDNS.begin("skyloong-screen");
    server.enableCORS(true);

    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
        server.onNotFound([]() {
            server.sendHeader("Location", "http://192.168.4.1/wifi");
            server.send(302, "text/plain", "redirect to captive portal");
        });
    } else {
        server.onNotFound([]() { server.send(404, "text/plain", "FileNotFound"); });
    }

    // 调试/验证用：/bgm?vol=5 或 /bgm?slot=5&vol=5
        server.on("/info", HTTP_GET, []() {
        cJSON *json = cJSON_CreateObject();

        if (WiFi.getMode() == WIFI_AP) {
            cJSON_AddStringToObject(json, "mode", "AP");
            cJSON_AddStringToObject(json, "ssid", "SKYLOONG 4.0 Screen");
            cJSON_AddStringToObject(json, "ip", "192.168.4.1");
        } else {
            cJSON_AddStringToObject(json, "mode", "STA");
            cJSON_AddStringToObject(json, "ssid", WiFi.SSID().c_str());
            cJSON_AddStringToObject(json, "ip", WiFi.localIP().toString().c_str());
        }

        cJSON_AddBoolToObject(json, "weather_enable", hal.weather_enable);
        long timezone = i18n::getNTPOffset();
        timezone /= 3600;
        cJSON_AddNumberToObject(json, "timezone", timezone);
        cJSON_AddNumberToObject(json, "language", i18n::getLanguage());

        strncpy(jsonbuffer, cJSON_PrintUnformatted(json), 1024);
        cJSON_Delete(json);
        server.send(200, "application/json", jsonbuffer);
    });

    server.on("/scan_networks", HTTP_GET, []() {
        wifi_mode_t mode = WiFi.getMode();
        int8_t result = WiFi.scanNetworks(false, false);
        if (mode == WIFI_AP) {
            WiFi.mode(WIFI_AP);
        }

        cJSON *json = cJSON_CreateObject();
        cJSON *array = cJSON_AddArrayToObject(json, "networks");

        if (result >= 0) {
            for (int i = 0; i < result; ++i) {
                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "ssid", WiFi.SSID(i).c_str());
                cJSON_AddNumberToObject(item, "rssi", WiFi.RSSI(i));
                cJSON_AddItemToArray(array, item);
            }
        }

        strncpy(jsonbuffer, cJSON_PrintUnformatted(json), 1024);
        cJSON_Delete(json);
        server.send(200, "application/json", jsonbuffer);
    });

    server.on("/config_wifi", HTTP_POST, []() {
        if (server.hasArg("ssid") && server.hasArg("password")) {
            strcpy(hal.ssid, server.arg("ssid").c_str());
            strcpy(hal.password, server.arg("password").c_str());
            hal.config_wifi = true;
            server.send(200, "text/plain", "OK");
        } else {
            server.send(500, "text/plain", "ERR 500");
        }
    });

    server.on("/config_timezone", HTTP_POST, []() {
        if (server.hasArg("timezone")) {
            long timezone = server.arg("timezone").toInt();
            if (timezone >= -12 && timezone <= 12) {
                timezone *= 3600;
                i18n::setNTPOffset(timezone);
                hal.pref.putInt("ntp", timezone);
                if(hal.NTPSync())
                    hal.time_sync = true;
            } else {
               server.send(500, "text/plain", "ERR 500"); 
            }

            server.send(200, "text/plain", "OK");
        } else {
            server.send(500, "text/plain", "ERR 500");
        }
    });

    server.on("/config_language", HTTP_POST, []() {
        if (server.hasArg("language")) {
            long language = server.arg("language").toInt();
            if (language == 0 || language == 1) {
                i18n::setLanguage(language);
                hal.pref.putUInt("lang", language);
                hal.lang_refresh = true;
            } else {
               server.send(500, "text/plain", "ERR 500"); 
            }

            server.send(200, "text/plain", "OK");
        } else {
            server.send(500, "text/plain", "ERR 500");
        }
    });

    server.on("/config_app_weather", HTTP_POST, []() {
        if (server.hasArg("enable")) {
            hal.weather_enable = server.arg("enable").toBool();
            hal.pref.putBool("weather_enable", hal.weather_enable);
            server.send(200, "text/plain", "OK");
        } else {
            server.send(500, "text/plain", "ERR 500");
        }
    });

    server.on("/music", HTTP_POST, []() {
        String payload = server.arg("plain");
        if (payload.length() == 0 || payload.length() > 2048) {
            server.send(400, "text/plain", "invalid music payload");
            return;
        }
        cJSON *root = cJSON_Parse(payload.c_str());
        if (root == nullptr) {
            server.send(400, "text/plain", "invalid json");
            return;
        }

        const bool active = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "active"));
        const bool playing = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "playing"));
        const cJSON *titleItem = cJSON_GetObjectItemCaseSensitive(root, "title");
        const cJSON *artistItem = cJSON_GetObjectItemCaseSensitive(root, "artist");
        const cJSON *lyricItem = cJSON_GetObjectItemCaseSensitive(root, "lyric");
        const cJSON *nextItem = cJSON_GetObjectItemCaseSensitive(root, "next");
        const cJSON *positionItem = cJSON_GetObjectItemCaseSensitive(root, "position_ms");
        const cJSON *durationItem = cJSON_GetObjectItemCaseSensitive(root, "duration_ms");
        const char *title = cJSON_IsString(titleItem) ? titleItem->valuestring : "";
        const char *artist = cJSON_IsString(artistItem) ? artistItem->valuestring : "";

        bool wasActive;
        bool wasPlaying;
        hal.LOCKLV();
        wasActive = hal.music.active;
        wasPlaying = hal.music.playing;
        const bool changedTrack = strncmp(hal.music.title, title, sizeof(hal.music.title) - 1) != 0;
        hal.music.active = active;
        hal.music.playing = playing;
        hal.music.position_ms = cJSON_IsNumber(positionItem) ? (uint32_t)positionItem->valuedouble : 0;
        hal.music.duration_ms = cJSON_IsNumber(durationItem) ? (uint32_t)durationItem->valuedouble : 0;
        strlcpy(hal.music.title, title, sizeof(hal.music.title));
        strlcpy(hal.music.artist, artist, sizeof(hal.music.artist));
        strlcpy(hal.music.lyric, cJSON_IsString(lyricItem) ? lyricItem->valuestring : "", sizeof(hal.music.lyric));
        strlcpy(hal.music.next_lyric, cJSON_IsString(nextItem) ? nextItem->valuestring : "", sizeof(hal.music.next_lyric));
        ++hal.music.revision;
        hal.UNLOCKLV();
        cJSON_Delete(root);

        if (active && playing && (!wasActive || !wasPlaying || changedTrack))
            hal.send_sysctl(EVENT_GOTO_MUSIC);
        else if ((!active && wasActive) || (active && !playing && wasPlaying))
            hal.send_sysctl(EVENT_EXIT_MUSIC);
        server.send(200, "text/plain", "OK");
    });

    server.on("/time", HTTP_POST, handleTime);
    server.on("/reboot", HTTP_POST, []() {
        server.send(200, "text/plain", "OK");
        delay(100);
        ESP.restart(); 
    });
    server.on("/config.json", handleJson);
    server.on("/", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", kWifiSetupPage); });
    server.on("/wifi", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", kWifiSetupPage); });

#if 0 // Legacy media-manager routes and assets.
    server.on("/favicon.ico", HTTP_GET, []() { 
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/x-icon", (const char *)__web_favicon_ico_gz, sizeof(__web_favicon_ico_gz));
    });
    server.on("/", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "text/html", (const char *)__web_index_html_gz, sizeof(__web_index_html_gz)); 
    });
    server.on("/wifi", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "text/html", (const char *)__web_index_html_gz, sizeof(__web_index_html_gz)); 
    });
    server.on("/index.js", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "application/javascript", (const char *)__web_index_js_gz, sizeof(__web_index_js_gz));
    });
    server.on("/ffmpeg.js", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "application/javascript", (const char *)__web_ffmpeg_js_gz, sizeof(__web_ffmpeg_js_gz));
    });
    server.on("/index.css", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "text/css", (const char *)__web_index_css_gz, sizeof(__web_index_css_gz));
    });

    server.on("/menu.jpg", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/jpeg", (const char *)__web_menu_jpg_gz, sizeof(__web_menu_jpg_gz));
    });
    server.on("/close_eye.svg", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/svg+xml", (const char *)__web_close_eye_svg_gz, sizeof(__web_close_eye_svg_gz));
    });
    server.on("/i.svg", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/svg+xml", (const char *)__web_i_svg_gz, sizeof(__web_i_svg_gz));
    });
    server.on("/local.svg", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/svg+xml", (const char *)__web_local_svg_gz, sizeof(__web_local_svg_gz));
    });
    server.on("/open_eye.svg", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/svg+xml", (const char *)__web_open_eye_svg_gz, sizeof(__web_open_eye_svg_gz));
    });
    server.on("/wifi.svg", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/svg+xml", (const char *)__web_wifi_svg_gz, sizeof(__web_wifi_svg_gz));
    });
    server.on("/arrow_left.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_arrow_left_png_gz, sizeof(__web_arrow_left_png_gz));
    });
    server.on("/arrow_right.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_arrow_right_png_gz, sizeof(__web_arrow_right_png_gz));
    });
    server.on("/check.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_check_png_gz, sizeof(__web_check_png_gz));
    });
    server.on("/check2.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_check2_png_gz, sizeof(__web_check2_png_gz));
    });
    server.on("/cpu.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_cpu_png_gz, sizeof(__web_cpu_png_gz));
    });
    server.on("/demo.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_demo_png_gz, sizeof(__web_demo_png_gz));
    });
    server.on("/error_bg.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_error_bg_png_gz, sizeof(__web_error_bg_png_gz));
    });
    server.on("/error_m.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_error_m_png_gz, sizeof(__web_error_m_png_gz));
    });
    server.on("/ic_d.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_ic_d_png_gz, sizeof(__web_ic_d_png_gz));
    });
    server.on("/ic_del.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_ic_del_png_gz, sizeof(__web_ic_del_png_gz));
    });
    server.on("/image2.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_image2_png_gz, sizeof(__web_image2_png_gz));
    });
    server.on("/nothing.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_nothing_png_gz, sizeof(__web_nothing_png_gz));
    });
    server.on("/play.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_play_png_gz, sizeof(__web_play_png_gz));
    });
    server.on("/setting2.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_setting2_png_gz, sizeof(__web_setting2_png_gz));
    });
    server.on("/spead.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_spead_png_gz, sizeof(__web_spead_png_gz));
    });
    server.on("/stop.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_stop_png_gz, sizeof(__web_stop_png_gz));
    });
    server.on("/theme1.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_theme1_png_gz, sizeof(__web_theme1_png_gz));
    });
    server.on("/theme2.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_theme2_png_gz, sizeof(__web_theme2_png_gz));
    });
    server.on("/theme3.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_theme3_png_gz, sizeof(__web_theme3_png_gz));
    });
    server.on("/time_bg.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_time_bg_png_gz, sizeof(__web_time_bg_png_gz));
    });
    server.on("/time_ic__.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_time_ic___png_gz, sizeof(__web_time_ic___png_gz));
    });
    server.on("/video2.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_video2_png_gz, sizeof(__web_video2_png_gz));
    });
    server.on("/weather.png", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "image/png", (const char *)__web_weather_png_gz, sizeof(__web_weather_png_gz));
    });
    server.on("/assets/worker-224792ee.js", HTTP_GET, []() {
        server.sendHeader("Content-Encoding", "gzip", true);
        server.send_P(200, "application/javascript", (const char *)__web_assets_worker_224792ee_js_gz, sizeof(__web_assets_worker_224792ee_js_gz));
    });
#endif


    server.on("/art", HTTP_GET, []() {
        server.send_P(200, "text/html; charset=utf-8", kArtPage);
    });
    server.on("/art/state", HTTP_GET, []() {
        cJSON *json = cJSON_CreateObject();
        for (int i = 0; i < artstore::backgroundSlots; ++i)
            cJSON_AddBoolToObject(json, artstore::backgroundSlotName(i), artstore::hasBackground(i));
        cJSON_AddBoolToObject(json, "home", artstore::hasHomeImage());
        const artstore::HomeLayout layout = artstore::homeLayout();
        cJSON_AddNumberToObject(json, "homeX", layout.x);
        cJSON_AddNumberToObject(json, "homeY", layout.y);
        cJSON_AddNumberToObject(json, "homeZoom", layout.zoom);
        char *text = cJSON_PrintUnformatted(json);
        server.send(200, "application/json", text);
        free(text);
        cJSON_Delete(json);
    });
    server.on("/art/home", HTTP_POST, []() { finishArtUpload(false, -1); }, handleArtUpload);
    server.on("/art/clear-home", HTTP_POST, []() {
        artstore::clearHomeImage();
        server.send(200, "application/json", "{\"ok\":true}");
    });
    server.on("/art/preview", HTTP_GET, []() { sendArtPreview(); });
    server.on("/art/home-layout", HTTP_POST, []() {
        const int x = server.arg("x").toInt();
        const int y = server.arg("y").toInt();
        const int zoom = server.arg("zoom").toInt();
        artstore::saveHomeLayout(x, y, zoom);
        server.send(200, "application/json", "{\"ok\":true}");
    });
    for (int i = 0; i < artstore::backgroundSlots; ++i) {
        String upload = String("/art/bg/") + artstore::backgroundSlotName(i);
        String clear = String("/art/clear-bg/") + artstore::backgroundSlotName(i);
        server.on(upload, HTTP_POST, [i]() { finishArtUpload(true, i); }, handleArtUpload);
        server.on(clear, HTTP_POST, [i]() {
            artstore::clearBackground(i);
            server.send(200, "application/json", "{\"ok\":true}");
        });
    }

    server.begin();
    ESP_LOGI("SERVER", "HTTP server started");
    GUI::toast(_tr(I18N_ID_SERVER_STARTED));
    xTaskCreatePinnedToCore([](void *p)
    {
        while (1)
        {
            if(hal.server_started) {
                server.handleClient();
            }
            vTaskDelay(5);
        }
    }, "webserver", 4096, NULL, 3, &webserver_task, 1);
}

void HAL::stop_webserver()
{
    if (webserver_task == NULL)
        return;
    server.stop();
    if (WiFi.getMode() == WIFI_AP)
    {
        WiFi.mode(WIFI_OFF);
    }
    hal.server_started = false;
    delay(50);
    vTaskDelete(webserver_task);
    webserver_task = NULL;
    GUI::toast(_tr(I18N_ID_SERVER_STOPPED));
}
extern bool stop_protocol;
void HAL::forceExitSettings()
{
    stop_protocol = true;
    hal.send_sysctl(EVENT_EXIT_SETTING);
}
