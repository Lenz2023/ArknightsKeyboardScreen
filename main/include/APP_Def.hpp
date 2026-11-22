#pragma once
#include "AppManagerLite.h"
class AppHome : public BaseApp
{
public:
    void init()
    {
        appid = 1;
    }
    void setup();
    void loop();
    void destroy();
};

class AppWeather : public BaseApp
{
public:
    void init()
    {
        appid = 5;
    }
    void setup();
    void loop();
    void destroy();
};

class AppSettings : public BaseApp
{
public:
    void init()
    {
        appid = 50;
    }
    void setup();
    void loop();
    void destroy();
};

class AppQRCode : public BaseApp {
public:
    void init()
    {
        appid = 100;
    }
    void setup();
    void loop();
    void destroy();
};

class AppMusic : public BaseApp {
public:
    void init() { appid = 7; }
    void setup();
    void loop();
    void destroy();
};

class AppSklandLogin : public BaseApp {
public:
    void init() { appid = 8; }
    void setup();
    void loop();
    void destroy();
};

class AppSklandGeneral : public BaseApp {
public:
    void init() { appid = 9; }
    void setup();
    void loop();
    void destroy();
    bool nextPage() override;
};

class AppSklandGarrison : public BaseApp {
public:
    void init() { appid = 10; }
    void setup();
    void loop();
    void destroy();
};
