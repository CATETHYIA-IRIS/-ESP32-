/*
 * 文件：14_setup_loop_tasks.ino
 * 标题：启动、循环与定时任务
 * 说明：负责 setup、loop、定时轮询、初始化顺序和运行维护任务。
 * 可改：可调轮询间隔
 * 禁止：大改初始化顺序和主循环阻塞逻辑。
 */

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String StorageDiagPathJson(const char* key, const char* path) {
  bool exists = false;
#if ENABLE_SD_VOICE
  exists = Voice.sdOnline && SD_MMC.exists(path);
#endif
  String j = "\"" + String(key) + "\":{";
  j += "\"path\":\"" + JsonEscape(String(path)) + "\",";
  j += "\"exists\":" + String(exists ? "true" : "false");
  j += "}";
  return j;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleStorageDiagnoseApi() {
  SendCorsHeader();
  String json = "{";
  json += "\"ok\":" + String(Voice.sdOnline ? "true" : "false") + ",";
  json += "\"sdOnline\":" + String(Voice.sdOnline ? "true" : "false") + ",";
  json += "\"lastVoiceError\":\"" + JsonEscape(Voice.lastVoiceError) + "\",";
  json += "\"protocol\":\"SD root must contain /record, /voice and /roles. /roles must NOT be inside /voice.\",";
  json += "\"expectedLayout\":[\"/record\",\"/voice/shorekeeper\",\"/voice/cartethyia\",\"/roles/shorekeeper\",\"/roles/cartethyia\"],";
  json += "\"paths\":{";
  json += StorageDiagPathJson("record", VOICE_RECORD_DIR); json += ",";
  json += StorageDiagPathJson("voice_root", VOICE_ROOT_DIR); json += ",";
  json += StorageDiagPathJson("voice_shorekeeper", ROLE_SHOREKEEPER_DIR); json += ",";
  json += StorageDiagPathJson("voice_cartethyia", ROLE_CARTETHYIA_DIR); json += ",";
  json += StorageDiagPathJson("roles_root", ROLE_ASSET_ROOT_DIR); json += ",";
  json += StorageDiagPathJson("roles_shorekeeper", ROLE_SHOREKEEPER_ASSET_DIR); json += ",";
  json += StorageDiagPathJson("roles_cartethyia", ROLE_CARTETHYIA_ASSET_DIR); json += ",";
  json += StorageDiagPathJson("shorekeeper_avatar", "/roles/shorekeeper/avatar.webp"); json += ",";
  json += StorageDiagPathJson("shorekeeper_portrait", "/roles/shorekeeper/portrait.webp"); json += ",";
  json += StorageDiagPathJson("shorekeeper_role_json", "/roles/shorekeeper/role.json"); json += ",";
  json += StorageDiagPathJson("cartethyia_avatar", "/roles/cartethyia/avatar.webp"); json += ",";
  json += StorageDiagPathJson("cartethyia_portrait", "/roles/cartethyia/portrait.webp"); json += ",";
  json += StorageDiagPathJson("cartethyia_role_json", "/roles/cartethyia/role.json");
  json += "},";
  json += "\"hint\":\"If avatar/portrait is missing, move /voice/roles to /roles at SD root, then reboot and clear browser cache.\"";
  json += "}";
  server.send(200, "application/json", json);
}


// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String JsonArrayFromLiteralList(const char* const* items, int count) {
  String j = "[";
  for (int i = 0; i < count; ++i) {
    if (i) j += ",";
    j += "\"" + JsonEscape(String(items[i])) + "\"";
  }
  j += "]";
  return j;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleEngineeringStatusApi() {
  SendCorsHeader();
  static const char* mainline[] = {
    "V5 WebUI",
    "V5 Voice Experience",
    "SensorNode V3",
    "ActionNode V3",
    "SD role resources",
    "ASR/TTS/Board speaker/Web replay",
    "Local whitelist + Confirm rule layer",
    "Weather + MQTT/Bemfa + Settings + Logs"
  };
  static const char* legacy[] = {
    "V4 local wake route",
    "CI1302 mainline integration",
    "Streaming ASR",
    "Continuous listening",
    "Chat mode / continuous talk mode",
    "ESP-SR local wake as mainline",
    "V4 wake/say/record serial commands",
    "V4 voice APIs"
  };
  static const char* diagnostics[] = {
    "/api/storage/diagnose",
    "/api/role/status",
    "/api/role/library",
    "/api/voice/status",
    "/api/voice_io/diag",
    "/api/voice_task/status",
    "/api/logs"
  };

  String json = "{";
  json += "\"ok\":true,";
  json += "\"product\":\"" + JsonEscape(String(LINGXI_PRODUCT_NAME)) + "\",";
  json += "\"release\":\"" + JsonEscape(String(LINGXI_RELEASE_NAME)) + "\",";
  json += "\"version\":\"" + JsonEscape(String(LINGXI_VERSION_CODE)) + "\",";
  json += "\"patch\":\"" + JsonEscape(String(LINGXI_VERSION_PATCH)) + "\",";
  json += "\"baseline\":\"" + JsonEscape(String(LINGXI_BASELINE_VERSION)) + "\",";
  json += "\"stableAnchor\":\"V6.0.1 tested mainline + V6.0.3 engineering diagnostics, V6.0.2 audio pop patch not required as separate gate\",";
  json += "\"uiPolicy\":\"Keep V5 UI and user experience. Rewrite backend through stable API contracts only.\",";
  json += "\"voicePolicy\":\"V5 voice chain is mainline: record, ASR, TTS, board speaker, web replay and role voice pack must be preserved.\",";
  json += "\"storagePolicy\":\"/record, /voice and /roles are SD root siblings. /roles must not be inside /voice.\",";
  json += "\"legacyPolicy\":\"V4, CI1302, streaming ASR, continuous listening and ESP-SR local wake are archived as failed exploration, not mainline.\",";
  json += "\"mainlineModules\":" + JsonArrayFromLiteralList(mainline, sizeof(mainline)/sizeof(mainline[0])) + ",";
  json += "\"legacyArchive\":" + JsonArrayFromLiteralList(legacy, sizeof(legacy)/sizeof(legacy[0])) + ",";
  json += "\"diagnosticApis\":" + JsonArrayFromLiteralList(diagnostics, sizeof(diagnostics)/sizeof(diagnostics[0]));
  json += "}";
  server.send(200, "application/json", json);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleApiContractApi() {
  SendCorsHeader();
  String json = "{";
  json += "\"ok\":true,";
  json += "\"note\":\"V6 freezes V5 UI-facing API behavior before deeper backend refactor.\",";
  json += "\"groups\":{";
  json += "\"system\":[\"/api/status\",\"/api/logs\",\"/api/engineering/status\"],";
  json += "\"node\":[\"/api/node_device/status\",\"/api/node/register\",\"/api/node/report\"],";
  json += "\"device\":[\"/api/device_config/status\",\"/api/cmd\"],";
  json += "\"role\":[\"/api/role/status\",\"/api/role/library\",\"/api/role_asset\",\"/asset/role_shouanren.png\",\"/asset/role_cartethyia.png\"],";
  json += "\"storage\":[\"/api/storage/diagnose\"],";
  json += "\"voice\":[\"/api/voice/status\",\"/api/voice/file\",\"/api/voice/play\",\"/api/voice_io/status\",\"/api/voice_io/record\",\"/api/voice_task/status\",\"/api/doubao_tts/say\",\"/api/doubao_tts/file\"],";
  json += "\"cloud\":[\"/api/weather/status\",\"/api/cloud_config\",\"/api/doubao_ai/text\",\"/api/doubao_ai/config\"]";
  json += "}}";
  server.send(200, "application/json", json);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void SendRoleSvg(const char *name, const char *mainColor, const char *subColor) {
  String svg = "<svg xmlns='http://www.w3.org/2000/svg' width='360' height='520' viewBox='0 0 360 520'>";
  svg += "<defs><linearGradient id='g' x1='0' y1='0' x2='1' y2='1'><stop offset='0' stop-color='" + String(mainColor) + "'/><stop offset='1' stop-color='" + String(subColor) + "'/></linearGradient></defs>";
  svg += "<rect width='360' height='520' rx='34' fill='#071126'/><circle cx='180' cy='188' r='136' fill='url(#g)' opacity='.88'/>";
  svg += "<circle cx='180' cy='145' r='64' fill='rgba(255,255,255,.72)'/><path d='M96 430c22-104 50-164 84-164s62 60 84 164z' fill='rgba(255,255,255,.72)'/>";
  svg += "<text x='180' y='470' fill='#fff' font-size='32' font-weight='700' text-anchor='middle'>" + String(name) + "</text>";
  svg += "<text x='180' y='505' fill='#a9c8ff' font-size='18' text-anchor='middle'>replace with PNG asset</text></svg>";
  SendCorsHeader();
  server.send(200, "image/svg+xml", svg);
}

void HandleAssetRole1() { if (!TryServeRoleAsset(ROLE_SHOREKEEPER, "portrait")) SendRoleSvg("守岸人", "#3ba7ff", "#10295b"); }
void HandleAssetRole2() { if (!TryServeRoleAsset(ROLE_CARTETHYIA, "portrait")) SendRoleSvg("卡提希娅", "#f1d58a", "#416bff"); }
void HandleAssetThumb1() { if (!TryServeRoleAsset(ROLE_SHOREKEEPER, "avatar")) SendRoleSvg("守岸人", "#3ba7ff", "#10295b"); }
void HandleAssetThumb2() { if (!TryServeRoleAsset(ROLE_CARTETHYIA, "avatar")) SendRoleSvg("卡提希娅", "#f1d58a", "#416bff"); }

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitWebServer() {
  
  static const char* authHeaderKeys[] = {"Cookie"};
  server.collectHeaders(authHeaderKeys, 1);

  server.on("/", HTTP_GET, HandleRoot);
  server.on("/login", HTTP_GET, HandleLoginPage);
  server.on("/register", HTTP_GET, HandleRegisterPage);
  server.on("/forgot", HTTP_GET, HandleForgotPasswordPage);
  server.on("/api/auth/status", HTTP_GET, HandleAuthStatusApi);
  server.on("/api/auth/register", HTTP_POST, HandleAuthRegisterApi);
  server.on("/api/auth/register", HTTP_GET, HandleAuthRegisterApi);
  server.on("/api/auth/login", HTTP_POST, HandleAuthLoginApi);
  server.on("/api/auth/login", HTTP_GET, HandleAuthLoginApi);
  server.on("/api/auth/logout", HTTP_GET, HandleAuthLogoutApi);
  server.on("/api/auth/change_password", HTTP_GET, HandleAuthChangePasswordApi);
  server.on("/api/auth/reset_maint", HTTP_GET, HandleAuthResetMaintApi);
  server.on("/api/status", HTTP_GET, HandleStatusApi);
  server.on("/api/weather/status", HTTP_GET, HandleWeatherStatusApi);
  server.on("/nodes", HTTP_GET, HandleNodeDevicePage);
  server.on("/device-config", HTTP_GET, HandleDeviceConfigPage);
  server.on("/device-manager", HTTP_GET, HandleDeviceConfigPage);
  server.on("/api/device_config/status", HTTP_GET, HandleDeviceConfigStatusApi);
  server.on("/api/device_config/save", HTTP_GET, HandleDeviceConfigSaveApiAuth);
  server.on("/api/device_config/reset", HTTP_GET, HandleDeviceConfigResetApiAuth);
  server.on("/api/node_device/status", HTTP_GET, HandleNodeDeviceStatusApi);
  server.on("/api/node_device", HTTP_GET, HandleNodeDeviceStatusApi);
  server.on("/api/node/register", HTTP_POST, HandleNodeRegisterApi);
  server.on("/api/node/register", HTTP_GET, HandleNodeRegisterApi);
  server.on("/api/node/report", HTTP_POST, HandleNodeReportApi);
  server.on("/api/node/report", HTTP_GET, HandleNodeReportApi);
  server.on("/api/node/delete", HTTP_GET, HandleNodeDeleteApiAuth);
  server.on("/api/node/add", HTTP_GET, HandleNodeAddApiAuth);
  server.on("/api/node/recheck", HTTP_GET, HandleNodeRecheckApiAuth);
  server.on("/api/logs", HTTP_GET, HandleLogsApi);
  server.on("/api/cmd", HTTP_GET, HandleCmdApiAuth);
  server.on("/api/wifi/scan", HTTP_GET, HandleWifiScanApi);
  server.on("/api/voice/status", HTTP_GET, HandleVoiceStatusApi);
  server.on("/api/role/status", HTTP_GET, HandleRoleStatusApi);
  server.on("/api/role/library", HTTP_GET, HandleRoleLibraryApi);
  server.on("/api/storage/diagnose", HTTP_GET, HandleStorageDiagnoseApi);
  server.on("/api/engineering/status", HTTP_GET, HandleEngineeringStatusApi);
  server.on("/api/engineering/api_contract", HTTP_GET, HandleApiContractApi);
  server.on("/api/role_asset", HTTP_GET, HandleRoleAssetApi);
  server.on("/api/role/asset", HTTP_GET, HandleRoleAssetApi);
  server.on("/api/voice/file", HTTP_GET, HandleVoiceFileApi);
  server.on("/api/voice/play", HTTP_GET, HandleVoicePlayApiAuth);
  server.on("/api/voice/list", HTTP_GET, HandleVoiceListApi);
  server.on("/api/voice_io/status", HTTP_GET, HandleVoiceIoStatusApi);
  server.on("/api/voice_io/record", HTTP_GET, HandleVoiceIoRecordApiAuth);
  server.on("/api/voice_io/start", HTTP_GET, HandleVoiceIoStartApiAuth);
  server.on("/api/voice_io/stop", HTTP_GET, HandleVoiceIoStopApiAuth);
  server.on("/api/voice_io/file", HTTP_GET, HandleVoiceIoFileApi);
  server.on("/api/voice_io/diag", HTTP_GET, HandleVoiceIoDiagApi);
  server.on("/api/voice_io/play_last", HTTP_GET, HandleVoiceIoPlayLastApiAuth);
  server.on("/api/voice_io/set", HTTP_GET, HandleVoiceIoSetApiAuth);
  server.on("/api/voice_task/status", HTTP_GET, HandleVoiceTaskStatusApi);
  server.on("/api/voice_task/clear", HTTP_GET, HandleVoiceTaskClearApiAuth);
  server.on("/api/voice_task/mock_text", HTTP_GET, HandleVoiceTaskMockTextApiAuth);
  server.on("/api/voice_task/mock_command", HTTP_GET, HandleVoiceTaskMockCommandApiAuth);
  server.on("/api/voice_task/mock_reply", HTTP_GET, HandleVoiceTaskMockReplyApiAuth);
  server.on("/api/voice_task/asr", HTTP_GET, HandleVoiceTaskAsrApiAuth);
  server.on("/api/voice_task/parse", HTTP_GET, HandleVoiceTaskParseApiAuth);
  server.on("/api/voice_task/asr_execute", HTTP_GET, HandleVoiceTaskAsrExecuteApiAuth);
  server.on("/api/doubao_ai/text", HTTP_GET, HandleDoubaoAiTextApiAuth);
  server.on("/api/doubao_ai/config", HTTP_GET, HandleDoubaoAiConfigApi);
  server.on("/api/cloud_config", HTTP_GET, HandleCloudConfigGetApi);
  server.on("/api/cloud_config/save", HTTP_GET, HandleCloudConfigSaveApiAuth);
  server.on("/api/voice_task/doubao_asr", HTTP_GET, HandleVoiceTaskDoubaoAsrApiAuth);
  server.on("/api/voice_task/doubao_ai_execute", HTTP_GET, HandleVoiceTaskDoubaoAiExecuteApiAuth);
  
  server.on("/api/doubao_tts/say", HTTP_GET, HandleDoubaoTtsApiAuth);
  server.on("/api/doubao_tts/file", HTTP_GET, HandleDoubaoTtsFileApi);
  server.on("/asset/role_shouanren.png", HTTP_GET, HandleAssetRole1);
  server.on("/asset/role_cartethyia.png", HTTP_GET, HandleAssetRole2);
  server.on("/asset/role_shouanren_thumb.png", HTTP_GET, HandleAssetThumb1);
  server.on("/asset/role_cartethyia_thumb.png", HTTP_GET, HandleAssetThumb2);

  server.begin();

  UF_Log("OK", "WEB", "WebServer started with V7 auth gate");
  UF_Log("INFO", "AP", "Connect WiFi: " + NetApSsid());
  UF_Log("INFO", "WEBUI", "Open browser: http://192.168.4.1");
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void RefreshEspNowPeer(uint8_t *mac) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  
  esp_now_del_peer(mac);
  esp_err_t result = esp_now_add_peer(&peerInfo);
  if (result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST) {
    Serial.println("[ESP-NOW Peer] refreshed");
  } else {
    Serial.println("[ESP-NOW Peer] refresh failed: " + String((int)result));
  }
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void RefreshEspNowAllPeers() {
  RefreshEspNowPeer(sensorBoardMac);
  RefreshEspNowPeer(actionBoardMac);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void AddPeer(uint8_t *mac) {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  
  
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&peerInfo);

  if (result == ESP_OK) {
    Serial.println("Peer added OK");
  } else if (result == ESP_ERR_ESPNOW_EXIST) {
    esp_now_mod_peer(&peerInfo);
    Serial.println("Peer already exists, channel updated");
  } else {
    Serial.println("Peer add failed");
  }
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitEspNow() {
  WiFi.mode(WIFI_AP_STA);

  bool apOk = StartOrUpdateSoftAp();

  Serial.print("S3 STA MAC: ");
  Serial.println(WiFi.macAddress());

  
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  AddPeer(sensorBoardMac);
  AddPeer(actionBoardMac);

  Serial.println("ESP-NOW init OK");
}


// 函数说明：Arduino 主流程函数，负责系统启动或循环调度。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void setup() {
  Serial.begin(921600);
  Serial.setTimeout(30);
  delay(1000);

  UF_LogBanner(String(LINGXI_RELEASE_NAME) + " / " + String(LINGXI_VERSION_PATCH));
  UF_Log("BOOT", "SYSTEM", "Lingxi PublicBeta V6 UIVoiceCore + NodeDevice + SD Voice + Bemfa MQTT + AMap Weather");
  UF_Log("BOOT", "BOARD", "ESP32-S3 Touch LCD 4.3C / ES7210 Mic / ES8311 Speaker");

  StateMutex = xSemaphoreCreateMutex();
  V4ReqMutex = xSemaphoreCreateMutex();
  InitWeatherState();
  InitVoiceState();
  InitVoiceIoState();
  InitVoiceTaskState();
  InitV4LocalVoiceState();
  AddCenterLog("sys", String(LINGXI_RELEASE_NAME) + " 启动 / " + String(LINGXI_VERSION_PATCH) + " / 基于：" + String(LINGXI_BASELINE_VERSION));
  LoadNetworkConfig();
  LoadCloudConfig();
  LoadDeviceConfig();
  InitNodeDeviceFoundation();

  State.temperature = 0;
  State.humidity = 0;
  State.lightLevelValue = 0;
  State.smokeValue = 0;
  State.gasValue = 0;
  State.livingPirState = 0;
  State.bedroomPirState = 0;
  State.mainDoorContact = 0;
  State.windowContact = 0;

  State.sensorBoardOnline = false;
  State.autoMode = true;
  State.awayMode = false;
  State.alarmMode = false;
  State.previewMode = ENABLE_DEMO_MODE_DEFAULT;

  State.lightState = 0;
  State.relayState = 0;
  State.buzzerState = 0;
  State.SceneState = 0;
  State.LastSensorTime = 0;
  State.lastPacketSeq = 0;

  memset(&CurrentCmd, 0, sizeof(CurrentCmd));
  memset(&actionBoardState, 0, sizeof(actionBoardState));
  DeviceStatePatchInit();
  CurrentCmd.cmdSeq = 0;
  lastActionBoardTime = 0;

  LastYouRenTime = millis();

  InitEspNow();
  InitStaWifiForCloud();
  
  RefreshEspNowAllPeers();
  BemfaReconnect(true);
  InitWebServer();
  VoiceReload();
  HeadlessDisplayOff("setup after SD init");


  xTaskCreatePinnedToCore(TaskEspNowAndState, "TaskEspNowAndState", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(TaskRule, "TaskRule", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(TaskCommandSend, "TaskCommandSend", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(TaskWebServer, "TaskWebServer", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskBemfaMqtt, "TaskBemfaMqtt", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskQWeather, "TaskQWeather", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskV4LocalVoice, "TaskV4LocalVoice", 16384, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(TaskSerial, "TaskSerial", 8192, NULL, 1, NULL, 1);
#if ENABLE_CI1302_I2C_WAKE
  xTaskCreatePinnedToCore(TaskCI1302I2CWake, "TaskCI1302I2CWake", 6144, NULL, 1, NULL, 1);
#endif
  
  

  UF_Log("BOOT", "RTOS", "FreeRTOS tasks started");
  UF_Log("BOOT", "SERIAL", "Type help for commands");
  UF_Log("BOOT", "WEBUI", "Connect WiFi Lingxi-Gateway, open http://192.168.4.1");
  UF_Log("BOOT", "V4VOICE", "Serial: v4 wake / v4 say 打开客厅灯 / v4 status");
  UF_Log("BOOT", "LEGACY", "V4/CI1302/Stream/ListenSession archived; V6 uses V5 file-ASR/TTS/SD voice core.");
  UF_Log("BOOT", "BEMFA", "MQTT topics configurable in WebUI");
}

// 函数说明：Arduino 主流程函数，负责系统启动或循环调度。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
