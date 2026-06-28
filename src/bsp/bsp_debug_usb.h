/*
 * 文件名称: bsp_debug_usb.h
 * 功能说明: USB-CDC 调试串口 BSP 封装
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 从 esp32-opendoor 迁移，封装 Serial (USB-CDC)
 */

#ifndef _BSP_DEBUG_USB_H_
#define _BSP_DEBUG_USB_H_

#include "common/err_code.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

app_err_t bsp_debug_usb_init(void);
int bsp_debug_usb_available(void);
int bsp_debug_usb_read(uint8_t *buf, int len);
int bsp_debug_usb_write(const uint8_t *buf, int len);
void bsp_debug_usb_print(const char *str);
void bsp_debug_usb_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
