/*
 * 文件名称: bsp_uart.h
 * 功能说明: UART1 串口 BSP 封装
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 从 esp32-opendoor 迁移，封装 Serial1 (GPIO20/21)
 */

#ifndef _BSP_UART_H_
#define _BSP_UART_H_

#include "common/err_code.h"
#include <Arduino.h>
#include <HardwareSerial.h>

#ifdef __cplusplus
extern "C" {
#endif

app_err_t bsp_uart1_init(void);
int bsp_uart1_available(void);
int bsp_uart1_read(uint8_t *buf, int len);
int bsp_uart1_write(const uint8_t *buf, int len);
void bsp_uart1_print(const char *str);
HardwareSerial *bsp_uart1_get_serial(void);

#ifdef __cplusplus
}
#endif

#endif
