/*
 * 文件：09_cloud_weather_mqtt_core.ino
 * 标题：云端、天气与 MQTT/Bemfa
 * 说明：负责天气接口、云平台状态、MQTT/Bemfa 指令和外部服务。
 * 可改：可改城市和显示文案
 * 禁止：公开真实 token、secret、key。
 */

// 函数说明：火山方舟 Chat API 请求函数，负责发送自由对话并提取回复正文。
// 可改范围：仅可调整 max_tokens、temperature；不要改回 Responses API。
bool DoubaoChatCompletionsPost(String userText, String model, String logTag, uint32_t startMs, String &outText) {
  outText = "";
  String cleanText = DoubaoBuildChatPrompt(userText);
  cleanText.trim();
  if (cleanText.length() == 0) {
    LastDoubaoAiError = "Chat API 输入为空";
    LastDoubaoAiState = "error";
    return false;
  }

  String body = "{";
  body += "\"model\":\"" + JsonEscape(model) + "\",";
  body += "\"messages\":[{";
  body += "\"role\":\"user\",";
  body += "\"content\":\"" + JsonEscape(cleanText) + "\"";
  body += "}],";
  body += "\"max_tokens\":" + String(DOUBAO_MAX_OUTPUT_TOKENS) + ",";
  body += "\"temperature\":0.7,";
  body += "\"stream\":false";
  body += "}";

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, DOUBAO_RESPONSES_URL)) {
    LastDoubaoAiError = "HTTP begin Doubao Chat API 失败";
    LastDoubaoAiState = "error";
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }
  http.setTimeout(DOUBAO_LLM_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + CloudArkApiKey());

  UF_Log("REQ", "DOUBAO", logTag + " Chat API model=" + model + " text_len=" + String(cleanText.length()));
  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  LastDoubaoAiElapsedMs = millis() - startMs;
  UF_Log((code >= 200 && code < 300) ? "OK" : "FAIL", "DOUBAO", logTag + " chat_http=" + String(code) + " elapsed=" + String(LastDoubaoAiElapsedMs) + "ms");

  if (code < 200 || code >= 300) {
    LastDoubaoAiError = "Doubao Chat API HTTP " + String(code) + " " + resp.substring(0, 180);
    LastDoubaoAiState = "error";
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }

  outText = DoubaoExtractOutputText(resp);
  outText.trim();
  if (outText.length() == 0) {
    LastDoubaoAiError = "Doubao Chat API 返回成功，但未解析到 choices.message.content";
    LastDoubaoAiState = "parse_empty";
    AddCenterLog("sys", LastDoubaoAiError + " body=" + resp.substring(0, 220));
    return false;
  }
  return true;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4ChatModeIsStartCommand(String text) {
  (void)text;
  return false;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4ChatModeIsStopCommand(String text) {
  (void)text;
  return false;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V4SpeakSystemReply(String reply) {
  reply.trim();
  if (!reply.length()) return;

  String speaker = CurrentRoleTtsSpeaker();
  DoubaoTtsSynthesizeToSd(reply, speaker, String(DOUBAO_TTS_LAST_PATH));

  if (Voice.sdOnline && SD_MMC.exists(DOUBAO_TTS_LAST_PATH)) {
    BoardAudioQueueTtsPlayback(String(DOUBAO_TTS_LAST_PATH), "V4SpeakSystemReply");
  }
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V4ChatModeStart(String reason) {
  (void)reason;
  
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V4ChatModeStop(String reason) {
  (void)reason;
  V4ChatModeActive = false;
  V4ChatModeRounds = 0;
  V4ChatModeStartedMs = 0;
  V4ChatModeLastActivityMs = 0;
  V49ListenSessionActive = false;
  V49ListenSessionRounds = 0;
  V49ListenSessionStartedMs = 0;
  V49ListenSessionLastActivityMs = 0;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4ChatModeActiveNow() {
  return false;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4ChatModeTryHandle(String text, String source) {
  (void)text;
  (void)source;
  
  return false;
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V49ListenSource(String source) {
  (void)source;
  
  
  return false;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V49ListenSessionStart(String reason) {
#if ENABLE_LINGXI_V49_LISTEN_SESSION
  V49ListenSessionActive = true;
  V49ListenSessionRounds = 0;
  V49ListenSessionStartedMs = millis();
  V49ListenSessionLastActivityMs = millis();
  Serial.println("[V4 FinalSeal LISTEN] session opened: " + reason);
  AddCenterLog("voice", "中控监听会话已开启：" + reason);
#else
  (void)reason;
#endif
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V49ListenSessionStop(String reason) {
#if ENABLE_LINGXI_V49_LISTEN_SESSION
  if (V49ListenSessionActive) {
    Serial.println("[V4 FinalSeal LISTEN] session closed: " + reason);
    AddCenterLog("voice", "中控监听会话已关闭：" + reason);
  }
  V49ListenSessionActive = false;
  V49ListenSessionRounds = 0;
  V49ListenSessionStartedMs = 0;
  V49ListenSessionLastActivityMs = 0;
  if (V4ChatModeActive) {
    V4ChatModeActive = false;
    V4ChatModeRounds = 0;
    V4ChatModeStartedMs = 0;
    V4ChatModeLastActivityMs = 0;
    Serial.println("[V4 CHATMODE] auto closed with listen session: " + reason);
  }
#else
  (void)reason;
#endif
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V49ListenSessionMaybeContinue(String source, bool routeOk) {
#if ENABLE_LINGXI_V49_LISTEN_SESSION
  if (!V49ListenSessionActive) return false;
  if (!V49ListenSource(source)) return false;

  uint32_t now = millis();
  if (!routeOk) {
    V49ListenSessionStop("route/asr failed");
    return false;
  }
  if (now - V49ListenSessionStartedMs > LINGXI_V49_LISTEN_SESSION_TIMEOUT_MS) {
    V49ListenSessionStop("timeout");
    return false;
  }
  if (V4ChatModeRounds >= LINGXI_CHAT_MODE_MAX_ROUNDS) {
    V49ListenSessionStop("chat mode max rounds reached");
    return false;
  }

  bool pending = false;
  if (V4ReqMutex) {
    xSemaphoreTake(V4ReqMutex, portMAX_DELAY);
    pending = V4Req.pending;
    xSemaphoreGive(V4ReqMutex);
  }
  if (pending || V4Voice.busy || VoiceIO.recording || VoiceRecordAsyncActive) {
    Serial.println("[V4 FinalSeal LISTEN] next record skipped: busy/pending");
    return false;
  }

  V49ListenSessionRounds++;
  V4ChatModeRounds++;
  V49ListenSessionLastActivityMs = now;
  V4ChatModeLastActivityMs = now;
  delay(LINGXI_V49_LISTEN_NEXT_DELAY_MS);
  Serial.printf("[V4 CHATMODE] auto next stream round=%u/%u seconds=%d\n",
                V4ChatModeRounds, LINGXI_CHAT_MODE_MAX_ROUNDS, LINGXI_CHAT_MODE_RECORD_SECONDS);
  AddCenterLog("voice", "单轮语音继续：自动准备下一句");
  
  return true;
#else
  (void)source;
  (void)routeOk;
  return false;
#endif
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V49TextIsConfirm(String t) {
  t = VoiceTextNormalize(t);
  return VoiceTextHasAny(t, "确认", "确定", "可以", "好的", "好", "执行", "是的", "对") ||
         VoiceTextHasAny(t, "帮我", "开吧", "关吧", "就这样", "同意");
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V49TextIsCancel(String t) {
  t = VoiceTextNormalize(t);
  return VoiceTextHasAny(t, "取消", "不用", "不要", "算了", "别", "不需要", "先不用", "不用了");
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V49ConfirmClear(String reason) {
  if (V49ConfirmPending) {
    Serial.println("[V4 FinalSeal RULE] confirm cleared: " + reason);
  }
  V49ConfirmPending = false;
  V49ConfirmIntent = "";
  V49ConfirmCommand = "";
  V49ConfirmReply = "";
  V49ConfirmCreatedMs = 0;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V49ConfirmSet(String intent, String command, String reply) {
  V49ConfirmPending = true;
  V49ConfirmIntent = intent;
  V49ConfirmCommand = command;
  V49ConfirmReply = reply;
  V49ConfirmCreatedMs = millis();
  Serial.println("[V4 FinalSeal RULE] pending confirm command=" + command + " intent=" + intent);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V49ConfirmTryResolve(String text, LingxiVoiceRouteResult &r) {
  if (!V49ConfirmPending) return false;
  if (millis() - V49ConfirmCreatedMs > LINGXI_V49_CONFIRM_TIMEOUT_MS) {
    V49ConfirmClear("timeout");
    return false;
  }

  String t = VoiceTextNormalize(text);
  if (V49TextIsCancel(t)) {
    r.layer = ROUTE_LAYER_CONFIRM_AGENT;
    r.text = text;
    r.intent = V49ConfirmIntent;
    r.command = "";
    r.reply = "好的，已取消。";
    r.needConfirm = false;
    r.executed = false;
    V49ConfirmClear("user canceled");
    return true;
  }

  if (V49TextIsConfirm(t)) {
    r.layer = ROUTE_LAYER_CONFIRM_AGENT;
    r.text = text;
    r.intent = V49ConfirmIntent;
    r.command = V49ConfirmCommand;
    r.reply = "好的，马上执行。";
    r.needConfirm = false;
    r.executed = false;
    V49ConfirmClear("user confirmed");
    return true;
  }

  return false;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ConfirmAgentTryRoute(String text, LingxiVoiceRouteResult &r) {
  text.trim();
  r.layer = ROUTE_LAYER_NONE;
  r.text = text;
  r.intent = "";
  r.command = "";
  r.reply = "";
  r.needConfirm = false;
  r.executed = false;

#if ENABLE_LINGXI_CONFIRM_AGENT
  if (V49ConfirmTryResolve(text, r)) return true;

  String t = VoiceTextNormalize(text);
  if (t.length() == 0) return false;
  if (VoiceTextShouldStayChat(t)) return false;

  String intent = "";
  String cmd = "";
  String reply = "";

  
  if (VoiceTextHasAny(t, "我回来了", "回家")) {
    intent = "scene_suggest";
    cmd = "scene_home";
    reply = "我可以帮你切换到回家模式，要执行吗？";
  } else if (VoiceTextHasAny(t, "有点暗", "太暗", "屋里暗", "光线暗", "看不清", "黑乎乎")) {
    intent = "ambient_light";
    cmd = "light_living_on";
    reply = "我可以帮你打开客厅灯，要执行吗？";
  } else if (VoiceTextHasAny(t, "有点亮", "太亮", "刺眼")) {
    intent = "ambient_light";
    cmd = "light_living_off";
    reply = "我可以帮你关闭客厅灯，要执行吗？";
  } else if (VoiceTextHasAny(t, "有点热", "太热", "闷热", "好热")) {
    intent = "comfort_temperature";
    cmd = "relay_air_on";
    reply = "我可以帮你打开空调，要执行吗？";
  } else if (VoiceTextHasAny(t, "我要睡", "准备睡", "想睡觉", "困了", "晚安")) {
    intent = "scene_suggest";
    cmd = "scene_sleep";
    reply = "我可以帮你切换到睡眠模式，要执行吗？";
  } else if ((VoiceTextHasAny(t, "我要出门", "准备出门", "家里没人", "没人了", "出门", "我要走", "我走了", "上班去了") || VoiceTextHasAny(t, "去上班"))) {
    intent = "scene_suggest";
    cmd = "scene_away";
    reply = "我可以帮你切换到离家模式，要执行吗？";
  } else if (VoiceTextHasAny(t, "看电影", "准备观影", "想看电影")) {
    intent = "scene_suggest";
    cmd = "scene_movie";
    reply = "我可以帮你切换到观影模式，要执行吗？";
  } else if (VoiceTextHasAny(t, "空气不好", "有味道", "有烟味", "闷得慌")) {
    intent = "air_quality";
    cmd = "window_open";
    reply = "我可以帮你打开窗户通风，要执行吗？";
  }

  if (cmd.length()) {
    if (!AiCommandAllowed(cmd)) {
      Serial.println("[V4 FinalSeal RULE] blocked non-whitelist command=" + cmd);
      return false;
    }
    V49ConfirmSet(intent, cmd, reply);
    r.layer = ROUTE_LAYER_CONFIRM_AGENT;
    r.text = text;
    r.intent = intent;
    r.command = cmd;
    r.reply = reply;
    r.needConfirm = true;
    r.executed = false;
    return true;
  }

  return false;
#else
  (void)text;
  return false;
#endif
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DoubaoFreeChatRunText(String userText, String modelMode, bool useTts) {
#if ENABLE_DOUBAO_AI_CENTER
  userText = LingxiCleanUserRawText(userText);
  userText.trim();
  LastDoubaoAiReply = "";
  LastDoubaoAiCommands = "";
  LastDoubaoAiError = "";
  LastCloudCommand = "";
  LastCloudReply = "";

  if (userText.length() == 0) {
    LastDoubaoAiError = "自由对话输入为空";
    LastDoubaoAiState = "error";
    return false;
  }
  if (userText.length() > 300) userText = userText.substring(0, 300);

  if (millis() - LastDoubaoAiMs < DOUBAO_AI_COOLDOWN_MS) {
    LastDoubaoAiError = "自由对话请求过快，已防抖";
    LastDoubaoAiState = "cooldown";
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }

  LastDoubaoAiMs = millis();
  uint32_t aiStartMs = LastDoubaoAiMs;
  LastDoubaoAiElapsedMs = 0;
  LastDoubaoAiState = "free_chat_requesting";
  LastDoubaoAiError = "";
  LastDoubaoAiText = userText;
  LastDoubaoAiModel = modelMode.length() ? modelMode : CloudModelMode();

  if (WiFi.status() != WL_CONNECTED) {
    LastDoubaoAiError = "STA WiFi 未连接，无法调用豆包自由对话";
    LastDoubaoAiState = "error";
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }
  if (IsPlaceholderText(CloudArkApiKey())) {
    LastDoubaoAiError = "大模型接口密钥未填写";
    LastDoubaoAiState = "error";
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }

  String model = DoubaoModelByMode(LastDoubaoAiModel);
  String outText = "";
  if (!DoubaoChatCompletionsPost(userText, model, "FREE_CHAT", aiStartMs, outText)) return false;

  String reply = DoubaoCleanReply(outText, userText);
  reply.trim();
  if (reply.length() == 0) {
    LastDoubaoAiError = "豆包自由对话回复清洗后为空";
    LastDoubaoAiState = "reply_empty";
    AddCenterLog("sys", LastDoubaoAiError + " raw=" + outText.substring(0, 160));
    return false;
  }

  LastDoubaoAiReply = reply;
  LastDoubaoAiCommands = "";
  LastCloudReply = reply;
  LastCloudCommand = "";

  VoiceTaskSetText(userText, "doubao_free_chat_input");
  VoiceTask.intent = "free_chat";
  VoiceTask.command = "";
  VoiceTask.commandReady = false;
  VoiceTask.reply = reply;
  VoiceTask.replyReady = true;
  VoiceTask.brain = "doubao_free_chat_" + LastDoubaoAiModel;
  VoiceTask.status = VOICE_TASK_REPLIED;
  VoiceTask.updatedMs = millis();

  AddCenterLog("op", "豆包自由对话回复：" + reply);
  AddCenterLog("op", "豆包自由对话：仅回复，不控制设备");

  if (useTts && reply.length()) {
    String roleSpeaker = CurrentRoleTtsSpeaker();
    LingxiLogTtsRole("自由对话", roleSpeaker);
    DoubaoTtsSynthesizeToSd(reply, roleSpeaker, String(DOUBAO_TTS_LAST_PATH));
  }

  LastDoubaoAiState = "free_chat_done";
  BemfaPublishStatus();
  return true;
#else
  (void)userText;
  (void)modelMode;
  (void)useTts;
  LastDoubaoAiError = "豆包自由对话未启用";
  return false;
#endif
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DoubaoExtractOutputText(String body) {
  body.trim();
  if (body.length() == 0) return "";

  // Chat Completions 主路径：choices[0].message.content
  int choices = body.indexOf("\"choices\"");
  if (choices >= 0) {
    int msg = body.indexOf("\"message\"", choices);
    int start = (msg >= 0) ? msg : choices;
    String c = JsonGetStringAfter(body, "content", start);
    c.trim();
    if (c.length()) return c;
  }

  // 兼容少量非标准返回：message.content
  int msg = body.indexOf("\"message\"");
  if (msg >= 0) {
    String c = JsonGetStringAfter(body, "content", msg);
    c.trim();
    if (c.length()) return c;
  }

  // 兼容 Responses API 旧字段，避免旧配置返回时完全不可读。
  String direct = JsonGetStringAfter(body, "output_text", 0);
  direct.trim();
  if (direct.length()) return direct;

  int typePos = body.indexOf("\"output_text\"");
  if (typePos >= 0) {
    String t = JsonGetStringAfter(body, "text", typePos);
    t.trim();
    if (t.length()) return t;
  }

  String t = JsonGetStringAfter(body, "text", 0);
  t.trim();
  return t;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DoubaoFallbackChatReply(String userText) {
  (void)userText;
  return "";
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DoubaoCleanReply(String reply, String userText) {
  reply.replace("\r", " ");
  reply.replace("\n", " ");
  reply.replace("*", "");
  reply.replace("#", "");
  reply.replace("```json", "");
  reply.replace("```JSON", "");
  reply.replace("```", "");
  reply.trim();

  
  reply.replace("我是灵汐", "我在");
  reply.replace("我是林汐", "我在");
  reply.replace("我是 Lingxi", "我在");
  reply.replace("灵汐会", "我会");
  reply.replace("林汐会", "我会");
  reply.replace("灵汐可以", "我可以");
  reply.replace("林汐可以", "我可以");
  reply.replace("灵汐", "我");
  reply.replace("林汐", "我");
  reply.replace("Lingxi", "我");
  reply.replace("我是我", "我在");

  
  reply.replace("针对你的询问，", "");
  reply.replace("针对你的询问", "");
  reply.replace("针对你的问题，", "");
  reply.replace("针对你的问题", "");
  reply.replace("根据你的描述，", "");
  reply.replace("根据你的描述", "");
  reply.replace("我将友好回应。", "");
  reply.replace("我将友好回应", "");

  while (reply.indexOf("  ") >= 0) reply.replace("  ", " ");
  reply.trim();

  bool leaked = false;
  String low = reply;
  low.toLowerCase();

  
  if (reply.indexOf("用户") >= 0 || reply.indexOf("该用户") >= 0 || reply.indexOf("对方") >= 0 || reply.indexOf("提问者") >= 0 || reply.indexOf("使用者") >= 0) leaked = true;
  if (reply.indexOf("根据用户") >= 0 || reply.indexOf("用户可能") >= 0 || reply.indexOf("用户想") >= 0 || reply.indexOf("用户询问") >= 0 || reply.indexOf("用户问") >= 0) leaked = true;
  if (reply.indexOf("我将给出") >= 0 || reply.indexOf("自然回应") >= 0 || reply.indexOf("自然的回应") >= 0 || reply.indexOf("我在理解") >= 0) leaked = true;
  if (reply.indexOf("作为智能助手") >= 0 || reply.indexOf("AI助手") >= 0 || reply.indexOf("语言模型") >= 0 || reply.indexOf("我是AI") >= 0 || reply.indexOf("我是 AI") >= 0) leaked = true;
  if (reply.indexOf("规则说明") >= 0 || reply.indexOf("系统说明") >= 0 || reply.indexOf("开发者说明") >= 0 || reply.indexOf("系统规则") >= 0) leaked = true;
  if (reply.indexOf("API") >= 0 || reply.indexOf("密钥") >= 0 || reply.indexOf("token") >= 0 || reply.indexOf("Token") >= 0 || reply.indexOf("内部判断") >= 0 || reply.indexOf("白名单") >= 0) leaked = true;
  if (reply.indexOf("输出格式") >= 0 || reply.indexOf("JSON") >= 0 || low.indexOf("command") >= 0 || low.indexOf("commands") >= 0 || low.indexOf("endpoint_id") >= 0) leaked = true;
  if (reply.indexOf("{") >= 0 || reply.indexOf("}") >= 0 || reply.indexOf(";") >= 0) leaked = true;

  if (reply.length() == 0 || leaked) return DoubaoFallbackChatReply(userText);

  
  const int maxChars = 90;
  if (reply.length() > maxChars) {
    String head = reply.substring(0, maxChars);
    int cut = max(head.lastIndexOf("。"), max(head.lastIndexOf("！"), head.lastIndexOf("？")));
    if (cut < 36) cut = head.lastIndexOf("，");
    if (cut > 36) reply = head.substring(0, cut + 3);
    else reply = head + "……";
  }

  return reply;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String JsonArrayStringItems(const String &json, const char *key, uint8_t maxItems) {
  String pat = "\"" + String(key) + "\"";
  int p = json.indexOf(pat);
  if (p < 0) return "";
  p = json.indexOf('[', p);
  if (p < 0) return "";
  int end = json.indexOf(']', p);
  if (end < 0) return "";
  String csv = "";
  uint8_t count = 0;
  int i = p + 1;
  while (i < end && count < maxItems) {
    int q = json.indexOf('"', i);
    if (q < 0 || q >= end) break;
    String item = JsonStringAt(json, q);
    if (item.length()) {
      if (csv.length()) csv += ",";
      csv += item;
      count++;
    }
    i = q + 1;
    bool esc = false;
    while (i < end) {
      char c = json[i];
      if (esc) { esc = false; i++; continue; }
      if (c == '\\') { esc = true; i++; continue; }
      if (c == '"') { i++; break; }
      i++;
    }
  }
  return csv;
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool AiCommandAllowed(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return false;
  if (IsSceneCommand(cmd)) return true;
  if (IsManualDeviceCommand(cmd)) return true;
  if (cmd == "alarm_reset") return true;
  if (cmd == "set_role_shorekeeper" || cmd == "set_role_cartethyia") return true;
  return false;
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ExecuteAiCommandsCsv(String csv, String source, String &execReply) {
  execReply = "";
  bool okAny = false;
  int start = 0;
  uint8_t count = 0;
  while (start <= (int)csv.length() && count < DOUBAO_MAX_COMMANDS) {
    int comma = csv.indexOf(',', start);
    String cmd = (comma < 0) ? csv.substring(start) : csv.substring(start, comma);
    cmd.trim();
    if (cmd.length()) {
      if (AiCommandAllowed(cmd)) {
        String r = ExecuteUnifiedCommand(cmd, source.c_str());
        if (execReply.length()) execReply += "；";
        execReply += cmd + " => " + r;
        okAny = true;
      } else {
        if (execReply.length()) execReply += "；";
        execReply += "拦截未知命令 " + cmd;
      }
      count++;
    }
    if (comma < 0) break;
    start = comma + 1;
  }
  return okAny;
}


// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String LocalWeatherReplyText() {
  int lightPercent = (int)constrain(State.lightLevelValue / 500.0 * 100.0, 0, 100);
  String reply = "当前室内温度" + String(State.temperature, 1) + "℃，湿度" + String(State.humidity, 1) + "%";
  reply += "，光照" + String(lightPercent) + "%";
  if (Weather.weatherText.length() && Weather.weatherText != "--" && Weather.weatherText != "待接入") {
    reply += "。外部天气：" + Weather.weatherText;
    reply += "，" + String(Weather.outdoorTemp, 1) + "℃";
    reply += "，湿度" + String(Weather.outdoorHumidity) + "%";
    if (Weather.windowTip.length()) reply += "。" + Weather.windowTip;
  } else {
    reply += "。外部天气暂未更新。";
  }
  return reply;
}


// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String LocalSoftChatReplyText(String userText) {
  (void)userText;
  return "";
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DoubaoTryLocalFastPath(String userText, bool executeNow) {
  String intent;
  String cmd = VoiceTextToCommand(userText, intent);
  String t = VoiceTextNormalize(userText);
  bool isWeatherAsk = VoiceTextHasAny(t, "天气", "温度", "湿度", "冷不冷") || VoiceTextHasAny(t, "热不热", "下雨", "气温", "光照");
  String softReply = "";

  if (!cmd.length() && !isWeatherAsk) return false;

  uint32_t startMs = millis();
  LastDoubaoAiText = userText;
  LastDoubaoAiModel = "local_fast";
  LastDoubaoAiError = "";
  LastDoubaoAiState = "local_fast";

  VoiceTaskSetText(userText, "local_fast_text_input");
  VoiceTask.brain = "local_fast";
  VoiceTask.intent = intent.length() ? intent : (isWeatherAsk ? "weather" : (softReply.length() ? "chat" : "local"));

  

  if (isWeatherAsk && !cmd.length()) {
    String reply = LocalWeatherReplyText();
    LastDoubaoAiReply = reply;
    LastDoubaoAiCommands = "";
    LastCloudReply = reply;
    LastCloudCommand = "";
    VoiceTask.command = "";
    VoiceTask.commandReady = false;
    VoiceTask.reply = reply;
    VoiceTask.replyReady = true;
    VoiceTask.status = VOICE_TASK_REPLIED;
    LastDoubaoAiElapsedMs = millis() - startMs;
    AddCenterLog("op", "本地极速回复天气：" + reply);
    BemfaPublishStatus();
    return true;
  }

  if (cmd.length()) {
    String execReply = "";
    if (executeNow) execReply = ExecuteUnifiedCommand(cmd, "LocalFastAI");
    String reply;
    if (executeNow) reply = execReply.length() ? execReply : "指令已执行";
    else reply = "已解析到指令：" + cmd;

    LastDoubaoAiReply = reply;
    LastDoubaoAiCommands = cmd;
    LastCloudReply = reply;
    LastCloudCommand = cmd;
    VoiceTask.command = cmd;
    VoiceTask.commandReady = true;
    VoiceTask.reply = reply;
    VoiceTask.replyReady = true;
    VoiceTask.executed = executeNow;
    VoiceTask.status = executeNow ? VOICE_TASK_EXECUTED : VOICE_TASK_COMMAND_READY;
    VoiceTask.updatedMs = millis();
    LastDoubaoAiElapsedMs = millis() - startMs;
    AddCenterLog("op", "本地极速AI：" + userText + " => " + cmd + (executeNow ? " 已执行" : " 未执行"));
    BemfaPublishStatus();
    return true;
  }

  return false;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DoubaoAiRunText(String userText, String modelMode, bool executeNow, bool useTts) {
#if ENABLE_DOUBAO_AI_CENTER
  userText = LingxiCleanUserRawText(userText);
  userText.trim();

  LastDoubaoAiReply = "";
  LastDoubaoAiCommands = "";
  LastDoubaoAiError = "";
  LastCloudCommand = "";
  LastCloudReply = "";
  if (userText.length() == 0) {
    LastDoubaoAiError = "AI 输入为空";
    LastDoubaoAiState = "error";
    return false;
  }
  if (userText.length() > 300) userText = userText.substring(0, 300);

  // 第一层严格白名单和本地天气仍由本地快速路径处理；未命中才进入云端自由对话。
  if (DoubaoTryLocalFastPath(userText, executeNow)) {
    return true;
  }
  if (millis() - LastDoubaoAiMs < DOUBAO_AI_COOLDOWN_MS) {
    LastDoubaoAiError = "AI 请求过快，已防抖";
    LastDoubaoAiState = "cooldown";
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }
  LastDoubaoAiMs = millis();
  uint32_t aiStartMs = LastDoubaoAiMs;
  LastDoubaoAiElapsedMs = 0;
  LastDoubaoAiState = "requesting";
  LastDoubaoAiError = "";
  LastDoubaoAiText = userText;
  LastDoubaoAiModel = modelMode.length() ? modelMode : CloudModelMode();

  if (WiFi.status() != WL_CONNECTED) {
    LastDoubaoAiError = "STA WiFi 未连接，无法调用豆包";
    LastDoubaoAiState = "error";
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }
  if (IsPlaceholderText(CloudArkApiKey())) {
    LastDoubaoAiError = "大模型接口密钥未填写";
    LastDoubaoAiState = "error";
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }

  String model = DoubaoModelByMode(LastDoubaoAiModel);
  String outText = "";
  if (!DoubaoChatCompletionsPost(userText, model, "TEXT", aiStartMs, outText)) return false;

  String reply = DoubaoCleanReply(outText, userText);
  reply.trim();
  if (reply.length() == 0) {
    LastDoubaoAiError = "豆包回复清洗后为空";
    LastDoubaoAiState = "reply_empty";
    AddCenterLog("sys", LastDoubaoAiError + " raw=" + outText.substring(0, 160));
    return false;
  }

  String commands = "";
  LastDoubaoAiReply = reply;
  LastDoubaoAiCommands = commands;
  LastCloudReply = reply;
  LastCloudCommand = commands;

  VoiceTaskSetText(userText, "doubao_text_input");
  VoiceTask.intent = "chat";
  VoiceTask.command = "";
  VoiceTask.commandReady = false;
  VoiceTask.reply = reply;
  VoiceTask.replyReady = true;
  VoiceTask.brain = "doubao_chat_api_" + LastDoubaoAiModel;

  AddCenterLog("op", "豆包自由对话回复：" + reply);
  AddCenterLog("op", "豆包自由对话：仅回复，不控制设备");
  VoiceTask.status = VOICE_TASK_REPLIED;

  if (useTts && reply.length()) {
    String roleSpeaker = CurrentRoleTtsSpeaker();
    LingxiLogTtsRole("自由对话", roleSpeaker);
    DoubaoTtsSynthesizeToSd(reply, roleSpeaker, String(DOUBAO_TTS_LAST_PATH));
  }

  VoiceTask.updatedMs = millis();
  LastDoubaoAiState = "done";
  BemfaPublishStatus();
  return true;
#else
  LastDoubaoAiError = "ENABLE_DOUBAO_AI_CENTER=0";
  return false;
#endif
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool FileToBase64String(String path, String &outB64, size_t maxBytes) {
  outB64 = "";
  File f = SD_MMC.open(path.c_str(), "r");
  if (!f) return false;
  size_t len = f.size();
  if (len == 0 || len > maxBytes) { f.close(); return false; }
  uint8_t *buf = (uint8_t*)malloc(len);
  if (!buf) { f.close(); return false; }
  size_t rd = f.read(buf, len);
  f.close();
  if (rd != len) { free(buf); return false; }
  size_t outLen = 0;
  mbedtls_base64_encode(nullptr, 0, &outLen, buf, len);
  unsigned char *out = (unsigned char*)malloc(outLen + 1);
  if (!out) { free(buf); return false; }
  if (mbedtls_base64_encode(out, outLen + 1, &outLen, buf, len) != 0) {
    free(buf); free(out); return false;
  }
  out[outLen] = 0;
  outB64 = String((char*)out);
  free(buf); free(out);
  return outB64.length() > 0;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DecodeBase64ToFile(String b64, File &file) {
  b64.trim();
  if (b64.length() == 0) return false;
  size_t outLen = 0;
  mbedtls_base64_decode(nullptr, 0, &outLen, (const unsigned char*)b64.c_str(), b64.length());
  if (outLen == 0) return false;
  uint8_t *buf = (uint8_t*)malloc(outLen);
  if (!buf) return false;
  if (mbedtls_base64_decode(buf, outLen, &outLen, (const unsigned char*)b64.c_str(), b64.length()) != 0) {
    free(buf); return false;
  }
  file.write(buf, outLen);
  free(buf);
  return true;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DoubaoAsrPostOnce(String path, String b64, const char* url, const char* resourceId, String &outText, String &outResp, int &outCode, uint32_t &outElapsed) {
  outText = "";
  outResp = "";
  outCode = -1;
  outElapsed = 0;

  String reqid = "lingxi-" + String(millis()) + "-" + String(VoiceTask.seq);
  String body = "{\"user\":{\"uid\":\"" + String(DOUBAO_SPEECH_APP_ID) + "\"},";
  body += "\"audio\":{\"data\":\"" + b64 + "\",\"format\":\"wav\",\"rate\":16000,\"bits\":16,\"channel\":1},";
  body += "\"request\":{\"model_name\":\"bigmodel\",\"enable_itn\":true,\"enable_punc\":true}}";

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, String(url))) {
    outResp = "HTTP begin ASR 失败";
    return false;
  }
  http.setTimeout(DOUBAO_ASR_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Api-Key", CloudVoiceApiKey());
  http.addHeader("X-Api-Resource-Id", String(resourceId));
  http.addHeader("X-Api-Request-Id", reqid);
  http.addHeader("X-Api-Sequence", "-1");

  uint32_t asrStartMs = millis();
  UF_Log("REQ", "ASR", "POST url=" + String(url) + " resource=" + String(resourceId) + " file=" + path + " bytes=" + String(b64.length()));
  outCode = http.POST(body);
  outResp = http.getString();
  http.end();
  outElapsed = millis() - asrStartMs;

  outText = VoiceTaskExtractAsrText(outResp);
  outText.trim();
  UF_Log((outCode >= 200 && outCode < 300 && outText.length()) ? "OK" : "FAIL", "ASR", "http=" + String(outCode) + " elapsed=" + String(outElapsed) + "ms text_len=" + String(outText.length()));
  return (outCode >= 200 && outCode < 300 && outText.length() > 0);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceTaskRunDoubaoAsr() {
#if ENABLE_DOUBAO_DIRECT_ASR
  LastDoubaoAsrState = "requesting";
  LastDoubaoAsrError = "";
  LastDoubaoAsrText = "";
  LastDoubaoAsrEndpoint = "";
  LastDoubaoAsrResource = "";
  LastDoubaoAsrHttpCode = 0;
  LastDoubaoAsrElapsedMs = 0;

  String path = VoiceTask.audioFile.length() ? VoiceTask.audioFile : VoiceIO.lastRecordFile;
  if (!VoiceIO.lastRecordReady || path.length() == 0 || !SD_MMC.exists(path.c_str())) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "没有可上传的录音文件";
    LastDoubaoAsrState = "error";
    LastDoubaoAsrError = VoiceTask.error;
    return false;
  }
  if (VoiceIO.peakLevel < 80 || VoiceIO.rmsLevel < 8) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "录音音量过低，请靠近中控麦克风再说一次 peak=" + String(VoiceIO.peakLevel) + " rms=" + String(VoiceIO.rmsLevel);
    LastDoubaoAsrState = "error";
    LastDoubaoAsrError = VoiceTask.error;
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "STA WiFi 未连接，无法调用豆包ASR";
    LastDoubaoAsrState = "error";
    LastDoubaoAsrError = VoiceTask.error;
    return false;
  }
  if (IsPlaceholderText(CloudVoiceApiKey())) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "语音接口密钥未填写，无法调用录音文件识别";
    LastDoubaoAsrState = "error";
    LastDoubaoAsrError = VoiceTask.error;
    return false;
  }

  String b64;
  if (!FileToBase64String(path, b64, 380000)) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "录音文件 Base64 编码失败或文件过大";
    LastDoubaoAsrState = "error";
    LastDoubaoAsrError = VoiceTask.error;
    return false;
  }

  String text, resp;
  int code = -1;
  uint32_t elapsed = 0;
  bool ok = false;

  
  
  ok = DoubaoAsrPostOnce(path, b64, DOUBAO_ASR_FLASH_URL, CloudAsrResource().c_str(), text, resp, code, elapsed);
  LastDoubaoAsrEndpoint = DOUBAO_ASR_FLASH_URL;
  LastDoubaoAsrResource = CloudAsrResource();
  LastDoubaoAsrHttpCode = code;
  LastDoubaoAsrElapsedMs = elapsed;

  if (!ok) {
    AddCenterLog("sys", "ASR flash未识别到文本 http=" + String(code) + " peak=" + String(VoiceIO.peakLevel) + " rms=" + String(VoiceIO.rmsLevel) + " resp=" + resp.substring(0, 160));
  }

  if (!ok) {
    VoiceTask.status = VOICE_TASK_ERROR;
    if (code >= 200 && code < 300) {
      VoiceTask.error = "ASR已收到音频但未识别到文字，请靠近麦克风/提高音量再说一次；" + resp.substring(0, 180);
    } else if (code == 404) {
      VoiceTask.error = "ASR接口路径不匹配，请检查录音文件识别接口版本；" + resp.substring(0, 180);
    } else {
      VoiceTask.error = "豆包ASR失败，请核对语音接口密钥和录音文件识别资源。" + resp.substring(0, 180);
    }
    LastDoubaoAsrState = "error";
    LastDoubaoAsrError = VoiceTask.error;
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }

  VoiceTaskSetText(text, "doubao_asr_file");
  VoiceTask.brain = "doubao_asr_file";
  VoiceTask.reply = "已识别：" + text;
  LastDoubaoAsrState = "done";
  LastDoubaoAsrText = text;
  LastDoubaoAsrError = "";
  AddCenterLog("op", "豆包ASR识别文本：" + text + " resource=" + LastDoubaoAsrResource);
  return true;
#else
  VoiceTask.status = VOICE_TASK_ERROR;
  VoiceTask.error = "ENABLE_DOUBAO_DIRECT_ASR=0";
  return false;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DoubaoTtsSynthesizeToSdOnce(String text, String speakerId, String outPath, String resourceId) {
#if ENABLE_DOUBAO_DIRECT_TTS
  text.trim();
  if (text.length() == 0) return false;
  
  text.replace("\n", "，");
  text.replace("\r", "");
  
  if (text.length() > 180) {
    text = text.substring(0, 180);
    if (!text.endsWith("。") && !text.endsWith("！") && !text.endsWith("？")) text += "。";
  }

  if (WiFi.status() != WL_CONNECTED) {
    LastDoubaoAiError = "STA WiFi 未连接，无法 TTS";
    return false;
  }
  if (IsPlaceholderText(CloudVoiceApiKey())) {
    LastDoubaoAiError = "语音接口密钥未填写，无法调用语音合成";
    return false;
  }
  if (speakerId.length() == 0) speakerId = String(DOUBAO_VOICE_SHOREKEEPER);

  
  
  
  String reqid = "ultra-tts-" + String(millis());
  String body = "{\"user\":{\"uid\":\"" + String(DOUBAO_SPEECH_APP_ID) + "\"},";
  body += "\"req_params\":{\"text\":\"" + JsonEscape(text) + "\",\"speaker\":\"" + JsonEscape(speakerId) + "\",";
  body += "\"audio_params\":{\"format\":\"wav\",\"sample_rate\":24000}";
  
  
  if (speakerId.startsWith("S_")) {
    body += ",\"additions\":\"{\\\"model_type\\\":4}\"";
  }
  body += "}}";

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  if (!http.begin(client, DOUBAO_TTS_URL)) {
    LastDoubaoAiError = "HTTP begin TTS 失败";
    return false;
  }
  const char* respHeaderKeys[] = {"X-Tt-Logid"};
  http.collectHeaders(respHeaderKeys, 1);
  http.setTimeout(DOUBAO_TTS_TIMEOUT_MS);
  http.addHeader("Content-Type", "application/json");
  
  http.addHeader("X-Api-Key", CloudVoiceApiKey());
  http.addHeader("X-Api-Resource-Id", resourceId);
  http.addHeader("X-Api-Request-Id", reqid);
  http.addHeader("X-Control-Require-Usage-Tokens-Return", "text_words");

  uint32_t ttsStartMs = millis();
  UF_Log("REQ", "TTS", "POST resource=" + resourceId + " 音色编号=" + speakerId + " text_len=" + String(text.length()) + " reqid=" + reqid);
  int code = http.POST(body);
  String resp = http.getString();
  String logid = http.header("X-Tt-Logid");
  http.end();
  UF_Log((code >= 200 && code < 300) ? "OK" : "FAIL", "TTS", "http=" + String(code) + " elapsed=" + String(millis() - ttsStartMs) + "ms logid=" + logid);
  if (code < 200 || code >= 300) {
    LastDoubaoAiError = "Doubao TTS HTTP " + String(code) + " logid=" + logid + " " + resp.substring(0, 220);
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }

  File out = SD_MMC.open(outPath.c_str(), "w");
  if (!out) {
    LastDoubaoAiError = "TTS 输出文件打开失败";
    return false;
  }

  uint32_t audioBytes = 0;
  int pos = 0;
  bool gotAudio = false;
  String firstPayload = "";
  String seenKeys = "";
  while (pos < (int)resp.length()) {
    int nl = resp.indexOf('\n', pos);
    String line = (nl < 0) ? resp.substring(pos) : resp.substring(pos, nl);
    pos = (nl < 0) ? resp.length() : nl + 1;
    line.trim();
    if (line.length() == 0 || line == "data: [DONE]" || line == "[DONE]") continue;
    if (line.startsWith("event:")) continue;
    if (line.startsWith("data:")) {
      line = line.substring(5);
      line.trim();
    }
    if (line.length() == 0) continue;
    if (firstPayload.length() == 0) firstPayload = line.substring(0, min((int)line.length(), 220));
    if (seenKeys.length() < 180) {
      if (line.indexOf("delta") >= 0) seenKeys += " delta";
      if (line.indexOf("audio") >= 0) seenKeys += " audio";
      if (line.indexOf("payload_msg") >= 0) seenKeys += " payload_msg";
      if (line.indexOf("data") >= 0) seenKeys += " data";
      if (line.indexOf("code") >= 0) seenKeys += " code";
    }

    
    
    
    String bizCode = JsonGetString(line, "code");
    String bizMsg  = JsonGetString(line, "message");
    
    
    bool bizOk = (bizCode.length() == 0 || bizCode == "0" || bizCode == "00000000" || bizCode == "20000000" || bizMsg == "OK");
    if (!bizOk) {
      LastDoubaoAiError = "TTS业务错误 resource=" + resourceId + " 音色编号=" + speakerId + " code=" + bizCode + " msg=" + bizMsg + " logid=" + logid;
      AddCenterLog("sys", LastDoubaoAiError);
      out.close();
      SD_MMC.remove(outPath.c_str());
      return false;
    }

    String data = JsonGetString(line, "delta");
    if (data.length() == 0) data = JsonGetString(line, "audio");
    if (data.length() == 0) data = JsonGetString(line, "audio_data");
    if (data.length() == 0) data = JsonGetString(line, "data");
    if (data.length() == 0) data = JsonGetString(line, "binary");
    if (data.length() == 0) {
      String nested = JsonGetStringAfter(line, "payload_msg", 0);
      if (nested.length() == 0) nested = JsonGetStringAfter(line, "message", 0);
      if (nested.length() == 0) nested = JsonGetStringAfter(line, "response", 0);
      if (nested.length()) {
        String nData = JsonGetStringAfter(nested, "delta", 0);
        if (nData.length() == 0) nData = JsonGetStringAfter(nested, "audio", 0);
        if (nData.length() == 0) nData = JsonGetStringAfter(nested, "audio_data", 0);
        if (nData.length() == 0) nData = JsonGetStringAfter(nested, "data", 0);
        if (nData.length()) data = nData;
      }
    }
    if (data.length()) {
      uint32_t before = out.position();
      if (DecodeBase64ToFile(data, out)) {
        uint32_t after = out.position();
        if (after > before) {
          gotAudio = true;
          audioBytes += (after - before);
        }
      }
    }
  }
  out.close();
  if (!gotAudio || audioBytes == 0) {
    SD_MMC.remove(outPath.c_str());
    LastDoubaoAiError = "TTS 未解析到音频字段，logid=" + logid;
    AddCenterLog("sys", LastDoubaoAiError + " keys=" + seenKeys + " resp=" + firstPayload);
    return false;
  }
  AudioFixWavHeaderDataSize(outPath);
  AddCenterLog("op", "豆包TTS已生成WAV：" + outPath + " bytes=" + String(audioBytes));
  return true;
#else
  return false;
#endif
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DoubaoTtsSynthesizeToSd(String text, String speakerId, String outPath) {
#if ENABLE_DOUBAO_DIRECT_TTS
  
  
  String publicErr = "";
  String cloneErr = "";
  bool ok = false;

  if (DOUBAO_TTS_PUBLIC_FIRST) {
    ok = DoubaoTtsSynthesizeToSdOnce(text, CloudPublicSpeaker(), outPath, CloudTtsPublicResource());
    if (ok) {
      LastDoubaoAiError = "";
      AddCenterLog("op", "TTS公版稳定模式成功：浏览器可播放。");
      return true;
    }
    publicErr = LastDoubaoAiError;
    AddCenterLog("sys", "TTS公版模式失败，尝试角色复刻音色。" + publicErr.substring(0, 120));

    ok = DoubaoTtsSynthesizeToSdOnce(text, speakerId, outPath, CloudTtsCloneResource());
    if (ok) {
      LastDoubaoAiError = "";
      AddCenterLog("op", "TTS角色音色成功：TTS2 role 音色编号=" + speakerId);
      return true;
    }
    cloneErr = LastDoubaoAiError;
    LastDoubaoAiError = "TTS公版与复刻均失败。public_err=" + publicErr.substring(0, 120) + " clone_err=" + cloneErr.substring(0, 120);
    AddCenterLog("sys", LastDoubaoAiError);
    return false;
  }

  ok = DoubaoTtsSynthesizeToSdOnce(text, speakerId, outPath, CloudTtsCloneResource());
  if (ok) {
    AddCenterLog("op", "TTS角色音色成功：TTS2 resource=" + CloudTtsCloneResource() + " 音色编号=" + speakerId);
    return true;
  }
  cloneErr = LastDoubaoAiError;
  AddCenterLog("sys", "TTS角色音色失败，自动回退公版TTS。" + cloneErr.substring(0, 140));

  ok = DoubaoTtsSynthesizeToSdOnce(text, CloudPublicSpeaker(), outPath, CloudTtsPublicResource());
  if (ok) {
    LastDoubaoAiError = "";
    AddCenterLog("op", "TTS公版兜底成功：角色音色失败但语音播放保持可用。public_speaker=" + CloudPublicSpeaker());
    return true;
  }
  LastDoubaoAiError = "TTS双模式均失败。clone_err=" + cloneErr.substring(0, 120) + " public_err=" + LastDoubaoAiError.substring(0, 160);
  AddCenterLog("sys", LastDoubaoAiError);
  return false;
#else
  return false;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
uint16_t FirstAiCommandVoiceIdForTtsFallback() {
  String cmd = LastDoubaoAiCommands;
  if (!cmd.length()) cmd = VoiceTask.command;
  cmd.trim();
  int comma = cmd.indexOf(',');
  if (comma >= 0) cmd = cmd.substring(0, comma);
  cmd.trim();
  if (!cmd.length()) return 0;
  return VoiceIdForCommand(cmd, LastDoubaoAiReply.length() ? LastDoubaoAiReply : VoiceTask.reply);
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleDoubaoAiTextApi() {
  String text = server.hasArg("text") ? server.arg("text") : "";
  UF_Log("REQ", "DOUBAO", "AI text request: " + text);
  String model = server.hasArg("model") ? server.arg("model") : String(DOUBAO_DEFAULT_MODEL_MODE);
  bool executeNow = true;
  bool useTts = false;
  if (server.hasArg("execute")) {
    String e = server.arg("execute"); e.toLowerCase(); executeNow = !(e == "0" || e == "false" || e == "no");
  }
  if (server.hasArg("tts")) {
    String t = server.arg("tts"); t.toLowerCase(); useTts = (t == "1" || t == "true" || t == "yes");
  }
  bool ok = DoubaoAiRunText(text, model, executeNow, useTts);
  if (ok && LastDoubaoAiReply.length() == 0) {
    ok = false;
    LastDoubaoAiError = "自由对话执行成功但回复为空";
    LastDoubaoAiState = "reply_empty";
  }
  UF_Log(ok ? "OK" : "FAIL", "DOUBAO", ok ? ("reply=" + LastDoubaoAiReply + " commands=" + LastDoubaoAiCommands) : LastDoubaoAiError);
  String json = "{\"ok\":" + String(ok ? "true" : "false") + ",";
  json += "\"model\":\"" + JsonEscape(LastDoubaoAiModel) + "\",";
  json += "\"text\":\"" + JsonEscape(LastDoubaoAiText) + "\",";
  json += "\"reply\":\"" + JsonEscape(LastDoubaoAiReply) + "\",";
  json += "\"commands\":\"" + JsonEscape(LastDoubaoAiCommands) + "\",";
  json += "\"error\":\"" + JsonEscape(LastDoubaoAiError) + "\",";
  json += "\"state\":\"" + JsonEscape(LastDoubaoAiState) + "\",";
  json += "\"elapsed_ms\":" + String(LastDoubaoAiElapsedMs) + ",";
  json += "\"task\":" + VoiceTaskStatusJson() + "}";
  SendCorsHeader();
  server.send(ok ? 200 : 500, "application/json", json);
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleDoubaoAiConfigApi() {
  String json = "{\"ok\":true,";
  json += "\"base_url\":\"" + JsonEscape(DOUBAO_RESPONSES_URL) + "\",";
  json += "\"api_key_set\":" + String((CloudArkApiKey().length() && !IsPlaceholderText(CloudArkApiKey())) ? "true" : "false") + ",";
  json += "\"ark_api_key_set\":" + String((CloudArkApiKey().length() && !IsPlaceholderText(CloudArkApiKey())) ? "true" : "false") + ",";
  json += "\"voice_api_key_set\":" + String((CloudVoiceApiKey().length() && !IsPlaceholderText(CloudVoiceApiKey())) ? "true" : "false") + ",";
  json += "\"model_mini\":\"" + JsonEscape(CloudModelMini()) + "\",";
  json += "\"model_lite\":\"" + JsonEscape(CloudModelLite()) + "\",";
  json += "\"model_pro\":\"" + JsonEscape(CloudModelPro()) + "\",";
  json += "\"model_mode\":\"" + JsonEscape(CloudModelMode()) + "\",";
  json += "\"asr_resource\":\"" + JsonEscape(CloudAsrResource()) + "\",";
  json += "\"tts_public_resource\":\"" + JsonEscape(CloudTtsPublicResource()) + "\",";
  json += "\"tts_clone_resource\":\"" + JsonEscape(CloudTtsCloneResource()) + "\",";
  json += "\"public_speaker\":\"" + JsonEscape(CloudPublicSpeaker()) + "\",";
  json += "\"shorekeeper_voice\":\"" + JsonEscape(CloudVoiceShorekeeper()) + "\",";
  json += "\"cartethyia_voice\":\"" + JsonEscape(CloudVoiceCartethyia()) + "\"}";
  SendCorsHeader();
  server.send(200, "application/json", json);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskDoubaoAsrApi() {
  bool ok = VoiceTaskRunDoubaoAsr();
  String json = VoiceTaskStatusJson();
  if (!ok) json.replace("\"ok\":true", "\"ok\":false");
  SendCorsHeader();
  server.send(ok ? 200 : 500, "application/json", json);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskDoubaoAiExecuteApi() {
  bool ok = true;
  if (VoiceTask.text.length() == 0) ok = VoiceTaskRunDoubaoAsr();

  String cleanText = VoiceTaskExtractAsrText(VoiceTask.text);
  if (ok && cleanText.length() == 0) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "语音没有识别到有效文字";
    VoiceTask.updatedMs = millis();
    ok = false;
  }

  bool useTts = false;
  if (server.hasArg("tts")) {
    String t = server.arg("tts");
    t.toLowerCase();
    useTts = (t == "1" || t == "true" || t == "yes");
  }

  if (ok) {
    String intent = "";
    String cmd = VoiceTextToCommand(cleanText, intent);

    if (cmd.length()) {
      VoiceTaskSetCommand(cmd, intent);
      String reply = ExecuteUnifiedCommand(cmd, "VoiceASR");
      VoiceTask.status = VOICE_TASK_EXECUTED;
      VoiceTask.executed = true;
      VoiceTask.replyReady = true;
      VoiceTask.reply = reply;
      VoiceTask.updatedMs = millis();
      LastCloudCommand = cmd;
      LastCloudReply = reply;
      AddCenterLog("op", "语音第一层白名单执行：" + cmd + " => " + reply);
      BemfaPublishStatus();
      ok = true;
    } else {
      LingxiVoiceRouteResult route;
      if (ConfirmAgentTryRoute(cleanText, route)) {
        VoiceTask.intent = route.intent.length() ? route.intent : "confirm_agent";
        VoiceTask.command = route.command;
        VoiceTask.commandReady = route.command.length() > 0;
        VoiceTask.reply = route.reply.length() ? route.reply : "我理解到一个可能的设备动作，需要你确认后再执行。";
        VoiceTask.replyReady = true;
        VoiceTask.updatedMs = millis();

        if (route.command.length() && !route.needConfirm) {
          String execReply = ExecuteUnifiedCommand(route.command, "VoiceRuleEngine");
          VoiceTask.status = VOICE_TASK_EXECUTED;
          VoiceTask.executed = true;
          VoiceTask.reply = execReply.length() ? execReply : VoiceTask.reply;
          LastCloudCommand = route.command;
          LastCloudReply = VoiceTask.reply;
          AddCenterLog("op", "语音第二层确认执行：" + route.command + " => " + VoiceTask.reply);
          BemfaPublishStatus();
        } else {
          VoiceTask.status = VOICE_TASK_REPLIED;
          AddCenterLog("op", "语音第二层规则引擎回复：" + VoiceTask.reply);
        }

        if (useTts && VoiceTask.reply.length()) {
          DoubaoTtsSynthesizeToSd(VoiceTask.reply, CurrentRoleTtsSpeaker(), String(DOUBAO_TTS_LAST_PATH));
        }
        ok = true;
      } else {
        
        ok = DoubaoFreeChatRunText(cleanText, server.hasArg("model") ? server.arg("model") : String(DOUBAO_DEFAULT_MODEL_MODE), useTts);
      }
    }
  }

  String json = VoiceTaskStatusJson();
  if (!ok) json.replace("\"ok\":true", "\"ok\":false");
  SendCorsHeader();
  server.send(ok ? 200 : 500, "application/json", json);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleDoubaoTtsApi() {
  String text = server.hasArg("text") ? server.arg("text") : LastDoubaoAiReply;
  text = LingxiCleanUserRawText(text);
  String resource = server.hasArg("resource") ? server.arg("resource") : "";
  resource.trim();
  
  
  bool allowRequestSpeaker = server.hasArg("speaker") && resource.length() > 0 && !server.hasArg("play");
  String speaker = CurrentRoleTtsSpeaker();
  if (allowRequestSpeaker) {
    speaker = server.arg("speaker");
    speaker.trim();
  } else if (server.hasArg("speaker")) {
    String ignored = server.arg("speaker");
    ignored.trim();
    if (ignored.length() && ignored != speaker) {
      AddCenterLog("sys", "已忽略前端残留TTS音色参数：request=" + ignored + " active=" + speaker);
    }
  }
  LingxiLogTtsRole("HandleDoubaoTtsApi", speaker);
  if (resource.length() == 0) resource = CloudTtsCloneResource();
  
  
  if (resource.startsWith("TTS-SeedTTS") || resource.startsWith("TTS-SeedICL") || resource.startsWith("VoiceCloning")) {
    AddCenterLog("sys", "语音资源已自动修正：用户填写的是旧版实例名，角色音色实际按新版复刻音色资源请求。raw=" + resource);
    resource = CloudTtsCloneResource();
  }
  bool allowFallback = true;
  if (server.hasArg("fallback")) {
    String fb = server.arg("fallback");
    fb.toLowerCase();
    allowFallback = !(fb == "0" || fb == "false" || fb == "no");
  }

  bool ok = false;
  String ttsMode = "role_clone_first_public_fallback";

  if (resource.length() > 0) {
    
    ok = DoubaoTtsSynthesizeToSdOnce(text, speaker, String(DOUBAO_TTS_LAST_PATH), resource);
    ttsMode = "custom_resource_probe";
    if (ok) {
      LastDoubaoAiError = "";
      AddCenterLog("op", "语音合成配置探针成功：请求资源=" + resource + " 音色编号=" + speaker);
    } else {
      String probeErr = LastDoubaoAiError;
      String tip = "";
      if (probeErr.indexOf("requested resource not granted") >= 0) tip = "；判断：语音合成请求资源或当前音色编号未授权/未开通/不在当前项目权限内";
      else if (probeErr.indexOf("mismatched") >= 0) tip = "；判断：角色音色编号不能通过当前语音合成请求资源生成音频，请确认音色所属应用和权限";
      AddCenterLog("sys", "语音合成配置探针失败：请求资源=" + resource + " 音色编号=" + speaker + tip + " " + probeErr.substring(0, 140));
      if (allowFallback) {
        ok = DoubaoTtsSynthesizeToSdOnce(text, CloudPublicSpeaker(), String(DOUBAO_TTS_LAST_PATH), CloudTtsPublicResource());
        ttsMode = ok ? "custom_probe_public_fallback" : "custom_probe_all_failed";
        if (ok) AddCenterLog("op", "角色音色探针失败后公版兜底成功。注意：这不是角色音色成功。");
      } else {
        ttsMode = "role_strict_failed_no_fallback";
        AddCenterLog("sys", "角色声音试听失败：已按要求禁止公版兜底。请检查语音合成请求资源、音色编号、应用权限或新版API接入方式。");
      }
    }
  } else {
    ok = DoubaoTtsSynthesizeToSd(text, speaker, String(DOUBAO_TTS_LAST_PATH));
  }

  bool boardFallback = false;
  bool boardTtsPlayed = false;
  uint16_t boardVoiceId = 0;
  if (ok && server.hasArg("play")) {
    bool wantBoardSpeaker = (VoiceIO.outputTarget == VOICE_OUTPUT_BOARD_SPEAKER || VoiceIO.outputTarget == VOICE_OUTPUT_BOTH || VoiceIO.outputTarget == VOICE_OUTPUT_AUTO);
    if (wantBoardSpeaker) {
      boardTtsPlayed = BoardAudioQueueTtsPlayback(String(DOUBAO_TTS_LAST_PATH), "DoubaoTTS");
      if (boardTtsPlayed) {
        AddCenterLog("op", "AI语音WAV已生成；中控扬声器已异步播放AI回复。 ");
      } else {
        AddCenterLog("sys", "AI语音WAV已生成，但中控扬声器播放入队失败：" + Voice.lastVoiceError);
      }
    }

    if (!boardTtsPlayed) {
      boardVoiceId = FirstAiCommandVoiceIdForTtsFallback();
      if (boardVoiceId > 0 && wantBoardSpeaker) {
        boardFallback = VoiceRequestPlay(boardVoiceId, "TTS_SD_Fallback");
        AddCenterLog(boardFallback ? "op" : "sys", boardFallback ? ("完整AI语音未由中控播出；已使用SD语音包兜底播报 #" + String(boardVoiceId)) : ("完整AI语音未由中控播出；SD兜底播报失败 #" + String(boardVoiceId)));
      } else if (!wantBoardSpeaker) {
        AddCenterLog("op", "AI语音WAV已生成；当前选择 WebUI/浏览器扬声器。 ");
      } else {
        AddCenterLog("sys", "AI语音WAV已生成；当前无设备命令，也没有可用SD兜底语音。 ");
      }
    }
  }

  String json = "{\"ok\":" + String(ok ? "true" : "false") + ",\"file\":\"" + JsonEscape(DOUBAO_TTS_LAST_PATH) + "\",\"error\":\"" + JsonEscape(LastDoubaoAiError) + "\",";
  json += "\"tts_mode\":\"" + JsonEscape(ttsMode) + "\",";
  json += "\"resource\":\"" + JsonEscape(resource) + "\",";
  json += "\"speaker\":\"" + JsonEscape(speaker) + "\",";
  json += "\"active_role\":\"" + JsonEscape(RoleIdByIndex(Voice.currentRole)) + "\",";
  json += "\"active_role_name\":\"" + JsonEscape(VoiceRoleName()) + "\",";
  json += "\"board_tts_played\":" + String(boardTtsPlayed ? "true" : "false") + ",";
  json += "\"board_fallback\":" + String(boardFallback ? "true" : "false") + ",";
  json += "\"board_voice_id\":" + String(boardVoiceId) + "}";
  SendCorsHeader();
  server.send(ok ? 200 : 500, "application/json", json);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleDoubaoTtsFileApi() {
  SendCorsHeader();
  String path = String(DOUBAO_TTS_LAST_PATH);
  if (!SD_MMC.exists(path.c_str())) {
    server.send(404, "text/plain", "TTS file not found");
    return;
  }
  AudioFixWavHeaderDataSize(path);
  File f = SD_MMC.open(path.c_str(), "r");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Content-Length", String(f.size()));
  server.streamFile(f, "audio/wav");
  f.close();
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskStatusApi() {
  SendCorsHeader();
  server.send(200, "application/json", VoiceTaskStatusJson());
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskClearApi() {
  VoiceTaskClear("VoiceTask cleared");
  AddCenterLog("op", "VoiceTask 已清空");
  SendCorsHeader();
  server.send(200, "application/json", VoiceTaskStatusJson());
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskMockTextApi() {
  String text = server.hasArg("text") ? server.arg("text") : "";
  text.trim();
  if (text.length() == 0) text = "打开电视";
  VoiceTaskSetText(text, "mock_asr");
  AddCenterLog("op", "VoiceTask 模拟ASR文本：" + text);
  SendCorsHeader();
  server.send(200, "application/json", VoiceTaskStatusJson());
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskMockCommandApi() {
  String cmd = server.hasArg("cmd") ? server.arg("cmd") : "";
  String intent = server.hasArg("intent") ? server.arg("intent") : "manual_mock";
  cmd.trim();
  intent.trim();
  VoiceTaskSetCommand(cmd, intent);
  AddCenterLog("op", "VoiceTask 模拟命令：" + cmd);
  SendCorsHeader();
  server.send(200, "application/json", VoiceTaskStatusJson());
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskMockReplyApi() {
  String reply = server.hasArg("reply") ? server.arg("reply") : "";
  reply.trim();
  if (reply.length() == 0) reply = "语音任务回复文本待生成";
  VoiceTaskSetReply(reply);
  AddCenterLog("op", "VoiceTask 模拟回复：" + reply);
  SendCorsHeader();
  server.send(200, "application/json", VoiceTaskStatusJson());
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void RefreshLegacyLightField(ControlCommand &c) {
  if (c.livingLightCmd || c.bedroomLightCmd) {
    c.lightCmd = 1;
    c.lightLevel = 180;
  } else {
    c.lightCmd = 0;
    c.lightLevel = 0;
  }
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CommitWebCommand(ControlCommand c) {
  c.cmdSeq++;
  SetControlCommand(c);
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CommitEndpointCommand(ControlCommand c, const char* endpointId) {
  NodeCompatSetPendingRoute(endpointId);
  CommitWebCommand(c);
}

// 函数说明：节点/端点函数，负责设备模型、注册、状态或控制。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CommitEndpointGroupCommand(ControlCommand c, const char* endpointList) {
  NodeCompatSetPendingRoute(endpointList);
  CommitWebCommand(c);
}

