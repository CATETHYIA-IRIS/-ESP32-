/*
 * 灵汐全屋智能控制中心 - 执行层子节点B：门窗伴侣
 * 节点封装版本：V1.0.2 C3PinStatusLightPatch
 * 兼容中控版本：公测版 V1.4.2
 *
 * 节点职责：
 * 1. 控制客厅窗帘 28BYJ-48 + ULN2003 步进电机。
 * 2. 控制卧室窗帘 28BYJ-48 + ULN2003 步进电机。
 * 3. 控制窗户 180° 舵机，并保留 360° 连续舵机兼容开关。
 * 4. 通过 HTTP 向冻结中控注册、上报状态、接收控制命令。
 * 5. 保留本地配网页、保存 WiFi/中控地址、WiFi 断线重连、失败后自动开启配网热点。
 *
 * 工程边界：
 * - 中控 Lingxi_PublicBeta_V1_4_2 已冻结，本节点不得要求修改中控。
 * - 本文件只保留门窗电机与舵机逻辑，不允许混入灯光、继电器代码。
 * - ESP32-C3 只输出控制信号，不给电机和舵机供电。
 *
 * 供电要求：
 * - 28BYJ-48 + ULN2003 必须外部 5V 供电。
 * - 舵机必须外部 5V 供电。
 * - ESP32-C3 GND、ULN2003 GND、舵机 GND、外部 5V 负极必须共地。
 * - ULN2003 跳线帽必须插上，否则电机供电通路断开。
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
#define LINGXI_NODE_ID                   "ActionNode-DoorWindow-01"
#define LINGXI_NODE_NAME                 "执行层子节点B-门窗伴侣"
#define LINGXI_NODE_TYPE                 "action_board"
#define LINGXI_NODE_ROLE                 "executor"
#define LINGXI_NODE_FIRMWARE             "Lingxi_ActionNode_C3_DoorWindow_B_V1_0_2_ForHubV1_4_2"
#define LINGXI_NODE_VERSION              "执行层子节点B V1.0.2 C3通用板 / 兼容中控 V1.4.2"

#define LINGXI_DEFAULT_GATEWAY_HOST      "192.168.4.1"
#define LINGXI_GATEWAY_PORT              80
#define LINGXI_CONFIG_AP_SSID            "灵汐-执行B门窗伴侣"
#define LINGXI_CONFIG_AP_PASSWORD        "999999999"
#define LINGXI_DNS_PORT                  53

// =====================================================
// 2. ESP32-C3 通用执行子节点板引脚规划
// =====================================================
// 说明：本版本面向后续 C3 PCB 样板。A/B 两种执行子节点共用同一块 PCB，
//       通过不同固件解释同一批 IO。
//
// B 模式实际接口：
//   电机1：VCC GND IO2 IO3 IO0 IO1
//   电机2：VCC GND IO4 IO5 IO6 IO7
//   舵机：GND VCC IO10
//
// C3 引脚约定：
//   IO8 固定给 RGB 状态灯；IO9 保留 BOOT；IO18/IO19 保留 USB；IO20/IO21 尽量保留串口。
//   IO2 可用，但 PCB 外部电路不要强拉高/低，避免影响启动。
#define LIVING_MOTOR_IN1                 2
#define LIVING_MOTOR_IN2                 3
#define LIVING_MOTOR_IN3                 0
#define LIVING_MOTOR_IN4                 1

#define BEDROOM_MOTOR_IN1                4
#define BEDROOM_MOTOR_IN2                5
#define BEDROOM_MOTOR_IN3                6
#define BEDROOM_MOTOR_IN4                7

#define WINDOW_SERVO_PIN                 10

#define STATUS_RGB_PIN                   8
#define STATUS_RGB_NUM                   1

// =====================================================
// 3. 电机和舵机参数
// =====================================================
#define CURTAIN_TOTAL_STEPS              4096L
#define CURTAIN_DEMO_STEPS               4096L
#define CURTAIN_STEP_INTERVAL_MS         20UL

// 默认使用 180° 舵机。若临时仍接 360° 连续舵机，可把 1 改为 0。
#define WINDOW_SERVO_USE_180             1
#define WINDOW_SERVO_OPEN_ANGLE          90
#define WINDOW_SERVO_CLOSE_ANGLE         0
#define WINDOW_SERVO_MIN_US              500
#define WINDOW_SERVO_MAX_US              2500

#define SERVO_STOP_US                    1500
#define SERVO_OPEN_US                    1700
#define SERVO_CLOSE_US                   1300
#define WINDOW_FULL_TIME_MS              1200UL

// =====================================================
// 4. 网络周期参数
// =====================================================
#define WIFI_CONNECT_TIMEOUT_MS          12000UL
#define WIFI_RETRY_INTERVAL_MS           10000UL
#define HTTP_REGISTER_INTERVAL_MS        15000UL
#define HTTP_REPORT_INTERVAL_MS          2000UL
#define HTTP_TIMEOUT_MS                  2500UL
#define CONFIG_AP_CLOSE_DELAY_MS         30000UL
#define REGISTER_FAIL_OPEN_AP_COUNT      5

// =====================================================
// 5. 函数前置声明
// =====================================================
String buildConfigPage(String message);
void handleConfigRoot();
void handleConfigSave();
void startConfigPortal(const char* reason);
void stopConfigPortalIfReady();
bool handleCommandBody(const String &body);
bool sendHttpReport(bool forceNow);

// =====================================================
// 5. 运行态对象与状态
// =====================================================
Preferences nodePrefs;
WebServer server(80);
DNSServer dnsServer;

Adafruit_NeoPixel statusRgb(STATUS_RGB_NUM, STATUS_RGB_PIN, NEO_GRB + NEO_KHZ800);

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
unsigned long lastCmdSeq = 0;

const int HALF_STEP_SEQ[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

struct CurtainMotorState {
  int pinA;
  int pinB;
  int pinC;
  int pinD;
  bool running;
  int direction;
  int stepIndex;
  long remainSteps;
  int currentPercent;
  int targetPercent;
  unsigned long lastStepMs;
};

CurtainMotorState livingMotor;
CurtainMotorState bedroomMotor;

int windowPercent = 0;
bool windowServoRunning = false;
int windowServoDirection = 0;
int windowServoTargetPercent = 0;
unsigned long windowServoStartMs = 0;
unsigned long windowServoRunMs = 0;

// =====================================================
// 6. 字符串工具
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
// 7. 配置存储
// =====================================================
void loadNodeConfig() {
  nodePrefs.begin("lx_action_b", true);
  wifiSsid = nodePrefs.getString("ssid", "");
  wifiPassword = nodePrefs.getString("pass", "");
  gatewayHost = nodePrefs.getString("gw", LINGXI_DEFAULT_GATEWAY_HOST);
  livingMotor.currentPercent = nodePrefs.getInt("living_pct", 0);
  bedroomMotor.currentPercent = nodePrefs.getInt("bedroom_pct", 0);
  windowPercent = nodePrefs.getInt("window_pct", 0);
  nodePrefs.end();

  wifiSsid.trim();
  gatewayHost.trim();
  if (!gatewayHost.length()) gatewayHost = LINGXI_DEFAULT_GATEWAY_HOST;
  hasSavedWifiConfig = wifiSsid.length() > 0;
  livingMotor.currentPercent = constrain(livingMotor.currentPercent, 0, 100);
  bedroomMotor.currentPercent = constrain(bedroomMotor.currentPercent, 0, 100);
  windowPercent = constrain(windowPercent, 0, 100);

  Serial.println("[Config] load done");
  Serial.println("[Config] ssid=" + wifiSsid);
  Serial.println("[Config] gateway=" + gatewayHost);
}

void saveNodeConfig(const String &ssid, const String &pass, const String &gw) {
  nodePrefs.begin("lx_action_b", false);
  nodePrefs.putString("ssid", ssid);
  nodePrefs.putString("pass", pass);
  nodePrefs.putString("gw", gw.length() ? gw : String(LINGXI_DEFAULT_GATEWAY_HOST));
  nodePrefs.end();
}

void saveDoorWindowPositions() {
  nodePrefs.begin("lx_action_b", false);
  nodePrefs.putInt("living_pct", livingMotor.currentPercent);
  nodePrefs.putInt("bedroom_pct", bedroomMotor.currentPercent);
  nodePrefs.putInt("window_pct", windowPercent);
  nodePrefs.end();
}

// =====================================================
// 8. 配网页面
// =====================================================
String buildConfigPage(String message) {
  String html;
  html += "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>灵汐执行B门窗伴侣配网</title>";
  html += "<style>body{margin:0;background:#eef4ff;font-family:Arial,'Microsoft YaHei',sans-serif;color:#102033;}";
  html += ".wrap{max-width:620px;margin:0 auto;padding:22px}.card{background:#fff;border-radius:18px;padding:22px;box-shadow:0 12px 36px rgba(20,60,120,.14)}";
  html += "h1{font-size:24px;margin:0 0 6px}.sub{color:#667085;margin-bottom:18px}.row{margin:12px 0}.label{font-weight:700;margin-bottom:6px}";
  html += "input{box-sizing:border-box;width:100%;padding:12px;border:1px solid #ccd6e6;border-radius:12px;font-size:16px}button{width:100%;border:0;border-radius:12px;padding:13px;margin-top:16px;font-size:16px;background:#0b7cff;color:white;font-weight:700}";
  html += ".tip{font-size:13px;color:#667085;line-height:1.7;background:#f5f8ff;border-radius:12px;padding:10px;margin-top:14px}.msg{color:#0b7cff;font-weight:700;margin:8px 0}";
  html += "</style></head><body><div class='wrap'><div class='card'>";
  html += "<h1>灵汐执行层子节点B</h1><div class='sub'>门窗伴侣配网</div>";
  html += "<div class='tip'>节点名称：执行层子节点B-门窗伴侣<br>节点 ID：" + String(LINGXI_NODE_ID) + "<br>节点类型：执行节点<br>热点密码：" + String(LINGXI_CONFIG_AP_PASSWORD) + "</div>";
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

// =====================================================
// 9. 步进电机与舵机控制
// =====================================================
void driveMotorOutput(CurtainMotorState &m, int idx) {
  digitalWrite(m.pinA, HALF_STEP_SEQ[idx][0]);
  digitalWrite(m.pinB, HALF_STEP_SEQ[idx][1]);
  digitalWrite(m.pinC, HALF_STEP_SEQ[idx][2]);
  digitalWrite(m.pinD, HALF_STEP_SEQ[idx][3]);
}

void stopMotor(CurtainMotorState &m) {
  digitalWrite(m.pinA, LOW);
  digitalWrite(m.pinB, LOW);
  digitalWrite(m.pinC, LOW);
  digitalWrite(m.pinD, LOW);
  m.running = false;
  m.remainSteps = 0;
}

void initCurtainMotor(CurtainMotorState &m, int logicalA, int logicalB, int logicalC, int logicalD) {
  m.pinA = logicalA;
  m.pinB = logicalB;
  m.pinC = logicalC;
  m.pinD = logicalD;
  pinMode(m.pinA, OUTPUT);
  pinMode(m.pinB, OUTPUT);
  pinMode(m.pinC, OUTPUT);
  pinMode(m.pinD, OUTPUT);
  m.running = false;
  m.direction = 1;
  m.stepIndex = 0;
  m.remainSteps = 0;
  m.targetPercent = m.currentPercent;
  m.lastStepMs = 0;
  stopMotor(m);
}

void setCurtainTarget(CurtainMotorState &m, int percent) {
  percent = constrain(percent, 0, 100);
  int delta = percent - m.currentPercent;
  if (delta == 0) {
    stopMotor(m);
    return;
  }
  m.targetPercent = percent;
  m.direction = delta > 0 ? 1 : -1;
  m.remainSteps = abs(delta) * CURTAIN_TOTAL_STEPS / 100L;
  if (m.remainSteps < 10) m.remainSteps = 10;
  m.running = true;
  Serial.print("[Curtain] target="); Serial.print(percent); Serial.print(" steps="); Serial.println(m.remainSteps);
}

void startCurtainPhysicalDemo(CurtainMotorState &m, int direction, long steps) {
  if (steps <= 0) steps = CURTAIN_DEMO_STEPS;
  m.direction = direction >= 0 ? 1 : -1;
  m.remainSteps = steps;
  m.targetPercent = direction >= 0 ? 100 : 0;
  m.running = true;
}

void updateCurtainMotor(CurtainMotorState &m) {
  if (!m.running) return;
  unsigned long now = millis();
  if (now - m.lastStepMs < CURTAIN_STEP_INTERVAL_MS) return;
  m.lastStepMs = now;
  m.stepIndex += m.direction;
  if (m.stepIndex > 7) m.stepIndex = 0;
  if (m.stepIndex < 0) m.stepIndex = 7;
  driveMotorOutput(m, m.stepIndex);
  m.remainSteps--;
  if (m.remainSteps <= 0) {
    m.currentPercent = m.targetPercent;
    stopMotor(m);
    saveDoorWindowPositions();
  }
}

void servoWriteUs(int us) {
  int duty = map(us, 0, 20000, 0, 65535);
  ledcWrite(WINDOW_SERVO_PIN, duty);
}

void initWindowServo() {
  ledcAttach(WINDOW_SERVO_PIN, 50, 16);
  if (WINDOW_SERVO_USE_180) {
    int us = map(windowPercent, 0, 100, WINDOW_SERVO_MIN_US, WINDOW_SERVO_MAX_US);
    servoWriteUs(us);
  } else {
    servoWriteUs(SERVO_STOP_US);
  }
}

void stopWindowServo() {
  if (!WINDOW_SERVO_USE_180) servoWriteUs(SERVO_STOP_US);
  windowServoRunning = false;
  windowServoDirection = 0;
}

void setWindowTarget(int percent) {
  percent = constrain(percent, 0, 100);
  windowServoTargetPercent = percent;
  if (WINDOW_SERVO_USE_180) {
    int angle = map(percent, 0, 100, WINDOW_SERVO_CLOSE_ANGLE, WINDOW_SERVO_OPEN_ANGLE);
    int us = map(angle, 0, 180, WINDOW_SERVO_MIN_US, WINDOW_SERVO_MAX_US);
    servoWriteUs(us);
    windowPercent = percent;
    saveDoorWindowPositions();
    Serial.print("[WindowServo] 180 angle="); Serial.println(angle);
    return;
  }

  int delta = percent - windowPercent;
  if (delta == 0) {
    stopWindowServo();
    return;
  }
  windowServoDirection = delta > 0 ? 1 : -1;
  windowServoRunMs = abs(delta) * WINDOW_FULL_TIME_MS / 100UL;
  if (windowServoRunMs < 200) windowServoRunMs = 200;
  windowServoStartMs = millis();
  windowServoRunning = true;
  servoWriteUs(windowServoDirection > 0 ? SERVO_OPEN_US : SERVO_CLOSE_US);
}

void updateWindowServo() {
  if (WINDOW_SERVO_USE_180) return;
  if (!windowServoRunning) return;
  if (millis() - windowServoStartMs >= windowServoRunMs) {
    windowPercent = windowServoTargetPercent;
    stopWindowServo();
    saveDoorWindowPositions();
  }
}

void initHardware() {
  statusRgb.begin();
  statusRgb.clear();
  statusRgb.show();
  statusRgb.setBrightness(255);

  // M2 有效相序：逻辑 A/B/C/D -> IN1/IN3/IN2/IN4。
  // 注意：下面的顺序不是写错，是为了适配已经实测跑通的 28BYJ-48 + ULN2003 相序。
  initCurtainMotor(livingMotor, LIVING_MOTOR_IN1, LIVING_MOTOR_IN3, LIVING_MOTOR_IN2, LIVING_MOTOR_IN4);
  initCurtainMotor(bedroomMotor, BEDROOM_MOTOR_IN1, BEDROOM_MOTOR_IN3, BEDROOM_MOTOR_IN2, BEDROOM_MOTOR_IN4);
  initWindowServo();
  Serial.println("[Hardware] DoorWindow outputs ready");
}

// =====================================================
// 10. 命令处理
// =====================================================
bool handleCommandBody(const String &body) {
  if (!body.length()) return false;
  unsigned long seq = (unsigned long)jsonInt(body, "cmdSeq", 0);
  if (seq != 0 && seq == lastCmdSeq) {
    Serial.println("[Command] duplicate ignored");
    return true;
  }
  if (seq != 0) lastCmdSeq = seq;
  Serial.println("[Command] HTTP command received seq=" + String(seq));

  // 注意：0 在中控协议中经常代表“本次未操作该门窗端点”，不是窗帘关闭。
  // 因此门窗节点只响应 1=打开、2=关闭、3=百分比；忽略 0，避免旧状态字段导致误动作。
  int livingCmd = jsonInt(body, "keTingCurtainCmd", 0);
  int bedroomCmd = jsonInt(body, "woShiCurtainCmd", 0);
  int windowCmd = jsonInt(body, "windowServoCmd", 0);

  if (livingCmd == 1) setCurtainTarget(livingMotor, 100);
  else if (livingCmd == 2) setCurtainTarget(livingMotor, 0);

  if (bedroomCmd == 1) setCurtainTarget(bedroomMotor, 100);
  else if (bedroomCmd == 2) setCurtainTarget(bedroomMotor, 0);

  if (windowCmd == 1) setWindowTarget(100);
  else if (windowCmd == 2) setWindowTarget(0);

  sendHttpReport(true);
  return true;
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
  json += "\"living_curtain_percent\":" + String(livingMotor.currentPercent) + ",";
  json += "\"bedroom_curtain_percent\":" + String(bedroomMotor.currentPercent) + ",";
  json += "\"window_percent\":" + String(windowPercent);
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
// 11. RGB 状态灯
// =====================================================
uint8_t breathBrightness(unsigned long now) {
  unsigned long phase = now % 2000UL;
  if (phase > 1000UL) phase = 2000UL - phase;
  return 5 + (uint8_t)(phase * 95 / 1000UL);
}

void setStatusRgb(uint8_t r, uint8_t g, uint8_t b) {
  statusRgb.setPixelColor(0, statusRgb.Color(r, g, b));
  statusRgb.show();
}

// ============================================================
// RGB 状态灯刷新
// 可改范围：颜色、呼吸亮度、状态显示优先级
// 禁止更改：不要删除 return 结构，避免多个状态颜色互相覆盖
//
// 当前灯光含义：
// 1. 红色常亮：WiFi 未连接
// 2. 蓝绿色深呼吸：配网热点开启 / 配网中
// 3. 蓝色深呼吸：WiFi 已连接，但中控通信未确认
// 4. 绿色深呼吸：WiFi 已连接，且中控注册/上报正常
// ============================================================
void updateStatusLight() {
  unsigned long now = millis();
  uint8_t b = breathBrightness(now);

  if (configPortalRunning) {
    setStatusRgb(0, b, b);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    setStatusRgb(120, 0, 0);
    return;
  }

  if (lastHttpSuccessMs != 0 && now - lastHttpSuccessMs < 8000UL) {
    setStatusRgb(0, b, 0);
    return;
  }

  setStatusRgb(0, 0, b);
}

// =====================================================
// 12. 中控注册与上报
// =====================================================
String endpointsJson() {
  String json = "[";
  json += "{\"id\":\"living_curtain\",\"name\":\"客厅窗帘\",\"type\":\"action\",\"capability\":\"curtain\",\"room\":\"客厅\",\"unit\":\"%\"},";
  json += "{\"id\":\"bedroom_curtain\",\"name\":\"卧室窗帘\",\"type\":\"action\",\"capability\":\"curtain\",\"room\":\"卧室\",\"unit\":\"%\"},";
  json += "{\"id\":\"window_servo\",\"name\":\"窗户舵机\",\"type\":\"action\",\"capability\":\"window\",\"room\":\"卧室\",\"unit\":\"%\"}";
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
  json += "\"living_curtain\":" + String(livingMotor.currentPercent > 50 ? 1 : 0) + ",";
  json += "\"bedroom_curtain\":" + String(bedroomMotor.currentPercent > 50 ? 1 : 0) + ",";
  json += "\"window_servo\":" + String(windowPercent > 50 ? 1 : 0) + ",";
  json += "\"living_curtain_percent\":" + String(livingMotor.currentPercent) + ",";
  json += "\"bedroom_curtain_percent\":" + String(bedroomMotor.currentPercent) + ",";
  json += "\"window_percent\":" + String(windowPercent) + ",";
  json += "\"living_curtain_running\":" + String(livingMotor.running ? "true" : "false") + ",";
  json += "\"bedroom_curtain_running\":" + String(bedroomMotor.running ? "true" : "false");
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
// 13. WiFi 与 HTTP 主链路
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
// 13. 串口演示
// =====================================================
void printHelp() {
  Serial.println();
  Serial.println("========== Lingxi ActionNode B Help ==========");
  Serial.println("1: 客厅窗帘打开，按百分比动作");
  Serial.println("2: 客厅窗帘关闭，按百分比动作");
  Serial.println("3: 卧室窗帘打开，按百分比动作");
  Serial.println("4: 卧室窗帘关闭，按百分比动作");
  Serial.println("t: 两个电机同时正转一圈，直观演示");
  Serial.println("y: 两个电机同时反转一圈，直观演示");
  Serial.println("s: 立即停止两个电机");
  Serial.println("o: 窗户舵机打开");
  Serial.println("c: 窗户舵机关闭");
  Serial.println("?: 显示帮助");
  Serial.println("============================================");
}

void handleSerial() {
  if (!Serial.available()) return;
  char ch = Serial.read();
  if (ch == '\r' || ch == '\n' || ch == ' ') return;
  if (ch == '1') setCurtainTarget(livingMotor, 100);
  else if (ch == '2') setCurtainTarget(livingMotor, 0);
  else if (ch == '3') setCurtainTarget(bedroomMotor, 100);
  else if (ch == '4') setCurtainTarget(bedroomMotor, 0);
  else if (ch == 't') { startCurtainPhysicalDemo(livingMotor, 1, CURTAIN_DEMO_STEPS); startCurtainPhysicalDemo(bedroomMotor, 1, CURTAIN_DEMO_STEPS); }
  else if (ch == 'y') { startCurtainPhysicalDemo(livingMotor, -1, CURTAIN_DEMO_STEPS); startCurtainPhysicalDemo(bedroomMotor, -1, CURTAIN_DEMO_STEPS); }
  else if (ch == 's') { stopMotor(livingMotor); stopMotor(bedroomMotor); }
  else if (ch == 'o') setWindowTarget(100);
  else if (ch == 'c') setWindowTarget(0);
  else if (ch == '?') { printHelp(); return; }
  else return;
  sendHttpReport(true);
}

// =====================================================
// 15. Arduino 主流程
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println();
  Serial.println("========================================");
  Serial.println("灵汐执行层子节点B - 门窗伴侣（C3通用板）");
  Serial.println("兼容中控：公测版 V1.4.2");
  Serial.println("========================================");

  // 注意：先加载位置百分比，再初始化电机结构。
  loadNodeConfig();
  int savedLiving = livingMotor.currentPercent;
  int savedBedroom = bedroomMotor.currentPercent;
  initHardware();
  livingMotor.currentPercent = savedLiving;
  bedroomMotor.currentPercent = savedBedroom;
  livingMotor.targetPercent = savedLiving;
  bedroomMotor.targetPercent = savedBedroom;

  registerWebRoutes();

  // 重要修复：WebServer.begin() 必须放在 WiFi.mode()/softAP()/begin() 之后。
  // 这样首次无配置开机时，会先开启“灵汐-执行B门窗伴侣”热点，再启动本地网页服务。
  connectWifiOnce();
  server.begin();
  Serial.println("[Web] server started");
  printHelp();
}

void loop() {
  updateCurtainMotor(livingMotor);
  updateCurtainMotor(bedroomMotor);
  updateWindowServo();
  updateNetwork();
  updateStatusLight();
  handleSerial();
}
