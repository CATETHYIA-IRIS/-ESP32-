/*
 * 文件：04_role_voice_audio_core.ino
 * 标题：角色资源与音频播放核心
 * 说明：负责 SD 角色资源、语音包、浏览器/中控播报和音频状态。
 * 可改：可改角色显示文案
 * 禁止：改 SD 路径、语音 ID 表和播放状态机。
 */

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void SendCorsHeader() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleRoot() {
  
  if (!AuthIsValid()) {
    HandleLoginPage();
    return;
  }
  SendCorsHeader();
  server.sendHeader("Content-Encoding", "gzip");
  server.sendHeader("Vary", "Accept-Encoding");
  server.send_P(200, "text/html", (const char*)INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
}

// 函数说明：日志函数，负责记录系统运行和用户操作。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleLogsApi() {
  String json = "[";
  bool first = true;
  for (int i = 0; i < CENTER_LOG_COUNT; i++) {
    int idx = (CenterLogHead - 1 - i + CENTER_LOG_COUNT) % CENTER_LOG_COUNT;
    if (CenterLogs[idx].msg.length() == 0) continue;
    if (!first) json += ",";
    first = false;
    json += "{\"type\":\"" + JsonEscape(CenterLogs[idx].type) + "\",";
    json += "\"time\":\"" + String(CenterLogs[idx].ms / 1000) + "s\",";
    json += "\"msg\":\"" + JsonEscape(CenterLogs[idx].msg) + "\"}";
  }
  json += "]";
  SendCorsHeader();
  server.send(200, "application/json", json);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleStatusApi() {
  xSemaphoreTake(StateMutex, portMAX_DELAY);

  int GuangZhaoPercent = (int)constrain(State.lightLevelValue / 500.0 * 100.0, 0, 100);
  int smoke = State.smokeValue > YANWU_YUZHI ? 1 : 0;
  int gas = State.gasValue > RANQI_YUZHI ? 1 : 0;
  int ActiveAlarmCount = 0;
  if (smoke) ActiveAlarmCount++;
  if (gas) ActiveAlarmCount++;
  if (State.awayMode && State.mainDoorContact) ActiveAlarmCount++;
  if (State.awayMode && State.windowContact) ActiveAlarmCount++;
  String CurrentMode = State.awayMode ? "离家模式" : (State.autoMode ? "自动控制" : "手动控制");
  unsigned long lastAgeSec = State.LastSensorTime > 0 ? (millis() - State.LastSensorTime) / 1000 : 0;

  ActionBoardState z = getPatchedActionStateSnapshot();
  bool actionBoardOnline = (lastActionBoardTime > 0 && millis() - lastActionBoardTime <= 3000);

  uint32_t heapSize = ESP.getHeapSize();
  uint32_t freeHeap = ESP.getFreeHeap();
  int MemUsage = heapSize > 0 ? (int)constrain((heapSize - freeHeap) * 100.0 / heapSize, 0, 100) : 0;

  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t sketchSize = ESP.getSketchSize();
  int FlashUsage = flashSize > 0 ? (int)constrain(sketchSize * 100.0 / flashSize, 0, 100) : 0;

  String json = "{";
  json += "\"LingxiProduct\":\"" + JsonEscape(LINGXI_PRODUCT_NAME) + "\",";
  json += "\"LingxiReleaseName\":\"" + JsonEscape(LINGXI_RELEASE_NAME) + "\",";
  json += "\"LingxiVersionCode\":\"" + JsonEscape(LINGXI_VERSION_CODE) + "\",";
  json += "\"LingxiVersionStage\":\"" + JsonEscape(LINGXI_VERSION_STAGE) + "\",";
  json += "\"LingxiVersionPatch\":\"" + JsonEscape(LINGXI_VERSION_PATCH) + "\",";
  json += "\"LingxiBaseline\":\"" + JsonEscape(LINGXI_BASELINE_VERSION) + "\",";
  json += "\"NodeDevice\":" + NodeDeviceSummaryJson() + ",";
  json += "\"temperature\":" + String(State.temperature, 1) + ",";
  json += "\"humidity\":" + String(State.humidity, 1) + ",";
  json += "\"GuangZhao\":" + String(State.lightLevelValue, 1) + ",";
  json += "\"GuangZhaoPercent\":" + String(GuangZhaoPercent) + ",";

  
  
  json += "\"WenDu\":" + String(State.temperature, 1) + ",";
  json += "\"ShiDu\":" + String(State.humidity, 1) + ",";
  json += "\"LightLevel\":" + String(GuangZhaoPercent) + ",";
  json += "\"IndoorTemperature\":" + String(State.temperature, 1) + ",";
  json += "\"IndoorHumidity\":" + String(State.humidity, 1) + ",";
  json += "\"IndoorLight\":" + String(GuangZhaoPercent) + ",";

  json += "\"YanWu\":" + String(smoke) + ",";
  json += "\"RanQi\":" + String(gas) + ",";
  json += "\"YanWuZhi\":" + String(State.smokeValue) + ",";
  json += "\"RanQiZhi\":" + String(State.gasValue) + ",";

  json += "\"livingPir\":" + String(State.livingPirState) + ",";
  json += "\"bedroomPir\":" + String(State.bedroomPirState) + ",";
  json += "\"DaMenCi\":" + String(State.mainDoorContact) + ",";
  json += "\"windowContact\":" + String(State.windowContact) + ",";

  
  json += "\"KeTingRenTi\":" + String(State.livingPirState) + ",";
  json += "\"WoShiRenTi\":" + String(State.bedroomPirState) + ",";
  json += "\"ChuangHuCi\":" + String(State.windowContact) + ",";
  json += "\"BaoJingMoShi\":" + String((smoke || gas || State.alarmMode) ? "true" : "false") + ",";

  json += "\"autoMode\":" + String(State.autoMode ? "true" : "false") + ",";
  json += "\"awayMode\":" + String(State.awayMode ? "true" : "false") + ",";
  json += "\"alarmMode\":" + String(State.alarmMode ? "true" : "false") + ",";
  json += "\"YanShiMoShi\":" + String(State.previewMode ? "true" : "false") + ",";

  json += "\"sensorBoardOnline\":" + String(State.sensorBoardOnline ? "true" : "false") + ",";
  json += "\"actionBoardOnline\":" + String(actionBoardOnline ? "true" : "false") + ",";

  
  json += "\"GanZhiOnline\":" + String(State.sensorBoardOnline ? "true" : "false") + ",";
  json += "\"ZhiXingOnline\":" + String(actionBoardOnline ? "true" : "false") + ",";
  json += "\"MqttOnline\":" + String(BemfaOnline ? "true" : "false") + ",";
  json += "\"BemfaCmdSubscribed\":" + String(BemfaCmdSubscribed ? "true" : "false") + ",";
  json += "\"BemfaStatusPublished\":" + String(LastBemfaStatusPublishTime > 0 ? "true" : "false") + ",";
  json += "\"BemfaLogPublished\":" + String(LastBemfaLogPublishTime > 0 ? "true" : "false") + ",";
  json += "\"EspNowOnline\":true,";
  json += "\"WifiStatus\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ApStatus\":true,";
  json += "\"BluetoothStatus\":false,";
  json += "\"WifiSsid\":\"" + JsonEscape(WiFi.status() == WL_CONNECTED ? String(WiFi.SSID()) : NetStaSsid()) + "\",";
  json += "\"WifiIp\":\"" + String(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("--")) + "\",";
  json += "\"WifiRssi\":\"" + String(WiFi.status() == WL_CONNECTED ? String(WiFi.RSSI()) + " dBm" : String("--")) + "\",";
  json += "\"ApSsid\":\"" + JsonEscape(NetApSsid()) + "\",";
  json += "\"ApIp\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"ApClientCount\":" + String(WiFi.softAPgetStationNum()) + ",";
  json += "\"SavedStaSsid\":\"" + JsonEscape(NetStaSsid()) + "\",";
  json += "\"StaManualDisconnected\":" + String(StaManualDisconnected ? "true" : "false") + ",";
    json += "\"LivingLightState\":" + String(z.livingLightState) + ",";
  json += "\"BedroomLightState\":" + String(z.bedroomLightState) + ",";
  
  json += "\"KeTingLightState\":" + String(z.livingLightState) + ",";
  json += "\"WoShiLightState\":" + String(z.bedroomLightState) + ",";
  json += "\"AirRelayState\":" + String(z.airRelayState) + ",";
  json += "\"TvRelayState\":" + String(z.tvRelayState) + ",";
  json += "\"FridgeRelayState\":" + String(z.fridgeRelayState) + ",";
  json += "\"WaterRelayState\":" + String(z.waterRelayState) + ",";
  json += "\"LivingCurtainState\":" + String(z.livingCurtainState) + ",";
  json += "\"BedroomCurtainState\":" + String(z.bedroomCurtainState) + ",";
  json += "\"LivingCurtainPercent\":" + String(z.livingCurtainPercent) + ",";
  json += "\"BedroomCurtainPercent\":" + String(z.bedroomCurtainPercent) + ",";
  
  json += "\"KeTingCurtainState\":" + String(z.livingCurtainState) + ",";
  json += "\"WoShiCurtainState\":" + String(z.bedroomCurtainState) + ",";
  json += "\"KeTingCurtainPercent\":" + String(z.livingCurtainPercent) + ",";
  json += "\"WoShiCurtainPercent\":" + String(z.bedroomCurtainPercent) + ",";
  json += "\"WindowState\":" + String(z.windowState) + ",";
  json += "\"WindowPercent\":" + String(z.windowPercent) + ",";
  json += "\"ActionBoardStatusSeq\":" + String(z.statusSeq) + ",";
  json += "\"ActionBoardLastCmdSeq\":" + String(z.lastCmdSeq) + ",";
  json += "\"DeviceStatePatchVersion\":" + String(DeviceStatePatchVersion) + ",";
  json += "\"CPU1Usage\":null,";
  json += "\"CPU2Usage\":null,";
  json += "\"MemUsage\":" + String(MemUsage) + ",";
  json += "\"FlashUsage\":" + String(FlashUsage) + ",";
  json += "\"ActiveAlarmCount\":" + String(ActiveAlarmCount) + ",";
  json += "\"CurrentMode\":\"" + CurrentMode + "\",";
  json += "\"SceneState\":" + String(State.SceneState) + ",";
  json += "\"LastBaoXuHao\":" + String(State.lastPacketSeq) + ",";
  json += "\"LastSensorAgeSec\":" + String(lastAgeSec) + ",";
  json += "\"BemfaOnline\":" + String(BemfaOnline ? "true" : "false") + ",";
  json += "\"BemfaCmdSubscribed\":" + String(BemfaCmdSubscribed ? "true" : "false") + ",";
  json += "\"BemfaCmdSetSubscribed\":" + String(BemfaCmdSetSubscribed ? "true" : "false") + ",";
  json += "\"BemfaStatusPublished\":" + String(LastBemfaStatusPublishTime > 0 ? "true" : "false") + ",";
  json += "\"BemfaLogPublished\":" + String(LastBemfaLogPublishTime > 0 ? "true" : "false") + ",";
  json += "\"BemfaStatusAgeSec\":" + String(LastBemfaStatusPublishTime > 0 ? (millis() - LastBemfaStatusPublishTime) / 1000 : -1) + ",";
  json += "\"BemfaLogAgeSec\":" + String(LastBemfaLogPublishTime > 0 ? (millis() - LastBemfaLogPublishTime) / 1000 : -1) + ",";
  json += "\"BemfaLastState\":" + String(BemfaLastStateCode) + ",";
  json += "\"LastCloudCommand\":\"" + JsonEscape(LastCloudCommand) + "\",";
  json += "\"LastCloudReply\":\"" + JsonEscape(LastCloudReply) + "\",";
  bool weatherOk = Weather.online || Weather.lastNowMs > 0 || (Weather.weatherText.length() && Weather.weatherText != "--" && Weather.weatherText != "待接入" && Weather.weatherText != "待更新");
  json += "\"WeatherOnline\":" + String(weatherOk ? "true" : "false") + ",";
  json += "\"WeatherCity\":\"" + JsonEscape(Weather.city) + "\",";
  json += "\"WeatherText\":\"" + JsonEscape(Weather.weatherText) + "\",";
  json += "\"OutdoorTemp\":" + String(Weather.outdoorTemp, 1) + ",";
  json += "\"FeelTemp\":" + String(Weather.feelsLike, 1) + ",";
  json += "\"OutdoorHumidity\":" + String(Weather.outdoorHumidity) + ",";
  json += "\"WindDirection\":\"" + JsonEscape(Weather.windDir) + "\",";
  json += "\"WindLevel\":\"" + JsonEscape(Weather.windScale) + "级\",";
  json += "\"Precip\":" + String(Weather.precip, 1) + ",";
  json += "\"ForecastOnline\":" + String(Weather.forecastOnline ? "true" : "false") + ",";
  json += "\"TodayTempRange\":\"" + JsonEscape(Weather.todayTempRange) + "\",";
  json += "\"TomorrowForecast\":\"" + JsonEscape(Weather.tomorrowForecast) + "\",";
  json += "\"AirQualityOnline\":" + String(Weather.airOnline ? "true" : "false") + ",";
  json += "\"AirQualityIndex\":" + String(Weather.airAqi) + ",";
  json += "\"AirQualityLevel\":\"" + JsonEscape(Weather.airCategory) + "\",";
  json += "\"PrimaryPollutant\":\"" + JsonEscape(Weather.primaryPollutant) + "\",";
  json += "\"WeatherUmbrellaTip\":\"" + JsonEscape(Weather.umbrellaTip) + "\",";
  json += "\"WeatherMaskTip\":\"" + JsonEscape(Weather.maskTip) + "\",";
  json += "\"WeatherWindowTip\":\"" + JsonEscape(Weather.windowTip) + "\",";
  json += "\"WeatherClothesTip\":\"" + JsonEscape(Weather.clothesTip) + "\",";
  json += "\"WeatherAiTip\":\"" + JsonEscape(Weather.aiTip) + "\",";
  json += "\"SdOnline\":" + String(Voice.sdOnline ? "true" : "false") + ",";
  json += "\"VoicePackShorekeeper\":" + String(Voice.shorekeeperReady ? "true" : "false") + ",";
  json += "\"VoicePackCartethyia\":" + String(Voice.cartethyiaReady ? "true" : "false") + ",";
  json += "\"VoiceCountShorekeeper\":" + String(Voice.shorekeeperCount) + ",";
  json += "\"VoiceCountCartethyia\":" + String(Voice.cartethyiaCount) + ",";
  json += "\"CurrentRole\":\"" + String(Voice.currentRole == ROLE_CARTETHYIA ? "cartethyia" : "shorekeeper") + "\",";
  json += "\"RoleAssets\":" + RoleAssetsStatusJson() + ",";
  json += "\"AudioBusy\":" + String(Voice.audioBusy ? "true" : "false") + ",";
  json += "\"CurrentVoiceId\":" + String(Voice.currentVoiceId) + ",";
  json += "\"CurrentVoiceFile\":\"" + JsonEscape(Voice.currentVoiceFile) + "\",";
  json += "\"LastVoiceError\":\"" + JsonEscape(Voice.lastVoiceError) + "\",";
  json += "\"VoiceInputSource\":\"" + VoiceInputSourceName() + "\",";
  json += "\"VoiceOutputTarget\":\"" + VoiceOutputTargetName() + "\",";
  json += "\"VoiceBrainMode\":\"" + VoiceBrainModeName() + "\",";
  json += "\"VoiceMicReady\":" + String(VoiceIO.micReady ? "true" : "false") + ",";
  json += "\"VoiceRecording\":" + String(VoiceIO.recording ? "true" : "false") + ",";
  json += "\"VoiceLastRecordReady\":" + String(VoiceIO.lastRecordReady ? "true" : "false") + ",";
  json += "\"VoiceLastRecordFile\":\"" + JsonEscape(VoiceIO.lastRecordFile) + "\",";
  json += "\"VoicePeakLevel\":" + String(VoiceIO.peakLevel) + ",";
  json += "\"VoiceRmsLevel\":" + String(VoiceIO.rmsLevel) + ",";
  json += "\"VoiceLastDurationMs\":" + String(VoiceIO.lastDurationMs) + ",";
  json += "\"VoiceLastDataBytes\":" + String(VoiceIO.lastDataBytes) + ",";
  json += "\"VoiceTask\":" + VoiceTaskStatusJson() + ",";
  json += "\"DoubaoModel\":\"" + JsonEscape(LastDoubaoAiModel) + "\",";
  json += "\"DoubaoLastText\":\"" + JsonEscape(LastDoubaoAiText) + "\",";
  json += "\"DoubaoLastReply\":\"" + JsonEscape(LastDoubaoAiReply) + "\",";
  json += "\"DoubaoLastCommands\":\"" + JsonEscape(LastDoubaoAiCommands) + "\",";
  json += "\"DoubaoLastError\":\"" + JsonEscape(LastDoubaoAiError) + "\",";
  json += "\"DoubaoAiState\":\"" + JsonEscape(LastDoubaoAiState) + "\",";
  json += "\"DoubaoAiElapsedMs\":" + String(LastDoubaoAiElapsedMs) + ",";
  json += "\"CenterLogSeq\":" + String(CenterLogSeq) + ",";
  json += "\"UptimeSec\":" + String(millis() / 1000);
  json += "}";

  xSemaphoreGive(StateMutex);

  SendCorsHeader();
  server.send(200, "application/json", json);
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleWeatherStatusApi() {
  bool weatherOk = Weather.online || Weather.lastNowMs > 0 || (Weather.weatherText.length() && Weather.weatherText != "--" && Weather.weatherText != "待接入" && Weather.weatherText != "待更新");
  bool forecastOk = Weather.forecastOnline || Weather.lastAirMs > 0 || (Weather.todayTempRange.length() && Weather.todayTempRange != "待更新");
  String windLevel = Weather.windScale;
  if (windLevel.length() && windLevel.indexOf("级") < 0) windLevel += "级";

  String json = "{\"ok\":true,";
  json += "\"WeatherOnline\":" + String(weatherOk ? "true" : "false") + ",";
  json += "\"WeatherCity\":\"" + JsonEscape(Weather.city) + "\",";
  json += "\"WeatherText\":\"" + JsonEscape(Weather.weatherText) + "\",";
  json += "\"OutdoorTemp\":" + String(Weather.outdoorTemp, 1) + ",";
  json += "\"OutdoorHumidity\":" + String(Weather.outdoorHumidity) + ",";
  json += "\"WindDirection\":\"" + JsonEscape(Weather.windDir) + "\",";
  json += "\"WindLevel\":\"" + JsonEscape(windLevel) + "\",";
  json += "\"ForecastOnline\":" + String(forecastOk ? "true" : "false") + ",";
  json += "\"TodayTempRange\":\"" + JsonEscape(Weather.todayTempRange) + "\",";
  json += "\"TomorrowForecast\":\"" + JsonEscape(Weather.tomorrowForecast) + "\",";
  json += "\"WeatherUmbrellaTip\":\"" + JsonEscape(Weather.umbrellaTip) + "\",";
  json += "\"WeatherWindowTip\":\"" + JsonEscape(Weather.windowTip) + "\",";
  json += "\"WeatherClothesTip\":\"" + JsonEscape(Weather.clothesTip) + "\",";
  json += "\"WeatherAiTip\":\"" + JsonEscape(Weather.aiTip) + "\",";
  json += "\"city\":\"" + JsonEscape(Weather.city) + "\",";
  json += "\"weather\":\"" + JsonEscape(Weather.weatherText) + "\",";
  json += "\"temperature\":" + String(Weather.outdoorTemp, 1) + ",";
  json += "\"humidity\":" + String(Weather.outdoorHumidity) + ",";
  json += "\"windDirection\":\"" + JsonEscape(Weather.windDir) + "\",";
  json += "\"windLevel\":\"" + JsonEscape(windLevel) + "\",";
  json += "\"todayRange\":\"" + JsonEscape(Weather.todayTempRange) + "\",";
  json += "\"tomorrowForecast\":\"" + JsonEscape(Weather.tomorrowForecast) + "\",";
  json += "\"advice\":\"" + JsonEscape(Weather.windowTip.length() ? Weather.windowTip : Weather.aiTip) + "\"}";
  SendCorsHeader();
  server.send(200, "application/json", json);
}


