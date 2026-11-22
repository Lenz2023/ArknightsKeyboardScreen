#pragma once
#include "lvgl.h"
#include "Arduino.h"
#include "SklandApi.h"
#include <stack>
class BaseApp
{
public:
    bool _addToSwitchApp = true; // 是否将当前App加入到切换列表
    uint32_t appid = 0;
    lv_obj_t *_appScreen = NULL;
    virtual void setup() = 0;
    virtual void loop() = 0;
    virtual void destroy() = 0; // 销毁当前App，注意应该在此处释放LVGL定时器
    virtual void stop(){};      // 停止loop中正在运行的循环
    virtual bool nextPage() { return false; }
};

extern uint32_t RTC_DATA_ATTR last_appid;
class AppManagerLite
{
public:
    BaseApp *currentApp = NULL;
    BaseApp *lastAppBeforeSetting = NULL;
    BaseApp *appHome = NULL;
    BaseApp *appWeather = NULL;
    BaseApp *appSettings = NULL;
    BaseApp *appQRCode = NULL;
    BaseApp *appMusic = NULL;
    BaseApp *appSklandLogin = NULL;
    BaseApp *appSklandGeneral = NULL;
    BaseApp *appSklandGarrison = NULL;
    BaseApp *requestedApp = NULL;
    BaseApp *getAppByName(const uint32_t appid);
    void init(const uint32_t lastAppid);
    void switchApp(BaseApp *app);
    void switchApp(const uint32_t appid)
    {
        switchApp(getAppByName(appid));
    }
    void switchNext() {
        BaseApp *nextApp = NULL;
        if (currentApp != NULL) {
            switch (currentApp->appid) {
                case 1:
                    if (hal.weather_enable) {
                        nextApp = appWeather;
                        break;   
                    }
                    nextApp = appMusic;
                    break;
                case 5:
                    nextApp = appMusic;
                    break;
                case 7:
                    nextApp = SklandApi::hasSavedAccount() ? appSklandGeneral : appSklandLogin;
                    break;
                case 8:
                    nextApp = appHome;
                    break;
                case 9:
                    if (currentApp->nextPage()) return;
                    nextApp = appSklandGarrison;
                    break;
                case 10:
                    nextApp = appHome;
                    break;
                case 50:
                case 100:
                    nextApp = appHome;
                    break;
            }
        }

        switchApp(nextApp);
    }
    void switchSetting()
    {
        if (currentApp->appid == 50)
            return;
        lastAppBeforeSetting = currentApp;
        switchApp(50);
    }
    void exitSetting()
    {
        if (currentApp->appid == 50)
            switchApp(lastAppBeforeSetting);
    }
    void switchQRCode()
    {
        if (currentApp->appid == 100)
            return;
        switchApp(100);
    }
    void exitQRCode()
    {
        if (currentApp->appid == 100) {
            switchApp(1);
        }
    }
    void openMusic()
    {
        if (currentApp != appMusic)
            switchApp(appMusic);
    }
    void exitMusic()
    {
        if (currentApp != NULL && currentApp->appid == 7)
            switchApp(appHome);
    }
    void openSklandLogin()
    {
        if (currentApp != appSklandLogin)
            switchApp(appSklandLogin);
    }
    void requestApp(BaseApp *app) { requestedApp = app; }
    enum HomePage { Garrison, Clock, Weather, Music, Profile, Daily };
    uint8_t homePage() const {
        uint8_t choice = hal.pref.getUChar("boot_page", Garrison);
        return choice <= Daily ? choice : static_cast<uint8_t>(Garrison);
    }
    bool startDaily = false;
    bool loginForHome = false;
    uint32_t bootApp() {
        const auto choice = homePage();
        if (choice == Clock) return 1;
        if (choice == Weather) return 5;
        if (choice == Music) return 7;
        startDaily = choice == Daily;
        loginForHome = !SklandApi::hasSavedAccount();
        if (loginForHome) return 8;
        return choice == Garrison ? 10 : 9;
    }
    void openSklandGeneral() {
        requestApp(loginForHome && homePage() == Garrison ? appSklandGarrison : appSklandGeneral);
        loginForHome = false;
    }
    void loop();
    SemaphoreHandle_t _mutex = NULL;
    SemaphoreHandle_t _binary_switchApp = NULL;
};
extern AppManagerLite appManagerLite;
