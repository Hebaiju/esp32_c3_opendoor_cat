/*
 * 文件名称: srv_webconfig.cpp
 * 功能说明: Web 配置服务实现
 * 作    者:
 * 创建日期: 2026-06-28
 * 版    本: v1.0
 * 说    明: HTTP服务器 + DNS劫持 + WiFi配置页
 *           简洁工业风设计，支持WiFi扫描+选择+连接
 */

#include "srv_webconfig.h"
#include "common/config.h"
#include "common/log.h"
#include "common/service_reg.h"
#include "service/srv_wifi.h"
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFi.h>

static WebServer *s_server = NULL;
static DNSServer *s_dns = NULL;
static bool s_initialized = false;

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>WiFi 配置</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,Segoe UI,Roboto,sans-serif;background:#0f1115;color:#e6e6e6;min-height:100vh}
.header{padding:20px;background:#171a21;border-bottom:1px solid #2a2f3a}
.header h1{font-size:18px;font-weight:600;letter-spacing:.5px}
.header p{font-size:12px;color:#8892a0;margin-top:4px}
.container{max-width:480px;margin:0 auto;padding:16px}
.card{background:#171a21;border:1px solid #2a2f3a;border-radius:6px;padding:16px;margin-bottom:12px}
.card h2{font-size:14px;font-weight:600;margin-bottom:12px;display:flex;justify-content:space-between;align-items:center}
.btn{display:inline-block;padding:10px 20px;background:#2563eb;color:#fff;border:none;border-radius:4px;font-size:14px;cursor:pointer;text-align:center;min-width:80px}
.btn:active{transform:scale(.97)}
.btn-ghost{background:transparent;border:1px solid #2a2f3a;color:#8892a0}
.btn-small{padding:6px 12px;font-size:12px;min-width:auto}
.status{font-size:12px;color:#8892a0}
.wifi-list{list-style:none}
.wifi-item{display:flex;justify-content:space-between;align-items:center;padding:12px 8px;margin:0 -8px;border-bottom:1px solid #2a2f3a;cursor:pointer}
.wifi-item:last-child{border-bottom:none}
.wifi-item:hover{background:#1c2029;border-radius:4px}
.wifi-name{flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-size:14px}
.wifi-info{font-size:12px;color:#8892a0;margin-left:10px;display:flex;gap:8px;align-items:center}
.lock{font-size:11px;color:#f59e0b}
.modal{display:none;position:fixed;top:0;left:0;right:0;bottom:0;background:rgba(0,0,0,.6);z-index:100;align-items:center;justify-content:center;padding:16px}
.modal.show{display:flex}
.modal-box{background:#171a21;border:1px solid #2a2f3a;border-radius:6px;padding:20px;width:100%;max-width:360px}
.modal-box h3{font-size:16px;margin-bottom:16px}
.form-group{margin-bottom:14px}
.form-group label{display:block;font-size:12px;color:#8892a0;margin-bottom:6px}
.form-group input{width:100%;padding:10px 12px;background:#0f1115;border:1px solid #2a2f3a;border-radius:4px;color:#e6e6e6;font-size:14px}
.form-actions{display:flex;gap:10px;justify-content:flex-end;margin-top:20px}
.msg{padding:10px 12px;border-radius:4px;font-size:13px;margin-top:12px;display:none}
.msg.show{display:block}
.msg-error{background:#1c1414;border:1px solid #3b1f1f;color:#f87171}
.msg-success{background:#0f1f14;border:1px solid #1f3b28;color:#4ade80}
</style>
</head>
<body>
<div class="header">
<h1>WiFi 配置</h1>
<p>选择 WiFi 网络并输入密码连接</p>
</div>
<div class="container">
<div class="card">
<h2><span>WiFi 列表</span><button class="btn btn-small" onclick="scanWiFi()">刷新</button></h2>
<div class="status" id="scanStatus">正在扫描...</div>
<ul class="wifi-list" id="wifiList"></ul>
</div>
<div class="card">
<h2>当前状态</h2>
<div class="status" id="staStatus">--</div>
</div>
</div>
<div class="modal" id="passModal">
<div class="modal-box">
<h3 id="passTitle">连接 WiFi</h3>
<div class="form-group">
<label>WiFi 密码</label>
<input type="password" id="passInput" placeholder="请输入密码">
</div>
<div id="passMsg" class="msg"></div>
<div class="form-actions">
<button class="btn btn-ghost" onclick="closeModal()">取消</button>
<button class="btn" onclick="doConnect()">连接</button>
</div>
</div>
</div>
<script>
var curSsid='';

function loadSta(){
  fetch('/api/sta').then(r=>r.json()).then(d=>{
    var s=d.state;
    if(d.ip)s+=' '+d.ip;
    document.getElementById('staStatus').textContent=s;
  }).catch(function(e){});
}

function scanWiFi(){
  document.getElementById('scanStatus').textContent='正在扫描，热点可能短暂卡顿...';
  document.getElementById('wifiList').innerHTML='';
  var retries=0;
  function doScan(url){
    fetch(url).then(r=>r.json()).then(function poll(d){
      if(d.scanning){
        setTimeout(function(){doScan('/api/scan');},600);
        return;
      }
      if(!d||!d.length){
        document.getElementById('scanStatus').textContent='未找到网络';
        return;
      }
      document.getElementById('scanStatus').textContent='找到 '+d.length+' 个网络';
      var html='';
      d.forEach(function(n){
        html+='<li class="wifi-item" onclick="showPass(\''+n.ssid+'\')">';
        html+='<div class="wifi-name">'+n.ssid+'</div>';
        html+='<div class="wifi-info">';
        html+='<span>'+n.rssi+'dBm</span>';
        if(!n.open){html+='<span class="lock">&#128274;</span>';}
        html+='</div></li>';
      });
      document.getElementById('wifiList').innerHTML=html;
    }).catch(function(e){
      retries++;
      if(retries<10){
        setTimeout(function(){doScan('/api/scan');},800);
      }else{
        document.getElementById('scanStatus').textContent='扫描失败，请刷新重试';
      }
    });
  }
  doScan('/api/scan?refresh=1');
}

function showPass(ssid){
  curSsid=ssid;
  document.getElementById('passTitle').textContent='连接: '+ssid;
  document.getElementById('passInput').value='';
  document.getElementById('passMsg').className='msg';
  document.getElementById('passModal').classList.add('show');
}

function closeModal(){
  document.getElementById('passModal').classList.remove('show');
}

function doConnect(){
  var pwd=document.getElementById('passInput').value;
  var m=document.getElementById('passMsg');
  if(!pwd){m.textContent='请输入密码';m.className='msg show msg-error';return;}
  m.textContent='正在连接...';m.className='msg show';
  fetch('/api/connect?ssid='+encodeURIComponent(curSsid)+'&pwd='+encodeURIComponent(pwd))
    .then(r=>r.json()).then(function poll(d){
      if(d.connecting){
        setTimeout(function(){fetch('/api/sta').then(r=>r.json()).then(function(s){
          if(s.state=='CONNECTED'){m.textContent='连接成功！';m.className='msg show msg-success';setTimeout(closeModal,1500);loadSta();}
          else if(s.state=='CONNECTING'){poll({connecting:true});}
          else{m.textContent='连接失败';m.className='msg show msg-error';}
        });},1000);
        return;
      }
      if(d.ok){
        m.textContent='连接成功！';m.className='msg show msg-success';
        setTimeout(closeModal,1500);
        setTimeout(loadSta,1000);
      }else{
        m.textContent='连接失败: '+d.msg;m.className='msg show msg-error';
      }
    });
}

scanWiFi();
loadSta();
setInterval(loadSta,3000);
</script>
</body>
</html>
)rawliteral";

static void _handle_root()
{
    s_server->send_P(200, "text/html", INDEX_HTML);
}

static void _handle_scan()
{
    bool need_refresh = s_server->hasArg("refresh");

    if (!srv_wifi_scan_done()) {
        s_server->send(200, "application/json", "{\"scanning\":true}");
        return;
    }

    if (need_refresh || srv_wifi_scan_count() == 0) {
        srv_wifi_scan_start();
        s_server->send(200, "application/json", "{\"scanning\":true}");
        return;
    }

    String json = "[";
    uint8_t n = srv_wifi_scan_count();
    for (uint8_t i = 0; i < n; i++) {
        wifi_scan_item_t item;
        if (srv_wifi_scan_get_item(i, &item)) {
            if (i > 0) json += ",";
            json += "{";
            json += "\"ssid\":\"" + String(item.ssid) + "\",";
            json += "\"rssi\":" + String(item.rssi) + ",";
            json += "\"ch\":" + String(item.channel) + ",";
            json += "\"open\":" + String(item.enc_type == WIFI_AUTH_OPEN ? "true" : "false");
            json += "}";
        }
    }
    json += "]";
    s_server->send(200, "application/json", json);
}

static void _handle_connect()
{
    if (!s_server->hasArg("ssid") || !s_server->hasArg("pwd")) {
        s_server->send(200, "application/json", "{\"ok\":false,\"msg\":\"参数错误\"}");
        return;
    }
    String ssid = s_server->arg("ssid");
    String pwd = s_server->arg("pwd");

    srv_wifi_connect(ssid.c_str(), pwd.c_str());
    s_server->send(200, "application/json", "{\"connecting\":true}");
}

static void _handle_sta_status()
{
    String json = "{";
    json += "\"state\":\"" + String(srv_wifi_get_state_str()) + "\",";
    json += "\"ssid\":\"" + String(srv_wifi_get_ssid()) + "\",";
    json += "\"ip\":\"" + String(srv_wifi_get_ip()) + "\"";
    json += "}";
    s_server->send(200, "application/json", json);
}

static void _handle_not_found()
{
    IPAddress ip = WiFi.softAPIP();
    String url = "http://" + ip.toString() + "/";
    s_server->sendHeader("Location", url, true);
    s_server->send(302, "text/plain", "");
}

app_err_t srv_webconfig_init(void)
{
    if (s_initialized) {
        return APP_OK;
    }

    s_server = new WebServer(WEBCONFIG_PORT);
    s_dns = new DNSServer();

    s_server->on("/", HTTP_GET, _handle_root);
    s_server->on("/api/scan", HTTP_GET, _handle_scan);
    s_server->on("/api/connect", HTTP_GET, _handle_connect);
    s_server->on("/api/sta", HTTP_GET, _handle_sta_status);
    s_server->onNotFound(_handle_not_found);
    s_server->begin();

    s_dns->start(WEBCONFIG_DNS_PORT, "*", WiFi.softAPIP());

    s_initialized = true;
    LOG_I("webconfig init ok, port=%d", WEBCONFIG_PORT);
    return APP_OK;
}

void srv_webconfig_task_process(void)
{
    if (!s_initialized) return;
    s_dns->processNextRequest();
    s_server->handleClient();
}

static app_err_t srv_webconfig_service_init(void)
{
    return srv_webconfig_init();
}

SERVICE_REGISTER("webconfig", srv_webconfig_service_init, 70);
