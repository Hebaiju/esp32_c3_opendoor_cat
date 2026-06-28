/*
 * 文件名称: cmd_def.h
 * 功能说明: 命令行命令表定义
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 从 esp32-opendoor 迁移
 */

#ifndef _CMD_DEF_H_
#define _CMD_DEF_H_

#include "common/err_code.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cmd_handler_t)(const char *args);

typedef struct {
    const char *cmd;
    const char *desc;
    cmd_handler_t handler;
} cmd_entry_t;

#ifdef __cplusplus
}
#endif

#endif
