/*
 * 文件名称: Menu.h
 * 功能说明: 通用数字菜单库 - 表驱动精简版
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: 用结构体数组定义菜单，一行一项，简洁直观
 *           支持多级子菜单嵌套，深度可配置
 *           支持动态列表选择（WiFi扫描等运行时数据）
 *           支持文本/数字/确认三种输入模式
 */

#ifndef MENU_H
#define MENU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MENU_MAX_DEPTH      8
#define MENU_MAX_ITEMS      15
#define MENU_MAX_PROMPT     64

typedef struct menu_def_s menu_def_t;
typedef void (*menu_action_cb_t)(void);
typedef void (*menu_input_cb_t)(const char* input);
typedef void (*menu_number_cb_t)(int32_t value);
typedef void (*menu_confirm_cb_t)(uint8_t confirmed);
typedef void (*menu_select_cb_t)(uint8_t index);
typedef void (*menu_print_cb_t)(const char* fmt, ...);

/* 菜单项类型 */
typedef enum {
    MENU_ACTION = 0,   /* 动作，执行回调函数 */
    MENU_SUBMENU,      /* 子菜单，指向另一张 menu_def_t 数组 */
} menu_item_type_t;

/* 菜单定义结构 —— 用结构体数组定义菜单，最后一项 label 为 NULL 表示结束 */
struct menu_def_s {
    const char*       label;
    menu_item_type_t  type;
    union {
        menu_action_cb_t  action;
        const menu_def_t* submenu;
    } data;
};

/* ===== 核心控制 ===== */
void    menu_init(menu_print_cb_t print_cb);
void    menu_start(void);
void    menu_stop(void);
uint8_t menu_is_active(void);
void    menu_input(const char* line);
void    menu_show(void);

/* ===== 注册 ===== */
void    menu_register(const char* label, const menu_def_t* items);

/* ===== 动态输入提示（动作函数内调用） ===== */
void menu_list(const char* title, const char** items, uint8_t count, menu_select_cb_t cb);
void menu_prompt_text(const char* prompt, const char* def_val, menu_input_cb_t cb);
void menu_prompt_number(const char* prompt, int32_t min, int32_t max, int32_t def, menu_number_cb_t cb);
void menu_prompt_confirm(const char* msg, menu_confirm_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif
