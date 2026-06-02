/*
 * 文件：10_rules_logs_control_core.ino
 * 标题：规则、日志与统一控制核心
 * 说明：负责系统日志、场景命令、统一控制入口和状态回写。
 * 可改：可新增场景文案
 * 禁止：让继电器/电机在循环里高速反复动作。
 */

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void DeviceStatePatchInit() {
  portENTER_CRITICAL(&DataMux);
  memset(&uiActionBoardState, 0, sizeof(uiActionBoardState));
  uiActionBoardState.statusSeq = 1;
  uiActionBoardState.lastCmdSeq = 0;
  uiActionBoardState.executorOnline = 0;
  DeviceStatePatchVersion = 1;
  LastDeviceStatePatchMs = millis();
  portEXIT_CRITICAL(&DataMux);
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void ApplyDeviceStatePatchFromCommand(ControlCommand c) {
  unsigned long now = millis();

  portENTER_CRITICAL(&DataMux);

  uiActionBoardState.statusSeq++;
  uiActionBoardState.lastCmdSeq = c.cmdSeq;
  uiActionBoardState.uptimeMs = now;
  uiActionBoardState.executorOnline = (lastActionBoardTime > 0 && now - lastActionBoardTime <= 3000) ? 1 : 0;

  
  
  uiActionBoardState.livingLightState = c.livingLightCmd ? 1 : 0;
  uiActionBoardState.bedroomLightState = c.bedroomLightCmd ? 1 : 0;
  State.lightState = (uiActionBoardState.livingLightState || uiActionBoardState.bedroomLightState) ? 1 : 0;

  uiActionBoardState.airRelayState = c.airRelayCmd ? 1 : 0;
  uiActionBoardState.tvRelayState = c.tvRelayCmd ? 1 : 0;
  uiActionBoardState.fridgeRelayState = c.fridgeRelayCmd ? 1 : 0;
  uiActionBoardState.waterRelayState = c.waterRelayCmd ? 1 : 0;

  
  
  if (c.livingCurtainCmd == 1) {
    uiActionBoardState.livingCurtainState = 1;
    uiActionBoardState.livingCurtainPercent = 100;
  } else if (c.livingCurtainCmd == 2) {
    uiActionBoardState.livingCurtainState = 2;
    uiActionBoardState.livingCurtainPercent = 0;
  }

  if (c.bedroomCurtainCmd == 1) {
    uiActionBoardState.bedroomCurtainState = 1;
    uiActionBoardState.bedroomCurtainPercent = 100;
  } else if (c.bedroomCurtainCmd == 2) {
    uiActionBoardState.bedroomCurtainState = 2;
    uiActionBoardState.bedroomCurtainPercent = 0;
  }

  if (c.windowServoCmd == 1) {
    uiActionBoardState.windowState = 1;
    uiActionBoardState.windowPercent = 100;
  } else if (c.windowServoCmd == 2) {
    uiActionBoardState.windowState = 2;
    uiActionBoardState.windowPercent = 0;
  }

  uiActionBoardState.irAirPower = c.irAirCmd ? 1 : uiActionBoardState.irAirPower;
  uiActionBoardState.alarmState = c.sceneCmd == SCENE_ALARM ? 1 : uiActionBoardState.alarmState;

  DeviceStatePatchVersion++;
  LastDeviceStatePatchMs = now;

  portEXIT_CRITICAL(&DataMux);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void mergeActionFeedbackToUiShadow(ActionBoardState raw) {
  unsigned long now = millis();
  uint32_t currentCmdSeq = CurrentCmd.cmdSeq;

  portENTER_CRITICAL(&DataMux);

  
  
  
  
  bool freshAck = (raw.lastCmdSeq >= currentCmdSeq);

  uiActionBoardState.statusSeq = raw.statusSeq;
  uiActionBoardState.lastCmdSeq = raw.lastCmdSeq;
  uiActionBoardState.uptimeMs = raw.uptimeMs;
  uiActionBoardState.executorOnline = raw.executorOnline;
  uiActionBoardState.lastCommandSource = raw.lastCommandSource;

  if (freshAck) {
    
    
    
    
    uiActionBoardState.airRelayState = raw.airRelayState;
    uiActionBoardState.tvRelayState = raw.tvRelayState;
    uiActionBoardState.fridgeRelayState = raw.fridgeRelayState;
    uiActionBoardState.waterRelayState = raw.waterRelayState;
    uiActionBoardState.irAirPower = raw.irAirPower;
    uiActionBoardState.irAirMode = raw.irAirMode;
    uiActionBoardState.irAirTemp = raw.irAirTemp;
    uiActionBoardState.irAirFan = raw.irAirFan;
    uiActionBoardState.alarmState = raw.alarmState;
  }

  
  
  DeviceStatePatchVersion++;

  portEXIT_CRITICAL(&DataMux);
}

// 函数说明：读取函数，负责获取配置、状态或资源。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
ActionBoardState getPatchedActionStateSnapshot() {
  ActionBoardState z;
  portENTER_CRITICAL(&DataMux);
  z = uiActionBoardState;
  portEXIT_CRITICAL(&DataMux);
  return z;
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DeviceStatePatchIsOn(String key) {
  ActionBoardState z = getPatchedActionStateSnapshot();
  if (key == "light_living") return z.livingLightState == 1;
  if (key == "light_bed") return z.bedroomLightState == 1;
  if (key == "air") return z.airRelayState == 1;
  if (key == "tv") return z.tvRelayState == 1;
  if (key == "fridge") return z.fridgeRelayState == 1;
  if (key == "water") return z.waterRelayState == 1;
  if (key == "curtain_living") return z.livingCurtainPercent > 50;
  if (key == "curtain_bed") return z.bedroomCurtainPercent > 50;
  if (key == "window") return z.windowPercent > 50;
  return false;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ReplyMeansOffOrClose(String reply) {
  return reply.indexOf("关闭") >= 0 || reply.indexOf("关停") >= 0 || reply.indexOf("已关") >= 0 || reply.indexOf("闭合") >= 0;
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ApplySceneCommand(String cmd, ControlCommand &c, String &reply) {
  if (cmd == "scene_none") {
    State.SceneState = SCENE_NONE;
    State.autoMode = false;
    State.awayMode = false;
    c.sceneCmd = SCENE_NONE;
    reply = "当前场景已关闭";
    CommitWebCommand(c);
    return true;
  }

  
  State.autoMode = false;
  State.awayMode = false;

  if (cmd == "scene_home") {
    State.SceneState = SCENE_HOME;
    State.awayMode = false;
    c.sceneCmd = SCENE_HOME;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.lightCmd = 1;
    c.lightLevel = 180;
    reply = "回家模式已执行";
  }

  else if (cmd == "scene_away") {
    State.SceneState = SCENE_AWAY;
    State.awayMode = true;
    State.autoMode = true;
    c.sceneCmd = SCENE_AWAY;
    c.livingLightCmd = 0;
    c.bedroomLightCmd = 0;
    c.tvRelayCmd = 0;
    c.waterRelayCmd = 0;
    c.livingCurtainCmd = 2;
    c.bedroomCurtainCmd = 2;
    c.windowServoCmd = 2;
    c.lightCmd = 0;
    c.lightLevel = 0;
    reply = "离家模式已执行";
  }

  else if (cmd == "scene_sleep") {
    State.SceneState = SCENE_SLEEP;
    c.sceneCmd = SCENE_SLEEP;
    c.livingLightCmd = 0;
    c.bedroomLightCmd = 1;
    c.tvRelayCmd = 0;
    c.livingCurtainCmd = 2;
    c.bedroomCurtainCmd = 2;
    c.windowServoCmd = 2;
    c.lightCmd = 1;
    c.lightLevel = 60;
    reply = "睡眠模式已执行";
  }

  else if (cmd == "scene_comfort") {
    State.SceneState = SCENE_COMFORT;
    c.sceneCmd = SCENE_COMFORT;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.airRelayCmd = 1;
    c.livingCurtainCmd = 1;
    c.bedroomCurtainCmd = 1;
    c.lightCmd = 1;
    c.lightLevel = 160;
    reply = "舒适模式已执行";
  }

  else if (cmd == "scene_quiet") {
    State.SceneState = SCENE_QUIET;
    c.sceneCmd = SCENE_QUIET;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.tvRelayCmd = 0;
    c.lightCmd = 1;
    c.lightLevel = 80;
    reply = "静谧模式已执行";
  }

  else if (cmd == "scene_office") {
    State.SceneState = SCENE_OFFICE;
    c.sceneCmd = SCENE_OFFICE;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.tvRelayCmd = 0;
    c.airRelayCmd = 1;
    c.livingCurtainCmd = 1;
    c.bedroomCurtainCmd = 1;
    c.lightCmd = 1;
    c.lightLevel = 220;
    reply = "办公模式已执行";
  }

  else if (cmd == "scene_reading") {
    State.SceneState = SCENE_READING;
    c.sceneCmd = SCENE_READING;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.tvRelayCmd = 0;
    c.lightCmd = 1;
    c.lightLevel = 200;
    reply = "阅读模式已执行";
  }

  else if (cmd == "scene_movie") {
    State.SceneState = SCENE_MOVIE;
    c.sceneCmd = SCENE_MOVIE;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 0;
    c.tvRelayCmd = 1;
    c.airRelayCmd = 1;
    c.livingCurtainCmd = 2;
    c.bedroomCurtainCmd = 2;
    c.windowServoCmd = 2;
    c.lightCmd = 1;
    c.lightLevel = 70;
    reply = "观影模式已执行";
  }

  else if (cmd == "scene_gaming") {
    State.SceneState = SCENE_GAMING;
    c.sceneCmd = SCENE_GAMING;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.tvRelayCmd = 1;
    c.airRelayCmd = 1;
    c.lightCmd = 1;
    c.lightLevel = 150;
    reply = "电竞模式已执行";
  }

  else if (cmd == "scene_romantic") {
    State.SceneState = SCENE_ROMANTIC;
    c.sceneCmd = SCENE_ROMANTIC;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.livingCurtainCmd = 2;
    c.bedroomCurtainCmd = 2;
    c.lightCmd = 1;
    c.lightLevel = 100;
    reply = "浪漫模式已执行";
  }

  else if (cmd == "scene_emo") {
    State.SceneState = SCENE_EMO;
    c.sceneCmd = SCENE_EMO;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.tvRelayCmd = 0;
    c.livingCurtainCmd = 2;
    c.bedroomCurtainCmd = 2;
    c.lightCmd = 1;
    c.lightLevel = 60;
    reply = "EMO 模式已执行";
  }

  else if (cmd == "scene_night") {
    State.SceneState = SCENE_NIGHT;
    c.sceneCmd = SCENE_NIGHT;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.lightCmd = 1;
    c.lightLevel = 40;
    reply = "起夜模式已执行";
  }

  else if (cmd == "scene_morning") {
    State.SceneState = SCENE_MORNING;
    c.sceneCmd = SCENE_MORNING;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.livingCurtainCmd = 1;
    c.bedroomCurtainCmd = 1;
    c.lightCmd = 1;
    c.lightLevel = 150;
    reply = "晨起模式已执行";
  }

  else if (cmd == "scene_noon") {
    State.SceneState = SCENE_NOON;
    c.sceneCmd = SCENE_NOON;
    c.livingLightCmd = 0;
    c.bedroomLightCmd = 1;
    c.livingCurtainCmd = 2;
    c.bedroomCurtainCmd = 2;
    c.lightCmd = 1;
    c.lightLevel = 50;
    reply = "午休模式已执行";
  }

  else if (cmd == "scene_party") {
    State.SceneState = SCENE_PARTY;
    c.sceneCmd = SCENE_PARTY;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.tvRelayCmd = 1;
    c.lightCmd = 1;
    c.lightLevel = 180;
    reply = "派对模式已执行";
  }

  else if (cmd == "scene_alarm") {
    State.SceneState = SCENE_ALARM;
    State.alarmMode = true;
    c.sceneCmd = SCENE_ALARM;
    c.livingLightCmd = 1;
    c.bedroomLightCmd = 1;
    c.lightCmd = 2;
    c.lightLevel = 255;
    c.buzzerCmd = 0;
    reply = "报警模式已执行";
  }

  else if (cmd == "scene_custom") {
    State.SceneState = SCENE_CUSTOM;
    c.sceneCmd = SCENE_CUSTOM;
    reply = "自定义场景接口已预留";
  }

  else {
    return false;
  }

  CommitWebCommand(c);
  return true;
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleCmdApi() {
  if (!server.hasArg("cmd")) {
    SendCorsHeader();
    server.send(400, "text/plain", "missing cmd");
    return;
  }

  String cmd = server.arg("cmd");
  cmd.trim();

  const char* webuiSource = (server.hasArg("no_voice") && server.arg("no_voice") == "1") ? "WebUI_NoVoice" : "WebUI";
  String reply = ExecuteUnifiedCommand(cmd, webuiSource);
  AddCenterLog("op", String(webuiSource) + " " + cmd + " => " + reply);
  BemfaPublishStatus();

  SendCorsHeader();
  server.send(200, "text/plain", reply);
}


// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool IsSceneCommand(String cmd) {
  return cmd == "scene_none" || cmd == "scene_home" || cmd == "scene_away" ||
         cmd == "scene_sleep" || cmd == "scene_comfort" || cmd == "scene_quiet" ||
         cmd == "scene_office" || cmd == "scene_reading" || cmd == "scene_movie" ||
         cmd == "scene_gaming" || cmd == "scene_romantic" || cmd == "scene_emo" ||
         cmd == "scene_night" || cmd == "scene_morning" || cmd == "scene_noon" ||
         cmd == "scene_party" || cmd == "scene_alarm" || cmd == "scene_custom";
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool IsManualDeviceCommand(String cmd) {
  if (cmd.startsWith("light_")) return true;
  if (cmd.startsWith("relay_")) return true;
  if (cmd.startsWith("curtain_")) return true;
  if (cmd == "window_toggle" || cmd == "window_open" || cmd == "window_close") return true;
  if (cmd == "auto_on" || cmd == "auto_off" || cmd == "away_on" || cmd == "away_off") return true;
  return false;
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CancelSceneForManualCommand(String cmd, const char* source, ControlCommand &c) {
  if (!IsManualDeviceCommand(cmd)) return;

  if (State.SceneState != SCENE_NONE && State.SceneState != SCENE_ALARM) {
    AddCenterLog("op", String(source ? source : "手动") + " 设备控制已打断当前场景，切换为手动控制");
  }

  State.SceneState = SCENE_NONE;
  State.autoMode = false;
  State.awayMode = false;
  c.sceneCmd = SCENE_NONE;
}


