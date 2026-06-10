/*
 * 文件：03_json_html_utils_storage_base.ino
 * 标题：JSON、HTML 与基础存储工具
 * 说明：提供 JSON 转义、HTML 输出、Preferences 存储等通用工具。
 * 可改：可加安全工具函数
 * 禁止：改变已有函数返回格式。
 */

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitNodeDeviceFoundation() {
  NodeDeviceRuntimeInit();
  AddCenterLog("sys", String(LINGXI_RELEASE_NAME) + " 节点注册中心已启用：静态 " + String(LINGXI_NODE_COUNT) + " 个网关/节点 / " + String(LINGXI_ENDPOINT_COUNT) + " 个设备 / 注册表容量 " + String(LINGXI_RUNTIME_NODE_MAX));
  AddCenterLog("sys", "V4.0.5 日志节流：节点周期上报不再刷系统日志，/nodes 页面仍实时显示最近上报。");
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
int NodeDeviceFindEndpointIndex(const char* endpointId) {
  String id = String(endpointId);
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (id == String(LingxiEndpoints[i].id)) return i;
  }
  return -1;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigKey(const char* prefix, int idx) {
  return String(prefix) + String(idx);
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void LoadDeviceConfig() {
  DevicePrefs.begin("nodecfg", true);
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    String roomKey = DeviceConfigKey("rm", i);
    DeviceEndpointName[i] = String(LingxiEndpoints[i].name);   
    DeviceEndpointRoom[i] = DevicePrefs.getString(roomKey.c_str(), LingxiEndpoints[i].room);
    DeviceEndpointEnabled[i] = !LingxiEndpoints[i].reserved;   
  }
  DevicePrefs.end();
  AddCenterLog("sys", "设备登记配置已载入。");
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void ResetDeviceConfig() {
  DevicePrefs.begin("nodecfg", false);
  DevicePrefs.clear();
  DevicePrefs.end();
  LoadDeviceConfig();
  AddCenterLog("sys", "V3 设备配置已恢复默认模板。");
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DeviceConfigEndpointEnabled(int idx) {
  if (idx < 0 || idx >= LINGXI_ENDPOINT_COUNT) return false;
  if (LingxiEndpoints[idx].reserved) return false;
  return DeviceEndpointEnabled[idx];
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigEndpointName(int idx) {
  if (idx < 0 || idx >= LINGXI_ENDPOINT_COUNT) return "";
  return String(LingxiEndpoints[idx].name);
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigEndpointRoom(int idx) {
  if (idx < 0 || idx >= LINGXI_ENDPOINT_COUNT) return "";
  if (!DeviceEndpointRoom[idx].length()) return String(LingxiEndpoints[idx].room);
  return DeviceEndpointRoom[idx];
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigPhysicalModeText(const LingxiEndpointInfo &ep) {
  String nodeId = String(ep.nodeId);
  if (nodeId == "SensorBoard-01") return "感知层集成板适配";
  if (nodeId == "ActionBoard-01") return "执行层集成板适配";
  if (nodeId == "Lingxi-Gateway") return "中控网关本机能力";
  return "独立子节点预留";
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool NodeDeviceNodeOnline(const char* nodeId) {
  String id = String(nodeId);
  if (id == "Lingxi-Gateway") return true;

  int runtimeIdx = NodeDeviceRuntimeFind(id);
  if (runtimeIdx >= 0 && NodeDeviceRuntimeOnline(runtimeIdx)) return true;

  if (id == "SensorBoard-01") return State.sensorBoardOnline;
  if (id == "ActionBoard-01") return lastActionBoardTime > 0 && millis() - lastActionBoardTime <= 3000;
  return false;
}


// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool NodeDeviceRuntimeEndpointOnline(const char* endpointId) {
  String pat = "\"id\":\"" + String(endpointId) + "\"";
  String pat2 = "\"id\": \"" + String(endpointId) + "\"";
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (!RuntimeNodes[i].used) continue;
    if (!NodeDeviceRuntimeOnline(i)) continue;
    if (RuntimeNodes[i].endpointsJson.indexOf(pat) >= 0 || RuntimeNodes[i].endpointsJson.indexOf(pat2) >= 0) return true;
  }
  return false;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceNodeMac(const char* nodeId) {
  String id = String(nodeId);
  int runtimeIdx = NodeDeviceRuntimeFind(id);
  if (runtimeIdx >= 0 && RuntimeNodes[runtimeIdx].mac.length()) return RuntimeNodes[runtimeIdx].mac;

  if (id == "Lingxi-Gateway") {
    String mac = WiFi.macAddress();
    if (mac.length() == 0) mac = "runtime";
    return mac;
  }
  if (id == "SensorBoard-01") return "待节点自动上报";
  if (id == "ActionBoard-01") return "待节点自动上报";
  return "";
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceBoolDisplay(bool on) {
  return on ? "开启" : "关闭";
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceContactDisplay(int v) {
  return v ? "打开" : "关闭";
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDevicePresenceDisplay(int v) {
  return v ? "有人" : "无人";
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceCurtainDisplay(int percent, int state) {
  String action = "停止";
  if (state == 1) action = "打开";
  else if (state == 2) action = "关闭";
  return action + " / " + String(percent) + "%";
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceEndpointStateJson(const LingxiEndpointInfo &ep) {
  String id = String(ep.id);
  ActionBoardState z = getPatchedActionStateSnapshot();
  int epIndex = NodeDeviceFindEndpointIndex(ep.id);
  bool enabled = DeviceConfigEndpointEnabled(epIndex);
  bool online = enabled && !ep.reserved && (NodeDeviceNodeOnline(ep.nodeId) || NodeDeviceRuntimeEndpointOnline(ep.id));  
  String value = "";
  String display = "";
  String state = enabled ? "normal" : "disabled";
  String raw = "";

  if (!enabled) {
    value = ep.reserved ? "reserved" : "disabled";
    display = ep.reserved ? "预留" : "已停用";
    state = ep.reserved ? "reserved" : "disabled";
  } else if (id == "temperature") {
    value = String(State.temperature, 1);
    display = value + " °C";
  } else if (id == "humidity") {
    value = String(State.humidity, 1);
    display = value + " %";
  } else if (id == "light_level") {
    int p = (int)constrain(State.lightLevelValue / 500.0 * 100.0, 0, 100);
    value = String(p);
    raw = String(State.lightLevelValue, 1);
    display = String(p) + " %";
  } else if (id == "door_main_contact") {
    value = String(State.mainDoorContact);
    display = NodeDeviceContactDisplay(State.mainDoorContact);
    state = State.mainDoorContact ? "open" : "closed";
  } else if (id == "window_contact") {
    value = String(State.windowContact);
    display = NodeDeviceContactDisplay(State.windowContact);
    state = State.windowContact ? "open" : "closed";
  } else if (id == "human_living") {
    value = String(State.livingPirState);
    display = NodeDevicePresenceDisplay(State.livingPirState);
    state = State.livingPirState ? "presence" : "clear";
  } else if (id == "human_bedroom") {
    value = String(State.bedroomPirState);
    display = NodeDevicePresenceDisplay(State.bedroomPirState);
    state = State.bedroomPirState ? "presence" : "clear";
  } else if (id == "smoke") {
    value = String(State.smokeValue);
    display = String(State.smokeValue) + (State.smokeValue > YANWU_YUZHI ? " / 报警" : " / 正常");
    state = State.smokeValue > YANWU_YUZHI ? "alarm" : "normal";
  } else if (id == "gas") {
    value = String(State.gasValue);
    display = String(State.gasValue) + (State.gasValue > RANQI_YUZHI ? " / 报警" : " / 正常");
    state = State.gasValue > RANQI_YUZHI ? "alarm" : "normal";
  } else if (id == "living_light") {
    value = String(z.livingLightState);
    display = NodeDeviceBoolDisplay(z.livingLightState == 1);
    state = z.livingLightState == 1 ? "on" : "off";
  } else if (id == "bedroom_light") {
    value = String(z.bedroomLightState);
    display = NodeDeviceBoolDisplay(z.bedroomLightState == 1);
    state = z.bedroomLightState == 1 ? "on" : "off";
  } else if (id == "living_curtain") {
    value = String(z.livingCurtainPercent);
    display = NodeDeviceCurtainDisplay(z.livingCurtainPercent, z.livingCurtainState);
    state = z.livingCurtainPercent > 50 ? "open" : "closed";
  } else if (id == "bedroom_curtain") {
    value = String(z.bedroomCurtainPercent);
    display = NodeDeviceCurtainDisplay(z.bedroomCurtainPercent, z.bedroomCurtainState);
    state = z.bedroomCurtainPercent > 50 ? "open" : "closed";
  } else if (id == "window_servo") {
    value = String(z.windowPercent);
    display = NodeDeviceCurtainDisplay(z.windowPercent, z.windowState);
    state = z.windowPercent > 50 ? "open" : "closed";
  } else if (id == "air_relay") {
    value = String(z.airRelayState);
    display = NodeDeviceBoolDisplay(z.airRelayState == 1);
    state = z.airRelayState == 1 ? "on" : "off";
  } else if (id == "tv_relay") {
    value = String(z.tvRelayState);
    display = NodeDeviceBoolDisplay(z.tvRelayState == 1);
    state = z.tvRelayState == 1 ? "on" : "off";
  } else if (id == "fridge_relay") {
    value = String(z.fridgeRelayState);
    display = NodeDeviceBoolDisplay(z.fridgeRelayState == 1);
    state = z.fridgeRelayState == 1 ? "on" : "off";
  } else if (id == "water_heater_relay") {
    value = String(z.waterRelayState);
    display = NodeDeviceBoolDisplay(z.waterRelayState == 1);
    state = z.waterRelayState == 1 ? "on" : "off";
  } else if (id == "mi_plug_3_reserved") {
    online = false;
    value = "reserved";
    display = "预留";
    state = "reserved";
  } else {
    value = "unknown";
    display = "未知";
    state = "unknown";
  }

  String json = "{";
  json += "\"online\":" + String(online ? "true" : "false") + ",";
  json += "\"enabled\":" + String(enabled ? "true" : "false") + ",";
  json += "\"value\":\"" + JsonEscape(value) + "\",";
  json += "\"display\":\"" + JsonEscape(display) + "\",";
  json += "\"state\":\"" + JsonEscape(state) + "\"";
  if (raw.length()) json += ",\"raw\":\"" + JsonEscape(raw) + "\"";
  json += "}";
  return json;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceTemplatesJson() {
  String json = "[";
  for (int i = 0; i < LINGXI_TEMPLATE_COUNT; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"id\":\"" + JsonEscape(LingxiTemplates[i].id) + "\",";
    json += "\"name\":\"" + JsonEscape(LingxiTemplates[i].name) + "\",";
    json += "\"nodeType\":\"" + JsonEscape(LingxiTemplates[i].nodeType) + "\",";
    json += "\"endpointType\":\"" + JsonEscape(LingxiTemplates[i].endpointType) + "\",";
    json += "\"capability\":\"" + JsonEscape(LingxiTemplates[i].capability) + "\",";
    json += "\"defaultRoom\":\"" + JsonEscape(LingxiTemplates[i].defaultRoom) + "\",";
    json += "\"description\":\"" + JsonEscape(LingxiTemplates[i].description) + "\"";
    json += "}";
  }
  json += "]";
  return json;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceSummaryJson() {
  int onlineNodes = 0;
  int visibleNodeCount = 0;
  for (int i = 0; i < LINGXI_NODE_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiNodes[i].id))) continue;
    visibleNodeCount++;
    if (NodeDeviceNodeOnline(LingxiNodes[i].id)) onlineNodes++;
  }
  String json = "{";
  json += "\"enabled\":true,";
  json += "\"version\":\"" + JsonEscape(LINGXI_VERSION_CODE) + "\",";
  json += "\"releaseName\":\"" + JsonEscape(LINGXI_RELEASE_NAME) + "\",";
  json += "\"patch\":\"" + JsonEscape(LINGXI_VERSION_PATCH) + "\",";
  json += "\"nodeCount\":" + String(LINGXI_NODE_COUNT) + ",";
  json += "\"onlineNodeCount\":" + String(onlineNodes) + ",";
  json += "\"runtimeNodeCount\":" + String(NodeDeviceRuntimeCount(false)) + ",";
  json += "\"runtimeOnlineNodeCount\":" + String(NodeDeviceRuntimeCount(true)) + ",";
  json += "\"endpointCount\":" + String(LINGXI_ENDPOINT_COUNT) + ",";
  json += "\"templateCount\":" + String(LINGXI_TEMPLATE_COUNT);
  json += "}";
  return json;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceBuildJson() {
  int onlineNodes = 0;
  int visibleNodeCount = 0;
  for (int i = 0; i < LINGXI_NODE_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiNodes[i].id))) continue;
    visibleNodeCount++;
    if (NodeDeviceNodeOnline(LingxiNodes[i].id)) onlineNodes++;
  }

  int visibleEndpointCount = 0;
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiEndpoints[i].nodeId))) continue;
    visibleEndpointCount++;
  }

  String json = "{";
  json += "\"ok\":true,";
  json += "\"product\":\"" + JsonEscape(LINGXI_PRODUCT_NAME) + "\",";
  json += "\"releaseName\":\"" + JsonEscape(LINGXI_RELEASE_NAME) + "\",";
  json += "\"versionCode\":\"" + JsonEscape(LINGXI_VERSION_CODE) + "\",";
  json += "\"stage\":\"" + JsonEscape(LINGXI_VERSION_STAGE) + "\",";
  json += "\"patch\":\"" + JsonEscape(LINGXI_VERSION_PATCH) + "\",";
  json += "\"baseline\":\"" + JsonEscape(LINGXI_BASELINE_VERSION) + "\",";
  json += "\"nodeCount\":" + String(visibleNodeCount) + ",";
  json += "\"onlineNodeCount\":" + String(onlineNodes) + ",";
  json += "\"runtimeNodeCount\":" + String(NodeDeviceRuntimeCount(false)) + ",";
  json += "\"runtimeOnlineNodeCount\":" + String(NodeDeviceRuntimeCount(true)) + ",";
  json += "\"endpointCount\":" + String(visibleEndpointCount) + ",";
  json += "\"templateCount\":" + String(LINGXI_TEMPLATE_COUNT) + ",";

  json += "\"nodes\":[";
  bool firstNode = true;
  for (int i = 0; i < LINGXI_NODE_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiNodes[i].id))) continue;
    if (!firstNode) json += ",";
    firstNode = false;
    bool online = NodeDeviceNodeOnline(LingxiNodes[i].id);
    int rIdx = NodeDeviceRuntimeFind(String(LingxiNodes[i].id));
    String stateLabel = rIdx >= 0 ? NodeDeviceRuntimeStateLabel(rIdx) : (online ? String("在线") : String("离线"));
    String stateClass = rIdx >= 0 ? NodeDeviceRuntimeStateClass(rIdx) : (online ? String("ok") : String("off"));
    json += "{";
    json += "\"id\":\"" + JsonEscape(LingxiNodes[i].id) + "\",";
    json += "\"name\":\"" + JsonEscape(LingxiNodes[i].name) + "\",";
    json += "\"nodeType\":\"" + JsonEscape(LingxiNodes[i].nodeType) + "\",";
    json += "\"mode\":\"" + JsonEscape(LingxiNodes[i].mode) + "\",";
    json += "\"role\":\"" + JsonEscape(LingxiNodes[i].role) + "\",";
    json += "\"transport\":\"" + JsonEscape(LingxiNodes[i].transport) + "\",";
    json += "\"mac\":\"" + JsonEscape(NodeDeviceNodeMac(LingxiNodes[i].id)) + "\",";
    json += "\"online\":" + String(online ? "true" : "false") + ",";
    json += "\"state_label\":\"" + JsonEscape(stateLabel) + "\",";
    json += "\"state_class\":\"" + JsonEscape(stateClass) + "\",";
    json += "\"description\":\"" + JsonEscape(LingxiNodes[i].description) + "\",";
    json += "\"registry\":" + NodeDeviceRuntimeForStaticJson(LingxiNodes[i].id);
    json += "}";
  }
  json += "],";

  json += "\"endpoints\":[";
  bool firstEndpoint = true;
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiEndpoints[i].nodeId))) continue;
    if (!firstEndpoint) json += ",";
    firstEndpoint = false;
    json += "{";
    json += "\"id\":\"" + JsonEscape(LingxiEndpoints[i].id) + "\",";
    json += "\"nodeId\":\"" + JsonEscape(LingxiEndpoints[i].nodeId) + "\",";
    json += "\"physicalMode\":\"" + JsonEscape(DeviceConfigPhysicalModeText(LingxiEndpoints[i])) + "\",";
    json += "\"name\":\"" + JsonEscape(DeviceConfigEndpointName(i)) + "\",";
    json += "\"defaultName\":\"" + JsonEscape(LingxiEndpoints[i].name) + "\",";
    json += "\"endpointType\":\"" + JsonEscape(LingxiEndpoints[i].endpointType) + "\",";
    json += "\"category\":\"" + JsonEscape(LingxiEndpoints[i].category) + "\",";
    json += "\"room\":\"" + JsonEscape(DeviceConfigEndpointRoom(i)) + "\",";
    json += "\"defaultRoom\":\"" + JsonEscape(LingxiEndpoints[i].room) + "\",";
    json += "\"capability\":\"" + JsonEscape(LingxiEndpoints[i].capability) + "\",";
    json += "\"unit\":\"" + JsonEscape(LingxiEndpoints[i].unit) + "\",";
    json += "\"commandOn\":\"" + JsonEscape(LingxiEndpoints[i].commandOn) + "\",";
    json += "\"commandOff\":\"" + JsonEscape(LingxiEndpoints[i].commandOff) + "\",";
    json += "\"templateId\":\"" + JsonEscape(LingxiEndpoints[i].templateId) + "\",";
    json += "\"enabled\":" + String(DeviceConfigEndpointEnabled(i) ? "true" : "false") + ",";
    json += "\"reserved\":" + String(LingxiEndpoints[i].reserved ? "true" : "false") + ",";
    json += "\"runtime\":" + NodeDeviceEndpointStateJson(LingxiEndpoints[i]);
    json += "}";
  }
  json += "],";
  json += "\"registeredNodes\":" + NodeDeviceRuntimeListJson() + ",";
  json += "\"templates\":" + NodeDeviceTemplatesJson() + ",";
  json += "\"routes\":{\"diagnosticPage\":\"/nodes\",\"deviceConfigPage\":\"/device-config\",\"nodeStatus\":\"/api/node_device/status\",\"nodeRegister\":\"/api/node/register\",\"nodeReport\":\"/api/node/report\",\"nodeDelete\":\"/api/node/delete\",\"nodeAdd\":\"/api/node/add\",\"nodeRecheck\":\"/api/node/recheck\",\"deviceConfigStatus\":\"/api/device_config/status\",\"legacyStatus\":\"/api/status\"},";
  json += "\"note\":\"V7.3 keeps V6 UI and adds lightweight node lifecycle operations: recheck, add, delete/hide.\"";
  json += "}";
  return json;
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceHtmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceJsonField(String json, const char* key) {
  String pat = "\"" + String(key) + "\":\"";
  int p = json.indexOf(pat);
  if (p < 0) return "";
  p += pat.length();
  int e = json.indexOf("\"", p);
  if (e < 0) return "";
  String v = json.substring(p, e);
  v.replace("\\\"", "\"");
  v.replace("\\\\", "\\");
  return v;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceEndpointDisplayText(const LingxiEndpointInfo &ep) {
  String runtime = NodeDeviceEndpointStateJson(ep);
  String display = NodeDeviceJsonField(runtime, "display");
  if (!display.length()) display = "--";
  String state = NodeDeviceJsonField(runtime, "state");
  if (state.length()) display += " / " + state;
  return display;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceActionButton(const char* cmd, const char* text, bool offStyle) {
  String c = String(cmd);
  if (!c.length()) return "";
  String html = "<button";
  if (offStyle) html += " class='off'";
  html += " onclick=\"fetch('/api/cmd?cmd=" + NodeDeviceHtmlEscape(c) + "').then(function(){location.reload();})\">";
  html += NodeDeviceHtmlEscape(String(text));
  html += "</button>";
  return html;
}


// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDeviceRuntimeTableHtml() {
  String html;
  html.reserve(9000);
  html += F("<section class='card' id='v3-node-registry'><h2>V3 节点注册表</h2><p class='muted'>节点通过 <code>POST /api/node/register</code> 注册能力，通过 <code>POST /api/node/report</code> 上报数据。当前用于展示中控、感知层、执行层的节点注册、在线状态、能力与最近上报；不改变原控制链路。</p><div class='tablewrap'><table><thead><tr><th>状态</th><th>节点</th><th>角色/类型</th><th>网络</th><th>能力</th><th>最近上报</th><th>次数</th><th>操作</th></tr></thead><tbody>");
  for (int i = 0; i < LINGXI_RUNTIME_NODE_MAX; i++) {
    if (!RuntimeNodes[i].used) continue;
    LingxiRuntimeNode &n = RuntimeNodes[i];
    bool online = NodeDeviceRuntimeOnline(i);
    String stateLabel = NodeDeviceRuntimeStateLabel(i);
    String stateClass = NodeDeviceRuntimeStateClass(i);
    String eps = n.endpointsJson;
    if (eps.length() > 180) eps = eps.substring(0, 180) + "...";
    html += "<tr><td><span class='pill " + stateClass + "'>" + stateLabel + "</span></td>";
    html += "<td><b>" + NodeDeviceHtmlEscape(n.name) + "</b><br><code>" + NodeDeviceHtmlEscape(n.nodeId) + "</code></td>";
    html += "<td>" + NodeDeviceHtmlEscape(n.role) + "<br><span class='muted'>" + NodeDeviceHtmlEscape(n.nodeType) + " · " + NodeDeviceHtmlEscape(n.transport) + "</span></td>";
    String macText = n.mac.length() ? n.mac : String("未上报");
    html += "<td>IP：" + NodeDeviceHtmlEscape(n.ip) + "<br><span class='muted'>MAC：" + NodeDeviceHtmlEscape(macText) + "</span></td>";
    html += "<td><span class='pill info'>" + String(n.endpointCount) + " 个能力</span><br><span class='muted'>" + NodeDeviceHtmlEscape(eps) + "</span></td>";
    html += "<td>" + NodeDeviceHtmlEscape(NodeDeviceRuntimeAgeText(n.lastSeenMs)) + "<br><span class='muted'>状态：" + NodeDeviceHtmlEscape(n.lastStatus) + "</span></td>";
    html += "<td>注册 " + String(n.registerCount) + "<br>上报 " + String(n.reportCount) + "</td>";
    if (n.nodeId == "Lingxi-Gateway") html += "<td><a class='btn ghost' href='/api/node/recheck'>重新检测</a></td></tr>";
    else html += "<td><a class='btn ghost' href='/api/node/recheck?node_id=" + NodeDeviceHtmlEscape(n.nodeId) + "'>重新检测</a> <a class='btn danger' href='/api/node/delete?node_id=" + NodeDeviceHtmlEscape(n.nodeId) + "' onclick=\"return confirm('确认删除节点：" + NodeDeviceHtmlEscape(n.nodeId) + "？')\">删除节点</a></td></tr>";
  }
  html += F("</tbody></table></div><div class='note'>V3.4.2 说明：此表用于开发者诊断和技术汇报展示。日常用户不需要手动输入地址，入口已放入主 WebUI 的“系统设置 → 关于系统 → 开发者管理”。</div></section>");
  return html;
}


// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigEndpointTypeText(const LingxiEndpointInfo &ep) {
  String t = String(ep.endpointType);
  if (t == "sensor") return "传感器";
  if (t == "action") return "执行器";
  return t;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigCategoryText(const LingxiEndpointInfo &ep) {
  String c = String(ep.category);
  if (c == "environment") return "环境";
  if (c == "security") return "安防";
  if (c == "presence") return "人体";
  if (c == "safety") return "安全";
  if (c == "light") return "灯光";
  if (c == "curtain") return "窗帘";
  if (c == "window") return "窗户";
  if (c == "relay") return "电源";
  if (c == "plug") return "插座";
  return c;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigCapabilityText(const LingxiEndpointInfo &ep) {
  String c = String(ep.capability);
  if (c == "temperature") return "温度";
  if (c == "humidity") return "湿度";
  if (c == "light") return "光照";
  if (c == "contact") return "门窗磁";
  if (c == "presence") return "人体感应";
  if (c == "smoke") return "烟雾";
  if (c == "gas") return "燃气";
  if (c == "switch") return "开关";
  if (c == "curtain") return "窗帘";
  if (c == "window") return "窗户";
  if (c == "power") return "电源";
  if (c == "reserved") return "预留";
  return c;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigEndpointUiStatus(const LingxiEndpointInfo &ep) {
  if (ep.reserved) return "待添加";
  if (!NodeDeviceNodeOnline(ep.nodeId)) return "离线";
  String id = String(ep.id);
  if (id == "smoke" && State.smokeValue > YANWU_YUZHI) return "异常";
  if (id == "gas" && State.gasValue > RANQI_YUZHI) return "异常";
  return "在线";
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigEndpointUiStatusClass(const LingxiEndpointInfo &ep) {
  String st = DeviceConfigEndpointUiStatus(ep);
  if (st == "在线") return "ok";
  if (st == "异常") return "warn";
  if (st == "待添加") return "warn";
  return "off";
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
int DeviceConfigCountEndpointType(const char* endpointType) {
  int count = 0;
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (LingxiEndpoints[i].reserved) continue;
    if (String(LingxiEndpoints[i].endpointType) == String(endpointType)) count++;
  }
  return count;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
int DeviceConfigCountEndpointNode(const char* nodeId) {
  int count = 0;
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (LingxiEndpoints[i].reserved) continue;
    if (String(LingxiEndpoints[i].nodeId) == String(nodeId)) count++;
  }
  return count;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
int DeviceConfigRoomList(String rooms[], int maxRooms) {
  int roomCount = 0;
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (LingxiEndpoints[i].reserved) continue;
    String room = DeviceConfigEndpointRoom(i);
    room.trim();
    if (!room.length()) room = "未分配";
    bool exists = false;
    for (int j = 0; j < roomCount; j++) {
      if (rooms[j] == room) { exists = true; break; }
    }
    if (!exists && roomCount < maxRooms) rooms[roomCount++] = room;
  }
  return roomCount;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigStatusJson() {
  int enabledEndpoints = 0;
  int sensorEndpoints = 0;
  int actionEndpoints = 0;
  int visibleEndpointCount = 0;
  int visibleNodeCount = 0;
  for (int i = 0; i < LINGXI_NODE_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiNodes[i].id))) continue;
    visibleNodeCount++;
  }
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiEndpoints[i].nodeId))) continue;
    visibleEndpointCount++;
    if (DeviceConfigEndpointEnabled(i)) enabledEndpoints++;
    if (String(LingxiEndpoints[i].nodeId) == "SensorBoard-01") sensorEndpoints++;
    if (String(LingxiEndpoints[i].nodeId) == "ActionBoard-01") actionEndpoints++;
  }

  String json = "{";
  json += "\"releaseName\":\"" + JsonEscape(LINGXI_RELEASE_NAME) + "\",";
  json += "\"versionCode\":\"" + JsonEscape(LINGXI_VERSION_CODE) + "\",";
  json += "\"patch\":\"" + JsonEscape(LINGXI_VERSION_PATCH) + "\",";
  json += "\"mode\":\"integrated_board_adapter\",";
  json += "\"summary\":{";
  json += "\"nodes\":" + String(visibleNodeCount) + ",";
  json += "\"endpoints\":" + String(visibleEndpointCount) + ",";
  json += "\"enabledEndpoints\":" + String(enabledEndpoints) + ",";
  json += "\"sensorBoardEndpoints\":" + String(sensorEndpoints) + ",";
  json += "\"actionBoardEndpoints\":" + String(actionEndpoints);
  json += "},";
  json += "\"description\":\"当前仍然是三板集成架构：V7.3 在 V6 UI 基础上增加节点重新检测、添加和删除/隐藏。\",";
  json += "\"registeredNodes\":" + NodeDeviceRuntimeListJson() + ",";
  json += "\"endpoints\":[";
  bool firstEndpoint = true;
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiEndpoints[i].nodeId))) continue;
    if (!firstEndpoint) json += ",";
    firstEndpoint = false;
    const LingxiEndpointInfo &ep = LingxiEndpoints[i];
    json += "{";
    json += "\"id\":\"" + JsonEscape(ep.id) + "\",";
    json += "\"nodeId\":\"" + JsonEscape(ep.nodeId) + "\",";
    json += "\"physicalMode\":\"" + JsonEscape(DeviceConfigPhysicalModeText(ep)) + "\",";
    json += "\"name\":\"" + JsonEscape(DeviceConfigEndpointName(i)) + "\",";
    json += "\"defaultName\":\"" + JsonEscape(ep.name) + "\",";
    json += "\"room\":\"" + JsonEscape(DeviceConfigEndpointRoom(i)) + "\",";
    json += "\"defaultRoom\":\"" + JsonEscape(ep.room) + "\",";
    json += "\"endpointType\":\"" + JsonEscape(ep.endpointType) + "\",";
    json += "\"category\":\"" + JsonEscape(ep.category) + "\",";
    json += "\"capability\":\"" + JsonEscape(ep.capability) + "\",";
    json += "\"templateId\":\"" + JsonEscape(ep.templateId) + "\",";
    json += "\"reserved\":" + String(ep.reserved ? "true" : "false") + ",";
    json += "\"enabled\":" + String(DeviceConfigEndpointEnabled(i) ? "true" : "false") + ",";
    json += "\"runtime\":" + NodeDeviceEndpointStateJson(ep);
    json += "}";
  }
  json += "]}";
  return json;
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DeviceConfigPageHtml() {
  String html;
  html.reserve(44000);

  int onlineNodes = 0;
  int visibleNodeCount = 0;
  for (int i = 0; i < LINGXI_NODE_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiNodes[i].id))) continue;
    visibleNodeCount++;
    if (NodeDeviceNodeOnline(LingxiNodes[i].id)) onlineNodes++;
  }
  int sensorCount = DeviceConfigCountEndpointType("sensor");
  int actionCount = DeviceConfigCountEndpointType("action");
  String rooms[24];
  int roomCount = DeviceConfigRoomList(rooms, 24);

  html += F("<!doctype html><html lang='zh-CN'><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += "<title>" + String(LINGXI_RELEASE_NAME) + " 设备管理</title><style>";
  html += F(":root{--bg:#f4f7fb;--card:#fff;--ink:#111827;--muted:#64748b;--line:#e2e8f0;--brand:#2563eb;--brand2:#38bdf8;--ok:#16a34a;--bad:#dc2626;--warn:#d97706;--soft:#eff6ff;--off:#64748b;}*{box-sizing:border-box}body{margin:0;background:linear-gradient(180deg,#eef5ff 0,#f7f9fc 240px,#f4f7fb 100%);color:var(--ink);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC','Microsoft YaHei',Arial,sans-serif;}header{padding:22px 18px 18px;background:linear-gradient(135deg,#0f172a,#1d4ed8 58%,#38bdf8);color:white}header h1{margin:0 0 8px;font-size:25px;letter-spacing:.02em}header p{margin:0;color:#dbeafe;line-height:1.6}.toplink{float:right;color:white;text-decoration:none;font-weight:800;margin-left:12px}.wrap{padding:16px;max-width:1240px;margin:0 auto}.summary{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:12px;margin-top:14px}.card{background:rgba(255,255,255,.96);border:1px solid var(--line);border-radius:20px;padding:16px;box-shadow:0 10px 28px rgba(15,23,42,.07);margin-top:14px}.card h2{margin:0 0 12px;font-size:18px}.metric{font-size:28px;font-weight:900}.muted{color:var(--muted);font-size:13px;line-height:1.55}.tabs{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin:14px 0}.tab{display:block;text-decoration:none;color:#1e293b;background:var(--card);border:1px solid var(--line);border-radius:16px;padding:12px 14px;font-weight:800;box-shadow:0 6px 16px rgba(15,23,42,.05)}.tab span{display:block;color:var(--muted);font-size:12px;font-weight:600;margin-top:4px}.pill{display:inline-flex;align-items:center;border-radius:999px;padding:4px 9px;font-size:12px;font-weight:800;white-space:nowrap}.ok{background:#dcfce7;color:#166534}.bad{background:#f1f5f9;color:#64748b}.off{background:#f1f5f9;color:#64748b}.warn{background:#fef3c7;color:#92400e}.info{background:#dbeafe;color:#1d4ed8}.devicegrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(235px,1fr));gap:12px}.device{border:1px solid var(--line);background:#fff;border-radius:18px;padding:14px}.device b{font-size:16px}.device .icon{width:38px;height:38px;border-radius:14px;background:linear-gradient(135deg,#dbeafe,#bfdbfe);display:inline-flex;align-items:center;justify-content:center;margin-right:10px}.row{display:flex;align-items:center;gap:8px}.tablewrap{overflow:auto}table{width:100%;border-collapse:collapse}th,td{border-bottom:1px solid var(--line);padding:10px 8px;text-align:left;font-size:14px;vertical-align:top}th{color:var(--muted);font-weight:800;background:#f8fafc}input{border:1px solid var(--line);border-radius:10px;padding:8px;min-width:110px;background:#fff}button,.btn{border:0;border-radius:10px;padding:8px 12px;background:var(--brand);color:white;font-weight:800;text-decoration:none;display:inline-flex}.danger{background:#64748b}.ghost{background:#eef2ff;color:#1d4ed8}code{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;background:#eef2ff;border-radius:6px;padding:2px 5px}.note{background:#f8fafc;border:1px dashed #cbd5e1;border-radius:16px;padding:12px;color:#475569;margin-top:10px}</style></head><body>");
  html += "<header><a class='toplink' href='/'>返回主控台</a><h1>设备管理</h1><p>" + String(LINGXI_RELEASE_NAME) + "：设备管理只负责房间、归属和设备身份信息；执行器仍在设备控制操作，传感器仍在环境监测展示。</p></header>";
  html += F("<div class='wrap'>");

  html += F("<section class='summary'>");
  html += "<div class='card'><div class='muted'>已接入设备</div><div class='metric'>" + String(sensorCount + actionCount) + "</div><div class='muted'>传感器 " + String(sensorCount) + " · 执行器 " + String(actionCount) + "</div></div>";
  html += "<div class='card'><div class='muted'>房间</div><div class='metric'>" + String(roomCount) + "</div><div class='muted'>可在房间管理中调整归属</div></div>";
  html += "<div class='card'><div class='muted'>网关与节点</div><div class='metric'>" + String(onlineNodes) + "/" + String(visibleNodeCount) + "</div><div class='muted'>在线 / 总数</div></div>";
  html += "<div class='card'><div class='muted'>版本</div><div class='metric' style='font-size:20px'>" + String(LINGXI_VERSION_CODE) + "</div><div class='muted'>" + String(LINGXI_VERSION_PATCH) + "</div></div>";
  html += F("</section>");

  html += F("<nav class='tabs'><a class='tab' href='#devices'>已接入设备<span>查看传感器与执行器</span></a><a class='tab' href='#rooms'>房间管理<span>只调整房间归属</span></a><a class='tab' href='#gateway-nodes'>网关与节点<span>查看在线与归属</span></a><a class='tab' href='#add-device'>添加设备<span>预留未来节点化</span></a></nav>");

  html += F("<section class='card' id='devices'><h2>已接入设备</h2><p class='muted'>设备名称、设备类型、能力、所属节点属于设备身份信息，当前页面只调整房间归属。</p><div class='devicegrid'>");
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    const LingxiEndpointInfo &ep = LingxiEndpoints[i];
    if (NodeDeviceHidden(String(ep.nodeId))) continue;
    String kind = DeviceConfigEndpointTypeText(ep);
    String status = DeviceConfigEndpointUiStatus(ep);
    String cls = DeviceConfigEndpointUiStatusClass(ep);
    String target = String(ep.endpointType) == "sensor" ? "环境监测" : "设备控制";
    html += "<div class='device'><div class='row'><span class='icon'>" + String(String(ep.endpointType) == "sensor" ? "感" : "控") + "</span><div><b>" + NodeDeviceHtmlEscape(DeviceConfigEndpointName(i)) + "</b><br><span class='muted'>" + NodeDeviceHtmlEscape(DeviceConfigEndpointRoom(i)) + " · " + kind + "</span></div></div>";
    html += "<p><span class='pill " + cls + "'>" + status + "</span> <span class='pill info'>" + DeviceConfigCategoryText(ep) + " / " + DeviceConfigCapabilityText(ep) + "</span></p>";
    html += "<p class='muted'>入口：" + target + "<br>身份：<code>" + NodeDeviceHtmlEscape(ep.id) + "</code><br>归属：<code>" + NodeDeviceHtmlEscape(ep.nodeId) + "</code></p>";
    html += "<p><b>当前：</b>" + NodeDeviceHtmlEscape(NodeDeviceEndpointDisplayText(ep)) + "</p></div>";
  }
  html += F("</div></section>");

  html += F("<section class='card' id='rooms'><h2>房间管理</h2><p class='muted'>这里只调整房间归属，设备身份和控制能力由系统自动维护。</p><div class='tablewrap'><table><thead><tr><th>设备</th><th>固定身份</th><th>类型/能力</th><th>当前房间</th><th>修改房间</th></tr></thead><tbody>");
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    const LingxiEndpointInfo &ep = LingxiEndpoints[i];
    if (NodeDeviceHidden(String(ep.nodeId))) continue;
    if (ep.reserved) continue;
    html += "<tr><form method='GET' action='/api/device_config/save'>";
    html += "<td><b>" + NodeDeviceHtmlEscape(DeviceConfigEndpointName(i)) + "</b><br><span class='muted'>" + NodeDeviceHtmlEscape(DeviceConfigEndpointRoom(i)) + "</span></td>";
    html += "<td><code>" + NodeDeviceHtmlEscape(ep.id) + "</code><br><span class='muted'>" + NodeDeviceHtmlEscape(ep.nodeId) + "</span><input type='hidden' name='id' value='" + NodeDeviceHtmlEscape(ep.id) + "'></td>";
    html += "<td>" + DeviceConfigEndpointTypeText(ep) + "<br><span class='muted'>" + DeviceConfigCapabilityText(ep) + "</span></td>";
    html += "<td>" + NodeDeviceHtmlEscape(DeviceConfigEndpointRoom(i)) + "</td>";
    html += "<td><input name='room' value='" + NodeDeviceHtmlEscape(DeviceConfigEndpointRoom(i)) + "'><button type='submit'>保存</button></td>";
    html += F("</form></tr>");
  }
  html += F("</tbody></table></div></section>");

  html += F("<section class='card' id='gateway-nodes'><h2>网关与节点</h2><p class='muted'>这里查看网关、感知层集成板、执行层集成板和后续子节点的在线情况。</p><div class='tablewrap'><table><thead><tr><th>状态</th><th>名称</th><th>身份标识</th><th>角色</th><th>通信</th><th>挂载设备</th><th>说明</th><th>操作</th></tr></thead><tbody>");
  for (int i = 0; i < LINGXI_NODE_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiNodes[i].id))) continue;
    bool online = NodeDeviceNodeOnline(LingxiNodes[i].id);
    int rIdx = NodeDeviceRuntimeFind(String(LingxiNodes[i].id));
    String stateLabel = rIdx >= 0 ? NodeDeviceRuntimeStateLabel(rIdx) : (online ? String("在线") : String("离线"));
    String stateClass = rIdx >= 0 ? NodeDeviceRuntimeStateClass(rIdx) : (online ? String("ok") : String("off"));
    String role = String(LingxiNodes[i].role);
    if (role == "center") role = "中控网关";
    else if (role == "sensor") role = "感知节点";
    else if (role == "executor") role = "执行节点";
    html += "<tr><td><span class='pill " + stateClass + "'>" + stateLabel + "</span></td>";
    html += "<td><b>" + NodeDeviceHtmlEscape(LingxiNodes[i].name) + "</b></td>";
    html += "<td><code>" + NodeDeviceHtmlEscape(LingxiNodes[i].id) + "</code><br><span class='muted'>" + NodeDeviceHtmlEscape(NodeDeviceNodeMac(LingxiNodes[i].id)) + "</span></td>";
    html += "<td>" + NodeDeviceHtmlEscape(role) + "<br><span class='muted'>" + NodeDeviceHtmlEscape(LingxiNodes[i].mode) + "</span></td>";
    html += "<td>" + NodeDeviceHtmlEscape(LingxiNodes[i].transport) + "</td>";
    html += "<td>" + String(DeviceConfigCountEndpointNode(LingxiNodes[i].id)) + "</td>";
    html += "<td>" + NodeDeviceHtmlEscape(LingxiNodes[i].description) + "</td>";
    if (String(LingxiNodes[i].id) == "Lingxi-Gateway") html += "<td><a class='btn ghost' href='/api/node/recheck'>重新检测</a></td></tr>";
    else html += "<td><a class='btn ghost' href='/api/node/recheck?node_id=" + NodeDeviceHtmlEscape(String(LingxiNodes[i].id)) + "'>重新检测</a> <a class='btn danger' href='/api/node/delete?node_id=" + NodeDeviceHtmlEscape(String(LingxiNodes[i].id)) + "' onclick=\"return confirm('确认删除节点：" + NodeDeviceHtmlEscape(String(LingxiNodes[i].id)) + "？')\">删除节点</a></td></tr>";
  }
  html += F("</tbody></table></div></section>");

  html += NodeDeviceRuntimeTableHtml();

  html += F("<section class='card' id='add-device'><h2>添加设备</h2><div class='devicegrid'><div class='device'><b>当前网关与节点</b><p class='muted'>现阶段默认接入中控网关、感知层集成板和执行层集成板，保持原有稳定链路。</p><span class='pill ok'>已启用</span></div><div class='device'><b>新增子节点</b><p class='muted'>新节点首次上电后可进入配网模式，完成 WiFi 与网关绑定后会出现在设备管理中。</p><span class='pill warn'>后续支持</span></div><div class='device'><b>系统升级</b><p class='muted'>后续可扩展为资源包、语音包和节点固件的统一管理。</p><span class='pill info'>后续支持</span></div></div><div class='note'>为保证稳定，设备接线和高级通信参数请在对应节点中维护。</div></section>");

  html += F("<section class='card'><h2>维护</h2><p><a class='btn danger' href='/api/device_config/reset'>恢复默认房间配置</a> <a class='btn ghost' href='/api/device_config/status'>查看配置状态</a> <a class='btn ghost' href='/api/node_device/status'>查看节点状态</a></p></section>");
  html += F("</div></body></html>");
  return html;
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String NodeDevicePageHtml() {
  String html;
  html.reserve(22000);
  int onlineNodes = 0;
  int visibleNodeCount = 0;
  int enabledEndpoints = 0;
  for (int i = 0; i < LINGXI_NODE_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiNodes[i].id))) continue;
    visibleNodeCount++;
    if (NodeDeviceNodeOnline(LingxiNodes[i].id)) onlineNodes++;
  }
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiEndpoints[i].nodeId))) continue;
    if (DeviceConfigEndpointEnabled(i)) enabledEndpoints++;
  }

  html += F("<!doctype html><html lang='zh-CN'><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>灵汐正式版V3.4.0 网关与节点</title><style>");
  html += F(":root{--bg:#f5f7fb;--card:#fff;--ink:#142033;--muted:#6a7890;--line:#dfe7f3;--brand:#2563eb;--ok:#16a34a;--bad:#dc2626;--warn:#d97706;}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC','Microsoft YaHei',Arial,sans-serif;}header{padding:20px 18px;background:linear-gradient(135deg,#0f172a,#1d4ed8);color:white}header h1{margin:0 0 8px;font-size:24px}header p{margin:0;color:#dbeafe}main{padding:16px;max-width:1180px;margin:0 auto}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px}.card{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:16px;box-shadow:0 8px 24px rgba(15,23,42,.06);margin-top:14px}.card h2{font-size:18px;margin:0 0 12px}.metric{font-size:28px;font-weight:800}.muted{color:var(--muted);font-size:13px}.pill{display:inline-flex;align-items:center;border-radius:999px;padding:4px 9px;font-size:12px;font-weight:700}.ok{background:#dcfce7;color:#166534}.bad{background:#f1f5f9;color:#64748b}.off{background:#f1f5f9;color:#64748b}.warn{background:#fef3c7;color:#92400e}table{width:100%;border-collapse:collapse}th,td{border-bottom:1px solid var(--line);padding:10px 8px;text-align:left;font-size:14px;vertical-align:top}th{color:var(--muted);font-weight:700;background:#f8fafc}.toplink{float:right;color:white;text-decoration:none;font-weight:700;margin-left:12px}code{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;background:#eef2ff;border-radius:6px;padding:2px 5px}.tablewrap{overflow:auto}</style></head><body>");
  html += F("<header><a class='toplink' href='/'>返回主控台</a><a class='toplink' href='/device-config'>设备管理配置区</a><h1>网关与节点</h1><p>这里用于开发者查看网关、感知层、执行层的注册、在线、能力与状态；日常控制仍在主控台或设备控制页。</p></header><main>");

  html += F("<section class='grid'>");
  html += "<div class='card'><div class='muted'>版本</div><div class='metric'>" + String(LINGXI_VERSION_CODE) + "</div><div>" + String(LINGXI_RELEASE_NAME) + "</div></div>";
  html += "<div class='card'><div class='muted'>节点</div><div class='metric'>" + String(onlineNodes) + "/" + String(visibleNodeCount) + "</div><div>在线 / 总数</div></div>";
  html += "<div class='card'><div class='muted'>接入设备</div><div class='metric'>" + String(enabledEndpoints) + "/" + String(LINGXI_ENDPOINT_COUNT) + "</div><div>设备状态</div></div>";
  html += "<div class='card'><div class='muted'>更新内容</div><div class='metric' style='font-size:20px'>" + String(LINGXI_VERSION_PATCH) + "</div><div class='muted'>基于：" + String(LINGXI_BASELINE_VERSION) + "</div></div>";
  html += F("</section>");

  html += F("<section class='card'><h2>节点列表</h2><div class='tablewrap'><table><thead><tr><th>状态</th><th>节点</th><th>类型</th><th>模式</th><th>通信</th><th>MAC</th><th>说明</th><th>操作</th></tr></thead><tbody>");
  for (int i = 0; i < LINGXI_NODE_COUNT; i++) {
    if (NodeDeviceHidden(String(LingxiNodes[i].id))) continue;
    bool online = NodeDeviceNodeOnline(LingxiNodes[i].id);
    int rIdx = NodeDeviceRuntimeFind(String(LingxiNodes[i].id));
    String stateLabel = rIdx >= 0 ? NodeDeviceRuntimeStateLabel(rIdx) : (online ? String("在线") : String("离线"));
    String stateClass = rIdx >= 0 ? NodeDeviceRuntimeStateClass(rIdx) : (online ? String("ok") : String("off"));
    html += "<tr><td><span class='pill " + stateClass + "'>" + stateLabel + "</span></td>";
    html += "<td><b>" + NodeDeviceHtmlEscape(LingxiNodes[i].name) + "</b><br><code>" + NodeDeviceHtmlEscape(LingxiNodes[i].id) + "</code></td>";
    html += "<td>" + NodeDeviceHtmlEscape(LingxiNodes[i].nodeType) + "</td>";
    html += "<td>" + NodeDeviceHtmlEscape(LingxiNodes[i].mode) + "</td>";
    html += "<td>" + NodeDeviceHtmlEscape(LingxiNodes[i].transport) + "</td>";
    html += "<td>" + NodeDeviceHtmlEscape(NodeDeviceNodeMac(LingxiNodes[i].id)) + "</td>";
    html += "<td>" + NodeDeviceHtmlEscape(LingxiNodes[i].description) + "</td>";
    if (String(LingxiNodes[i].id) == "Lingxi-Gateway") html += "<td><a class='btn ghost' href='/api/node/recheck'>重新检测</a></td></tr>";
    else html += "<td><a class='btn ghost' href='/api/node/recheck?node_id=" + NodeDeviceHtmlEscape(String(LingxiNodes[i].id)) + "'>重新检测</a> <a class='btn danger' href='/api/node/delete?node_id=" + NodeDeviceHtmlEscape(String(LingxiNodes[i].id)) + "' onclick=\"return confirm('确认删除节点：" + NodeDeviceHtmlEscape(String(LingxiNodes[i].id)) + "？')\">删除节点</a></td></tr>";
  }
  html += F("</tbody></table></div></section>");

  html += NodeDeviceRuntimeTableHtml();

  html += F("<section class='card'><h2>设备状态</h2><div class='tablewrap'><table><thead><tr><th>状态</th><th>设备</th><th>所属节点</th><th>房间/能力</th><th>当前值</th></tr></thead><tbody>");
  for (int i = 0; i < LINGXI_ENDPOINT_COUNT; i++) {
    const LingxiEndpointInfo &ep = LingxiEndpoints[i];
    if (NodeDeviceHidden(String(ep.nodeId))) continue;
    bool enabled = DeviceConfigEndpointEnabled(i);
    bool online = enabled && !ep.reserved && (NodeDeviceNodeOnline(ep.nodeId) || NodeDeviceRuntimeEndpointOnline(ep.id));  
    html += "<tr><td><span class='pill ";
    html += ep.reserved ? "warn'>预留" : (enabled ? (online ? "ok'>启用/在线" : "warn'>启用/离线") : "bad'>停用");
    html += "</span></td>";
    html += "<td><b>" + NodeDeviceHtmlEscape(DeviceConfigEndpointName(i)) + "</b><br><code>" + NodeDeviceHtmlEscape(ep.id) + "</code></td>";
    html += "<td>" + NodeDeviceHtmlEscape(ep.nodeId) + "<br><span class='muted'>" + NodeDeviceHtmlEscape(DeviceConfigPhysicalModeText(ep)) + "</span></td>";
    html += "<td>" + NodeDeviceHtmlEscape(DeviceConfigEndpointRoom(i)) + " / " + NodeDeviceHtmlEscape(ep.capability) + "<br><span class='muted'>" + NodeDeviceHtmlEscape(ep.category) + " · " + NodeDeviceHtmlEscape(ep.templateId) + "</span></td>";
    html += "<td><b>" + NodeDeviceHtmlEscape(NodeDeviceEndpointDisplayText(ep)) + "</b></td></tr>";
  }
  html += F("</tbody></table></div></section>");
  html += F("<section class='card'><h2>说明</h2><p>此页面用于查看节点和设备状态。日常控制请前往主控台，房间归属请前往设备管理。</p></section>");
  html += F("</main></body></html>");
  return html;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleDeviceConfigStatusApi() {
  xSemaphoreTake(StateMutex, portMAX_DELAY);
  String json = DeviceConfigStatusJson();
  xSemaphoreGive(StateMutex);
  SendCorsHeader();
  server.send(200, "application/json", json);
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleDeviceConfigPage() {
  SendCorsHeader();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "Redirect to main WebUI");
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleDeviceConfigSaveApi() {
  String id = server.arg("id");
  int idx = NodeDeviceFindEndpointIndex(id.c_str());
  if (idx < 0) {
    SendCorsHeader();
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"endpoint not found\"}");
    return;
  }

  String room = server.arg("room");
  room.trim();
  if (room.length() == 0) room = LingxiEndpoints[idx].room;
  if (room.length() > 24) room = room.substring(0, 24);

  DeviceEndpointName[idx] = String(LingxiEndpoints[idx].name);
  DeviceEndpointRoom[idx] = room;
  DeviceEndpointEnabled[idx] = !LingxiEndpoints[idx].reserved;

  DevicePrefs.begin("nodecfg", false);
  String roomKey = DeviceConfigKey("rm", idx);
  DevicePrefs.putString(roomKey.c_str(), room);
  DevicePrefs.end();

  AddCenterLog("device", "设备房间已保存：" + id + " -> " + room);
  SendCorsHeader();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "OK");
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleDeviceConfigResetApi() {
  ResetDeviceConfig();
  SendCorsHeader();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "OK");
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleNodeDeviceStatusApi() {
  xSemaphoreTake(StateMutex, portMAX_DELAY);
  String json = NodeDeviceBuildJson();
  xSemaphoreGive(StateMutex);
  SendCorsHeader();
  server.send(200, "application/json", json);
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleNodeDevicePage() {
  xSemaphoreTake(StateMutex, portMAX_DELAY);
  String html = NodeDevicePageHtml();
  xSemaphoreGive(StateMutex);
  SendCorsHeader();
  server.send(200, "text/html; charset=utf-8", html);
}


// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String JsonGetString(const String &json, const char *key) {
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

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
float JsonGetFloat(const String &json, const char *key, float defVal = 0) {
  String v = JsonGetString(json, key);
  if (v.length() == 0) return defVal;
  return v.toFloat();
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
int JsonGetInt(const String &json, const char *key, int defVal = 0) {
  String v = JsonGetString(json, key);
  if (v.length() == 0) return defVal;
  return v.toInt();
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitWeatherState() {
  Weather.online = false;
  Weather.airOnline = false;
  Weather.lastNowMs = 0;
  Weather.lastAirMs = 0;
  Weather.city = "包头";
  Weather.weatherText = "待接入";
  Weather.outdoorTemp = 0;
  Weather.feelsLike = 0;
  Weather.outdoorHumidity = 0;
  Weather.windDir = "--";
  Weather.windScale = "--";
  Weather.precip = 0;
  Weather.forecastOnline = false;
  Weather.todayTempRange = "待更新";
  Weather.tomorrowForecast = "待更新";
  Weather.airAqi = 0;
  Weather.airCategory = "待接入";
  Weather.primaryPollutant = "--";
  Weather.umbrellaTip = "天气数据待更新";
  Weather.maskTip = "空气质量待更新";
  Weather.windowTip = "开窗建议待更新";
  Weather.clothesTip = "穿衣建议待更新";
  Weather.aiTip = "天气服务待更新";
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String QWeatherBuildAiTip() {
  String rainText = Weather.weatherText;
  bool mayRain = rainText.indexOf("雨") >= 0 || rainText.indexOf("雪") >= 0 || Weather.precip > 0.1;
  int wind = Weather.windScale.toInt();

  Weather.umbrellaTip = mayRain ? "建议带伞" : "暂无降水，通常不用带伞";
  Weather.maskTip = (Weather.airAqi > 100) ? "空气质量偏差，建议戴口罩" : "空气质量接口待扩展";
  Weather.windowTip = (!mayRain && wind <= 3) ? "适合短时开窗通风" : "不建议长时间开窗";
  Weather.clothesTip = (Weather.feelsLike <= 5) ? "体感较冷，建议加衣" : (Weather.feelsLike >= 30 ? "体感偏热，注意降温补水" : "体感舒适");

  String tip = Weather.umbrellaTip + "；" + Weather.maskTip + "；" + Weather.windowTip + "；" + Weather.clothesTip;
  return tip;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AMapHttpGet(String url, int &httpCode) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  httpCode = -1;
  if (!http.begin(client, url)) return "";
  http.setTimeout(10000);
  http.useHTTP10(true);
  http.addHeader("Accept", "application/json");
  http.addHeader("Accept-Encoding", "identity");
  http.addHeader("User-Agent", String("ESP32-Lingxi/") + LINGXI_VERSION_CODE);
  httpCode = http.GET();
  String body = http.getString();
  http.end();
  return body;
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool QWeatherFetchNow() {
#if ENABLE_QWEATHER
  if (IsPlaceholderText(CloudAmapWeatherKey())) return false;
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = "https://" + String(AMAP_WEATHER_HOST) + "/v3/weather/weatherInfo?city=" + CloudAmapCityCode() + "&key=" + CloudAmapWeatherKey() + "&extensions=base&output=JSON";
  int code = 0;
  String body = AMapHttpGet(url, code);

  String status = JsonGetString(body, "status");
  String info = JsonGetString(body, "info");
  String infocode = JsonGetString(body, "infocode");
  String weather = JsonGetString(body, "weather");
  String temp = JsonGetString(body, "temperature");
  String hum = JsonGetString(body, "humidity");
  String windDir = JsonGetString(body, "winddirection");
  String windPower = JsonGetString(body, "windpower");
  String city = JsonGetString(body, "city");

  if (code != 200 || status != "1" || temp.length() == 0 || weather.length() == 0) {
    Weather.online = false;
    String detail = "高德天气请求失败 HTTP " + String(code);
    if (status.length()) detail += " status " + status;
    if (infocode.length()) detail += " infocode " + infocode;
    if (info.length()) detail += " " + info;
    AddCenterLog("sys", detail);
    Serial.println("[AMapWeather] URL=" + url);
    Serial.println("[AMapWeather] BODY=" + body.substring(0, min(420, (int)body.length())));
    BemfaPublishStatus();
    return false;
  }

  Weather.online = true;
  Weather.lastNowMs = millis();
  Weather.city = city.length() ? city : "包头";
  Weather.weatherText = weather;
  Weather.outdoorTemp = temp.toFloat();
  Weather.feelsLike = Weather.outdoorTemp;  
  Weather.outdoorHumidity = hum.toInt();
  Weather.windDir = windDir.length() ? windDir : "--";
  Weather.windScale = windPower.length() ? windPower : "--";
  Weather.precip = 0;                       
  Weather.airOnline = false;
  Weather.airAqi = 0;
  Weather.airCategory = "不显示";            
  Weather.primaryPollutant = "--";
  Weather.aiTip = QWeatherBuildAiTip();
  AddCenterLog("sys", "高德天气已更新：" + Weather.weatherText + " " + String(Weather.outdoorTemp, 1) + "C");
  BemfaPublishStatus();
  return true;
#else
  return false;
#endif
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool QWeatherFetchAir() {
#if ENABLE_QWEATHER
  
  if (IsPlaceholderText(CloudAmapWeatherKey())) return false;
  if (WiFi.status() != WL_CONNECTED) return false;

  String url = "https://" + String(AMAP_WEATHER_HOST) + "/v3/weather/weatherInfo?city=" + CloudAmapCityCode() + "&key=" + CloudAmapWeatherKey() + "&extensions=all&output=JSON";
  int code = 0;
  String body = AMapHttpGet(url, code);

  String status = JsonGetString(body, "status");
  String info = JsonGetString(body, "info");

  if (code != 200 || status != "1") {
    Weather.forecastOnline = false;
    AddCenterLog("sys", "高德天气预报请求失败 HTTP " + String(code) + " " + info);
    Serial.println("[AMapForecast] URL=" + url);
    Serial.println("[AMapForecast] BODY=" + body.substring(0, min(420, (int)body.length())));
    BemfaPublishStatus();
    return false;
  }

  int castsPos = body.indexOf("\"casts\"");
  int todayStart = body.indexOf("{", castsPos);
  int todayEnd = body.indexOf("}", todayStart);
  int tomorrowStart = body.indexOf("{", todayEnd + 1);
  int tomorrowEnd = body.indexOf("}", tomorrowStart);

  if (castsPos < 0 || todayStart < 0 || todayEnd < 0) {
    Weather.forecastOnline = false;
    AddCenterLog("sys", "高德天气预报解析失败");
    Serial.println("[AMapForecast] BODY=" + body.substring(0, min(420, (int)body.length())));
    BemfaPublishStatus();
    return false;
  }

  String todayObj = body.substring(todayStart, todayEnd + 1);
  String todayDayTemp = JsonGetString(todayObj, "daytemp");
  String todayNightTemp = JsonGetString(todayObj, "nighttemp");
  String todayDayWeather = JsonGetString(todayObj, "dayweather");
  String todayNightWeather = JsonGetString(todayObj, "nightweather");

  if (todayNightTemp.length() && todayDayTemp.length()) {
    Weather.todayTempRange = todayNightTemp + "°C / " + todayDayTemp + "°C";
  } else {
    Weather.todayTempRange = "待更新";
  }

  if (tomorrowStart >= 0 && tomorrowEnd >= 0) {
    String tomorrowObj = body.substring(tomorrowStart, tomorrowEnd + 1);
    String tomorrowDayTemp = JsonGetString(tomorrowObj, "daytemp");
    String tomorrowNightTemp = JsonGetString(tomorrowObj, "nighttemp");
    String tomorrowDayWeather = JsonGetString(tomorrowObj, "dayweather");
    String tomorrowNightWeather = JsonGetString(tomorrowObj, "nightweather");

    if (tomorrowDayWeather.length() == 0) tomorrowDayWeather = tomorrowNightWeather;
    if (tomorrowNightTemp.length() && tomorrowDayTemp.length()) {
      Weather.tomorrowForecast = tomorrowDayWeather + " " + tomorrowNightTemp + "°C / " + tomorrowDayTemp + "°C";
    } else {
      Weather.tomorrowForecast = tomorrowDayWeather.length() ? tomorrowDayWeather : "待更新";
    }
  } else {
    Weather.tomorrowForecast = "待更新";
  }

  Weather.forecastOnline = true;
  Weather.lastAirMs = millis();
  Weather.aiTip = QWeatherBuildAiTip();

  AddCenterLog("sys", "高德预报已更新：今日 " + Weather.todayTempRange + "，明日 " + Weather.tomorrowForecast);
  BemfaPublishStatus();
  return true;
#else
  return false;
#endif
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void QWeatherLoop() {
#if ENABLE_QWEATHER
  if (WiFi.status() != WL_CONNECTED || IsPlaceholderText(CloudAmapWeatherKey())) return;
  unsigned long now = millis();
  if (Weather.lastNowMs == 0 || now - Weather.lastNowMs > AMAP_WEATHER_INTERVAL_MS) {
    QWeatherFetchNow();
  }
  if (Weather.lastAirMs == 0 || now - Weather.lastAirMs > AMAP_FORECAST_INTERVAL_MS) {
    QWeatherFetchAir();
  }
#endif
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskQWeather(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(6000));
  while (1) {
    QWeatherLoop();
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

