/*
 * 文件名称: log.h
 * 功能说明: 日志打印宏
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 从 esp32-opendoor 迁移，使用 USB 调试串口输出
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <Arduino.h>

#define LOG_TAG "APP"

#define LOG_E(fmt, ...)  Serial.printf("[%s][E] " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_W(fmt, ...)  Serial.printf("[%s][W] " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_I(fmt, ...)  Serial.printf("[%s][I] " fmt "\n", LOG_TAG, ##__VA_ARGS__)
#define LOG_D(fmt, ...)  Serial.printf("[%s][D] " fmt "\n", LOG_TAG, ##__VA_ARGS__)

#endif
