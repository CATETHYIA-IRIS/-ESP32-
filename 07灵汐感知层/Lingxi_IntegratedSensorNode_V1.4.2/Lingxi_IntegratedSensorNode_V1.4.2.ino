/*
 * 灵汐全屋智能控制中心 - 感知层集成节点
 * 节点封装版本：V1.0.0
 * 兼容中控版本：公测版 V1.4.2
 * 节点编号：SensorBoard-01
 *
 * 本固件用于一块 ESP32-C3 集成承载 9 个感知端点：
 * temperature、humidity、light_level、door_main_contact、window_contact、
 * human_living、human_bedroom、smoke、gas。
 *
 * 设计原则：
 * 1. 中控 V1.4.2 已冻结，本节点只按既有协议注册和上报，不要求中控改代码。
 * 2. 感知层只负责采集、去抖、状态封装、节点注册、心跳/数据上报与配网。
 * 3. 场景判断、报警模式、规则联动、页面显示与设备控制全部由中控负责。
 * 4. 人体传感器按 HC-SR505 / PIR 运动触发理解：检测人体移动，不等同于毫米波“存在检测”。
 * 5. 当前为 1×9 集成节点版本；未来拆分成 1×1、1×2、1×8 等子节点时，端点 ID 保持一致。
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>
#include <esp_arduino_version.h>

// =====================================================
// 1. 固件与节点身份
// =====================================================

#define LINGXI_PRODUCT_NAME             "灵汐全屋智能控制中心"
#define LINGXI_PUBLIC_VERSION           "感知节点 V1.0.0 / 兼容中控 V1.4.2"
#define LINGXI_NODE_ID                  "SensorBoard-01"
#define LINGXI_NODE_NAME                "感知层集成节点"
#define LINGXI_NODE_TYPE                "sensor_board"
#define LINGXI_NODE_ROLE                "sensor"
#define LINGXI_NODE_FIRMWARE            "Lingxi_IntegratedSensorNode_V1_0_0_ForHubV1_4_2"

// 中控默认连接信息。若中控热点名称已修改，可通过节点配网页重新配置。
#define LINGXI_DEFAULT_GATEWAY_HOST     "192.168.4.1"   // 默认按中控热点模式；若走家庭路由器，可在配网页改成中控局域网 IP
#define LINGXI_GATEWAY_PORT             80
#define LINGXI_DEFAULT_WIFI_SSID        "网关/路由器WIFI的名称"
#define LINGXI_DEFAULT_WIFI_PASSWORD    "网关/路由器WIFI的密码"

// 感知层节点本地配网页热点。
#define LINGXI_CONFIG_AP_SSID           "灵汐-感知层集成节点"
#define LINGXI_CONFIG_AP_PASSWORD       "999999999"
#define LINGXI_DNS_PORT                 53

// ESP-NOW 兼容链路：中控网关 MAC。
uint8_t gatewayMac[] = {0x20, 0x6E, 0xF1, 0xA6, 0x9D, 0x3C};

// =====================================================
// 2. 硬件引脚
// =====================================================

#define DHT_PIN                         4
#define DHT_TYPE                        DHT22

#define BH1750_SDA                      6
#define BH1750_SCL                      7
#define BH1750_I2C_ADDRESS              0x23

#define SMOKE_ADC_PIN                   0      // MQ-2 烟雾
#define GAS_ADC_PIN                     1      // MQ-5 燃气

#define LIVING_PIR_PIN                  5      // 客厅人体 HC-SR505 / PIR 人体运动感应
#define BEDROOM_PIR_PIN                 10     // 卧室人体 HC-SR505 / PIR 人体运动感应

#define MAIN_DOOR_CONTACT_PIN           2      // 大门门磁干簧管模块 DO
#define WINDOW_CONTACT_PIN              3      // 窗户门磁干簧管模块 DO

#define RGB_PIN                         8
#define RGB_NUM                         1

// =====================================================
// 3. 传感器电平语义
// =====================================================

// HC-SR505 / 常见 PIR：检测到人体移动时 OUT 输出高电平，未触发时为低电平。
// 注意：PIR 是“运动触发”，不是“静止存在检测”；人静止不动时可能恢复无人。
#define PIR_PRESENT_LEVEL               HIGH

// PIR 输入模式：使用内部下拉可避免某一路传感器未接时输入悬空乱跳。
// 若实际模块输出受影响，可改为 INPUT。
#define PIR_INPUT_MODE                  INPUT_PULLDOWN

// PIR 软件保持：用于过滤 HC-SR505 短暂抖动，避免刚触发后马上变无人。
// 该保持不宜过长，避免中控长时间误显示有人。
#define PIR_SOFTWARE_HOLD_MS            2000UL

// 干簧管门磁模块：磁铁靠近时 DO 输出低电平，表示门/窗吸合关闭；
// 无磁铁时 DO 输出高电平，表示门/窗打开。
#define CONTACT_OPEN_LEVEL              HIGH

// =====================================================
// 4. 采集与上报参数
// =====================================================

#define SMOKE_ALARM_THRESHOLD           2000
#define GAS_ALARM_THRESHOLD             2000
#define MQ_WARMUP_MS                    30000UL

#define ESPNOW_SEND_INTERVAL_MS         500UL
#define ESPNOW_SEND_MIN_INTERVAL_MS     80UL
#define DIGITAL_READ_INTERVAL_MS        50UL
#define MQ_READ_INTERVAL_MS             300UL
#define LIGHT_READ_INTERVAL_MS          500UL
#define DHT_READ_INTERVAL_MS            2000UL
#define DEBOUNCE_MS                     50UL

#define WIFI_CONNECT_TIMEOUT_MS         12000UL
#define WIFI_RETRY_INTERVAL_MS          10000UL
#define HTTP_REGISTER_INTERVAL_MS       15000UL
#define HTTP_REPORT_INTERVAL_MS         2000UL
#define HTTP_TIMEOUT_MS                 2500UL

// =====================================================
// 5. ESP-NOW 数据包结构
// 必须与中控 SensorData 保持字段顺序一致。
// =====================================================

typedef struct {
  float temperature;
  float humidity;
  float lightLevelValue;

  int smokeValue;
  int gasValue;

  int livingPirState;
  int bedroomPirState;

  int mainDoorContact;
  int windowContact;

  uint32_t timestampMs;
  uint32_t packetSeq;
} SensorData;

SensorData sensorPacket;

// =====================================================
// 6. 设备对象与运行状态
// =====================================================

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
bool wifiHttpOk = false;
bool httpRegisterOk = false;
bool recentHttpOk = false;
bool espNowOk = false;
volatile bool recentEspNowOk = false;

unsigned long lastWifiRetryMs = 0;
unsigned long lastHttpRegisterMs = 0;
unsigned long lastHttpReportMs = 0;
unsigned long lastHttpSuccessMs = 0;
unsigned long lastEspNowSendMs = 0;
unsigned long lastEspNowSuccessMs = 0;
unsigned long lastDigitalReadMs = 0;
unsigned long lastMqReadMs = 0;
unsigned long lastLightReadMs = 0;
unsigned long lastDhtReadMs = 0;

uint32_t espNowPacketSeq = 0;
uint32_t httpReportSeq = 0;
bool immediateReportRequested = false;

bool bh1750Ready = false;
float lastValidLux = 0.0f;

// 上一次稳定状态，用于状态变化立即上报。
int lastLivingPir = -1;
int lastBedroomPir = -1;
int lastMainDoorContact = -1;
int lastWindowContact = -1;
int lastSmokeAlarm = -1;
int lastGasAlarm = -1;

struct DebouncedInput {
  int raw;
  int stable;
  unsigned long changedAt;
};

DebouncedInput livingPirInput = {LOW, LOW, 0};
DebouncedInput bedroomPirInput = {LOW, LOW, 0};
DebouncedInput mainDoorInput = {LOW, LOW, 0};
DebouncedInput windowInput = {LOW, LOW, 0};

// PIR 最近一次有效触发时间。用于 HC-SR505 运动传感器的软件短保持。
unsigned long lastLivingMotionMs = 0;
unsigned long lastBedroomMotionMs = 0;

// =====================================================
// 7. 基础工具
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

void setRgbColor(uint8_t r, uint8_t g, uint8_t b) {
  rgb.setPixelColor(0, rgb.Color(r, g, b));
  rgb.show();
}

uint8_t breathBrightness(unsigned long now) {
  const unsigned long cycleMs = 4200UL;
  unsigned long phase = now % cycleMs;
  if (phase < cycleMs / 2) {
    return map(phase, 0, cycleMs / 2, 0, 128);
  }
  return map(phase, cycleMs / 2, cycleMs, 128, 0);
}

void updateStatusLight() {
  static unsigned long lastRgbUpdateMs = 0;
  unsigned long now = millis();
  if (now - lastRgbUpdateMs < 30) return;
  lastRgbUpdateMs = now;

  uint8_t b = breathBrightness(now);

  // 蓝色呼吸：配网模式。
  if (configPortalRunning) {
    setRgbColor(0, 60, b);
    return;
  }

  // 红色常亮：WiFi 未连接。
  if (WiFi.status() != WL_CONNECTED) {
    setRgbColor(120, 0, 0);
    return;
  }

  // 蓝白呼吸：WiFi + HTTP 在线。
  uint8_t r = b / 4;
  uint8_t g = (b * 3) / 4;
  setRgbColor(r, g, b);
}

int readAverageAdc(int pin, int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delayMicroseconds(500);
  }
  return sum / samples;
}

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

int updateMotionInput(int pin, DebouncedInput &input, unsigned long &lastMotionMs) {
  int presentNow = updateDebouncedInput(pin, PIR_PRESENT_LEVEL, input);
  unsigned long now = millis();

  if (presentNow) {
    lastMotionMs = now;
    return 1;
  }

  // HC-SR505 可能会出现短暂低电平抖动；这里做很短的软件保持。
  if (lastMotionMs > 0 && now - lastMotionMs <= PIR_SOFTWARE_HOLD_MS) {
    return 1;
  }

  return 0;
}

int lightLuxToPercent(float lux) {
  int percent = (int)(lux / 500.0f * 100.0f);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  return percent;
}

// =====================================================
// 8. BH1750 直接 I2C 驱动
// 不依赖 BH1750.h，减少节点库依赖。
// =====================================================

bool bh1750WriteCommand(uint8_t cmd) {
  Wire.beginTransmission(BH1750_I2C_ADDRESS);
  Wire.write(cmd);
  return (Wire.endTransmission() == 0);
}

bool initLightSensor() {
  Wire.begin(BH1750_SDA, BH1750_SCL);
  delay(10);
  bool ok = true;
  ok = ok && bh1750WriteCommand(0x01);   // Power on
  delay(10);
  ok = ok && bh1750WriteCommand(0x10);   // Continuous high resolution mode
  delay(180);
  return ok;
}

float readLightLux() {
  Wire.requestFrom(BH1750_I2C_ADDRESS, 2);
  if (Wire.available() < 2) {
    return NAN;
  }
  uint16_t raw = ((uint16_t)Wire.read() << 8) | Wire.read();
  return raw / 1.2f;
}

// =====================================================
// 9. 传感器采集
// =====================================================

void readDht22() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t)) sensorPacket.temperature = t;
  if (!isnan(h)) sensorPacket.humidity = h;
}

void readLightSensor() {
  if (!bh1750Ready) {
    bh1750Ready = initLightSensor();
    if (!bh1750Ready) return;
  }

  float lux = readLightLux();
  if (isnan(lux) || lux < 0) {
    bh1750Ready = false;
    return;
  }

  lastValidLux = lux;
  sensorPacket.lightLevelValue = lux;
}

void readSmokeGasSensors() {
  sensorPacket.smokeValue = readAverageAdc(SMOKE_ADC_PIN, 5);
  sensorPacket.gasValue = readAverageAdc(GAS_ADC_PIN, 5);
}

void readDigitalSensors() {
  // HC-SR505 / PIR：高电平表示检测到人体移动。
  // 这里输出给中控的是“最近检测到活动”，不是毫米波静止存在。
  sensorPacket.livingPirState = updateMotionInput(
    LIVING_PIR_PIN,
    livingPirInput,
    lastLivingMotionMs
  );

  sensorPacket.bedroomPirState = updateMotionInput(
    BEDROOM_PIR_PIN,
    bedroomPirInput,
    lastBedroomMotionMs
  );

  // 干簧管门磁：高电平表示磁铁离开，即门/窗打开；低电平表示吸合关闭。
  sensorPacket.mainDoorContact = updateDebouncedInput(
    MAIN_DOOR_CONTACT_PIN,
    CONTACT_OPEN_LEVEL,
    mainDoorInput
  );

  sensorPacket.windowContact = updateDebouncedInput(
    WINDOW_CONTACT_PIN,
    CONTACT_OPEN_LEVEL,
    windowInput
  );
}

void updateImmediateReportFlag() {
  bool changed = false;

  if (sensorPacket.livingPirState != lastLivingPir) {
    lastLivingPir = sensorPacket.livingPirState;
    changed = true;
  }

  if (sensorPacket.bedroomPirState != lastBedroomPir) {
    lastBedroomPir = sensorPacket.bedroomPirState;
    changed = true;
  }

  if (sensorPacket.mainDoorContact != lastMainDoorContact) {
    lastMainDoorContact = sensorPacket.mainDoorContact;
    changed = true;
  }

  if (sensorPacket.windowContact != lastWindowContact) {
    lastWindowContact = sensorPacket.windowContact;
    changed = true;
  }

  if (millis() > MQ_WARMUP_MS) {
    int smokeAlarm = (sensorPacket.smokeValue > SMOKE_ALARM_THRESHOLD) ? 1 : 0;
    int gasAlarm = (sensorPacket.gasValue > GAS_ALARM_THRESHOLD) ? 1 : 0;

    if (smokeAlarm != lastSmokeAlarm) {
      lastSmokeAlarm = smokeAlarm;
      changed = true;
    }

    if (gasAlarm != lastGasAlarm) {
      lastGasAlarm = gasAlarm;
      changed = true;
    }
  }

  if (changed) immediateReportRequested = true;
}

// =====================================================
// 10. ESP-NOW 兼容发送
// =====================================================

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
void onEspNowSent(const wifi_tx_info_t *txInfo, esp_now_send_status_t status) {
  (void)txInfo;
#else
void onEspNowSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  (void)macAddr;
#endif
  if (status == ESP_NOW_SEND_SUCCESS) {
    recentEspNowOk = true;
    lastEspNowSuccessMs = millis();
  } else {
    recentEspNowOk = false;
  }
}

void sendEspNowSensorPacket() {
  if (!espNowOk) return;

  sensorPacket.timestampMs = millis();
  sensorPacket.packetSeq = espNowPacketSeq++;

  esp_err_t result = esp_now_send(
    gatewayMac,
    (uint8_t *)&sensorPacket,
    sizeof(sensorPacket)
  );

  if (result == ESP_OK) {
    Serial.print("[ESP-NOW] send seq=");
    Serial.print(sensorPacket.packetSeq);
    Serial.print(" temp=");
    Serial.print(sensorPacket.temperature, 1);
    Serial.print(" hum=");
    Serial.print(sensorPacket.humidity, 1);
    Serial.print(" lux=");
    Serial.print(sensorPacket.lightLevelValue, 1);
    Serial.print(" door=");
    Serial.print(sensorPacket.mainDoorContact);
    Serial.print(" window=");
    Serial.print(sensorPacket.windowContact);
    Serial.print(" living_pir=");
    Serial.print(sensorPacket.livingPirState);
    Serial.print(" bedroom_pir=");
    Serial.println(sensorPacket.bedroomPirState);
  } else {
    Serial.println("[ESP-NOW] send request failed");
  }
}

// =====================================================
// 11. 配网、注册与 HTTP 上报
// =====================================================

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
  Serial.println(hasSavedWifiConfig ? "yes" : "no, use default");
  Serial.print("[Config] ssid=");
  Serial.println(wifiSsid);
  Serial.print("[Config] gateway=");
  Serial.println(gatewayHost);
}

void sendConfigPage() {
  String html;
  html.reserve(7800);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
  html += "<title>灵汐-感知层集成节点</title>";
  html += "<style>";
  html += "*{box-sizing:border-box}body{margin:0;min-height:100vh;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','Microsoft YaHei',sans-serif;color:#16314f;background:linear-gradient(160deg,#eef8ff 0%,#f7fbff 45%,#dceeff 100%);padding:18px}.shell{max-width:640px;margin:0 auto}.card{position:relative;overflow:hidden;background:rgba(255,255,255,.92);border:1px solid rgba(72,160,220,.18);box-shadow:0 20px 60px rgba(45,121,196,.18);border-radius:28px;padding:24px}.card:before{content:'';position:absolute;right:-70px;top:-90px;width:220px;height:220px;border-radius:50%;background:rgba(59,168,255,.15)}.brand{position:relative}.eyebrow{display:inline-flex;padding:6px 12px;border-radius:999px;background:#e8f5ff;color:#1676c8;font-size:12px;font-weight:800}.brand h1{margin:14px 0 8px;font-size:26px;line-height:1.25;color:#102a43}.sub{margin:0;color:#60758d;line-height:1.8;font-size:14px}.panel{margin-top:18px;background:#f7fbff;border:1px solid #d7ecff;border-radius:20px;padding:15px}.kv{display:grid;grid-template-columns:88px 1fr;gap:8px 10px;font-size:13px;line-height:1.65}.kv b{color:#2b7fc3}.form{margin-top:16px}.field{margin-top:14px}label{display:block;margin-bottom:8px;font-weight:800;color:#16314f}input{width:100%;border:1px solid #c9e5fa;background:white;color:#102a43;border-radius:15px;padding:13px 14px;font-size:16px;outline:none}input:focus{border-color:#53aef0;box-shadow:0 0 0 4px rgba(83,174,240,.16)}.hint{font-size:12px;color:#6e8298;margin-top:6px;line-height:1.6}.btn{width:100%;border:0;border-radius:16px;padding:14px 16px;margin-top:18px;font-size:16px;font-weight:900;color:white;background:linear-gradient(135deg,#4eb7ff,#1b77d2);box-shadow:0 12px 26px rgba(27,119,210,.24)}.danger{background:linear-gradient(135deg,#ff8b8b,#ef4444);box-shadow:0 12px 26px rgba(239,68,68,.18);margin-top:10px}.foot{font-size:12px;color:#71879e;text-align:center;margin-top:14px;line-height:1.7}";
  html += "</style></head><body><div class='shell'><div class='card'>";
  html += "<div class='brand'><div class='eyebrow'>SensorBoard-01 · 感知节点 V1.0.0 / 兼容中控 V1.4.2</div>";
  html += "<h1>灵汐-全屋智能感知层集成节点</h1>";
  html += "<p class='sub'>感知层集成中心：温湿度、光照、门磁、人体、烟雾、燃气传感器。节点按中控标准协议注册和上报，显示兼容由中控统一处理；人体传感器按“运动触发”理解。</p></div>";
  html += "<div class='panel'><div class='kv'>";
  html += "<b>节点</b><span>" + htmlEscape(String(LINGXI_NODE_ID)) + "</span>";
  html += "<b>MAC</b><span>" + htmlEscape(WiFi.macAddress()) + "</span>";
  html += "<b>中控 IP</b><span>" + htmlEscape(gatewayHost) + "</span>";
  html += "<b>固件</b><span>" + htmlEscape(String(LINGXI_NODE_FIRMWARE)) + "</span>";
  html += "</div></div>";
  html += "<form class='form' method='POST' action='/save'>";
  html += "<div class='field'><label>WiFi 名称 / 中控热点名称</label><input name='ssid' value='" + htmlEscape(wifiSsid) + "' placeholder='例如：Lingxi-Gateway' required><div class='hint'>若中控热点已改名，请填写中控当前热点名称；也可以填写中控所在家庭 WiFi。</div></div>";
  html += "<div class='field'><label>WiFi 密码</label><input name='pass' type='password' value='" + htmlEscape(wifiPassword) + "' placeholder='请输入 WiFi 密码'></div>";
  html += "<div class='field'><label>中控 IP 地址</label><input name='host' value='" + htmlEscape(gatewayHost) + "' placeholder='192.168.4.1' required><div class='hint'>中控热点模式下通常为 192.168.4.1。</div></div>";
  html += "<button class='btn' type='submit'>保存配置并重启节点</button></form>";
  html += "<form method='POST' action='/reset'><button class='btn danger' type='submit'>清除配置并重启</button></form>";
  html += "<div class='foot'>保存后，请在中控 WebUI 的设备管理 / 网关与节点页面查看 SensorBoard-01 是否在线。</div>";
  html += "</div></div></body></html>";
  configServer.send(200, "text/html; charset=utf-8", html);
}

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

  configServer.send(200, "text/html; charset=utf-8", "<meta charset='utf-8'><h2>已保存，节点正在重启...</h2><p>请稍后回到中控页面查看节点在线状态。</p>");
  delay(800);
  ESP.restart();
}

void resetNodeConfig() {
  nodePrefs.begin("lx_node", false);
  nodePrefs.clear();
  nodePrefs.end();
  configServer.send(200, "text/html; charset=utf-8", "<meta charset='utf-8'><h2>配置已清除，节点正在重启...</h2>");
  delay(800);
  ESP.restart();
}

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

void startConfigPortal(const char* reason) {
  if (configPortalRunning) return;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 8, 1), IPAddress(192, 168, 8, 1), IPAddress(255, 255, 255, 0));
  configApSsid = LINGXI_CONFIG_AP_SSID;
  bool apOk = WiFi.softAP(configApSsid.c_str(), LINGXI_CONFIG_AP_PASSWORD);

  configServer.on("/", HTTP_GET, sendConfigPage);
  configServer.on("/generate_204", HTTP_GET, sendConfigPage);
  configServer.on("/fwlink", HTTP_GET, sendConfigPage);
  configServer.on("/hotspot-detect.html", HTTP_GET, sendConfigPage);
  configServer.on("/save", HTTP_POST, saveNodeConfig);
  configServer.on("/save", HTTP_GET, sendConfigPage);
  configServer.on("/reset", HTTP_POST, resetNodeConfig);
  configServer.on("/status", HTTP_GET, sendConfigStatus);
  configServer.onNotFound(sendConfigPage);

  dnsServer.start(LINGXI_DNS_PORT, "*", IPAddress(192, 168, 8, 1));
  configServer.begin();
  configPortalRunning = true;

  Serial.print("[Config] portal started, reason=");
  Serial.println(reason);
  Serial.print("[Config] AP SSID=");
  Serial.println(configApSsid);
  Serial.print("[Config] AP PASS=");
  Serial.println(LINGXI_CONFIG_AP_PASSWORD);
  Serial.print("[Config] AP IP=");
  Serial.println(WiFi.softAPIP());
  if (!apOk) Serial.println("[Config] softAP start failed");
}

void configPortalTick() {
  if (!configPortalRunning) return;
  dnsServer.processNextRequest();
  configServer.handleClient();
}

String endpointsJson() {
  String json = "[";
  json += "{\"id\":\"temperature\",\"name\":\"室内温度\",\"type\":\"sensor\",\"capability\":\"temperature\",\"room\":\"客厅\",\"unit\":\"C\"},";
  json += "{\"id\":\"humidity\",\"name\":\"室内湿度\",\"type\":\"sensor\",\"capability\":\"humidity\",\"room\":\"客厅\",\"unit\":\"%\"},";
  json += "{\"id\":\"light_level\",\"name\":\"环境光照\",\"type\":\"sensor\",\"capability\":\"light\",\"room\":\"客厅\",\"unit\":\"%\"},";
  json += "{\"id\":\"door_main_contact\",\"name\":\"大门门磁\",\"type\":\"sensor\",\"capability\":\"contact\",\"room\":\"玄关\"},";
  json += "{\"id\":\"window_contact\",\"name\":\"窗户门磁\",\"type\":\"sensor\",\"capability\":\"contact\",\"room\":\"阳台\"},";
  json += "{\"id\":\"human_living\",\"name\":\"客厅人体\",\"type\":\"sensor\",\"capability\":\"presence\",\"room\":\"客厅\"},";
  json += "{\"id\":\"human_bedroom\",\"name\":\"卧室人体\",\"type\":\"sensor\",\"capability\":\"presence\",\"room\":\"卧室\"},";
  json += "{\"id\":\"smoke\",\"name\":\"烟雾传感器\",\"type\":\"sensor\",\"capability\":\"smoke\",\"room\":\"厨房\"},";
  json += "{\"id\":\"gas\",\"name\":\"燃气传感器\",\"type\":\"sensor\",\"capability\":\"gas\",\"room\":\"厨房\"}";
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
  json += "\"transport\":\"wifi_http+esp_now\",";
  json += "\"mac\":\"" + jsonEscape(localMac) + "\",";
  json += "\"firmware\":\"" + jsonEscape(String(LINGXI_NODE_FIRMWARE)) + "\",";
  json += "\"status\":\"registered\",";
  json += "\"endpoints\":" + endpointsJson();
  json += "}";
  return json;
}

String buildReportJson() {
  int smokeAlarm = (millis() > MQ_WARMUP_MS && sensorPacket.smokeValue > SMOKE_ALARM_THRESHOLD) ? 1 : 0;
  int gasAlarm = (millis() > MQ_WARMUP_MS && sensorPacket.gasValue > GAS_ALARM_THRESHOLD) ? 1 : 0;

  String json = "{";
  json += "\"node_id\":\"" + jsonEscape(String(LINGXI_NODE_ID)) + "\",";
  json += "\"name\":\"" + jsonEscape(String(LINGXI_NODE_NAME)) + "\",";
  json += "\"node_type\":\"" + jsonEscape(String(LINGXI_NODE_TYPE)) + "\",";
  json += "\"role\":\"" + jsonEscape(String(LINGXI_NODE_ROLE)) + "\",";
  json += "\"transport\":\"wifi_http+esp_now\",";
  json += "\"mac\":\"" + jsonEscape(localMac) + "\",";
  json += "\"status\":\"online\",";
  json += "\"seq\":" + String(httpReportSeq++) + ",";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"temperature\":" + String(sensorPacket.temperature, 1) + ",";
  json += "\"humidity\":" + String(sensorPacket.humidity, 1) + ",";
  json += "\"light_lux\":" + String(sensorPacket.lightLevelValue, 1) + ",";
  json += "\"light_level\":" + String(lightLuxToPercent(sensorPacket.lightLevelValue)) + ",";
  json += "\"door_main_contact\":" + String(sensorPacket.mainDoorContact) + ",";
  json += "\"window_contact\":" + String(sensorPacket.windowContact) + ",";
  json += "\"human_living\":" + String(sensorPacket.livingPirState) + ",";
  json += "\"human_bedroom\":" + String(sensorPacket.bedroomPirState) + ",";
  json += "\"smoke\":" + String(sensorPacket.smokeValue) + ",";
  json += "\"gas\":" + String(sensorPacket.gasValue) + ",";
  json += "\"smoke_alarm\":" + String(smokeAlarm) + ",";
  json += "\"gas_alarm\":" + String(gasAlarm) + ",";
  json += "\"temperature_valid\":" + String(sensorPacket.temperature > -50.0f ? "true" : "false") + ",";
  json += "\"humidity_valid\":" + String(sensorPacket.humidity >= 0.0f ? "true" : "false") + ",";
  json += "\"light_valid\":" + String(sensorPacket.lightLevelValue >= 0.0f ? "true" : "false") + ",";
  json += "\"pir_mode\":\"motion_trigger\"";
  json += "}";
  return json;
}

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
  Serial.print(ok ? "yes" : "no");
  if (response.length()) {
    Serial.print(" resp=");
    if (response.length() > 120) Serial.println(response.substring(0, 120) + "...");
    else Serial.println(response);
  } else {
    Serial.println();
  }

  recentHttpOk = ok;
  if (ok) lastHttpSuccessMs = millis();
  return ok;
}

bool sendHttpRegister() {
  bool ok = postJsonToGateway("/api/node/register", buildRegisterJson());
  httpRegisterOk = ok;
  if (ok) Serial.println("[HTTP] node register OK");
  else Serial.println("[HTTP] node register failed");
  return ok;
}

bool sendHttpReport() {
  bool ok = postJsonToGateway("/api/node/report", buildReportJson());
  if (ok) Serial.println("[HTTP] node report OK");
  else Serial.println("[HTTP] node report failed");
  return ok;
}

void initWifiHttp() {
  Serial.println("[HTTP] WiFi init...");

  if (!hasSavedWifiConfig) {
    startConfigPortal("first_boot_no_saved_config");
  }

  WiFi.mode(configPortalRunning ? WIFI_AP_STA : WIFI_STA);
  WiFi.setSleep(false);
  localMac = WiFi.macAddress();

  Serial.print("[HTTP] MAC: ");
  Serial.println(localMac);

  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiHttpOk = true;
    Serial.print("[HTTP] WiFi connected. IP=");
    Serial.print(WiFi.localIP());
    Serial.print(" channel=");
    Serial.println(WiFi.channel());
    sendHttpRegister();
  } else {
    wifiHttpOk = false;
    Serial.println("[HTTP] WiFi connect failed. Config portal and ESP-NOW compatibility path remain available.");
    startConfigPortal("wifi_connect_failed");
  }
}

void updateWifiHttp() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    wifiHttpOk = false;
    httpRegisterOk = false;

    if (now - lastWifiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
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
  }

  if (now - lastHttpReportMs >= HTTP_REPORT_INTERVAL_MS) {
    lastHttpReportMs = now;
    sendHttpReport();
  }
}

// =====================================================
// 12. ESP-NOW 初始化
// =====================================================

void initEspNow() {
  WiFi.mode(configPortalRunning ? WIFI_AP_STA : WIFI_STA);

  if (WiFi.status() != WL_CONNECTED) {
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] init failed");
    espNowOk = false;
    return;
  }

  esp_now_register_send_cb(onEspNowSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayMac, 6);
  peerInfo.channel = (WiFi.status() == WL_CONNECTED) ? WiFi.channel() : 1;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ESP-NOW] add gateway peer failed");
    espNowOk = false;
    return;
  }

  espNowOk = true;
  Serial.print("[ESP-NOW] init OK, channel=");
  Serial.println(peerInfo.channel);
}

// =====================================================
// 13. Arduino setup / loop
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("灵汐全屋智能控制中心 - 感知层集成节点");
  Serial.println("感知节点 V1.0.0 / 兼容中控 V1.4.2 / SensorBoard-01 Integrated Rewrite");
  Serial.println("========================================");
  Serial.print("C3 MAC: ");
  Serial.println(WiFi.macAddress());

  loadNodeConfig();

  rgb.begin();
  rgb.clear();
  rgb.show();
  rgb.setBrightness(24);

  analogReadResolution(12);
  analogSetPinAttenuation(SMOKE_ADC_PIN, ADC_11db);
  analogSetPinAttenuation(GAS_ADC_PIN, ADC_11db);

  pinMode(LIVING_PIR_PIN, PIR_INPUT_MODE);
  pinMode(BEDROOM_PIR_PIN, PIR_INPUT_MODE);
  pinMode(MAIN_DOOR_CONTACT_PIN, INPUT_PULLUP);
  pinMode(WINDOW_CONTACT_PIN, INPUT_PULLUP);

  dht.begin();
  bh1750Ready = initLightSensor();
  if (bh1750Ready) Serial.println("[Light] BH1750 direct I2C init OK");
  else Serial.println("[Light] BH1750 direct I2C init failed, will retry");

  sensorPacket.temperature = -1.0f;
  sensorPacket.humidity = -1.0f;
  sensorPacket.lightLevelValue = 0.0f;
  sensorPacket.smokeValue = 0;
  sensorPacket.gasValue = 0;
  sensorPacket.livingPirState = 0;
  sensorPacket.bedroomPirState = 0;
  sensorPacket.mainDoorContact = 0;
  sensorPacket.windowContact = 0;
  sensorPacket.timestampMs = 0;
  sensorPacket.packetSeq = 0;

  // 先读一次，避免刚启动时状态为空。
  readDigitalSensors();
  readSmokeGasSensors();
  readLightSensor();
  readDht22();

  initWifiHttp();
  initEspNow();

  Serial.println("[System] SensorBoard-01 running");
}

void loop() {
  unsigned long now = millis();

  if (now - lastDigitalReadMs >= DIGITAL_READ_INTERVAL_MS) {
    lastDigitalReadMs = now;
    readDigitalSensors();
    updateImmediateReportFlag();
  }

  if (now - lastMqReadMs >= MQ_READ_INTERVAL_MS) {
    lastMqReadMs = now;
    readSmokeGasSensors();
    updateImmediateReportFlag();
  }

  if (now - lastLightReadMs >= LIGHT_READ_INTERVAL_MS) {
    lastLightReadMs = now;
    readLightSensor();
  }

  if (now - lastDhtReadMs >= DHT_READ_INTERVAL_MS) {
    lastDhtReadMs = now;
    readDht22();
  }

  bool periodicSend = (now - lastEspNowSendMs >= ESPNOW_SEND_INTERVAL_MS);
  bool immediateSend = (immediateReportRequested && (now - lastEspNowSendMs >= ESPNOW_SEND_MIN_INTERVAL_MS));

  if (periodicSend || immediateSend) {
    lastEspNowSendMs = now;
    immediateReportRequested = false;
    sendEspNowSensorPacket();
  }

  updateWifiHttp();
  configPortalTick();
  updateStatusLight();
}
