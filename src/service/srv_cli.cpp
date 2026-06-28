/*
 * 文件名称: srv_cli.cpp
 * 功能说明: 命令行服务（精简版）
 * 说    明: 菜单为默认交互，命令表支持动态注册。
 */

#include "srv_cli.h"
#include "common/log.h"
#include "common/config.h"
#include "common/service_reg.h"
#include "bsp/bsp_debug_usb.h"
#include "bsp/bsp_uart.h"
#include <Menu.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <Arduino.h>

#define CLI_MAX_CMDS 16

static cmd_entry_t s_cmd_table[CLI_MAX_CMDS];
static int         s_cmd_count = 0;
static uint8_t     s_enabled = 0;

static void cmd_help(const char *args);
static void cmd_menu(const char *args);

static void _menu_reboot(void)
{
    srv_cli_print("系统重启...\r\n");
    delay(1000);
    ESP.restart();
}

static const menu_def_t s_tool_menu[] = {
    { "重启系统", MENU_ACTION, { .action = _menu_reboot } },
    { NULL, MENU_ACTION, {0} }
};

app_err_t srv_cli_register_cmd(const char *cmd, const char *desc, cmd_handler_t handler)
{
    if (!cmd || !handler || s_cmd_count >= CLI_MAX_CMDS) return APP_FAIL;
    s_cmd_table[s_cmd_count].cmd     = cmd;
    s_cmd_table[s_cmd_count].desc    = desc;
    s_cmd_table[s_cmd_count].handler = handler;
    s_cmd_count++;
    return APP_OK;
}

app_err_t srv_cli_init(void)
{
    s_enabled = 1;
    s_cmd_count = 0;
    menu_init(srv_cli_print);
    srv_cli_register_cmd("help", "显示命令列表", cmd_help);
    srv_cli_register_cmd("menu", "进入数字菜单", cmd_menu);
    return APP_OK;
}

static app_err_t srv_cli_tool_init(void)
{
    menu_register("系统工具", s_tool_menu);
    return APP_OK;
}

SERVICE_REGISTER("cli", srv_cli_init, 0);
SERVICE_REGISTER("cli_tools", srv_cli_tool_init, 10);

void srv_cli_print_str(const char *str)
{
    if (!s_enabled || !str) return;
    bsp_debug_usb_print(str);
    bsp_uart1_print(str);
}

void srv_cli_print(const char *fmt, ...)
{
    char buf[256];
    va_list args;

    if (!s_enabled) return;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    buf[sizeof(buf) - 1] = '\0';

    bsp_debug_usb_print(buf);
    bsp_uart1_print(buf);
}

static void cmd_help(const char *args)
{
    (void)args;
    srv_cli_print("\r\n==== 命令列表 ====\r\n");
    for (int i = 0; i < s_cmd_count; i++) {
        srv_cli_print("  %-10s - %s\r\n", s_cmd_table[i].cmd, s_cmd_table[i].desc);
    }
    srv_cli_print("==================\r\n\r\n");
}

static void cmd_menu(const char *args)
{
    (void)args;
    menu_start();
}

void srv_cli_exec(const char *cmd_line)
{
    char buf[CMD_BUF_SIZE];
    char *cmd, *args;

    if (!s_enabled) return;
    if (menu_is_active()) { menu_input(cmd_line); return; }

    strncpy(buf, cmd_line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    cmd = buf;
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    if (*cmd == '\0' || *cmd == '\r' || *cmd == '\n') return;

    args = strchr(cmd, ' ');
    if (args) {
        *args++ = '\0';
        while (*args == ' ' || *args == '\t') args++;
    } else {
        args = (char *)"";
    }

    for (int i = 0; i < s_cmd_count; i++) {
        if (strcmp(cmd, s_cmd_table[i].cmd) == 0) {
            s_cmd_table[i].handler(args);
            return;
        }
    }

    srv_cli_print("未知命令: %s\r\n", cmd);
}
