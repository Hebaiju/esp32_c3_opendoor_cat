/*
 * 文件名称: err_code.h
 * 功能说明: 统一错误码定义
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 从 esp32-opendoor 迁移
 */

#ifndef _ERR_CODE_H_
#define _ERR_CODE_H_

typedef int app_err_t;

#define APP_OK                    0
#define APP_FAIL                 -1
#define APP_INVALID_PARAM        -2
#define APP_NO_MEMORY            -3
#define APP_TIMEOUT              -4
#define APP_NOT_INIT             -5

#endif
