/*
 * 文件名称: Menu.cpp
 * 功能说明: 通用数字菜单库 - 表驱动精简版
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v2.0
 * 说    明: 表驱动菜单定义，递归构建菜单树
 *           动态列表选择用于WiFi扫描等运行时数据
 *           文本/数字/确认三种输入模式
 */

#include "Menu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* ===== 内部类型 ===== */
typedef struct menu_item_s menu_item_t;
typedef struct menu_s menu_t;

typedef enum {
    ITEM_ACTION = 0,
    ITEM_SUBMENU,
} item_type_t;

struct menu_item_s {
    char*            label;
    item_type_t      type;
    union {
        menu_action_cb_t action;
        menu_t*          submenu;
    } data;
};

struct menu_s {
    char*        title;
    menu_item_t  items[MENU_MAX_ITEMS];
    uint8_t      item_count;
};

/* ===== 输入模式 ===== */
typedef enum {
    INPUT_NONE = 0,
    INPUT_TEXT,
    INPUT_NUMBER,
    INPUT_CONFIRM,
    INPUT_LIST,
} input_mode_t;

/* ===== 静态变量 ===== */
static menu_print_cb_t   s_print = NULL;
static menu_t            s_root;
static menu_t*           s_current = NULL;
static menu_t*           s_stack[MENU_MAX_DEPTH];
static uint8_t           s_depth = 0;
static uint8_t           s_active = 0;

static input_mode_t      s_input_mode = INPUT_NONE;
static char              s_prompt[MENU_MAX_PROMPT];
static char              s_default[MENU_MAX_PROMPT];
static int32_t           s_num_min, s_num_max;

static const char*       s_list_items[MENU_MAX_ITEMS];
static uint8_t           s_list_count = 0;
static char              s_list_title[MENU_MAX_PROMPT];

static menu_input_cb_t    s_cb_text;
static menu_number_cb_t   s_cb_number;
static menu_confirm_cb_t  s_cb_confirm;
static menu_select_cb_t   s_cb_select;

/* ===== 工具函数 ===== */

static void _p(const char* fmt, ...)
{
    if (!s_print) return;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    s_print("%s", buf);
    va_end(ap);
}

/* 递归构建菜单树 */
static menu_t* _build_menu(const char* title, const menu_def_t* def)
{
    if (!def) return NULL;

    menu_t* menu = (menu_t*)malloc(sizeof(menu_t));
    if (!menu) return NULL;
    memset(menu, 0, sizeof(menu_t));
    menu->title = (char*)title;

    for (uint8_t i = 0; def[i].label && i < MENU_MAX_ITEMS; i++) {
        menu_item_t* it = &menu->items[i];
        it->label = (char*)def[i].label;

        if (def[i].type == MENU_ACTION) {
            it->type = ITEM_ACTION;
            it->data.action = def[i].data.action;
        }
        else if (def[i].type == MENU_SUBMENU) {
            it->type = ITEM_SUBMENU;
            it->data.submenu = _build_menu(def[i].label, def[i].data.submenu);
        }
        menu->item_count++;
    }
    return menu;
}

/* ===== 核心控制 ===== */

void menu_init(menu_print_cb_t print_cb)
{
    s_print = print_cb;
    s_active = 0;
    s_depth = 0;
    s_current = NULL;
    s_input_mode = INPUT_NONE;
    memset(&s_root, 0, sizeof(s_root));
    s_root.title = (char*)"主菜单";
}

void menu_start(void)
{
    s_current = &s_root;
    s_depth = 0;
    s_active = 1;
    s_input_mode = INPUT_NONE;
    menu_show();
}

void menu_stop(void)
{
    s_active = 0;
    s_input_mode = INPUT_NONE;
    _p("\r\n已退出菜单\r\n\r\n");
}

uint8_t menu_is_active(void) { return s_active; }

/* ===== 注册 ===== */

void menu_register(const char* label, const menu_def_t* items)
{
    if (!label || !items) return;
    if (s_root.item_count >= MENU_MAX_ITEMS) return;

    menu_item_t* it = &s_root.items[s_root.item_count++];
    it->label = (char*)label;
    it->type = ITEM_SUBMENU;
    it->data.submenu = _build_menu(label, items);
}

/* ===== 显示 ===== */

void menu_show(void)
{
    if (!s_current) return;
    _p("\r\n===== %s =====\r\n", s_current->title);
    _p("  ----------------------------------------\r\n");
    for (uint8_t i = 0; i < s_current->item_count; i++) {
        const char* suf = (s_current->items[i].type == ITEM_SUBMENU) ? "..." : "";
        _p("  %u. %s%s\r\n", i + 1, s_current->items[i].label, suf);
    }
    _p("  ----------------------------------------\r\n");
    _p("  0. %s\r\n", s_depth > 0 ? "返回上级" : "刷新菜单");
    _p("\r\n请输入序号: ");
}

/* ===== 动态输入 ===== */

void menu_list(const char* title, const char** items, uint8_t count, menu_select_cb_t cb)
{
    if (!title || !items || count == 0 || !cb) return;
    s_input_mode = INPUT_LIST;
    strncpy(s_list_title, title, sizeof(s_list_title) - 1);
    s_list_count = count > MENU_MAX_ITEMS ? MENU_MAX_ITEMS : count;
    for (uint8_t i = 0; i < s_list_count; i++) {
        s_list_items[i] = items[i];
    }
    s_cb_select = cb;

    _p("\r\n===== %s =====\r\n", s_list_title);
    _p("  ----------------------------------------\r\n");
    for (uint8_t i = 0; i < s_list_count; i++) {
        _p("  %u. %s\r\n", i + 1, s_list_items[i]);
    }
    _p("  ----------------------------------------\r\n");
    _p("  0. 取消\r\n");
    _p("\r\n请选择: ");
}

void menu_prompt_text(const char* prompt, const char* def_val, menu_input_cb_t cb)
{
    if (!prompt || !cb) return;
    s_input_mode = INPUT_TEXT;
    strncpy(s_prompt, prompt, sizeof(s_prompt) - 1);
    strncpy(s_default, def_val ? def_val : "", sizeof(s_default) - 1);
    s_cb_text = cb;
    _p("\r\n%s", s_prompt);
    if (def_val && def_val[0]) _p("[%s]", s_default);
    _p(": ");
}

void menu_prompt_number(const char* prompt, int32_t min, int32_t max, int32_t def, menu_number_cb_t cb)
{
    if (!prompt || !cb) return;
    s_input_mode = INPUT_NUMBER;
    strncpy(s_prompt, prompt, sizeof(s_prompt) - 1);
    s_num_min = min;
    s_num_max = max;
    s_cb_number = cb;
    _p("\r\n%s[%ld~%ld] (默认%ld): ", s_prompt, (long)min, (long)max, (long)def);
}

void menu_prompt_confirm(const char* msg, menu_confirm_cb_t cb)
{
    if (!msg || !cb) return;
    s_input_mode = INPUT_CONFIRM;
    strncpy(s_prompt, msg, sizeof(s_prompt) - 1);
    s_cb_confirm = cb;
    _p("\r\n%s (y/n): ", s_prompt);
}

/* ===== 输入处理 ===== */

void menu_input(const char* line)
{
    if (!s_active || !line) return;

    /* 输入模式 */
    if (s_input_mode != INPUT_NONE) {
        input_mode_t mode = s_input_mode;
        s_input_mode = INPUT_NONE;

        if (mode == INPUT_TEXT) {
            const char* v = (line[0] == '\0') ? s_default : line;
            if (s_cb_text) s_cb_text(v);
        }
        else if (mode == INPUT_NUMBER) {
            if (line[0] == '\0') _p("\r\n已取消\r\n");
            else {
                int32_t v = atol(line);
                if (v < s_num_min || v > s_num_max)
                    _p("\r\n超出范围 (%ld~%ld)\r\n", (long)s_num_min, (long)s_num_max);
                else if (s_cb_number) s_cb_number(v);
            }
        }
        else if (mode == INPUT_CONFIRM) {
            if (s_cb_confirm) s_cb_confirm(line[0] == 'y' || line[0] == 'Y');
        }
        else if (mode == INPUT_LIST) {
            if (line[0] == '0' || line[0] == '\0') {
                _p("\r\n已取消\r\n");
                if (s_cb_select) s_cb_select(0xFF);
            } else {
                uint8_t idx = (uint8_t)atoi(line) - 1;
                if (idx >= s_list_count)
                    _p("\r\n无效选项\r\n");
                else if (s_cb_select) s_cb_select(idx);
            }
        }

        if (s_active && s_input_mode == INPUT_NONE)
            _p("\r\n按回车刷新菜单，按0返回...");
        return;
    }

    /* 空行重绘 */
    if (line[0] == '\0') { menu_show(); return; }

    char c = line[0];

    /* 0 = 返回上级 / 刷新菜单（根菜单不退出） */
    if (c == '0') {
        if (s_depth > 0) {
            s_depth--;
            s_current = s_stack[s_depth];
            _p("\r\n返回上级\r\n");
            menu_show();
        } else {
            _p("\r\n已在主菜单\r\n");
            menu_show();
        }
        return;
    }

    /* 1-9 选择 */
    if (c < '1' || c > '9') {
        _p("\r\n无效输入\r\n");
        menu_show();
        return;
    }

    uint8_t idx = (uint8_t)(c - '1');
    if (idx >= s_current->item_count) {
        _p("\r\n无效选项\r\n");
        menu_show();
        return;
    }

    _p("\r\n");
    menu_item_t* it = &s_current->items[idx];

    if (it->type == ITEM_SUBMENU) {
        if (s_depth >= MENU_MAX_DEPTH) { _p("深度超限\r\n"); menu_show(); return; }
        s_stack[s_depth++] = s_current;
        s_current = it->data.submenu;
        menu_show();
    }
    else if (it->type == ITEM_ACTION) {
        if (it->data.action) it->data.action();
        if (s_input_mode == INPUT_NONE)
            _p("\r\n按回车刷新菜单，按0返回...");
    }
}
