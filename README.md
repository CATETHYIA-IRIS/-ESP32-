灵汐全屋智能控制中心 (Lingxi Smart Home Control Center)

公测版 V1.4.2
一套基于 ESP32 系列芯片的全屋智能网关系统，集设备控制、环境感知、语音交互、云端服务与角色化 AI 对话于一体。

📖 项目简介
灵汐是一个面向全屋智能的开源控制中心，采用“中控网关 + 感知节点 + 执行节点”的三层分布式架构。  
它通过 WiFi + ESP‑NOW与子节点通信，提供 WebUI 控制台、本地语音交互（ASR/TTS）、角色化 AI 对话（豆包大模型）、高德天气、巴法云 MQTT 远程控制等功能。

系统已在公测版 V1.4.2 中稳定运行，支持：
- 环境数据采集（温湿度、光照、门磁、人体红外、烟雾、燃气）
- 设备执行（灯光、窗帘、窗户、继电器电源）
- 本地白名单与确认代理（语音指令智能路由）
- 豆包自由对话（闲聊、查询、非设备控制）
- 角色语音（守岸人 / 卡提希娅 音色切换）
- 离线语音包播放（SD 卡加载）
- 完整 Web 管理界面（设备管理、房间分配、节点注册、系统设置）
> 本项目适合智能家居爱好者、物联网开发者、嵌入式学习者和开源社区共建。

✨ 功能特性
    🧠 核心能力
        分布式节点架构：中控（ESP32-S3）+ 感知节点（ESP32-C3）+ 执行节点（ESP32-C3/S3），可扩展多子节点
        多协议通信：WiFi STA/AP、ESP‑NOW（低延迟控制）、HTTP（节点注册/上报）、MQTT（巴法云远程）
        实时状态同步：节点定期上报，中控汇总并推送 WebUI / MQTT
        规则引擎：自动场景（离家、回家、睡眠等）、安防联动（烟雾/燃气报警）
     🎙️ 语音与 AI
        本地语音识别（ASR）：通过 I2S 麦克风（ES7210）录音，调用豆包录音文件识别 API
        语音合成（TTS）：豆包 TTS，支持角色音色（守岸人 / 卡提希娅）及公版兜底
        角色化对话：本地白名单 + 确认代理 + 豆包自由对话，自然语言控制设备
        离线语音包：SD 卡存放 WAV 语音包，支持中控本机播放（ES8311）
    🌐 云端集成
        巴法云 MQTT：远程命令、状态上报、日志同步
        高德天气：实时天气、预报、生活建议
        豆包大模型：自由对话、智能问答
    🖥️ Web 管理界面
        设备管理（感知/执行设备列表、房间分配）
        节点注册表（查看/删除/添加节点）
        系统设置（WiFi、热点、云密钥、语音配置）
        实时状态看板（环境数据、设备状态、日志）
        角色资源管理（SD 卡角色图片/语音包）
        
🛠️ 硬件要求
       中控网关 (ESP32-S3)
- 推荐使用 ESP32-S3 开发板（如 ESP32-S3-DevKitC-1）
- 板载音频编解码器：ES8311（扬声器）和 ES7210（麦克风）
- SD 卡槽（用于语音包和角色资源）
- I2C 扩展 IO（可控制屏幕背光等）
如果使用其他 ESP32 系列（如 ESP32），需调整部分引脚定义（见代码注释）。
       感知节点 (ESP32-C3)
- ESP32-C3 开发板
- DHT22（温湿度）GPIO4
- BH1750（光照）I2C（SDA=6, SCL=7）
- 门磁、人体红外等数字传感器
        执行节点 (ESP32-C3 / S3)
- 灯光：WS2812 灯带（IO10/IO4）
- 继电器：低电平触发（IO2/IO3/IO0/IO1）
- 步进电机（ULN2003）
- 舵机（180°）
> 详细引脚定义请参考各 `.ino` 文件中的 `#define` 部分。

 📂 项目结构
Lingxi_PublicBeta_V1_4_2/
>├── 00_foundation_config_models.ino      # 基础配置、数据模型、节点模板
>├── 01_webui_assets_part1.ino            # WebUI 首页 gzip 资源 (前半)
>├── 02_webui_assets_part2.ino            # WebUI 首页 gzip 资源 (后半)
>├── 03_json_html_utils_storage_base.ino  # JSON/HTML 工具、存储基类
>├── 03a_auth_core.ino                    # 登录认证、管理员管理
>├── 04_role_voice_audio_core.ino         # 角色资源、语音包播放
>├── 05_asr_tts_chat_core.ino             # ASR/TTS、对话任务状态
>├── 06_node_device_core.ino              # 节点注册、端点模板
>├── 07_pages_settings_part1.ino          # 系统设置页（前半）
>├── 08_pages_settings_part2.ino          # 系统设置页（后半）+ 豆包配置
>├── 09_cloud_weather_mqtt_core.ino       # 天气、MQTT 巴法云
>├── 10_rules_logs_control_core.ino       # 规则引擎、日志、统一控制
>├── 11_voice_api_handlers_core.ino       # 语音相关 Web API
>├── 12_device_api_handlers_core.ino      # 设备状态与控制 API
>├── 13_web_routes_core.ino               # Web 路由注册表
>├── 14_setup_loop_tasks.ino              # 启动、循环、任务调度
>├── Lingxi_PublicBeta_V1_4_2.ino         # 主入口说明（无代码）
>├── STATIC_CHECK_REPORT.c                # 静态检查报告
>└── README.md                            # 本文件
> 注意：Arduino IDE 会把所有 `.ino` 文件合并编译，无需手动包含。
🔧 烧录与配置

1. 开发环境
- Arduino IDE 2.x 或 PlatformIO
- 安装 ESP32 开发板支持包（esp32 版本 >= 2.0.14）
- 安装所需库（见下方）
2. 依赖库（需要手动安装）
- `WiFi.h`, `HTTPClient.h`, `WebServer.h`, `DNSServer.h`, `Preferences.h`
- `Adafruit_NeoPixel.h`
- `DHT.h` (DHT sensor library)
- `Wire.h`
- `PubSubClient.h` (MQTT)
- `FS.h`, `SD_MMC.h`
- `esp_now.h`, `esp_wifi.h`
- `mbedtls/base64.h`, `mbedtls/sha256.h`
> 可通过 Arduino Library Manager 搜索安装，或直接从 GitHub 获取。
 3. 配置 WiFi / 密钥
首次上电后，中控会开启热点 `灵汐-执行A灯光家电`（密码 `999999999`）或 `Lingxi-Gateway`（密码 `12345678`），连接后访问 `192.168.4.1` 进入 WebUI，在「系统设置」页面填写：
- **WiFi 名称/密码**（STA 模式）
- **巴法云 UID / 主题**（如需远程控制）
- **高德天气 Key / 城市编码**（如需天气）
- **豆包 API Key**（如需自由对话和语音合成）
 4. 烧录
- 分别编译并烧录中控、感知节点、执行节点的固件（每个节点独立 `.ino` 文件）。
- 中控固件为 `Lingxi_PublicBeta_V1_4_2.ino`（实际代码分散在多个 `.ino` 中，Arduino IDE 会自动合并）。
- 节点固件请参考同目录下 `Lingxi_SensorNode_*.ino` 和 `Lingxi_ActionNode_*.ino`。

📡 API 概览
| 端点                        | 方法   |说明 |
|-----------------------------|------ |-----|
| `/`                         | GET   | 主 WebUI 页面（需认证）|
| `/api/status`               | GET   | 获取系统状态（温度、湿度、设备状态等）|
| `/api/cmd?cmd=<command>`    | GET   | 执行命令（如 `light_living_on`）|
| `/api/node/register`        | POST  | 节点注册（JSON body）|
| `/api/node/report`          | POST  | 节点数据上报（JSON body） |
| `/api/device_config/status` | GET   | 获取设备配置（端点列表、房间等） |   
| `/api/voice_io/record`      | GET   | 录音（麦克风）|
| `/api/doubao_ai/text`       | GET   | 豆包自由对话 |
| `/api/doubao_tts/say`       | GET   | 生成 TTS 语音 |
| `/api/role_asset`           | GET   | 获取角色图片（avatar/portrait） |
| 更多 API 请查看 `13_web_routes_core.ino` | | |

🚀 使用指南
首次启动
1. 给中控上电，连接热点（SSID 见串口打印）。
2. 浏览器打开 `192.168.4.1`，进入注册页面，创建管理员账号。
3. 登录后，进入「系统设置」配置 WiFi、云服务等。
4. 将感知节点和执行节点上电，它们会自动注册到中控（通过 `/api/node/register`）。
5. 在「设备管理」页面为设备分配房间。
6. 在「设备控制」页面测试开关灯、窗帘等。
语音交互
- 网页端点击「智慧语音」标签，按住“按住说话”按钮录音，或输入文字。
- 中控麦克风可通过 `voice_io` API 录音，支持录音文件识别（豆包 ASR）。
- 语音命令支持本地白名单（如“打开客厅灯”）和自由对话（如“今天天气怎么样？”）。
 角色切换
- 在「系统设置 → 角色资源配置」中启用角色（守岸人 / 卡提希娅）。
- 语音回复会优先使用角色音色（需配置豆包 TTS 资源）。

🤝 开发与贡献
欢迎提交 Issue、Pull Request 或加入讨论。
代码风格
- 使用 Arduino 风格（`.ino` 文件）。
- 注释采用中文，便于阅读。
- 所有配置宏定义在头部，便于修改。
贡献指南
1. Fork 本项目。
2. 创建您的特性分支 (`git checkout -b feature/AmazingFeature`)。
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)。
4. 推送到分支 (`git push origin feature/AmazingFeature`)。
5. 打开 Pull Request。

🙏 致谢
- [Espressif](https://www.espressif.com/) 提供优秀的 ESP32 平台
- [巴法云](https://bemfa.com/) 提供免费 MQTT 服务
- [高德地图](https://lbs.amap.com/) 提供天气 API
- [火山引擎](https://www.volcengine.com/) 提供豆包大模型、语音服务


灵汐 取自“心有灵犀（汐），一点通”，愿这套开源系统能为你的智能家居带来更多想象力。
如有问题，请提交 Issue 或联系维护者。  
祝使用愉快！ 🌟
