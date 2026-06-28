/*
 * 文件名称: main.cpp
 * 功能说明: 程序入口
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 仅初始化并创建 RTOS 任务，不包含业务逻辑
 *           当前只创建串口任务（USB 与 UART1 共享命令行）
 */

#include <Arduino.h>
#include "common/config.h"
#include "common/log.h"
#include "rtos/task_def.h"

void setup()
{
    Serial.begin(SERIAL_BAUDRATE);
    delay(100);

    LOG_I("system init start");

    task_led_create();
    task_wifi_create();
    task_uart_create();

    LOG_I("system init done");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
