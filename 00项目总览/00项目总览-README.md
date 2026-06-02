# 灵汐全屋智能控制终端

基于 ESP32 的全屋智能控制中心工程包。项目包含毕业设计资料、硬件资料、ESP32-S3 中控固件、感知层节点固件、执行层节点固件、WebUI/语音/规则/云服务相关代码，以及软件架构图。

当前主线版本为 `灵汐全屋智能控制中心 公测版 V1.4.2`。中控协议已冻结，节点通过固定 HTTP API / ESP-NOW 兼容链路注册、上报和接收控制。

## 目录结构

| 目录 | 内容 |
| --- | --- |
| `01任务书` | 任务书和设计说明 |
| `02开题报告` | 开题报告 |
| `03硬件资料` | 传感器、执行器、模块数据手册和参考代码 |
| `04硬件原理图` | 硬件原理图预留目录 |
| `05硬件PCB图` | PCB 图预留目录 |
| `06灵汐公测版/Lingxi_PublicBeta_V1_4_2` | ESP32-S3 中控网关主固件 |
| `07灵汐感知层/Lingxi_IntegratedSensorNode_V1.4.2` | 集成感知节点固件 |
| `08灵汐执行层/Lingxi_ActionLayerNode_V1_4_2` | 集成执行节点固件 |
| `09灵汐子节点-感知层` | 拆分后的 C3 感知子节点 A/B |
| `10灵汐子节点-执行层` | 拆分后的执行子节点 A/B，优先使用 C3 V1.0.2 版本 |
| `11灵汐网页端` 到 `13灵汐移动端` | 前端/远程端/移动端预留目录 |
| `14软件设计` | 系统架构图、模块架构图、节点端点映射图 |
| `15联合调试` 到 `20论文终章` | 调试、论文阶段资料预留目录 |

## 推荐烧录工具链

本项目当前没有 `platformio.ini`，主流程按 Arduino IDE 组织。中控使用 Arduino 多标签页 `.ino` 工程，节点大多为单文件 `.ino` 工程。

| 项目 | 建议 |
| --- | --- |
| IDE | Arduino IDE 2.x |
| 板卡包 | `esp32 by Espressif Systems`，建议锁定同一台演示机已验证的版本 |
| 主控板卡 | `ESP32S3 Dev Module` 或实际 ESP32-S3 触摸屏开发板对应选项 |
| C3 子节点板卡 | `ESP32C3 Dev Module` 或实际 ESP32-C3 开发板对应选项 |
| 分区 | 主控建议选择 `Huge APP` / `No OTA` 类大应用分区；节点默认分区通常足够 |
| PSRAM | 主控硬件有 PSRAM 时建议开启；C3 节点无 PSRAM |
| 串口监视器 | 主控 `921600`，集成执行层 `921600`，其余节点多为 `115200` |

注意：项目根目录含中文路径。若 Arduino IDE 或第三方库在编译时出现路径编码问题，可以临时把对应固件目录复制到纯英文路径下再打开烧录。

## 依赖版本表

仓库内未锁定 Arduino IDE、ESP32 core 和第三方库版本。正式答辩或交付前，建议在下表补上实际安装版本，保证复现实验环境。

| 依赖 | 来源 | 用途 | 建议记录 |
| --- | --- | --- | --- |
| `esp32 by Espressif Systems` | Arduino Boards Manager | WiFi、WebServer、Preferences、HTTPClient、SD_MMC、ESP-NOW、I2S、LEDC、mbedtls 等 | 记录实际版本 |
| `DHT sensor library` | Arduino Library Manager / Adafruit | DHT22 温湿度传感器 | 记录实际版本，同时安装 `Adafruit Unified Sensor` |
| `Adafruit NeoPixel` | Arduino Library Manager / Adafruit | WS2812 / RGB 状态灯 | 记录实际版本 |
| `PubSubClient` | Arduino Library Manager / Nick O'Leary | 巴法云 MQTT 客户端 | 记录实际版本 |

以下库由 ESP32 Arduino core 或 Arduino 环境提供，不需要单独安装：`WiFi`、`WebServer`、`Preferences`、`HTTPClient`、`WiFiClient`、`WiFiClientSecure`、`DNSServer`、`Wire`、`FS`、`SD_MMC`、`esp_now`、`esp_wifi`、`driver/i2s`、`mbedtls`、`pgmspace`。

项目当前没有使用 `ArduinoJson`，JSON 大多由字符串函数手工拼接/解析。

## 固件清单

| 固件 | 路径 | 推荐板卡 | 串口 | 说明 |
| --- | --- | --- | --- | --- |
| 中控网关 | `06灵汐公测版/Lingxi_PublicBeta_V1_4_2/Lingxi_PublicBeta_V1_4_2.ino` | ESP32-S3 | 921600 | WebUI、认证、规则、语音、SD、MQTT、天气、节点注册中心 |
| 集成感知节点 | `07灵汐感知层/Lingxi_IntegratedSensorNode_V1.4.2/Lingxi_IntegratedSensorNode_V1.4.2.ino` | ESP32-C3 | 115200 | 1 块板承载 9 个感知端点 |
| 集成执行节点 | `08灵汐执行层/Lingxi_ActionLayerNode_V1_4_2/Lingxi_ActionLayerNode_V1_4_2.ino` | ESP32-S3 | 921600 | 1 块板承载灯、继电器、窗帘、电机、舵机 |
| 感知子节点 A | `09灵汐子节点-感知层/感知子节点A-环境窗户/Lingxi_SensorNode_C3_EnvWindow_V1_0_1_ForHubV1_4_2/Lingxi_SensorNode_C3_EnvWindow_V1_0_1_ForHubV1_4_2.ino` | ESP32-C3 | 115200 | 温湿度、燃气、窗户门磁、客厅人体 |
| 感知子节点 B | `09灵汐子节点-感知层/感知子节点B-光照安防/Lingxi_SensorNode_C3_LightSecurity_V1_0_1_ForHubV1_4_2/Lingxi_SensorNode_C3_LightSecurity_V1_0_1_ForHubV1_4_2.ino` | ESP32-C3 | 115200 | 光照、烟雾、大门门磁、卧室人体 |
| 执行子节点 A | `10灵汐子节点-执行层/执行A/Lingxi_ActionNode_C3_LightAppliance_A_V1_0_2_ForHubV1_4_2/Lingxi_ActionNode_C3_LightAppliance_A_V1_0_2_ForHubV1_4_2.ino` | ESP32-C3 | 115200 | 客厅灯、卧室灯、空调/电视/冰箱/热水器继电器 |
| 执行子节点 B | `10灵汐子节点-执行层/执行B/Lingxi_ActionNode_C3_DoorWindow_B_V1_0_2_ForHubV1_4_2/Lingxi_ActionNode_C3_DoorWindow_B_V1_0_2_ForHubV1_4_2.ino` | ESP32-C3 | 115200 | 客厅窗帘、卧室窗帘、窗户舵机 |

`10灵汐子节点-执行层/执行子节点A-灯光家电` 和 `10灵汐子节点-执行层/执行子节点B-门窗伴侣` 是早期 ESP32/S3 版本，当前拆分部署优先使用 `执行A` / `执行B` 下的 C3 V1.0.2 固件。

## 烧录前必改配置

### 中控网关

主要配置在：

```text
06灵汐公测版/Lingxi_PublicBeta_V1_4_2/00_foundation_config_models.ino
06灵汐公测版/Lingxi_PublicBeta_V1_4_2/03a_auth_core.ino
```

重点检查：

| 配置项 | 默认值/位置 | 说明 |
| --- | --- | --- |
| `AP_SSID` | `Lingxi-Gateway` | 中控默认热点名 |
| `AP_PASSWORD` | `12345678` | 中控默认热点密码，正式演示建议修改 |
| `BEMFA_UID` | `YOUR_BEMFA_UID` | 巴法云 UID |
| `BEMFA_TOPIC_CMD` | `smarthomecmd` | 云端命令主题 |
| `BEMFA_TOPIC_STATUS` | `smarthomestatus` | 云端状态主题 |
| `BEMFA_TOPIC_LOG` | `smarthomelog` | 云端日志主题 |
| `AMAP_WEATHER_KEY` | `YOUR_AMAP_WEATHER_KEY` | 高德天气 Key |
| `AMAP_CITY_CODE` | `150200` | 天气城市编码，当前为包头示例 |
| `DOUBAO_API_KEY` | `YOUR_DOUBAO_API_KEY` | 火山方舟 / 豆包 Chat API Key |
| `DOUBAO_SPEECH_*` | 见 `00_foundation_config_models.ino` | 豆包 ASR/TTS 相关 AppID、Access Token、Cluster、Speaker |
| `ASR_BRIDGE_URL` | `http://YOUR_ASR_BRIDGE_HOST:8000/asr` | 如使用桥接 ASR，需要改成实际服务地址 |
| `AUTH_RESET_CODE` | `03a_auth_core.ino` | 管理员重置维护码，不要在公开截图中展示 |

中控也提供 WebUI 云配置入口。若选择通过 WebUI 配置云服务参数，源码中的占位值可保留，但演示前必须在页面中确认已经写入。

### 感知节点

主要检查：

| 配置项 | 说明 |
| --- | --- |
| `LINGXI_DEFAULT_GATEWAY_HOST` | 默认 `192.168.4.1`，表示节点连接中控热点时的中控地址 |
| `LINGXI_DEFAULT_WIFI_SSID` / `LINGXI_DEFAULT_WIFI_PASSWORD` | 集成感知节点和感知子节点源码中仍是占位值，可通过节点配网页保存真实 WiFi |
| `LINGXI_CONFIG_AP_SSID` / `LINGXI_CONFIG_AP_PASSWORD` | 节点配网页热点名和密码，默认密码 `999999999` |
| 传感器阈值 | `SMOKE_ALARM_THRESHOLD`、`GAS_ALARM_THRESHOLD` 默认 2000，需结合现场 MQ 模块标定 |

如果节点接入中控热点，WiFi 填 `Lingxi-Gateway` 和中控 AP 密码，网关地址保持 `192.168.4.1`。如果节点和中控都接入家庭路由器，网关地址改为中控在局域网中的 IP。

### 执行节点

主要检查：

| 配置项 | 说明 |
| --- | --- |
| `LINGXI_DEFAULT_GATEWAY_HOST` | 默认 `192.168.4.1` |
| `LINGXI_CONFIG_AP_SSID` / `LINGXI_CONFIG_AP_PASSWORD` | 节点配网页热点，默认密码 `999999999` |
| 灯带数量 | `LIVING_LIGHT_NUM`、`BEDROOM_LIGHT_NUM` 默认 8 |
| 继电器触发电平 | C3 执行 A 使用 `RELAY_ON_LEVEL LOW`、`RELAY_OFF_LEVEL HIGH` |
| 窗帘步数 | `CURTAIN_TOTAL_STEPS` 默认 4096 |
| 窗户舵机 | `WINDOW_SERVO_USE_180` 默认 1，按 180 度位置舵机处理 |

执行节点只输出控制信号，不给继电器线圈、电机和舵机供电。继电器、电机、舵机必须使用稳定外部 5V，并与 ESP32 共地。

## SD 卡资源布局

中控语音链路使用 SD_MMC，推荐 FAT32 格式。SD 根目录建议按以下结构准备：

```text
/record
/voice/shorekeeper
/voice/cartethyia
/roles/shorekeeper
/roles/cartethyia
```

说明：

- `/record` 用于录音文件，例如 `/record/last.wav`。
- `/voice/<role>` 存放角色语音包。
- `/roles/<role>` 存放角色资源，例如 `avatar.webp`、`portrait.webp`、`role.json`。
- `/roles` 必须位于 SD 根目录，不要放到 `/voice/roles` 下。

中控启动后可访问 `/api/storage/diagnose` 检查 SD、语音包和角色资源状态。

## 烧录顺序

1. 安装 Arduino IDE、ESP32 板卡包和依赖库。
2. 先烧录中控网关，打开 `06灵汐公测版/Lingxi_PublicBeta_V1_4_2/Lingxi_PublicBeta_V1_4_2.ino`。
3. 中控启动后连接 `Lingxi-Gateway`，浏览器打开 `http://192.168.4.1`。
4. 首次进入 WebUI，注册本地管理员账号。
5. 在 WebUI 中配置 WiFi、云服务、天气、语音等参数，或在源码中提前写入。
6. 根据部署方案烧录节点：
   - 集成方案：烧录 `07灵汐感知层` 和 `08灵汐执行层`。
   - 拆分方案：烧录 `09` 下感知子节点 A/B，烧录 `10/执行A` 和 `10/执行B`。
7. 节点首次启动若没有有效 WiFi 配置，会开启本地配网热点。连接节点热点，进入配网页，填写 WiFi、网关地址。
8. 回到中控 WebUI 的设备/节点页面，确认节点在线、端点出现、状态更新正常。
9. 再做传感器触发、灯光/继电器/窗帘/舵机控制、语音指令、天气和 MQTT 联调。

## Arduino IDE 烧录步骤

对每个固件目录重复以下流程：

1. 打开对应 `.ino` 文件。中控必须打开 `Lingxi_PublicBeta_V1_4_2.ino`，Arduino IDE 会自动合并同目录其他 `.ino` 标签页。
2. 在 `工具 -> 开发板` 中选择对应 ESP32-S3 或 ESP32-C3 板卡。
3. 设置分区方案。中控优先选择大应用分区，避免 WebUI gzip 资源导致程序过大。
4. 选择串口端口。
5. 点击编译，确认无库缺失或程序大小超限。
6. 点击上传。
7. 打开串口监视器，按固件清单设置波特率。

如出现 `Multiple libraries were found` 或库版本冲突，优先保留 Arduino IDE 库管理器安装的正式版本，移除旧的手工复制库。

## 节点端点映射

中控端点定义集中在：

```text
06灵汐公测版/Lingxi_PublicBeta_V1_4_2/00_foundation_config_models.ino
```

感知端点：

| 端点 ID | 设备 |
| --- | --- |
| `temperature` | 室内温度 |
| `humidity` | 室内湿度 |
| `light_level` | 环境光照 |
| `door_main_contact` | 大门门磁 |
| `window_contact` | 窗户门磁 |
| `human_living` | 客厅人体 |
| `human_bedroom` | 卧室人体 |
| `smoke` | 烟雾传感器 |
| `gas` | 燃气传感器 |

执行端点：

| 端点 ID | 设备 |
| --- | --- |
| `living_light` | 客厅灯 |
| `bedroom_light` | 卧室灯 |
| `living_curtain` | 客厅窗帘 |
| `bedroom_curtain` | 卧室窗帘 |
| `window_servo` | 窗户舵机 |
| `air_relay` | 空调电源 |
| `tv_relay` | 电视电源 |
| `fridge_relay` | 冰箱电源 |
| `water_heater_relay` | 热水器电源 |

这些 ID 是中控、WebUI、规则引擎、语音解析和节点固件之间的协议。除非同步修改全部模块，否则不要改端点 ID、节点 ID、HTTP 路径和上报字段名。

## 关键 API

| 路径 | 说明 |
| --- | --- |
| `/api/status` | 中控状态 |
| `/api/logs` | 日志 |
| `/api/node_device/status` | 节点和端点状态 |
| `/api/node/register` | 节点注册 |
| `/api/node/report` | 节点状态上报 |
| `/api/device_config/status` | 设备配置状态 |
| `/api/cmd` | 统一控制入口 |
| `/api/weather/status` | 天气状态 |
| `/api/cloud_config` | 云服务配置 |
| `/api/voice/status` | 语音状态 |
| `/api/voice_io/status` | 录音/播放状态 |
| `/api/voice_task/status` | 语音任务状态 |
| `/api/storage/diagnose` | SD 卡和角色资源诊断 |
| `/api/engineering/status` | 工程状态说明 |
| `/api/engineering/api_contract` | API 契约说明 |

需要登录权限的接口在代码中通常有 `Auth` 后缀包装，例如 `HandleCmdApiAuth`、`HandleCloudConfigSaveApiAuth`。

## 联调检查清单

- 中控串口启动日志出现 WebServer、ESP-NOW、MQTT、Weather、SD Voice 等初始化信息。
- 手机或电脑能连接 `Lingxi-Gateway` 并打开 `http://192.168.4.1`。
- 首次注册管理员账号后，能进入主控台。
- `/api/storage/diagnose` 返回 SD 卡在线，角色资源路径正确。
- 节点配网后，在中控节点页面显示在线。
- 感知节点触发门磁、人体、烟雾、燃气等状态变化后，中控页面能更新。
- 执行节点控制灯光、继电器、窗帘、电机、舵机时，状态能回传。
- 天气配置后 `/api/weather/status` 有城市和天气数据。
- 巴法云配置后 MQTT 命令、状态、日志主题能收发。
- 豆包配置后 ASR、TTS、自由对话和白名单控制路径能跑通。

## 当前验证状态

项目内已有中控静态检查报告：

```text
06灵汐公测版/Lingxi_PublicBeta_V1_4_2/STATIC_CHECK_REPORT.txt
```

报告显示中控 `.ino` 花括号配平、Chat API 路径、WebUI gzip 资源、版本标识等静态检查通过。

当前工程目录没有 Git 仓库，也没有 `platformio.ini`。本次 README 整理未在本机进行 Arduino 编译或实物烧录验证；正式答辩前建议在实际烧录机上逐个固件编译一次，并把工具链版本补入依赖版本表。

## 维护建议

- 建议初始化 Git 仓库，至少保存 `README.md`、主控固件、当前 C3 子节点固件和软件设计图。
- 建议新增 `docs/烧录记录.md`，记录每次烧录的板卡、端口、库版本、编译结果和硬件现象。
- 建议把云服务 Key、管理员维护码、热点密码作为答辩私密项管理，不放入公开论文截图。
- 建议以 `00_foundation_config_models.ino` 中的端点表为协议源头，新增节点时只注册新节点承载关系，不随意重命名既有端点。
