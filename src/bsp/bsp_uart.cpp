/*
 * 文件名称: bsp_uart.cpp
 * 功能说明: UART1 串口 BSP 实现
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 从 esp32-opendoor 迁移，封装 Serial1 (GPIO20/21)
 */

#include "bsp_uart.h"
#include "common/config.h"
#include "common/log.h"

app_err_t bsp_uart1_init(void)
{
    Serial1.begin(UART1_BAUDRATE, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);
    LOG_I("uart1 init: tx=%d, rx=%d, baud=%d", UART1_TX_PIN, UART1_RX_PIN, UART1_BAUDRATE);
    return APP_OK;
}

int bsp_uart1_available(void)
{
    return Serial1.available();
}

int bsp_uart1_read(uint8_t *buf, int len)
{
    int i = 0;
    while (i < len && Serial1.available()) {
        buf[i++] = Serial1.read();
    }
    return i;
}

int bsp_uart1_write(const uint8_t *buf, int len)
{
    return Serial1.write(buf, len);
}

void bsp_uart1_print(const char *str)
{
    Serial1.print(str);
}

HardwareSerial *bsp_uart1_get_serial(void)
{
    return &Serial1;
}
