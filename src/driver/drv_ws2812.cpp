/*
 * 文件名称: drv_ws2812.cpp
 * 功能说明: WS2812 LED 驱动 + 灯效引擎实现
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 基于 FastLED 库，支持常亮、闪烁、呼吸三种灯效
 *           纯算法实现，不包含业务状态
 */

#include "drv_ws2812.h"
#include "common/log.h"

static CRGB s_leds[WS2812_NUM_LEDS];

static drv_ws2812_mode_t s_mode = LED_MODE_OFF;
static CRGB      s_color = CRGB::Black;
static uint16_t  s_period_ms = 1000;
static uint8_t   s_duty = 50;
static uint32_t  s_tick_ms = 0;
static bool      s_initialized = false;

app_err_t drv_ws2812_init(void)
{
    if (s_initialized) {
        return APP_OK;
    }

    FastLED.addLeds<WS2812, WS2812_PIN, GRB>(s_leds, WS2812_NUM_LEDS);
    FastLED.setBrightness(WS2812_BRIGHTNESS);

    s_leds[0] = CRGB::Black;
    FastLED.show();

    s_initialized = true;
    s_mode = LED_MODE_OFF;
    s_tick_ms = 0;

    LOG_I("ws2812 init ok, pin=%d, num=%d", WS2812_PIN, WS2812_NUM_LEDS);
    return APP_OK;
}

void drv_ws2812_set_mode(drv_ws2812_mode_t mode,
                         CRGB color,
                         uint16_t period_ms,
                         uint8_t  duty_cycle)
{
    s_mode = mode;
    s_color = color;
    s_period_ms = (period_ms > 0) ? period_ms : 1;
    s_duty = (duty_cycle <= 100) ? duty_cycle : 50;
    s_tick_ms = 0;
}

void drv_ws2812_set_brightness(uint8_t brightness)
{
    FastLED.setBrightness(brightness);
}

static CRGB _breathe_color(CRGB base, uint8_t phase)
{
    uint8_t scale = phase;
    CRGB result;
    result.r = (uint16_t)base.r * scale / 255;
    result.g = (uint16_t)base.g * scale / 255;
    result.b = (uint16_t)base.b * scale / 255;
    return result;
}

void drv_ws2812_refresh(void)
{
    if (!s_initialized) {
        return;
    }

    s_tick_ms += TASK_LED_PERIOD_MS;
    uint32_t t = s_tick_ms % s_period_ms;

    switch (s_mode) {
    case LED_MODE_OFF:
        s_leds[0] = CRGB::Black;
        break;

    case LED_MODE_SOLID:
        s_leds[0] = s_color;
        break;

    case LED_MODE_BLINK: {
        uint32_t on_time = (uint32_t)s_period_ms * s_duty / 100;
        if (t < on_time) {
            s_leds[0] = s_color;
        } else {
            s_leds[0] = CRGB::Black;
        }
        break;
    }

    case LED_MODE_BREATHE: {
        uint8_t phase;
        uint32_t half = s_period_ms / 2;
        if (t < half) {
            phase = (uint8_t)(t * 255 / half);
        } else {
            phase = (uint8_t)((s_period_ms - t) * 255 / half);
        }
        s_leds[0] = _breathe_color(s_color, phase);
        break;
    }

    default:
        s_leds[0] = CRGB::Black;
        break;
    }

    FastLED.show();
}
