/*
 * 文件：08_pages_settings_part2.ino
 * 标题：系统设置页后半部分与豆包配置
 * 说明：负责设置页后半部分、模型配置和对话提示相关函数。
 * 可改：可调模型模式
 * 禁止：写入真实密钥和账号信息。
 */

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String CloudOrDefault(String v, const char* defv) {
  v.trim();
  if (v.length() == 0) return String(defv);
  return v;
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String CloudArkApiKey() {
  String k = SavedArkApiKey;
  k.trim();
  if (k.length() == 0) k = SavedNewApiKey; 
  return CloudOrDefault(k, DOUBAO_API_KEY);
}
// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String CloudVoiceApiKey() {
  String k = SavedVoiceApiKey;
  k.trim();
  if (k.length() == 0) k = SavedNewApiKey; 
  return CloudOrDefault(k, DOUBAO_SPEECH_API_KEY);
}

String CloudApiKey() { return CloudArkApiKey(); }
String CloudModelMode() { return CloudOrDefault(SavedDefaultModelMode, DOUBAO_DEFAULT_MODEL_MODE); }
String CloudModelMini() { return CloudOrDefault(SavedModelMini, DOUBAO_MODEL_MINI); }
String CloudModelLite() { return CloudOrDefault(SavedModelLite, DOUBAO_MODEL_LITE); }
String CloudModelPro() { return CloudOrDefault(SavedModelPro, DOUBAO_MODEL_PRO); }
String CloudAsrResource() { return CloudOrDefault(SavedAsrResource, DOUBAO_ASR_RESOURCE_ID_STANDARD); }
String CloudTtsPublicResource() { return CloudOrDefault(SavedTtsPublicResource, DOUBAO_TTS_RESOURCE_ID_PUBLIC); }
String CloudTtsCloneResource() { return CloudOrDefault(SavedTtsCloneResource, DOUBAO_TTS_RESOURCE_ID_ROLE); }
String CloudPublicSpeaker() { return CloudOrDefault(SavedPublicSpeaker, DOUBAO_TTS_PUBLIC_SPEAKER); }
String CloudVoiceShorekeeper() { return CloudOrDefault(SavedVoiceShorekeeper, DOUBAO_VOICE_SHOREKEEPER); }
String CloudVoiceCartethyia() { return CloudOrDefault(SavedVoiceCartethyia, DOUBAO_VOICE_CARTETHYIA); }
// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleTtsSpeakerByIndex(uint8_t role) {
  
  
  
  String fallback = (role == ROLE_CARTETHYIA) ? String(DOUBAO_VOICE_CARTETHYIA) : String(DOUBAO_VOICE_SHOREKEEPER);
  String cfg = (role == ROLE_CARTETHYIA) ? CloudVoiceCartethyia() : CloudVoiceShorekeeper();
  cfg.trim();
  if (cfg.length()) fallback = cfg;

  String voiceId = fallback;
#if ENABLE_SD_VOICE
  if (Voice.sdOnline) {
    String roleJson = SdReadSmallTextFile(RoleJsonPathByIndex(role), 4096);
    String fromJson = JsonFieldLoose(roleJson, "voice_id", "");
    fromJson.trim();
    if (fromJson.length()) voiceId = fromJson;
  }
#endif

  String shore = CloudVoiceShorekeeper(); shore.trim();
  String cart = CloudVoiceCartethyia(); cart.trim();
  if (role == ROLE_CARTETHYIA) {
    if (voiceId.length() == 0 || voiceId == shore || voiceId == String(DOUBAO_VOICE_SHOREKEEPER)) {
      voiceId = String(DOUBAO_VOICE_CARTETHYIA);
    }
  } else {
    if (voiceId.length() == 0 || voiceId == cart || voiceId == String(DOUBAO_VOICE_CARTETHYIA)) {
      voiceId = String(DOUBAO_VOICE_SHOREKEEPER);
    }
  }
  return voiceId;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String CurrentRoleTtsSpeaker() {
  return RoleTtsSpeakerByIndex(Voice.currentRole);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String LingxiCleanUserRawText(String text) {
  
  
  text.replace("\r", " ");
  text.replace("\n", " ");
  text.replace("文字加语音回复", "");
  text.replace("文字输入：", "");
  text.replace("文字输入:", "");
  text.replace("文字输入", "");
  text.replace("语音输入：", "");
  text.replace("语音输入:", "");
  text.replace("语音输入", "");
  text.replace("语音识别：", "");
  text.replace("语音识别:", "");
  text.replace("语音识别", "");
  text.replace("识别结果：", "");
  text.replace("识别结果:", "");
  text.replace("语音回复", "");
  text.replace("按住说话", "");
  text.replace("用户：", "");
  text.replace("用户:", "");
  text.replace("ASR：", "");
  text.replace("ASR:", "");
  while (text.indexOf("  ") >= 0) text.replace("  ", " ");
  text.trim();
  return text;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void LingxiLogTtsRole(String source, String speaker) {
  AddCenterLog("op", String(source) + " TTS角色音色：role=" + RoleIdByIndex(Voice.currentRole) + "/" + VoiceRoleName() + " speaker=" + speaker);
  Serial.println("[V6.0.1 TTS] " + String(source) + " role=" + RoleIdByIndex(Voice.currentRole) + " name=" + VoiceRoleName() + " speaker=" + speaker);
}

String CloudAmapWeatherKey() { return CloudOrDefault(SavedAmapWeatherKey, AMAP_WEATHER_KEY); }
String CloudAmapCityCode() { return CloudOrDefault(SavedAmapCityCode, AMAP_CITY_CODE); }
String CloudBemfaUid() { return CloudOrDefault(SavedBemfaUid, BEMFA_UID); }
String CloudBemfaTopicCmd() { return CloudOrDefault(SavedBemfaTopicCmd, BEMFA_TOPIC_CMD); }
String CloudBemfaTopicStatus() { return CloudOrDefault(SavedBemfaTopicStatus, BEMFA_TOPIC_STATUS); }
String CloudBemfaTopicLog() { return CloudOrDefault(SavedBemfaTopicLog, BEMFA_TOPIC_LOG); }

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void LoadCloudConfig() {
  CloudPrefs.begin("lx_cloud", true);
  SavedArkApiKey = CloudPrefs.getString("ark_key", "");
  SavedVoiceApiKey = CloudPrefs.getString("voice_key", "");
  SavedNewApiKey = CloudPrefs.getString("api_key", "");  
  SavedModelMini = CloudPrefs.getString("model_mini", DOUBAO_MODEL_MINI);
  SavedModelLite = CloudPrefs.getString("model_lite", DOUBAO_MODEL_LITE);
  SavedModelPro = CloudPrefs.getString("model_pro", DOUBAO_MODEL_PRO);
  SavedDefaultModelMode = CloudPrefs.getString("model_mode", DOUBAO_DEFAULT_MODEL_MODE);
  SavedAsrResource = CloudPrefs.getString("asr_res", DOUBAO_ASR_RESOURCE_ID_STANDARD);
  SavedTtsPublicResource = CloudPrefs.getString("tts_pub", DOUBAO_TTS_RESOURCE_ID_PUBLIC);
  SavedTtsCloneResource = CloudPrefs.getString("tts_clone", DOUBAO_TTS_RESOURCE_ID_ROLE);
  SavedPublicSpeaker = CloudPrefs.getString("pub_spk", DOUBAO_TTS_PUBLIC_SPEAKER);
  SavedVoiceShorekeeper = CloudPrefs.getString("voice_shore", DOUBAO_VOICE_SHOREKEEPER);
  SavedVoiceCartethyia = CloudPrefs.getString("voice_cart", DOUBAO_VOICE_CARTETHYIA);
  SavedAmapWeatherKey = CloudPrefs.getString("amap_key", AMAP_WEATHER_KEY);
  SavedAmapCityCode = CloudPrefs.getString("amap_city", AMAP_CITY_CODE);
  SavedBemfaUid = CloudPrefs.getString("bemfa_uid", BEMFA_UID);
  SavedBemfaTopicCmd = CloudPrefs.getString("bemfa_cmd", BEMFA_TOPIC_CMD);
  SavedBemfaTopicStatus = CloudPrefs.getString("bemfa_status", BEMFA_TOPIC_STATUS);
  SavedBemfaTopicLog = CloudPrefs.getString("bemfa_log", BEMFA_TOPIC_LOG);
  CloudPrefs.end();
  AddCenterLog("sys", "系统配置已加载：火山引擎 / 高德天气 / 巴法云 MQTT");
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void SaveCloudConfigFromCurrent() {
  CloudPrefs.begin("lx_cloud", false);
  CloudPrefs.putString("ark_key", SavedArkApiKey);
  CloudPrefs.putString("voice_key", SavedVoiceApiKey);
  CloudPrefs.putString("api_key", SavedNewApiKey); 
  CloudPrefs.putString("model_mini", SavedModelMini);
  CloudPrefs.putString("model_lite", SavedModelLite);
  CloudPrefs.putString("model_pro", SavedModelPro);
  CloudPrefs.putString("model_mode", SavedDefaultModelMode);
  CloudPrefs.putString("asr_res", SavedAsrResource);
  CloudPrefs.putString("tts_pub", SavedTtsPublicResource);
  CloudPrefs.putString("tts_clone", SavedTtsCloneResource);
  CloudPrefs.putString("pub_spk", SavedPublicSpeaker);
  CloudPrefs.putString("voice_shore", SavedVoiceShorekeeper);
  CloudPrefs.putString("voice_cart", SavedVoiceCartethyia);
  CloudPrefs.putString("amap_key", SavedAmapWeatherKey);
  CloudPrefs.putString("amap_city", SavedAmapCityCode);
  CloudPrefs.putString("bemfa_uid", SavedBemfaUid);
  CloudPrefs.putString("bemfa_cmd", SavedBemfaTopicCmd);
  CloudPrefs.putString("bemfa_status", SavedBemfaTopicStatus);
  CloudPrefs.putString("bemfa_log", SavedBemfaTopicLog);
  CloudPrefs.end();
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String MaskSecretForUi(String v) {
  v.trim();
  if (v.length() <= 8) return v.length() ? "********" : "";
  return v.substring(0, 4) + "****" + v.substring(v.length() - 4);
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleCloudConfigGetApi() {
  String json = "{\"ok\":true,";
  bool arkSet = CloudArkApiKey().length() && !IsPlaceholderText(CloudArkApiKey());
  bool voiceSet = CloudVoiceApiKey().length() && !IsPlaceholderText(CloudVoiceApiKey());
  json += "\"api_key_set\":" + String((arkSet && voiceSet) ? "true" : "false") + ","; 
  json += "\"api_key_mask\":\"" + JsonEscape(MaskSecretForUi(CloudArkApiKey())) + "\",";
  json += "\"ark_api_key_set\":" + String(arkSet ? "true" : "false") + ",";
  json += "\"ark_api_key_mask\":\"" + JsonEscape(MaskSecretForUi(CloudArkApiKey())) + "\",";
  json += "\"voice_api_key_set\":" + String(voiceSet ? "true" : "false") + ",";
  json += "\"voice_api_key_mask\":\"" + JsonEscape(MaskSecretForUi(CloudVoiceApiKey())) + "\",";
  json += "\"model_mode\":\"" + JsonEscape(CloudModelMode()) + "\",";
  json += "\"model_mini\":\"" + JsonEscape(CloudModelMini()) + "\",";
  json += "\"model_lite\":\"" + JsonEscape(CloudModelLite()) + "\",";
  json += "\"model_pro\":\"" + JsonEscape(CloudModelPro()) + "\",";
  json += "\"asr_resource\":\"" + JsonEscape(CloudAsrResource()) + "\",";
  json += "\"tts_public_resource\":\"" + JsonEscape(CloudTtsPublicResource()) + "\",";
  json += "\"tts_clone_resource\":\"" + JsonEscape(CloudTtsCloneResource()) + "\",";
  json += "\"public_speaker\":\"" + JsonEscape(CloudPublicSpeaker()) + "\",";
  json += "\"voice_shore\":\"" + JsonEscape(CloudVoiceShorekeeper()) + "\",";
  json += "\"voice_cart\":\"" + JsonEscape(CloudVoiceCartethyia()) + "\",";
  bool amapSet = CloudAmapWeatherKey().length() && !IsPlaceholderText(CloudAmapWeatherKey());
  bool bemfaSet = CloudBemfaUid().length() && !IsPlaceholderText(CloudBemfaUid());
  json += "\"amap_key_set\":" + String(amapSet ? "true" : "false") + ",";
  json += "\"amap_key_mask\":\"" + JsonEscape(MaskSecretForUi(CloudAmapWeatherKey())) + "\",";
  json += "\"amap_city\":\"" + JsonEscape(CloudAmapCityCode()) + "\",";
  json += "\"bemfa_uid_set\":" + String(bemfaSet ? "true" : "false") + ",";
  json += "\"bemfa_uid_mask\":\"" + JsonEscape(MaskSecretForUi(CloudBemfaUid())) + "\",";
  json += "\"bemfa_cmd\":\"" + JsonEscape(CloudBemfaTopicCmd()) + "\",";
  json += "\"bemfa_status\":\"" + JsonEscape(CloudBemfaTopicStatus()) + "\",";
  json += "\"bemfa_log\":\"" + JsonEscape(CloudBemfaTopicLog()) + "\"}";
  SendCorsHeader();
  server.send(200, "application/json", json);
}

// 函数说明：网络服务函数，负责 WiFi、云平台、天气或 MQTT/Bemfa 相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleCloudConfigSaveApi() {
  if (server.hasArg("ark_api_key")) SavedArkApiKey = server.arg("ark_api_key");
  if (server.hasArg("voice_api_key")) SavedVoiceApiKey = server.arg("voice_api_key");
  if (server.hasArg("api_key")) { 
    SavedNewApiKey = server.arg("api_key");
    if (!server.hasArg("ark_api_key")) SavedArkApiKey = SavedNewApiKey;
    if (!server.hasArg("voice_api_key")) SavedVoiceApiKey = SavedNewApiKey;
  }
  if (server.hasArg("model_mini")) SavedModelMini = server.arg("model_mini");
  if (server.hasArg("model_lite")) SavedModelLite = server.arg("model_lite");
  if (server.hasArg("model_pro")) SavedModelPro = server.arg("model_pro");
  if (server.hasArg("model_mode")) SavedDefaultModelMode = server.arg("model_mode");
  if (server.hasArg("asr_resource")) SavedAsrResource = server.arg("asr_resource");
  if (server.hasArg("tts_public_resource")) SavedTtsPublicResource = server.arg("tts_public_resource");
  if (server.hasArg("tts_clone_resource")) SavedTtsCloneResource = server.arg("tts_clone_resource");
  if (server.hasArg("public_speaker")) SavedPublicSpeaker = server.arg("public_speaker");
  if (server.hasArg("voice_shore")) SavedVoiceShorekeeper = server.arg("voice_shore");
  if (server.hasArg("voice_cart")) SavedVoiceCartethyia = server.arg("voice_cart");
  if (server.hasArg("amap_key")) SavedAmapWeatherKey = server.arg("amap_key");
  if (server.hasArg("amap_city")) SavedAmapCityCode = server.arg("amap_city");
  if (server.hasArg("bemfa_uid")) SavedBemfaUid = server.arg("bemfa_uid");
  if (server.hasArg("bemfa_cmd")) SavedBemfaTopicCmd = server.arg("bemfa_cmd");
  if (server.hasArg("bemfa_status")) SavedBemfaTopicStatus = server.arg("bemfa_status");
  if (server.hasArg("bemfa_log")) SavedBemfaTopicLog = server.arg("bemfa_log");
  SavedArkApiKey.trim();
  SavedVoiceApiKey.trim();
  SavedNewApiKey.trim();
  SavedAmapWeatherKey.trim();
  SavedAmapCityCode.trim();
  SavedBemfaUid.trim();
  SavedBemfaTopicCmd.trim();
  SavedBemfaTopicStatus.trim();
  SavedBemfaTopicLog.trim();
  SavedDefaultModelMode.toLowerCase();
  SaveCloudConfigFromCurrent();
  if (server.hasArg("amap_key") || server.hasArg("amap_city")) {
    Weather.lastNowMs = 0;
    Weather.lastAirMs = 0;
    QWeatherFetchNow();
    QWeatherFetchAir();
  }
  if (server.hasArg("bemfa_uid") || server.hasArg("bemfa_cmd") || server.hasArg("bemfa_status") || server.hasArg("bemfa_log")) {
#if ENABLE_BEMFA_MQTT
    BemfaMqtt.disconnect();
#endif
    BemfaOnline = false;
    BemfaCmdSubscribed = false;
    BemfaCmdSetSubscribed = false;
    LastBemfaReconnectTime = 0;
    BemfaReconnect(true);
  }
  AddCenterLog("sys", "系统配置已保存：火山引擎 / 高德天气 / 巴法云 MQTT");
  HandleCloudConfigGetApi();
}


// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String JsonUnescape(String s) {
  String out;
  out.reserve(s.length());
  for (int i = 0; i < (int)s.length(); i++) {
    char c = s[i];
    if (c == '\\' && i + 1 < (int)s.length()) {
      char n = s[++i];
      if (n == 'n' || n == 'r' || n == 't') out += ' ';
      else if (n == '"') out += '"';
      else if (n == '\\') out += '\\';
      else if (n == '/') out += '/';
      else out += n;
    } else {
      out += c;
    }
  }
  return out;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String JsonStringAt(const String &json, int quotePos) {
  if (quotePos < 0 || quotePos >= (int)json.length() || json[quotePos] != '"') return "";
  String raw;
  for (int i = quotePos + 1; i < (int)json.length(); i++) {
    char c = json[i];
    if (c == '\\' && i + 1 < (int)json.length()) {
      raw += c;
      raw += json[++i];
      continue;
    }
    if (c == '"') return JsonUnescape(raw);
    raw += c;
  }
  return "";
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String JsonGetStringAfter(const String &json, const char *key, int startPos) {
  String pat = "\"" + String(key) + "\"";
  int p = json.indexOf(pat, startPos);
  if (p < 0) return "";
  p = json.indexOf(':', p + pat.length());
  if (p < 0) return "";
  p++;
  while (p < (int)json.length() && (json[p] == ' ' || json[p] == '\n' || json[p] == '\r' || json[p] == '\t')) p++;
  if (p >= (int)json.length()) return "";
  if (json[p] == '"') return JsonStringAt(json, p);
  int e = p;
  while (e < (int)json.length() && json[e] != ',' && json[e] != '}' && json[e] != ']') e++;
  String v = json.substring(p, e);
  v.trim();
  return v;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String ExtractJsonObjectFromText(String text) {
  text.trim();
  text.replace("```json", "");
  text.replace("```JSON", "");
  text.replace("```", "");
  int a = text.indexOf('{');
  int b = text.lastIndexOf('}');
  if (a >= 0 && b > a) return text.substring(a, b + 1);
  return text;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DoubaoModelByMode(String mode) {
  mode.toLowerCase();
  if (mode == "mini") return CloudModelMini();
  if (mode == "pro") return CloudModelPro();
  return CloudModelLite();
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DoubaoBuildPrompt(String userText) {
  return DoubaoBuildChatPrompt(userText);
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String DoubaoBuildChatPrompt(String userText) {
  userText = LingxiCleanUserRawText(userText);
  userText.trim();

  // 自由对话不再使用大段身份设定或设备控制提示词。
  // 前端仍显示用户原文；这里只在发给豆包前追加极简短答要求。
  if (userText.length() > 220) userText = userText.substring(0, 220);

  if (userText.indexOf("简要回答") < 0 && userText.indexOf("简单回答") < 0) {
    userText += "（简要回答）";
  }

  return userText;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DoubaoShouldUseStructuredPrompt(String userText) {
  (void)userText;
  return false;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceRouteLayerName(uint8_t layer) {
  switch (layer) {
    case ROUTE_LAYER_LOCAL_COMMAND: return "local_command";
    case ROUTE_LAYER_CONFIRM_AGENT: return "confirm_agent";
    case ROUTE_LAYER_FREE_CHAT: return "free_chat";
    default: return "none";
  }
}


