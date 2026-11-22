#include "A_Config.h"
int32_t anim_path_ease_out(const lv_anim_t * a)
{
    /*Calculate the current step*/
    uint32_t t = lv_map(a->act_time, 0, a->time, 0, LV_BEZIER_VAL_MAX);
    int32_t step = lv_bezier3(t, 0, LV_BEZIER_VAL_MAX, LV_BEZIER_VAL_MAX, LV_BEZIER_VAL_MAX);

    int32_t new_value;
    new_value = step * (a->end_value - a->start_value);
    new_value = new_value >> LV_BEZIER_VAL_SHIFT;
    new_value += a->start_value;

    return new_value;
}
void lv_obj_pop_up(lv_obj_t *obj, uint16_t distance,
                   uint16_t time, uint16_t waitBeforeStart)
{
    lv_anim_t a;
    uint16_t p;
    lv_obj_set_style_opa(obj, 0, 0);
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    p = lv_obj_get_style_y(obj, 0);
    lv_anim_set_values(&a, p + distance, p);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_path_cb(&a, anim_path_ease_out);
    lv_anim_set_time(&a, time);
    lv_anim_set_delay(&a, waitBeforeStart);
    lv_anim_start(&a);
    lv_obj_fade_in(obj, time, waitBeforeStart);
}
