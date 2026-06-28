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

#endif
