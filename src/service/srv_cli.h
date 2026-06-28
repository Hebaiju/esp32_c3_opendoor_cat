/*
 * 文件名称: srv_cli.h
 * 功能说明: 命令行服务接口 - 菜单为默认交互方式
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v2.0
 * 说    明: 菜单为系统默认界面，启动后直接进入数字菜单模式
 *           支持动态命令注册，各服务可调用 srv_cli_register_cmd() 添加命令
 */

#ifndef _SRV_CLI_H_
#define _SRV_CLI_H_

#include "common/err_code.h"
#include "common/cmd_def.h"

#ifdef __cplusplus
extern "C" {
#endif

app_err_t srv_cli_init(void);

/* 注册一个命令，注册后可在命令行模式下使用 */
app_err_t srv_cli_register_cmd(const char *cmd, const char *desc, cmd_handler_t handler);

void      srv_cli_exec(const char *cmd_line);
void      srv_cli_print(const char *fmt, ...);
void      srv_cli_print_str(const char *str);

#ifdef __cplusplus
}
#endif

#endif
