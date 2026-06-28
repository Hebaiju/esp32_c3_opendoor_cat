/*
 * 文件名称: drv_ws2812.h
 * 功能说明: WS2812 LED 驱动 + 灯效引擎
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 纯硬件封装 + 灯效算法，不包含业务逻辑
 *           支持常亮、闪烁、呼吸三种模式
 */

#ifndef _DRV_WS2812_H_
#define _DRV_WS2812_H_

#include "common/err_code.h"
#include "common/config.h"
#include <FastLED.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_SOLID,
    LED_MODE_BLINK,
    LED_MODE_BREATHE,
} drv_ws2812_mode_t;

app_err_t drv_ws2812_init(void);

void drv_ws2812_set_mode(drv_ws2812_mode_t mode,
                         CRGB color,
                         uint16_t period_ms,
                         uint8_t  duty_cycle);

void drv_ws2812_set_brightness(uint8_t brightness);

void drv_ws2812_refresh(void);

#ifdef __cplusplus
}
#endif

#endif
