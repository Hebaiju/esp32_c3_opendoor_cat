/*
 * 文件名称: bsp_debug_usb.cpp
 * 功能说明: USB-CDC 调试串口 BSP 实现
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 从 esp32-opendoor 迁移，封装 Serial (USB-CDC)
 */

#include "bsp_debug_usb.h"
#include "common/config.h"
#include "common/log.h"
#include <Arduino.h>
#include <stdarg.h>

app_err_t bsp_debug_usb_init(void)
{
    Serial.begin(SERIAL_BAUDRATE);
    LOG_I("debug usb init: baud=%d", SERIAL_BAUDRATE);
    return APP_OK;
}

int bsp_debug_usb_available(void)
{
    return Serial.available();
}

int bsp_debug_usb_read(uint8_t *buf, int len)
{
    int i = 0;
    int ch;
    while (i < len && Serial.available()) {
        ch = Serial.read();
        if (ch < 0) break;
        buf[i++] = (uint8_t)ch;
    }
    return i;
}

int bsp_debug_usb_write(const uint8_t *buf, int len)
{
    return Serial.write(buf, len);
}

void bsp_debug_usb_print(const char *str)
{
    if (str) {
        Serial.print(str);
    }
}

void bsp_debug_usb_printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    buf[sizeof(buf) - 1] = '\0';
    Serial.print(buf);
}
