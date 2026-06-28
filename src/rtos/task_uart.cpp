/*
 * 文件名称: task_uart.cpp
 * 功能说明: 串口命令行任务 - 统一管理USB串口与UART1的命令行交互，
 *           负责输入拼包、回显、命令解析与分发，输出同时发往两个串口
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v2.0
 * 说    明: 启动后自动进入数字菜单模式，菜单为默认交互界面
 *           所有服务通过 service_init_all() 统一初始化，无需在此硬编码
 */

#include "task_def.h"
#include "common/config.h"
#include "common/log.h"
#include "common/service_reg.h"
#include "bsp/bsp_debug_usb.h"
#include "bsp/bsp_uart.h"
#include "service/srv_cli.h"
#include <Menu.h>
#include <string.h>

typedef struct {
    char rx_buf[CMD_BUF_SIZE];
    int rx_len;
} cli_port_t;

static cli_port_t s_port_usb;
static cli_port_t s_port_uart1;

static void _process_line(cli_port_t *port, uint8_t port_id)
{
    (void)port_id;

    if (port->rx_len == 0) {
        /* 空回车：菜单模式下刷新，命令模式下显示提示符 */
        if (menu_is_active()) {
            menu_show();
        } else {
            srv_cli_print_str("> ");
        }
        return;
    }

    port->rx_buf[port->rx_len] = '\0';
    srv_cli_exec(port->rx_buf);
    port->rx_len = 0;
}

static void _handle_byte(cli_port_t *port, uint8_t ch, uint8_t port_id)
{
    if (ch == '\r' || ch == '\n') {
        _process_line(port, port_id);
    } else if (ch == '\b' || ch == 0x7F) {
        if (port->rx_len > 0) {
            port->rx_len--;
            if (port_id == 0) {
                bsp_debug_usb_print("\b \b");
            } else {
                bsp_uart1_print("\b \b");
            }
        }
    } else if (ch >= 0x20 && ch < 0x7F) {
        if (port->rx_len < CMD_BUF_SIZE - 1) {
            port->rx_buf[port->rx_len++] = (char)ch;
            if (port_id == 0) {
                bsp_debug_usb_write(&ch, 1);
            } else {
                bsp_uart1_write(&ch, 1);
            }
        }
    }
}

static void task_uart_entry(void *pvParameters)
{
    (void)pvParameters;
    uint8_t ch;
    int ret;

    bsp_debug_usb_init();
    bsp_uart1_init();

    LOG_I("uart task start");

    memset(&s_port_usb, 0, sizeof(s_port_usb));
    memset(&s_port_uart1, 0, sizeof(s_port_uart1));

    /* 统一初始化所有已注册服务 */
    service_init_all();

    /* 自动进入数字菜单模式 */
    menu_start();

    for (;;) {
        while (bsp_debug_usb_available() > 0) {
            ret = bsp_debug_usb_read(&ch, 1);
            if (ret <= 0) break;
            _handle_byte(&s_port_usb, ch, 0);
        }

        while (bsp_uart1_available() > 0) {
            ret = bsp_uart1_read(&ch, 1);
            if (ret <= 0) break;
            _handle_byte(&s_port_uart1, ch, 1);
        }

        vTaskDelay(pdMS_TO_TICKS(TASK_UART_PERIOD_MS));
    }
}

app_err_t task_uart_create(void)
{
    app_err_t ret = app_task_create(
        TASK_UART_NAME,
        task_uart_entry,
        TASK_UART_STACK_SIZE,
        TASK_UART_PRIORITY,
        NULL
    );

    if (ret != APP_OK) {
        return ret;
    }

    LOG_I("uart task created");
    return APP_OK;
}
