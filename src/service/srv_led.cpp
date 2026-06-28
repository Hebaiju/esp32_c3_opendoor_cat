/*
 * 文件名称: srv_led.cpp
 * 功能说明: LED 状态服务实现
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 状态优先级：ERROR > CONNECTING > NORMAL
 *           使用状态堆叠机制，set_state 压入，clear_state 弹出
 *           始终显示当前最高优先级的活跃状态
 */

#include "srv_led.h"
#include "common/config.h"
#include "common/log.h"
#include "common/service_reg.h"
#include "driver/drv_ws2812.h"

typedef struct {
    bool     active;
    uint8_t  priority;
    drv_ws2812_mode_t mode;
    CRGB     color;
    uint16_t period_ms;
    uint8_t  duty;
} led_state_config_t;

static led_state_config_t s_state_table[LED_STATE_MAX];
static bool s_manual_mode = false;

static void _init_state_table(void)
{
    s_state_table[LED_STATE_NORMAL].active = true;
    s_state_table[LED_STATE_NORMAL].priority = 1;
    s_state_table[LED_STATE_NORMAL].mode = LED_MODE_BREATHE;
    s_state_table[LED_STATE_NORMAL].color = CRGB::White;
    s_state_table[LED_STATE_NORMAL].period_ms = 2000;
    s_state_table[LED_STATE_NORMAL].duty = 50;

    s_state_table[LED_STATE_CONNECTING].active = false;
    s_state_table[LED_STATE_CONNECTING].priority = 2;
    s_state_table[LED_STATE_CONNECTING].mode = LED_MODE_BLINK;
    s_state_table[LED_STATE_CONNECTING].color = CRGB::Blue;
    s_state_table[LED_STATE_CONNECTING].period_ms = 1000;
    s_state_table[LED_STATE_CONNECTING].duty = 50;

    s_state_table[LED_STATE_ERROR].active = false;
    s_state_table[LED_STATE_ERROR].priority = 3;
    s_state_table[LED_STATE_ERROR].mode = LED_MODE_BLINK;
    s_state_table[LED_STATE_ERROR].color = CRGB::Red;
    s_state_table[LED_STATE_ERROR].period_ms = 200;
    s_state_table[LED_STATE_ERROR].duty = 50;
}

static void _apply_highest_state(void)
{
    if (s_manual_mode) {
        return;
    }

    int highest = -1;
    uint8_t max_prio = 0;

    for (int i = 0; i < LED_STATE_MAX; i++) {
        if (s_state_table[i].active && s_state_table[i].priority > max_prio) {
            max_prio = s_state_table[i].priority;
            highest = i;
        }
    }

    if (highest < 0) {
        drv_ws2812_set_mode(LED_MODE_OFF, CRGB::Black, 1000, 0);
        return;
    }

    led_state_config_t *cfg = &s_state_table[highest];
    drv_ws2812_set_mode(cfg->mode, cfg->color, cfg->period_ms, cfg->duty);
}

app_err_t srv_led_init(void)
{
    app_err_t ret = drv_ws2812_init();
    if (ret != APP_OK) {
        LOG_E("ws2812 init failed");
        return ret;
    }

    _init_state_table();
    s_manual_mode = false;

    srv_led_set_state(LED_STATE_CONNECTING);

    LOG_I("led service init ok");
    return APP_OK;
}

void srv_led_set_state(led_state_t state)
{
    if (state >= LED_STATE_MAX) {
        return;
    }
    s_state_table[state].active = true;
    s_manual_mode = false;
    _apply_highest_state();
}

void srv_led_clear_state(led_state_t state)
{
    if (state >= LED_STATE_MAX) {
        return;
    }
    s_state_table[state].active = false;
    s_manual_mode = false;
    _apply_highest_state();
}

void srv_led_solid(CRGB color)
{
    s_manual_mode = true;
    drv_ws2812_set_mode(LED_MODE_SOLID, color, 1000, 100);
}

void srv_led_blink(CRGB color, uint16_t freq_hz, uint8_t duty)
{
    s_manual_mode = true;
    uint16_t period_ms = (freq_hz > 0) ? (1000 / freq_hz) : 1000;
    drv_ws2812_set_mode(LED_MODE_BLINK, color, period_ms, duty);
}

void srv_led_breathe(CRGB color, uint16_t period_ms)
{
    s_manual_mode = true;
    drv_ws2812_set_mode(LED_MODE_BREATHE, color, period_ms, 50);
}

void srv_led_off(void)
{
    s_manual_mode = true;
    drv_ws2812_set_mode(LED_MODE_OFF, CRGB::Black, 1000, 0);
}

SERVICE_REGISTER("led", srv_led_init, 10);
