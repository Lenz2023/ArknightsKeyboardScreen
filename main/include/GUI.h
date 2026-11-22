#pragma once

#include <stdint.h>
#include "lvgl.h"

void lv_obj_pop_up(lv_obj_t *obj, uint16_t distance = 30,
                   uint16_t time = 300, uint16_t waitBeforeStart = 0);

namespace GUI
{
    void toast(const char *str, bool lock = true, uint32_t delay = 0);
}

