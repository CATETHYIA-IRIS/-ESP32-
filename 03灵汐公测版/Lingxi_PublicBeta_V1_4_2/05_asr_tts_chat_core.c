/*
 * 文件：05_asr_tts_chat_core.ino
 * 标题：ASR、TTS 与对话任务核心
 * 说明：负责录音识别、TTS 请求、语音任务状态和对话链路。
 * 可改：可调请求超时和输出长度
 * 禁止：泄露或硬编码真实密钥。
 */

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitVoiceState() {
  Voice.sdOnline = false;
  Voice.shorekeeperReady = false;
  Voice.cartethyiaReady = false;
  Voice.shorekeeperCount = 0;
  Voice.cartethyiaCount = 0;
  Voice.currentRole = ROLE_SHOREKEEPER;

  VoicePrefs.begin("lx_voice", true);
  String savedRole = VoicePrefs.getString("cur_role", "shorekeeper");
  VoicePrefs.end();
  savedRole.toLowerCase();
  if (savedRole.indexOf("cart") >= 0 || savedRole.indexOf("kati") >= 0) {
    Voice.currentRole = ROLE_CARTETHYIA;
  }

  Voice.audioBusy = false;
  Voice.currentVoiceId = 0;
  Voice.currentVoiceFile = "";
  Voice.lastVoiceError = "";
  Voice.lastVoiceMs = 0;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceRoleName() {
  return Voice.currentRole == ROLE_CARTETHYIA ? "卡提希娅" : "守岸人";
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoicePersistCurrentRole() {
  VoicePrefs.begin("lx_voice", false);
  VoicePrefs.putString("cur_role", Voice.currentRole == ROLE_CARTETHYIA ? "cartethyia" : "shorekeeper");
  VoicePrefs.end();
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceRoleDir(uint8_t role) {
  return role == ROLE_CARTETHYIA ? String(ROLE_CARTETHYIA_DIR) : String(ROLE_SHOREKEEPER_DIR);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
uint16_t VoiceCountWavFiles(const char* dirPath) {
#if ENABLE_SD_VOICE
  if (!Voice.sdOnline) return 0;
  File dir = SD_MMC.open(dirPath);
  if (!dir || !dir.isDirectory()) return 0;
  uint16_t count = 0;
  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String name = String(f.name());
      name.toLowerCase();
      if (name.endsWith(".wav")) count++;
    }
    f.close();
    f = dir.openNextFile();
  }
  dir.close();
  return count;
#else
  return 0;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceInitSD() {
#if ENABLE_SD_VOICE
  Voice.sdOnline = false;
  Voice.shorekeeperReady = false;
  Voice.cartethyiaReady = false;
  Voice.shorekeeperCount = 0;
  Voice.cartethyiaCount = 0;
  Voice.lastVoiceError = "";

  if (!BoardIoExtInitForSd()) {
    Voice.lastVoiceError = "IO Extension init failed / SD_CS not enabled";
    Serial.println("[Voice] IO Extension init failed, SD_CS may not be enabled.");
    return false;
  }

  if (!SD_MMC.setPins(SDMMC_CLK_PIN, SDMMC_CMD_PIN, SDMMC_D0_PIN)) {
    Voice.lastVoiceError = "SD_MMC setPins failed";
    Serial.println("[Voice] SD_MMC setPins failed.");
    return false;
  }

  if (!SD_MMC.begin(VOICE_MOUNT_POINT, true, false)) {
    Voice.lastVoiceError = "SD_MMC mount failed: check FAT32 / SD_CS / card slot";
    Serial.println("[Voice] SD_MMC mount failed.");
    return false;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Voice.lastVoiceError = "No SD card";
    Serial.println("[Voice] No SD card detected.");
    return false;
  }

  Voice.sdOnline = true;
  Voice.shorekeeperReady = SD_MMC.exists(ROLE_SHOREKEEPER_DIR);
  Voice.cartethyiaReady = SD_MMC.exists(ROLE_CARTETHYIA_DIR);
  Voice.shorekeeperCount = VoiceCountWavFiles(ROLE_SHOREKEEPER_DIR);
  Voice.cartethyiaCount = VoiceCountWavFiles(ROLE_CARTETHYIA_DIR);
  RoleLoadAssets();

  Serial.printf("[Voice] SD OK. shorekeeper=%d, cartethyia=%d\n", Voice.shorekeeperCount, Voice.cartethyiaCount);
  return true;
#else
  Voice.lastVoiceError = "ENABLE_SD_VOICE=0";
  return false;
#endif
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleIdByIndex(uint8_t role) {
  return role == ROLE_CARTETHYIA ? "cartethyia" : "shorekeeper";
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleDisplayNameByIndex(uint8_t role) {
  return role == ROLE_CARTETHYIA ? "卡提希娅" : "守岸人";
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleAssetDirByIndex(uint8_t role) {
  return role == ROLE_CARTETHYIA ? String(ROLE_CARTETHYIA_ASSET_DIR) : String(ROLE_SHOREKEEPER_ASSET_DIR);
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleJsonPathByIndex(uint8_t role) {
  return RoleAssetDirByIndex(role) + "/role.json";
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleAssetPathByIndex(uint8_t role, String type) {
  type.toLowerCase();
  if (type != "avatar" && type != "portrait" && type != "icon") type = "avatar";
  return RoleAssetDirByIndex(role) + "/" + type + ".webp";
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleAssetUrlByIndex(uint8_t role, String type) {
  return "/api/role_asset?role=" + RoleIdByIndex(role) + "&type=" + type + "&v=6";
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String SdReadSmallTextFile(String path, size_t limitBytes) {
#if ENABLE_SD_VOICE
  if (!Voice.sdOnline) return "";
  File f = SD_MMC.open(path.c_str(), FILE_READ);
  if (!f) return "";
  String out = "";
  size_t n = 0;
  while (f.available() && n < limitBytes) {
    out += (char)f.read();
    n++;
  }
  f.close();
  return out;
#else
  (void)path;
  (void)limitBytes;
  return "";
#endif
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String JsonFieldLoose(String json, String key, String fallback) {
  if (json.length() == 0 || key.length() == 0) return fallback;
  String mark = "\"" + key + "\"";
  int p = json.indexOf(mark);
  if (p < 0) return fallback;
  p = json.indexOf(':', p + mark.length());
  if (p < 0) return fallback;
  p++;
  while (p < (int)json.length() && (json[p] == ' ' || json[p] == '\r' || json[p] == '\n' || json[p] == '\t')) p++;
  if (p >= (int)json.length()) return fallback;
  if (json[p] == '"') {
    p++;
    String v = "";
    bool esc = false;
    while (p < (int)json.length()) {
      char c = json[p++];
      if (esc) {
        if (c == 'n') v += '\n';
        else if (c == 'r') v += '\r';
        else if (c == 't') v += '\t';
        else v += c;
        esc = false;
      } else if (c == '\\') {
        esc = true;
      } else if (c == '"') {
        break;
      } else {
        v += c;
      }
    }
    return v.length() ? v : fallback;
  }
  int e = p;
  while (e < (int)json.length() && json[e] != ',' && json[e] != '}') e++;
  String v = json.substring(p, e);
  v.trim();
  return v.length() ? v : fallback;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool RoleAssetFileReady(uint8_t role, String type) {
  type.toLowerCase();
  if (role == ROLE_CARTETHYIA) {
    if (type == "portrait") return RoleAssetCartethyiaPortraitReady;
    if (type == "icon") return RoleAssetCartethyiaIconReady;
    return RoleAssetCartethyiaAvatarReady;
  }
  if (type == "portrait") return RoleAssetShorekeeperPortraitReady;
  if (type == "icon") return RoleAssetShorekeeperIconReady;
  return RoleAssetShorekeeperAvatarReady;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void RoleLoadAssets() {
#if ENABLE_SD_VOICE
  RoleAssetLastError = "";
  RoleAssetLastLoadMs = millis();

  RoleAssetShorekeeperJsonReady = Voice.sdOnline && SD_MMC.exists(RoleJsonPathByIndex(ROLE_SHOREKEEPER).c_str());
  RoleAssetCartethyiaJsonReady = Voice.sdOnline && SD_MMC.exists(RoleJsonPathByIndex(ROLE_CARTETHYIA).c_str());

  RoleAssetShorekeeperAvatarReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathByIndex(ROLE_SHOREKEEPER, "avatar").c_str());
  RoleAssetShorekeeperPortraitReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathByIndex(ROLE_SHOREKEEPER, "portrait").c_str());
  RoleAssetShorekeeperIconReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathByIndex(ROLE_SHOREKEEPER, "icon").c_str());

  RoleAssetCartethyiaAvatarReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathByIndex(ROLE_CARTETHYIA, "avatar").c_str());
  RoleAssetCartethyiaPortraitReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathByIndex(ROLE_CARTETHYIA, "portrait").c_str());
  RoleAssetCartethyiaIconReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathByIndex(ROLE_CARTETHYIA, "icon").c_str());

  if (Voice.sdOnline) {
    Serial.printf("[RoleAsset] shore json=%d avatar=%d portrait=%d icon=%d / cart json=%d avatar=%d portrait=%d icon=%d\n",
      RoleAssetShorekeeperJsonReady, RoleAssetShorekeeperAvatarReady, RoleAssetShorekeeperPortraitReady, RoleAssetShorekeeperIconReady,
      RoleAssetCartethyiaJsonReady, RoleAssetCartethyiaAvatarReady, RoleAssetCartethyiaPortraitReady, RoleAssetCartethyiaIconReady);
  }
#else
  RoleAssetLastError = "SD voice disabled";
#endif
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleOneStatusJson(uint8_t role) {
  String id = RoleIdByIndex(role);
  String jsonText = SdReadSmallTextFile(RoleJsonPathByIndex(role), 4096);
  String name = JsonFieldLoose(jsonText, "name", RoleDisplayNameByIndex(role));
  String voiceId = JsonFieldLoose(jsonText, "voice_id", role == ROLE_CARTETHYIA ? CloudVoiceCartethyia() : CloudVoiceShorekeeper());
  String ttsResource = JsonFieldLoose(jsonText, "tts_resource", CloudTtsCloneResource());
  String voiceDir = JsonFieldLoose(jsonText, "voice_dir", VoiceRoleDir(role));

  String j = "{";
  j += "\"id\":\"" + JsonEscape(id) + "\",";
  j += "\"name\":\"" + JsonEscape(name) + "\",";
  j += "\"voice_id\":\"" + JsonEscape(voiceId) + "\",";
  j += "\"tts_resource\":\"" + JsonEscape(ttsResource) + "\",";
  j += "\"voice_dir\":\"" + JsonEscape(voiceDir) + "\",";
  j += "\"role_json\":" + String(role == ROLE_CARTETHYIA ? (RoleAssetCartethyiaJsonReady ? "true" : "false") : (RoleAssetShorekeeperJsonReady ? "true" : "false")) + ",";
  j += "\"avatar_ready\":" + String(RoleAssetFileReady(role, "avatar") ? "true" : "false") + ",";
  j += "\"portrait_ready\":" + String(RoleAssetFileReady(role, "portrait") ? "true" : "false") + ",";
  j += "\"icon_ready\":" + String(RoleAssetFileReady(role, "icon") ? "true" : "false") + ",";
  j += "\"avatar\":\"" + JsonEscape(RoleAssetUrlByIndex(role, "avatar")) + "\",";
  j += "\"portrait\":\"" + JsonEscape(RoleAssetUrlByIndex(role, "portrait")) + "\",";
  j += "\"icon\":\"" + JsonEscape(RoleAssetUrlByIndex(role, "icon")) + "\"";
  j += "}";
  return j;
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool RoleIdSafe(String id) {
  id.trim();
  if (id.length() < 1 || id.length() > 40) return false;
  for (uint16_t i = 0; i < id.length(); i++) {
    char c = id[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    if (!ok) return false;
  }
  return true;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleIdNormalize(String id, String fallback) {
  id.trim();
  id.toLowerCase();
  if (!RoleIdSafe(id)) return fallback;
  return id;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleAssetDirById(String id) {
  id = RoleIdNormalize(id, "shorekeeper");
  return String(ROLE_ASSET_ROOT_DIR) + "/" + id;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleAssetPathById(String id, String type) {
  type.toLowerCase();
  if (type != "avatar" && type != "portrait" && type != "icon") type = "avatar";
  return RoleAssetDirById(id) + "/" + type + ".webp";
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleAssetUrlById(String id, String type) {
  id = RoleIdNormalize(id, "shorekeeper");
  type.toLowerCase();
  if (type != "avatar" && type != "portrait" && type != "icon") type = "avatar";
  return "/api/role_asset?role=" + id + "&type=" + type + "&v=6";
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleLibraryOneJson(String id) {
  id = RoleIdNormalize(id, "shorekeeper");
  String dir = RoleAssetDirById(id);
  String jsonPath = dir + "/role.json";
  String jsonText = SdReadSmallTextFile(jsonPath, 4096);
  String name = JsonFieldLoose(jsonText, "name", id == "shorekeeper" ? "守岸人" : (id == "cartethyia" ? "卡提希娅" : id));
  String voiceId = JsonFieldLoose(jsonText, "voice_id", id == "shorekeeper" ? CloudVoiceShorekeeper() : (id == "cartethyia" ? CloudVoiceCartethyia() : ""));
  String ttsResource = JsonFieldLoose(jsonText, "tts_resource", CloudTtsCloneResource());
  String voiceDir = JsonFieldLoose(jsonText, "voice_dir", String("/voice/") + id);
  bool avatarReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathById(id, "avatar").c_str());
  bool portraitReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathById(id, "portrait").c_str());
  bool iconReady = Voice.sdOnline && SD_MMC.exists(RoleAssetPathById(id, "icon").c_str());
  bool jsonReady = Voice.sdOnline && SD_MMC.exists(jsonPath.c_str());
  String j = "{";
  j += "\"id\":\"" + JsonEscape(id) + "\",";
  j += "\"name\":\"" + JsonEscape(name) + "\",";
  j += "\"voice_id\":\"" + JsonEscape(voiceId) + "\",";
  j += "\"tts_resource\":\"" + JsonEscape(ttsResource) + "\",";
  j += "\"voice_dir\":\"" + JsonEscape(voiceDir) + "\",";
  j += "\"role_json\":" + String(jsonReady ? "true" : "false") + ",";
  j += "\"avatar_ready\":" + String(avatarReady ? "true" : "false") + ",";
  j += "\"portrait_ready\":" + String(portraitReady ? "true" : "false") + ",";
  j += "\"icon_ready\":" + String(iconReady ? "true" : "false") + ",";
  j += "\"avatar\":\"" + JsonEscape(RoleAssetUrlById(id, "avatar")) + "\",";
  j += "\"portrait\":\"" + JsonEscape(RoleAssetUrlById(id, "portrait")) + "\",";
  j += "\"icon\":\"" + JsonEscape(RoleAssetUrlById(id, "icon")) + "\"";
  j += "}";
  return j;
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleAssetsLibraryJson() {
  String out = "{";
  out += "\"sdOnline\":" + String(Voice.sdOnline ? "true" : "false") + ",";
  out += "\"roles\":[";
  bool first = true;
#if ENABLE_SD_VOICE
  if (Voice.sdOnline) {
    File dir = SD_MMC.open(ROLE_ASSET_ROOT_DIR);
    if (dir && dir.isDirectory()) {
      File f = dir.openNextFile();
      uint8_t count = 0;
      while (f && count < 16) {
        if (f.isDirectory()) {
          String id = VoiceBasename(String(f.name()));
          id = RoleIdNormalize(id, "");
          if (id.length()) {
            if (!first) out += ",";
            out += RoleLibraryOneJson(id);
            first = false;
            count++;
          }
        }
        f.close();
        f = dir.openNextFile();
      }
      dir.close();
    }
  }
#endif
  if (first) {
    out += RoleOneStatusJson(ROLE_SHOREKEEPER);
    out += ",";
    out += RoleOneStatusJson(ROLE_CARTETHYIA);
  }
  out += "],";
  out += "\"lastError\":\"" + JsonEscape(RoleAssetLastError) + "\"";
  out += "}";
  return out;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleRoleLibraryApi() {
  SendCorsHeader();
  RoleLoadAssets();
  server.send(200, "application/json", RoleAssetsLibraryJson());
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool TryServeRoleAssetById(String id, String type) {
#if ENABLE_SD_VOICE
  if (!Voice.sdOnline) return false;
  id = RoleIdNormalize(id, "shorekeeper");
  type.toLowerCase();
  if (type != "avatar" && type != "portrait" && type != "icon") type = "avatar";
  String path = RoleAssetPathById(id, type);
  if (!SD_MMC.exists(path.c_str())) return false;
  File f = SD_MMC.open(path.c_str(), FILE_READ);
  if (!f) return false;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "public, max-age=604800");
  server.streamFile(f, "image/webp");
  f.close();
  return true;
#else
  (void)id;
  (void)type;
  return false;
#endif
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String RoleAssetsStatusJson() {
  String j = "{";
  j += "\"sdOnline\":" + String(Voice.sdOnline ? "true" : "false") + ",";
  j += "\"currentRole\":\"" + RoleIdByIndex(Voice.currentRole) + "\",";
  j += "\"roles\":[" + RoleOneStatusJson(ROLE_SHOREKEEPER) + "," + RoleOneStatusJson(ROLE_CARTETHYIA) + "],";
  j += "\"lastError\":\"" + JsonEscape(RoleAssetLastError) + "\"";
  j += "}";
  return j;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleRoleStatusApi() {
  SendCorsHeader();
  server.send(200, "application/json", RoleAssetsStatusJson());
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool TryServeRoleAsset(uint8_t role, String type) {
#if ENABLE_SD_VOICE
  if (!Voice.sdOnline) return false;
  type.toLowerCase();
  if (type != "avatar" && type != "portrait" && type != "icon") type = "avatar";
  String path = RoleAssetPathByIndex(role, type);
  if (!SD_MMC.exists(path.c_str())) return false;
  File f = SD_MMC.open(path.c_str(), FILE_READ);
  if (!f) return false;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "public, max-age=604800");
  server.streamFile(f, "image/webp");
  f.close();
  return true;
#else
  (void)role;
  (void)type;
  return false;
#endif
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleRoleAssetApi() {
  String roleId = "shorekeeper";
  if (server.hasArg("role")) {
    roleId = RoleIdNormalize(server.arg("role"), "shorekeeper");
  }
  String type = server.hasArg("type") ? server.arg("type") : "avatar";
  type.toLowerCase();
  if (TryServeRoleAssetById(roleId, type)) return;
  uint8_t fallbackRole = (roleId.indexOf("cart") >= 0 || roleId.indexOf("kati") >= 0) ? ROLE_CARTETHYIA : ROLE_SHOREKEEPER;
  if (TryServeRoleAsset(fallbackRole, type)) return;
  SendRoleSvg(roleId.c_str(), fallbackRole == ROLE_CARTETHYIA ? "#f1d58a" : "#3ba7ff", fallbackRole == ROLE_CARTETHYIA ? "#416bff" : "#10295b");
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceReload() {
  bool ok = VoiceInitSD();
  if (ok) {
    AddCenterLog("sys", "SD资源已加载：守岸人语音 " + String(Voice.shorekeeperCount) + " 条，卡提希娅语音 " + String(Voice.cartethyiaCount) + " 条；角色图片 " + String((RoleAssetShorekeeperAvatarReady || RoleAssetShorekeeperPortraitReady || RoleAssetCartethyiaAvatarReady || RoleAssetCartethyiaPortraitReady) ? "已就绪" : "未检测到"));
  } else {
    AddCenterLog("sys", "SD语音包加载失败：" + Voice.lastVoiceError);
  }
  BemfaPublishStatus();
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceBasename(String path) {
  int slash = path.lastIndexOf('/');
  if (slash >= 0) return path.substring(slash + 1);
  return path;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceFindFileById(uint16_t id, uint8_t role) {
#if ENABLE_SD_VOICE
  if (!Voice.sdOnline) return "";
  char prefix[8];
  snprintf(prefix, sizeof(prefix), "%03u_", id);
  String dirPath = VoiceRoleDir(role);
  File dir = SD_MMC.open(dirPath.c_str());
  if (!dir || !dir.isDirectory()) return "";

  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String name = VoiceBasename(String(f.name()));
      if (name.startsWith(prefix) && name.endsWith(".wav")) {
        String full = dirPath + "/" + name;
        f.close();
        dir.close();
        return full;
      }
    }
    f.close();
    f = dir.openNextFile();
  }
  dir.close();
#endif
  return "";
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceFindFileByIdInDir(uint16_t id, String dirPath) {
#if ENABLE_SD_VOICE
  if (!Voice.sdOnline || dirPath.length() == 0) return "";
  char prefix[8];
  snprintf(prefix, sizeof(prefix), "%03u_", id);
  File dir = SD_MMC.open(dirPath.c_str());
  if (!dir || !dir.isDirectory()) return "";
  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String name = VoiceBasename(String(f.name()));
      String lower = name;
      lower.toLowerCase();
      if (name.startsWith(prefix) && lower.endsWith(".wav")) {
        String full = dirPath + "/" + name;
        f.close();
        dir.close();
        return full;
      }
    }
    f.close();
    f = dir.openNextFile();
  }
  dir.close();
#endif
  return "";
}


