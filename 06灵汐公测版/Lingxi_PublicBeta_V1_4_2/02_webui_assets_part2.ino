/*
 * 文件：02_webui_assets_part2.ino
 * 标题：WebUI 首页资源第二段
 * 说明：保存前端脚本和页面辅助资源的第二部分。
 * 可改：可通过源码重新生成
 * 禁止：直接改压缩数组和关键 JS 映射。
 */

#include <esp_now.h>
#include <esp_wifi.h>


#if ESP_ARDUINO_VERSION_MAJOR >= 3
auto OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) -> void
#else
auto OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) -> void
#endif
{
  if (len == sizeof(SensorData)) {
    portENTER_CRITICAL(&DataMux);
    memcpy(&latestSensorData, incomingData, sizeof(SensorData));
    hasNewSensorData = true;
    portEXIT_CRITICAL(&DataMux);
    
  }
  else if (len == sizeof(ActionBoardState)) {
    ActionBoardState raw;
    memcpy(&raw, incomingData, sizeof(ActionBoardState));

    portENTER_CRITICAL(&DataMux);
    actionBoardState = raw;
    hasNewActionData = true;
    lastActionBoardTime = millis();
    portEXIT_CRITICAL(&DataMux);

    
    
    mergeActionFeedbackToUiShadow(raw);
    static uint32_t lastActionRecvLogMs = 0;
    uint32_t nowLog = millis();
    if (nowLog - lastActionRecvLogMs > 15000) {
      lastActionRecvLogMs = nowLog;
      Serial.println("[ESP-NOW Recv] ActionBoard status merged");
    }
  }
  else {
    Serial.print("[ESP-NOW Recv] Unknown data length: ");
    Serial.println(len);
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
auto OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) -> void
#else
auto OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) -> void
#endif
{
  
  
  static bool lastWasFail = false;
  static uint32_t lastFailLogMs = 0;
  static uint32_t failCount = 0;
  if (status == ESP_NOW_SEND_SUCCESS) {
    if (lastWasFail) {
      Serial.println("[ESP-NOW Send] Success after fail");
    }
    lastWasFail = false;
    failCount = 0;
    return;
  }
  failCount++;
  lastWasFail = true;
  uint32_t now = millis();
  if (now - lastFailLogMs > 5000) {
    lastFailLogMs = now;
    Serial.println("[ESP-NOW Send] Fail throttled x" + String(failCount) + "，已保留重试/HTTP节点状态兜底");
  }
}


bool IsCmdDifferent(ControlCommand a, ControlCommand b) {
  return a.cmdSeq != b.cmdSeq ||

         a.lightCmd != b.lightCmd ||
         a.lightLevel != b.lightLevel ||
         a.relayCmd != b.relayCmd ||
         a.servoCmd != b.servoCmd ||
         a.motorCmd != b.motorCmd ||
         a.buzzerCmd != b.buzzerCmd ||
         a.sceneCmd != b.sceneCmd ||

         a.livingLightCmd != b.livingLightCmd ||
         a.bedroomLightCmd != b.bedroomLightCmd ||

         a.airRelayCmd != b.airRelayCmd ||
         a.tvRelayCmd != b.tvRelayCmd ||
         a.fridgeRelayCmd != b.fridgeRelayCmd ||
         a.waterRelayCmd != b.waterRelayCmd ||

         a.livingCurtainCmd != b.livingCurtainCmd ||
         a.bedroomCurtainCmd != b.bedroomCurtainCmd ||

         a.windowServoCmd != b.windowServoCmd ||
         a.irAirCmd != b.irAirCmd;
}  

void SetControlCommand(ControlCommand newCmd) {
  bool changed = IsCmdDifferent(CurrentCmd, newCmd);
  CurrentCmd = newCmd;

  
  
  
  
  ApplyDeviceStatePatchFromCommand(CurrentCmd);

  CmdNeedSend = true;

  if (!changed) {
    DeviceStatePatchVersion++;
    LastDeviceStatePatchMs = millis();
  }
}


void UpdateDemoData() {
  unsigned long now = millis();

  if (now - LastDemoUpdateTime < 500) return;
  LastDemoUpdateTime = now;

  SensorData d;
  uint32_t t = now / 1000;

  d.temperature = 25.0 + (t % 10) * 0.1;
  d.humidity = 55.0 + (t % 5);
  d.lightLevelValue = (t % 20 < 10) ? 40 : 180;

  d.smokeValue = (t % 40 > 30) ? 2300 : 1200;
  d.gasValue = (t % 50 > 42) ? 2300 : 1100;

  d.livingPirState = (t % 12 < 6) ? 1 : 0;
  d.bedroomPirState = (t % 18 < 5) ? 1 : 0;

  d.mainDoorContact = 0;
  d.windowContact = 0;

  d.timestampMs = now;
  d.packetSeq = t;

  portENTER_CRITICAL(&DataMux);
  latestSensorData = d;
  hasNewSensorData = true;
  portEXIT_CRITICAL(&DataMux);
}


void UpdateSystemStateFromSensor() {
  SensorData d;
  bool hasNew = false;

  portENTER_CRITICAL(&DataMux);
  if (hasNewSensorData) {
    d = latestSensorData;
    hasNewSensorData = false;
    hasNew = true;
  }
  portEXIT_CRITICAL(&DataMux);

  if (!hasNew) return;

  xSemaphoreTake(StateMutex, portMAX_DELAY);

  State.temperature = d.temperature;
  State.humidity = d.humidity;
  State.lightLevelValue = d.lightLevelValue;
  State.smokeValue = d.smokeValue;
  State.gasValue = d.gasValue;
  State.livingPirState = d.livingPirState;
  State.bedroomPirState = d.bedroomPirState;
  State.mainDoorContact = d.mainDoorContact;
  State.windowContact = d.windowContact;

  State.sensorBoardOnline = true;
  State.LastSensorTime = millis();
  State.lastPacketSeq = d.packetSeq;

  if (State.livingPirState || State.bedroomPirState) {
    LastYouRenTime = millis();
  }

  xSemaphoreGive(StateMutex);
}


void RunRuleEngine() {
  xSemaphoreTake(StateMutex, portMAX_DELAY);

  bool youRen = State.livingPirState || State.bedroomPirState;
  bool guangAn = State.lightLevelValue < GUANGZHAO_YUZHI;
  bool yanWuYiChang = State.smokeValue > YANWU_YUZHI;
  bool ranQiYiChang = State.gasValue > RANQI_YUZHI;
  bool anFangYiChang = State.awayMode && (State.mainDoorContact || State.windowContact);

  State.alarmMode = yanWuYiChang || ranQiYiChang || anFangYiChang;

  ControlCommand newCmd = CurrentCmd;

  if (State.alarmMode) {
    newCmd.lightCmd = 2;
    newCmd.lightLevel = 255;
    newCmd.relayCmd = 1;
    newCmd.buzzerCmd = 0;
    newCmd.sceneCmd = SCENE_ALARM;

    State.lightState = 2;
    State.relayState = 1;
    State.buzzerState = 0;
    State.SceneState = 9;
  } else {
    newCmd.buzzerCmd = 0;
    newCmd.relayCmd = 0;

    if (State.autoMode) {
      if (youRen && guangAn) {
        newCmd.lightCmd = 1;
        newCmd.lightLevel = 180;
        newCmd.livingLightCmd = 1;
        newCmd.bedroomLightCmd = 1;
        State.lightState = 1;
      }

      if (!youRen && millis() - LastYouRenTime > WUREN_GUANDENG_TIME) {
        newCmd.lightCmd = 0;
        newCmd.lightLevel = 0;
        newCmd.livingLightCmd = 0;
        newCmd.bedroomLightCmd = 0;
        State.lightState = 0;
      }
    }

    State.relayState = newCmd.relayCmd;
    State.buzzerState = newCmd.buzzerCmd;
    State.SceneState = newCmd.sceneCmd;
  }

  xSemaphoreGive(StateMutex);

  SetControlCommand(newCmd);
}


void SendControlCommand() {
  if (SendControlCommandRoutedCompat(CurrentCmd)) {
    return;
  }

  esp_err_t result = esp_now_send(
    actionBoardMac,
    (uint8_t *)&CurrentCmd,
    sizeof(CurrentCmd)
  );

  if (result == ESP_OK) {
    Serial.println("[ControlCommand] Send request OK");
  } else {
    
    RefreshEspNowPeer(actionBoardMac);
    result = esp_now_send(actionBoardMac, (uint8_t *)&CurrentCmd, sizeof(CurrentCmd));
    if (result == ESP_OK) {
      Serial.println("[ControlCommand] Send request OK after peer refresh");
    } else {
      Serial.println("[ControlCommand] Send request failed");
    }
  }

  
  SendControlCommandHttpCompat(CurrentCmd);
}


String JsonEscape(String s) {
  s.replace("\\", "\\\\");
  s.replace("\"", "\\\"");
  s.replace("\n", " ");
  s.replace("\r", " ");
  return s;
}


String UF_LogCompact(String s) {
  s.replace("\r", " ");
  s.replace("\n", " ");
  while (s.indexOf("  ") >= 0) s.replace("  ", " ");
  if (s.length() > 220) s = s.substring(0, 220) + "...";
  return s;
}

void UF_Log(String level, String tag, String msg) {
  char head[80];
  snprintf(head, sizeof(head), "[%010lu][%-5s][%-12s] ", (unsigned long)millis(), level.c_str(), tag.c_str());
  Serial.print(head);
  Serial.println(UF_LogCompact(msg));
}

void UF_LogKV(String tag, String key, String value) {
  UF_Log("INFO", tag, key + " = " + value);
}

void UF_LogBanner(String title) {
  Serial.println();
  Serial.println("============================================================");
  Serial.println("  Lingxi Official Smart Home Gateway");
  Serial.print  ("  "); Serial.println(title);
  Serial.println("  WebUI + WiFi/MQTT + Weather + VoiceTask + Doubao AI");
  Serial.println("============================================================");
}

void AddCenterLog(String type, String msg) {
  CenterLogs[CenterLogHead].type = type;
  CenterLogs[CenterLogHead].msg = msg;
  CenterLogs[CenterLogHead].ms = millis();
  CenterLogHead = (CenterLogHead + 1) % CENTER_LOG_COUNT;
  CenterLogSeq++;
  UF_Log("LOG", type, msg);
}


const uint32_t LINGXI_NODE_ABNORMAL_MS = 7UL * 24UL * 3600UL * 1000UL;

String NodeDeviceHideKey(String nodeId) {
  nodeId.trim();
  String k = "n_";
  for (int i = 0; i < (int)nodeId.length() && k.length() < 28; i++) {
    char c = nodeId[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) k += c;
    else k += '_';
  }
  return k;
}

bool NodeDeviceHidden(String nodeId) {
  nodeId.trim();
  if (!nodeId.length() || nodeId == "Lingxi-Gateway") return false;
  DevicePrefs.begin("nodehide", true);
  bool v = DevicePrefs.getBool(NodeDeviceHideKey(nodeId).c_str(), false);
  DevicePrefs.end();
  return v;
}

void NodeDeviceSetHidden(String nodeId, bool hidden) {
  nodeId.trim();
  if (!nodeId.length() || nodeId == "Lingxi-Gateway") return;
  DevicePrefs.begin("nodehide", false);
  DevicePrefs.putBool(NodeDeviceHideKey(nodeId).c_str(), hidden);
  DevicePrefs.end();
}

bool NodeDeviceRuntimeAbnormal(int idx) {
  if (idx < 0 || idx >= LINGXI_RUNTIME_NODE_MAX || !RuntimeNodes[idx].used) return false;
  LingxiRuntimeNode &n = RuntimeNodes[idx];
  if (n.nodeId == "Lingxi-Gateway") return false;
  String st = n.lastStatus;
  st.toLowerCase();
  if (st.indexOf("error") >= 0 || st.indexOf("fail") >= 0 || st.indexOf("abnormal") >= 0 || st.indexOf("异常") >= 0 || st.indexOf("失败") >= 0) return true;
  if (n.lastSeenMs > 0 && (millis() - n.lastSeenMs) >= LINGXI_NODE_ABNORMAL_MS) return true;
  return false;
}

String NodeDeviceRuntimeStateLabel(int idx) {
  if (NodeDeviceRuntimeAbnormal(idx)) return "异常";
  return NodeDeviceRuntimeOnline(idx) ? "在线" : "离线";
}

String NodeDeviceRuntimeStateClass(int idx) {
  if (NodeDeviceRuntimeAbnormal(idx)) return "warn";
  return NodeDeviceRuntimeOnline(idx) ? "ok" : "off";
}

void NodeDeviceRuntimeClearSlot(int idx) {
  if (idx < 0 || idx >= LINGXI_RUNTIME_NODE_MAX) return;
  if (RuntimeNodes[idx].nodeId == "Lingxi-Gateway") return;
  RuntimeNodes[idx].used = false;
  RuntimeNodes[idx].nodeId = "";
  RuntimeNodes[idx].name = "";
  RuntimeNodes[idx].nodeType = "";
  RuntimeNodes[idx].role = "";
  RuntimeNodes[idx].transport = "";
  RuntimeNodes[idx].mac = "";
  RuntimeNodes[idx].ip = "";
  RuntimeNodes[idx].bindTokenMask = "";
  RuntimeNodes[idx].endpointsJson = "[]";
  RuntimeNodes[idx].lastReportJson = "{}";
  RuntimeNodes[idx].lastStatus = "";
  RuntimeNodes[idx].endpointCount = 0;
  RuntimeNodes[idx].registerCount = 0;
  RuntimeNodes[idx].reportCount = 0;
  RuntimeNodes[idx].registeredMs = 0;
  RuntimeNodes[idx].lastSeenMs = 0;
  RuntimeNodes[idx].lastReportMs = 0;
}

int NodeDeviceRuntimeFind(String nodeId) {
  nodeId.trim();
  if (!nodeId.length()) return -1;
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (RuntimeNodes[i].used && RuntimeNodes[i].nodeId == nodeId) return i;
  }
  return -1;
}

int NodeDeviceRuntimeAlloc(String nodeId) {
  nodeId.trim();
  if (!nodeId.length()) return -1;
  int exists = NodeDeviceRuntimeFind(nodeId);
  if (exists >= 0) return exists;

  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (!RuntimeNodes[i].used) {
      RuntimeNodes[i].used = true;
      RuntimeNodes[i].nodeId = nodeId;
      RuntimeNodes[i].name = nodeId;
      RuntimeNodes[i].nodeType = "node";
      RuntimeNodes[i].role = "unknown";
      RuntimeNodes[i].transport = "wifi_http";
      RuntimeNodes[i].mac = "";
      RuntimeNodes[i].ip = "";
      RuntimeNodes[i].bindTokenMask = "";
      RuntimeNodes[i].endpointsJson = "[]";
      RuntimeNodes[i].lastReportJson = "{}";
      RuntimeNodes[i].lastStatus = "registered";
      RuntimeNodes[i].endpointCount = 0;
      RuntimeNodes[i].registerCount = 0;
      RuntimeNodes[i].reportCount = 0;
      RuntimeNodes[i].registeredMs = millis();
      RuntimeNodes[i].lastSeenMs = 0;
      RuntimeNodes[i].lastReportMs = 0;
      RuntimeNodeSeq++;
      return i;
    }
  }

  
  int replaceIdx = 1;
  unsigned long oldest = RuntimeNodes[1].lastSeenMs;
  for (int i = 2; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (RuntimeNodes[i].lastSeenMs < oldest) {
      oldest = RuntimeNodes[i].lastSeenMs;
      replaceIdx = i;
    }
  }
  RuntimeNodes[replaceIdx].used = false;
  RuntimeNodes[replaceIdx].nodeId = "";
  return NodeDeviceRuntimeAlloc(nodeId);
}

bool NodeDeviceRuntimeOnline(int idx) {
  if (idx < 0 || idx >= LINGXI_RUNTIME_NODE_MAX) return false;
  if (!RuntimeNodes[idx].used) return false;
  if (RuntimeNodes[idx].nodeId == "Lingxi-Gateway") return true;
  if (RuntimeNodes[idx].lastSeenMs == 0) return false;
  return millis() - RuntimeNodes[idx].lastSeenMs <= LINGXI_NODE_HTTP_ONLINE_TIMEOUT_MS;
}

int NodeDeviceRuntimeCount(bool onlineOnly) {
  int count = 0;
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (!RuntimeNodes[i].used) continue;
    if (onlineOnly && !NodeDeviceRuntimeOnline(i)) continue;
    count++;
  }
  return count;
}

String NodeDeviceRuntimeAgeText(unsigned long ms) {
  if (ms == 0) return "未上报";
  unsigned long age = (millis() - ms) / 1000UL;
  if (age < 3) return "刚刚";
  if (age < 60) return String(age) + " 秒前";
  if (age < 3600) return String(age / 60) + " 分钟前";
  return String(age / 3600) + " 小时前";
}

String NodeDeviceRequestBody() {
  String body = server.arg("plain");
  body.trim();
  if (body.length() > 4096) body = body.substring(0, 4096);
  return body;
}

String NodeDeviceRequestField(String body, const char* key, String fallback) {
  if (server.hasArg(key)) {
    String v = server.arg(key);
    v.trim();
    return v;
  }
  String v = JsonGetStringAfter(body, key);
  v.trim();
  if (v.length()) return v;
  return fallback;
}

String NodeDeviceJsonArrayRaw(const String &json, const char* key) {
  String pat = "\"" + String(key) + "\"";
  int p = json.indexOf(pat);
  if (p < 0) return "";
  p = json.indexOf('[', p + pat.length());
  if (p < 0) return "";

  int depth = 0;
  bool inString = false;
  bool esc = false;
  for (int i = p; i < (int)json.length(); i++) {
    char c = json[i];
    if (inString) {
      if (esc) { esc = false; continue; }
      if (c == '\\') { esc = true; continue; }
      if (c == '"') inString = false;
      continue;
    }
    if (c == '"') { inString = true; continue; }
    if (c == '[') depth++;
    if (c == ']') {
      depth--;
      if (depth == 0) {
        String raw = json.substring(p, i + 1);
        raw.trim();
        if (raw.length() > 1800) raw = raw.substring(0, 1800);
        return raw;
      }
    }
  }
  return "";
}

String NodeDeviceMaskToken(String token) {
  token.trim();
  if (!token.length()) return "";
  if (token.length() <= 4) return "***";
  return token.substring(0, 2) + "***" + token.substring(token.length() - 2);
}

int NodeDeviceEndpointCountFromJson(String endpointsJson) {
  endpointsJson.trim();
  if (!endpointsJson.length() || endpointsJson == "[]") return 0;
  int count = 0;
  for (int i = 0; i < (int)endpointsJson.length(); i++) {
    if (endpointsJson[i] == '{') count++;
  }
  if (count > 0) return count;
  count = 1;
  for (int i = 0; i < (int)endpointsJson.length(); i++) {
    if (endpointsJson[i] == ',') count++;
  }
  return count;
}

void NodeDeviceRuntimeInit() {
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    RuntimeNodes[i].used = false;
    RuntimeNodes[i].nodeId = "";
    RuntimeNodes[i].name = "";
    RuntimeNodes[i].nodeType = "";
    RuntimeNodes[i].role = "";
    RuntimeNodes[i].transport = "";
    RuntimeNodes[i].mac = "";
    RuntimeNodes[i].ip = "";
    RuntimeNodes[i].bindTokenMask = "";
    RuntimeNodes[i].endpointsJson = "[]";
    RuntimeNodes[i].lastReportJson = "{}";
    RuntimeNodes[i].lastStatus = "";
    RuntimeNodes[i].endpointCount = 0;
    RuntimeNodes[i].registerCount = 0;
    RuntimeNodes[i].reportCount = 0;
    RuntimeNodes[i].registeredMs = 0;
    RuntimeNodes[i].lastSeenMs = 0;
    RuntimeNodes[i].lastReportMs = 0;
  }

  int idx = NodeDeviceRuntimeAlloc("Lingxi-Gateway");
  if (idx >= 0) {
    RuntimeNodes[idx].name = "中控网关";
    RuntimeNodes[idx].nodeType = "gateway";
    RuntimeNodes[idx].role = "center";
    RuntimeNodes[idx].transport = "local+wifi_http+esp_now+mqtt";
    RuntimeNodes[idx].mac = WiFi.macAddress();
    RuntimeNodes[idx].ip = WiFi.softAPIP().toString();
    RuntimeNodes[idx].lastStatus = "online";
    RuntimeNodes[idx].lastSeenMs = millis();
    RuntimeNodes[idx].registeredMs = millis();
  }
}

String NodeDeviceRuntimeOneJson(int idx) {
  if (idx < 0 || idx >= LINGXI_RUNTIME_NODE_MAX || !RuntimeNodes[idx].used) return "null";
  LingxiRuntimeNode &n = RuntimeNodes[idx];
  if (n.nodeId == "Lingxi-Gateway") {
    String mac = WiFi.macAddress();
    String ip = WiFi.softAPIP().toString();
    if (mac.length()) n.mac = mac;
    if (ip.length() && ip != "0.0.0.0") n.ip = ip;
    n.lastSeenMs = millis();
  }
  String json = "{";
  json += "\"node_id\":\"" + JsonEscape(n.nodeId) + "\",";
  json += "\"name\":\"" + JsonEscape(n.name) + "\",";
  json += "\"node_type\":\"" + JsonEscape(n.nodeType) + "\",";
  json += "\"role\":\"" + JsonEscape(n.role) + "\",";
  json += "\"transport\":\"" + JsonEscape(n.transport) + "\",";
  json += "\"mac\":\"" + JsonEscape(n.mac) + "\",";
  json += "\"ip\":\"" + JsonEscape(n.ip) + "\",";
  json += "\"bind_token_mask\":\"" + JsonEscape(n.bindTokenMask) + "\",";
  json += "\"endpoint_count\":" + String(n.endpointCount) + ",";
  json += "\"register_count\":" + String(n.registerCount) + ",";
  json += "\"report_count\":" + String(n.reportCount) + ",";
  json += "\"registered_ms\":" + String(n.registeredMs) + ",";
  json += "\"last_seen_ms\":" + String(n.lastSeenMs) + ",";
  json += "\"last_report_ms\":" + String(n.lastReportMs) + ",";
  json += "\"last_seen_text\":\"" + JsonEscape(NodeDeviceRuntimeAgeText(n.lastSeenMs)) + "\",";
  json += "\"last_report_text\":\"" + JsonEscape(NodeDeviceRuntimeAgeText(n.lastReportMs)) + "\",";
  json += "\"status\":\"" + JsonEscape(n.lastStatus) + "\",";
  json += "\"online\":" + String(NodeDeviceRuntimeOnline(idx) ? "true" : "false") + ",";
  json += "\"endpoints\":" + (n.endpointsJson.length() ? n.endpointsJson : "[]") + ",";
  json += "\"last_report\":" + (n.lastReportJson.length() ? n.lastReportJson : "{}") ;
  json += "}";
  return json;
}

String NodeDeviceRuntimeListJson() {
  String json = "[";
  bool first = true;
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (!RuntimeNodes[i].used) continue;
    if (!first) json += ",";
    first = false;
    json += NodeDeviceRuntimeOneJson(i);
  }
  json += "]";
  return json;
}

String NodeDeviceRuntimeForStaticJson(const char* nodeId) {
  int idx = NodeDeviceRuntimeFind(String(nodeId));
  if (idx < 0) return "null";
  return NodeDeviceRuntimeOneJson(idx);
}


String NodeCompatJsonField(const String &json, const char *key) {
  String pat = "\"" + String(key) + "\":";
  int p = json.indexOf(pat);
  if (p < 0) {
    pat = "\"" + String(key) + "\"";
    p = json.indexOf(pat);
    if (p < 0) return "";
    p = json.indexOf(':', p + pat.length());
    if (p < 0) return "";
    p++;
  } else {
    p += pat.length();
  }
  while (p < (int)json.length() && (json[p] == ' ' || json[p] == '\n' || json[p] == '\r' || json[p] == '\t')) p++;
  if (p >= (int)json.length()) return "";
  if (json[p] == '\"') {
    p++;
    int e = json.indexOf("\"", p);
    if (e < 0) return "";
    return json.substring(p, e);
  }
  int e1 = json.indexOf(',', p);
  int e2 = json.indexOf('}', p);
  int e3 = json.indexOf(']', p);
  int e = -1;
  if (e1 >= 0) e = e1;
  if (e2 >= 0) e = (e < 0 ? e2 : min(e, e2));
  if (e3 >= 0) e = (e < 0 ? e3 : min(e, e3));
  if (e < 0) return "";
  String v = json.substring(p, e);
  v.trim();
  return v;
}

bool NodeCompatHasField(const String &json, const char *key) {
  String pat = "\"" + String(key) + "\"";
  return json.indexOf(pat) >= 0;
}

void NodeCompatApplyIntIfPresent(const String &body, const char *key, int &target, bool &changed) {
  String v = NodeCompatJsonField(body, key);
  if (!v.length()) return;
  target = v.toInt();
  changed = true;
}

void NodeCompatApplyFloatIfPresent(const String &body, const char *key, float &target, bool &changed) {
  String v = NodeCompatJsonField(body, key);
  if (!v.length()) return;
  target = v.toFloat();
  changed = true;
}

void NodeDeviceApplyCompatReport(String body, String nodeId, unsigned long now) {
  bool sensorChanged = false;
  bool actionChanged = false;

  
  if (NodeCompatShouldAcceptReport(nodeId, "temperature")) {
    NodeCompatApplyFloatIfPresent(body, "temperature", State.temperature, sensorChanged);
  }
  if (NodeCompatShouldAcceptReport(nodeId, "humidity")) {
    NodeCompatApplyFloatIfPresent(body, "humidity", State.humidity, sensorChanged);
  }

  if (NodeCompatShouldAcceptReport(nodeId, "light_level")) {
    if (NodeCompatHasField(body, "light_lux")) {
      String v = NodeCompatJsonField(body, "light_lux");
      if (v.length()) { State.lightLevelValue = v.toFloat(); sensorChanged = true; }
    } else if (NodeCompatHasField(body, "light_level")) {
      String v = NodeCompatJsonField(body, "light_level");
      if (v.length()) { State.lightLevelValue = constrain(v.toFloat(), 0.0f, 100.0f) * 5.0f; sensorChanged = true; }
    }
  }

  if (NodeCompatShouldAcceptReport(nodeId, "door_main_contact")) NodeCompatApplyIntIfPresent(body, "door_main_contact", State.mainDoorContact, sensorChanged);
  if (NodeCompatShouldAcceptReport(nodeId, "window_contact")) NodeCompatApplyIntIfPresent(body, "window_contact", State.windowContact, sensorChanged);
  if (NodeCompatShouldAcceptReport(nodeId, "human_living")) NodeCompatApplyIntIfPresent(body, "human_living", State.livingPirState, sensorChanged);
  if (NodeCompatShouldAcceptReport(nodeId, "human_bedroom")) NodeCompatApplyIntIfPresent(body, "human_bedroom", State.bedroomPirState, sensorChanged);
  if (NodeCompatShouldAcceptReport(nodeId, "smoke")) NodeCompatApplyIntIfPresent(body, "smoke", State.smokeValue, sensorChanged);
  if (NodeCompatShouldAcceptReport(nodeId, "gas")) NodeCompatApplyIntIfPresent(body, "gas", State.gasValue, sensorChanged);

  if (sensorChanged) {
    State.sensorBoardOnline = true;
    State.LastSensorTime = now;
    State.lastPacketSeq++;
    if (State.livingPirState || State.bedroomPirState) LastYouRenTime = now;
  }

  
  
  
  bool isIntegratedActionBoardReport = (nodeId == "ActionBoard-01");

  if (!isIntegratedActionBoardReport && NodeCompatShouldAcceptReport(nodeId, "living_light")) NodeCompatApplyIntIfPresent(body, "living_light", uiActionBoardState.livingLightState, actionChanged);
  if (!isIntegratedActionBoardReport && NodeCompatShouldAcceptReport(nodeId, "bedroom_light")) NodeCompatApplyIntIfPresent(body, "bedroom_light", uiActionBoardState.bedroomLightState, actionChanged);
  if (!isIntegratedActionBoardReport && NodeCompatShouldAcceptReport(nodeId, "living_light")) NodeCompatApplyIntIfPresent(body, "living_light_level", uiActionBoardState.livingLightLevel, actionChanged);
  if (!isIntegratedActionBoardReport && NodeCompatShouldAcceptReport(nodeId, "bedroom_light")) NodeCompatApplyIntIfPresent(body, "bedroom_light_level", uiActionBoardState.bedroomLightLevel, actionChanged);

  if (NodeCompatShouldAcceptReport(nodeId, "air_relay")) NodeCompatApplyIntIfPresent(body, "air_relay", uiActionBoardState.airRelayState, actionChanged);
  if (NodeCompatShouldAcceptReport(nodeId, "tv_relay")) NodeCompatApplyIntIfPresent(body, "tv_relay", uiActionBoardState.tvRelayState, actionChanged);
  if (NodeCompatShouldAcceptReport(nodeId, "fridge_relay")) NodeCompatApplyIntIfPresent(body, "fridge_relay", uiActionBoardState.fridgeRelayState, actionChanged);
  if (NodeCompatShouldAcceptReport(nodeId, "water_heater_relay")) NodeCompatApplyIntIfPresent(body, "water_heater_relay", uiActionBoardState.waterRelayState, actionChanged);

  if (!isIntegratedActionBoardReport && NodeCompatShouldAcceptReport(nodeId, "living_curtain") && NodeCompatHasField(body, "living_curtain_percent")) {
    String v = NodeCompatJsonField(body, "living_curtain_percent");
    if (v.length()) { uiActionBoardState.livingCurtainPercent = v.toInt(); actionChanged = true; }
  }
  if (!isIntegratedActionBoardReport && NodeCompatShouldAcceptReport(nodeId, "bedroom_curtain") && NodeCompatHasField(body, "bedroom_curtain_percent")) {
    String v = NodeCompatJsonField(body, "bedroom_curtain_percent");
    if (v.length()) { uiActionBoardState.bedroomCurtainPercent = v.toInt(); actionChanged = true; }
  }
  if (!isIntegratedActionBoardReport && NodeCompatShouldAcceptReport(nodeId, "window_servo") && NodeCompatHasField(body, "window_percent")) {
    String v = NodeCompatJsonField(body, "window_percent");
    if (v.length()) { uiActionBoardState.windowPercent = v.toInt(); actionChanged = true; }
  }

  String lastSeq = NodeCompatJsonField(body, "last_cmd_seq");
  if (lastSeq.length()) uiActionBoardState.lastCmdSeq = max(uiActionBoardState.lastCmdSeq, (uint32_t)lastSeq.toInt());

  if (actionChanged || nodeId.indexOf("ActionNode-") >= 0) {
    lastActionBoardTime = now;
    uiActionBoardState.executorOnline = 1;
    DeviceStatePatchVersion++;
    LastDeviceStatePatchMs = now;
  }
}
String ControlCommandToHttpJson(ControlCommand c) {
  String json = "{";
  json += "\"cmdSeq\":" + String(c.cmdSeq) + ",";
  json += "\"lightCmd\":" + String(c.lightCmd) + ",";
  json += "\"lightLevel\":" + String(c.lightLevel) + ",";
  json += "\"relayCmd\":" + String(c.relayCmd) + ",";
  json += "\"servoCmd\":" + String(c.servoCmd) + ",";
  json += "\"motorCmd\":" + String(c.motorCmd) + ",";
  json += "\"buzzerCmd\":" + String(c.buzzerCmd) + ",";
  json += "\"sceneCmd\":" + String(c.sceneCmd) + ",";
  json += "\"keTingLightCmd\":" + String(c.livingLightCmd) + ",";
  json += "\"woShiLightCmd\":" + String(c.bedroomLightCmd) + ",";
  json += "\"airRelayCmd\":" + String(c.airRelayCmd) + ",";
  json += "\"tvRelayCmd\":" + String(c.tvRelayCmd) + ",";
  json += "\"fridgeRelayCmd\":" + String(c.fridgeRelayCmd) + ",";
  json += "\"waterRelayCmd\":" + String(c.waterRelayCmd) + ",";
  json += "\"keTingCurtainCmd\":" + String(c.livingCurtainCmd) + ",";
  json += "\"woShiCurtainCmd\":" + String(c.bedroomCurtainCmd) + ",";
  json += "\"windowServoCmd\":" + String(c.windowServoCmd) + ",";
  json += "\"irAirCmd\":" + String(c.irAirCmd);
  json += "}";
  return json;
}


String NodeCompatPendingRouteEndpoints = "";

void NodeCompatSetPendingRoute(const char* endpointList) {
  NodeCompatPendingRouteEndpoints = endpointList ? String(endpointList) : "";
  NodeCompatPendingRouteEndpoints.trim();
}

bool NodeCompatEndpointListed(const String &list, const char* endpointId) {
  String ep = endpointId ? String(endpointId) : "";
  if (!ep.length()) return false;
  String padded = "," + list + ",";
  padded.replace(" ", "");
  return padded.indexOf("," + ep + ",") >= 0;
}

bool NodeCompatRuntimeNodeHasEndpoint(int idx, const char* endpointId) {
  if (idx < 0 || idx >= LINGXI_RUNTIME_NODE_MAX) return false;
  if (!endpointId || !endpointId[0]) return false;
  LingxiRuntimeNode &n = RuntimeNodes[idx];
  String ep = String(endpointId);
  String pat1 = "\"id\":\"" + ep + "\"";
  String pat2 = "\"id\": \"" + ep + "\"";
  String pat3 = "\"endpoint_id\":\"" + ep + "\"";
  String pat4 = "\"endpoint_id\": \"" + ep + "\"";
  return n.endpointsJson.indexOf(pat1) >= 0 || n.endpointsJson.indexOf(pat2) >= 0 ||
         n.endpointsJson.indexOf(pat3) >= 0 || n.endpointsJson.indexOf(pat4) >= 0;
}


int NodeCompatFindAnyStandaloneEndpointOwner(const char* endpointId) {
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (!RuntimeNodes[i].used) continue;
    if (!NodeDeviceRuntimeOnline(i)) continue;
    if (RuntimeNodes[i].nodeId == "Lingxi-Gateway" || RuntimeNodes[i].nodeId == "ActionBoard-01" || RuntimeNodes[i].nodeId == "SensorBoard-01") continue;
    if (NodeCompatRuntimeNodeHasEndpoint(i, endpointId)) return i;
  }
  return -1;
}

bool NodeCompatShouldAcceptReport(String fromNodeId, const char* endpointId) {
  int owner = NodeCompatFindAnyStandaloneEndpointOwner(endpointId);
  if (owner < 0) return true;
  return RuntimeNodes[owner].nodeId == fromNodeId;
}

bool NodeCompatIsIndependentActionNode(int idx) {
  if (!NodeCompatShouldSendToRuntimeNode(idx)) return false;
  if (!NodeDeviceRuntimeOnline(idx)) return false;
  
  if (RuntimeNodes[idx].nodeId == "ActionBoard-01") return false;
  return true;
}

int NodeCompatFindStandaloneOwner(const char* endpointId) {
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (!NodeCompatIsIndependentActionNode(i)) continue;
    if (NodeCompatRuntimeNodeHasEndpoint(i, endpointId)) return i;
  }
  return -1;
}

bool NodeCompatPostCommandToNode(int idx, ControlCommand c, const char* endpointId) {
  if (idx < 0 || idx >= LINGXI_RUNTIME_NODE_MAX) return false;
  if (!RuntimeNodes[idx].ip.length() || RuntimeNodes[idx].ip == "0.0.0.0") return false;

  String payload = ControlCommandToHttpJson(c);
  String url = "http://" + RuntimeNodes[idx].ip + "/api/node/command";
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(650);
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  http.end();

  bool ok = (code >= 200 && code < 300);
  String ep = endpointId ? String(endpointId) : String("unknown");
  if (ok) {
    AddCenterLog("control", "端点 " + ep + " 已由独立节点 " + RuntimeNodes[idx].nodeId + " 接管执行");
  } else {
    AddCenterLog("warn", "端点 " + ep + " 独立节点 " + RuntimeNodes[idx].nodeId + " 执行失败，准备回退集成板");
  }
  return ok;
}

bool NodeCompatSendEspNowFallback(ControlCommand c, const char* reason) {
  esp_err_t result = esp_now_send(actionBoardMac, (uint8_t *)&c, sizeof(c));
  if (result != ESP_OK) {
    RefreshEspNowPeer(actionBoardMac);
    result = esp_now_send(actionBoardMac, (uint8_t *)&c, sizeof(c));
  }
  if (result == ESP_OK) {
    AddCenterLog("control", String("执行层集成板 fallback 已发送：") + (reason ? reason : "route"));
    Serial.println("[ActionRoute] ESP-NOW fallback sent to ActionBoard-01");
    return true;
  }
  AddCenterLog("error", String("执行层集成板 fallback 发送失败：") + (reason ? reason : "route"));
  Serial.println("[ActionRoute] ESP-NOW fallback failed");
  return false;
}

bool SendControlCommandRoutedCompat(ControlCommand c) {
  String routeList = NodeCompatPendingRouteEndpoints;
  NodeCompatPendingRouteEndpoints = "";
  routeList.trim();

  if (!routeList.length()) {
    return false;
  }

  bool anyEndpoint = false;
  bool anyStandaloneOk = false;
  bool needIntegratedFallback = false;

  const char* actionEndpoints[] = {
    "living_light", "bedroom_light", "living_curtain", "bedroom_curtain", "window_servo",
    "air_relay", "tv_relay", "fridge_relay", "water_heater_relay"
  };

  for (uint8_t i = 0; i < sizeof(actionEndpoints) / sizeof(actionEndpoints[0]); i++) {
    const char* ep = actionEndpoints[i];
    if (!NodeCompatEndpointListed(routeList, ep)) continue;
    anyEndpoint = true;

    int owner = NodeCompatFindStandaloneOwner(ep);
    if (owner >= 0) {
      bool ok = NodeCompatPostCommandToNode(owner, c, ep);
      if (ok) {
        anyStandaloneOk = true;
      } else {
        needIntegratedFallback = true;
      }
    } else {
      needIntegratedFallback = true;
    }
  }

  if (!anyEndpoint) {
    return false;
  }

  if (needIntegratedFallback) {
    NodeCompatSendEspNowFallback(c, routeList.c_str());
  }

  if (anyStandaloneOk && !needIntegratedFallback) {
    Serial.println("[ActionRoute] Command handled by independent action node(s), ActionBoard fallback skipped.");
  }

  return true;
}

bool NodeCompatShouldSendToRuntimeNode(int idx) {
  if (idx < 0 || idx >= LINGXI_RUNTIME_NODE_MAX) return false;
  LingxiRuntimeNode &n = RuntimeNodes[idx];
  if (!n.used) return false;
  if (n.nodeId == "Lingxi-Gateway" || n.nodeId == "ActionBoard-01" || n.nodeId == "SensorBoard-01") return false;
  if (!n.ip.length() || n.ip == "0.0.0.0") return false;
  if (n.role == "executor" || n.nodeType.indexOf("action") >= 0 || n.nodeId.indexOf("ActionNode-") >= 0) return true;
  return false;
}

void SendControlCommandHttpCompat(ControlCommand c) {
  static uint32_t lastHttpSeq = 0;
  static unsigned long lastHttpSendMs = 0;
  unsigned long now = millis();
  if (c.cmdSeq == lastHttpSeq && now - lastHttpSendMs < 5000) return;
  lastHttpSeq = c.cmdSeq;
  lastHttpSendMs = now;

  String payload = ControlCommandToHttpJson(c);
  int sentCount = 0;
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (!NodeCompatShouldSendToRuntimeNode(i)) continue;
    if (!NodeDeviceRuntimeOnline(i)) continue;
    String url = "http://" + RuntimeNodes[i].ip + "/api/node/command";
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(550);
    if (!http.begin(client, url)) continue;
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(payload);
    http.end();
    if (code >= 200 && code < 300) sentCount++;
  }
  if (sentCount > 0) {
    Serial.println("[DualCompat] HTTP command sent to independent action nodes: " + String(sentCount));
  }
}


String NodeDeviceUpsertRuntimeNode(String body, bool isReport) {
  String nodeId = NodeDeviceRequestField(body, "node_id", "");
  if (!nodeId.length()) nodeId = NodeDeviceRequestField(body, "id", "");
  nodeId.trim();
  if (!nodeId.length()) {
    return "{\"ok\":false,\"error\":\"missing node_id\"}";
  }
  if (nodeId.length() > 40) nodeId = nodeId.substring(0, 40);
  NodeDeviceSetHidden(nodeId, false);

  int idx = NodeDeviceRuntimeAlloc(nodeId);
  if (idx < 0) {
    return "{\"ok\":false,\"error\":\"registry full\"}";
  }

  LingxiRuntimeNode &n = RuntimeNodes[idx];
  unsigned long now = millis();
  bool wasOnline = (n.lastSeenMs > 0 && (now - n.lastSeenMs) <= LINGXI_NODE_HTTP_ONLINE_TIMEOUT_MS);
  bool firstReportBefore = (n.reportCount == 0);
  bool firstRegisterBefore = (n.registerCount == 0);
  int oldEndpointCount = n.endpointCount;

  String name = NodeDeviceRequestField(body, "name", "");
  if (!name.length()) name = NodeDeviceRequestField(body, "display_name", "");
  String nodeType = NodeDeviceRequestField(body, "node_type", "");
  if (!nodeType.length()) nodeType = NodeDeviceRequestField(body, "type", "");
  String role = NodeDeviceRequestField(body, "role", "");
  String transport = NodeDeviceRequestField(body, "transport", "wifi_http");
  String mac = NodeDeviceRequestField(body, "mac", "");
  String ip = NodeDeviceRequestField(body, "ip", "");
  String token = NodeDeviceRequestField(body, "bind_token", "");
  String status = NodeDeviceRequestField(body, "status", isReport ? "report" : "registered");
  String endpoints = NodeDeviceJsonArrayRaw(body, "endpoints");

  if (!name.length()) {
    if (nodeId == "Lingxi-Gateway") name = "中控网关";
    else if (nodeId == "SensorBoard-01") name = "感知层节点";
    else if (nodeId == "ActionBoard-01") name = "执行层节点";
    else name = nodeId;
  }
  if (!nodeType.length()) {
    if (nodeId == "Lingxi-Gateway") nodeType = "gateway";
    else if (nodeId.indexOf("Sensor") >= 0) nodeType = "sensor_board";
    else if (nodeId.indexOf("Action") >= 0) nodeType = "action_board";
    else nodeType = "node";
  }
  if (!role.length()) {
    if (nodeId == "Lingxi-Gateway") role = "center";
    else if (nodeId.indexOf("Sensor") >= 0) role = "sensor";
    else if (nodeId.indexOf("Action") >= 0) role = "executor";
    else role = "unknown";
  }
  if (!ip.length()) ip = server.client().remoteIP().toString();
  if (endpoints.length()) {
    n.endpointsJson = endpoints;
    n.endpointCount = NodeDeviceEndpointCountFromJson(endpoints);
  }

  n.name = name;
  n.nodeType = nodeType;
  n.role = role;
  n.transport = transport;
  if (mac.length()) n.mac = mac;
  if (ip.length()) n.ip = ip;
  if (token.length()) n.bindTokenMask = NodeDeviceMaskToken(token);
  n.lastStatus = status;
  n.lastSeenMs = now;
  if (n.registeredMs == 0) n.registeredMs = now;
  if (isReport) {
    n.lastReportMs = now;
    n.reportCount++;
    if (body.length()) {
      String compactBody = body;
      compactBody.trim();
      String lowerBody = compactBody;
      lowerBody.toLowerCase();
      if (lowerBody.indexOf("token") >= 0 || lowerBody.indexOf("secret") >= 0 || lowerBody.indexOf("key") >= 0 || lowerBody.indexOf("password") >= 0) {
        n.lastReportJson = "{\"masked\":true,\"reason\":\"report contains sensitive-looking field\"}";
      } else if (compactBody.startsWith("{")) n.lastReportJson = compactBody;
      else n.lastReportJson = "{\"raw\":\"" + JsonEscape(compactBody) + "\"}";
    } else {
      n.lastReportJson = "{}";
    }
    if (n.lastReportJson.length() > 1800) n.lastReportJson = n.lastReportJson.substring(0, 1800);
    NodeDeviceApplyCompatReport(body, nodeId, now);
  } else {
    n.registerCount++;
  }

  if (nodeId == "SensorBoard-01") {
    State.sensorBoardOnline = true;
    State.LastSensorTime = now;
  }
  if (nodeId == "ActionBoard-01") {
    lastActionBoardTime = now;
  }

  RuntimeNodeSeq++;

  
  
  if (!isReport) {
    AddCenterLog("node", String(firstRegisterBefore ? "节点注册：" : "节点重新注册：") + nodeId + " / " + n.ip + " / endpoints=" + String(n.endpointCount));
  } else if (firstReportBefore) {
    AddCenterLog("node", "节点首次上报：" + nodeId + " / " + n.ip + " / endpoints=" + String(n.endpointCount));
  } else if (!wasOnline) {
    AddCenterLog("node", "节点恢复在线：" + nodeId + " / " + n.ip);
  } else if (oldEndpointCount != n.endpointCount) {
    AddCenterLog("node", "节点能力变化：" + nodeId + " / endpoints " + String(oldEndpointCount) + " -> " + String(n.endpointCount));
  }

  String json = "{";
  json += "\"ok\":true,";
  json += "\"action\":\"" + String(isReport ? "report" : "register") + "\",";
  json += "\"seq\":" + String(RuntimeNodeSeq) + ",";
  json += "\"online_timeout_ms\":" + String(LINGXI_NODE_HTTP_ONLINE_TIMEOUT_MS) + ",";
  json += "\"node\":" + NodeDeviceRuntimeOneJson(idx);
  json += "}";
  return json;
}

void HandleNodeRegisterApi() {
  String body = NodeDeviceRequestBody();
  xSemaphoreTake(StateMutex, portMAX_DELAY);
  String json = NodeDeviceUpsertRuntimeNode(body, false);
  xSemaphoreGive(StateMutex);
  SendCorsHeader();
  server.send(json.indexOf("\"ok\":false") >= 0 ? 400 : 200, "application/json", json);
}

void HandleNodeReportApi() {
  String body = NodeDeviceRequestBody();
  xSemaphoreTake(StateMutex, portMAX_DELAY);
  String json = NodeDeviceUpsertRuntimeNode(body, true);
  xSemaphoreGive(StateMutex);
  SendCorsHeader();
  server.send(json.indexOf("\"ok\":false") >= 0 ? 400 : 200, "application/json", json);
}


void HandleNodeDeleteApi() {
  String nodeId = server.arg("node_id");
  nodeId.trim();
  SendCorsHeader();
  if (!nodeId.length()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing node_id\"}");
    return;
  }
  if (nodeId == "Lingxi-Gateway") {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"gateway cannot be deleted\"}");
    return;
  }

  xSemaphoreTake(StateMutex, portMAX_DELAY);
  NodeDeviceSetHidden(nodeId, true);
  int idx = NodeDeviceRuntimeFind(nodeId);
  if (idx >= 0) NodeDeviceRuntimeClearSlot(idx);
  RuntimeNodeSeq++;
  xSemaphoreGive(StateMutex);

  AddCenterLog("node", "节点已删除/隐藏：" + nodeId);
  String json = "{\"ok\":true,\"action\":\"delete\",\"node_id\":\"" + JsonEscape(nodeId) + "\"}";
  server.send(200, "application/json", json);
}

void HandleNodeAddApi() {
  String nodeId = server.arg("node_id");
  if (!nodeId.length()) nodeId = server.arg("id");
  nodeId.trim();
  SendCorsHeader();
  if (!nodeId.length()) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing node_id\"}");
    return;
  }
  if (nodeId.length() > 40) nodeId = nodeId.substring(0, 40);

  String name = server.arg("name");
  String role = server.arg("role");
  String nodeType = server.arg("node_type");
  String transport = server.arg("transport");
  name.trim(); role.trim(); nodeType.trim(); transport.trim();
  if (!name.length()) name = nodeId;
  if (!role.length()) role = "unknown";
  if (!nodeType.length()) nodeType = "node";
  if (!transport.length()) transport = "wifi_http";

  xSemaphoreTake(StateMutex, portMAX_DELAY);
  NodeDeviceSetHidden(nodeId, false);
  int idx = NodeDeviceRuntimeAlloc(nodeId);
  if (idx >= 0) {
    RuntimeNodes[idx].name = name;
    RuntimeNodes[idx].role = role;
    RuntimeNodes[idx].nodeType = nodeType;
    RuntimeNodes[idx].transport = transport;
    RuntimeNodes[idx].lastStatus = "manual_added";
    if (RuntimeNodes[idx].registeredMs == 0) RuntimeNodes[idx].registeredMs = millis();
    RuntimeNodeSeq++;
  }
  xSemaphoreGive(StateMutex);

  if (idx < 0) {
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"registry full\"}");
    return;
  }
  AddCenterLog("node", "节点已手动登记：" + nodeId + " / " + name);
  String json = "{\"ok\":true,\"action\":\"add\",\"node_id\":\"" + JsonEscape(nodeId) + "\"}";
  server.send(200, "application/json", json);
}

void HandleNodeRecheckApi() {
  String nodeId = server.arg("node_id");
  nodeId.trim();
  SendCorsHeader();

  xSemaphoreTake(StateMutex, portMAX_DELAY);
  if (nodeId.length()) {
    int idx = NodeDeviceRuntimeFind(nodeId);
    if (idx >= 0 && RuntimeNodes[idx].lastSeenMs == 0) {
      RuntimeNodes[idx].lastStatus = "waiting_report";
    }
  }
  RuntimeNodeSeq++;
  xSemaphoreGive(StateMutex);

  if (nodeId.length()) AddCenterLog("node", "节点重新检测：" + nodeId);
  else AddCenterLog("node", "节点状态已重新检测");
  String json = "{\"ok\":true,\"action\":\"recheck\",\"node_id\":\"" + JsonEscape(nodeId) + "\"}";
  server.send(200, "application/json", json);
}

