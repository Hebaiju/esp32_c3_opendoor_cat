/*
 * 文件名称: srv_sys_info.cpp
 * 功能说明: 系统信息服务（精简版）
 * 说    明: 提供系统资源查看菜单，任务自注册，信息页动态注册。
 */

#include "srv_sys_info.h"
#include "common/log.h"
#include "common/config.h"
#include "common/service_reg.h"
#include "service/srv_cli.h"
#include <Menu.h>
#include <Arduino.h>
#include <string.h>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
}

#define MAX_TASKS  16
#define MAX_PAGES  16

typedef struct {
    char        name[configMAX_TASK_NAME_LEN];
    TaskHandle_t handle;
    uint32_t    stack_words;
    uint8_t     system;
    uint8_t     valid;
} task_info_t;

typedef struct {
    const char      *name;
    menu_action_cb_t handler;
} page_entry_t;

static task_info_t s_tasks[MAX_TASKS];
static page_entry_t s_pages[MAX_PAGES];
static int         s_page_count = 0;
static menu_def_t  s_menu[MAX_PAGES + 1];

/* ===== 任务管理 ===== */

static int _task_count(uint8_t system)
{
    int n = 0;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (s_tasks[i].valid && s_tasks[i].system == system) n++;
    }
    return n;
}

static app_err_t _add_task(const char *name, TaskHandle_t handle, uint32_t words, uint8_t system)
{
    if (!name || !handle) return APP_FAIL;

    for (int i = 0; i < MAX_TASKS; i++) {
        if (s_tasks[i].valid && strcmp(s_tasks[i].name, name) == 0) {
            s_tasks[i].handle = handle;
            s_tasks[i].stack_words = words;
            return APP_OK;
        }
    }

    for (int i = 0; i < MAX_TASKS; i++) {
        if (!s_tasks[i].valid) {
            strncpy(s_tasks[i].name, name, sizeof(s_tasks[i].name) - 1);
            s_tasks[i].name[sizeof(s_tasks[i].name) - 1] = '\0';
            s_tasks[i].handle = handle;
            s_tasks[i].stack_words = words;
            s_tasks[i].system = system;
            s_tasks[i].valid = 1;
            return APP_OK;
        }
    }
    return APP_FAIL;
}

static void _reg_sys_tasks(void)
{
    TaskHandle_t h;
    if ((h = xTaskGetIdleTaskHandle()) != NULL) _add_task("idle", h, 0, 1);
    if ((h = xTaskGetHandle("ipc0")) != NULL)  _add_task("ipc0", h, 0, 1);
    if ((h = xTaskGetHandle("Tmr Svc")) != NULL) _add_task("Tmr Svc", h, 0, 1);
}

/* ===== 辅助函数 ===== */

static const char* _reset_reason(void)
{
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:  return "上电复位";
        case ESP_RST_EXT:      return "外部引脚复位";
        case ESP_RST_SW:       return "软件复位";
        case ESP_RST_PANIC:    return "异常复位";
        case ESP_RST_INT_WDT:  return "中断看门狗";
        case ESP_RST_TASK_WDT: return "任务看门狗";
        case ESP_RST_WDT:      return "看门狗";
        case ESP_RST_DEEPSLEEP:return "深度睡眠唤醒";
        case ESP_RST_BROWNOUT: return "欠压复位";
        default:               return "未知";
    }
}

static const char* _task_state(eTaskState s)
{
    switch (s) {
        case eRunning:   return "运行";
        case eReady:     return "就绪";
        case eBlocked:   return "阻塞";
        case eSuspended: return "挂起";
        default:         return "?";
    }
}

/* ===== 显示函数 ===== */

static void _show_summary(void)
{
    uint32_t total = ESP.getHeapSize();
    uint32_t free  = ESP.getFreeHeap();
    uint32_t min   = ESP.getMinFreeHeap();
    uint32_t up    = millis();

    srv_cli_print("\r\n========== 系统综合信息 ==========\r\n");
    srv_cli_print("  型号: %s @ %lu MHz\r\n", ESP.getChipModel(), (unsigned long)ESP.getCpuFreqMHz());
    srv_cli_print("  Flash: %lu KB, SDK: %s\r\n", (unsigned long)(ESP.getFlashChipSize() / 1024), ESP.getSdkVersion());
    srv_cli_print("  复位: %s\r\n", _reset_reason());
    srv_cli_print("  运行: %lu天 %02lu:%02lu:%02lu\r\n",
           up / 86400000, (up % 86400000) / 3600000, (up % 3600000) / 60000, (up % 60000) / 1000);
    srv_cli_print("  任务: 总计%lu 用户%d 系统%d\r\n",
           (unsigned long)uxTaskGetNumberOfTasks(), _task_count(0), _task_count(1));
    srv_cli_print("  堆: 总计%lu 已用%lu 空闲%lu 最小%lu (KB)\r\n",
           total / 1024, (total - free) / 1024, free / 1024, min / 1024);
    srv_cli_print("==================================\r\n\r\n");
}

static void _print_task(task_info_t *t)
{
    eTaskState s = eTaskGetState(t->handle);
    UBaseType_t p = uxTaskPriorityGet(t->handle);
    uint32_t hwm = uxTaskGetStackHighWaterMark(t->handle);
    srv_cli_print("  %-14s %4s %4lu %7lu字\r\n",
           t->name, _task_state(s), (unsigned long)p, (unsigned long)hwm);
}

static void _show_tasks_group(uint8_t system, const char *title)
{
    srv_cli_print("\r\n  [%s]\r\n", title);
    srv_cli_print("  %-14s %4s %4s %7s\r\n", "任务名", "状态", "优先级", "栈水位");
    srv_cli_print("  ------------------------------\r\n");
    for (int i = 0; i < MAX_TASKS; i++) {
        if (s_tasks[i].valid && s_tasks[i].system == system) _print_task(&s_tasks[i]);
    }
}

static void _show_tasks(void)
{
    srv_cli_print("\r\n========== 任务列表 ==========\r\n");
    srv_cli_print("  总计: %lu  用户: %d  系统: %d\r\n",
           (unsigned long)uxTaskGetNumberOfTasks(), _task_count(0), _task_count(1));
    _show_tasks_group(0, "用户任务");
    _show_tasks_group(1, "系统任务");
    srv_cli_print("==============================\r\n\r\n");
}

static void _show_heap(void)
{
    uint32_t total = ESP.getHeapSize();
    uint32_t free  = ESP.getFreeHeap();
    uint32_t min   = ESP.getMinFreeHeap();

    srv_cli_print("\r\n========== 内存信息 ==========\r\n");
    srv_cli_print("  总计: %lu KB\r\n", total / 1024);
    srv_cli_print("  空闲: %lu KB (%.1f%%)\r\n", free / 1024, free * 100.0f / total);
    srv_cli_print("  已用: %lu KB (%.1f%%)\r\n", (total - free) / 1024, (total - free) * 100.0f / total);
    srv_cli_print("  最小: %lu KB (%.1f%%)\r\n", min / 1024, min * 100.0f / total);
    srv_cli_print("==============================\r\n\r\n");
}

static void _show_stack(void)
{
    srv_cli_print("\r\n========== 任务栈 ==========\r\n");
    srv_cli_print("  %-14s %8s %8s %6s\r\n", "任务名", "总栈", "最小剩余", "使用率");
    srv_cli_print("  ------------------------------\r\n");
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!s_tasks[i].valid || s_tasks[i].system) continue;
        uint32_t total = s_tasks[i].stack_words * sizeof(StackType_t);
        uint32_t min   = uxTaskGetStackHighWaterMark(s_tasks[i].handle) * sizeof(StackType_t);
        uint32_t usage = ((total - min) * 100) / total;
        srv_cli_print("  %-14s %7luB %7luB %5lu%%\r\n",
               s_tasks[i].name, (unsigned long)total, (unsigned long)min, (unsigned long)usage);
    }
    srv_cli_print("==============================\r\n\r\n");
}

static void _show_chip(void)
{
    uint64_t mac = ESP.getEfuseMac();
    srv_cli_print("\r\n========== 芯片信息 ==========\r\n");
    srv_cli_print("  型号: %s Rev %d\r\n", ESP.getChipModel(), ESP.getChipRevision());
    srv_cli_print("  CPU: %lu MHz, Flash: %lu KB\r\n",
           (unsigned long)ESP.getCpuFreqMHz(), (unsigned long)(ESP.getFlashChipSize() / 1024));
    srv_cli_print("  SDK: %s\r\n", ESP.getSdkVersion());
    srv_cli_print("  MAC: %08X%08X\r\n", (unsigned int)(mac >> 32), (unsigned int)mac);
    srv_cli_print("  复位: %s\r\n", _reset_reason());
    srv_cli_print("==============================\r\n\r\n");
}

/* ===== 菜单包装 ===== */

static void _menu_summary(void) { _show_summary(); }
static void _menu_tasks(void)   { _show_tasks(); }
static void _menu_heap(void)    { _show_heap(); }
static void _menu_stack(void)   { _show_stack(); }
static void _menu_chip(void)    { _show_chip(); }

/* ===== 注册接口 ===== */

app_err_t srv_sys_info_register_page(const char *name, menu_action_cb_t handler)
{
    if (!name || !handler || s_page_count >= MAX_PAGES) return APP_FAIL;
    s_pages[s_page_count].name    = name;
    s_pages[s_page_count].handler = handler;
    s_page_count++;
    return APP_OK;
}

app_err_t srv_sys_info_init(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    s_page_count = 0;
    _reg_sys_tasks();

    srv_sys_info_register_page("综合信息", _menu_summary);
    srv_sys_info_register_page("任务列表", _menu_tasks);
    srv_sys_info_register_page("内存信息", _menu_heap);
    srv_sys_info_register_page("任务栈",   _menu_stack);
    srv_sys_info_register_page("芯片信息", _menu_chip);

    for (int i = 0; i < s_page_count; i++) {
        s_menu[i] = (menu_def_t){ s_pages[i].name, MENU_ACTION, { .action = s_pages[i].handler } };
    }
    s_menu[s_page_count] = (menu_def_t){ NULL, MENU_ACTION, {0} };
    menu_register("系统信息", s_menu);

    return APP_OK;
}

SERVICE_REGISTER("sys_info", srv_sys_info_init, 5);

app_err_t srv_sys_info_register_task(const char *name, TaskHandle_t handle, uint32_t stack_size_words)
{
    return _add_task(name, handle, stack_size_words, 0);
}

app_err_t srv_sys_info_register_system_task(const char *name, TaskHandle_t handle, uint32_t stack_size_words)
{
    return _add_task(name, handle, stack_size_words, 1);
}

void srv_sys_info_show_summary(const char *args) { (void)args; _show_summary(); }
void srv_sys_info_show_tasks(const char *args)   { (void)args; _show_tasks(); }
void srv_sys_info_show_heap(const char *args)    { (void)args; _show_heap(); }
void srv_sys_info_show_stack(const char *args)   { (void)args; _show_stack(); }
void srv_sys_info_show_chip(const char *args)    { (void)args; _show_chip(); }
