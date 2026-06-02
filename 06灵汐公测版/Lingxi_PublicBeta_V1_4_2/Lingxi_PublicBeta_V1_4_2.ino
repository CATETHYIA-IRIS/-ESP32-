/*
 * 灵汐全屋智能控制中心
 * 公开版本：公测版 V1.4.2
 *
 * 本页是 Arduino IDE 打开的主标签页，只作为烧录目录和工程说明使用。
 * 业务代码分布在同目录其他 .ino 标签页中，Arduino IDE 会把这些标签页合并编译。
 * 可修改：本页说明文字、公开版本说明、后续维护备注。
 * 禁止修改：不要在本页新增设备控制、网络任务、语音任务或全局业务变量。
 *
 * 文件目录：
 * 00_foundation_config_models.ino       基础配置、版本号、数据模型、节点端点模板。
 * 01_webui_assets_part1.ino             WebUI 首页 gzip 资源第一段，含浏览器播放映射补丁。
 * 02_webui_assets_part2.ino             WebUI 首页 gzip 资源第二段。
 * 03_json_html_utils_storage_base.ino   JSON/HTML 工具、存储基础函数。
 * 03a_auth_core.ino                     登录认证、管理员和会话控制。
 * 04_role_voice_audio_core.ino          角色资源、SD 语音包和音频播放核心。
 * 05_asr_tts_chat_core.ino              ASR、TTS、聊天任务和语音状态。
 * 06_node_device_core.ino               节点注册、端点模板和设备运行态。
 * 07_pages_settings_part1.ino           设置页前半部分、本地白名单语音命令。
 * 08_pages_settings_part2.ino           设置页后半部分、模型配置和对话相关函数。
 * 09_cloud_weather_mqtt_core.ino        天气、云平台、MQTT/Bemfa。
 * 10_rules_logs_control_core.ino        日志、规则、场景和统一控制入口。
 * 11_voice_api_handlers_core.ino        语音相关 Web API。
 * 12_device_api_handlers_core.ino       设备状态、设备控制和节点状态 API。
 * 13_web_routes_core.ino                WebServer 路由注册表。
 * 14_setup_loop_tasks.ino               setup、loop 和定时任务。
 */

// 主入口页不放业务代码。实际初始化入口在 14_setup_loop_tasks.ino 的 setup() 中。
