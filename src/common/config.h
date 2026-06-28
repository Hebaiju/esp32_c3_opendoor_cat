/*
 * 文件名称: config.h
 * 功能说明: 全局配置参数
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 从 esp32-opendoor 迁移，保留串口与任务参数配置
 *           当前仅存在 task_uart；新增任务时在此追加对应宏，
 *           并在 srv_sys_info.cpp 的 s_task_stack_table 中注册栈监控
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <Arduino.h>

/* ===== 串口配置 ===== */
#define SERIAL_BAUDRATE           115200

#define UART1_TX_PIN              21
#define UART1_RX_PIN              20
#define UART1_BAUDRATE            115200
#define UART1_BUF_SIZE            256
#define CMD_BUF_SIZE              128

/* ===== 串口任务配置 ===== */
#define TASK_UART_STACK_SIZE      (configMINIMAL_STACK_SIZE * 4)
#define TASK_UART_PRIORITY        2
#define TASK_UART_NAME            "task_uart"
#define TASK_UART_PERIOD_MS       10

/* ===== WS2812 LED 配置 ===== */
#define WS2812_PIN                10
#define WS2812_NUM_LEDS           1
#define WS2812_BRIGHTNESS         100

/* ===== LED 任务配置 ===== */
#define TASK_LED_STACK_SIZE       (configMINIMAL_STACK_SIZE * 3)
#define TASK_LED_PRIORITY         1
#define TASK_LED_NAME             "task_led"
#define TASK_LED_PERIOD_MS        20

/* ===== WiFi 配置 ===== */
#define WIFI_SSID                 "xxxx"
#define WIFI_PASSWORD             "31415926"
#define WIFI_CONNECT_TIMEOUT_MS   15000
#define WIFI_RETRY_BASE_DELAY_MS  2000
#define WIFI_RETRY_MAX_DELAY_MS   30000

/* ===== WiFi 任务配置 ===== */
#define TASK_WIFI_STACK_SIZE      (configMINIMAL_STACK_SIZE * 5)
#define TASK_WIFI_PRIORITY        2
#define TASK_WIFI_NAME            "task_wifi"
#define TASK_WIFI_PERIOD_MS       100

#endif
