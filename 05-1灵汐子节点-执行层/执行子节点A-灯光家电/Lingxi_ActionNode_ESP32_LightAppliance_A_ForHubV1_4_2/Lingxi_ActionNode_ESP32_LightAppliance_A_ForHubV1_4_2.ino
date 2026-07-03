/*
 * 灵汐全屋智能控制中心 - 执行层子节点A：灯光家电
 * 节点封装版本：V1.0.1
 * 兼容中控版本：公测版 V1.4.2
 *
 * 节点职责：
 * 1. 控制客厅灯与卧室灯。
 * 2. 控制四路 5V 继电器：空调、电视、冰箱、热水器。
 * 3. 通过 HTTP 向冻结中控注册、上报状态、接收控制命令。
 * 4. 保留本地配网页、保存 WiFi/中控地址、WiFi 断线重连、失败后自动开启配网热点。
 *
 * 工程边界：
 * - 中控 Lingxi_PublicBeta_V1_4_2 已冻结，本节点不得要求修改中控。
 * - 本文件只保留灯光与家电继电器逻辑，不允许混入电机、舵机代码。
 * - 设备管理前端当前仍是固定模板，本节点通过 endpoint id 与中控兼容层对接。
 *
 * 可改范围：
 * - 可按实际接线调整 GPIO 引脚。
 * - 可按实际灯带数量调整 LED 数量。
 * - 可按现场网络调整默认中控地址。
 *
 * 禁止更改：
 * - node_id、endpoint id、HTTP 接口路径、上报字段名，除非同步解冻并修改中控。
 */

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

// =====================================================
// 1. 固件身份与协议常量
// =====================================================
#define LINGXI_PRODUCT_NAME              "灵汐全屋智能控制中心"
#define LINGXI_NODE_ID                   "ActionNode-LightAppliance-01"
#define LINGXI_NODE_NAME                 "执行层子节点A-灯光家电"
#define LINGXI_NODE_TYPE                 "action_board"
#define LINGXI_NODE_ROLE                 "executor"
#define LINGXI_NODE_FIRMWARE             "Lingxi_ActionNode_ESP32_LightAppliance_A_V1_0_1_ForHubV1_4_2"
#define LINGXI_NODE_VERSION              "执行层子节点A V1.0.1 / 兼容中控 V1.4.2"

#define LINGXI_DEFAULT_GATEWAY_HOST      "192.168.4.1"
#define LINGXI_GATEWAY_PORT              80
#define LINGXI_CONFIG_AP_SSID            "灵汐-执行A灯光家电"
#define LINGXI_CONFIG_AP_PASSWORD        "999999999"
#define LINGXI_DNS_PORT                  53

// =====================================================
// 2. ESP32 基础版引脚规划
// =====================================================
// 说明：本节点使用 ESP32 基础版。避开 GPIO6~GPIO11、GPIO34~GPIO39、串口下载脚和启动敏感脚。
// 灯光按 WS2812/NeoPixel 灯带处理；如果后续换成普通 LED，可只改 applyLightStrip()。
#define LIVING_LIGHT_PIN                 16
#define BEDROOM_LIGHT_PIN                17
#define LIVING_LIGHT_NUM                 8
#define BEDROOM_LIGHT_NUM                8

#define RELAY_AIR_PIN                    18
#define RELAY_TV_PIN                     19
#define RELAY_FRIDGE_PIN                 21
#define RELAY_WATER_HEATER_PIN           22

// 四路继电器按常见 5V 继电器模块低电平触发设计。
// 重要：GPIO 只接 IN 信号，不给继电器线圈供电；继电器 VCC 走稳定 5V，并与 ESP32 共地。
#define RELAY_ON_LEVEL                   LOW
#define RELAY_OFF_LEVEL                  HIGH

// =====================================================
// 3. 周期参数
// =====================================================
#define WIFI_CONNECT_TIMEOUT_MS          12000UL
#define WIFI_RETRY_INTERVAL_MS           10000UL
#define HTTP_REGISTER_INTERVAL_MS        15000UL
#define HTTP_REPORT_INTERVAL_MS          2000UL
#define HTTP_TIMEOUT_MS                  2500UL
#define CONFIG_AP_CLOSE_DELAY_MS         30000UL
#define REGISTER_FAIL_OPEN_AP_COUNT      5
#define RELAY_MIN_ACTION_INTERVAL_MS     180UL

// =====================================================
// 4. 函数前置声明
// =====================================================
String buildConfigPage(String message);
void handleConfigRoot();
void handleConfigSave();
void startConfigPortal(const char* reason);
void stopConfigPortalIfReady();
bool handleCommandBody(const String &body);
bool sendHttpReport(bool forceNow);

// =====================================================
// 4. 运行态对象与状态
// =====================================================
Preferences nodePrefs;
WebServer server(80);
DNSServer dnsServer;

Adafruit_NeoPixel livingStrip(LIVING_LIGHT_NUM, LIVING_LIGHT_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel bedroomStrip(BEDROOM_LIGHT_NUM, BEDROOM_LIGHT_PIN, NEO_GRB + NEO_KHZ800);

String wifiSsid = "";
String wifiPassword = "";
String gatewayHost = LINGXI_DEFAULT_GATEWAY_HOST;
String localMac = "";

bool hasSavedWifiConfig = false;
bool configPortalRunning = false;
bool webRoutesReady = false;
bool httpRegisterOk = false;
bool wifiHttpOk = false;
int registerFailCount = 0;

unsigned long lastWifiRetryMs = 0;
unsigned long wifiConnectedSinceMs = 0;
unsigned long lastHttpRegisterMs = 0;
unsigned long lastHttpReportMs = 0;
unsigned long reportSeq = 1;
unsigned long lastHttpSuccessMs = 0;
unsigned long configPortalOkSinceMs = 0;
unsigned long lastAirRelayMs = 0;
unsigned long lastTvRelayMs = 0;
unsigned long lastFridgeRelayMs = 0;
unsigned long lastWaterRelayMs = 0;
unsigned long lastCmdSeq = 0;

struct LightApplianceState {
  int livingLight;
  int bedroomLight;
  int livingLightLevel;
  int bedroomLightLevel;
  int airRelay;
  int tvRelay;
  int fridgeRelay;
  int waterHeaterRelay;
};

LightApplianceState state = {0, 0, 0, 0, 0, 0, 0, 0};

// =====================================================
// 5. 字符串工具
// =====================================================
String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "\\r");
  s.replace("\t", "\\t");
  return s;
}

String htmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

String jsonValue(const String &body, const char* key, const String &fallback = "") {
  String pattern = String("\"") + key + "\"";
  int p = body.indexOf(pattern);
  if (p < 0) return fallback;
  p = body.indexOf(':', p + pattern.length());
  if (p < 0) return fallback;
  p++;
  while (p < (int)body.length() && (body[p] == ' ' || body[p] == '\t' || body[p] == '\r' || body[p] == '\n')) p++;
  if (p >= (int)body.length()) return fallback;
  if (body[p] == '"') {
    p++;
    String out = "";
    bool esc = false;
    for (; p < (int)body.length(); p++) {
      char c = body[p];
      if (esc) { out += c; esc = false; continue; }
      if (c == '\\') { esc = true; continue; }
      if (c == '"') break;
      out += c;
    }
    return out;
  }
  int e = p;
  while (e < (int)body.length() && body[e] != ',' && body[e] != '}') e++;
  String out = body.substring(p, e);
  out.trim();
  return out.length() ? out : fallback;
}

int jsonInt(const String &body, const char* key, int fallback = 0) {
  String v = jsonValue(body, key, "");
  if (!v.length()) return fallback;
  return v.toInt();
}

// =====================================================
// 6. 配置存储
// =====================================================
void loadNodeConfig() {
  nodePrefs.begin("lx_action_a", true);
  wifiSsid = nodePrefs.getString("ssid", "");
  wifiPassword = nodePrefs.getString("pass", "");
  gatewayHost = nodePrefs.getString("gw", LINGXI_DEFAULT_GATEWAY_HOST);
  nodePrefs.end();

  wifiSsid.trim();
  gatewayHost.trim();
  if (!gatewayHost.length()) gatewayHost = LINGXI_DEFAULT_GATEWAY_HOST;
  hasSavedWifiConfig = wifiSsid.length() > 0;

  Serial.println("[Config] load done");
  Serial.println("[Config] ssid=" + wifiSsid);
  Serial.println("[Config] gateway=" + gatewayHost);
}

void saveNodeConfig(const String &ssid, const String &pass, const String &gw) {
  nodePrefs.begin("lx_action_a", false);
  nodePrefs.putString("ssid", ssid);
  nodePrefs.putString("pass", pass);
  nodePrefs.putString("gw", gw.length() ? gw : String(LINGXI_DEFAULT_GATEWAY_HOST));
  nodePrefs.end();
}

// =====================================================
// 7. 配网页面
// =====================================================
String buildConfigPage(String message) {
  String html;
  html += "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>灵汐执行A灯光家电配网</title>";
  html += "<style>body{margin:0;background:#eef4ff;font-family:Arial,'Microsoft YaHei',sans-serif;color:#102033;}";
  html += ".wrap{max-width:620px;margin:0 auto;padding:22px}.card{background:#fff;border-radius:18px;padding:22px;box-shadow:0 12px 36px rgba(20,60,120,.14)}";
  html += "h1{font-size:24px;margin:0 0 6px}.sub{color:#667085;margin-bottom:18px}.row{margin:12px 0}.label{font-weight:700;margin-bottom:6px}";
  html += "input{box-sizing:border-box;width:100%;padding:12px;border:1px solid #ccd6e6;border-radius:12px;font-size:16px}button{width:100%;border:0;border-radius:12px;padding:13px;margin-top:16px;font-size:16px;background:#0b7cff;color:white;font-weight:700}";
  html += ".tip{font-size:13px;color:#667085;line-height:1.7;background:#f5f8ff;border-radius:12px;padding:10px;margin-top:14px}.msg{color:#0b7cff;font-weight:700;margin:8px 0}";
  html += "</style></head><body><div class='wrap'><div class='card'>";
  html += "<h1>灵汐执行层子节点A</h1><div class='sub'>灯光家电配网</div>";
  html += "<div class='tip'>节点名称：执行层子节点A-灯光家电<br>节点 ID：" + String(LINGXI_NODE_ID) + "<br>节点类型：执行节点<br>热点密码：" + String(LINGXI_CONFIG_AP_PASSWORD) + "</div>";
  if (message.length()) html += "<div class='msg'>" + htmlEscape(message) + "</div>";
  html += "<form method='POST' action='/save'>";
  html += "<div class='row'><div class='label'>WiFi 名称</div><input name='ssid' value='" + htmlEscape(wifiSsid) + "' required></div>";
  html += "<div class='row'><div class='label'>WiFi 密码</div><input name='pass' type='password' value='" + htmlEscape(wifiPassword) + "'></div>";
  html += "<div class='row'><div class='label'>中控地址</div><input name='gw' value='" + htmlEscape(gatewayHost) + "' placeholder='192.168.4.1'></div>";
  html += "<button type='submit'>保存并连接</button></form>";
  html += "<div class='tip'>配网成功并注册中控后，热点将在约 30 秒后自动关闭。WiFi 连接失败或运行中断开时，节点会自动重新开启配网热点。</div>";
  html += "</div></div></body></html>";
  return html;
}

void handleConfigRoot() {
  server.send(200, "text/html; charset=utf-8", buildConfigPage(""));
}

void handleConfigSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  String gw = server.arg("gw");
  ssid.trim();
  gw.trim();
  if (!ssid.length()) {
    server.send(200, "text/html; charset=utf-8", buildConfigPage("WiFi 名称不能为空"));
    return;
  }
  if (!gw.length()) gw = LINGXI_DEFAULT_GATEWAY_HOST;
  saveNodeConfig(ssid, pass, gw);
  server.send(200, "text/html; charset=utf-8", buildConfigPage("配置已保存，节点正在重启并连接路由器 WiFi。"));
  delay(800);
  ESP.restart();
}

String buildLocalStatusJson() {
  String json = "{";
  json += "\"ok\":true,";
  json += "\"node_id\":\"" + jsonEscape(String(LINGXI_NODE_ID)) + "\",";
  json += "\"name\":\"" + jsonEscape(String(LINGXI_NODE_NAME)) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"ap\":" + String(configPortalRunning ? "true" : "false") + ",";
  json += "\"registered\":" + String(httpRegisterOk ? "true" : "false") + ",";
  json += "\"last_cmd_seq\":" + String(lastCmdSeq) + ",";
  json += "\"living_light\":" + String(state.livingLight) + ",";
  json += "\"bedroom_light\":" + String(state.bedroomLight) + ",";
  json += "\"air_relay\":" + String(state.airRelay) + ",";
  json += "\"tv_relay\":" + String(state.tvRelay) + ",";
  json += "\"fridge_relay\":" + String(state.fridgeRelay) + ",";
  json += "\"water_heater_relay\":" + String(state.waterHeaterRelay);
  json += "}";
  return json;
}

void registerWebRoutes() {
  if (webRoutesReady) return;
  webRoutesReady = true;
  server.on("/", HTTP_GET, handleConfigRoot);
  server.on("/save", HTTP_POST, handleConfigSave);
  server.on("/api/status", HTTP_GET, []() { server.send(200, "application/json; charset=utf-8", buildLocalStatusJson()); });
  server.on("/api/node/command", HTTP_POST, []() {
    String body = server.arg("plain");
    bool ok = handleCommandBody(body);
    server.send(ok ? 200 : 400, "application/json; charset=utf-8", ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });
  server.onNotFound([]() {
    if (configPortalRunning) server.send(200, "text/html; charset=utf-8", buildConfigPage(""));
    else server.send(404, "text/plain; charset=utf-8", "not found");
  });
}

void startConfigPortal(const char* reason) {
  if (configPortalRunning) return;
  Serial.print("[ConfigPortal] start reason=");
  Serial.println(reason ? reason : "unknown");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 8, 1), IPAddress(192, 168, 8, 1), IPAddress(255, 255, 255, 0));
  bool apOk = WiFi.softAP(LINGXI_CONFIG_AP_SSID, LINGXI_CONFIG_AP_PASSWORD);
  configPortalRunning = apOk;
  configPortalOkSinceMs = 0;
  if (apOk) {
    dnsServer.start(LINGXI_DNS_PORT, "*", WiFi.softAPIP());
    Serial.print("[ConfigPortal] SSID="); Serial.println(LINGXI_CONFIG_AP_SSID);
    Serial.print("[ConfigPortal] IP="); Serial.println(WiFi.softAPIP());
  }
}

void stopConfigPortalIfReady() {
  if (!configPortalRunning) return;
  if (WiFi.status() != WL_CONNECTED || !httpRegisterOk) {
    configPortalOkSinceMs = 0;
    return;
  }
  if (configPortalOkSinceMs == 0) {
    configPortalOkSinceMs = millis();
    return;
  }
  if (millis() - configPortalOkSinceMs < CONFIG_AP_CLOSE_DELAY_MS) return;
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  configPortalRunning = false;
  configPortalOkSinceMs = 0;
  Serial.println("[ConfigPortal] registered, AP closed");
}

// =====================================================
// 8. 灯光与继电器控制
// =====================================================
void fillStrip(Adafruit_NeoPixel &strip, int num, int r, int g, int b, int level) {
  level = constrain(level, 0, 255);
  uint32_t color = strip.Color((r * level) / 255, (g * level) / 255, (b * level) / 255);
  for (int i = 0; i < num; i++) strip.setPixelColor(i, color);
  strip.show();
}

void applyLightStrip(Adafruit_NeoPixel &strip, int num, bool on, int level) {
  if (!on) {
    strip.clear();
    strip.show();
    return;
  }
  if (level <= 0) level = 180;
  fillStrip(strip, num, 255, 225, 180, level);
}

bool setRelaySafe(const char* name, uint8_t pin, bool on, int &savedState, unsigned long &lastActionMs) {
  unsigned long now = millis();
  if (savedState == (on ? 1 : 0)) return false;
  if (lastActionMs != 0 && now - lastActionMs < RELAY_MIN_ACTION_INTERVAL_MS) {
    Serial.print("[Relay] fast repeat ignored: ");
    Serial.println(name);
    return false;
  }
  digitalWrite(pin, on ? RELAY_ON_LEVEL : RELAY_OFF_LEVEL);
  savedState = on ? 1 : 0;
  lastActionMs = now;
  Serial.print("[Relay] "); Serial.print(name); Serial.println(on ? " ON" : " OFF");
  return true;
}

void applyLightApplianceCommand(const String &body) {
  int livingCmd = jsonInt(body, "keTingLightCmd", state.livingLight);
  int bedroomCmd = jsonInt(body, "woShiLightCmd", state.bedroomLight);
  int lightLevel = jsonInt(body, "lightLevel", 180);

  state.livingLight = livingCmd ? 1 : 0;
  state.bedroomLight = bedroomCmd ? 1 : 0;
  state.livingLightLevel = state.livingLight ? lightLevel : 0;
  state.bedroomLightLevel = state.bedroomLight ? lightLevel : 0;
  applyLightStrip(livingStrip, LIVING_LIGHT_NUM, state.livingLight, state.livingLightLevel);
  applyLightStrip(bedroomStrip, BEDROOM_LIGHT_NUM, state.bedroomLight, state.bedroomLightLevel);

  setRelaySafe("air", RELAY_AIR_PIN, jsonInt(body, "airRelayCmd", state.airRelay) != 0, state.airRelay, lastAirRelayMs);
  setRelaySafe("tv", RELAY_TV_PIN, jsonInt(body, "tvRelayCmd", state.tvRelay) != 0, state.tvRelay, lastTvRelayMs);
  setRelaySafe("fridge", RELAY_FRIDGE_PIN, jsonInt(body, "fridgeRelayCmd", state.fridgeRelay) != 0, state.fridgeRelay, lastFridgeRelayMs);
  setRelaySafe("water_heater", RELAY_WATER_HEATER_PIN, jsonInt(body, "waterRelayCmd", state.waterHeaterRelay) != 0, state.waterHeaterRelay, lastWaterRelayMs);
}

bool handleCommandBody(const String &body) {
  if (!body.length()) return false;
  unsigned long seq = (unsigned long)jsonInt(body, "cmdSeq", 0);
  if (seq != 0 && seq == lastCmdSeq) {
    Serial.println("[Command] duplicate ignored");
    return true;
  }
  if (seq != 0) lastCmdSeq = seq;
  Serial.println("[Command] HTTP command received seq=" + String(seq));
  applyLightApplianceCommand(body);
  sendHttpReport(true);
  return true;
}

void initHardware() {
  livingStrip.begin();
  livingStrip.clear();
  livingStrip.show();
  livingStrip.setBrightness(255);

  bedroomStrip.begin();
  bedroomStrip.clear();
  bedroomStrip.show();
  bedroomStrip.setBrightness(255);

  digitalWrite(RELAY_AIR_PIN, RELAY_OFF_LEVEL);
  digitalWrite(RELAY_TV_PIN, RELAY_OFF_LEVEL);
  digitalWrite(RELAY_FRIDGE_PIN, RELAY_OFF_LEVEL);
  digitalWrite(RELAY_WATER_HEATER_PIN, RELAY_OFF_LEVEL);
  pinMode(RELAY_AIR_PIN, OUTPUT);
  pinMode(RELAY_TV_PIN, OUTPUT);
  pinMode(RELAY_FRIDGE_PIN, OUTPUT);
  pinMode(RELAY_WATER_HEATER_PIN, OUTPUT);
  digitalWrite(RELAY_AIR_PIN, RELAY_OFF_LEVEL);
  digitalWrite(RELAY_TV_PIN, RELAY_OFF_LEVEL);
  digitalWrite(RELAY_FRIDGE_PIN, RELAY_OFF_LEVEL);
  digitalWrite(RELAY_WATER_HEATER_PIN, RELAY_OFF_LEVEL);
  Serial.println("[Hardware] LightAppliance outputs ready, all safe OFF");
}

// =====================================================
// 9. 中控注册与上报
// =====================================================
String endpointsJson() {
  String json = "[";
  json += "{\"id\":\"living_light\",\"name\":\"客厅灯\",\"type\":\"action\",\"capability\":\"switch\",\"room\":\"客厅\"},";
  json += "{\"id\":\"bedroom_light\",\"name\":\"卧室灯\",\"type\":\"action\",\"capability\":\"switch\",\"room\":\"卧室\"},";
  json += "{\"id\":\"air_relay\",\"name\":\"空调电源\",\"type\":\"action\",\"capability\":\"power\",\"room\":\"客厅\"},";
  json += "{\"id\":\"tv_relay\",\"name\":\"电视电源\",\"type\":\"action\",\"capability\":\"power\",\"room\":\"客厅\"},";
  json += "{\"id\":\"fridge_relay\",\"name\":\"冰箱电源\",\"type\":\"action\",\"capability\":\"power\",\"room\":\"厨房\"},";
  json += "{\"id\":\"water_heater_relay\",\"name\":\"热水器电源\",\"type\":\"action\",\"capability\":\"power\",\"room\":\"卫生间\"}";
  json += "]";
  return json;
}

String gatewayUrl(const char* path) {
  return String("http://") + gatewayHost + ":" + String(LINGXI_GATEWAY_PORT) + String(path);
}

String buildRegisterJson() {
  String json = "{";
  json += "\"node_id\":\"" + jsonEscape(String(LINGXI_NODE_ID)) + "\",";
  json += "\"name\":\"" + jsonEscape(String(LINGXI_NODE_NAME)) + "\",";
  json += "\"display_name\":\"" + jsonEscape(String(LINGXI_NODE_NAME)) + "\",";
  json += "\"node_type\":\"" + jsonEscape(String(LINGXI_NODE_TYPE)) + "\",";
  json += "\"role\":\"" + jsonEscape(String(LINGXI_NODE_ROLE)) + "\",";
  json += "\"transport\":\"wifi_http\",";
  json += "\"mac\":\"" + jsonEscape(localMac) + "\",";
  json += "\"firmware\":\"" + jsonEscape(String(LINGXI_NODE_FIRMWARE)) + "\",";
  json += "\"status\":\"registered\",";
  json += "\"endpoints\":" + endpointsJson();
  json += "}";
  return json;
}

String buildReportJson() {
  String json = "{";
  json += "\"node_id\":\"" + jsonEscape(String(LINGXI_NODE_ID)) + "\",";
  json += "\"name\":\"" + jsonEscape(String(LINGXI_NODE_NAME)) + "\",";
  json += "\"node_type\":\"" + jsonEscape(String(LINGXI_NODE_TYPE)) + "\",";
  json += "\"role\":\"" + jsonEscape(String(LINGXI_NODE_ROLE)) + "\",";
  json += "\"transport\":\"wifi_http\",";
  json += "\"mac\":\"" + jsonEscape(localMac) + "\",";
  json += "\"status\":\"online\",";
  json += "\"seq\":" + String(reportSeq++) + ",";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"last_cmd_seq\":" + String(lastCmdSeq) + ",";
  json += "\"living_light\":" + String(state.livingLight) + ",";
  json += "\"bedroom_light\":" + String(state.bedroomLight) + ",";
  json += "\"living_light_level\":" + String(state.livingLightLevel) + ",";
  json += "\"bedroom_light_level\":" + String(state.bedroomLightLevel) + ",";
  json += "\"air_relay\":" + String(state.airRelay) + ",";
  json += "\"tv_relay\":" + String(state.tvRelay) + ",";
  json += "\"fridge_relay\":" + String(state.fridgeRelay) + ",";
  json += "\"water_heater_relay\":" + String(state.waterHeaterRelay);
  json += "}";
  return json;
}

bool postJsonToGateway(const char* path, const String &payload) {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClient client;
  HTTPClient http;
  String url = gatewayUrl(path);
  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  http.end();
  bool ok = (code >= 200 && code < 300);
  Serial.print("[HTTP] POST "); Serial.print(path); Serial.print(" code="); Serial.println(code);
  if (ok) lastHttpSuccessMs = millis();
  return ok;
}

bool sendHttpRegister() {
  bool ok = postJsonToGateway("/api/node/register", buildRegisterJson());
  httpRegisterOk = ok;
  if (ok) registerFailCount = 0;
  else registerFailCount++;
  return ok;
}

bool sendHttpReport(bool forceNow) {
  (void)forceNow;
  return postJsonToGateway("/api/node/report", buildReportJson());
}

// =====================================================
// 10. WiFi 与 HTTP 主链路
// =====================================================
void connectWifiOnce() {
  if (!hasSavedWifiConfig) {
    startConfigPortal("first_boot_no_saved_config");
    return;
  }
  WiFi.mode(configPortalRunning ? WIFI_AP_STA : WIFI_STA);
  WiFi.setSleep(false);
  localMac = WiFi.macAddress();
  Serial.println("[WiFi] connect ssid=" + wifiSsid);
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    wifiHttpOk = true;
    wifiConnectedSinceMs = millis();
    Serial.print("[WiFi] connected IP="); Serial.println(WiFi.localIP());
    sendHttpRegister();
  } else {
    wifiHttpOk = false;
    Serial.println("[WiFi] connect failed, open config portal");
    startConfigPortal("wifi_connect_failed");
  }
}

void updateNetwork() {
  if (configPortalRunning) dnsServer.processNextRequest();
  server.handleClient();
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    wifiHttpOk = false;
    httpRegisterOk = false;
    wifiConnectedSinceMs = 0;
    if (!configPortalRunning) startConfigPortal("wifi_disconnected_reopen_portal");
    if (hasSavedWifiConfig && now - lastWifiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
      lastWifiRetryMs = now;
      WiFi.disconnect(false, false);
      delay(50);
      WiFi.mode(configPortalRunning ? WIFI_AP_STA : WIFI_STA);
      WiFi.setSleep(false);
      WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
      Serial.println("[WiFi] retry connect");
    }
    return;
  }

  if (wifiConnectedSinceMs == 0) wifiConnectedSinceMs = now;
  wifiHttpOk = true;

  if (!httpRegisterOk || now - lastHttpRegisterMs >= HTTP_REGISTER_INTERVAL_MS) {
    lastHttpRegisterMs = now;
    sendHttpRegister();
    if (!httpRegisterOk && registerFailCount >= REGISTER_FAIL_OPEN_AP_COUNT && !configPortalRunning) {
      startConfigPortal("gateway_register_failed");
    }
  }

  if (now - lastHttpReportMs >= HTTP_REPORT_INTERVAL_MS) {
    lastHttpReportMs = now;
    sendHttpReport(false);
  }

  stopConfigPortalIfReady();
}

// =====================================================
// 11. 串口辅助
// =====================================================
void printHelp() {
  Serial.println();
  Serial.println("========== Lingxi ActionNode A Help ==========");
  Serial.println("1: 客厅灯开");
  Serial.println("2: 客厅灯关");
  Serial.println("3: 卧室灯开");
  Serial.println("4: 卧室灯关");
  Serial.println("a/z: 空调继电器开/关");
  Serial.println("s/x: 电视继电器开/关");
  Serial.println("d/c: 冰箱继电器开/关");
  Serial.println("f/v: 热水器继电器开/关");
  Serial.println("?: 显示帮助");
  Serial.println("============================================");
}

void handleSerial() {
  if (!Serial.available()) return;
  char ch = Serial.read();
  if (ch == '\r' || ch == '\n' || ch == ' ') return;
  String body = "{\"cmdSeq\":" + String(millis()) + ",";
  body += "\"keTingLightCmd\":" + String(state.livingLight) + ",";
  body += "\"woShiLightCmd\":" + String(state.bedroomLight) + ",";
  body += "\"lightLevel\":180,";
  body += "\"airRelayCmd\":" + String(state.airRelay) + ",";
  body += "\"tvRelayCmd\":" + String(state.tvRelay) + ",";
  body += "\"fridgeRelayCmd\":" + String(state.fridgeRelay) + ",";
  body += "\"waterRelayCmd\":" + String(state.waterHeaterRelay) + "}";
  if (ch == '1') body.replace("\"keTingLightCmd\":" + String(state.livingLight), "\"keTingLightCmd\":1");
  else if (ch == '2') body.replace("\"keTingLightCmd\":" + String(state.livingLight), "\"keTingLightCmd\":0");
  else if (ch == '3') body.replace("\"woShiLightCmd\":" + String(state.bedroomLight), "\"woShiLightCmd\":1");
  else if (ch == '4') body.replace("\"woShiLightCmd\":" + String(state.bedroomLight), "\"woShiLightCmd\":0");
  else if (ch == 'a') body.replace("\"airRelayCmd\":" + String(state.airRelay), "\"airRelayCmd\":1");
  else if (ch == 'z') body.replace("\"airRelayCmd\":" + String(state.airRelay), "\"airRelayCmd\":0");
  else if (ch == 's') body.replace("\"tvRelayCmd\":" + String(state.tvRelay), "\"tvRelayCmd\":1");
  else if (ch == 'x') body.replace("\"tvRelayCmd\":" + String(state.tvRelay), "\"tvRelayCmd\":0");
  else if (ch == 'd') body.replace("\"fridgeRelayCmd\":" + String(state.fridgeRelay), "\"fridgeRelayCmd\":1");
  else if (ch == 'c') body.replace("\"fridgeRelayCmd\":" + String(state.fridgeRelay), "\"fridgeRelayCmd\":0");
  else if (ch == 'f') body.replace("\"waterRelayCmd\":" + String(state.waterHeaterRelay), "\"waterRelayCmd\":1");
  else if (ch == 'v') body.replace("\"waterRelayCmd\":" + String(state.waterHeaterRelay), "\"waterRelayCmd\":0");
  else if (ch == '?') { printHelp(); return; }
  else return;
  handleCommandBody(body);
}

// =====================================================
// 12. Arduino 主流程
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println();
  Serial.println("========================================");
  Serial.println("灵汐执行层子节点A - 灯光家电");
  Serial.println("兼容中控：公测版 V1.4.2");
  Serial.println("========================================");

  loadNodeConfig();
  initHardware();
  registerWebRoutes();

  // 重要修复：ESP32 基础版上 WebServer.begin() 必须放在 WiFi.mode()/softAP()/begin() 之后。
  // 如果在网络栈初始化前启动 WebServer，部分 ESP32 Arduino Core 会触发
  // xQueueSemaphoreTake queue.c 断言并反复重启，导致热点看不见。
  connectWifiOnce();
  server.begin();
  Serial.println("[Web] server started");
  printHelp();
}

void loop() {
  updateNetwork();
  handleSerial();
}
