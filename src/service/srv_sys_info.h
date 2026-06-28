/*
 * 文件名称: srv_sys_info.h
 * 功能说明: 系统信息服务接口
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v2.0
 * 说    明: 提供系统资源查看接口，支持任务自注册与系统任务自动注册，
 *           任务列表分用户/系统显示。
 *           信息页支持动态注册，新增信息页无需修改本文件。
 */

#ifndef _SRV_SYS_INFO_H_
#define _SRV_SYS_INFO_H_

#include "common/err_code.h"
#include <Menu.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

app_err_t srv_sys_info_init(void);

/* 用户任务自注册：任务创建后调用，供系统信息统一监控 */
app_err_t srv_sys_info_register_task(const char *name, TaskHandle_t handle, uint32_t stack_size_words);

/* 系统任务注册：内部自动调用，也可手动注册 */
app_err_t srv_sys_info_register_system_task(const char *name, TaskHandle_t handle, uint32_t stack_size_words);

/* 注册一个系统信息页，name 为菜单显示名称，handler 为显示函数 */
app_err_t srv_sys_info_register_page(const char *name, menu_action_cb_t handler);

/* 显示函数，可直接在命令回调中使用 */
void srv_sys_info_show_summary(const char *args);
void srv_sys_info_show_tasks(const char *args);
void srv_sys_info_show_heap(const char *args);
void srv_sys_info_show_stack(const char *args);
void srv_sys_info_show_chip(const char *args);

#ifdef __cplusplus
}
#endif

#endif
