/*
 * 文件名称: srv_wifi.cpp
 * 功能说明: WiFi 服务实现
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v2.0
 * 说    明: AP+STA双模式 + NVS存储3条配置 + 扫描 + 指数退避重连
 *           与 LED 状态灯联动
 *           注册命令行和菜单项
 */

#include "srv_wifi.h"
#include "common/config.h"
#include "common/log.h"
#include "common/service_reg.h"
#include "service/srv_led.h"
#include "service/srv_cli.h"
#include "service/srv_sys_info.h"
#include <WiFi.h>
#include <Preferences.h>
#include <Menu.h>
#include <string.h>

static Preferences s_prefs;
static wifi_state_t s_state = WIFI_STATE_IDLE;
static char s_cur_ssid[WIFI_MAX_SSID_LEN + 1] = {0};
static char s_cur_password[WIFI_MAX_PASS_LEN + 1] = {0};
static char s_ip_str[16] = {0};
static char s_ap_ssid[32] = {0};
static char s_ap_ip_str[16] = {0};
static uint32_t s_retry_count = 0;
static uint32_t s_retry_delay_ms = WIFI_RETRY_BASE_DELAY_MS;
static uint32_t s_wait_start_ms = 0;
static uint32_t s_connect_start_ms = 0;
static uint8_t  s_saved_count = 0;
static uint8_t  s_scan_count = 0;
static bool     s_scan_done = false;
static uint32_t s_scan_start_ms = 0;
static bool     s_initialized = false;

/* ===== NVS 存储 ===== */

static void _saved_load(void)
{
    s_prefs.begin(WIFI_NVS_NAMESPACE, true);
    s_saved_count = s_prefs.getUChar(WIFI_NVS_COUNT_KEY, 0);
    if (s_saved_count > WIFI_MAX_SAVED_NETS) {
        s_saved_count = WIFI_MAX_SAVED_NETS;
    }
    s_prefs.end();
}

static void _saved_store(void)
{
    s_prefs.begin(WIFI_NVS_NAMESPACE, false);
    s_prefs.putUChar(WIFI_NVS_COUNT_KEY, s_saved_count);
    s_prefs.end();
}

uint8_t srv_wifi_saved_count(void)
{
    return s_saved_count;
}

bool srv_wifi_saved_get(uint8_t idx, char *ssid, char *password)
{
    if (idx >= s_saved_count || !ssid || !password) {
        return false;
    }
    char key_s[8], key_p[8];
    snprintf(key_s, sizeof(key_s), "s%d", idx);
    snprintf(key_p, sizeof(key_p), "p%d", idx);
    s_prefs.begin(WIFI_NVS_NAMESPACE, true);
    s_prefs.getString(key_s, ssid, WIFI_MAX_SSID_LEN + 1);
    s_prefs.getString(key_p, password, WIFI_MAX_PASS_LEN + 1);
    s_prefs.end();
    return true;
}

app_err_t srv_wifi_saved_add(const char *ssid, const char *password)
{
    if (!ssid || !password || strlen(ssid) == 0) {
        return APP_INVALID_PARAM;
    }

    for (uint8_t i = 0; i < s_saved_count; i++) {
        char old_ssid[WIFI_MAX_SSID_LEN + 1];
        char old_pass[WIFI_MAX_PASS_LEN + 1];
        srv_wifi_saved_get(i, old_ssid, old_pass);
        if (strcmp(old_ssid, ssid) == 0) {
            char key_p[8];
            snprintf(key_p, sizeof(key_p), "p%d", i);
            s_prefs.begin(WIFI_NVS_NAMESPACE, false);
            s_prefs.putString(key_p, password);
            s_prefs.end();
            return APP_OK;
        }
    }

    if (s_saved_count >= WIFI_MAX_SAVED_NETS) {
        for (uint8_t i = 0; i < WIFI_MAX_SAVED_NETS - 1; i++) {
            char ssid_i[WIFI_MAX_SSID_LEN + 1];
            char pass_i[WIFI_MAX_PASS_LEN + 1];
            srv_wifi_saved_get(i + 1, ssid_i, pass_i);
            char key_s[8], key_p[8];
            snprintf(key_s, sizeof(key_s), "s%d", i);
            snprintf(key_p, sizeof(key_p), "p%d", i);
            s_prefs.begin(WIFI_NVS_NAMESPACE, false);
            s_prefs.putString(key_s, ssid_i);
            s_prefs.putString(key_p, pass_i);
            s_prefs.end();
        }
        s_saved_count = WIFI_MAX_SAVED_NETS - 1;
    }

    char key_s[8], key_p[8];
    snprintf(key_s, sizeof(key_s), "s%d", s_saved_count);
    snprintf(key_p, sizeof(key_p), "p%d", s_saved_count);
    s_prefs.begin(WIFI_NVS_NAMESPACE, false);
    s_prefs.putString(key_s, ssid);
    s_prefs.putString(key_p, password);
    s_prefs.end();

    s_saved_count++;
    _saved_store();

    LOG_I("wifi saved: ssid=%s, total=%u", ssid, s_saved_count);
    return APP_OK;
}

void srv_wifi_saved_remove(uint8_t idx)
{
    if (idx >= s_saved_count) return;

    for (uint8_t i = idx; i < s_saved_count - 1; i++) {
        char ssid_i[WIFI_MAX_SSID_LEN + 1];
        char pass_i[WIFI_MAX_PASS_LEN + 1];
        srv_wifi_saved_get(i + 1, ssid_i, pass_i);
        char key_s[8], key_p[8];
        snprintf(key_s, sizeof(key_s), "s%d", i);
        snprintf(key_p, sizeof(key_p), "p%d", i);
        s_prefs.begin(WIFI_NVS_NAMESPACE, false);
        s_prefs.putString(key_s, ssid_i);
        s_prefs.putString(key_p, pass_i);
        s_prefs.end();
    }
    s_saved_count--;
    _saved_store();
}

void srv_wifi_saved_clear(void)
{
    s_prefs.begin(WIFI_NVS_NAMESPACE, false);
    s_prefs.clear();
    s_prefs.end();
    s_saved_count = 0;
    LOG_I("wifi saved cleared");
}

/* ===== AP 模式 ===== */

static void _ap_init(void)
{
    uint64_t chipid = ESP.getEfuseMac();
    snprintf(s_ap_ssid, sizeof(s_ap_ssid), "%s%04X",
             WIFI_AP_SSID_PREFIX, (uint16_t)(chipid >> 32));

    WiFi.softAP(s_ap_ssid, NULL, WIFI_AP_CHANNEL, 0, WIFI_AP_MAX_CONN);

    IPAddress ip = WiFi.softAPIP();
    snprintf(s_ap_ip_str, sizeof(s_ap_ip_str), "%d.%d.%d.%d",
             ip[0], ip[1], ip[2], ip[3]);

    LOG_I("wifi AP: ssid=%s, ip=%s", s_ap_ssid, s_ap_ip_str);
}

const char* srv_wifi_get_ap_ssid(void)
{
    return s_ap_ssid;
}

const char* srv_wifi_get_ap_ip(void)
{
    return s_ap_ip_str;
}

/* ===== WiFi 事件回调 ===== */

static void _on_wifi_event(WiFiEvent_t event)
{
    switch (event) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        LOG_I("wifi connected: %s", s_cur_ssid);
        break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP: {
        IPAddress ip = WiFi.localIP();
        snprintf(s_ip_str, sizeof(s_ip_str), "%d.%d.%d.%d",
                 ip[0], ip[1], ip[2], ip[3]);
        LOG_I("wifi got ip: %s", s_ip_str);
        s_state = WIFI_STATE_CONNECTED;
        s_retry_count = 0;
        s_retry_delay_ms = WIFI_RETRY_BASE_DELAY_MS;
        srv_led_clear_state(LED_STATE_CONNECTING);
        srv_wifi_saved_add(s_cur_ssid, s_cur_password);
        break;
    }

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        LOG_I("wifi disconnected");
        if (s_state == WIFI_STATE_CONNECTED || s_state == WIFI_STATE_CONNECTING) {
            s_retry_count++;
            s_state = WIFI_STATE_RETRY_WAIT;
            s_wait_start_ms = millis();
            srv_led_set_state(LED_STATE_CONNECTING);
            s_ip_str[0] = '\0';
            LOG_I("wifi retry #%lu, delay=%lu ms",
                  (unsigned long)s_retry_count,
                  (unsigned long)s_retry_delay_ms);
        }
        break;

    case ARDUINO_EVENT_WIFI_SCAN_DONE: {
        int16_t n = WiFi.scanComplete();
        if (n < 0) n = 0;
        s_scan_count = (uint8_t)n;
        if (s_scan_count > WIFI_SCAN_MAX_NETS) {
            s_scan_count = WIFI_SCAN_MAX_NETS;
        }
        s_scan_done = true;
        s_state = (WiFi.status() == WL_CONNECTED) ? WIFI_STATE_CONNECTED : WIFI_STATE_IDLE;
        uint32_t cost = s_scan_start_ms ? (millis() - s_scan_start_ms) : 0;
        LOG_I("wifi scan done: %u networks, cost=%ums", s_scan_count, cost);
        break;
    }

    default:
        break;
    }
}

/* ===== 初始化 & 连接 ===== */

app_err_t srv_wifi_init(void)
{
    if (s_initialized) {
        return APP_OK;
    }

    WiFi.mode(WIFI_AP_STA);
    WiFi.onEvent(_on_wifi_event);

    _ap_init();
    _saved_load();

    s_initialized = true;
    s_state = WIFI_STATE_IDLE;

    LOG_I("wifi service init ok, saved=%u", s_saved_count);
    return APP_OK;
}

static bool _try_connect_saved(void)
{
    if (s_saved_count == 0) {
        return false;
    }

    char ssid[WIFI_MAX_SSID_LEN + 1];
    char pass[WIFI_MAX_PASS_LEN + 1];

    for (uint8_t i = 0; i < s_saved_count; i++) {
        srv_wifi_saved_get(i, ssid, pass);
        if (strlen(ssid) > 0) {
            LOG_I("wifi trying saved #%u: %s", i, ssid);
            srv_wifi_connect(ssid, pass);
            return true;
        }
    }
    return false;
}

app_err_t srv_wifi_connect(const char *ssid, const char *password)
{
    if (!ssid || !password || strlen(ssid) == 0) {
        return APP_INVALID_PARAM;
    }

    strncpy(s_cur_ssid, ssid, sizeof(s_cur_ssid) - 1);
    s_cur_ssid[sizeof(s_cur_ssid) - 1] = '\0';
    strncpy(s_cur_password, password, sizeof(s_cur_password) - 1);
    s_cur_password[sizeof(s_cur_password) - 1] = '\0';

    s_retry_count = 0;
    s_retry_delay_ms = WIFI_RETRY_BASE_DELAY_MS;
    s_ip_str[0] = '\0';

    LOG_I("wifi connecting: ssid=%s", s_cur_ssid);
    s_state = WIFI_STATE_CONNECTING;
    s_connect_start_ms = millis();
    srv_led_set_state(LED_STATE_CONNECTING);

    WiFi.begin(s_cur_ssid, s_cur_password);

    return APP_OK;
}

void srv_wifi_disconnect(void)
{
    LOG_I("wifi disconnect");
    WiFi.disconnect(false);
    s_state = WIFI_STATE_IDLE;
    s_retry_count = 0;
    s_retry_delay_ms = WIFI_RETRY_BASE_DELAY_MS;
    s_ip_str[0] = '\0';
    srv_led_clear_state(LED_STATE_CONNECTING);
}

void srv_wifi_reconnect(void)
{
    if (strlen(s_cur_ssid) == 0) {
        if (!_try_connect_saved()) {
            LOG_I("wifi reconnect: no saved networks");
        }
        return;
    }

    srv_wifi_disconnect();
    delay(100);
    srv_wifi_connect(s_cur_ssid, s_cur_password);
}

/* ===== 状态查询 ===== */

wifi_state_t srv_wifi_get_state(void)
{
    return s_state;
}

const char* srv_wifi_get_state_str(void)
{
    switch (s_state) {
    case WIFI_STATE_IDLE:        return "IDLE";
    case WIFI_STATE_CONNECTING:  return "CONNECTING";
    case WIFI_STATE_CONNECTED:   return "CONNECTED";
    case WIFI_STATE_RETRY_WAIT:  return "RETRY_WAIT";
    case WIFI_STATE_SCANNING:    return "SCANNING";
    default:                      return "UNKNOWN";
    }
}

const char* srv_wifi_get_ssid(void)
{
    return s_cur_ssid;
}

const char* srv_wifi_get_ip(void)
{
    return s_ip_str;
}

int8_t srv_wifi_get_rssi(void)
{
    if (s_state != WIFI_STATE_CONNECTED) {
        return 0;
    }
    return WiFi.RSSI();
}

uint32_t srv_wifi_get_retry_count(void)
{
    return s_retry_count;
}

/* ===== WiFi 扫描 ===== */

app_err_t srv_wifi_scan_start(void)
{
    if (s_state == WIFI_STATE_SCANNING) {
        return APP_OK;
    }

    int16_t prev = WiFi.scanComplete();
    if (prev >= 0 || prev == WIFI_SCAN_RUNNING) {
        WiFi.scanDelete();
    }

    s_scan_done = false;
    s_scan_count = 0;
    s_state = WIFI_STATE_SCANNING;
    s_scan_start_ms = millis();

    int ret = WiFi.scanNetworks(true, false, true, 200);
    if (ret == WIFI_SCAN_RUNNING) {
        LOG_I("wifi scan started");
        return APP_OK;
    }

    s_state = (WiFi.status() == WL_CONNECTED) ? WIFI_STATE_CONNECTED : WIFI_STATE_IDLE;
    s_scan_start_ms = 0;
    return APP_FAIL;
}

bool srv_wifi_scan_done(void)
{
    if (s_scan_done) {
        return true;
    }
    int16_t n = WiFi.scanComplete();
    if (n >= 0) {
        s_scan_count = (uint8_t)n;
        if (s_scan_count > WIFI_SCAN_MAX_NETS) {
            s_scan_count = WIFI_SCAN_MAX_NETS;
        }
        s_scan_done = true;
        s_state = (WiFi.status() == WL_CONNECTED) ? WIFI_STATE_CONNECTED : WIFI_STATE_IDLE;
        uint32_t cost = s_scan_start_ms ? (millis() - s_scan_start_ms) : 0;
        LOG_I("wifi scan done(poll): %u networks, cost=%ums", s_scan_count, cost);
        return true;
    }
    return false;
}

uint8_t srv_wifi_scan_count(void)
{
    return s_scan_count;
}

bool srv_wifi_scan_get_item(uint8_t idx, wifi_scan_item_t *item)
{
    if (!item || idx >= s_scan_count) {
        return false;
    }

    String ssid = WiFi.SSID(idx);
    strncpy(item->ssid, ssid.c_str(), sizeof(item->ssid) - 1);
    item->ssid[sizeof(item->ssid) - 1] = '\0';
    item->rssi = WiFi.RSSI(idx);
    item->enc_type = WiFi.encryptionType(idx);
    item->channel = WiFi.channel(idx);

    return true;
}

void srv_wifi_scan_clear(void)
{
    WiFi.scanDelete();
    s_scan_done = false;
    s_scan_count = 0;
    s_scan_start_ms = 0;
}

/* ===== 任务处理 ===== */

void srv_wifi_task_process(void)
{
    if (!s_initialized) {
        return;
    }

    if (s_state == WIFI_STATE_CONNECTING) {
        uint32_t elapsed = millis() - s_connect_start_ms;
        if (elapsed > WIFI_CONNECT_TIMEOUT_MS) {
            LOG_I("wifi connect timeout");
            WiFi.disconnect(false);
            s_retry_count++;
            s_state = WIFI_STATE_RETRY_WAIT;
            s_wait_start_ms = millis();
        }
        return;
    }

    if (s_state == WIFI_STATE_RETRY_WAIT) {
        uint32_t elapsed = millis() - s_wait_start_ms;
        if (elapsed >= s_retry_delay_ms) {
            s_retry_delay_ms *= 2;
            if (s_retry_delay_ms > WIFI_RETRY_MAX_DELAY_MS) {
                s_retry_delay_ms = WIFI_RETRY_MAX_DELAY_MS;
            }

            LOG_I("wifi retry #%lu, delay=%lu ms",
                  (unsigned long)s_retry_count,
                  (unsigned long)s_retry_delay_ms);

            s_state = WIFI_STATE_CONNECTING;
            s_connect_start_ms = millis();
            WiFi.begin(s_cur_ssid, s_cur_password);
        }
        return;
    }
}

/* ===== 命令行命令 ===== */

static void cmd_wifi_status(const char *args)
{
    (void)args;
    srv_cli_print("--- WiFi Status ---\r\n");
    srv_cli_print("STA State : %s\r\n", srv_wifi_get_state_str());
    srv_cli_print("STA SSID  : %s\r\n", srv_wifi_get_ssid());
    srv_cli_print("STA IP    : %s\r\n", srv_wifi_get_ip());
    if (srv_wifi_get_state() == WIFI_STATE_CONNECTED) {
        srv_cli_print("RSSI      : %d dBm\r\n", srv_wifi_get_rssi());
    }
    srv_cli_print("Retry     : %lu\r\n", (unsigned long)srv_wifi_get_retry_count());
    srv_cli_print("Saved     : %u / %u\r\n", srv_wifi_saved_count(), WIFI_MAX_SAVED_NETS);
    srv_cli_print("AP SSID   : %s\r\n", srv_wifi_get_ap_ssid());
    srv_cli_print("AP IP     : %s\r\n", srv_wifi_get_ap_ip());
}

static void cmd_wifi_connect(const char *args)
{
    char ssid[33], pwd[65];
    int n = sscanf(args, "%32s %64s", ssid, pwd);
    if (n < 2) {
        srv_cli_print("usage: wifi_connect <ssid> <password>\r\n");
        return;
    }
    srv_wifi_connect(ssid, pwd);
    srv_cli_print("connecting to %s...\r\n", ssid);
}

static void cmd_wifi_disconnect(const char *args)
{
    (void)args;
    srv_wifi_disconnect();
    srv_cli_print("wifi disconnected\r\n");
}

static void cmd_wifi_reconnect(const char *args)
{
    (void)args;
    srv_wifi_reconnect();
    srv_cli_print("wifi reconnecting...\r\n");
}

static void cmd_wifi_scan(const char *args)
{
    (void)args;
    srv_wifi_scan_start();
    srv_cli_print("wifi scanning...\r\n");
}

static void cmd_wifi_list(const char *args)
{
    (void)args;
    if (!srv_wifi_scan_done()) {
        srv_cli_print("scan not done, try wifi_scan first\r\n");
        return;
    }
    uint8_t n = srv_wifi_scan_count();
    srv_cli_print("--- WiFi Networks (%u) ---\r\n", n);
    for (uint8_t i = 0; i < n; i++) {
        wifi_scan_item_t item;
        if (srv_wifi_scan_get_item(i, &item)) {
            srv_cli_print("  %2u. %-32s  %4d dBm  ch%u%s\r\n",
                          i + 1, item.ssid, item.rssi, item.channel,
                          item.enc_type == WIFI_AUTH_OPEN ? "  [OPEN]" : "");
        }
    }
}

static void cmd_wifi_saved(const char *args)
{
    (void)args;
    uint8_t n = srv_wifi_saved_count();
    srv_cli_print("--- Saved WiFi (%u/%u) ---\r\n", n, WIFI_MAX_SAVED_NETS);
    for (uint8_t i = 0; i < n; i++) {
        char ssid[WIFI_MAX_SSID_LEN + 1];
        char pass[WIFI_MAX_PASS_LEN + 1];
        srv_wifi_saved_get(i, ssid, pass);
        srv_cli_print("  %u. %s\r\n", i + 1, ssid);
    }
}

/* ===== 菜单 ===== */

static void _menu_wifi_status(void)
{
    cmd_wifi_status(NULL);
}

static void _menu_wifi_reconnect(void)
{
    cmd_wifi_reconnect(NULL);
}

static void _menu_wifi_disconnect(void)
{
    cmd_wifi_disconnect(NULL);
}

static void _menu_wifi_scan(void)
{
    cmd_wifi_scan(NULL);
}

static void _menu_wifi_saved(void)
{
    cmd_wifi_saved(NULL);
}

static const menu_def_t s_wifi_menu[] = {
    { "查看WiFi状态",   MENU_ACTION, { .action = _menu_wifi_status } },
    { "扫描周围WiFi",   MENU_ACTION, { .action = _menu_wifi_scan } },
    { "已保存WiFi",     MENU_ACTION, { .action = _menu_wifi_saved } },
    { "重新连接",       MENU_ACTION, { .action = _menu_wifi_reconnect } },
    { "断开连接",       MENU_ACTION, { .action = _menu_wifi_disconnect } },
    { NULL, MENU_ACTION, {0} }
};

/* ===== 系统信息页 ===== */

static void _show_wifi_info(void)
{
    srv_cli_print("WiFi STA: %s", srv_wifi_get_state_str());
    if (srv_wifi_get_state() == WIFI_STATE_CONNECTED) {
        srv_cli_print(" %s", srv_wifi_get_ip());
    }
    if (srv_wifi_get_retry_count() > 0) {
        srv_cli_print(" (retry:%lu)", (unsigned long)srv_wifi_get_retry_count());
    }
    srv_cli_print("\r\n");
    srv_cli_print("WiFi AP: %s %s\r\n", srv_wifi_get_ap_ssid(), srv_wifi_get_ap_ip());
}

static app_err_t srv_wifi_post_init(void)
{
    srv_cli_register_cmd("wifi",            "查看WiFi状态",          cmd_wifi_status);
    srv_cli_register_cmd("wifi_connect",    "连接WiFi <ssid> <pwd>", cmd_wifi_connect);
    srv_cli_register_cmd("wifi_disconnect", "断开WiFi",             cmd_wifi_disconnect);
    srv_cli_register_cmd("wifi_reconnect",  "重新连接WiFi",         cmd_wifi_reconnect);
    srv_cli_register_cmd("wifi_scan",       "扫描WiFi",             cmd_wifi_scan);
    srv_cli_register_cmd("wifi_list",       "列出扫描结果",         cmd_wifi_list);
    srv_cli_register_cmd("wifi_saved",      "已保存WiFi列表",       cmd_wifi_saved);

    menu_register("WiFi 管理", s_wifi_menu);

    srv_sys_info_register_page("WiFi 信息", _show_wifi_info);

    return APP_OK;
}

static app_err_t srv_wifi_service_init(void)
{
    srv_wifi_init();
    srv_wifi_post_init();

    if (!_try_connect_saved()) {
        LOG_I("wifi: no saved networks, AP mode only");
    }

    return APP_OK;
}

SERVICE_REGISTER("wifi", srv_wifi_service_init, 60);
