/*
 * 文件：12_device_api_handlers_core.ino
 * 标题：设备 API 处理器
 * 说明：负责 /api/status、设备控制、节点状态和设备页面接口。
 * 可改：可补充兼容字段
 * 禁止：删除旧前端依赖字段。
 */

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V4VoiceClearLastInteraction(String source) {
  V4Voice.lastWakeSource = source;
  V4Voice.lastText = "";
  V4Voice.lastIntent = "";
  V4Voice.lastCommand = "";
  V4Voice.lastReply = "";
  V4Voice.lastError = "";
  V4Voice.updatedMs = millis();
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V4QueueRequest(String type, String text, uint32_t seconds, String source) {
  if (!V4ReqMutex) return;
  xSemaphoreTake(V4ReqMutex, portMAX_DELAY);
  V4Req.pending = true;
  V4Req.type = type;
  V4Req.text = text;
  V4Req.seconds = seconds;
  V4Req.source = source;
  xSemaphoreGive(V4ReqMutex);

  
  
  if (type == "wake" || type == "record") {
    V4VoiceClearLastInteraction(source);
    V4VoiceSetState(V4_VOICE_IDLE, "新本机语音会话已入队，等待录音");
  }
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4TakeQueuedRequest(String &type, String &text, uint32_t &seconds, String &source) {
  if (!V4ReqMutex) return false;
  bool has = false;
  xSemaphoreTake(V4ReqMutex, portMAX_DELAY);
  if (V4Req.pending) {
    has = true;
    type = V4Req.type;
    text = V4Req.text;
    seconds = V4Req.seconds;
    source = V4Req.source;
    V4Req.pending = false;
  }
  xSemaphoreGive(V4ReqMutex);
  return has;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitV4LocalVoiceState() {
  V4Voice.state = V4_VOICE_IDLE;
  V4Voice.enabled = ENABLE_LINGXI_V4_LOCAL_VOICE;
  V4Voice.wakeReady = false;       
  V4Voice.streamAsrReady = true;   
  V4Voice.busy = false;
  V4Voice.lastWakeSource = "boot";
  V4Voice.lastText = "";
  V4Voice.lastIntent = "";
  V4Voice.lastCommand = "";
  V4Voice.lastReply = "";
  V4Voice.lastError = "";
  V4Voice.updatedMs = millis();
  V4Req.pending = false;
  V4Req.type = "";
  V4Req.text = "";
  V4Req.seconds = LINGXI_V4_RECORD_SECONDS;
  V4Req.source = "boot";
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String V4VoiceStateName(uint8_t state) {
  switch (state) {
    case V4_VOICE_IDLE: return "idle";
    case V4_VOICE_WAKE_DETECTED: return "wake_detected";
    case V4_VOICE_WAKE_REPLY: return "wake_reply";
    case V4_VOICE_RECORDING: return "recording";
    case V4_VOICE_ASR_RUNNING: return "asr_running";
    case V4_VOICE_INTENT_ROUTING: return "intent_routing";
    case V4_VOICE_COMMAND_EXECUTING: return "command_executing";
    case V4_VOICE_CHAT_RUNNING: return "chat_running";
    case V4_VOICE_TTS_PLAYING: return "tts_playing";
    case V4_VOICE_ERROR: return "error";
    default: return "unknown";
  }
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V4VoiceSetState(uint8_t state, String note) {
  V4Voice.state = state;
  V4Voice.updatedMs = millis();
  if (note.length()) AddCenterLog("op", "V4本机语音：" + V4VoiceStateName(state) + " / " + note);
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String V4VoiceStatusJson() {
  String json = "{";
  json += "\"ok\":true,";
  json += "\"version\":\"" + String(LINGXI_VERSION_CODE) + "\",";
  json += "\"enabled\":" + String(V4Voice.enabled ? "true" : "false") + ",";
  json += "\"state\":\"" + JsonEscape(V4VoiceStateName(V4Voice.state)) + "\",";
  json += "\"busy\":" + String(V4Voice.busy ? "true" : "false") + ",";
  json += "\"wake_ready\":" + String(V4Voice.wakeReady ? "true" : "false") + ",";
  json += "\"stream_asr_ready\":" + String(V4Voice.streamAsrReady ? "true" : "false") + ",";
  json += "\"wake_source\":\"" + JsonEscape(V4Voice.lastWakeSource) + "\",";
  json += "\"text\":\"" + JsonEscape(V4Voice.lastText) + "\",";
  json += "\"intent\":\"" + JsonEscape(V4Voice.lastIntent) + "\",";
  json += "\"command\":\"" + JsonEscape(V4Voice.lastCommand) + "\",";
  json += "\"reply\":\"" + JsonEscape(V4Voice.lastReply) + "\",";
  json += "\"error\":\"" + JsonEscape(V4Voice.lastError) + "\",";
  json += "\"updated_ms\":" + String(V4Voice.updatedMs) + ",";
  json += "\"voice_task\":" + VoiceTaskStatusJson();
  json += "}";
  return json;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4VoicePlayWakeReply() {
  V4VoiceSetState(V4_VOICE_WAKE_REPLY, "播放短唤醒提示");
  
  
  
  
  bool ok = VoiceRequestPlayOnBoard(LINGXI_V4_WAKE_REPLY_VOICE_ID, "V4WakeReply");
  if (!ok && LINGXI_V4_WAKE_REPLY_VOICE_ID != 2) {
    ok = VoiceRequestPlayOnBoard(2, "V4WakeReplyFallback");
  }
  if (!ok) {
    AddCenterLog("sys", "V4唤醒提示未播放，继续进入录音：" + Voice.lastVoiceError);
  }
  return true;
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String V4StreamAsrResource() {
  String r = CloudAsrResource();
  r.trim();
  if (r.indexOf("sauc") >= 0) return r;
  return String(LINGXI_STREAM_ASR_RESOURCE_FALLBACK);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String V4RandomRequestId() {
  String s = "lx-";
  s += String((uint32_t)millis(), HEX);
  s += "-";
  s += String((uint32_t)esp_random(), HEX);
  s += "-";
  s += String((uint32_t)esp_random(), HEX);
  return s;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String V4WebSocketKey() {
  uint8_t raw[16];
  for (int i = 0; i < 16; i++) raw[i] = (uint8_t)(esp_random() & 0xFF);
  unsigned char out[32];
  size_t olen = 0;
  mbedtls_base64_encode(out, sizeof(out), &olen, raw, sizeof(raw));
  String s = "";
  for (size_t i = 0; i < olen; i++) s += (char)out[i];
  return s;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void V4PutU32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)((v >> 24) & 0xFF);
  p[1] = (uint8_t)((v >> 16) & 0xFF);
  p[2] = (uint8_t)((v >> 8) & 0xFF);
  p[3] = (uint8_t)(v & 0xFF);
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4WsSendFrame(WiFiClientSecure &client, uint8_t opcode, const uint8_t *payload, size_t len) {
  if (!client.connected()) return false;

  uint8_t hdr[14];
  size_t h = 0;
  hdr[h++] = 0x80 | (opcode & 0x0F);

  if (len < 126) {
    hdr[h++] = 0x80 | (uint8_t)len;
  } else if (len <= 65535) {
    hdr[h++] = 0x80 | 126;
    hdr[h++] = (uint8_t)((len >> 8) & 0xFF);
    hdr[h++] = (uint8_t)(len & 0xFF);
  } else {
    hdr[h++] = 0x80 | 127;
    uint64_t n64 = (uint64_t)len;
    for (int i = 7; i >= 0; i--) hdr[h++] = (uint8_t)((n64 >> (8 * i)) & 0xFF);
  }

  uint8_t mask[4];
  for (int i = 0; i < 4; i++) {
    mask[i] = (uint8_t)(esp_random() & 0xFF);
    hdr[h++] = mask[i];
  }

  if (client.write(hdr, h) != h) return false;

  const size_t CHUNK = 512;
  uint8_t buf[CHUNK];
  size_t pos = 0;
  while (pos < len) {
    size_t n = len - pos;
    if (n > CHUNK) n = CHUNK;
    for (size_t i = 0; i < n; i++) buf[i] = payload[pos + i] ^ mask[(pos + i) & 3];
    if (client.write(buf, n) != n) return false;
    pos += n;
  }

  return true;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4WsReadFrame(WiFiClientSecure &client, String &out, uint32_t timeoutMs) {
  out = "";
  uint32_t start = millis();

  while (client.connected() && client.available() < 2 && millis() - start < timeoutMs) delay(5);
  if (!client.connected() || client.available() < 2) return false;

  uint8_t h1 = (uint8_t)client.read();
  uint8_t h2 = (uint8_t)client.read();
  uint8_t opcode = h1 & 0x0F;
  bool masked = (h2 & 0x80) != 0;
  uint64_t len = h2 & 0x7F;

  if (len == 126) {
    while (client.available() < 2 && millis() - start < timeoutMs) delay(2);
    if (client.available() < 2) return false;
    len = ((uint16_t)((uint8_t)client.read()) << 8) | (uint8_t)client.read();
  } else if (len == 127) {
    while (client.available() < 8 && millis() - start < timeoutMs) delay(2);
    if (client.available() < 8) return false;
    len = 0;
    for (int i = 0; i < 8; i++) len = (len << 8) | (uint8_t)client.read();
  }

  uint8_t mask[4] = {0, 0, 0, 0};
  if (masked) {
    while (client.available() < 4 && millis() - start < timeoutMs) delay(2);
    if (client.available() < 4) return false;
    for (int i = 0; i < 4; i++) mask[i] = (uint8_t)client.read();
  }

  if (len > 16384) {
    for (uint64_t i = 0; i < len && client.connected(); i++) {
      while (!client.available() && millis() - start < timeoutMs) delay(1);
      if (client.available()) client.read();
    }
    return false;
  }

  uint8_t *buf = (uint8_t*)malloc((size_t)len + 1);
  if (!buf) return false;

  size_t got = 0;
  while (got < len && millis() - start < timeoutMs) {
    if (client.available()) {
      uint8_t b = (uint8_t)client.read();
      if (masked) b ^= mask[got & 3];
      buf[got++] = b;
    } else {
      delay(1);
    }
  }

  if (got < len) {
    free(buf);
    return false;
  }
  buf[len] = 0;

  if (opcode == 0x8) {
    free(buf);
    return false;
  }

  if (opcode == 0x9) {
    V4WsSendFrame(client, 0xA, buf, (size_t)len);
    free(buf);
    return false;
  }

  if (opcode == 0x1) {
    out = String((char*)buf);
    free(buf);
    return true;
  }

  if (opcode == 0x2) {
    int jsonPos = -1;
    for (size_t i = 0; i < (size_t)len; i++) {
      if (buf[i] == '{' || buf[i] == '[') {
        jsonPos = (int)i;
        break;
      }
    }

    if (jsonPos >= 0) {
      out = String((char*)buf + jsonPos);
      free(buf);
      return true;
    }

    if (len >= 8) {
      uint8_t headerSize = (buf[0] & 0x0F) * 4;
      uint8_t flags = buf[1] & 0x0F;
      size_t p = headerSize;

      if ((flags == 0x01 || flags == 0x02 || flags == 0x03) && p + 4 <= (size_t)len) p += 4;
      if (p + 4 <= (size_t)len) {
        uint32_t sz = ((uint32_t)buf[p] << 24) | ((uint32_t)buf[p + 1] << 16) | ((uint32_t)buf[p + 2] << 8) | buf[p + 3];
        p += 4;
        if (p <= (size_t)len && sz <= (uint32_t)((size_t)len - p)) {
          buf[p + sz] = 0;
          out = String((char*)buf + p);
          free(buf);
          return out.length() > 0;
        }
      }
    }
  }

  free(buf);
  return false;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4SaucSendFullRequest(WiFiClientSecure &client, String reqid) {
  String body = "{";
  body += "\"user\":{\"uid\":\"lingxi-center\"},";
  body += "\"audio\":{\"format\":\"pcm\",\"codec\":\"raw\",\"rate\":16000,\"bits\":16,\"channel\":1},";
  body += "\"request\":{\"model_name\":\"bigmodel\",\"enable_itn\":true,\"enable_punc\":true,\"enable_ddc\":true,\"show_utterances\":true}";
  body += "}";

  size_t payloadLen = body.length();
  uint8_t *pkt = (uint8_t*)malloc(8 + payloadLen);
  if (!pkt) return false;

  pkt[0] = 0x11;
  pkt[1] = 0x10;
  pkt[2] = 0x10;
  pkt[3] = 0x00;
  V4PutU32(pkt + 4, payloadLen);
  memcpy(pkt + 8, body.c_str(), payloadLen);

  bool ok = V4WsSendFrame(client, 0x2, pkt, 8 + payloadLen);
  free(pkt);
  return ok;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4SaucSendAudioPacket(WiFiClientSecure &client, const uint8_t *audio, size_t len, int32_t seq, bool last) {
  size_t total = 12 + len;
  uint8_t *pkt = (uint8_t*)malloc(total);
  if (!pkt) return false;

  pkt[0] = 0x11;
  pkt[1] = last ? 0x22 : 0x21;
  pkt[2] = 0x00;
  pkt[3] = 0x00;

  uint32_t s = last ? (uint32_t)(-seq) : (uint32_t)seq;
  V4PutU32(pkt + 4, s);
  V4PutU32(pkt + 8, (uint32_t)len);
  if (len > 0 && audio) memcpy(pkt + 12, audio, len);

  bool ok = V4WsSendFrame(client, 0x2, pkt, total);
  free(pkt);
  return ok;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String V4SaucExtractText(String payload) {
  payload.trim();
  if (!payload.length()) return "";

  String t = JsonGetStringAfter(payload, "text", 0);
  if (t.length()) return t;

  t = JsonGetStringAfter(payload, "utterance", 0);
  if (t.length()) return t;

  t = JsonGetStringAfter(payload, "result", 0);
  if (t.length()) return t;

  t = JsonGetStringAfter(payload, "sentence", 0);
  if (t.length()) return t;

  return "";
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4BoardMicReadPcmChunk(uint8_t *outPcm, size_t maxBytes, uint32_t targetMs, size_t &outBytes, int &chunkPeak, int &chunkRms) {
  outBytes = 0;
  chunkPeak = 0;
  chunkRms = 0;
  if (!outPcm || maxBytes < 640) return false;

  uint8_t* rxBuf = (uint8_t*)heap_caps_malloc(BOARD_AUDIO_BUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!rxBuf) rxBuf = (uint8_t*)malloc(BOARD_AUDIO_BUF_SIZE);
  if (!rxBuf) return false;

  uint32_t startMs = millis();
  uint64_t sumSq = 0;
  uint32_t sampleCount = 0;
  size_t maxSamples = maxBytes / 2;

  while (millis() - startMs < targetMs && outBytes < maxBytes) {
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(AUDIO_I2S_PORT, rxBuf, BOARD_AUDIO_BUF_SIZE, &bytesRead, 120 / portTICK_PERIOD_MS);
    if (err != ESP_OK || bytesRead == 0) {
      delay(1);
      continue;
    }

    i2s_bits_per_sample_t activeBits = VoiceMicI2SBits();
    size_t frames = 0;
    int16_t *out16 = (int16_t*)outPcm;
    size_t outSamples = outBytes / 2;

    if (activeBits == I2S_BITS_PER_SAMPLE_16BIT) {
      int16_t* samples16 = (int16_t*)rxBuf;
      size_t sample16Count = bytesRead / 2;

      if (VoiceMicDecodeMode == 7) {
        frames = sample16Count;
        for (size_t i = 0; i < frames && outSamples < maxSamples; i++) {
          int32_t amplified = (int32_t)samples16[i] * VOICE_RECORD_SOFTWARE_GAIN;
          if (amplified > 32767) amplified = 32767;
          if (amplified < -32768) amplified = -32768;

          out16[outSamples++] = (int16_t)amplified;

          int32_t a = amplified < 0 ? -amplified : amplified;
          if (a > chunkPeak) chunkPeak = a;
          sumSq += (uint32_t)a * (uint32_t)a;
          sampleCount++;
        }
      } else {
        frames = sample16Count / 2;
        for (size_t i = 0; i < frames && outSamples < maxSamples; i++) {
          int32_t left = samples16[i * 2];
          int32_t right = samples16[i * 2 + 1];
          int32_t sample = (LingxiAbs32(right) > LingxiAbs32(left)) ? right : left;
          int32_t amplified = sample * VOICE_RECORD_SOFTWARE_GAIN;

          if (amplified > 32767) amplified = 32767;
          if (amplified < -32768) amplified = -32768;

          out16[outSamples++] = (int16_t)amplified;

          int32_t a = amplified < 0 ? -amplified : amplified;
          if (a > chunkPeak) chunkPeak = a;
          sumSq += (uint32_t)a * (uint32_t)a;
          sampleCount++;
        }
      }
    } else {
      int32_t* samples32 = (int32_t*)rxBuf;
      size_t sample32Count = bytesRead / 4;
      frames = sample32Count / 2;

      for (size_t i = 0; i < frames && outSamples < maxSamples; i++) {
        int32_t left32 = samples32[i * 2];
        int32_t right32 = samples32[i * 2 + 1];
        int32_t left = 0;
        int32_t right = 0;

        if (VoiceMicDecodeMode == 2) {
          left = (int16_t)(left32 & 0xFFFF);
          right = (int16_t)(right32 & 0xFFFF);
        } else if (VoiceMicDecodeMode == 3) {
          left = (int16_t)((left32 >> 8) & 0xFFFF);
          right = (int16_t)((right32 >> 8) & 0xFFFF);
        } else if (VoiceMicDecodeMode == 4) {
          left = ((int32_t)LingxiBswap32((uint32_t)left32)) >> 16;
          right = ((int32_t)LingxiBswap32((uint32_t)right32)) >> 16;
        } else {
          left = left32 >> 16;
          right = right32 >> 16;
        }

        int32_t sample = left;
        if (VoiceMicDecodeMode == 6) sample = right;
        else if (VoiceMicDecodeMode != 5) sample = (LingxiAbs32(right) > LingxiAbs32(left)) ? right : left;

        int32_t amplified = sample * VOICE_RECORD_SOFTWARE_GAIN;
        if (amplified > 32767) amplified = 32767;
        if (amplified < -32768) amplified = -32768;

        out16[outSamples++] = (int16_t)amplified;

        int32_t a = amplified < 0 ? -amplified : amplified;
        if (a > chunkPeak) chunkPeak = a;
        sumSq += (uint32_t)a * (uint32_t)a;
        sampleCount++;
      }
    }

    outBytes = outSamples * 2;
  }

  free(rxBuf);

  if (sampleCount > 0) chunkRms = (int)sqrt((double)sumSq / (double)sampleCount);
  return outBytes > 0;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4TrueStreamAsrToText(String &outText, String source) {
#if ENABLE_LINGXI_TRUE_STREAM_ASR
  (void)source;
  outText = "";

  LastDoubaoAsrState = "stream_connecting";
  LastDoubaoAsrError = "";
  LastDoubaoAsrText = "";
  LastDoubaoAsrEndpoint = String("wss://") + LINGXI_STREAM_ASR_HOST + LINGXI_STREAM_ASR_PATH;
  LastDoubaoAsrResource = V4StreamAsrResource();
  LastDoubaoAsrHttpCode = 0;
  LastDoubaoAsrElapsedMs = 0;

  if (WiFi.status() != WL_CONNECTED) {
    LastDoubaoAsrError = "STA WiFi 未连接，无法启动录音文件ASR";
    return false;
  }

  if (IsPlaceholderText(CloudVoiceApiKey())) {
    LastDoubaoAsrError = "语音接口密钥未填写，无法启动录音文件ASR";
    return false;
  }

  uint32_t t0 = millis();
  String reqid = V4RandomRequestId();

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(8);

  if (!client.connect(LINGXI_STREAM_ASR_HOST, 443)) {
    LastDoubaoAsrError = "录音文件ASR WebSocket TLS连接失败";
    LastDoubaoAsrState = "stream_connect_failed";
    return false;
  }

  String key = V4WebSocketKey();
  client.print(String("GET ") + LINGXI_STREAM_ASR_PATH + " HTTP/1.1\r\n");
  client.print(String("Host: ") + LINGXI_STREAM_ASR_HOST + "\r\n");
  client.print("Upgrade: websocket\r\n");
  client.print("Connection: Upgrade\r\n");
  client.print("Sec-WebSocket-Version: 13\r\n");
  client.print("Sec-WebSocket-Key: " + key + "\r\n");
  client.print("X-Api-Key: " + CloudVoiceApiKey() + "\r\n");
  client.print("X-Api-Resource-Id: " + LastDoubaoAsrResource + "\r\n");
  client.print("X-Api-Request-Id: " + reqid + "\r\n");
  client.print("X-Api-Connect-Id: " + reqid + "\r\n");
  client.print("X-Api-Sequence: -1\r\n");
  client.print("\r\n");

  String statusLine = client.readStringUntil('\n');
  statusLine.trim();
  if (statusLine.indexOf("101") < 0) {
    String err = statusLine;
    uint32_t hsStart = millis();
    while (client.connected() && client.available() && millis() - hsStart < 800) {
      err += client.readStringUntil('\n');
    }
    client.stop();
    LastDoubaoAsrError = "录音文件ASR握手失败：" + err.substring(0, 160);
    LastDoubaoAsrState = "stream_handshake_failed";
    return false;
  }

  uint32_t hst = millis();
  while (client.connected() && millis() - hst < 1000) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) break;
  }

  if (!BoardMicI2SBegin(VOICE_RECORD_SAMPLE_RATE)) {
    client.stop();
    LastDoubaoAsrError = "录音文件ASR麦克风初始化失败";
    LastDoubaoAsrState = "stream_mic_failed";
    return false;
  }

  if (!V4SaucSendFullRequest(client, reqid)) {
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    BoardAudioReady = false;
    client.stop();
    LastDoubaoAsrError = "录音文件ASR发送初始化包失败";
    LastDoubaoAsrState = "stream_init_failed";
    return false;
  }

  LastDoubaoAsrState = "streaming";
  Serial.println("[V4 STREAM] WebSocket connected resource=" + LastDoubaoAsrResource + " reqid=" + reqid);
  AddCenterLog("voice", "录音文件ASR已启动：" + LastDoubaoAsrResource);

  const size_t chunkBytesMax = (VOICE_RECORD_SAMPLE_RATE * 2 * LINGXI_STREAM_ASR_CHUNK_MS) / 1000 + 512;
  uint8_t *pcm = (uint8_t*)malloc(chunkBytesMax);
  if (!pcm) {
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    BoardAudioReady = false;
    client.stop();
    LastDoubaoAsrError = "录音文件ASR音频缓冲申请失败";
    LastDoubaoAsrState = "stream_buffer_failed";
    return false;
  }

  bool hadSpeech = false;
  uint32_t firstSpeechMs = 0;
  uint32_t lastSpeechMs = 0;
  uint32_t startMs = millis();
  uint32_t lastLogMs = 0;
  int32_t seq = 1;
  String lastText = "";

  while (client.connected() && millis() - startMs < LINGXI_STREAM_ASR_MAX_MS) {
    size_t pcmBytes = 0;
    int peak = 0;
    int rms = 0;
    bool got = V4BoardMicReadPcmChunk(pcm, chunkBytesMax, LINGXI_STREAM_ASR_CHUNK_MS, pcmBytes, peak, rms);
    uint32_t now = millis();

    if (got && pcmBytes > 0) {
      bool speech = (rms >= LINGXI_STREAM_ASR_SPEECH_RMS) || (peak >= LINGXI_STREAM_ASR_SPEECH_PEAK);
      bool silence = (rms <= LINGXI_STREAM_ASR_SILENCE_RMS) && (peak <= LINGXI_STREAM_ASR_SILENCE_PEAK);

      if (speech) {
        if (!hadSpeech) {
          firstSpeechMs = now;
          Serial.printf("[V4 STREAM] speech detected at %lums rms=%d peak=%d\n", (unsigned long)(now - startMs), rms, peak);
        }
        hadSpeech = true;
        lastSpeechMs = now;
      }

      if (!V4SaucSendAudioPacket(client, pcm, pcmBytes, seq++, false)) {
        LastDoubaoAsrError = "录音文件ASR音频包发送失败";
        break;
      }

      String msg;
      while (V4WsReadFrame(client, msg, 20)) {
        String t = V4SaucExtractText(msg);
        if (t.length()) {
          lastText = t;
          Serial.println("[V4 STREAM] partial/final text=" + t);
        }
      }

      if (now - lastLogMs > 1000) {
        lastLogMs = now;
        Serial.printf("[V4 STREAM] chunk rms=%d peak=%d bytes=%u speech=%d\n",
                      rms, peak, (unsigned)pcmBytes, speech ? 1 : 0);
      }

      if (hadSpeech && silence && now - startMs >= LINGXI_STREAM_ASR_MIN_MS && now - lastSpeechMs >= LINGXI_STREAM_ASR_SILENCE_MS) {
        Serial.printf("[V4 STREAM] endpoint at %lums silence=%lums firstSpeech=%lums\n",
                      (unsigned long)(now - startMs),
                      (unsigned long)(now - lastSpeechMs),
                      (unsigned long)(firstSpeechMs - startMs));
        break;
      }
    }

    delay(1);
  }

  V4SaucSendAudioPacket(client, nullptr, 0, seq, true);

  uint32_t waitStart = millis();
  while (client.connected() && millis() - waitStart < 2500) {
    String msg;
    if (V4WsReadFrame(client, msg, 250)) {
      String t = V4SaucExtractText(msg);
      if (t.length()) {
        lastText = t;
        Serial.println("[V4 STREAM] final text=" + t);
      }
    }

    if (lastText.length() && millis() - waitStart > 600) break;
    delay(5);
  }

  free(pcm);
  i2s_driver_uninstall(AUDIO_I2S_PORT);
  BoardAudioReady = false;
  client.stop();

  LastDoubaoAsrElapsedMs = millis() - t0;
  if (!lastText.length()) {
    LastDoubaoAsrState = "stream_no_text";
    if (!LastDoubaoAsrError.length()) LastDoubaoAsrError = "录音文件ASR未返回文本";
    AddCenterLog("sys", LastDoubaoAsrError);
    return false;
  }

  lastText.trim();
  outText = lastText;
  VoiceTaskSetText(outText, "doubao_stream_asr");
  VoiceTask.brain = "doubao_stream_asr";
  VoiceTask.reply = "已识别：" + outText;
  VoiceTask.status = VOICE_TASK_TEXT_READY;
  VoiceTask.updatedMs = millis();

  LastDoubaoAsrState = "stream_done";
  LastDoubaoAsrText = outText;
  LastDoubaoAsrError = "";
  AddCenterLog("op", "豆包录音文件ASR识别文本：" + outText + " resource=" + LastDoubaoAsrResource);
  return true;
#else
  (void)source;
  outText = "";
  return false;
#endif
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4VoiceRouteText(String text, String source) {
  text.trim();
  if (text.length() == 0) {
    V4Voice.lastError = "识别文本为空";
    V4VoiceSetState(V4_VOICE_ERROR, V4Voice.lastError);
    return false;
  }

  V4Voice.lastText = text;
  V4Voice.lastWakeSource = source;
  V4Voice.lastIntent = "";
  V4Voice.lastCommand = "";
  V4Voice.lastReply = "";
  V4Voice.lastError = "";

  VoiceTaskSetText(text, source);
  V4VoiceSetState(V4_VOICE_INTENT_ROUTING, text);

  
  
  if (V4ChatModeTryHandle(text, source)) {
    return true;
  }

  String intent = "";
  String cmd = VoiceTextToCommand(text, intent);
  V4Voice.lastIntent = intent;

  if (cmd.length()) {
    V4Voice.lastCommand = cmd;
    V4VoiceSetState(V4_VOICE_COMMAND_EXECUTING, cmd);
    AddCenterLog("op", "语音路由：本地白名单命中 " + cmd);
    String reply = ExecuteUnifiedCommand(cmd, "V4LocalVoice");
    VoiceTaskSetCommand(cmd, intent);
    VoiceTask.status = VOICE_TASK_EXECUTED;
    VoiceTask.executed = true;
    VoiceTask.replyReady = true;
    VoiceTask.reply = reply;
    VoiceTask.updatedMs = millis();
    V4Voice.lastReply = reply;
    LastCloudCommand = cmd;
    LastCloudReply = reply;
    
    
    
    BemfaPublishStatus();
    V4VoiceSetState(V4_VOICE_IDLE, "命令完成：" + reply);
    return true;
  }

  LingxiVoiceRouteResult route;
  if (ConfirmAgentTryRoute(text, route)) {
    V4Voice.lastIntent = route.intent.length() ? route.intent : "confirm_agent";
    V4Voice.lastCommand = route.command;
    V4Voice.lastReply = route.reply.length() ? route.reply : "我理解到一个可能的设备动作，需要你确认后再执行。";

    if (route.command.length() && !route.needConfirm) {
      V4VoiceSetState(V4_VOICE_COMMAND_EXECUTING, route.command);
      AddCenterLog("op", "语音路由：确认层已确认，执行 " + route.command);
      String execReply = ExecuteUnifiedCommand(route.command, "V4ConfirmAgent");
      VoiceTaskSetCommand(route.command, V4Voice.lastIntent);
      VoiceTask.reply = execReply.length() ? execReply : route.reply;
      VoiceTask.replyReady = true;
      VoiceTask.status = VOICE_TASK_EXECUTED;
      VoiceTask.executed = true;
      VoiceTask.updatedMs = millis();
      V4Voice.lastReply = VoiceTask.reply;
      LastCloudCommand = route.command;
      LastCloudReply = VoiceTask.reply;
      BemfaPublishStatus();
      V4VoiceSetState(V4_VOICE_IDLE, "确认层命令完成：" + VoiceTask.reply);
      return true;
    }

    VoiceTask.intent = V4Voice.lastIntent;
    VoiceTask.command = route.command;
    VoiceTask.commandReady = route.command.length() > 0;
    VoiceTask.reply = V4Voice.lastReply;
    VoiceTask.replyReady = true;
    VoiceTask.status = VOICE_TASK_REPLIED;
    VoiceTask.updatedMs = millis();
    AddCenterLog("op", route.needConfirm ? "语音路由：规则引擎命中，等待确认" : "语音路由：确认层回复");
    V4VoiceSetState(V4_VOICE_IDLE, route.needConfirm ? "规则引擎等待确认" : "确认层已回复");
    return true;
  }

  V4VoiceSetState(V4_VOICE_CHAT_RUNNING, "自由对话直发原文给豆包：" + text);
  AddCenterLog("op", "语音路由：进入自由对话，直发用户原文");
  bool ok = DoubaoFreeChatRunText(text, CloudModelMode(), true);
  if (!ok) {
    V4Voice.lastError = LastDoubaoAiError.length() ? LastDoubaoAiError : "豆包聊天失败";
    V4VoiceSetState(V4_VOICE_ERROR, V4Voice.lastError);
    return false;
  }

  V4Voice.lastReply = LastDoubaoAiReply.length() ? LastDoubaoAiReply : VoiceTask.reply;
  if (Voice.sdOnline && SD_MMC.exists(DOUBAO_TTS_LAST_PATH)) {
    V4VoiceSetState(V4_VOICE_TTS_PLAYING, "播放豆包TTS回复");
    BoardAudioQueueTtsPlayback(String(DOUBAO_TTS_LAST_PATH), "V4VoiceRouteFreeChat");
  }
  V4VoiceSetState(V4_VOICE_IDLE, "聊天完成");
  return true;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4VoiceRunOnceFromBoardMic(uint32_t seconds, String source) {
  if (!V4Voice.enabled) return false;
  if (V4Voice.busy || VoiceIO.recording || VoiceRecordAsyncActive) {
    V4Voice.lastError = "本机语音正忙";
    return false;
  }

  V4Voice.busy = true;
  V4VoiceClearLastInteraction(source);
  VoiceTaskClear("V4本机语音新会话，等待录音识别");
  V4VoiceSetState(V4_VOICE_WAKE_DETECTED, source);
  if (source.startsWith("ci1302_")) {
    
    
    Serial.println("[V4 CI1302] wake prompt skipped; normal one-shot file ASR unless chat mode is enabled");
    delay(650);
  } else {
    V4VoicePlayWakeReply();
  }

  bool useQuasiStreamAsr = false;
  if (useQuasiStreamAsr && seconds > LINGXI_V48_STREAM_RECORD_SECONDS) seconds = LINGXI_V48_STREAM_RECORD_SECONDS;

  V4VoiceSetState(V4_VOICE_RECORDING, String(useQuasiStreamAsr ? "快速录音 " : "开始录音 ") + String(seconds) + " 秒；请在提示音结束后再说话");
  Serial.println(String("[V4 FinalSeal LISTEN-ASR] record start seconds=") + String(seconds)
                 + " path=" + String(VOICE_LAST_RECORD_PATH)
                 + " mode=one_wake_one_file_asr");

  VoiceRecordStopRequested = false;
  VoiceRecordVadEnabled = useQuasiStreamAsr;
  VoiceRecordVadMinDurationMs = useQuasiStreamAsr ? LINGXI_V48_STREAM_RECORD_MIN_MS : 0;
  VoiceRecordVadSilenceMs = useQuasiStreamAsr ? LINGXI_V48_STREAM_SILENCE_MS : 0;
  VoiceRecordVadSpeechRmsThreshold = useQuasiStreamAsr ? LINGXI_V48_SPEECH_RMS_THRESHOLD : 0;
  VoiceRecordVadSpeechPeakThreshold = useQuasiStreamAsr ? LINGXI_V48_SPEECH_PEAK_THRESHOLD : 0;
  VoiceRecordVadSilenceRmsThreshold = useQuasiStreamAsr ? LINGXI_V48_SILENCE_RMS_THRESHOLD : 0;
  VoiceRecordVadSilencePeakThreshold = useQuasiStreamAsr ? LINGXI_V48_SILENCE_PEAK_THRESHOLD : 0;

  bool recOk = BoardMicRecordWav(String(VOICE_LAST_RECORD_PATH), seconds);
  VoiceRecordVadEnabled = false;
  VoiceRecordVadMinDurationMs = 0;
  VoiceRecordVadSilenceMs = 0;
  if (!recOk) {
    V4Voice.lastError = VoiceIO.lastError.length() ? VoiceIO.lastError : "录音失败";
    Serial.println("[V4 REC] fail error=" + V4Voice.lastError);
    if (V49ListenSource(source)) V49ListenSessionStop("record failed");
    V4Voice.busy = false;
    V4VoiceSetState(V4_VOICE_ERROR, V4Voice.lastError);
    return false;
  }

  Serial.println("[V4 FinalSeal LISTEN-ASR] record done file=" + VoiceIO.lastRecordFile
                 + " bytes=" + String(VoiceIO.lastDataBytes)
                 + " duration=" + String(VoiceIO.lastDurationMs)
                 + " peak=" + String(VoiceIO.peakLevel)
                 + " rms=" + String(VoiceIO.rmsLevel));

  String recordSource = source.length() ? source : "v4_board_mic";
  if (source.startsWith("ci1302_") && (VoiceIO.peakLevel < 80 || VoiceIO.rmsLevel < 8 || VoiceIO.lastDataBytes < 4096)) {
    VoiceTaskCreateFromRecord(recordSource);
    V4Voice.lastError = "我没听清，再说一遍。peak=" + String(VoiceIO.peakLevel) + " rms=" + String(VoiceIO.rmsLevel);
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = V4Voice.lastError;
    VoiceTask.reply = "我没听清，再说一遍。";
    VoiceTask.updatedMs = millis();
    LastDoubaoAsrState = "skip_low_audio";
    LastDoubaoAsrError = V4Voice.lastError;
    AddCenterLog("voice", "监听会话录音为空或音量过低，已跳过豆包ASR");
    Serial.println("[V4 FinalSeal LISTEN-ASR] skip low/empty record; no Doubao call. " + V4Voice.lastError);
    if (V49ListenSource(source)) V49ListenSessionStop("low/empty audio");
    V4Voice.busy = false;
    V4VoiceSetState(V4_VOICE_ERROR, V4Voice.lastError);
    return false;
  }

  VoiceTaskCreateFromRecord(recordSource);
  Serial.println("[V4 FinalSeal ASR] start file=" + VoiceTask.audioFile
                 + " bytes=" + String(VoiceTask.dataBytes)
                 + " peak=" + String(VoiceTask.peak)
                 + " rms=" + String(VoiceTask.rms));
  V4VoiceSetState(V4_VOICE_ASR_RUNNING, "录音文件识别：流式断句后快速识别");
  bool asrOk = VoiceTaskRunDoubaoAsr();
  if (!asrOk) {
    V4Voice.lastError = VoiceTask.error.length() ? VoiceTask.error : LastDoubaoAsrError;
    Serial.println("[V4 FinalSeal ASR] fail http=" + String(LastDoubaoAsrHttpCode)
                   + " elapsed=" + String(LastDoubaoAsrElapsedMs)
                   + " state=" + LastDoubaoAsrState
                   + " error=" + V4Voice.lastError);
    if (V49ListenSource(source)) V49ListenSessionStop("asr failed");
    V4Voice.busy = false;
    V4VoiceSetState(V4_VOICE_ERROR, V4Voice.lastError);
    return false;
  }

  String asrText = VoiceTask.text;
  asrText.trim();
  Serial.println("[V4 FinalSeal ASR] ok http=" + String(LastDoubaoAsrHttpCode)
                 + " elapsed=" + String(LastDoubaoAsrElapsedMs)
                 + " text=" + asrText);
  if (!asrText.length()) {
    V4Voice.lastError = "ASR返回空文本";
    if (V49ListenSource(source)) V49ListenSessionStop("empty asr text");
    V4Voice.busy = false;
    V4VoiceSetState(V4_VOICE_ERROR, V4Voice.lastError);
    return false;
  }

  bool routeOk = V4VoiceRouteText(asrText, "v4_board_asr");
  V4Voice.busy = false;
  
  return routeOk;
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool V4HandleSerialCommand(String raw) {
  String cmd = raw;
  cmd.trim();
  String low = cmd;
  low.toLowerCase();

  if (low == "v4" || low == "v4 status") {
    Serial.println(V4VoiceStatusJson());
    return true;
  }
  if (low == "v4 wake" || low == "wake" || low == "你好灵汐" || low.startsWith("v4 wake ")) {
    uint32_t sec = LINGXI_V4_RECORD_SECONDS;
    if (low.startsWith("v4 wake ")) {
      int sp = low.lastIndexOf(' ');
      int v = low.substring(sp + 1).toInt();
      if (v >= 3 && v <= 15) sec = v;
    }
    V49ListenSessionStart("serial_wake");
    Serial.println("[V4.9] queued wake/listen seconds=" + String(sec) + " -> TaskV4LocalVoice");
    V4QueueRequest("wake", "", sec, "v49_listen_serial_wake");
    Serial.println(V4VoiceStatusJson());
    return true;
  }
  if (low.startsWith("v4 say ")) {
    String text = cmd.substring(7);
    text.trim();
    Serial.println("[V4] queued text: " + text);
    V4QueueRequest("say", text, 0, "serial_text");
    Serial.println(V4VoiceStatusJson());
    return true;
  }
  if (low.startsWith("v4 record")) {
    uint32_t sec = LINGXI_V4_RECORD_SECONDS;
    int sp = low.lastIndexOf(' ');
    if (sp > 0) {
      int v = low.substring(sp + 1).toInt();
      if (v >= 2 && v <= 15) sec = v;
    }
    Serial.println("[V4] queued record seconds=" + String(sec));
    V4QueueRequest("record", "", sec, "serial_record");
    Serial.println(V4VoiceStatusJson());
    return true;
  }
  return false;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleV4VoiceStatusApi() {
  SendCorsHeader();
  server.send(200, "application/json", V4VoiceStatusJson());
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleV4VoiceWakeApi() {
  uint32_t sec = LINGXI_V4_RECORD_SECONDS;
  if (server.hasArg("sec")) {
    int v = server.arg("sec").toInt();
    if (v >= 2 && v <= 15) sec = v;
  }
  V49ListenSessionStart("http_wake");
  V4QueueRequest("wake", "", sec, "v49_listen_http_wake");
  String json = V4VoiceStatusJson();
  json.replace("\"ok\":true", "\"ok\":true,\"queued\":true");
  SendCorsHeader();
  server.send(202, "application/json", json);
}

