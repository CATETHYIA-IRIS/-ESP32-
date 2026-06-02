/*
 * 项目名称：灵汐全屋智能控制中心
 * 固件名称：Lingxi_ActionLayerNode_V1_4_2
 * 公开版本：公测版 V1.4.2
 * 节点类型：执行层集成节点
 *
 * 模块职责：
 * 1. 作为 ActionBoard-01 兼容执行层节点，对接冻结版中控 Lingxi_PublicBeta_V1_4_2。
 * 2. 负责客厅灯、卧室灯、四路 5V 继电器、两个 28BYJ-48 窗帘步进电机、窗户舵机。
 * 3. 保留配网、配置保存、WiFi 重连、HTTP 注册/上报、ESP-NOW 命令接收。
 * 4. 电机采用已实测通过的 M2 相序和低速完整动作，演示时更直观。
 * 5. 窗户舵机默认按 180° 位置舵机处理；如临时仍用 360° 连续舵机，可改 WINDOW_SERVO_POSITION_MODE 为 0。
 *
 * 可改范围：
 * - WiFi 默认参数、中控地址、舵机开合角度、窗帘步数、步进间隔、串口调试命令。
 *
 * 禁止随意更改：
 * - node_id=ActionBoard-01、九个 endpoint id、GPIO 引脚表、HTTP 注册/上报路径、ESP-NOW 命令结构体。
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_arduino_version.h>
#include <Adafruit_NeoPixel.h>
#include <pgmspace.h>

// =====================================================
// Arduino IDE 自动函数声明修复
// =====================================================

typedef struct LightStripState LightStripState;
typedef struct CurtainMotorState CurtainMotorState;

void sendActionBoardState(bool forceSend);

void setLightMode(
  LightStripState &d,
  bool kai,
  int level,
  int mode,
  int r,
  int g,
  int b
);

void updateSingleLightStrip(
  Adafruit_NeoPixel &strip,
  int num,
  LightStripState &d
);

void driveMotorOutput(CurtainMotorState &m, int idx);
void stopMotor(CurtainMotorState &m);
void initCurtainMotor(CurtainMotorState &m, int a, int b, int c, int d);
void setCurtainTarget(CurtainMotorState &m, int percent);
void updateCurtainMotor(CurtainMotorState &m);
void startCurtainPhysicalDemo(CurtainMotorState &m, int direction, long steps);
void printSerialDemoHelp();
void handleSerialDemoCommand();
void setWindowServoTarget(int percent);
void stopWindowServo();

// =====================================================
// 公测版 V1.4.2 ActionNode HTTP Register/Report 函数声明
// =====================================================
bool LingxiNodeWifiConnect(unsigned long timeoutMs);
void InitLingxiNodeHttp();
void LingxiNodeHttpTick();
void LingxiNodeHttpRegister(bool forceSend);
void LingxiNodeHttpReport(bool forceSend);
bool LingxiNodeHttpPost(const char* path, const String &payload, const char* tag);
String LingxiJsonEscape(String s);
String LingxiNodeEndpointsJson();
String LingxiNodeRegisterJson();
String LingxiNodeReportJson();

// =====================================================
// 灵汐全屋智能控制中心 执行层集成节点
// 执行层封装优化版：配网页、RGB 状态灯与场景灯效收口
// 设计原则：GPIO 固定在执行层固件，中控只下发设备语义命令
// =====================================================


// =====================================================
// 1. 基础配置
// =====================================================

// 中控层 S3 MAC：20:6E:F1:A6:9D:3C
uint8_t gatewayMac[] = {0x20, 0x6E, 0xF1, 0xA6, 0x9D, 0x3C};

// 执行层 S3 MAC：A4:CB:8F:D8:BF:08
// 这里只做备注，不需要写入代码发送对象
// A4:CB:8F:D8:BF:08

#define ESPNOW_CHANNEL 1

// =====================================================
// 2. 引脚定义
// =====================================================

// 板载 RGB 状态灯
#define ZHUANGTAI_RGB_PIN 48
#define ZHUANGTAI_RGB_NUM 1

// 四路继电器，低电平触发
#define RELAY_AIR_PIN      7   // 空调电源 IN1
#define RELAY_TV_PIN       6   // 电视电源 IN2
#define RELAY_FRIDGE_PIN   5   // 冰箱电源 IN3
#define RELAY_WATER_PIN    4   // 热水器电源 IN4

#define RELAY_ON   LOW
#define RELAY_OFF  HIGH

// 两条 WS2812 灯带
#define KETING_LED_PIN 15
#define WOSHI_LED_PIN  16
#define KETING_LED_NUM 8
#define WOSHI_LED_NUM  8

// 窗户 360° 舵机
#define CHUANGHU_SERVO_PIN 17

// 红外串口模块
#define IR_RX_PIN 18
#define IR_TX_PIN 21
#define IR_BAUD   9600

// 客厅窗帘 ULN2003 + 28BYJ-48
#define KT_MOTOR_IN1 38
#define KT_MOTOR_IN2 39
#define KT_MOTOR_IN3 40
#define KT_MOTOR_IN4 41

// 卧室窗帘 ULN2003 + 28BYJ-48
#define WS_MOTOR_IN1 9
#define WS_MOTOR_IN2 10
#define WS_MOTOR_IN3 11
#define WS_MOTOR_IN4 12


// =====================================================
// 3. 命令来源
// =====================================================

#define SRC_UNKNOWN  0
#define SRC_WEB      1
#define SRC_LVGL     2
#define SRC_VOICE    3
#define SRC_CLOUD    4
#define SRC_AI       5
#define SRC_AUTO     6
#define SRC_ALARM    7


// =====================================================
// 4. 灯光模式
// =====================================================

#define LIGHT_MODE_NORMAL       0   // 常亮
#define LIGHT_MODE_WELCOME      1   // 回家迎宾灯
#define LIGHT_MODE_SENDOFF      2   // 离家送客灯
#define LIGHT_MODE_MOVIE        3   // 观影蓝紫低亮
#define LIGHT_MODE_SLEEP        4   // 睡眠低暖光
#define LIGHT_MODE_OFFICE       5   // 办公冷白光
#define LIGHT_MODE_COMFORT      6   // 舒适暖光
#define LIGHT_MODE_NIGHT        7   // 起夜淡光
#define LIGHT_MODE_ALERT        8   // 报警红蓝闪


// =====================================================
// 5. 场景模式
// =====================================================

#define SCENE_NONE       0
#define SCENE_AUTO       1
#define SCENE_AWAY       2
#define SCENE_HOME       3
#define SCENE_SLEEP      4
#define SCENE_ALARM      9
#define SCENE_COMFORT    10
#define SCENE_QUIET      11
#define SCENE_OFFICE     12
#define SCENE_READING    13
#define SCENE_MOVIE      14
#define SCENE_NIGHT      18


// =====================================================
// 6. 红外空调命令
// =====================================================

#define IR_AIR_NONE       0
#define IR_AIR_POWER      1
#define IR_AIR_TEMP_UP    2
#define IR_AIR_TEMP_DOWN  3
#define IR_AIR_MODE       4
#define IR_AIR_FAN        5
#define IR_AIR_SEND_FULL  6


// =====================================================
// 7. 当前中控 V2 结构体，兼容你现在的中控
// =====================================================

typedef struct {
  uint32_t cmdSeq;

  int lightCmd;
  int lightLevel;
  int relayCmd;
  int servoCmd;
  int motorCmd;
  int buzzerCmd;
  int sceneCmd;

  int livingLightCmd;
  int bedroomLightCmd;

  int airRelayCmd;
  int tvRelayCmd;
  int fridgeRelayCmd;
  int waterRelayCmd;

  int livingCurtainCmd;
  int bedroomCurtainCmd;

  int windowServoCmd;
  int irAirCmd;
} ControlCommandPacketV2;


// =====================================================
// 8. 旧版结构体兼容
// =====================================================

typedef struct {
  int lightCmd;
  int lightLevel;
  int relayCmd;
  int servoCmd;
  int motorCmd;
  int buzzerCmd;
  int sceneCmd;
} ControlCommandPacketOld;


// =====================================================
// 9. Ultra 最终结构体
// 后续中控、语音、云端、AI 都可以逐步升级到这个结构体
// =====================================================

typedef struct {
  uint32_t cmdSeq;

  int commandSource;
  int commandPriority;

  int livingLightCmd;
  int bedroomLightCmd;
  int livingLightLevel;
  int bedroomLightLevel;
  int livingLightMode;
  int bedroomLightMode;
  int globalLightMode;

  int livingLightR;
  int livingLightG;
  int livingLightB;
  int bedroomLightR;
  int bedroomLightG;
  int bedroomLightB;

  int airRelayCmd;
  int tvRelayCmd;
  int fridgeRelayCmd;
  int waterRelayCmd;

  int livingCurtainCmd;
  int bedroomCurtainCmd;
  int livingCurtainPercent;
  int bedroomCurtainPercent;

  int windowServoCmd;
  int windowPercent;

  int irAirCmd;
  int irAirPower;
  int irAirMode;
  int irAirTemp;
  int irAirFan;

  int buzzerCmd;
  int sceneCmd;
} ControlCommandPacket;


// =====================================================
// 10. 执行层回传状态结构体
// 中控后续需要接收这个，WebUI 才能真正同步
// =====================================================

typedef struct {
  uint32_t statusSeq;
  uint32_t lastCmdSeq;
  uint32_t uptimeMs;

  int executorOnline;
  int lastCommandSource;

  int livingLightState;
  int bedroomLightState;
  int livingLightLevel;
  int bedroomLightLevel;
  int livingLightMode;
  int bedroomLightMode;

  int airRelayState;
  int tvRelayState;
  int fridgeRelayState;
  int waterRelayState;

  int livingCurtainState;
  int bedroomCurtainState;
  int livingCurtainPercent;
  int bedroomCurtainPercent;

  int windowState;
  int windowPercent;

  int irAirPower;
  int irAirMode;
  int irAirTemp;
  int irAirFan;

  int alarmState;
} ActionBoardState;


// =====================================================
// 11. 对象与全局变量
// =====================================================

Adafruit_NeoPixel statusRgb(ZHUANGTAI_RGB_NUM, ZHUANGTAI_RGB_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel livingLedStrip(KETING_LED_NUM, KETING_LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel bedroomLedStrip(WOSHI_LED_NUM, WOSHI_LED_PIN, NEO_GRB + NEO_KHZ800);

ControlCommandPacket currentCommandPacket;
ActionBoardState actionStatus;

bool EspNowInitOk = false;
volatile unsigned long ZuiHouShouDaoTime = 0;

uint32_t statusSeqCounter = 0;
unsigned long lastStatusSendMs = 0;
unsigned long ShangCiDaYinTime = 0;

#define STATUS_SEND_INTERVAL 1000UL
#define STATUS_PRINT_INTERVAL 3000UL

// =====================================================
// 公测版 V1.4.2 ActionLayerNode IntegratedLogicPatch
// 保留节点配网 / HTTP 注册上报 / ESP-NOW 控制链路。
// 本版重点修复灯光场景、继电器保护、窗户舵机和配网热点自动关闭。
// =====================================================
#define LINGXI_ACTION_NODE_ID       "ActionBoard-01"
#define LINGXI_ACTION_NODE_NAME     "执行层集成节点"
#define LINGXI_ACTION_NODE_TYPE     "action_board"
#define LINGXI_ACTION_NODE_ROLE     "action"
#define LINGXI_ACTION_NODE_FW       "Lingxi_ActionLayerNode_V1_4_2_ForHubV1_4_2"

#define LINGXI_DEFAULT_WIFI_SSID      "Lingxi-Gateway"
#define LINGXI_DEFAULT_WIFI_PASSWORD  "12345678"
#define LINGXI_DEFAULT_CENTER_HOST    "192.168.4.1"
#define LINGXI_CONFIG_AP_PASSWORD     "999999999"

// 配网页不使用图片 Logo，避免占用 Flash；只保留蓝白文字品牌头部。

String LingxiWifiSsid = LINGXI_DEFAULT_WIFI_SSID;
String LingxiWifiPassword = LINGXI_DEFAULT_WIFI_PASSWORD;
String LingxiCenterHost = LINGXI_DEFAULT_CENTER_HOST;
const uint16_t LINGXI_CENTER_PORT = 80;
bool LingxiHasSavedWifiConfig = false;

Preferences LingxiNodePrefs;
WebServer LingxiConfigServer(80);
DNSServer LingxiDnsServer;
const byte LINGXI_DNS_PORT = 53;
bool LingxiConfigPortalRunning = false;
String LingxiConfigApSsid = "";

bool LingxiNodeWifiOk = false;
bool LingxiNodeRegistered = false;
unsigned long LingxiLastWifiCheckMs = 0;
unsigned long LingxiLastRegisterMs = 0;
unsigned long LingxiLastHttpReportMs = 0;
uint32_t LingxiHttpReportSeq = 0;

#define LINGXI_WIFI_RETRY_INTERVAL_MS     10000UL
#define LINGXI_REGISTER_INTERVAL_MS       30000UL
#define LINGXI_HTTP_REPORT_INTERVAL_MS     3000UL


// 执行层安全策略：继电器不能高速重复吸合，所有继电器动作必须经过统一保护入口。
#define RELAY_MIN_ACTION_INTERVAL_MS 1000UL
unsigned long lastAirRelayActionMs = 0;
unsigned long lastTvRelayActionMs = 0;
unsigned long lastFridgeRelayActionMs = 0;
unsigned long lastWaterRelayActionMs = 0;

// 配网热点策略：首次/失败时打开，注册并上报成功后自动关闭，避免一堆节点热点常驻。
#define CONFIG_PORTAL_CLOSE_AFTER_REPORT_OK 1

// 窗户舵机策略：默认按普通 0-180 度位置舵机处理；如果换成 360 度连续舵机，可把此宏改为 0。
#define WINDOW_SERVO_POSITION_MODE 1




// =====================================================
// 12. 工具函数
// =====================================================

int XianZhi(int v, int minV, int maxV) {
  if (v < minV) return minV;
  if (v > maxV) return maxV;
  return v;
}

const char* LaiYuanMingCheng(int src) {
  switch (src) {
    case SRC_WEB: return "WebUI";
    case SRC_LVGL: return "LVGL";
    case SRC_VOICE: return "Voice";
    case SRC_CLOUD: return "Cloud";
    case SRC_AI: return "AI";
    case SRC_AUTO: return "Auto";
    case SRC_ALARM: return "Alarm";
    default: return "Unknown";
  }
}

uint32_t SuoFangYanSe(Adafruit_NeoPixel &strip, int r, int g, int b, int level) {
  level = XianZhi(level, 0, 255);
  r = XianZhi((r * level) / 255, 0, 255);
  g = XianZhi((g * level) / 255, 0, 255);
  b = XianZhi((b * level) / 255, 0, 255);
  return strip.Color(r, g, b);
}

uint32_t CaiHongSe(Adafruit_NeoPixel &strip, byte pos, int level) {
  pos = 255 - pos;
  if (pos < 85) {
    return SuoFangYanSe(strip, 255 - pos * 3, 0, pos * 3, level);
  }
  if (pos < 170) {
    pos -= 85;
    return SuoFangYanSe(strip, 0, pos * 3, 255 - pos * 3, level);
  }
  pos -= 170;
  return SuoFangYanSe(strip, pos * 3, 255 - pos * 3, 0, level);
}


// =====================================================
// 13. 板载 RGB 状态灯
// =====================================================

void setStatusRgb(uint8_t r, uint8_t g, uint8_t b) {
  statusRgb.setPixelColor(0, statusRgb.Color(r, g, b));
  statusRgb.show();
}

void updateStatusLight() {
  static unsigned long lastRgbTime = 0;
  if (millis() - lastRgbTime < 30) return;
  lastRgbTime = millis();

  unsigned long now = millis();

  // 蓝色长呼吸：节点正在配网，等待用户连接“灵汐集成执行中心”热点并填写中控信息。
  if (LingxiConfigPortalRunning) {
    float p = (sin(now / 1300.0) + 1.0) / 2.0;
    setStatusRgb(0, 12 + p * 30, 70 + p * 150);
    return;
  }

  // 红色常亮：未连接 WiFi，说明节点暂时无法注册到中控。
  if (WiFi.status() != WL_CONNECTED) {
    setStatusRgb(120, 0, 0);
    return;
  }
// 青绿色长呼吸：WiFi + HTTP 在线。
// 亮度从 0 缓慢升到 128，再从 128 缓慢降到 0，避免状态灯过亮晃眼。
const unsigned long HU_XI_ZHOU_QI = 4200;  // 一次完整呼吸周期，单位 ms
unsigned long phase = now % HU_XI_ZHOU_QI;

uint8_t liangDu = 0;
if (phase < HU_XI_ZHOU_QI / 2) {
  liangDu = map(phase, 0, HU_XI_ZHOU_QI / 2, 0, 128);
} else {
  liangDu = map(phase, HU_XI_ZHOU_QI / 2, HU_XI_ZHOU_QI, 128, 0);
}

// 青绿色：绿色略高，蓝色略低，最大亮度控制在 128 以内。
uint8_t r = 0;
uint8_t g = liangDu;
uint8_t b = (liangDu * 90) / 128;

setStatusRgb(r, g, b);
}


// =====================================================
// 14. 四路继电器
// =====================================================

void InitJiDianQi() {
  digitalWrite(RELAY_AIR_PIN, RELAY_OFF);
  digitalWrite(RELAY_TV_PIN, RELAY_OFF);
  digitalWrite(RELAY_FRIDGE_PIN, RELAY_OFF);
  digitalWrite(RELAY_WATER_PIN, RELAY_OFF);

  pinMode(RELAY_AIR_PIN, OUTPUT);
  pinMode(RELAY_TV_PIN, OUTPUT);
  pinMode(RELAY_FRIDGE_PIN, OUTPUT);
  pinMode(RELAY_WATER_PIN, OUTPUT);

  digitalWrite(RELAY_AIR_PIN, RELAY_OFF);
  digitalWrite(RELAY_TV_PIN, RELAY_OFF);
  digitalWrite(RELAY_FRIDGE_PIN, RELAY_OFF);
  digitalWrite(RELAY_WATER_PIN, RELAY_OFF);

  actionStatus.airRelayState = 0;
  actionStatus.tvRelayState = 0;
  actionStatus.fridgeRelayState = 0;
  actionStatus.waterRelayState = 0;

  Serial.println("JiDianQi init OK. All OFF.");
}

bool setRelaySafe(const char* name, uint8_t pin, bool targetOn, int &state, unsigned long &lastActionMs) {
  unsigned long now = millis();

  // 幂等保护：当前状态已经符合目标，就不再重复写 GPIO，避免继电器抖动。
  if (state == (targetOn ? 1 : 0)) {
    return false;
  }

  // 防抖保护：短时间内禁止同一路继电器反复吸合释放。
  if (lastActionMs != 0 && now - lastActionMs < RELAY_MIN_ACTION_INTERVAL_MS) {
    Serial.print("[RelaySafe] ignore fast repeat: ");
    Serial.println(name);
    return false;
  }

  digitalWrite(pin, targetOn ? RELAY_ON : RELAY_OFF);
  state = targetOn ? 1 : 0;
  lastActionMs = now;

  Serial.print("[RelaySafe] ");
  Serial.print(name);
  Serial.println(targetOn ? " -> ON" : " -> OFF");
  return true;
}

void YingYongJiDianQi(ControlCommandPacket m) {
  // 继电器命令按绝对状态执行：1=开，0=关；状态相同则忽略，不允许高速启停。
  setRelaySafe("AirRelay", RELAY_AIR_PIN, m.airRelayCmd != 0, actionStatus.airRelayState, lastAirRelayActionMs);
  setRelaySafe("TvRelay", RELAY_TV_PIN, m.tvRelayCmd != 0, actionStatus.tvRelayState, lastTvRelayActionMs);
  setRelaySafe("FridgeRelay", RELAY_FRIDGE_PIN, m.fridgeRelayCmd != 0, actionStatus.fridgeRelayState, lastFridgeRelayActionMs);
  setRelaySafe("WaterHeaterRelay", RELAY_WATER_PIN, m.waterRelayCmd != 0, actionStatus.waterRelayState, lastWaterRelayActionMs);

  Serial.println("---------- Relay State ----------");
  Serial.print("AirRelay: "); Serial.println(actionStatus.airRelayState ? "ON" : "OFF");
  Serial.print("TvRelay : "); Serial.println(actionStatus.tvRelayState ? "ON" : "OFF");
  Serial.print("FridgeRelay: "); Serial.println(actionStatus.fridgeRelayState ? "ON" : "OFF");
  Serial.print("WaterHeaterRelay: "); Serial.println(actionStatus.waterRelayState ? "ON" : "OFF");
}


// =====================================================
// 15. 灯带状态结构
// =====================================================


struct LightStripState {
  bool kai;
  int level;
  int mode;
  int r;
  int g;
  int b;
  unsigned long modeStartTime;
  unsigned long lastUpdateTime;
  int step;
};

LightStripState livingLightStrip;
LightStripState bedroomLightStrip;

void QingKongDengDai(Adafruit_NeoPixel &strip) {
  strip.clear();
  strip.show();
}

void TianChongDengDai(Adafruit_NeoPixel &strip, int num, int r, int g, int b, int level) {
  uint32_t c = SuoFangYanSe(strip, r, g, b, level);
  for (int i = 0; i < num; i++) strip.setPixelColor(i, c);
  strip.show();
}

void InitDengGuang() {
  livingLedStrip.begin();
  livingLedStrip.clear();
  livingLedStrip.show();
  livingLedStrip.setBrightness(255);

  bedroomLedStrip.begin();
  bedroomLedStrip.clear();
  bedroomLedStrip.show();
  bedroomLedStrip.setBrightness(255);

  livingLightStrip = {false, 140, LIGHT_MODE_NORMAL, 255, 180, 90, 0, 0, 0};
  bedroomLightStrip = {false, 120, LIGHT_MODE_NORMAL, 220, 210, 180, 0, 0, 0};

  actionStatus.livingLightState = 0;
  actionStatus.bedroomLightState = 0;

  Serial.println("DengGuang init OK.");
}

void setLightMode(LightStripState &d, bool kai, int level, int mode, int r, int g, int b) {
  d.kai = kai;
  d.level = XianZhi(level, 0, 255);
  d.mode = mode;
  d.r = XianZhi(r, 0, 255);
  d.g = XianZhi(g, 0, 255);
  d.b = XianZhi(b, 0, 255);
  d.modeStartTime = millis();
  d.lastUpdateTime = 0;
  d.step = 0;
}

void updateSingleLightStrip(Adafruit_NeoPixel &strip, int num, LightStripState &d) {
  if (!d.kai) {
    QingKongDengDai(strip);
    return;
  }

  unsigned long now = millis();
  if (now - d.lastUpdateTime < 40) return;
  d.lastUpdateTime = now;

  switch (d.mode) {
    case LIGHT_MODE_NORMAL:
      TianChongDengDai(strip, num, d.r, d.g, d.b, d.level);
      break;

    case LIGHT_MODE_WELCOME: {
      // 回家迎宾：暖色从一端逐颗点亮。
      if (now - d.modeStartTime > 120) {
        d.modeStartTime = now;
        if (d.step < num) d.step++;
      }
      strip.clear();
      uint32_t c = SuoFangYanSe(strip, 255, 175, 85, d.level);
      for (int i = 0; i < d.step && i < num; i++) strip.setPixelColor(i, c);
      strip.show();
      break;
    }

    case LIGHT_MODE_SENDOFF: {
      // 离家送客：暖色流动数秒后自动熄灭。
      if (now - d.modeStartTime > 6500) {
        d.kai = false;
        QingKongDengDai(strip);
        return;
      }
      strip.clear();
      int head = (now / 120) % num;
      for (int i = 0; i < num; i++) {
        int distance = abs(i - head);
        int lv = max(10, d.level - distance * 28);
        strip.setPixelColor(i, SuoFangYanSe(strip, 255, 160, 60, lv));
      }
      strip.show();
      break;
    }

    case LIGHT_MODE_MOVIE:
      // 观影：低亮蓝紫色，降低环境干扰。
      TianChongDengDai(strip, num, 35, 45, 150, d.level > 0 ? d.level : 70);
      break;

    case LIGHT_MODE_SLEEP:
      // 睡眠：极低亮暖色。
      TianChongDengDai(strip, num, 255, 88, 24, d.level > 0 ? d.level : 18);
      break;

    case LIGHT_MODE_OFFICE:
      // 办公：冷白偏蓝，高亮专注。
      TianChongDengDai(strip, num, 150, 205, 255, d.level > 0 ? d.level : 210);
      break;

    case LIGHT_MODE_COMFORT:
      // 舒适：稳定暖色。
      TianChongDengDai(strip, num, 255, 185, 92, d.level > 0 ? d.level : 150);
      break;

    case LIGHT_MODE_NIGHT:
      // 起夜：淡暖光。
      TianChongDengDai(strip, num, 255, 135, 45, d.level > 0 ? d.level : 35);
      break;

    case LIGHT_MODE_ALERT: {
      // 报警：红蓝交替闪。
      bool red = ((now / 220) % 2) == 0;
      if (red) TianChongDengDai(strip, num, 255, 0, 0, 210);
      else TianChongDengDai(strip, num, 0, 45, 255, 210);
      break;
    }

    default:
      TianChongDengDai(strip, num, d.r, d.g, d.b, d.level);
      break;
  }
}

void GengXinDengGuang() {
  updateSingleLightStrip(livingLedStrip, KETING_LED_NUM, livingLightStrip);
  updateSingleLightStrip(bedroomLedStrip, WOSHI_LED_NUM, bedroomLightStrip);
}

void YingYongDengGuang(ControlCommandPacket m) {
  // 设备控制页的灯光开关只允许静态单色，绝对不能继承回家/离家/报警/其他场景动态灯效。
  int ktLevel = m.livingLightLevel > 0 ? m.livingLightLevel : 180;
  int wsLevel = m.bedroomLightLevel > 0 ? m.bedroomLightLevel : 140;

  int ktR = m.livingLightR > 0 ? m.livingLightR : 255;
  int ktG = m.livingLightG > 0 ? m.livingLightG : 235;
  int ktB = m.livingLightB > 0 ? m.livingLightB : 200;

  int wsR = m.bedroomLightR > 0 ? m.bedroomLightR : 255;
  int wsG = m.bedroomLightG > 0 ? m.bedroomLightG : 210;
  int wsB = m.bedroomLightB > 0 ? m.bedroomLightB : 150;

  // 普通开灯/关灯：强制 LIGHT_MODE_NORMAL，禁止逆天 RGB 动态。
  setLightMode(livingLightStrip, m.livingLightCmd == 1, ktLevel, LIGHT_MODE_NORMAL, ktR, ktG, ktB);
  setLightMode(bedroomLightStrip, m.bedroomLightCmd == 1, wsLevel, LIGHT_MODE_NORMAL, wsR, wsG, wsB);

  actionStatus.livingLightState = m.livingLightCmd == 1 ? 1 : 0;
  actionStatus.bedroomLightState = m.bedroomLightCmd == 1 ? 1 : 0;
  actionStatus.livingLightLevel = actionStatus.livingLightState ? ktLevel : 0;
  actionStatus.bedroomLightLevel = actionStatus.bedroomLightState ? wsLevel : 0;
  actionStatus.livingLightMode = LIGHT_MODE_NORMAL;
  actionStatus.bedroomLightMode = LIGHT_MODE_NORMAL;
}


// =====================================================
// 16. 步进电机窗帘，非阻塞
//
// 重要说明：
// - 当前电机按 28BYJ-48 + ULN2003 设计。
// - 必须外部 5V 供电，ESP32 GND、ULN2003 GND、外部 5V 负极必须共地。
// - ULN2003 跳线帽必须插上，否则电机没有供电通路。
// - 引脚常量仍按物理 IN1/IN2/IN3/IN4 命名。
// - 实际初始化时使用 M2 相序：逻辑 A/B/C/D -> IN1/IN3/IN2/IN4。
// - 4096 步约为一圈，演示更直观；512 步只适合短调试，不作为正式动作默认值。
// =====================================================

const int BanBuXuLie[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

#define CURTAIN_TOTAL_STEPS 4096L          // 完整开/关动作默认 4096 步，肉眼能明显看到一圈动作
#define CURTAIN_STEP_INTERVAL 20UL         // 实测保守稳定速度：15~20ms，默认 20ms 更稳
#define CURTAIN_DEMO_STEPS 4096L           // 串口演示动作步数，默认一圈

struct CurtainMotorState {
  int in1;
  int in2;
  int in3;
  int in4;

  bool running;
  int fangXiang;
  int stepIndex;
  long remainSteps;
  int currentPercent;
  int targetPercent;
  unsigned long lastStepTime;
};

CurtainMotorState livingCurtainMotor;
CurtainMotorState bedroomCurtainMotor;

void driveMotorOutput(CurtainMotorState &m, int idx) {
  digitalWrite(m.in1, BanBuXuLie[idx][0]);
  digitalWrite(m.in2, BanBuXuLie[idx][1]);
  digitalWrite(m.in3, BanBuXuLie[idx][2]);
  digitalWrite(m.in4, BanBuXuLie[idx][3]);
}

void stopMotor(CurtainMotorState &m) {
  digitalWrite(m.in1, LOW);
  digitalWrite(m.in2, LOW);
  digitalWrite(m.in3, LOW);
  digitalWrite(m.in4, LOW);
  m.running = false;
  m.remainSteps = 0;
}

void initCurtainMotor(CurtainMotorState &m, int a, int b, int c, int d) {
  m.in1 = a;
  m.in2 = b;
  m.in3 = c;
  m.in4 = d;

  pinMode(m.in1, OUTPUT);
  pinMode(m.in2, OUTPUT);
  pinMode(m.in3, OUTPUT);
  pinMode(m.in4, OUTPUT);

  m.running = false;
  m.fangXiang = 1;
  m.stepIndex = 0;
  m.remainSteps = 0;
  m.currentPercent = 0;
  m.targetPercent = 0;
  m.lastStepTime = 0;

  stopMotor(m);
}

void InitChuangLian() {
  // M2 有效相序：逻辑 A/B/C/D -> IN1/IN3/IN2/IN4。
  // 注意：这里不是写错顺序，是为了适配实测通过的 28BYJ-48 + ULN2003 接线/相序。
  initCurtainMotor(livingCurtainMotor, KT_MOTOR_IN1, KT_MOTOR_IN3, KT_MOTOR_IN2, KT_MOTOR_IN4);
  initCurtainMotor(bedroomCurtainMotor, WS_MOTOR_IN1, WS_MOTOR_IN3, WS_MOTOR_IN2, WS_MOTOR_IN4);

  actionStatus.livingCurtainState = 0;
  actionStatus.bedroomCurtainState = 0;
  actionStatus.livingCurtainPercent = 0;
  actionStatus.bedroomCurtainPercent = 0;

  Serial.println("ChuangLian motor init OK.");
}

void setCurtainTarget(CurtainMotorState &m, int percent) {
  percent = XianZhi(percent, 0, 100);

  int cha = percent - m.currentPercent;
  if (cha == 0) {
    stopMotor(m);
    return;
  }

  m.targetPercent = percent;
  m.fangXiang = cha > 0 ? 1 : -1;
  m.remainSteps = abs(cha) * CURTAIN_TOTAL_STEPS / 100L;
  if (m.remainSteps < 10) m.remainSteps = 10;

  m.running = true;
}

void updateCurtainMotor(CurtainMotorState &m) {
  if (!m.running) return;
  unsigned long now = millis();
  if (now - m.lastStepTime < CURTAIN_STEP_INTERVAL) return;
  m.lastStepTime = now;

  m.stepIndex += m.fangXiang;
  if (m.stepIndex > 7) m.stepIndex = 0;
  if (m.stepIndex < 0) m.stepIndex = 7;

  driveMotorOutput(m, m.stepIndex);

  m.remainSteps--;
  if (m.remainSteps <= 0) {
    m.currentPercent = m.targetPercent;
    stopMotor(m);
  }
}

void GengXinChuangLian() {
  updateCurtainMotor(livingCurtainMotor);
  updateCurtainMotor(bedroomCurtainMotor);

  actionStatus.livingCurtainPercent = livingCurtainMotor.currentPercent;
  actionStatus.bedroomCurtainPercent = bedroomCurtainMotor.currentPercent;

  actionStatus.livingCurtainState = livingCurtainMotor.running ? 1 : 0;
  actionStatus.bedroomCurtainState = bedroomCurtainMotor.running ? 1 : 0;
}

void YingYongChuangLian(ControlCommandPacket m) {
  if (m.livingCurtainCmd == 1) {
    setCurtainTarget(livingCurtainMotor, 100);
  } else if (m.livingCurtainCmd == 2) {
    setCurtainTarget(livingCurtainMotor, 0);
  } else if (m.livingCurtainCmd == 3) {
    setCurtainTarget(livingCurtainMotor, m.livingCurtainPercent);
  } else if (m.livingCurtainCmd == 0) {
    stopMotor(livingCurtainMotor);
  }

  if (m.bedroomCurtainCmd == 1) {
    setCurtainTarget(bedroomCurtainMotor, 100);
  } else if (m.bedroomCurtainCmd == 2) {
    setCurtainTarget(bedroomCurtainMotor, 0);
  } else if (m.bedroomCurtainCmd == 3) {
    setCurtainTarget(bedroomCurtainMotor, m.bedroomCurtainPercent);
  } else if (m.bedroomCurtainCmd == 0) {
    stopMotor(bedroomCurtainMotor);
  }
}


// 手动启动一个物理演示动作，不依赖当前百分比。
// 用途：拍视频、答辩展示、单独确认电机方向。
// direction = 1 为打开方向，direction = -1 为关闭方向。
void startCurtainPhysicalDemo(CurtainMotorState &m, int direction, long steps) {
  if (steps <= 0) steps = CURTAIN_DEMO_STEPS;
  m.fangXiang = direction >= 0 ? 1 : -1;
  m.remainSteps = steps;
  m.targetPercent = direction >= 0 ? 100 : 0;
  m.running = true;
  Serial.print("[CurtainDemo] start direction=");
  Serial.print(m.fangXiang);
  Serial.print(" steps=");
  Serial.println(m.remainSteps);
}

void printSerialDemoHelp() {
  Serial.println();
  Serial.println("========== Lingxi ActionLayerNode Serial Demo ==========");
  Serial.println("1: 客厅窗帘打开，完整 4096 步");
  Serial.println("2: 客厅窗帘关闭，完整 4096 步");
  Serial.println("3: 卧室窗帘打开，完整 4096 步");
  Serial.println("4: 卧室窗帘关闭，完整 4096 步");
  Serial.println("t: 两个窗帘电机同时正转一圈，用于直观演示");
  Serial.println("y: 两个窗帘电机同时反转一圈，用于直观演示");
  Serial.println("s: 立即停止两个窗帘电机");
  Serial.println("o: 窗户舵机打开到 100%");
  Serial.println("c: 窗户舵机关闭到 0%");
  Serial.println("?: 显示本帮助");
  Serial.println("======================================================");
}

void handleSerialDemoCommand() {
  if (!Serial.available()) return;
  char ch = Serial.read();
  if (ch == '\r' || ch == '\n' || ch == ' ') return;

  if (ch == '1') {
    livingCurtainMotor.currentPercent = 0;
    setCurtainTarget(livingCurtainMotor, 100);
    Serial.println("[SerialDemo] living curtain open");
  } else if (ch == '2') {
    livingCurtainMotor.currentPercent = 100;
    setCurtainTarget(livingCurtainMotor, 0);
    Serial.println("[SerialDemo] living curtain close");
  } else if (ch == '3') {
    bedroomCurtainMotor.currentPercent = 0;
    setCurtainTarget(bedroomCurtainMotor, 100);
    Serial.println("[SerialDemo] bedroom curtain open");
  } else if (ch == '4') {
    bedroomCurtainMotor.currentPercent = 100;
    setCurtainTarget(bedroomCurtainMotor, 0);
    Serial.println("[SerialDemo] bedroom curtain close");
  } else if (ch == 't' || ch == 'T') {
    startCurtainPhysicalDemo(livingCurtainMotor, 1, CURTAIN_DEMO_STEPS);
    startCurtainPhysicalDemo(bedroomCurtainMotor, 1, CURTAIN_DEMO_STEPS);
  } else if (ch == 'y' || ch == 'Y') {
    startCurtainPhysicalDemo(livingCurtainMotor, -1, CURTAIN_DEMO_STEPS);
    startCurtainPhysicalDemo(bedroomCurtainMotor, -1, CURTAIN_DEMO_STEPS);
  } else if (ch == 's' || ch == 'S') {
    stopMotor(livingCurtainMotor);
    stopMotor(bedroomCurtainMotor);
    Serial.println("[SerialDemo] all curtain motors stopped");
  } else if (ch == 'o' || ch == 'O') {
    setWindowServoTarget(100);
    Serial.println("[SerialDemo] window servo open");
  } else if (ch == 'c' || ch == 'C') {
    setWindowServoTarget(0);
    Serial.println("[SerialDemo] window servo close");
  } else if (ch == '?') {
    printSerialDemoHelp();
  }
}


// =====================================================
// 17. 窗户舵机
// 默认按普通 180° 位置舵机处理：打开/关闭直接写角度对应脉宽。
// 后续更换 180° 舵机后无需改主逻辑，只需要按机械结构微调开合角度。
// 如临时仍使用 360° 连续舵机，把 WINDOW_SERVO_POSITION_MODE 改为 0 即可回到时间控制。
// =====================================================

#if WINDOW_SERVO_POSITION_MODE
#define WINDOW_SERVO_CLOSE_ANGLE 0
#define WINDOW_SERVO_OPEN_ANGLE  90
#define WINDOW_SERVO_MIN_US      500
#define WINDOW_SERVO_MAX_US      2500
#else
#define SERVO_STOP_US          1500
#define SERVO_OPEN_US          1700
#define SERVO_CLOSE_US         1300
#define WINDOW_FULL_TIME       1200UL
#endif

bool windowServoRunning = false;
int windowServoDirection = 0;
int windowServoCurrentPercent = 0;
int windowServoTargetPercent = 0;
unsigned long windowServoStartMs = 0;
unsigned long windowServoRunMs = 0;

void ServoWriteUs(int us) {
  us = XianZhi(us, 500, 2500);
  uint32_t duty = (uint32_t)((uint64_t)us * 65535ULL / 20000ULL);
  ledcWrite(CHUANGHU_SERVO_PIN, duty);
}

void stopWindowServo() {
#if WINDOW_SERVO_POSITION_MODE
  windowServoRunning = false;
  windowServoDirection = 0;
#else
  ServoWriteUs(SERVO_STOP_US);
  windowServoRunning = false;
  windowServoDirection = 0;
#endif
}

void initWindowServo() {
  ledcAttach(CHUANGHU_SERVO_PIN, 50, 16);
#if WINDOW_SERVO_POSITION_MODE
  ServoWriteUs(map(WINDOW_SERVO_CLOSE_ANGLE, 0, 180, WINDOW_SERVO_MIN_US, WINDOW_SERVO_MAX_US));
#else
  stopWindowServo();
#endif
  actionStatus.windowState = 0;
  actionStatus.windowPercent = 0;
  Serial.println("Window servo init OK.");
}

void setWindowServoTarget(int percent) {
  percent = XianZhi(percent, 0, 100);
  windowServoTargetPercent = percent;

#if WINDOW_SERVO_POSITION_MODE
  int angle = map(percent, 0, 100, WINDOW_SERVO_CLOSE_ANGLE, WINDOW_SERVO_OPEN_ANGLE);
  angle = XianZhi(angle, 0, 180);
  int us = map(angle, 0, 180, WINDOW_SERVO_MIN_US, WINDOW_SERVO_MAX_US);
  us = XianZhi(us, WINDOW_SERVO_MIN_US, WINDOW_SERVO_MAX_US);
  ServoWriteUs(us);
  windowServoCurrentPercent = percent;
  windowServoRunning = false;
  windowServoDirection = 0;
  actionStatus.windowState = percent > 0 ? 1 : 0;
  actionStatus.windowPercent = percent;
  Serial.print("[WindowServo] position percent=");
  Serial.print(percent);
  Serial.print(" angle=");
  Serial.print(angle);
  Serial.print(" us=");
  Serial.println(us);
#else
  int cha = percent - windowServoCurrentPercent;
  if (cha == 0) {
    stopWindowServo();
    return;
  }
  windowServoDirection = cha > 0 ? 1 : -1;
  windowServoRunMs = abs(cha) * WINDOW_FULL_TIME / 100UL;
  if (windowServoRunMs < 200) windowServoRunMs = 200;
  windowServoStartMs = millis();
  windowServoRunning = true;
  if (windowServoDirection > 0) ServoWriteUs(SERVO_OPEN_US);
  else ServoWriteUs(SERVO_CLOSE_US);
#endif
}

void GengXinWindow() {
#if !WINDOW_SERVO_POSITION_MODE
  if (windowServoRunning) {
    if (millis() - windowServoStartMs >= windowServoRunMs) {
      windowServoCurrentPercent = windowServoTargetPercent;
      stopWindowServo();
    }
  }
  actionStatus.windowState = windowServoCurrentPercent > 0 ? 1 : 0;
  actionStatus.windowPercent = windowServoCurrentPercent;
#endif
}

void YingYongWindow(ControlCommandPacket m) {
  // 窗户动作只控制舵机，不允许影响灯光、窗帘或继电器。
  if (m.windowServoCmd == 1) {
    setWindowServoTarget(100);
  } else if (m.windowServoCmd == 2) {
    setWindowServoTarget(0);
  } else if (m.windowServoCmd == 3) {
    setWindowServoTarget(m.windowPercent);
  } else if (m.windowServoCmd == 0) {
    stopWindowServo();
  }
}


// =====================================================
// 18. 红外空调控制预留
// =====================================================

int IrAirPower = 0;
int IrAirMode = 1;
int IrAirTemp = 26;
int IrAirFan = 0;

void InitHongWai() {
  Serial1.begin(IR_BAUD, SERIAL_8N1, IR_RX_PIN, IR_TX_PIN);
  Serial.println("HongWai UART init OK.");
}

void HongWaiFaSongText(const char *msg) {
  Serial.print("[HongWai TX] ");
  Serial.println(msg);
  Serial1.println(msg);
}

void sendAirRelayState() {
  Serial.println("---------- IR Air State ----------");
  Serial.print("Power: "); Serial.println(IrAirPower);
  Serial.print("Mode : "); Serial.println(IrAirMode);
  Serial.print("Temp : "); Serial.println(IrAirTemp);
  Serial.print("Fan  : "); Serial.println(IrAirFan);
  Serial.println("----------------------------------");

  HongWaiFaSongText("IR_AIR_SEND_FULL_PLACEHOLDER");
}

void YingYongHongWai(ControlCommandPacket m) {
  if (m.irAirPower == 0 || m.irAirPower == 1) IrAirPower = m.irAirPower;
  if (m.irAirMode >= 0) IrAirMode = m.irAirMode;
  if (m.irAirTemp >= 16 && m.irAirTemp <= 30) IrAirTemp = m.irAirTemp;
  if (m.irAirFan >= 0) IrAirFan = m.irAirFan;

  switch (m.irAirCmd) {
    case IR_AIR_POWER:
      IrAirPower = !IrAirPower;
      HongWaiFaSongText("AIR_POWER");
      break;

    case IR_AIR_TEMP_UP:
      IrAirTemp = XianZhi(IrAirTemp + 1, 16, 30);
      HongWaiFaSongText("AIR_TEMP_UP");
      break;

    case IR_AIR_TEMP_DOWN:
      IrAirTemp = XianZhi(IrAirTemp - 1, 16, 30);
      HongWaiFaSongText("AIR_TEMP_DOWN");
      break;

    case IR_AIR_MODE:
      HongWaiFaSongText("AIR_MODE");
      break;

    case IR_AIR_FAN:
      HongWaiFaSongText("AIR_FAN");
      break;

    case IR_AIR_SEND_FULL:
      sendAirRelayState();
      break;

    default:
      break;
  }

  actionStatus.irAirPower = IrAirPower;
  actionStatus.irAirMode = IrAirMode;
  actionStatus.irAirTemp = IrAirTemp;
  actionStatus.irAirFan = IrAirFan;
}


// =====================================================
// 19. 场景执行
// =====================================================

void YingYongChangJing(ControlCommandPacket m) {
  // 场景灯效规则：只有回家、离家、报警允许动态；报警保留红蓝闪。
  // 观影、睡眠、办公、舒适、起夜全部只设置单一颜色和亮度。
  static int lastSceneCmd = -1;
  static unsigned long lastSceneMs = 0;
  unsigned long now = millis();

  if (m.sceneCmd == SCENE_NONE) {
    actionStatus.alarmState = 0;
    return;
  }

  // 同一场景短时间重复下发时忽略，避免场景反复刷设备状态。
  if (m.sceneCmd == lastSceneCmd && now - lastSceneMs < 1200) {
    Serial.println("[Scene] duplicate scene ignored.");
    return;
  }
  lastSceneCmd = m.sceneCmd;
  lastSceneMs = now;

  if (m.sceneCmd == SCENE_HOME) {
    setLightMode(livingLightStrip, true, 170, LIGHT_MODE_WELCOME, 255, 190, 90);
    setLightMode(bedroomLightStrip, true, 120, LIGHT_MODE_WELCOME, 255, 180, 95);
    actionStatus.livingLightState = 1;
    actionStatus.bedroomLightState = 1;
    actionStatus.livingLightLevel = 170;
    actionStatus.bedroomLightLevel = 120;
    actionStatus.livingLightMode = LIGHT_MODE_WELCOME;
    actionStatus.bedroomLightMode = LIGHT_MODE_WELCOME;
    actionStatus.alarmState = 0;
    Serial.println("Scene HOME: welcome dynamic light.");
  }

  else if (m.sceneCmd == SCENE_AWAY) {
    setLightMode(livingLightStrip, true, 100, LIGHT_MODE_SENDOFF, 255, 160, 60);
    setLightMode(bedroomLightStrip, true, 70, LIGHT_MODE_SENDOFF, 255, 150, 50);
    actionStatus.livingLightState = 1;
    actionStatus.bedroomLightState = 1;
    actionStatus.livingLightLevel = 100;
    actionStatus.bedroomLightLevel = 70;
    actionStatus.livingLightMode = LIGHT_MODE_SENDOFF;
    actionStatus.bedroomLightMode = LIGHT_MODE_SENDOFF;
    actionStatus.alarmState = 0;
    Serial.println("Scene AWAY: send-off dynamic light.");
  }

  else if (m.sceneCmd == SCENE_MOVIE) {
    setLightMode(livingLightStrip, true, 45, LIGHT_MODE_NORMAL, 45, 35, 120);
    setLightMode(bedroomLightStrip, false, 0, LIGHT_MODE_NORMAL, 0, 0, 0);
    actionStatus.livingLightState = 1;
    actionStatus.bedroomLightState = 0;
    actionStatus.livingLightLevel = 45;
    actionStatus.bedroomLightLevel = 0;
    actionStatus.livingLightMode = LIGHT_MODE_NORMAL;
    actionStatus.bedroomLightMode = LIGHT_MODE_NORMAL;
    actionStatus.alarmState = 0;
    Serial.println("Scene MOVIE: static blue-purple.");
  }

  else if (m.sceneCmd == SCENE_SLEEP) {
    setLightMode(livingLightStrip, false, 0, LIGHT_MODE_NORMAL, 0, 0, 0);
    setLightMode(bedroomLightStrip, true, 18, LIGHT_MODE_NORMAL, 255, 120, 35);
    actionStatus.livingLightState = 0;
    actionStatus.bedroomLightState = 1;
    actionStatus.livingLightLevel = 0;
    actionStatus.bedroomLightLevel = 18;
    actionStatus.livingLightMode = LIGHT_MODE_NORMAL;
    actionStatus.bedroomLightMode = LIGHT_MODE_NORMAL;
    actionStatus.alarmState = 0;
    Serial.println("Scene SLEEP: static amber low light.");
  }

  else if (m.sceneCmd == SCENE_OFFICE || m.sceneCmd == SCENE_READING) {
    setLightMode(livingLightStrip, true, 210, LIGHT_MODE_NORMAL, 210, 245, 255);
    setLightMode(bedroomLightStrip, true, 190, LIGHT_MODE_NORMAL, 210, 245, 255);
    actionStatus.livingLightState = 1;
    actionStatus.bedroomLightState = 1;
    actionStatus.livingLightLevel = 210;
    actionStatus.bedroomLightLevel = 190;
    actionStatus.livingLightMode = LIGHT_MODE_NORMAL;
    actionStatus.bedroomLightMode = LIGHT_MODE_NORMAL;
    actionStatus.alarmState = 0;
    Serial.println("Scene OFFICE/READING: static focus light.");
  }

  else if (m.sceneCmd == SCENE_COMFORT || m.sceneCmd == SCENE_QUIET) {
    setLightMode(livingLightStrip, true, 130, LIGHT_MODE_NORMAL, 255, 175, 95);
    setLightMode(bedroomLightStrip, true, 110, LIGHT_MODE_NORMAL, 255, 165, 85);
    actionStatus.livingLightState = 1;
    actionStatus.bedroomLightState = 1;
    actionStatus.livingLightLevel = 130;
    actionStatus.bedroomLightLevel = 110;
    actionStatus.livingLightMode = LIGHT_MODE_NORMAL;
    actionStatus.bedroomLightMode = LIGHT_MODE_NORMAL;
    actionStatus.alarmState = 0;
    Serial.println("Scene COMFORT/QUIET: static warm orange.");
  }

  else if (m.sceneCmd == SCENE_NIGHT) {
    setLightMode(livingLightStrip, true, 12, LIGHT_MODE_NORMAL, 255, 95, 20);
    setLightMode(bedroomLightStrip, true, 10, LIGHT_MODE_NORMAL, 255, 95, 20);
    actionStatus.livingLightState = 1;
    actionStatus.bedroomLightState = 1;
    actionStatus.livingLightLevel = 12;
    actionStatus.bedroomLightLevel = 10;
    actionStatus.livingLightMode = LIGHT_MODE_NORMAL;
    actionStatus.bedroomLightMode = LIGHT_MODE_NORMAL;
    actionStatus.alarmState = 0;
    Serial.println("Scene NIGHT: static very dim amber.");
  }

  else if (m.sceneCmd == SCENE_ALARM || m.buzzerCmd == 1) {
    setLightMode(livingLightStrip, true, 220, LIGHT_MODE_ALERT, 255, 0, 0);
    setLightMode(bedroomLightStrip, true, 220, LIGHT_MODE_ALERT, 0, 45, 255);
    actionStatus.livingLightState = 1;
    actionStatus.bedroomLightState = 1;
    actionStatus.livingLightLevel = 220;
    actionStatus.bedroomLightLevel = 220;
    actionStatus.livingLightMode = LIGHT_MODE_ALERT;
    actionStatus.bedroomLightMode = LIGHT_MODE_ALERT;
    actionStatus.alarmState = 1;
    Serial.println("Scene ALARM: red-blue flash.");
  }
}


// =====================================================
// 20. 命令转换：V2 / Old 转 Ultra
// =====================================================

ControlCommandPacket createDefaultCommandPacket() {
  ControlCommandPacket m;
  memset(&m, 0, sizeof(m));

  m.commandSource = SRC_WEB;
  m.commandPriority = 0;

  m.livingLightLevel = 140;
  m.bedroomLightLevel = 120;
  m.livingLightMode = LIGHT_MODE_NORMAL;
  m.bedroomLightMode = LIGHT_MODE_NORMAL;
  m.globalLightMode = 0;

  m.livingLightR = 255;
  m.livingLightG = 180;
  m.livingLightB = 90;

  m.bedroomLightR = 220;
  m.bedroomLightG = 220;
  m.bedroomLightB = 200;

  m.irAirPower = IrAirPower;
  m.irAirMode = IrAirMode;
  m.irAirTemp = IrAirTemp;
  m.irAirFan = IrAirFan;

  return m;
}

ControlCommandPacket convertV2ToCommandPacket(ControlCommandPacketV2 v) {
  ControlCommandPacket m = createDefaultCommandPacket();

  m.cmdSeq = v.cmdSeq;
  m.livingLightCmd = v.livingLightCmd;
  m.bedroomLightCmd = v.bedroomLightCmd;

  if (v.lightLevel > 0) {
    m.livingLightLevel = v.lightLevel;
    m.bedroomLightLevel = v.lightLevel;
  }

  m.airRelayCmd = v.airRelayCmd;
  m.tvRelayCmd = v.tvRelayCmd;
  m.fridgeRelayCmd = v.fridgeRelayCmd;
  m.waterRelayCmd = v.waterRelayCmd;

  m.livingCurtainCmd = v.livingCurtainCmd;
  m.bedroomCurtainCmd = v.bedroomCurtainCmd;

  m.windowServoCmd = v.windowServoCmd;
  m.irAirCmd = v.irAirCmd;

  m.buzzerCmd = v.buzzerCmd;
  m.sceneCmd = v.sceneCmd;

  return m;
}

ControlCommandPacket convertOldToCommandPacket(ControlCommandPacketOld o) {
  ControlCommandPacket m = createDefaultCommandPacket();

  m.livingLightCmd = o.lightCmd == 0 ? 0 : 1;
  m.bedroomLightCmd = o.lightCmd == 0 ? 0 : 1;
  m.livingLightLevel = o.lightLevel;
  m.bedroomLightLevel = o.lightLevel;

  m.airRelayCmd = o.relayCmd;
  m.tvRelayCmd = o.relayCmd;
  m.fridgeRelayCmd = o.relayCmd;
  m.waterRelayCmd = o.relayCmd;

  m.windowServoCmd = o.servoCmd;
  m.livingCurtainCmd = o.motorCmd;
  m.bedroomCurtainCmd = o.motorCmd;

  m.buzzerCmd = o.buzzerCmd;
  m.sceneCmd = o.sceneCmd;

  return m;
}


// =====================================================
// 21. 命令打印与执行
// =====================================================

void printCommandPacket(ControlCommandPacket m) {
  Serial.println();
  Serial.println("========== Xin ControlCommandPacket ==========");
  Serial.print("cmdSeq: "); Serial.println(m.cmdSeq);
  Serial.print("source: "); Serial.println(LaiYuanMingCheng(m.commandSource));

  Serial.print("LivingLight: "); Serial.print(m.livingLightCmd);
  Serial.print(" Level: "); Serial.print(m.livingLightLevel);
  Serial.print(" Mode: "); Serial.println(m.livingLightMode);

  Serial.print("BedroomLight : "); Serial.print(m.bedroomLightCmd);
  Serial.print(" Level: "); Serial.print(m.bedroomLightLevel);
  Serial.print(" Mode: "); Serial.println(m.bedroomLightMode);

  Serial.print("Air Relay: "); Serial.println(m.airRelayCmd);
  Serial.print("TV Relay : "); Serial.println(m.tvRelayCmd);
  Serial.print("Fridge   : "); Serial.println(m.fridgeRelayCmd);
  Serial.print("Water    : "); Serial.println(m.waterRelayCmd);

  Serial.print("KT Curtain Cmd: "); Serial.print(m.livingCurtainCmd);
  Serial.print(" Percent: "); Serial.println(m.livingCurtainPercent);

  Serial.print("WS Curtain Cmd: "); Serial.print(m.bedroomCurtainCmd);
  Serial.print(" Percent: "); Serial.println(m.bedroomCurtainPercent);

  Serial.print("Window Cmd: "); Serial.print(m.windowServoCmd);
  Serial.print(" Percent: "); Serial.println(m.windowPercent);

  Serial.print("IR Air Cmd: "); Serial.println(m.irAirCmd);
  Serial.print("Scene Cmd : "); Serial.println(m.sceneCmd);
  Serial.println("=========================================");
}

void handleCommandPacket(ControlCommandPacket m) {
  ZuiHouShouDaoTime = millis();

  if (m.cmdSeq != 0 && m.cmdSeq == actionStatus.lastCmdSeq) {
    return;
  }

  actionStatus.lastCmdSeq = m.cmdSeq;
  actionStatus.lastCommandSource = m.commandSource;

  currentCommandPacket = m;

  printCommandPacket(m);

  YingYongJiDianQi(m);
  YingYongDengGuang(m);
  YingYongChuangLian(m);
  YingYongWindow(m);
  YingYongHongWai(m);
  YingYongChangJing(m);

  sendActionBoardState(true);
}


// =====================================================
// 22. ESP-NOW 回调
// =====================================================

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len)
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
#endif
{
  ZuiHouShouDaoTime = millis();

  if (len == sizeof(ControlCommandPacket)) {
    ControlCommandPacket m;
    memcpy(&m, incomingData, sizeof(m));
    Serial.println("[ESP-NOW] Ultra command received.");
    handleCommandPacket(m);
  }

  else if (len == sizeof(ControlCommandPacketV2)) {
    ControlCommandPacketV2 v;
    memcpy(&v, incomingData, sizeof(v));
    Serial.println("[ESP-NOW] V2 command received, convert to Ultra.");
    handleCommandPacket(convertV2ToCommandPacket(v));
  }

  else if (len == sizeof(ControlCommandPacketOld)) {
    ControlCommandPacketOld o;
    memcpy(&o, incomingData, sizeof(o));
    Serial.println("[ESP-NOW] Old command received, convert to Ultra.");
    handleCommandPacket(convertOldToCommandPacket(o));
  }

  else {
    Serial.print("[ESP-NOW] Unknown length: ");
    Serial.println(len);
    Serial.print("Ultra expected: ");
    Serial.println(sizeof(ControlCommandPacket));
    Serial.print("V2 expected: ");
    Serial.println(sizeof(ControlCommandPacketV2));
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status)
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
#endif
{
  Serial.print("[ESP-NOW Status Send] ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}


// =====================================================
// 23. 执行层状态回传
// =====================================================

void sendActionBoardState(bool forceSend) {
  unsigned long now = millis();

  if (!forceSend && now - lastStatusSendMs < STATUS_SEND_INTERVAL) {
    return;
  }

  lastStatusSendMs = now;

  actionStatus.statusSeq = statusSeqCounter++;
  actionStatus.uptimeMs = millis();
  actionStatus.executorOnline = 1;

  esp_err_t result = esp_now_send(
    gatewayMac,
    (uint8_t *)&actionStatus,
    sizeof(actionStatus)
  );

  if (result == ESP_OK) {
    Serial.println("[ActionBoardState] Send request OK");
  } else {
    Serial.println("[ActionBoardState] Send request failed");
  }
}


// =====================================================
// 24A. V3.3 节点配网 + HTTP 节点注册与状态上报
// =====================================================

String LingxiJsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", "\\n");
  s.replace("\r", "\\r");
  return s;
}


String LingxiHtmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

String LingxiMacSuffix() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  if (mac.length() >= 4) return mac.substring(mac.length() - 4);
  return "NODE";
}

void LingxiLoadNodeConfig() {
  LingxiNodePrefs.begin("lx_node", true);
  LingxiWifiSsid = LingxiNodePrefs.getString("ssid", "");
  LingxiWifiPassword = LingxiNodePrefs.getString("pass", "");
  LingxiCenterHost = LingxiNodePrefs.getString("host", LINGXI_DEFAULT_CENTER_HOST);
  LingxiNodePrefs.end();

  LingxiHasSavedWifiConfig = (LingxiWifiSsid.length() > 0);
  if (!LingxiHasSavedWifiConfig) {
    LingxiWifiSsid = LINGXI_DEFAULT_WIFI_SSID;
    LingxiWifiPassword = LINGXI_DEFAULT_WIFI_PASSWORD;
    LingxiCenterHost = LINGXI_DEFAULT_CENTER_HOST;
  }

  Serial.print("[V1.4.2 Config] saved_config=");
  Serial.println(LingxiHasSavedWifiConfig ? "yes" : "no, use demo default");
  Serial.print("[V1.4.2 Config] ssid=");
  Serial.println(LingxiWifiSsid);
  Serial.print("[V1.4.2 Config] gateway=");
  Serial.println(LingxiCenterHost);
}

void LingxiSendConfigPage() {
  String html;
  html.reserve(7000);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no'>";
  html += "<title>灵汐全屋智能执行中心配网模式</title>";
  html += "<style>";
  html += "*{box-sizing:border-box}body{margin:0;min-height:100vh;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','Microsoft YaHei',sans-serif;color:#123047;background:linear-gradient(135deg,#eaf5ff,#ffffff);padding:18px}.shell{max-width:620px;margin:0 auto}.card{background:rgba(255,255,255,.94);border:1px solid #d7e9ff;border-radius:24px;box-shadow:0 18px 45px rgba(28,100,180,.16);padding:24px}h1{font-size:24px;line-height:1.35;margin:0 0 8px;color:#1268c4}.sub{font-size:14px;color:#5f7896;line-height:1.75;margin:0 0 16px}.badge{display:inline-block;background:#edf7ff;color:#1769aa;border:1px solid #bfe0ff;border-radius:999px;padding:6px 10px;margin:4px 4px 12px 0;font-size:12px;font-weight:800}.panel{margin-top:12px;background:#f4faff;border:1px solid #dcecff;border-radius:16px;padding:12px}.kv{display:grid;grid-template-columns:86px 1fr;gap:7px 10px;color:#31516f;font-size:13px;line-height:1.6}.kv b{color:#1769aa}.field{margin-top:14px}label{display:block;font-weight:800;color:#31516f;margin-bottom:8px}input{width:100%;border:1px solid #c8def4;background:#fbfdff;color:#0e2033;border-radius:14px;padding:13px 14px;font-size:16px;outline:none}input:focus{border-color:#2f8df5;box-shadow:0 0 0 4px rgba(47,141,245,.12)}.hint{font-size:12px;color:#7890a8;margin-top:6px;line-height:1.6}.btn{width:100%;border:0;border-radius:15px;padding:14px 16px;margin-top:18px;font-size:16px;font-weight:900;color:white;background:linear-gradient(135deg,#2f8df5,#44b7ff);box-shadow:0 12px 28px rgba(47,141,245,.22)}.danger{background:linear-gradient(135deg,#ff8b8b,#ef4444)}.foot{font-size:12px;color:#7890a8;text-align:center;margin-top:14px;line-height:1.7}";
  html += "</style></head><body><div class='shell'><div class='card'>";
  html += "<h1>灵汐全屋智能执行中心配网模式</h1>";
  html += "<p class='sub'>执行层集成中心：灯光、窗帘、窗户舵机、空调/电视/冰箱/热水器电源继电器。</p>";
  html += "<span class='badge'>" + LingxiHtmlEscape(String(LINGXI_ACTION_NODE_ID)) + "</span><span class='badge'>公测版 V1.4.2</span>";
  html += "<div class='panel'><div class='kv'><b>MAC</b><span>" + LingxiHtmlEscape(WiFi.macAddress()) + "</span><b>中控 IP</b><span>" + LingxiHtmlEscape(LingxiCenterHost) + "</span><b>节点热点</b><span>" + LingxiHtmlEscape(LingxiConfigApSsid) + "</span><b>固件</b><span>" + LingxiHtmlEscape(String(LINGXI_ACTION_NODE_FW)) + "</span></div></div>";
  html += "<form method='POST' action='/save'>";
  html += "<div class='field'><label>WiFi 名称 / 中控热点名称</label><input name='ssid' value='" + LingxiHtmlEscape(LingxiWifiSsid) + "' placeholder='例如：Lingxi-Gateway' required><div class='hint'>默认连接中控热点；也可以填写路由器或手机热点名称。</div></div>";
  html += "<div class='field'><label>WiFi 密码</label><input name='pass' type='password' value='" + LingxiHtmlEscape(LingxiWifiPassword) + "' placeholder='请输入 WiFi 密码'></div>";
  html += "<div class='field'><label>中控 IP 地址</label><input name='host' value='" + LingxiHtmlEscape(LingxiCenterHost) + "' placeholder='192.168.4.1' required><div class='hint'>中控热点模式通常为 192.168.4.1；路由器模式填写中控的 WifiIp。</div></div>";
  html += "<button class='btn' type='submit'>保存配置并重启节点</button></form>";
  html += "<form method='POST' action='/reset'><button class='btn danger' type='submit'>清除配置并重启</button></form>";
  html += "<div class='foot'>配网热点密码：999999999。配置成功并完成中控注册/上报后，热点会自动关闭。</div>";
  html += "</div></div></body></html>";
  LingxiConfigServer.send(200, "text/html; charset=utf-8", html);
}

void LingxiSaveNodeConfig() {
  String ssid = LingxiConfigServer.arg("ssid");
  String pass = LingxiConfigServer.arg("pass");
  String host = LingxiConfigServer.arg("host");
  ssid.trim();
  host.trim();
  if (ssid.length() == 0 || host.length() == 0) {
    LingxiConfigServer.send(400, "text/plain; charset=utf-8", "SSID 和中控 IP 不能为空");
    return;
  }
  LingxiNodePrefs.begin("lx_node", false);
  LingxiNodePrefs.putString("ssid", ssid);
  LingxiNodePrefs.putString("pass", pass);
  LingxiNodePrefs.putString("host", host);
  LingxiNodePrefs.end();
  LingxiConfigServer.send(200, "text/html; charset=utf-8", "<meta charset='utf-8'><h2>已保存，节点正在重启...</h2><p>请稍后回到中控 /nodes 页面查看节点在线状态。</p>");
  delay(800);
  ESP.restart();
}

void LingxiResetNodeConfig() {
  LingxiNodePrefs.begin("lx_node", false);
  LingxiNodePrefs.clear();
  LingxiNodePrefs.end();
  LingxiConfigServer.send(200, "text/html; charset=utf-8", "<meta charset='utf-8'><h2>配置已清除，节点正在重启...</h2>");
  delay(800);
  ESP.restart();
}

void LingxiSendConfigStatus() {
  String json = "{";
  json += "\"node_id\":\"" + LingxiJsonEscape(String(LINGXI_ACTION_NODE_ID)) + "\",";
  json += "\"mac\":\"" + LingxiJsonEscape(WiFi.macAddress()) + "\",";
  json += "\"ap_ssid\":\"" + LingxiJsonEscape(LingxiConfigApSsid) + "\",";
  json += "\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"gateway_host\":\"" + LingxiJsonEscape(LingxiCenterHost) + "\"";
  json += "}";
  LingxiConfigServer.send(200, "application/json; charset=utf-8", json);
}

void LingxiStartConfigPortal(const char* reason) {
  if (LingxiConfigPortalRunning) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 8, 1), IPAddress(192, 168, 8, 1), IPAddress(255, 255, 255, 0));
  LingxiConfigApSsid = "灵汐-执行层集成中心";
  bool apOk = WiFi.softAP(LingxiConfigApSsid.c_str(), LINGXI_CONFIG_AP_PASSWORD);
  LingxiConfigServer.on("/", HTTP_GET, LingxiSendConfigPage);
  LingxiConfigServer.on("/generate_204", HTTP_GET, LingxiSendConfigPage);
  LingxiConfigServer.on("/fwlink", HTTP_GET, LingxiSendConfigPage);
  LingxiConfigServer.on("/hotspot-detect.html", HTTP_GET, LingxiSendConfigPage);
  LingxiConfigServer.on("/save", HTTP_POST, LingxiSaveNodeConfig);
  LingxiConfigServer.on("/save", HTTP_GET, LingxiSendConfigPage);
  LingxiConfigServer.on("/reset", HTTP_POST, LingxiResetNodeConfig);
  LingxiConfigServer.on("/status", HTTP_GET, LingxiSendConfigStatus);
  LingxiConfigServer.onNotFound(LingxiSendConfigPage);
  LingxiDnsServer.start(LINGXI_DNS_PORT, "*", IPAddress(192, 168, 8, 1));
  LingxiConfigServer.begin();
  LingxiConfigPortalRunning = true;
  Serial.print("[V1.4.2 Config] portal started, reason=");
  Serial.println(reason);
  Serial.print("[V1.4.2 Config] AP SSID=");
  Serial.println(LingxiConfigApSsid);
  Serial.print("[V1.4.2 Config] AP PASS=");
  Serial.println(LINGXI_CONFIG_AP_PASSWORD);
  Serial.print("[V1.4.2 Config] AP IP=");
  Serial.println(WiFi.softAPIP());
  if (!apOk) Serial.println("[V1.4.2 Config] softAP start failed");
}

void LingxiConfigPortalTick() {
  if (LingxiConfigPortalRunning) {
    LingxiDnsServer.processNextRequest();
    LingxiConfigServer.handleClient();
  }
}


void LingxiStopConfigPortal(const char* reason) {
  if (!LingxiConfigPortalRunning) return;
  Serial.print("[V1.4.2 Config] closing config portal, reason=");
  Serial.println(reason);
  WiFi.softAPdisconnect(true);
  LingxiConfigPortalRunning = false;
  LingxiConfigApSsid = "";
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
  }
}


bool LingxiNodeWifiConnect(unsigned long timeoutMs) {
  if (WiFi.status() == WL_CONNECTED) {
    LingxiNodeWifiOk = true;
    return true;
  }

  Serial.println();
  Serial.println("[V1.4.2 HTTP] Connecting to Lingxi gateway WiFi...");
  Serial.print("[V1.4.2 HTTP] SSID: ");
  Serial.println(LingxiWifiSsid);

  if (!LingxiHasSavedWifiConfig) {
    LingxiStartConfigPortal("first_boot_no_saved_config");
  }

  WiFi.mode(LingxiConfigPortalRunning ? WIFI_AP_STA : WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(LingxiWifiSsid.c_str(), LingxiWifiPassword.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  LingxiNodeWifiOk = (WiFi.status() == WL_CONNECTED);
  if (LingxiNodeWifiOk) {
    Serial.print("[V1.4.2 HTTP] WiFi connected. IP=");
    Serial.println(WiFi.localIP());
    Serial.print("[V1.4.2 HTTP] MAC=");
    Serial.println(WiFi.macAddress());
    Serial.print("[V1.4.2 HTTP] Channel=");
    Serial.println(WiFi.channel());
  } else {
    Serial.println("[V1.4.2 HTTP] WiFi connect failed. Keep ESP-NOW fallback mode.");
    LingxiStartConfigPortal("wifi_connect_failed");
  }

  return LingxiNodeWifiOk;
}

String LingxiNodeEndpointsJson() {
  String json = "[";
  json += "{\"id\":\"living_light\",\"name\":\"客厅灯\",\"type\":\"action\",\"capability\":\"switch\",\"room\":\"客厅\"},";
  json += "{\"id\":\"bedroom_light\",\"name\":\"卧室灯\",\"type\":\"action\",\"capability\":\"switch\",\"room\":\"卧室\"},";
  json += "{\"id\":\"living_curtain\",\"name\":\"客厅窗帘\",\"type\":\"action\",\"capability\":\"curtain\",\"room\":\"客厅\"},";
  json += "{\"id\":\"bedroom_curtain\",\"name\":\"卧室窗帘\",\"type\":\"action\",\"capability\":\"curtain\",\"room\":\"卧室\"},";
  json += "{\"id\":\"window_servo\",\"name\":\"窗户舵机\",\"type\":\"action\",\"capability\":\"window\",\"room\":\"卧室\"},";
  json += "{\"id\":\"air_relay\",\"name\":\"空调电源\",\"type\":\"action\",\"capability\":\"power\",\"room\":\"客厅\"},";
  json += "{\"id\":\"tv_relay\",\"name\":\"电视电源\",\"type\":\"action\",\"capability\":\"power\",\"room\":\"客厅\"},";
  json += "{\"id\":\"fridge_relay\",\"name\":\"冰箱电源\",\"type\":\"action\",\"capability\":\"power\",\"room\":\"厨房\"},";
  json += "{\"id\":\"water_heater_relay\",\"name\":\"热水器电源\",\"type\":\"action\",\"capability\":\"power\",\"room\":\"卫生间\"}";
  json += "]";
  return json;
}

String LingxiNodeRegisterJson() {
  String json = "{";
  json += "\"node_id\":\"" LINGXI_ACTION_NODE_ID "\",";
  json += "\"name\":\"" LINGXI_ACTION_NODE_NAME "\",";
  json += "\"display_name\":\"" LINGXI_ACTION_NODE_NAME "\",";
  json += "\"node_type\":\"" LINGXI_ACTION_NODE_TYPE "\",";
  json += "\"role\":\"" LINGXI_ACTION_NODE_ROLE "\",";
  json += "\"transport\":\"wifi_http+esp_now\",";
  json += "\"firmware\":\"" LINGXI_ACTION_NODE_FW "\",";
  json += "\"mac\":\"" + LingxiJsonEscape(WiFi.macAddress()) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"status\":\"online\",";
  json += "\"endpoint_count\":9,";
  json += "\"endpoints\":" + LingxiNodeEndpointsJson();
  json += "}";
  return json;
}

String LingxiNodeReportJson() {
  LingxiHttpReportSeq++;

  String json = "{";
  json += "\"node_id\":\"" LINGXI_ACTION_NODE_ID "\",";
  json += "\"name\":\"" LINGXI_ACTION_NODE_NAME "\",";
  json += "\"node_type\":\"" LINGXI_ACTION_NODE_TYPE "\",";
  json += "\"role\":\"" LINGXI_ACTION_NODE_ROLE "\",";
  json += "\"transport\":\"wifi_http+esp_now\",";
  json += "\"mac\":\"" + LingxiJsonEscape(WiFi.macAddress()) + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"status\":\"online\",";
  json += "\"report_seq\":" + String(LingxiHttpReportSeq) + ",";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"last_cmd_seq\":" + String(actionStatus.lastCmdSeq) + ",";
  json += "\"executor_online\":1,";
  json += "\"data\":{";
  json += "\"living_light\":" + String(actionStatus.livingLightState) + ",";
  json += "\"bedroom_light\":" + String(actionStatus.bedroomLightState) + ",";
  json += "\"living_light_level\":" + String(actionStatus.livingLightLevel) + ",";
  json += "\"bedroom_light_level\":" + String(actionStatus.bedroomLightLevel) + ",";
  json += "\"living_curtain\":" + String(actionStatus.livingCurtainState) + ",";
  json += "\"bedroom_curtain\":" + String(actionStatus.bedroomCurtainState) + ",";
  json += "\"living_curtain_percent\":" + String(actionStatus.livingCurtainPercent) + ",";
  json += "\"bedroom_curtain_percent\":" + String(actionStatus.bedroomCurtainPercent) + ",";
  json += "\"window_servo\":" + String(actionStatus.windowState) + ",";
  json += "\"window_percent\":" + String(actionStatus.windowPercent) + ",";
  json += "\"air_relay\":" + String(actionStatus.airRelayState) + ",";
  json += "\"tv_relay\":" + String(actionStatus.tvRelayState) + ",";
  json += "\"fridge_relay\":" + String(actionStatus.fridgeRelayState) + ",";
  json += "\"water_heater_relay\":" + String(actionStatus.waterRelayState) + ",";
  json += "\"ir_air_power\":" + String(actionStatus.irAirPower) + ",";
  json += "\"ir_air_mode\":" + String(actionStatus.irAirMode) + ",";
  json += "\"ir_air_temp\":" + String(actionStatus.irAirTemp) + ",";
  json += "\"ir_air_fan\":" + String(actionStatus.irAirFan);
  json += "},";
  json += "\"endpoints\":" + LingxiNodeEndpointsJson();
  json += "}";
  return json;
}

bool LingxiNodeHttpPost(const char* path, const String &payload, const char* tag) {
  if (WiFi.status() != WL_CONNECTED) {
    LingxiNodeWifiOk = false;
    Serial.print("[V1.4.2 HTTP] ");
    Serial.print(tag);
    Serial.println(" skipped: WiFi offline.");
    return false;
  }

  String url = "http://";
  url += LingxiCenterHost;
  url += ":";
  url += String(LINGXI_CENTER_PORT);
  url += path;

  HTTPClient http;
  http.setTimeout(2000);
  if (!http.begin(url)) {
    Serial.print("[V1.4.2 HTTP] ");
    Serial.print(tag);
    Serial.println(" begin failed.");
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  String resp = http.getString();
  http.end();

  Serial.print("[V1.4.2 HTTP] ");
  Serial.print(tag);
  Serial.print(" code=");
  Serial.print(code);
  if (resp.length()) {
    Serial.print(" resp=");
    Serial.println(resp.substring(0, 160));
  } else {
    Serial.println();
  }

  return code >= 200 && code < 300;
}

void LingxiNodeHttpRegister(bool forceSend) {
  unsigned long now = millis();
  if (!forceSend && LingxiNodeRegistered && now - LingxiLastRegisterMs < LINGXI_REGISTER_INTERVAL_MS) return;

  if (WiFi.status() != WL_CONNECTED) {
    LingxiNodeWifiConnect(2500);
  }
  if (WiFi.status() != WL_CONNECTED) return;

  LingxiLastRegisterMs = now;
  bool ok = LingxiNodeHttpPost("/api/node/register", LingxiNodeRegisterJson(), "register");
  if (ok) LingxiNodeRegistered = true;
}

void LingxiNodeHttpReport(bool forceSend) {
  unsigned long now = millis();
  if (!forceSend && now - LingxiLastHttpReportMs < LINGXI_HTTP_REPORT_INTERVAL_MS) return;
  LingxiLastHttpReportMs = now;

  if (WiFi.status() != WL_CONNECTED) {
    LingxiNodeWifiOk = false;
    return;
  }

  if (!LingxiNodeRegistered) {
    LingxiNodeHttpRegister(true);
  }

  bool reportOk = LingxiNodeHttpPost("/api/node/report", LingxiNodeReportJson(), "report");
#if CONFIG_PORTAL_CLOSE_AFTER_REPORT_OK
  if (reportOk && LingxiNodeRegistered && LingxiConfigPortalRunning) {
    LingxiStopConfigPortal("register_and_report_ok");
  }
#endif
}

void InitLingxiNodeHttp() {
  LingxiNodeWifiConnect(6000);
  if (LingxiNodeWifiOk) {
    LingxiNodeHttpRegister(true);
    LingxiNodeHttpReport(true);
  }
}

void LingxiNodeHttpTick() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    LingxiNodeWifiOk = false;
    if (now - LingxiLastWifiCheckMs > LINGXI_WIFI_RETRY_INTERVAL_MS) {
      LingxiLastWifiCheckMs = now;
      LingxiNodeWifiConnect(2500);
    }
    return;
  }

  LingxiNodeWifiOk = true;
  LingxiNodeHttpRegister(false);
  LingxiNodeHttpReport(false);
}

// =====================================================
// 24. ESP-NOW 初始化
// =====================================================

void InitEspNow() {
  WiFi.mode(LingxiConfigPortalRunning ? WIFI_AP_STA : WIFI_STA);

  if (LingxiNodeWifiOk && WiFi.status() == WL_CONNECTED) {
    Serial.print("[ESP-NOW] Keep WiFi STA connected for V3 HTTP. Channel=");
    Serial.println(WiFi.channel());
  } else {
    WiFi.disconnect();
    delay(50);
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    Serial.print("[ESP-NOW] HTTP WiFi offline, fallback channel=");
    Serial.println(ESPNOW_CHANNEL);
  }

  Serial.print("ActionBoard S3 STA MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    EspNowInitOk = false;
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gatewayMac, 6);
  // WiFi 已连接时 channel=0 表示使用当前 STA 信道；未连接时使用固定 ESPNOW_CHANNEL。
  peerInfo.channel = (LingxiNodeWifiOk && WiFi.status() == WL_CONNECTED) ? 0 : ESPNOW_CHANNEL;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);

  if (result == ESP_OK) {
    Serial.println("ZhongKong peer added OK.");
  } else if (result == ESP_ERR_ESPNOW_EXIST) {
    Serial.println("ZhongKong peer already exists.");
  } else {
    Serial.println("ZhongKong peer add failed.");
  }

  EspNowInitOk = true;
  Serial.println("ESP-NOW init OK.");
}


// =====================================================
// 25. 状态打印
// =====================================================

void printActionBoardState() {
  Serial.println();
  Serial.println("========== ActionBoardState ==========");
  Serial.print("statusSeq: "); Serial.println(actionStatus.statusSeq);
  Serial.print("lastCmdSeq: "); Serial.println(actionStatus.lastCmdSeq);
  Serial.print("lastSource: "); Serial.println(LaiYuanMingCheng(actionStatus.lastCommandSource));

  Serial.println("---------- Light ----------");
  Serial.print("LivingLight: "); Serial.print(actionStatus.livingLightState);
  Serial.print(" Level: "); Serial.print(actionStatus.livingLightLevel);
  Serial.print(" Mode: "); Serial.println(actionStatus.livingLightMode);

  Serial.print("BedroomLight : "); Serial.print(actionStatus.bedroomLightState);
  Serial.print(" Level: "); Serial.print(actionStatus.bedroomLightLevel);
  Serial.print(" Mode: "); Serial.println(actionStatus.bedroomLightMode);

  Serial.println("---------- Relay ----------");
  Serial.print("Air   : "); Serial.println(actionStatus.airRelayState);
  Serial.print("TV    : "); Serial.println(actionStatus.tvRelayState);
  Serial.print("Fridge: "); Serial.println(actionStatus.fridgeRelayState);
  Serial.print("Water : "); Serial.println(actionStatus.waterRelayState);

  Serial.println("---------- Curtain ----------");
  Serial.print("Living Curtain Percent: "); Serial.println(actionStatus.livingCurtainPercent);
  Serial.print("Bedroom Curtain Percent : "); Serial.println(actionStatus.bedroomCurtainPercent);

  Serial.println("---------- Window ----------");
  Serial.print("Window Percent: "); Serial.println(actionStatus.windowPercent);

  Serial.println("---------- IR Air ----------");
  Serial.print("Power: "); Serial.println(actionStatus.irAirPower);
  Serial.print("Mode : "); Serial.println(actionStatus.irAirMode);
  Serial.print("Temp : "); Serial.println(actionStatus.irAirTemp);
  Serial.print("Fan  : "); Serial.println(actionStatus.irAirFan);

  Serial.println("=====================================");
}


// =====================================================
// 26. 初始化默认状态
// =====================================================

void initActionState() {
  memset(&currentCommandPacket, 0, sizeof(currentCommandPacket));
  memset(&actionStatus, 0, sizeof(actionStatus));

  actionStatus.executorOnline = 1;
  actionStatus.irAirPower = 0;
  actionStatus.irAirMode = 1;
  actionStatus.irAirTemp = 26;
  actionStatus.irAirFan = 0;

  actionStatus.livingLightLevel = 140;
  actionStatus.bedroomLightLevel = 120;
}


// =====================================================
// 27. setup / loop
// =====================================================

void setup() {
  Serial.begin(921600);
  delay(1000);

  Serial.println();
  Serial.println("Lingxi ActionBoard-01 PublicBeta V1.4.2 ActionLayerNode");
  Serial.println("Executor node: fixed GPIO, config portal, HTTP report, ESP-NOW control, demo-friendly motors.");
  LingxiLoadNodeConfig();

  initActionState();

  statusRgb.begin();
  statusRgb.clear();
  statusRgb.show();
  statusRgb.setBrightness(100);

  InitJiDianQi();
  InitDengGuang();
  InitChuangLian();
  initWindowServo();
  InitHongWai();
  InitLingxiNodeHttp();
  InitEspNow();

  Serial.println("Lingxi ActionLayerNode PublicBeta V1.4.2 started.");
  Serial.println("Waiting commands from ZhongKong...");
  printSerialDemoHelp();
}

void loop() {
  updateStatusLight();
  GengXinDengGuang();
  GengXinChuangLian();
  GengXinWindow();
  handleSerialDemoCommand();

  sendActionBoardState(false);
  LingxiNodeHttpTick();
  LingxiConfigPortalTick();

  if (millis() - ShangCiDaYinTime > STATUS_PRINT_INTERVAL) {
    ShangCiDaYinTime = millis();
    printActionBoardState();
  }

  delay(2);
}

