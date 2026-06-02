/*
 * 灵汐全屋智能控制中心 - 感知层子节点A：环境窗户
 * 节点封装版本：V1.0.3
 * 兼容中控版本：公测版 V1.4.2
 *
 * 节点职责：
 * 1. 采集 DHT22 温湿度。
 * 2. 采集 MQ5 燃气模拟量。
 * 3. 采集窗户门磁开合状态。
 * 4. 采集客厅人体活动触发状态。
 * 5. 通过 HTTP 向中控注册节点与上报端点数据。
 * 6. 升级为同执行层一致的配网兜底逻辑：无配置开热点、WiFi/中控异常自动开热点、注册成功后延迟关闭热点。
 * 7. 调整 RGB 状态灯：红色常亮表示 WiFi 未连接，蓝绿色深呼吸表示配网中，蓝色深呼吸表示 WiFi 已连但中控通信未确认，绿色深呼吸表示完整在线。
 *
 * 工程边界：
 * - 中控 V1.4.2 已冻结，本节点不得要求中控修改。
 * - 本文件只保留环境窗户节点实际使用的采集逻辑。
 * - 本节点沿用原感知层集成板中对应传感器的 GPIO 引脚。
 */


// =====================================================
// 通用库
// =====================================================
// 说明：仅保留本节点实际需要的库。不要把另一块子节点的传感器库混入本文件。
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#include <DHT.h>

// =====================================================
// 1. 固件与节点身份
// =====================================================
// 可改范围：节点名称、默认网关、默认 WiFi 可按现场调整。
// 禁止更改：node_id、端点 ID、上报字段名，除非同步修改中控协议。
#define LINGXI_PRODUCT_NAME             "灵汐全屋智能控制中心"
#define LINGXI_PUBLIC_VERSION           "感知层子节点A V1.0.3 / 兼容中控 V1.4.2"
#define LINGXI_NODE_ID                  "SensorNode-EnvWindow-01"
#define LINGXI_NODE_NAME                "感知层子节点A-环境窗户"
#define LINGXI_NODE_TYPE                "sensor_board"
#define LINGXI_NODE_ROLE                "sensor"
#define LINGXI_NODE_FIRMWARE            "Lingxi_SensorNode_C3_EnvWindow_V1_0_3_StatusLightPatch_ForHubV1_4_2"

#define LINGXI_DEFAULT_GATEWAY_HOST     "192.168.4.1"
#define LINGXI_GATEWAY_PORT             80
#define LINGXI_DEFAULT_WIFI_SSID        "网关/路由器WIFI的名称"
#define LINGXI_DEFAULT_WIFI_PASSWORD    "网关/路由器WIFI的密码"

// 注意：用户指定全称“灵汐-感知层子节点A环境窗户”超过 32 字节 SSID 上限。
// 实际热点名使用短中文名，保证 ESP32 SoftAP 可稳定启动。
#define LINGXI_REQUESTED_AP_SSID        "灵汐-感知层子节点A环境窗户"
#define LINGXI_CONFIG_AP_SSID           "灵汐-感知A环境窗户"
#define LINGXI_CONFIG_AP_PASSWORD       "999999999"
#define LINGXI_DNS_PORT                 53

// =====================================================
// 2. 本节点硬件引脚
// =====================================================
// 可改范围：只有实际接线改变时才允许修改。
// 禁止更改：不要加入另一块子节点的传感器引脚。
#define DHT_PIN                         4
#define DHT_TYPE                        DHT22
#define GAS_ADC_PIN                     1      // MQ5 燃气，沿用原集成板 GPIO1
#define LIVING_PIR_PIN                  5      // 客厅人体，沿用原集成板 GPIO5
#define WINDOW_CONTACT_PIN              3      // 窗户门磁，沿用原集成板 GPIO3
#define RGB_PIN                         8
#define RGB_NUM                         1

// =====================================================
// 3. 电平语义与采集参数
// =====================================================
// 说明：人体传感器为 HC-SR505 / PIR 类运动触发传感器，检测的是人体移动，不是静止存在。
#define PIR_PRESENT_LEVEL               HIGH
#define CONTACT_OPEN_LEVEL              HIGH

#define GAS_ALARM_THRESHOLD             2000
#define MQ_WARMUP_MS                    30000UL
#define PIR_HOLD_MS                     2500UL

#define DIGITAL_READ_INTERVAL_MS        50UL
#define MQ_READ_INTERVAL_MS             300UL
#define DHT_READ_INTERVAL_MS            2000UL
#define DEBOUNCE_MS                     50UL

#define WIFI_CONNECT_TIMEOUT_MS         12000UL
#define WIFI_RETRY_INTERVAL_MS          10000UL
#define HTTP_REGISTER_INTERVAL_MS       15000UL
#define HTTP_REPORT_INTERVAL_MS         2000UL
#define HTTP_TIMEOUT_MS                 2500UL
#define CONFIG_AP_CLOSE_DELAY_MS        30000UL
#define REGISTER_FAIL_OPEN_AP_COUNT      3

// =====================================================
// 4. 运行态数据结构
// =====================================================
// 说明：只保存本节点真实负责的端点数据；不要扩展为全量 9 端点结构。
struct DebouncedInput {
  int raw;
  int stable;
  unsigned long changedAt;
};

struct EnvWindowData {
  float temperature;
  float humidity;
  bool temperatureValid;
  bool humidityValid;

  int gasValue;
  int gasAlarm;

  int windowContact;
  int livingMotion;
};

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_NeoPixel rgb(RGB_NUM, RGB_PIN, NEO_GRB + NEO_KHZ800);

Preferences nodePrefs;
WebServer configServer(80);
DNSServer dnsServer;

String gatewayHost = LINGXI_DEFAULT_GATEWAY_HOST;
String wifiSsid = LINGXI_DEFAULT_WIFI_SSID;
String wifiPassword = LINGXI_DEFAULT_WIFI_PASSWORD;
String configApSsid = "";
String localMac = "";

bool hasSavedWifiConfig = false;
bool configPortalRunning = false;
bool configRoutesReady = false;
bool wifiHttpOk = false;
bool httpRegisterOk = false;
bool recentHttpOk = false;
uint8_t registerFailCount = 0;

unsigned long wifiConnectedSinceMs = 0;
unsigned long lastWifiRetryMs = 0;
unsigned long lastHttpRegisterMs = 0;
unsigned long lastHttpReportMs = 0;
unsigned long lastHttpSuccessMs = 0;
unsigned long lastDigitalReadMs = 0;
unsigned long lastMqReadMs = 0;
unsigned long lastDhtReadMs = 0;
unsigned long httpReportSeq = 1;
unsigned long lastLivingMotionMs = 0;

bool immediateReportRequested = false;
int lastWindowContact = -1;
int lastLivingMotion = -1;
int lastGasAlarm = -1;

DebouncedInput livingPirInput = {LOW, LOW, 0};
DebouncedInput windowInput = {LOW, LOW, 0};

EnvWindowData data = {
  -1.0f, -1.0f, false, false,
  0, 0,
  0, 0
};

// =====================================================
// 5. 字符串与显示工具函数
// =====================================================

// 函数说明：转义 JSON 字符串，避免中文配置或特殊字符破坏上报 JSON。
// 可改范围：一般不需要修改；如果上报字段格式变化再调整。
String jsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "\\r");
  s.replace("\t", "\\t");
  return s;
}

// 函数说明：转义 HTML 字符串，避免配置页面显示异常。
// 可改范围：一般不需要修改。
String htmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

// 函数说明：设置板载 RGB 状态灯颜色。
// 可改范围：只建议调整颜色值，不建议改变调用逻辑。
void setRgbColor(uint8_t r, uint8_t g, uint8_t b) {
  rgb.setPixelColor(0, rgb.Color(r, g, b));
  rgb.show();
}

// 函数说明：生成呼吸灯亮度。
// 可改范围：可调整周期，但不影响核心协议。
uint8_t breathBrightness(unsigned long now) {
  unsigned long phase = now % 2000UL;
  if (phase > 1000UL) phase = 2000UL - phase;
  return 20 + (uint8_t)(phase * 80 / 1000UL);
}

// 函数说明：根据网络状态刷新 RGB 指示灯。
// 可改范围：可调整颜色；禁止在此加入传感器采集或网络上报逻辑。
void updateStatusLight() {
  static unsigned long lastRgbUpdateMs = 0;
  unsigned long now = millis();
  if (now - lastRgbUpdateMs < 30) return;
  lastRgbUpdateMs = now;

  uint8_t b = breathBrightness(now);

  // 状态 1：正在配网。
  // 表现：蓝绿色深呼吸。
  // 含义：节点正在开启配网热点，等待用户连接热点并填写 WiFi / 中控地址。
  if (configPortalRunning) {
    setRgbColor(0, b, b);
    return;
  }

  // 状态 2：WiFi 未连接。
  // 表现：红色常亮。
  // 含义：节点尚未连上路由器 WiFi，或 WiFi 已断开。
  if (WiFi.status() != WL_CONNECTED) {
    setRgbColor(120, 0, 0);
    return;
  }

  // 状态 3：WiFi 已连接，且最近 HTTP 注册 / 上报成功。
  // 表现：绿色深呼吸。
  // 含义：节点到中控的通信链路正常，这是完整在线状态。
  if (recentHttpOk && millis() - lastHttpSuccessMs < 8000UL) {
    setRgbColor(0, b, 0);
    return;
  }

  // 状态 4：WiFi 已连接，但最近没有中控 HTTP 成功记录。
  // 表现：蓝色深呼吸。
  // 含义：路由器 WiFi 已连接，但中控注册 / 上报暂未确认成功。
  setRgbColor(0, 0, b);
}

// =====================================================
// 6. 传感器采集函数
// =====================================================

// 函数说明：读取 ADC 多次取平均，用于 MQ5 燃气模拟量。
// 可改范围：samples 可按噪声情况调整；禁止用于不存在的传感器。
int readAverageAdc(int pin, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delayMicroseconds(500);
  }
  return (int)(sum / samples);
}

// 函数说明：通用数字输入去抖，门磁和 PIR 原始输入共用。
// 可改范围：DEBOUNCE_MS 可调；禁止改变返回 1/0 的含义。
int updateDebouncedInput(int pin, int activeLevel, DebouncedInput &input) {
  int nowRaw = digitalRead(pin);
  unsigned long now = millis();

  if (nowRaw != input.raw) {
    input.raw = nowRaw;
    input.changedAt = now;
  }

  if (now - input.changedAt >= DEBOUNCE_MS) {
    input.stable = input.raw;
  }

  return (input.stable == activeLevel) ? 1 : 0;
}

// 函数说明：读取 DHT22 温湿度。
// 可改范围：只允许更换 DHT 类型或异常默认值；字段名不要改。
void readDht22() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) {
    data.temperature = t;
    data.temperatureValid = true;
  } else {
    data.temperatureValid = false;
  }

  if (!isnan(h)) {
    data.humidity = h;
    data.humidityValid = true;
  } else {
    data.humidityValid = false;
  }
}

// 函数说明：读取 MQ5 燃气模拟量并计算报警标志。
// 可改范围：阈值 GAS_ALARM_THRESHOLD 可按实测校准。
void readGasMq5() {
  data.gasValue = readAverageAdc(GAS_ADC_PIN, 5);
  data.gasAlarm = (millis() > MQ_WARMUP_MS && data.gasValue > GAS_ALARM_THRESHOLD) ? 1 : 0;
}

// 函数说明：读取窗户门磁与客厅人体活动。
// 可改范围：PIR_HOLD_MS 可按体验调整；不要改 endpoint 含义。
void readDigitalSensors() {
  data.windowContact = updateDebouncedInput(
    WINDOW_CONTACT_PIN,
    CONTACT_OPEN_LEVEL,
    windowInput
  );

  int pirNow = updateDebouncedInput(
    LIVING_PIR_PIN,
    PIR_PRESENT_LEVEL,
    livingPirInput
  );

  if (pirNow) {
    lastLivingMotionMs = millis();
  }

  data.livingMotion = (millis() - lastLivingMotionMs <= PIR_HOLD_MS) ? 1 : 0;
}

// 函数说明：判断是否需要立即上报状态变化。
// 可改范围：只允许加入本节点已有端点；不要加入另一块子节点字段。
void updateImmediateReportFlag() {
  bool changed = false;

  if (data.windowContact != lastWindowContact) {
    lastWindowContact = data.windowContact;
    changed = true;
  }

  if (data.livingMotion != lastLivingMotion) {
    lastLivingMotion = data.livingMotion;
    changed = true;
  }

  if (data.gasAlarm != lastGasAlarm) {
    lastGasAlarm = data.gasAlarm;
    changed = true;
  }

  if (changed) immediateReportRequested = true;
}

// =====================================================
// 7. 本地配置与配网页
// =====================================================

// 函数说明：读取节点本地配置。
// 可改范围：Preferences 命名空间不要改，避免旧配置丢失。
void loadNodeConfig() {
  nodePrefs.begin("lx_node", true);
  wifiSsid = nodePrefs.getString("ssid", "");
  wifiPassword = nodePrefs.getString("pass", "");
  gatewayHost = nodePrefs.getString("host", LINGXI_DEFAULT_GATEWAY_HOST);
  nodePrefs.end();

  hasSavedWifiConfig = (wifiSsid.length() > 0);
  if (!hasSavedWifiConfig) {
    wifiSsid = LINGXI_DEFAULT_WIFI_SSID;
    wifiPassword = LINGXI_DEFAULT_WIFI_PASSWORD;
    gatewayHost = LINGXI_DEFAULT_GATEWAY_HOST;
  }

  Serial.print("[Config] saved_config=");
  Serial.println(hasSavedWifiConfig ? "yes" : "no");
  Serial.print("[Config] ssid=");
  Serial.println(wifiSsid);
  Serial.print("[Config] gateway=");
  Serial.println(gatewayHost);
}

// 函数说明：输出本地配网页。
// 可改范围：只改文案与样式；表单字段 ssid/pass/host 不要改。
// 禁止更改：当前版本不提供“清除配置”按钮，避免封装后误操作。
void sendConfigPage() {
  String html;
  html.reserve(7600);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
  html += "<title>灵汐感知层子节点A - 环境窗户配网</title>";
  html += "<style>";
  html += "*{box-sizing:border-box}body{margin:0;min-height:100vh;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','Microsoft YaHei',sans-serif;color:#102a43;background:linear-gradient(155deg,#eef8ff,#f7fbff 48%,#dceeff);padding:18px}.shell{max-width:640px;margin:0 auto}.card{position:relative;overflow:hidden;background:rgba(255,255,255,.95);border:1px solid rgba(72,160,220,.18);box-shadow:0 20px 60px rgba(45,121,196,.18);border-radius:28px;padding:24px}.tag{display:inline-flex;padding:6px 12px;border-radius:999px;background:#e8f5ff;color:#1676c8;font-size:12px;font-weight:900}h1{margin:14px 0 6px;font-size:25px;line-height:1.25;color:#102a43}h2{margin:0 0 10px;font-size:18px;color:#2b7fc3}.sub{margin:0;color:#60758d;line-height:1.8;font-size:14px}.panel{margin-top:18px;background:#f7fbff;border:1px solid #d7ecff;border-radius:20px;padding:15px}.kv{display:grid;grid-template-columns:92px 1fr;gap:8px 10px;font-size:13px;line-height:1.65}.kv b{color:#2b7fc3}.field{margin-top:14px}label{display:block;margin-bottom:8px;font-weight:900}input{width:100%;border:1px solid #c9e5fa;background:white;border-radius:15px;padding:13px 14px;font-size:16px;outline:none}input:focus{border-color:#4eb7ff;box-shadow:0 0 0 4px rgba(78,183,255,.14)}.hint{font-size:12px;color:#6e8298;margin-top:6px;line-height:1.6}button{width:100%;border:0;border-radius:16px;padding:14px 16px;margin-top:18px;font-size:16px;font-weight:900;color:white;background:linear-gradient(135deg,#4eb7ff,#1b77d2)}.tip{margin-top:16px;border-radius:18px;padding:13px 14px;background:#eef8ff;border:1px dashed #b8def8;color:#4d6a86;font-size:12px;line-height:1.75}.foot{font-size:12px;color:#71879e;text-align:center;margin-top:14px;line-height:1.7}";
  html += "</style></head><body><div class='shell'><div class='card'>";
  html += "<div class='tag'>" + htmlEscape(String(LINGXI_NODE_ID)) + " · " + htmlEscape(String(LINGXI_PUBLIC_VERSION)) + "</div>";
  html += "<h1>灵汐感知层子节点A</h1><h2>环境窗户配网</h2>";
  html += "<p class='sub'>负责 DHT22 温湿度、MQ5 燃气、窗户门磁与客厅人体活动触发。保存配置后节点会自动重启并连接中控。</p>";
  html += "<div class='panel'><div class='kv'>";
  html += "<b>节点名称</b><span>" + htmlEscape(String(LINGXI_NODE_NAME)) + "</span>";
  html += "<b>节点类型</b><span>感知节点 / sensor_board</span>";
  html += "<b>MAC</b><span>" + htmlEscape(WiFi.macAddress()) + "</span>";
  html += "<b>热点名</b><span>" + htmlEscape(String(LINGXI_CONFIG_AP_SSID)) + "</span>";
  html += "<b>热点密码</b><span>" + htmlEscape(String(LINGXI_CONFIG_AP_PASSWORD)) + "</span>";
  html += "<b>中控地址</b><span>" + htmlEscape(gatewayHost) + "</span>";
  html += "</div></div>";
  html += "<form method='POST' action='/save'>";
  html += "<div class='field'><label>WiFi 名称</label><input name='ssid' value='" + htmlEscape(hasSavedWifiConfig ? wifiSsid : String("")) + "' required><div class='hint'>填写路由器 WiFi 或中控热点名称。</div></div>";
  html += "<div class='field'><label>WiFi 密码</label><input name='pass' type='password' value='" + htmlEscape(hasSavedWifiConfig ? wifiPassword : String("")) + "'></div>";
  html += "<div class='field'><label>中控地址</label><input name='host' value='" + htmlEscape(gatewayHost) + "' placeholder='192.168.4.1' required><div class='hint'>中控热点模式通常为 192.168.4.1；路由器模式填写中控实际 IP。</div></div>";
  html += "<button type='submit'>保存并连接</button></form>";
  html += "<div class='tip'>无配置时会自动开启配网热点；WiFi 连接失败、运行中 WiFi 断开，或连续注册中控失败时，节点会自动重新开启配网热点。注册成功并稳定约 30 秒后，热点会自动关闭。</div>";
  html += "<div class='foot'>当前页面不提供清除配置按钮；如需强制从头配网，可擦除 Flash 后重烧同一固件。</div>";
  html += "</div></div></body></html>";
  configServer.send(200, "text/html; charset=utf-8", html);
}

// 函数说明：保存配网页提交的 WiFi 和中控地址。
// 可改范围：返回页面文案可改；配置字段不要改。
void saveNodeConfig() {
  String ssid = configServer.arg("ssid");
  String pass = configServer.arg("pass");
  String host = configServer.arg("host");
  ssid.trim();
  host.trim();

  if (ssid.length() == 0 || host.length() == 0) {
    configServer.send(400, "text/plain; charset=utf-8", "SSID 和中控 IP 不能为空");
    return;
  }

  nodePrefs.begin("lx_node", false);
  nodePrefs.putString("ssid", ssid);
  nodePrefs.putString("pass", pass);
  nodePrefs.putString("host", host);
  nodePrefs.end();

  configServer.send(200, "text/html; charset=utf-8", "<meta charset='utf-8'><h2>配置已保存，节点正在重启并连接...</h2>");
  delay(800);
  ESP.restart();
}

// 函数说明：返回配网状态 JSON。
// 可改范围：可增加字段；已有字段名不要改。
void sendConfigStatus() {
  String json = "{";
  json += "\"node_id\":\"" + jsonEscape(String(LINGXI_NODE_ID)) + "\",";
  json += "\"mac\":\"" + jsonEscape(WiFi.macAddress()) + "\",";
  json += "\"ap_ssid\":\"" + jsonEscape(configApSsid) + "\",";
  json += "\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"gateway_host\":\"" + jsonEscape(gatewayHost) + "\"";
  json += "}";
  configServer.send(200, "application/json; charset=utf-8", json);
}

// 函数说明：启动本地配网热点与网页服务。
// 可改范围：AP IP 可改；热点名和密码按当前项目约定保持中文与 9 个 9。
void startConfigPortal(const char* reason) {
  if (configPortalRunning) return;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 8, 1), IPAddress(192, 168, 8, 1), IPAddress(255, 255, 255, 0));
  configApSsid = LINGXI_CONFIG_AP_SSID;
  bool apOk = WiFi.softAP(configApSsid.c_str(), LINGXI_CONFIG_AP_PASSWORD);

  if (!configRoutesReady) {
    configServer.on("/", HTTP_GET, sendConfigPage);
    configServer.on("/generate_204", HTTP_GET, sendConfigPage);
    configServer.on("/fwlink", HTTP_GET, sendConfigPage);
    configServer.on("/hotspot-detect.html", HTTP_GET, sendConfigPage);
    configServer.on("/save", HTTP_POST, saveNodeConfig);
    configServer.on("/save", HTTP_GET, sendConfigPage);
    configServer.on("/status", HTTP_GET, sendConfigStatus);
    configServer.onNotFound(sendConfigPage);
    configRoutesReady = true;
  }

  dnsServer.start(LINGXI_DNS_PORT, "*", IPAddress(192, 168, 8, 1));
  configServer.begin();
  configPortalRunning = apOk;
  wifiConnectedSinceMs = 0;

  Serial.print("[ConfigPortal] start reason=");
  Serial.println(reason);
  Serial.print("[ConfigPortal] SSID=");
  Serial.println(configApSsid);
  Serial.print("[ConfigPortal] PASS=");
  Serial.println(LINGXI_CONFIG_AP_PASSWORD);
  Serial.print("[ConfigPortal] IP=");
  Serial.println(WiFi.softAPIP());
  if (!apOk) Serial.println("[ConfigPortal] softAP start failed");
}

// 函数说明：处理配网页请求与 DNS 劫持。
// 可改范围：不要在此加入长耗时采集。
void configPortalTick() {
  if (!configPortalRunning) return;
  dnsServer.processNextRequest();
  configServer.handleClient();
}

// 函数说明：WiFi 连接且中控注册成功后，延迟关闭配网热点。
// 可改范围：CONFIG_AP_CLOSE_DELAY_MS 可调整；不要改成一连上 WiFi 就立即关热点。
// 禁止更改：只有注册成功后才关闭热点，避免网关地址填错却失去配网入口。
void maybeCloseConfigPortalAfterWifiOk() {
  if (!configPortalRunning) return;

  if (WiFi.status() != WL_CONNECTED || !httpRegisterOk) {
    wifiConnectedSinceMs = 0;
    return;
  }

  if (wifiConnectedSinceMs == 0) {
    wifiConnectedSinceMs = millis();
    return;
  }

  if (millis() - wifiConnectedSinceMs < CONFIG_AP_CLOSE_DELAY_MS) return;

  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  configPortalRunning = false;
  configApSsid = "";
  Serial.println("[ConfigPortal] registered, AP closed");
}

// =====================================================
// 8. 中控 HTTP 注册与上报
// =====================================================

// 函数说明：返回本节点端点声明 JSON。
// 可改范围：禁止修改 id；name/room 可按文档优化。
String endpointsJson() {
  String json = "[";
  json += "{\"id\":\"temperature\",\"name\":\"室内温度\",\"type\":\"sensor\",\"capability\":\"temperature\",\"room\":\"环境窗户\",\"unit\":\"C\"},";
  json += "{\"id\":\"humidity\",\"name\":\"室内湿度\",\"type\":\"sensor\",\"capability\":\"humidity\",\"room\":\"环境窗户\",\"unit\":\"%\"},";
  json += "{\"id\":\"gas\",\"name\":\"燃气传感器\",\"type\":\"sensor\",\"capability\":\"gas\",\"room\":\"环境窗户\"},";
  json += "{\"id\":\"window_contact\",\"name\":\"窗户门磁\",\"type\":\"sensor\",\"capability\":\"contact\",\"room\":\"环境窗户\"},";
  json += "{\"id\":\"human_living\",\"name\":\"客厅人体\",\"type\":\"sensor\",\"capability\":\"presence\",\"room\":\"环境窗户\"}";
  json += "]";
  return json;
}

// 函数说明：拼接中控接口 URL。
// 可改范围：接口路径由调用者传入；端口常量可在配置区改。
String gatewayUrl(const char* path) {
  return String("http://") + gatewayHost + ":" + String(LINGXI_GATEWAY_PORT) + String(path);
}

// 函数说明：构建节点注册 JSON。
// 可改范围：禁止删除 endpoints、node_id、role 等关键字段。
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

// 函数说明：构建本节点数据上报 JSON。
// 可改范围：只允许新增本节点字段；不要加入另一块子节点字段。
String buildReportJson() {
  String json = "{";
  json += "\"node_id\":\"" + jsonEscape(String(LINGXI_NODE_ID)) + "\",";
  json += "\"name\":\"" + jsonEscape(String(LINGXI_NODE_NAME)) + "\",";
  json += "\"node_type\":\"" + jsonEscape(String(LINGXI_NODE_TYPE)) + "\",";
  json += "\"role\":\"" + jsonEscape(String(LINGXI_NODE_ROLE)) + "\",";
  json += "\"transport\":\"wifi_http\",";
  json += "\"mac\":\"" + jsonEscape(localMac) + "\",";
  json += "\"status\":\"online\",";
  json += "\"seq\":" + String(httpReportSeq++) + ",";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"temperature\":" + String(data.temperature, 1) + ",";
  json += "\"humidity\":" + String(data.humidity, 1) + ",";
  json += "\"temperature_valid\":" + String(data.temperatureValid ? "true" : "false") + ",";
  json += "\"humidity_valid\":" + String(data.humidityValid ? "true" : "false") + ",";
  json += "\"gas\":" + String(data.gasValue) + ",";
  json += "\"gas_alarm\":" + String(data.gasAlarm) + ",";
  json += "\"window_contact\":" + String(data.windowContact) + ",";
  json += "\"human_living\":" + String(data.livingMotion) + ",";
  json += "\"pir_mode\":\"motion_trigger\"";
  json += "}";
  return json;
}

// 函数说明：向中控 POST JSON。
// 可改范围：HTTP_TIMEOUT_MS 可调；接口路径不要随意改。
bool postJsonToGateway(const char* path, const String &payload) {
  if (WiFi.status() != WL_CONNECTED) {
    wifiHttpOk = false;
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  String url = gatewayUrl(path);

  http.setTimeout(HTTP_TIMEOUT_MS);
  if (!http.begin(client, url)) {
    Serial.println("[HTTP] begin failed: " + url);
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  String response = http.getString();
  http.end();

  bool ok = (code >= 200 && code < 300);
  Serial.print("[HTTP] POST ");
  Serial.print(path);
  Serial.print(" code=");
  Serial.print(code);
  Serial.print(" ok=");
  Serial.println(ok ? "yes" : "no");

  recentHttpOk = ok;
  if (ok) lastHttpSuccessMs = millis();
  return ok;
}

// 函数说明：发送节点注册。
// 可改范围：禁止修改注册路径，除非中控接口同步变化。
bool sendHttpRegister() {
  bool ok = postJsonToGateway("/api/node/register", buildRegisterJson());
  httpRegisterOk = ok;
  if (ok) registerFailCount = 0;
  else registerFailCount++;
  return ok;
}

// 函数说明：发送节点数据。
// 可改范围：禁止修改上报路径，除非中控接口同步变化。
bool sendHttpReport() {
  return postJsonToGateway("/api/node/report", buildReportJson());
}

// 函数说明：初始化 WiFi 与 HTTP 主链路。
// 可改范围：连接超时时间可调；不要删除无配置时开启热点的逻辑。
// 禁止更改：WebServer 必须在 WiFi/AP 初始化之后启动，避免 ESP32 首次无配置时崩溃重启。
void initWifiHttp() {
  Serial.println("[HTTP] WiFi init...");

  if (!hasSavedWifiConfig) {
    startConfigPortal("first_boot_no_saved_config");
    localMac = WiFi.macAddress();
    return;
  }

  WiFi.mode(configPortalRunning ? WIFI_AP_STA : WIFI_STA);
  WiFi.setSleep(false);
  localMac = WiFi.macAddress();

  Serial.print("[HTTP] WiFi connect ssid=");
  Serial.println(wifiSsid);
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
    Serial.print("[HTTP] WiFi connected. IP=");
    Serial.println(WiFi.localIP());
    sendHttpRegister();
  } else {
    wifiHttpOk = false;
    Serial.println("[HTTP] WiFi connect failed, open config portal");
    startConfigPortal("wifi_connect_failed");
  }
}

// 函数说明：维护 WiFi 重连、注册、上报与热点关闭。
// 可改范围：周期参数可调；不要在此加入传感器硬件操作。
void updateWifiHttp() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    wifiHttpOk = false;
    httpRegisterOk = false;
    wifiConnectedSinceMs = 0;

    if (!configPortalRunning) {
      startConfigPortal("wifi_disconnected_reopen_portal");
    }

    if (hasSavedWifiConfig && now - lastWifiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
      lastWifiRetryMs = now;
      Serial.println("[HTTP] WiFi reconnect...");
      WiFi.disconnect(false, false);
      delay(50);
      WiFi.mode(configPortalRunning ? WIFI_AP_STA : WIFI_STA);
      WiFi.setSleep(false);
      WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
    }
    return;
  }

  wifiHttpOk = true;

  if (!httpRegisterOk || now - lastHttpRegisterMs >= HTTP_REGISTER_INTERVAL_MS) {
    lastHttpRegisterMs = now;
    sendHttpRegister();
    if (!httpRegisterOk && registerFailCount >= REGISTER_FAIL_OPEN_AP_COUNT && !configPortalRunning) {
      startConfigPortal("gateway_register_failed");
    }
  }

  if (immediateReportRequested || now - lastHttpReportMs >= HTTP_REPORT_INTERVAL_MS) {
    lastHttpReportMs = now;
    immediateReportRequested = false;
    sendHttpReport();
  }

  maybeCloseConfigPortalAfterWifiOk();
}

// =====================================================
// 9. Arduino 主流程
// =====================================================

// 函数说明：系统启动入口。
// 可改范围：只允许调整本节点传感器初始化；不要加入另一块子节点硬件。
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("灵汐全屋智能控制中心 - 感知层子节点A环境窗户");
  Serial.println("兼容中控：公测版 V1.4.2");
  Serial.println("========================================");

  loadNodeConfig();

  rgb.begin();
  rgb.clear();
  rgb.show();
  rgb.setBrightness(24);

  analogReadResolution(12);
  analogSetPinAttenuation(GAS_ADC_PIN, ADC_11db);

  pinMode(LIVING_PIR_PIN, INPUT);
  pinMode(WINDOW_CONTACT_PIN, INPUT_PULLUP);

  dht.begin();

  readDigitalSensors();
  readGasMq5();
  readDht22();

  initWifiHttp();

  Serial.println("[System] Config portal logic: BootHotspotFix V1.0.2");
  Serial.print("[System] running node=");
  Serial.println(LINGXI_NODE_ID);
}

// 函数说明：系统循环入口。
// 可改范围：只允许调整本节点采集周期；不要加入阻塞式长延时。
void loop() {
  unsigned long now = millis();

  if (now - lastDigitalReadMs >= DIGITAL_READ_INTERVAL_MS) {
    lastDigitalReadMs = now;
    readDigitalSensors();
    updateImmediateReportFlag();
  }

  if (now - lastMqReadMs >= MQ_READ_INTERVAL_MS) {
    lastMqReadMs = now;
    readGasMq5();
    updateImmediateReportFlag();
  }

  if (now - lastDhtReadMs >= DHT_READ_INTERVAL_MS) {
    lastDhtReadMs = now;
    readDht22();
  }

  updateWifiHttp();
  configPortalTick();
  updateStatusLight();
}
