/*
 * 文件：11_voice_api_handlers_core.ino
 * 标题：语音 API 处理器
 * 说明：负责 WebUI 语音接口、录音接口、任务查询和语音结果输出。
 * 可改：可改返回文案
 * 禁止：改变接口 JSON 字段名。
 */

// 函数说明：读取函数，负责获取配置、状态或资源。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void LoadNetworkConfig() {
  NetPrefs.begin("ultra_net", true);
  SavedStaSsid = NetPrefs.getString("sta_ssid", "");
  SavedStaPass = NetPrefs.getString("sta_pass", "");
  SavedApSsid = NetPrefs.getString("ap_ssid", "");
  SavedApPass = NetPrefs.getString("ap_pass", "");
  
  ApConfigEnabled = true;
  NetPrefs.end();

  if (SavedStaSsid.length() == 0 && !IsPlaceholderText(STA_WIFI_SSID)) SavedStaSsid = String(STA_WIFI_SSID);
  if (SavedStaPass.length() == 0 && !IsPlaceholderText(STA_WIFI_PASSWORD)) SavedStaPass = String(STA_WIFI_PASSWORD);
  if (SavedApSsid.length() == 0) SavedApSsid = String(AP_SSID);
  if (SavedApPass.length() == 0) SavedApPass = String(AP_PASSWORD);

  if (SavedApSsid.length() == 0) SavedApSsid = "Lingxi-Gateway";
  if (SavedApPass.length() > 0 && SavedApPass.length() < 8) SavedApPass = "12345678";

  Serial.println("[NET] Config loaded from NVS/defaults.");
  Serial.println("[NET] STA SSID: " + (SavedStaSsid.length() ? SavedStaSsid : String("<empty>")));
  Serial.println("[NET] AP SSID: " + SavedApSsid + " always-on");
}

String NetStaSsid() { return SavedStaSsid; }
String NetStaPass() { return SavedStaPass; }
String NetApSsid() { return SavedApSsid.length() ? SavedApSsid : String(AP_SSID); }
String NetApPass() { return SavedApPass.length() ? SavedApPass : String(AP_PASSWORD); }

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void SaveStaWifiConfig(String ssid, String pass) {
  SavedStaSsid = ssid;
  SavedStaPass = pass;
  NetPrefs.begin("ultra_net", false);
  NetPrefs.putString("sta_ssid", SavedStaSsid);
  NetPrefs.putString("sta_pass", SavedStaPass);
  NetPrefs.end();
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void SaveApWifiConfig(String ssid, String pass, bool enabled) {
  (void)enabled;
  SavedApSsid = ssid;
  SavedApPass = pass;
  
  ApConfigEnabled = true;
  NetPrefs.begin("ultra_net", false);
  NetPrefs.putString("ap_ssid", SavedApSsid);
  NetPrefs.putString("ap_pass", SavedApPass);
  NetPrefs.putBool("ap_en", true);
  NetPrefs.end();
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool StartOrUpdateSoftAp() {
  WiFi.mode(WIFI_AP_STA);
  
  ApConfigEnabled = true;

  String apSsid = NetApSsid();
  String apPass = NetApPass();
  bool apOk = false;
  if (apPass.length() >= 8) apOk = WiFi.softAP(apSsid.c_str(), apPass.c_str(), 1);
  else apOk = WiFi.softAP(apSsid.c_str(), nullptr, 1);

  Serial.print("AP Start: ");
  Serial.println(apOk ? "OK" : "FAIL");
  Serial.print("AP SSID: ");
  Serial.println(apSsid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  return apOk;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String BuildWifiScanJson() {
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks(false, true);
  String json = "{\"ok\":true,\"count\":" + String(n < 0 ? 0 : n) + ",\"items\":[";
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      wifi_auth_mode_t enc = WiFi.encryptionType(i);
      json += "{\"ssid\":\"" + JsonEscape(WiFi.SSID(i)) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"channel\":" + String(WiFi.channel(i)) + ",";
      json += "\"enc\":" + String(enc == WIFI_AUTH_OPEN ? "false" : "true") + "}";
    }
  }
  json += "]}";
  LastWifiScanJson = json;
  LastWifiScanMs = millis();
  WiFi.scanDelete();
  return json;
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String WifiScanSummary() {
  String json = BuildWifiScanJson();
  int count = 0;
  int p = json.indexOf("\"count\":");
  if (p >= 0) count = json.substring(p + 8).toInt();
  return "WiFi 扫描完成，发现 " + String(count) + " 个网络；请在网络设置页选择 SSID";
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String WifiConnectAndSave(String ssid, String pass) {
  ssid.trim();
  pass.trim();
  if (ssid.length() == 0) return "WiFi 名称为空，未连接";

  SaveStaWifiConfig(ssid, pass);
  StaManualDisconnected = false;
  WiFi.mode(WIFI_AP_STA);
  AddCenterLog("sys", "正在连接 WiFi：" + ssid + "；本地热点保持开启");

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect(false, false);
    delay(200);
  }

  WiFi.begin(SavedStaSsid.c_str(), SavedStaPass.c_str());
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 12000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    String reply = "WiFi 已连接：" + WiFi.SSID() + "，IP=" + WiFi.localIP().toString();
    AddCenterLog("sys", reply);
    BemfaReconnect(true);
    BemfaPublishStatus();
    return reply;
  }

  String reply = "WiFi 已保存但本次连接超时，热点仍保留；请确认 2.4G WiFi、密码和路由器信道";
  AddCenterLog("sys", reply);
  BemfaPublishStatus();
  return reply;
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String WifiDisconnectKeepAp() {
  StaManualDisconnected = true;
  WiFi.disconnect(false, false);
  BemfaOnline = false;
  String reply = "已断开 STA WiFi，本地热点继续保留";
  AddCenterLog("sys", reply);
  BemfaPublishStatus();
  return reply;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void ScheduleApRestart() {
  ApRestartPending = true;
  ApRestartDueMs = millis() + 1200;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String ApSaveAndApply(String ssid, String pass) {
  ssid.trim();
  pass.trim();
  if (ssid.length() == 0) return "热点名称不能为空";
  if (pass.length() > 0 && pass.length() < 8) return "热点密码至少 8 位；留空则开放热点";

  SaveApWifiConfig(ssid, pass, true);
  ScheduleApRestart();
  String reply = "热点配置已保存：" + ssid + "；AP入口保持开启，约1秒后自动重启热点。若改了名称或密码，请重新连接新热点后继续访问 192.168.4.1";
  AddCenterLog("sys", reply);
  BemfaPublishStatus();
  return reply;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String ApToggleSafe() {
  
  SaveApWifiConfig(NetApSsid(), NetApPass(), true);
  if (WiFi.softAPIP().toString() == "0.0.0.0") {
    StartOrUpdateSoftAp();
  }
  String reply = "热点是进入中控的入口，已锁定为常开；这里只能修改热点名称和密码，不能关闭热点";
  AddCenterLog("sys", reply);
  BemfaPublishStatus();
  return reply;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void ApAlwaysOnLoop() {
  unsigned long now = millis();

  if (ApRestartPending && (long)(now - ApRestartDueMs) >= 0) {
    ApRestartPending = false;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPdisconnect(true);
    delay(180);
    bool ok = StartOrUpdateSoftAp();
    AddCenterLog("sys", ok ? ("热点已自动重启：" + NetApSsid()) : "热点自动重启失败，下一轮保活会继续尝试");
    BemfaPublishStatus();
    return;
  }

  
  if (now - LastApGuardMs > 10000) {
    LastApGuardMs = now;
    ApConfigEnabled = true;
    if (WiFi.softAPIP().toString() == "0.0.0.0") {
      StartOrUpdateSoftAp();
      AddCenterLog("sys", "检测到热点未开启，已自动恢复 AP 入口");
      BemfaPublishStatus();
    }
  }
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void WifiMaintainLoop() {
  ApAlwaysOnLoop();
  if (StaManualDisconnected) return;
  if (WiFi.status() == WL_CONNECTED) return;
  if (NetStaSsid().length() == 0 || IsPlaceholderText(NetStaSsid().c_str())) return;

  unsigned long now = millis();
  if (now - LastStaReconnectTry < 15000) return;
  LastStaReconnectTry = now;

  WiFi.mode(WIFI_AP_STA);
  Serial.println("[NET] STA reconnect try: " + NetStaSsid());
  WiFi.begin(NetStaSsid().c_str(), NetStaPass().c_str());
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleWifiScanApi() {
  SendCorsHeader();
  server.send(200, "application/json", BuildWifiScanJson());
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitStaWifiForCloud() {
#if ENABLE_BEMFA_MQTT
  if (StaManualDisconnected) {
    Serial.println("[Bemfa] STA WiFi is manually disconnected. AP/WebUI still works.");
    return;
  }

  if (NetStaSsid().length() == 0 || IsPlaceholderText(NetStaSsid().c_str())) {
    Serial.println("[Bemfa] STA WiFi not configured. Use WebUI Network Settings to configure it.");
    return;
  }

  Serial.print("[Bemfa] Connecting STA WiFi: ");
  Serial.println(NetStaSsid());
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(NetStaSsid().c_str(), NetStaPass().c_str());

  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 12000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[Bemfa] STA WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    AddCenterLog("sys", "STA WiFi 已连接：" + WiFi.SSID() + " / " + WiFi.localIP().toString());
  } else {
    Serial.println("[Bemfa] STA WiFi connect timeout. Local AP/WebUI still works.");
    AddCenterLog("sys", "STA WiFi 连接超时，本地热点继续可用");
  }
#endif
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String ExecuteUnifiedCommand(String cmd, const char* source) {
  cmd.trim();
  if (cmd.length() == 0) return "empty command";

  if (cmd == "set_role_shorekeeper") {
    Voice.currentRole = ROLE_SHOREKEEPER;
    VoicePersistCurrentRole();
    String reply = "已切换守岸人语音包";
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    AddCenterLog("op", reply);
    BemfaPublishStatus();
    return reply;
  }
  if (cmd == "set_role_cartethyia") {
    Voice.currentRole = ROLE_CARTETHYIA;
    VoicePersistCurrentRole();
    String reply = "已切换卡提希娅语音包";
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    AddCenterLog("op", reply);
    BemfaPublishStatus();
    return reply;
  }
  if (cmd.startsWith("play_voice_")) {
    uint16_t id = (uint16_t)cmd.substring(11).toInt();
    bool ok = VoiceRequestPlay(id, source);
    String reply = ok ? ("语音播放请求已处理 ID " + String(id)) : Voice.lastVoiceError;
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    BemfaPublishStatus();
    return reply;
  }
  if (cmd == "audio_stop") {
    Voice.audioBusy = false;
    Voice.currentVoiceId = 0;
    Voice.currentVoiceFile = "";
    String reply = "音频状态已清空";
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    BemfaPublishStatus();
    return reply;
  }
  if (cmd == "audio_reload") {
    VoiceReload();
    HeadlessDisplayOff("after audio_reload");
    String reply = Voice.sdOnline ? "SD语音包已重新加载" : ("SD语音包加载失败：" + Voice.lastVoiceError);
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    BemfaPublishStatus();
    return reply;
  }
  if (cmd == "screen_off" || cmd == "display_off") {
    HeadlessDisplayOff("cmd");
    String reply = HeadlessDisplayIsOff ? "中控屏幕/背光已强制关闭" : "中控屏幕关闭失败，请检查IO扩展";
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    AddCenterLog("sys", reply);
    BemfaPublishStatus();
    return reply;
  }
  if (cmd == "screen_on_debug" || cmd == "display_on_debug") {
    HeadlessDisplayOnForDebug();
    String reply = HeadlessDisplayIsOff ? "调试亮屏失败" : "已临时打开屏幕使能，仅供调试";
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    AddCenterLog("sys", reply);
    BemfaPublishStatus();
    return reply;
  }


  
  if (cmd == "wifi_scan") {
    String reply = WifiScanSummary();
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    BemfaPublishStatus();
    return reply;
  }
  if (cmd == "wifi_connect") {
    String reply = WifiConnectAndSave(server.arg("ssid"), server.arg("pass"));
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    BemfaPublishStatus();
    return reply;
  }
  if (cmd == "wifi_disconnect") {
    String reply = WifiDisconnectKeepAp();
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    return reply;
  }
  if (cmd == "ap_save") {
    String reply = ApSaveAndApply(server.arg("ssid"), server.arg("pass"));
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    return reply;
  }
  if (cmd == "ap_toggle") {
    String reply = ApToggleSafe();
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    return reply;
  }

  ControlCommand c = CurrentCmd;
  String reply = "ok";

  
  xSemaphoreTake(StateMutex, portMAX_DELAY);
  bool isScene = ApplySceneCommand(cmd, c, reply);
  xSemaphoreGive(StateMutex);
  if (isScene) {
    VoiceAutoFeedback(cmd, reply, source);
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    Serial.printf("[%s] cmd=%s, reply=%s\n", source ? source : "CMD", cmd.c_str(), reply.c_str());
    BemfaPublishStatus();
    return reply;
  }

  
  xSemaphoreTake(StateMutex, portMAX_DELAY);
  CancelSceneForManualCommand(cmd, source, c);

  if (cmd == "light_living_on") {
    State.lightState = 1;
    c.livingLightCmd = 1;
    RefreshLegacyLightField(c);
    CommitEndpointCommand(c, "living_light");
    reply = "客厅灯已开启";
  }
  else if (cmd == "light_living_off") {
    c.livingLightCmd = 0;
    State.lightState = c.bedroomLightCmd ? 1 : 0;
    RefreshLegacyLightField(c);
    CommitEndpointCommand(c, "living_light");
    reply = "客厅灯已关闭";
  }
  else if (cmd == "light_bed_on") {
    State.lightState = 1;
    c.bedroomLightCmd = 1;
    RefreshLegacyLightField(c);
    CommitEndpointCommand(c, "bedroom_light");
    reply = "卧室灯已开启";
  }
  else if (cmd == "light_bed_off") {
    c.bedroomLightCmd = 0;
    State.lightState = c.livingLightCmd ? 1 : 0;
    RefreshLegacyLightField(c);
    CommitEndpointCommand(c, "bedroom_light");
    reply = "卧室灯已关闭";
  }
  else if (cmd == "light_all_on") {
    State.lightState = 1;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    RefreshLegacyLightField(c);
    CommitEndpointGroupCommand(c, "living_light,bedroom_light");
    reply = "全屋灯光已开启";
  }
  else if (cmd == "light_all_off") {
    State.lightState = 0;
    c.livingLightCmd = 0;
    c.bedroomLightCmd = 0;
    RefreshLegacyLightField(c);
    CommitEndpointGroupCommand(c, "living_light,bedroom_light");
    reply = "全屋灯光已关闭";
  }
  else if (cmd == "relay_air_on" || cmd == "relay_air_off") {
    c.airRelayCmd = cmd.endsWith("_on") ? 1 : 0;
    c.relayCmd = c.airRelayCmd;
    CommitEndpointCommand(c, "air_relay");
    reply = c.airRelayCmd ? "空调电源已开启" : "空调电源已关闭";
  }
  else if (cmd == "relay_tv_on" || cmd == "relay_tv_off") {
    c.tvRelayCmd = cmd.endsWith("_on") ? 1 : 0;
    c.relayCmd = c.tvRelayCmd;
    CommitEndpointCommand(c, "tv_relay");
    reply = c.tvRelayCmd ? "电视电源已开启" : "电视电源已关闭";
  }
  else if (cmd == "relay_fridge_on" || cmd == "relay_fridge_off") {
    c.fridgeRelayCmd = cmd.endsWith("_on") ? 1 : 0;
    c.relayCmd = c.fridgeRelayCmd;
    CommitEndpointCommand(c, "fridge_relay");
    reply = c.fridgeRelayCmd ? "冰箱电源已开启" : "冰箱电源已关闭";
  }
  else if (cmd == "relay_water_heater_on" || cmd == "relay_water_heater_off") {
    c.waterRelayCmd = cmd.endsWith("_on") ? 1 : 0;
    c.relayCmd = c.waterRelayCmd;
    CommitEndpointCommand(c, "water_heater_relay");
    reply = c.waterRelayCmd ? "热水器电源已开启" : "热水器电源已关闭";
  }
  else if (cmd == "curtain_living_open" || cmd == "curtain_living_close") {
    c.livingCurtainCmd = cmd.endsWith("_open") ? 1 : 2;
    c.motorCmd = c.livingCurtainCmd;
    CommitEndpointCommand(c, "living_curtain");
    reply = (c.livingCurtainCmd == 1) ? "客厅窗帘打开指令已发送" : "客厅窗帘关闭指令已发送";
  }
  else if (cmd == "curtain_bed_open" || cmd == "curtain_bed_close") {
    c.bedroomCurtainCmd = cmd.endsWith("_open") ? 1 : 2;
    c.motorCmd = c.bedroomCurtainCmd;
    CommitEndpointCommand(c, "bedroom_curtain");
    reply = (c.bedroomCurtainCmd == 1) ? "卧室窗帘打开指令已发送" : "卧室窗帘关闭指令已发送";
  }
  else if (cmd == "curtain_all_open" || cmd == "curtain_all_close") {
    int curtainCmd = cmd.endsWith("_open") ? 1 : 2;
    c.livingCurtainCmd = curtainCmd;
    c.bedroomCurtainCmd = curtainCmd;
    c.motorCmd = curtainCmd;
    CommitEndpointGroupCommand(c, "living_curtain,bedroom_curtain");
    reply = (curtainCmd == 1) ? "全部窗帘打开指令已发送" : "全部窗帘关闭指令已发送";
  }
  else if (cmd == "window_open" || cmd == "window_close") {
    c.windowServoCmd = cmd.endsWith("_open") ? 1 : 2;
    c.servoCmd = c.windowServoCmd;
    CommitEndpointCommand(c, "window_servo");
    reply = (c.windowServoCmd == 1) ? "窗户打开指令已发送" : "窗户关闭指令已发送";
  }
  else if (cmd == "relay_air") {
    c.airRelayCmd = DeviceStatePatchIsOn("air") ? 0 : 1;
    c.relayCmd = c.airRelayCmd;
    CommitEndpointCommand(c, "air_relay");
    reply = c.airRelayCmd ? "空调电源已开启" : "空调电源已关闭";
  }
  else if (cmd == "relay_tv") {
    c.tvRelayCmd = DeviceStatePatchIsOn("tv") ? 0 : 1;
    c.relayCmd = c.tvRelayCmd;
    CommitEndpointCommand(c, "tv_relay");
    reply = c.tvRelayCmd ? "电视电源已开启" : "电视电源已关闭";
  }
  else if (cmd == "relay_fridge") {
    c.fridgeRelayCmd = DeviceStatePatchIsOn("fridge") ? 0 : 1;
    c.relayCmd = c.fridgeRelayCmd;
    CommitEndpointCommand(c, "fridge_relay");
    reply = c.fridgeRelayCmd ? "冰箱电源已开启" : "冰箱电源已关闭";
  }
  else if (cmd == "relay_water_heater") {
    c.waterRelayCmd = DeviceStatePatchIsOn("water") ? 0 : 1;
    c.relayCmd = c.waterRelayCmd;
    CommitEndpointCommand(c, "water_heater_relay");
    reply = c.waterRelayCmd ? "热水器电源已开启" : "热水器电源已关闭";
  }
  else if (cmd == "relay_3") {
    c.livingCurtainCmd = DeviceStatePatchIsOn("curtain_living") ? 2 : 1;
    c.motorCmd = c.livingCurtainCmd;
    CommitEndpointCommand(c, "living_curtain");
    reply = (c.livingCurtainCmd == 1) ? "客厅窗帘打开指令已发送" : "客厅窗帘关闭指令已发送";
  }
  else if (cmd == "relay_4") {
    c.bedroomCurtainCmd = DeviceStatePatchIsOn("curtain_bed") ? 2 : 1;
    c.motorCmd = c.bedroomCurtainCmd;
    CommitEndpointCommand(c, "bedroom_curtain");
    reply = (c.bedroomCurtainCmd == 1) ? "卧室窗帘打开指令已发送" : "卧室窗帘关闭指令已发送";
  }
  else if (cmd == "window_toggle") {
    c.windowServoCmd = DeviceStatePatchIsOn("window") ? 2 : 1;
    c.servoCmd = c.windowServoCmd;
    CommitEndpointCommand(c, "window_servo");
    reply = (c.windowServoCmd == 1) ? "窗户打开指令已发送" : "窗户关闭指令已发送";
  }
  else if (cmd == "auto_on") {
    State.autoMode = true;
    State.SceneState = SCENE_AUTO;
    c.sceneCmd = SCENE_AUTO;
    CommitWebCommand(c);
    reply = "自动控制已开启";
  }
  else if (cmd == "auto_off") {
    State.autoMode = false;
    State.SceneState = SCENE_NONE;
    c.sceneCmd = SCENE_NONE;
    CommitWebCommand(c);
    reply = "自动控制已关闭，已切换为手动控制";
  }
  else if (cmd == "away_on") {
    State.awayMode = true;
    State.autoMode = true;
    State.SceneState = SCENE_AWAY;
    c.sceneCmd = SCENE_AWAY;
    CommitWebCommand(c);
    reply = "离家模式已执行";
  }
  else if (cmd == "away_off") {
    State.awayMode = false;
    State.autoMode = false;
    State.SceneState = SCENE_HOME;
    c.sceneCmd = SCENE_HOME;
    CommitWebCommand(c);
    reply = "回家模式已执行";
  }
  else if (cmd == "alarm_reset") {
    State.alarmMode = false;
    State.buzzerState = 0;
    State.SceneState = SCENE_NONE;
    State.awayMode = false;
    c.buzzerCmd = 0;
    c.sceneCmd = SCENE_NONE;
    CommitWebCommand(c);
    reply = "报警已解除";
  }
  else {
    
    if (cmd == "wifi_scan") { reply = "WiFi 扫描功能已预留"; }
    else if (cmd == "wifi_connect") { reply = "WiFi 连接接口已预留：SSID=" + server.arg("ssid"); }
    else if (cmd == "wifi_disconnect") { reply = "WiFi 断开接口已预留"; }
    else if (cmd == "wifi_toggle") { reply = "WiFi 连接/断开功能已预留"; }
    else if (cmd == "ap_toggle") { reply = "热点入口已锁定常开，不能关闭"; }
    else if (cmd == "ap_save") { reply = "热点设置保存接口：SSID=" + server.arg("ssid") + "，热点保持常开"; }
    else if (cmd == "voice_wake") {
      reply = "无屏中控：语音唤醒接口已触发，后续接入 CI1302 / 在线语音";
    }
    else if (cmd == "voice_press_start") {
      reply = "开始录音/聆听接口已预留";
    }
    else if (cmd == "voice_press_end") {
      reply = "语音发送接口已预留";
    }
    else if (cmd == "voice_mode_offline") { reply = "已切换为离线语音优先"; }
    else if (cmd == "voice_mode_online") { reply = "已切换为在线语音优先，豆包接口后续接入"; }
    else if (cmd == "audio_sd_test") {
      bool ok = VoiceRequestPlay(1, source);
      reply = ok ? "SD语音包测试已触发" : Voice.lastVoiceError;
    }
    else if (cmd == "cloud_connect") { reply = BemfaOnline ? "巴法云 MQTT 已连接" : "巴法云 MQTT 后台重连中或配置未完成"; }
    else if (cmd == "cloud_report") { reply = "云端状态上报已请求"; }
    else if (cmd == "ai_start") {
      reply = "豆包 AI 对话接口已预留";
    }
    else {
      reply = "未知指令：" + cmd;
    }
  }

  xSemaphoreGive(StateMutex);

  VoiceAutoFeedback(cmd, reply, source);
  LastCloudCommand = cmd;
  LastCloudReply = reply;
  Serial.printf("[%s] cmd=%s, reply=%s\n", source ? source : "CMD", cmd.c_str(), reply.c_str());
  BemfaPublishStatus();
  return reply;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String ExtractCmdFromPayload(String payload) {
  payload.trim();
  if (payload.length() == 0) return "";

  
  if (!payload.startsWith("{")) return payload;

  
  int k = payload.indexOf("\"cmd\"");
  if (k < 0) return payload;
  int colon = payload.indexOf(':', k);
  if (colon < 0) return payload;
  int q1 = payload.indexOf('"', colon + 1);
  int q2 = payload.indexOf('"', q1 + 1);
  if (q1 < 0 || q2 < 0) return payload;
  return payload.substring(q1 + 1, q2);
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void BemfaPublishLog(String type, String msg) {
#if ENABLE_BEMFA_MQTT
  if (!BemfaMqtt.connected()) return;
  String payload = "{\"type\":\"" + type + "\",\"msg\":\"" + JsonEscape(msg) + "\",\"time\":" + String(millis() / 1000) + "}";
  String topic = CloudBemfaTopicLog();
  bool ok = BemfaMqtt.publish(topic.c_str(), payload.c_str());
  if (ok) LastBemfaLogPublishTime = millis();
#endif
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String BemfaBuildStatusJson() {
  xSemaphoreTake(StateMutex, portMAX_DELAY);
  SystemState s = State;
  xSemaphoreGive(StateMutex);

  ActionBoardState z = getPatchedActionStateSnapshot();
  unsigned long lastZx = lastActionBoardTime;

  bool zxOnline = lastZx > 0 && millis() - lastZx <= 3000;
  int luxPercent = (int)constrain(s.lightLevelValue / 500.0 * 100.0, 0, 100);
  int yanwu = s.smokeValue > YANWU_YUZHI ? 1 : 0;
  int ranqi = s.gasValue > RANQI_YUZHI ? 1 : 0;

  String json = "{";
  json += "\"temperature\":" + String(s.temperature, 1) + ",";
  json += "\"humidity\":" + String(s.humidity, 1) + ",";
  json += "\"GuangZhao\":" + String(s.lightLevelValue, 1) + ",";
  json += "\"GuangZhaoPercent\":" + String(luxPercent) + ",";
  json += "\"YanWu\":" + String(yanwu) + ",";
  json += "\"RanQi\":" + String(ranqi) + ",";
  json += "\"livingPir\":" + String(s.livingPirState) + ",";
  json += "\"bedroomPir\":" + String(s.bedroomPirState) + ",";
  json += "\"DaMenCi\":" + String(s.mainDoorContact) + ",";
  json += "\"windowContact\":" + String(s.windowContact) + ",";
  json += "\"sensorBoardOnline\":" + String(s.sensorBoardOnline ? "true" : "false") + ",";
  json += "\"actionBoardOnline\":" + String(zxOnline ? "true" : "false") + ",";
  json += "\"MqttOnline\":" + String(BemfaOnline ? "true" : "false") + ",";
  json += "\"BemfaCmdSubscribed\":" + String(BemfaCmdSubscribed ? "true" : "false") + ",";
  json += "\"BemfaStatusPublished\":" + String(LastBemfaStatusPublishTime > 0 ? "true" : "false") + ",";
  json += "\"BemfaLogPublished\":" + String(LastBemfaLogPublishTime > 0 ? "true" : "false") + ",";
  json += "\"LivingLightState\":" + String(z.livingLightState) + ",";
  json += "\"BedroomLightState\":" + String(z.bedroomLightState) + ",";
  json += "\"AirRelayState\":" + String(z.airRelayState) + ",";
  json += "\"TvRelayState\":" + String(z.tvRelayState) + ",";
  json += "\"FridgeRelayState\":" + String(z.fridgeRelayState) + ",";
  json += "\"WaterRelayState\":" + String(z.waterRelayState) + ",";
  json += "\"LivingCurtainPercent\":" + String(z.livingCurtainPercent) + ",";
  json += "\"BedroomCurtainPercent\":" + String(z.bedroomCurtainPercent) + ",";
  json += "\"WindowPercent\":" + String(z.windowPercent) + ",";
  json += "\"DeviceStatePatchVersion\":" + String(DeviceStatePatchVersion) + ",";
  json += "\"SceneState\":" + String(s.SceneState) + ",";
  json += "\"WeatherOnline\":" + String(Weather.online ? "true" : "false") + ",";
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
  json += "\"HeadlessDisplayOff\":" + String(HeadlessDisplayIsOff ? "true" : "false") + ",";
  json += "\"BoardAudioEnabled\":" + String(ENABLE_BOARD_AUDIO_PLAYBACK ? "true" : "false") + ",";
  json += "\"UptimeSec\":" + String(millis() / 1000);
  json += "}";
  return json;
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void BemfaPublishStatus() {
#if ENABLE_BEMFA_MQTT
  if (!BemfaMqtt.connected()) return;
  String payload = BemfaBuildStatusJson();
  String topic = CloudBemfaTopicStatus();
  bool ok = BemfaMqtt.publish(topic.c_str(), payload.c_str());
  if (ok) LastBemfaStatusPublishTime = millis();
#endif
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void BemfaOnMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  msg.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();

  String cmd = ExtractCmdFromPayload(msg);
  cmd.trim();
  Serial.print("[Bemfa] Topic: ");
  Serial.print(topic);
  Serial.print(" Payload: ");
  Serial.println(msg);

  if (TryAuthResetMaintCommand(cmd, "Bemfa")) {
    BemfaPublishLog("security", "管理员账号已通过巴法云维护码重置");
    BemfaPublishStatus();
    return;
  }

  String reply = ExecuteUnifiedCommand(cmd, "Bemfa");
  AddCenterLog("op", "巴法云 " + cmd + " => " + reply);
  BemfaPublishLog("cloudcmd", cmd + " => " + reply);
  BemfaPublishStatus();
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void BemfaReconnect(bool forceNow = false) {
#if ENABLE_BEMFA_MQTT
  if (IsPlaceholderText(CloudBemfaUid())) {
    BemfaOnline = false;
    BemfaCmdSubscribed = false;
    BemfaCmdSetSubscribed = false;
    Serial.println("[Bemfa] BEMFA_UID not configured.");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    BemfaOnline = false;
    BemfaCmdSubscribed = false;
    BemfaCmdSetSubscribed = false;
    return;
  }
  if (BemfaMqtt.connected()) {
    BemfaOnline = true;
    return;
  }

  unsigned long now = millis();
  if (!forceNow && now - LastBemfaReconnectTime < 5000) return;
  LastBemfaReconnectTime = now;

  BemfaMqtt.setServer(BEMFA_MQTT_SERVER, BEMFA_MQTT_PORT);
  BemfaMqtt.setCallback(BemfaOnMessage);
  BemfaMqtt.setBufferSize(1024);

  String clientId = CloudBemfaUid();
  Serial.println("[Bemfa] Connecting MQTT...");
  bool ok = BemfaMqtt.connect(clientId.c_str());

  if (ok) {
    BemfaOnline = true;
    BemfaLastStateCode = 0;
    String cmdTopic = CloudBemfaTopicCmd();
    BemfaCmdSubscribed = BemfaMqtt.subscribe(cmdTopic.c_str());
    String topicSet = CloudBemfaTopicCmd() + "/set";
    BemfaCmdSetSubscribed = BemfaMqtt.subscribe(topicSet.c_str());
    AddCenterLog("sys", String("中控已连接巴法云 MQTT，命令主题") + (BemfaCmdSubscribed ? "已订阅" : "订阅失败"));
    BemfaPublishLog("system", "中控已连接巴法云 MQTT");
    BemfaPublishStatus();
    Serial.println("[Bemfa] MQTT connected and subscribed.");
  } else {
    BemfaOnline = false;
    BemfaCmdSubscribed = false;
    BemfaCmdSetSubscribed = false;
    BemfaLastStateCode = BemfaMqtt.state();
    Serial.print("[Bemfa] MQTT connect failed, state=");
    Serial.println(BemfaMqtt.state());
  }
#endif
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void BemfaMqttLoop() {
  WifiMaintainLoop();
#if ENABLE_BEMFA_MQTT
  if (WiFi.status() != WL_CONNECTED) {
    BemfaOnline = false;
    BemfaCmdSubscribed = false;
    BemfaCmdSetSubscribed = false;
    return;
  }
  if (!BemfaMqtt.connected()) BemfaReconnect(false);
  BemfaMqtt.loop();
  if (BemfaMqtt.connected() && millis() - LastBemfaStatusPublishTime > 5000) {
    BemfaPublishStatus();
  }
#endif
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskBemfaMqtt(void *pvParameters) {
  while (1) {
    BemfaMqttLoop();
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}


