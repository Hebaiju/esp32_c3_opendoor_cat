/*
 * 文件名称: srv_led.h
 * 功能说明: LED 状态服务接口
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 状态管理 + 优先级仲裁 + 预设模式
 *           支持堆叠式状态 set/clear，高优先级覆盖低优先级
 *           同时提供自定义灯效接口
 */

#ifndef _SRV_LED_H_
#define _SRV_LED_H_

#include "common/err_code.h"
#include <FastLED.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_STATE_NORMAL = 0,
    LED_STATE_CONNECTING,
    LED_STATE_ERROR,
    LED_STATE_MAX,
} led_state_t;

app_err_t srv_led_init(void);

void srv_led_set_state(led_state_t state);
void srv_led_clear_state(led_state_t state);

void srv_led_solid(CRGB color);
void srv_led_blink(CRGB color, uint16_t freq_hz, uint8_t duty);
void srv_led_breathe(CRGB color, uint16_t period_ms);
void srv_led_off(void);

#ifdef __cplusplus
}
#endif

#endif
