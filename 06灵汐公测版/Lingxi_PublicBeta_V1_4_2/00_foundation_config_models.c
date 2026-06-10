/*
 * 文件：00_foundation_config_models.ino
 * 标题：基础配置与数据模型
 * 说明：定义产品版本、运行状态、节点端点模板、全局结构体和核心配置。
 * 可改：可改公开版本文案和默认展示名
 * 禁止：随意改结构体字段、端点 ID、命令 ID。
 */

#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <FS.h>
#include <SD_MMC.h>
#include <Wire.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ESP.h>
#include "esp_arduino_version.h"
#include "driver/i2s.h"
#include "esp_intr_alloc.h"
#include <math.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha256.h>

#ifndef I2S_COMM_FORMAT_STAND_I2S
#define I2S_COMM_FORMAT_STAND_I2S I2S_COMM_FORMAT_I2S
#endif


#define ULTRA_HEADLESS_GATEWAY 1


const char* LINGXI_PRODUCT_NAME = "灵汐全屋智能控制中心";
const char* LINGXI_RELEASE_NAME = "灵汐全屋智能控制中心 公测版 V1.4.2";
const char* LINGXI_VERSION_CODE = "PublicBeta_V1_4_2";
const char* LINGXI_VERSION_STAGE = "public_beta";
const char* LINGXI_VERSION_PATCH = "公测版 V1.4.2 / 自由对话 Chat API 适配 / 白名单与规则引擎收口";
const char* LINGXI_BASELINE_VERSION = "公测版 V1.4.2：基于 V1.4.0 稳定封装，修正豆包自由对话为火山方舟 Chat API";


const char* AP_SSID = "Lingxi-Gateway";
const char* AP_PASSWORD = "12345678";

WebServer server(80);


Preferences NetPrefs;
Preferences CloudPrefs;
Preferences DevicePrefs;   
Preferences VoicePrefs;    
Preferences AuthPrefs;     
String SavedStaSsid = "";
String SavedStaPass = "";
String SavedApSsid = "";
String SavedApPass = "";
bool ApConfigEnabled = true;   
bool StaManualDisconnected = false;
unsigned long LastStaReconnectTry = 0;
unsigned long LastWifiScanMs = 0;
String LastWifiScanJson = "{\"ok\":true,\"count\":0,\"items\":[]}";


bool ApRestartPending = false;
unsigned long ApRestartDueMs = 0;
unsigned long LastApGuardMs = 0;


#define ENABLE_BEMFA_MQTT 1

const char* STA_WIFI_SSID     = "YOUR_STA_WIFI_SSID";
const char* STA_WIFI_PASSWORD = "YOUR_STA_WIFI_PASSWORD";

const char* BEMFA_UID          = "YOUR_BEMFA_UID";
const char* BEMFA_MQTT_SERVER  = "mqtt.bemfa.com";
const uint16_t BEMFA_MQTT_PORT = 9501;

const char* BEMFA_TOPIC_CMD    = "smarthomecmd";
const char* BEMFA_TOPIC_STATUS = "smarthomestatus";
const char* BEMFA_TOPIC_LOG    = "smarthomelog";


#define ENABLE_QWEATHER 1
const char* AMAP_WEATHER_HOST = "restapi.amap.com";
const char* AMAP_WEATHER_KEY  = "YOUR_AMAP_WEATHER_KEY";
const char* AMAP_CITY_CODE    = "150200";   
const unsigned long AMAP_WEATHER_INTERVAL_MS  = 30UL * 60UL * 1000UL;      
const unsigned long AMAP_FORECAST_INTERVAL_MS = 2UL * 60UL * 60UL * 1000UL;    

// 函数说明：判断函数，返回当前条件是否满足。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool IsPlaceholderText(const char* text) {
  if (text == nullptr) return true;

  String s = String(text);
  s.trim();

  if (s.length() == 0) return true;
  if (s.indexOf("PASTE_") >= 0) return true;
  if (s.indexOf("YOUR_") >= 0) return true;
  if (s == "null") return true;
  if (s == "NULL") return true;
  if (s == "undefined") return true;

  return false;
}

// 函数说明：判断函数，返回当前条件是否满足。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool IsPlaceholderText(const String& text) {
  return IsPlaceholderText(text.c_str());
}


#define ENABLE_SD_VOICE 1
#define ENABLE_VOICE_AUTO_FEEDBACK 1


#define ENABLE_BOARD_AUDIO_PLAYBACK 1


#define HEADLESS_FORCE_DISPLAY_OFF 1

const char* VOICE_MOUNT_POINT = "/sdcard";
const char* VOICE_ROOT_DIR = "/voice";
const char* ROLE_SHOREKEEPER_DIR = "/voice/shorekeeper";
const char* ROLE_CARTETHYIA_DIR = "/voice/cartethyia";


const char* ROLE_ASSET_ROOT_DIR = "/roles";
const char* ROLE_SHOREKEEPER_ASSET_DIR = "/roles/shorekeeper";
const char* ROLE_CARTETHYIA_ASSET_DIR = "/roles/cartethyia";

#define SDMMC_CLK_PIN 12
#define SDMMC_CMD_PIN 11
#define SDMMC_D0_PIN  13


#define BOARD_I2C_SDA_PIN 8
#define BOARD_I2C_SCL_PIN 9
#define IOEXT_ADDR 0x24
#define IOEXT_MODE_REG 0x02
#define IOEXT_OUTPUT_REG 0x03
#define IOEXT_SD_CS_PIN 4
#define IOEXT_LCD_DISP_PIN 2   
#define IOEXT_PA_CTRL_PIN 3    
#define IOEXT_SD_PA_SAFE_PIN 4 


#define ENABLE_CI1302_I2C_WAKE 0
#define CI1302_I2C_POLL_INTERVAL_MS 120
#define CI1302_I2C_SCAN_INTERVAL_MS 15000
#define CI1302_I2C_READ_LEN 1
#define CI1302_WAKE_RECORD_SECONDS 5
#define CI1302_WAKE_DEBOUNCE_MS 6000
#define CI1302_I2C_ADDR_FIXED 0x2B
#define CI1302_I2C_WRITE_REGISTER 0x03
#define CI1302_I2C_READ_REGISTER 0x64
#define CI1302_I2C_INIT_VALUE 0x67
#define CI1302_I2C_IDLE_FF 0xFF
#define CI1302_I2C_IDLE_88 0x88
#define CI1302_I2C_WAKE_VALUE_00 0x00
#define CI1302_I2C_WAKE_VALUE_03 0x03
#define CI1302_I2C_SLEEP_VALUE_6F 0x6F
#define ENABLE_LINGXI_V48_QUASI_STREAM_ASR 0
#define LINGXI_V48_STREAM_RECORD_SECONDS 5
#define LINGXI_V48_STREAM_RECORD_MIN_MS 1400
#define LINGXI_V48_STREAM_SILENCE_MS 900
#define LINGXI_V48_SPEECH_RMS_THRESHOLD 260
#define LINGXI_V48_SPEECH_PEAK_THRESHOLD 2000
#define LINGXI_V48_SILENCE_RMS_THRESHOLD 180
#define LINGXI_V48_SILENCE_PEAK_THRESHOLD 1300
#define ENABLE_LINGXI_V49_LISTEN_SESSION 0
#define LINGXI_V49_LISTEN_SESSION_TIMEOUT_MS 30000
#define LINGXI_V49_LISTEN_SESSION_MAX_ROUNDS 4
#define LINGXI_V49_LISTEN_NEXT_DELAY_MS 900
#define LINGXI_V49_LISTEN_NEXT_RECORD_SECONDS 5
#define LINGXI_V49_CONFIRM_TIMEOUT_MS 30000
#define ENABLE_LINGXI_CHAT_MODE_STREAM 0
#define LINGXI_CHAT_MODE_RECORD_SECONDS 5
#define LINGXI_CHAT_MODE_MAX_ROUNDS 6
#define LINGXI_CHAT_MODE_TIMEOUT_MS 45000
#define ENABLE_LINGXI_TRUE_STREAM_ASR 0
#define ENABLE_LINGXI_TRUE_STREAM_ASR_FALLBACK_FILE 1
#define LINGXI_STREAM_ASR_HOST "openspeech.bytedance.com"
#define LINGXI_STREAM_ASR_PATH "/api/v3/sauc/bigmodel_async"
#define LINGXI_STREAM_ASR_RESOURCE_FALLBACK "volc.seedasr.sauc.concurrent"
#define LINGXI_STREAM_ASR_MAX_MS 9000
#define LINGXI_STREAM_ASR_MIN_MS 1000
#define LINGXI_STREAM_ASR_CHUNK_MS 200
#define LINGXI_STREAM_ASR_SILENCE_MS 900
#define LINGXI_STREAM_ASR_SPEECH_RMS 260
#define LINGXI_STREAM_ASR_SPEECH_PEAK 2000
#define LINGXI_STREAM_ASR_SILENCE_RMS 180
#define LINGXI_STREAM_ASR_SILENCE_PEAK 1300


#define AUDIO_I2S_PORT       I2S_NUM_1
#define AUDIO_I2S_MCLK_PIN   6
#define AUDIO_I2S_BCLK_PIN   44
#define AUDIO_I2S_LRCK_PIN   16
#define AUDIO_I2S_DOUT_PIN   15   
#define AUDIO_I2S_DIN_PIN    43   
#define ES8311_I2C_ADDR      0x18   
#define BOARD_AUDIO_VOLUME   0xC0   
#define BOARD_AUDIO_BUF_SIZE 4096


#define ENABLE_VOICE_IO_FOUNDATION 1
#define VOICE_RECORD_DIR "/record"
#define VOICE_LAST_RECORD_PATH "/record/last.wav"
#define VOICE_RECORD_SAMPLE_RATE 16000
#define VOICE_RECORD_BITS_PER_SAMPLE 16
#define VOICE_RECORD_CHANNELS 1
#define VOICE_RECORD_SOFTWARE_GAIN 12  
#define VOICE_RECORD_MAX_SECONDS 15


#define ES7210_ADDR_00 0x40
#define ES7210_ADDR_01 0x41
#define ES7210_ADDR_10 0x42
#define ES7210_ADDR_11 0x43
#define ES7210_MIC_GAIN_REG_VALUE 0x1A   
#define ES7210_MIC_BIAS_2V87 0x70
#define ES7210_ADC_VOLUME_0DB 0xBF


bool BoardAudioReady = false;
uint32_t BoardAudioSampleRate = 0;
uint8_t BoardAudioChannels = 0;
uint8_t BoardAudioBits = 16;


TaskHandle_t BoardTtsPlayTaskHandle = nullptr;
volatile bool BoardTtsPlayActive = false;
String BoardTtsPlayPath = "";
String BoardTtsPlaySource = "";

uint8_t IoExtLastOutputValue = 0xFF;


const uint8_t LINGXI_MIC_LOCK_FMT = 6;
uint8_t VoiceMicDecodeMode = LINGXI_MIC_LOCK_FMT;

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceMicDecodeModeName() {
  switch (VoiceMicDecodeMode) {
    case 0: return "fmt0_i2s16_stereo_louder";
    case 1: return "fmt1_i2s32_hi16_louder";
    case 2: return "fmt2_i2s32_lo16_louder";
    case 3: return "fmt3_i2s32_mid16_louder";
    case 4: return "fmt4_i2s32_bswap_hi16_louder";
    case 5: return "fmt5_i2s32_hi16_left";
    case 6: return "fmt6_i2s32_hi16_right";
    case 7: return "fmt7_i2s16_mono_raw";
    default: return "fmt_unknown";
  }
}

auto VoiceMicI2SBits() -> i2s_bits_per_sample_t {
  if (VoiceMicDecodeMode == 0 || VoiceMicDecodeMode == 7) return I2S_BITS_PER_SAMPLE_16BIT;
  return I2S_BITS_PER_SAMPLE_32BIT;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
uint32_t LingxiBswap32(uint32_t v) {
  return ((v & 0x000000FFUL) << 24) | ((v & 0x0000FF00UL) << 8) | ((v & 0x00FF0000UL) >> 8) | ((v & 0xFF000000UL) >> 24);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
int32_t LingxiAbs32(int32_t v) {
  return v < 0 ? -v : v;
}

bool IoExtReady = false;
bool HeadlessDisplayIsOff = false;

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool IoExtWriteReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(IOEXT_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool BoardIoExtInitForSd() {
  Wire.begin(BOARD_I2C_SDA_PIN, BOARD_I2C_SCL_PIN);
  Wire.setClock(400000);
  delay(10);

  
  bool ok1 = IoExtWriteReg(IOEXT_MODE_REG, 0xFF);

  
  IoExtLastOutputValue = 0xFF;
  IoExtLastOutputValue |= (1 << IOEXT_SD_CS_PIN);
  IoExtLastOutputValue |= (1 << IOEXT_PA_CTRL_PIN);
  IoExtLastOutputValue |= (1 << IOEXT_SD_PA_SAFE_PIN);

#if HEADLESS_FORCE_DISPLAY_OFF
  
  IoExtLastOutputValue &= ~(1 << IOEXT_LCD_DISP_PIN);
#endif

  bool ok2 = IoExtWriteReg(IOEXT_OUTPUT_REG, IoExtLastOutputValue);

  IoExtReady = ok1 && ok2;
  HeadlessDisplayIsOff = IoExtReady && ((IoExtLastOutputValue & (1 << IOEXT_LCD_DISP_PIN)) == 0);
  Serial.printf("[IOEXT] init=%s, SD_CS=HIGH, PA=HIGH, DISP=%s\n", IoExtReady ? "OK" : "FAIL", HeadlessDisplayIsOff ? "LOW/OFF" : "HIGH/ON");
  return IoExtReady;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool IoExtSetPin(uint8_t pin, bool level) {
  if (!IoExtReady) BoardIoExtInitForSd();
  if (!IoExtReady) return false;
  if (pin > 7) return false;

  if (level) IoExtLastOutputValue |= (1 << pin);
  else IoExtLastOutputValue &= ~(1 << pin);

  bool ok = IoExtWriteReg(IOEXT_OUTPUT_REG, IoExtLastOutputValue);
  if (ok && pin == IOEXT_LCD_DISP_PIN) HeadlessDisplayIsOff = !level;
  return ok;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HeadlessDisplayOff(const char* reason) {
#if HEADLESS_FORCE_DISPLAY_OFF
  bool ok = IoExtSetPin(IOEXT_LCD_DISP_PIN, false);
  HeadlessDisplayIsOff = ok;
  Serial.printf("[HEADLESS] DisplayOff %s: %s\n", reason ? reason : "", ok ? "OK" : "FAIL");
#endif
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HeadlessDisplayOnForDebug() {
#if HEADLESS_FORCE_DISPLAY_OFF
  bool ok = IoExtSetPin(IOEXT_LCD_DISP_PIN, true);
  HeadlessDisplayIsOff = !ok ? HeadlessDisplayIsOff : false;
  Serial.printf("[HEADLESS] DisplayOnForDebug: %s\n", ok ? "OK" : "FAIL");
#endif
}

enum VoiceRole : uint8_t {
  ROLE_SHOREKEEPER = 0,
  ROLE_CARTETHYIA = 1
};

typedef struct {
  bool sdOnline;
  bool shorekeeperReady;
  bool cartethyiaReady;
  uint16_t shorekeeperCount;
  uint16_t cartethyiaCount;
  uint8_t currentRole;
  bool audioBusy;
  uint16_t currentVoiceId;
  String currentVoiceFile;
  String lastVoiceError;
  unsigned long lastVoiceMs;
} VoiceState;

VoiceState Voice;


bool RoleAssetShorekeeperJsonReady = false;
bool RoleAssetCartethyiaJsonReady = false;
bool RoleAssetShorekeeperAvatarReady = false;
bool RoleAssetShorekeeperPortraitReady = false;
bool RoleAssetShorekeeperIconReady = false;
bool RoleAssetCartethyiaAvatarReady = false;
bool RoleAssetCartethyiaPortraitReady = false;
bool RoleAssetCartethyiaIconReady = false;
String RoleAssetLastError = "";
unsigned long RoleAssetLastLoadMs = 0;

enum VoiceInputSource : uint8_t {
  VOICE_INPUT_BOARD_MIC = 0,
  VOICE_INPUT_WEBUI_MIC = 1,
  VOICE_INPUT_AUTO = 2
};

enum VoiceOutputTarget : uint8_t {
  VOICE_OUTPUT_BOARD_SPEAKER = 0,
  VOICE_OUTPUT_WEBUI_SPEAKER = 1,
  VOICE_OUTPUT_BOTH = 2,
  VOICE_OUTPUT_AUTO = 3
};

enum VoiceBrainMode : uint8_t {
  VOICE_BRAIN_RECORD_TEST = 0,
  VOICE_BRAIN_CLOUD_ASR = 1,
  VOICE_BRAIN_DOUBAO_AI = 2,
  VOICE_BRAIN_LOCAL_ESPSR = 3
};

typedef struct {
  uint8_t inputSource;
  uint8_t outputTarget;
  uint8_t brainMode;
  bool micReady;
  bool recording;
  bool lastRecordReady;
  String lastRecordFile;
  String lastError;
  uint32_t lastRecordMs;
  uint32_t lastDurationMs;
  uint32_t lastDataBytes;
  uint16_t sampleRate;
  uint8_t channels;
  uint8_t bitsPerSample;
  int peakLevel;
  int rmsLevel;
} VoiceIoState;

VoiceIoState VoiceIO;


volatile bool VoiceRecordStopRequested = false;
volatile bool VoiceRecordAsyncActive = false;
volatile uint32_t VoiceRecordAsyncStartMs = 0;
volatile uint32_t VoiceRecordMinDurationMs = 0;
volatile bool VoiceRecordVadEnabled = false;
uint32_t VoiceRecordVadMinDurationMs = 0;
uint32_t VoiceRecordVadSilenceMs = 0;
int VoiceRecordVadSpeechRmsThreshold = 0;
int VoiceRecordVadSpeechPeakThreshold = 0;
int VoiceRecordVadSilenceRmsThreshold = 0;
int VoiceRecordVadSilencePeakThreshold = 0;
uint32_t VoiceRecordAsyncMaxSec = 60;
TaskHandle_t VoiceRecordTaskHandle = nullptr;


enum VoiceTaskStatus : uint8_t {
  VOICE_TASK_IDLE = 0,
  VOICE_TASK_RECORDING = 1,
  VOICE_TASK_RECORDED = 2,
  VOICE_TASK_TEXT_READY = 3,
  VOICE_TASK_COMMAND_READY = 4,
  VOICE_TASK_EXECUTED = 5,
  VOICE_TASK_REPLIED = 6,
  VOICE_TASK_ERROR = 9
};

typedef struct {
  uint32_t seq;
  uint8_t status;
  String source;
  String output;
  String brain;
  String audioFile;
  String text;
  String intent;
  String command;
  String reply;
  String error;
  bool uploadReady;
  bool asrReady;
  bool commandReady;
  bool executed;
  bool replyReady;
  uint32_t createdMs;
  uint32_t updatedMs;
  uint32_t durationMs;
  uint32_t dataBytes;
  int peak;
  int rms;
} VoiceTaskState;

VoiceTaskState VoiceTask;
uint32_t VoiceTaskSeqCounter = 1;


#define ENABLE_LINGXI_V4_LOCAL_VOICE 0
#define LINGXI_V4_RECORD_SECONDS 10
#define LINGXI_V4_WAKE_REPLY_VOICE_ID 3
#define ENABLE_LINGXI_CONFIRM_AGENT 1
#define ENABLE_LINGXI_EXTERNAL_AGENT_API 0
#define ENABLE_DOUBAO_STRUCTURED_COMMAND_PROMPT 0
#define ENABLE_DOUBAO_FREE_CHAT_COMMANDS 0

enum LingxiVoiceRouteLayer : uint8_t {
  ROUTE_LAYER_NONE = 0,
  ROUTE_LAYER_LOCAL_COMMAND = 1,
  ROUTE_LAYER_CONFIRM_AGENT = 2,
  ROUTE_LAYER_FREE_CHAT = 3
};

typedef struct {
  uint8_t layer;
  String text;
  String intent;
  String command;
  String reply;
  bool needConfirm;
  bool executed;
} LingxiVoiceRouteResult;


enum LingxiV4VoiceState : uint8_t {
  V4_VOICE_IDLE = 0,
  V4_VOICE_WAKE_DETECTED = 1,
  V4_VOICE_WAKE_REPLY = 2,
  V4_VOICE_RECORDING = 3,
  V4_VOICE_ASR_RUNNING = 4,
  V4_VOICE_INTENT_ROUTING = 5,
  V4_VOICE_COMMAND_EXECUTING = 6,
  V4_VOICE_CHAT_RUNNING = 7,
  V4_VOICE_TTS_PLAYING = 8,
  V4_VOICE_ERROR = 9
};

typedef struct {
  uint8_t state;
  bool enabled;
  bool wakeReady;
  bool streamAsrReady;
  bool busy;
  String lastWakeSource;
  String lastText;
  String lastIntent;
  String lastCommand;
  String lastReply;
  String lastError;
  uint32_t updatedMs;
} LingxiV4VoiceRuntime;

LingxiV4VoiceRuntime V4Voice;


typedef struct {
  bool pending;
  String type;
  String text;
  uint32_t seconds;
  String source;
} LingxiV4VoiceRequest;

LingxiV4VoiceRequest V4Req;
SemaphoreHandle_t V4ReqMutex = NULL;


bool V49ListenSessionActive = false;
uint8_t V49ListenSessionRounds = 0;
uint32_t V49ListenSessionStartedMs = 0;
uint32_t V49ListenSessionLastActivityMs = 0;

bool V49ConfirmPending = false;
String V49ConfirmIntent = "";
String V49ConfirmCommand = "";
String V49ConfirmReply = "";
uint32_t V49ConfirmCreatedMs = 0;

bool V4ChatModeActive = false;
uint8_t V4ChatModeRounds = 0;
uint32_t V4ChatModeStartedMs = 0;
uint32_t V4ChatModeLastActivityMs = 0;


#define ENABLE_CLOUD_ASR_BRIDGE 1
const char* ASR_BRIDGE_URL = "http://YOUR_ASR_BRIDGE_HOST:8000/asr";
const char* ASR_BRIDGE_TOKEN = "";   
const uint32_t ASR_BRIDGE_TIMEOUT_MS = 30000;


#define ENABLE_DOUBAO_AI_CENTER 1
#define ENABLE_DOUBAO_DIRECT_ASR 1
#define ENABLE_DOUBAO_DIRECT_TTS 1

const char* DOUBAO_RESPONSES_URL = "https://ark.cn-beijing.volces.com/api/v3/chat/completions";  // V1.4.2：自由对话统一走火山方舟 Chat API
const char* DOUBAO_API_KEY = "YOUR_DOUBAO_API_KEY";  
const char* DOUBAO_MODEL_MINI = "doubao-seed-2-0-mini-260428";
const char* DOUBAO_MODEL_LITE = "doubao-seed-2-0-lite-260428";
const char* DOUBAO_MODEL_PRO  = "doubao-seed-2-0-pro-260215";
const char* DOUBAO_DEFAULT_MODEL_MODE = "mini";   
const uint32_t DOUBAO_LLM_TIMEOUT_MS = 45000;       
const uint16_t DOUBAO_MAX_OUTPUT_TOKENS = 128;
const uint16_t DOUBAO_MAX_COMMANDS = 5;
const uint32_t DOUBAO_AI_COOLDOWN_MS = 1800;


const char* DOUBAO_SPEECH_APP_NAME = "default";
const char* DOUBAO_SPEECH_APP_ID = "LINGXI_NEW_CONSOLE";
const char* DOUBAO_SPEECH_ACCESS_TOKEN = "YOUR_DOUBAO_SPEECH_ACCESS_TOKEN";
const char* DOUBAO_SPEECH_SECRET_KEY = "YOUR_DOUBAO_SPEECH_SECRET_KEY";
const char* DOUBAO_SPEECH_API_KEY = "YOUR_DOUBAO_SPEECH_API_KEY";  


const char* DOUBAO_ASR_FILE_INSTANCE = "Doubao_Seed_ASR_AUC_2.020000000737931139682";
const char* DOUBAO_ASR_STREAMING_INSTANCE = "Doubao_Seed_ASR_Streaming_2.020000000738134890690"; 
const char* DOUBAO_ASR_STANDARD_URL = "https://openspeech.bytedance.com/api/v3/auc/bigmodel/recognize";
const char* DOUBAO_ASR_FLASH_URL = "https://openspeech.bytedance.com/api/v3/auc/bigmodel/recognize/flash";
const char* DOUBAO_ASR_RESOURCE_ID_STANDARD = "volc.seedasr.auc";
const char* DOUBAO_ASR_RESOURCE_ID_FLASH = "volc.seedasr.auc";
const char* DOUBAO_ASR_RESOURCE_ID = DOUBAO_ASR_RESOURCE_ID_STANDARD;
const uint32_t DOUBAO_ASR_TIMEOUT_MS = 45000;

const char* DOUBAO_TTS_INSTANCE = "Doubao-TTS-2.0";
const char* DOUBAO_VOICE_CLONE_INSTANCE = "Doubao-VoiceClone-2.0";  
const char* DOUBAO_TTS_URL = "https://openspeech.bytedance.com/api/v3/tts/unidirectional/sse";


const char* DOUBAO_TTS_RESOURCE_ID_PUBLIC = "seed-tts-2.0";
const char* DOUBAO_TTS_RESOURCE_ID_ROLE   = "seed-icl-2.0";

const char* DOUBAO_TTS_PUBLIC_SPEAKER = "zh_female_xiaohe_uranus_bigtts";

const bool DOUBAO_TTS_PUBLIC_FIRST = false;  
const char* DOUBAO_TTS_RESOURCE_ID = DOUBAO_TTS_RESOURCE_ID_ROLE;
const char* DOUBAO_VOICE_SHOREKEEPER = "S_DDV1VAL22";
const char* DOUBAO_VOICE_CARTETHYIA  = "S_p4f3VAL22";
const char* DOUBAO_TTS_LAST_PATH = "/record/ai_reply.wav";
const uint32_t DOUBAO_TTS_TIMEOUT_MS = 45000;

String LastDoubaoAiText = "";
String LastDoubaoAiReply = "";
String LastDoubaoAiCommands = "";
String LastDoubaoAiModel = "lite";
String LastDoubaoAiError = "";
uint32_t LastDoubaoAiMs = 0;
uint32_t LastDoubaoAiElapsedMs = 0;
String LastDoubaoAiState = "idle";
String LastDoubaoAsrState = "idle";
String LastDoubaoAsrError = "";
String LastDoubaoAsrText = "";
String LastDoubaoAsrEndpoint = "";
String LastDoubaoAsrResource = "";
int LastDoubaoAsrHttpCode = 0;
uint32_t LastDoubaoAsrElapsedMs = 0;


String SavedArkApiKey = "";       
String SavedVoiceApiKey = "";     
String SavedNewApiKey = "";       
String SavedModelMini = "";
String SavedModelLite = "";
String SavedModelPro = "";
String SavedDefaultModelMode = "lite";
String SavedAsrResource = "";
String SavedTtsPublicResource = "";
String SavedTtsCloneResource = "";
String SavedPublicSpeaker = "";
String SavedVoiceShorekeeper = "";
String SavedVoiceCartethyia = "";

String SavedAmapWeatherKey = "";
String SavedAmapCityCode = "";
String SavedBemfaUid = "";
String SavedBemfaTopicCmd = "";
String SavedBemfaTopicStatus = "";
String SavedBemfaTopicLog = "";


WiFiClient BemfaWifiClient;
PubSubClient BemfaMqtt(BemfaWifiClient);

bool BemfaOnline = false;
unsigned long LastBemfaReconnectTime = 0;
unsigned long LastBemfaStatusPublishTime = 0;
unsigned long LastBemfaLogPublishTime = 0;
bool BemfaCmdSubscribed = false;
bool BemfaCmdSetSubscribed = false;
int BemfaLastStateCode = 0;
String LastCloudCommand = "";
String LastCloudReply = "";


uint8_t sensorBoardMac[] = {0x38, 0x44, 0xBE, 0x44, 0x53, 0xEC};


uint8_t actionBoardMac[] = {0xA4, 0xCB, 0x8F, 0xD8, 0xBF, 0x08};


#define ENABLE_DEMO_MODE_DEFAULT 0


#define GUANGZHAO_YUZHI 80
#define YANWU_YUZHI     2000
#define RANQI_YUZHI     2000
#define WUREN_GUANDENG_TIME 600000UL   


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
#define SCENE_GAMING     15
#define SCENE_ROMANTIC   16
#define SCENE_EMO        17
#define SCENE_NIGHT      18
#define SCENE_MORNING    19
#define SCENE_NOON       20
#define SCENE_PARTY      21
#define SCENE_CUSTOM     99


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
} ControlCommand;


bool IsCmdDifferent(ControlCommand a, ControlCommand b);
void SetControlCommand(ControlCommand newCmd);
void RefreshLegacyLightField(ControlCommand &c);
void CommitWebCommand(ControlCommand c);
void ApplyDeviceStatePatchFromCommand(ControlCommand c);


bool NodeDeviceRuntimeEndpointOnline(const char* endpointId);
void NodeDeviceApplyCompatReport(String body, String nodeId, unsigned long now);
String ControlCommandToHttpJson(ControlCommand c);
void SendControlCommandHttpCompat(ControlCommand c);
void NodeCompatSetPendingRoute(const char* endpointList);
void CommitEndpointCommand(ControlCommand c, const char* endpointId);
void CommitEndpointGroupCommand(ControlCommand c, const char* endpointList);
bool SendControlCommandRoutedCompat(ControlCommand c);
bool NodeCompatShouldSendToRuntimeNode(int idx);
bool NodeCompatEndpointListed(const String &list, const char* endpointId);
bool NodeCompatRuntimeNodeHasEndpoint(int idx, const char* endpointId);
bool NodeCompatShouldAcceptReport(String fromNodeId, const char* endpointId);
bool NodeCompatIsIndependentActionNode(int idx);
int NodeCompatFindStandaloneOwner(const char* endpointId);
bool NodeCompatPostCommandToNode(int idx, ControlCommand c, const char* endpointId);
bool NodeCompatSendEspNowFallback(ControlCommand c, const char* reason);
void NodeCompatSetPendingRoute(const char* endpointList);
void CommitEndpointCommand(ControlCommand c, const char* endpointId);
void CommitEndpointGroupCommand(ControlCommand c, const char* endpointList);
bool SendControlCommandRoutedCompat(ControlCommand c);
bool ApplySceneCommand(String cmd, ControlCommand &c, String &reply);
void CancelSceneForManualCommand(String cmd, const char* source, ControlCommand &c);


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

  bool sensorBoardOnline;
  bool autoMode;
  bool awayMode;
  bool alarmMode;
  bool previewMode;

  int lightState;
  int relayState;
  int buzzerState;
  int SceneState;

  uint32_t LastSensorTime;
  uint32_t lastPacketSeq;
} SystemState;


typedef struct {
  bool online;
  bool airOnline;
  unsigned long lastNowMs;
  unsigned long lastAirMs;
  String city;
  String weatherText;
  float outdoorTemp;
  float feelsLike;
  int outdoorHumidity;
  String windDir;
  String windScale;
  float precip;
  bool forecastOnline;
  String todayTempRange;
  String tomorrowForecast;
  int airAqi;
  String airCategory;
  String primaryPollutant;
  String umbrellaTip;
  String maskTip;
  String windowTip;
  String clothesTip;
  String aiTip;
} WeatherState;

#define CENTER_LOG_COUNT 36
typedef struct {
  String type;
  String msg;
  unsigned long ms;
} CenterLogItem;

SensorData latestSensorData;
SystemState State;
ControlCommand CurrentCmd;
ActionBoardState actionBoardState;


ActionBoardState uiActionBoardState;
volatile uint32_t DeviceStatePatchVersion = 0;
unsigned long LastDeviceStatePatchMs = 0;

WeatherState Weather;
CenterLogItem CenterLogs[CENTER_LOG_COUNT];
uint8_t CenterLogHead = 0;
uint32_t CenterLogSeq = 0;


#define LINGXI_NODE_COUNT 3
#define LINGXI_ENDPOINT_COUNT 19
#define LINGXI_TEMPLATE_COUNT 14

typedef struct {
  const char* id;
  const char* name;
  const char* nodeType;
  const char* mode;
  const char* role;
  const char* transport;
  const char* macHint;
  const char* description;
} LingxiNodeInfo;

typedef struct {
  const char* id;
  const char* nodeId;
  const char* name;
  const char* endpointType;
  const char* category;
  const char* room;
  const char* capability;
  const char* unit;
  const char* commandOn;
  const char* commandOff;
  const char* templateId;
  bool reserved;
} LingxiEndpointInfo;

typedef struct {
  const char* id;
  const char* name;
  const char* nodeType;
  const char* endpointType;
  const char* capability;
  const char* defaultRoom;
  const char* description;
} LingxiDeviceTemplate;

const LingxiNodeInfo LingxiNodes[LINGXI_NODE_COUNT] = {
  {"Lingxi-Gateway", "中控网关", "gateway", "integrated_gateway", "center", "local+wifi_http+esp_now+mqtt", "runtime", "WebUI 主中控、云端 AI、MQTT、HTTP/ESP-NOW 双兼容网关"},
  {"SensorBoard-01", "感知层集成节点", "sensor_board", "integrated_board", "sensor", "wifi_http+esp_now", "38:44:BE:44:53:EC", "当前感知层集成板，也兼容拆分感知节点"},
  {"ActionBoard-01", "执行层集成节点", "action_board", "integrated_board", "executor", "wifi_http+esp_now", "A4:CB:8F:D8:BF:08", "当前执行层集成板，也兼容拆分执行节点"}
};

const LingxiEndpointInfo LingxiEndpoints[LINGXI_ENDPOINT_COUNT] = {
  {"temperature", "SensorBoard-01", "室内温度", "sensor", "environment", "客厅", "temperature", "°C", "", "", "template_temperature_sensor", false},
  {"humidity", "SensorBoard-01", "室内湿度", "sensor", "environment", "客厅", "humidity", "%", "", "", "template_humidity_sensor", false},
  {"light_level", "SensorBoard-01", "环境光照", "sensor", "environment", "客厅", "light", "%", "", "", "template_light_sensor", false},
  {"door_main_contact", "SensorBoard-01", "大门门磁", "sensor", "security", "客厅", "contact", "", "", "", "template_contact_sensor", false},
  {"window_contact", "SensorBoard-01", "窗户门磁", "sensor", "security", "阳台", "contact", "", "", "", "template_contact_sensor", false},
  {"human_living", "SensorBoard-01", "客厅人体", "sensor", "presence", "客厅", "presence", "", "", "", "template_presence_sensor", false},
  {"human_bedroom", "SensorBoard-01", "卧室人体", "sensor", "presence", "卧室", "presence", "", "", "", "template_presence_sensor", false},
  {"smoke", "SensorBoard-01", "烟雾传感器", "sensor", "safety", "厨房", "smoke", "raw", "", "", "template_smoke_sensor", false},
  {"gas", "SensorBoard-01", "燃气传感器", "sensor", "safety", "厨房", "gas", "raw", "", "", "template_gas_sensor", false},
  {"living_light", "ActionBoard-01", "客厅灯", "action", "light", "客厅", "switch", "", "light_living_on", "light_living_off", "template_light_switch", false},
  {"bedroom_light", "ActionBoard-01", "卧室灯", "action", "light", "卧室", "switch", "", "light_bed_on", "light_bed_off", "template_light_switch", false},
  {"living_curtain", "ActionBoard-01", "客厅窗帘", "action", "curtain", "客厅", "curtain", "%", "curtain_living_open", "curtain_living_close", "template_curtain_motor", false},
  {"bedroom_curtain", "ActionBoard-01", "卧室窗帘", "action", "curtain", "卧室", "curtain", "%", "curtain_bed_open", "curtain_bed_close", "template_curtain_motor", false},
  {"window_servo", "ActionBoard-01", "窗户舵机", "action", "window", "卧室", "window", "%", "window_open", "window_close", "template_window_servo", false},
  {"air_relay", "ActionBoard-01", "空调电源", "action", "relay", "客厅", "power", "", "relay_air_on", "relay_air_off", "template_power_relay", false},
  {"tv_relay", "ActionBoard-01", "电视电源", "action", "relay", "客厅", "power", "", "relay_tv_on", "relay_tv_off", "template_power_relay", false},
  {"fridge_relay", "ActionBoard-01", "冰箱电源", "action", "relay", "厨房", "power", "", "relay_fridge_on", "relay_fridge_off", "template_power_relay", false},
  {"water_heater_relay", "ActionBoard-01", "热水器电源", "action", "relay", "卫生间", "power", "", "relay_water_heater_on", "relay_water_heater_off", "template_power_relay", false},
  {"mi_plug_3_reserved", "ActionBoard-01", "小米智能插座3预留", "action", "plug", "扩展", "reserved", "", "", "", "template_smart_plug_reserved", true}
};

const LingxiDeviceTemplate LingxiTemplates[LINGXI_TEMPLATE_COUNT] = {
  {"template_integrated_sensor_board", "感知层集成板模板", "sensor_board", "multi_sensor", "temperature,humidity,light,contact,presence,smoke,gas", "全屋", "适配当前三板架构中的 SensorBoard-01"},
  {"template_integrated_action_board", "执行层集成板模板", "action_board", "multi_action", "light,curtain,window,relay", "全屋", "适配当前三板架构中的 ActionBoard-01"},
  {"template_temperature_sensor", "温度数据端点模板", "sensor_device", "sensor", "temperature", "房间", "软件上是温度数据端点，硬件上可与湿度由同一个温湿度节点承载"},
  {"template_humidity_sensor", "湿度数据端点模板", "sensor_device", "sensor", "humidity", "房间", "软件上是湿度数据端点，硬件上可与温度由同一个温湿度节点承载"},
  {"template_light_sensor", "光照传感器模板", "sensor_device", "sensor", "light", "房间", "未来一个 ESP32 接一个光照传感器"},
  {"template_contact_sensor", "门窗磁模板", "sensor_device", "sensor", "contact", "门窗", "未来一个 ESP32 接一个门磁或窗磁"},
  {"template_presence_sensor", "人体感应子节点模板", "sensor_device", "sensor", "presence", "房间", "未来一个 ESP32 接一个人体传感器"},
  {"template_smoke_sensor", "烟雾传感器模板", "sensor_device", "sensor", "smoke", "厨房", "未来一个 ESP32 接一个烟雾传感器"},
  {"template_gas_sensor", "燃气传感器模板", "sensor_device", "sensor", "gas", "厨房", "未来一个 ESP32 接一个燃气传感器"},
  {"template_light_switch", "灯光开关模板", "action_device", "action", "switch", "房间", "未来一个 ESP32 控制一路灯光"},
  {"template_curtain_motor", "窗帘伴侣节点模板", "action_device", "action", "curtain", "房间", "未来一个 ESP32 接窗帘电机/驱动板"},
  {"template_window_servo", "窗户舵机模板", "action_device", "action", "window", "窗户", "未来一个 ESP32 接窗户舵机"},
  {"template_power_relay", "插座/继电器节点模板", "action_device", "action", "power", "房间", "未来一个 ESP32 接继电器或智能插座"},
  {"template_smart_plug_reserved", "智能插座预留模板", "action_device", "action", "reserved", "扩展", "给小米智能插座3或后续外部生态预留"}
};


String DeviceEndpointName[LINGXI_ENDPOINT_COUNT];
String DeviceEndpointRoom[LINGXI_ENDPOINT_COUNT];
bool DeviceEndpointEnabled[LINGXI_ENDPOINT_COUNT];


#define LINGXI_RUNTIME_NODE_MAX 32  
#define LINGXI_NODE_HTTP_ONLINE_TIMEOUT_MS 30000UL

typedef struct {
  bool used;
  String nodeId;
  String name;
  String nodeType;
  String role;
  String transport;
  String mac;
  String ip;
  String bindTokenMask;
  String endpointsJson;
  String lastReportJson;
  String lastStatus;
  uint16_t endpointCount;
  uint32_t registerCount;
  uint32_t reportCount;
  unsigned long registeredMs;
  unsigned long lastSeenMs;
  unsigned long lastReportMs;
} LingxiRuntimeNode;

LingxiRuntimeNode RuntimeNodes[LINGXI_RUNTIME_NODE_MAX];
uint32_t RuntimeNodeSeq = 0;

volatile bool hasNewSensorData = false;
volatile bool hasNewActionData = false;
volatile bool CmdNeedSend = false;

portMUX_TYPE DataMux = portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t StateMutex;

unsigned long LastYouRenTime = 0;
unsigned long LastCmdSendTime = 0;
unsigned long LastDemoUpdateTime = 0;
unsigned long lastActionBoardTime = 0;


void InitStaWifiForCloud();
void LoadNetworkConfig();
String NetStaSsid();
String NetStaPass();
String NetApSsid();
String NetApPass();
void SaveStaWifiConfig(String ssid, String pass);
void SaveApWifiConfig(String ssid, String pass, bool enabled);
bool StartOrUpdateSoftAp();
String BuildWifiScanJson();
String WifiScanSummary();
String WifiConnectAndSave(String ssid, String pass);
String WifiDisconnectKeepAp();
String ApSaveAndApply(String ssid, String pass);
String ApToggleSafe();
void ScheduleApRestart();
void ApAlwaysOnLoop();
void WifiMaintainLoop();
void HandleWifiScanApi();
void BemfaReconnect(bool forceNow);
void BemfaMqttLoop();
void BemfaPublishStatus();
void BemfaPublishLog(String type, String msg);
void TaskBemfaMqtt(void *pvParameters);
String ExecuteUnifiedCommand(String cmd, const char* source);
void AddCenterLog(String type, String msg);
void HandleLogsApi();
String JsonEscape(String s);
void InitWeatherState();
void TaskQWeather(void *pvParameters);
void QWeatherLoop();
bool QWeatherFetchNow();
bool QWeatherFetchAir();
String QWeatherBuildAiTip();
void InitVoiceState();
bool VoiceInitSD();
void VoiceReload();
String VoiceRoleName();
String VoiceRoleDir(uint8_t role);
String VoiceFindFileById(uint16_t id, uint8_t role);
String RoleIdByIndex(uint8_t role);
String RoleDisplayNameByIndex(uint8_t role);
String RoleAssetDirByIndex(uint8_t role);
String RoleAssetPathByIndex(uint8_t role, String type);
String RoleJsonPathByIndex(uint8_t role);
String RoleAssetUrlByIndex(uint8_t role, String type);
String SdReadSmallTextFile(String path, size_t limitBytes);
String JsonFieldLoose(String json, String key, String fallback);
bool RoleAssetFileReady(uint8_t role, String type);
void RoleLoadAssets();
String RoleOneStatusJson(uint8_t role);
String RoleAssetsStatusJson();
void HandleRoleStatusApi();
void HandleRoleLibraryApi();
void HandleRoleAssetApi();
bool TryServeRoleAsset(uint8_t role, String type);
void SendRoleSvg(const char *name, const char *mainColor, const char *subColor);
bool VoiceRequestPlay(uint16_t id, const char* source);
bool VoiceRequestPlayOnBoard(uint16_t id, const char* source);
bool VoiceHardwarePlayPath(String path);
bool BoardAudioPlayWav(String path);
bool BoardAudioInit(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample);
uint16_t VoiceIdForCommand(String cmd, String reply);
void VoiceAutoFeedback(String cmd, String reply, const char* source);
void HandleVoiceStatusApi();
void HandleVoiceFileApi();
void HandleVoicePlayApi();
void HandleVoiceListApi();
void HeadlessDisplayOff(const char* reason);
void HeadlessDisplayOnForDebug();
void InitVoiceIoState();
String VoiceInputSourceName();
String VoiceOutputTargetName();
String VoiceBrainModeName();
String VoiceIoStatusJson();
bool ES7210WriteReg(uint8_t addr, uint8_t reg, uint8_t val);
uint8_t ES7210ProbeAddress();
bool ES7210MicAdcInit(uint32_t sampleRate);
bool BoardMicI2SBegin(uint32_t sampleRate);
void VoiceIoWriteWavHeader(File &file, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels, uint32_t dataSize);
bool BoardMicRecordWav(String path, uint32_t seconds);
void HandleVoiceIoStatusApi();
void HandleVoiceIoRecordApi();
void HandleVoiceIoFileApi();
void HandleVoiceIoDiagApi();
void HandleVoiceIoPlayLastApi();
void HandleVoiceIoSetApi();
void InitVoiceTaskState();
String VoiceTaskStatusName();
String VoiceTaskStatusJson();
void VoiceTaskClear(String reason);
void VoiceTaskCreateFromRecord(String source);
void VoiceTaskSetText(String text, String source);
void VoiceTaskSetCommand(String command, String intent);
void VoiceTaskSetReply(String reply);
void HandleVoiceTaskStatusApi();
void HandleVoiceTaskClearApi();
void HandleVoiceTaskMockTextApi();
void HandleVoiceTaskMockCommandApi();
void HandleVoiceTaskMockReplyApi();
String AsrBridgeUrl();
String VoiceTaskExtractAsrText(String body);
bool VoiceTaskRunCloudAsr();
String VoiceTextToCommand(String text, String &intent);
bool VoiceTaskParseCurrentText(bool executeNow);
void HandleVoiceTaskAsrApi();
void HandleVoiceTaskParseApi();
void HandleVoiceTaskAsrExecuteApi();
bool IsSceneCommand(String cmd);
bool IsManualDeviceCommand(String cmd);

String JsonUnescape(String s);
String JsonStringAt(const String &json, int quotePos);
String JsonGetStringAfter(const String &json, const char *key, int startPos = 0);
String ExtractJsonObjectFromText(String text);
String DoubaoModelByMode(String mode);
String DoubaoBuildPrompt(String userText);
String LingxiCleanUserRawText(String text);
String DoubaoExtractOutputText(String body);
String DoubaoFallbackChatReply(String userText);
String DoubaoCleanReply(String reply, String userText);
String JsonArrayStringItems(const String &json, const char *key, uint8_t maxItems);
bool AiCommandAllowed(String cmd);
bool ExecuteAiCommandsCsv(String csv, String source, String &execReply);
bool DoubaoAiRunText(String userText, String modelMode, bool executeNow, bool useTts);
bool FileToBase64String(String path, String &outB64, size_t maxBytes);
bool DecodeBase64ToFile(String b64, File &file);
bool VoiceTaskRunDoubaoAsr();
bool DoubaoTtsSynthesizeToSd(String text, String speakerId, String outPath);
void InitV4LocalVoiceState();
String V4VoiceStateName(uint8_t state);
String V4VoiceStatusJson();
void V4VoiceSetState(uint8_t state, String note);
bool V4VoicePlayWakeReply();
bool V4VoiceRouteText(String text, String source);
bool V4VoiceRunOnceFromBoardMic(uint32_t seconds, String source);
bool V4TrueStreamAsrToText(String &outText, String source);
String V4StreamAsrResource();
String V4RandomRequestId();
String V4WebSocketKey();
bool V4WsSendFrame(WiFiClientSecure &client, uint8_t opcode, const uint8_t *payload, size_t len);
bool V4WsReadFrame(WiFiClientSecure &client, String &out, uint32_t timeoutMs);
bool V4SaucSendFullRequest(WiFiClientSecure &client, String reqid);
bool V4SaucSendAudioPacket(WiFiClientSecure &client, const uint8_t *audio, size_t len, int32_t seq, bool last);
String V4SaucExtractText(String payload);
bool V4BoardMicReadPcmChunk(uint8_t *outPcm, size_t maxBytes, uint32_t targetMs, size_t &outBytes, int &chunkPeak, int &chunkRms);
bool ConfirmAgentTryRoute(String text, LingxiVoiceRouteResult &r);
bool DoubaoFreeChatRunText(String userText, String modelMode, bool useTts);
bool V49ListenSource(String source);
void V49ListenSessionStart(String reason);
void V49ListenSessionStop(String reason);
bool V49ListenSessionMaybeContinue(String source, bool routeOk);
bool V49ConfirmTryResolve(String text, LingxiVoiceRouteResult &r);
void V49ConfirmSet(String intent, String command, String reply);
void V49ConfirmClear(String reason);
bool V49TextIsConfirm(String t);
bool V49TextIsCancel(String t);
bool V4ChatModeIsStartCommand(String text);
bool V4ChatModeIsStopCommand(String text);
void V4ChatModeStart(String reason);
void V4ChatModeStop(String reason);
bool V4ChatModeActiveNow();
bool V4ChatModeTryHandle(String text, String source);
void V4SpeakSystemReply(String reply);
String VoiceRouteLayerName(uint8_t layer);
bool V4HandleSerialCommand(String raw);
void TaskV4LocalVoice(void *pvParameters);
void RefreshEspNowPeer(uint8_t *mac);
void RefreshEspNowAllPeers();
void V4QueueRequest(String type, String text, uint32_t seconds, String source);
bool V4TakeQueuedRequest(String &type, String &text, uint32_t &seconds, String &source);
void CI1302I2CInit();
void CI1302SoftRearmAfterOneShot(String reason);
void TaskCI1302I2CWake(void *pvParameters);
void CI1302ScanI2CBus(bool verbose);
bool CI1302IsKnownBoardI2CAddress(uint8_t addr);
bool CI1302ProbeAddress(uint8_t addr);
void CI1302PollOneAddress(uint8_t addr);
void CI1302HandleBytes(const uint8_t *data, int len, uint8_t addr);
void CI1302HandleFrame(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t addr);
String CI1302HexBytes(const uint8_t *data, int len);
void HandleV4VoiceStatusApi();
void HandleV4VoiceWakeApi();
void HandleDoubaoAiTextApi();
void HandleDoubaoAiConfigApi();
void HandleVoiceTaskDoubaoAsrApi();
void HandleVoiceTaskDoubaoAiExecuteApi();
void HandleDoubaoTtsApi();
void HandleDoubaoTtsFileApi();
void LoadCloudConfig();
String CloudApiKey();
String CloudArkApiKey();
String CloudVoiceApiKey();
String CloudModelMode();
String CloudModelMini();
String CloudModelLite();
String CloudModelPro();
String CloudAsrResource();
String CloudTtsPublicResource();
String CloudTtsCloneResource();
String CloudPublicSpeaker();
String CloudVoiceShorekeeper();
String CloudVoiceCartethyia();
String CloudAmapWeatherKey();
String CloudAmapCityCode();
String CloudBemfaUid();
String CloudBemfaTopicCmd();
String CloudBemfaTopicStatus();
String CloudBemfaTopicLog();
void HandleCloudConfigGetApi();
void HandleCloudConfigSaveApi();


bool AuthAdminExists();
bool AuthIsValid();
bool RequireAuth();
String AuthCurrentUser();
void HandleLoginPage();
void HandleRegisterPage();
void HandleForgotPasswordPage();
void HandleAuthStatusApi();
void HandleAuthRegisterApi();
void HandleAuthLoginApi();
void HandleAuthLogoutApi();
void HandleAuthChangePasswordApi();
void HandleAuthResetMaintApi();
void HandleCmdApiAuth();
void HandleDeviceConfigSaveApiAuth();
void HandleDeviceConfigResetApiAuth();
void HandleCloudConfigSaveApiAuth();
void HandleVoicePlayApiAuth();
void HandleVoiceIoRecordApiAuth();
void HandleVoiceIoStartApiAuth();
void HandleVoiceIoStopApiAuth();
void HandleVoiceIoPlayLastApiAuth();
void HandleVoiceIoSetApiAuth();
void HandleVoiceTaskClearApiAuth();
void HandleVoiceTaskMockTextApiAuth();
void HandleVoiceTaskMockCommandApiAuth();
void HandleVoiceTaskMockReplyApiAuth();
void HandleVoiceTaskAsrApiAuth();
void HandleVoiceTaskParseApiAuth();
void HandleVoiceTaskAsrExecuteApiAuth();
void HandleVoiceTaskDoubaoAsrApiAuth();
void HandleVoiceTaskDoubaoAiExecuteApiAuth();
void HandleDoubaoAiTextApiAuth();
void HandleDoubaoTtsApiAuth();
void HandleNodeDeleteApiAuth();
void HandleNodeAddApiAuth();
void HandleNodeRecheckApiAuth();


void InitNodeDeviceFoundation();
void NodeDeviceRuntimeInit();
int NodeDeviceRuntimeFind(String nodeId);
int NodeDeviceRuntimeAlloc(String nodeId);
bool NodeDeviceRuntimeOnline(int idx);
int NodeDeviceRuntimeCount(bool onlineOnly = false);
String NodeDeviceRuntimeAgeText(unsigned long ms);
String NodeDeviceRequestBody();
String NodeDeviceRequestField(String body, const char* key, String fallback = "");
String NodeDeviceJsonArrayRaw(const String &json, const char* key);
String NodeDeviceMaskToken(String token);
int NodeDeviceEndpointCountFromJson(String endpointsJson);
String NodeDeviceRuntimeOneJson(int idx);
String NodeDeviceRuntimeListJson();
String NodeDeviceRuntimeForStaticJson(const char* nodeId);
String NodeDeviceUpsertRuntimeNode(String body, bool isReport);
void HandleNodeRegisterApi();
void HandleNodeReportApi();
void HandleNodeDeleteApi();
void HandleNodeAddApi();
void HandleNodeRecheckApi();
bool NodeDeviceHidden(String nodeId);
void NodeDeviceSetHidden(String nodeId, bool hidden);
String NodeDeviceRuntimeStateLabel(int idx);
String NodeDeviceRuntimeStateClass(int idx);
bool NodeDeviceRuntimeAbnormal(int idx);
bool NodeDeviceNodeOnline(const char* nodeId);
String NodeDeviceNodeMac(const char* nodeId);
String NodeDeviceEndpointStateJson(const LingxiEndpointInfo &ep);
String NodeDeviceTemplatesJson();
String NodeDeviceSummaryJson();
String NodeDeviceBuildJson();
String NodeDevicePageHtml();
void HandleNodeDeviceStatusApi();
void HandleNodeDevicePage();
void SendCorsHeader();
String NodeDeviceHtmlEscape(String s);
String NodeDeviceJsonField(String json, const char* key);
String NodeDeviceEndpointDisplayText(const LingxiEndpointInfo &ep);
String NodeDeviceBoolDisplay(bool on);
String NodeDeviceContactDisplay(int v);
String NodeDevicePresenceDisplay(int v);
String NodeDeviceCurtainDisplay(int percent, int state);
String NodeDeviceActionButton(const char* cmd, const char* text, bool offStyle);
String NodeDeviceRuntimeTableHtml();
int NodeDeviceFindEndpointIndex(const char* endpointId);
String DeviceConfigKey(const char* prefix, int idx);
void LoadDeviceConfig();
void ResetDeviceConfig();
bool DeviceConfigEndpointEnabled(int idx);
String DeviceConfigEndpointName(int idx);
String DeviceConfigEndpointRoom(int idx);
String DeviceConfigPhysicalModeText(const LingxiEndpointInfo &ep);
String DeviceConfigEndpointTypeText(const LingxiEndpointInfo &ep);
String DeviceConfigCategoryText(const LingxiEndpointInfo &ep);
String DeviceConfigCapabilityText(const LingxiEndpointInfo &ep);
String DeviceConfigEndpointUiStatus(const LingxiEndpointInfo &ep);
String DeviceConfigEndpointUiStatusClass(const LingxiEndpointInfo &ep);
int DeviceConfigCountEndpointType(const char* endpointType);
int DeviceConfigCountEndpointNode(const char* nodeId);
int DeviceConfigRoomList(String rooms[], int maxRooms);
String DeviceConfigStatusJson();
String DeviceConfigPageHtml();
void HandleDeviceConfigPage();
void HandleDeviceConfigStatusApi();
void HandleDeviceConfigSaveApi();
void HandleDeviceConfigResetApi();


void DeviceStatePatchInit();
void ApplyDeviceStatePatchFromCommand(ControlCommand c);


bool NodeDeviceRuntimeEndpointOnline(const char* endpointId);
void NodeDeviceApplyCompatReport(String body, String nodeId, unsigned long now);
String ControlCommandToHttpJson(ControlCommand c);
void SendControlCommandHttpCompat(ControlCommand c);
void mergeActionFeedbackToUiShadow(ActionBoardState raw);
ActionBoardState getPatchedActionStateSnapshot();
bool DeviceStatePatchIsOn(String key);
bool ReplyMeansOffOrClose(String reply);


