/*
 * 文件：07_pages_settings_part1.ino
 * 标题：系统设置页与本地语音命令
 * 说明：负责设置页前半部分，以及本地白名单/语音文本转命令。
 * 可改：可调整白名单词
 * 禁止：把普通闲聊写成本地固定回复。
 */

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitVoiceIoState() {
  VoiceIO.inputSource = VOICE_INPUT_WEBUI_MIC;
  VoiceIO.outputTarget = VOICE_OUTPUT_WEBUI_SPEAKER;
  VoiceIO.brainMode = VOICE_BRAIN_RECORD_TEST;
  VoiceIO.micReady = false;
  VoiceIO.recording = false;
  VoiceIO.lastRecordReady = false;
  VoiceIO.lastRecordFile = VOICE_LAST_RECORD_PATH;
  VoiceIO.lastError = "";
  VoiceIO.lastRecordMs = 0;
  VoiceIO.lastDurationMs = 0;
  VoiceIO.lastDataBytes = 0;
  VoiceIO.sampleRate = VOICE_RECORD_SAMPLE_RATE;
  VoiceIO.channels = VOICE_RECORD_CHANNELS;
  VoiceIO.bitsPerSample = VOICE_RECORD_BITS_PER_SAMPLE;
  VoiceIO.peakLevel = 0;
  VoiceIO.rmsLevel = 0;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceInputSourceName() {
  if (VoiceIO.inputSource == VOICE_INPUT_WEBUI_MIC) return "webui_mic";
  if (VoiceIO.inputSource == VOICE_INPUT_AUTO) return "auto";
  return "board_mic";
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceOutputTargetName() {
  if (VoiceIO.outputTarget == VOICE_OUTPUT_WEBUI_SPEAKER) return "webui_speaker";
  if (VoiceIO.outputTarget == VOICE_OUTPUT_BOTH) return "both";
  if (VoiceIO.outputTarget == VOICE_OUTPUT_AUTO) return "auto";
  return "board_speaker";
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceBrainModeName() {
  if (VoiceIO.brainMode == VOICE_BRAIN_CLOUD_ASR) return "cloud_asr";
  if (VoiceIO.brainMode == VOICE_BRAIN_DOUBAO_AI) return "doubao_ai";
  if (VoiceIO.brainMode == VOICE_BRAIN_LOCAL_ESPSR) return "local_espsr";
  return "record_test";
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void InitVoiceTaskState() {
  VoiceTask.seq = 0;
  VoiceTask.status = VOICE_TASK_IDLE;
  VoiceTask.source = VoiceInputSourceName();
  VoiceTask.output = VoiceOutputTargetName();
  VoiceTask.brain = VoiceBrainModeName();
  VoiceTask.audioFile = "";
  VoiceTask.text = "";
  VoiceTask.intent = "";
  VoiceTask.command = "";
  VoiceTask.reply = "";
  VoiceTask.error = "";
  VoiceTask.uploadReady = false;
  VoiceTask.asrReady = false;
  VoiceTask.commandReady = false;
  VoiceTask.executed = false;
  VoiceTask.replyReady = false;
  VoiceTask.createdMs = 0;
  VoiceTask.updatedMs = 0;
  VoiceTask.durationMs = 0;
  VoiceTask.dataBytes = 0;
  VoiceTask.peak = 0;
  VoiceTask.rms = 0;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceTaskStatusName() {
  if (VoiceTask.status == VOICE_TASK_RECORDING) return "recording";
  if (VoiceTask.status == VOICE_TASK_RECORDED) return "recorded";
  if (VoiceTask.status == VOICE_TASK_TEXT_READY) return "text_ready";
  if (VoiceTask.status == VOICE_TASK_COMMAND_READY) return "command_ready";
  if (VoiceTask.status == VOICE_TASK_EXECUTED) return "executed";
  if (VoiceTask.status == VOICE_TASK_REPLIED) return "replied";
  if (VoiceTask.status == VOICE_TASK_ERROR) return "error";
  return "idle";
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceTaskStatusJson() {
  String json = "{";
  json += "\"ok\":true,";
  json += "\"seq\":" + String(VoiceTask.seq) + ",";
  json += "\"status\":\"" + VoiceTaskStatusName() + "\",";
  json += "\"source\":\"" + JsonEscape(VoiceTask.source) + "\",";
  json += "\"output\":\"" + JsonEscape(VoiceTask.output) + "\",";
  json += "\"brain\":\"" + JsonEscape(VoiceTask.brain) + "\",";
  json += "\"audio\":\"" + JsonEscape(VoiceTask.audioFile) + "\",";
  json += "\"text\":\"" + JsonEscape(VoiceTask.text) + "\",";
  json += "\"intent\":\"" + JsonEscape(VoiceTask.intent) + "\",";
  json += "\"command\":\"" + JsonEscape(VoiceTask.command) + "\",";
  json += "\"reply\":\"" + JsonEscape(VoiceTask.reply) + "\",";
  json += "\"error\":\"" + JsonEscape(VoiceTask.error) + "\",";
  json += "\"upload_ready\":" + String(VoiceTask.uploadReady ? "true" : "false") + ",";
  json += "\"asr_ready\":" + String(VoiceTask.asrReady ? "true" : "false") + ",";
  json += "\"command_ready\":" + String(VoiceTask.commandReady ? "true" : "false") + ",";
  json += "\"executed\":" + String(VoiceTask.executed ? "true" : "false") + ",";
  json += "\"reply_ready\":" + String(VoiceTask.replyReady ? "true" : "false") + ",";
  json += "\"created_ms\":" + String(VoiceTask.createdMs) + ",";
  json += "\"updated_ms\":" + String(VoiceTask.updatedMs) + ",";
  json += "\"duration_ms\":" + String(VoiceTask.durationMs) + ",";
  json += "\"bytes\":" + String(VoiceTask.dataBytes) + ",";
  json += "\"peak\":" + String(VoiceTask.peak) + ",";
  json += "\"rms\":" + String(VoiceTask.rms) + ",";
  json += "\"asr_bridge_url\":\"" + JsonEscape(AsrBridgeUrl()) + "\",";
  json += "\"asr_bridge_configured\":" + String(!IsPlaceholderText(ASR_BRIDGE_URL) ? "true" : "false");
  json += "}";
  return json;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceTaskClear(String reason) {
  InitVoiceTaskState();
  VoiceTask.source = VoiceInputSourceName();
  VoiceTask.output = VoiceOutputTargetName();
  VoiceTask.brain = VoiceBrainModeName();
  VoiceTask.reply = reason;
  VoiceTask.updatedMs = millis();
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceTaskCreateFromRecord(String source) {
  VoiceTask.seq = VoiceTaskSeqCounter++;
  VoiceTask.status = VOICE_TASK_RECORDED;
  VoiceTask.source = source.length() ? source : VoiceInputSourceName();
  VoiceTask.output = VoiceOutputTargetName();
  VoiceTask.brain = VoiceBrainModeName();
  VoiceTask.audioFile = VoiceIO.lastRecordFile;
  VoiceTask.text = "";
  VoiceTask.intent = "";
  VoiceTask.command = "";
  VoiceTask.reply = "录音完成，等待 V4.0 ASR 识别";
  VoiceTask.error = VoiceIO.lastError;
  VoiceTask.uploadReady = VoiceIO.lastRecordReady;
  VoiceTask.asrReady = false;
  VoiceTask.commandReady = false;
  VoiceTask.executed = false;
  VoiceTask.replyReady = false;
  VoiceTask.createdMs = millis();
  VoiceTask.updatedMs = VoiceTask.createdMs;
  VoiceTask.durationMs = VoiceIO.lastDurationMs;
  VoiceTask.dataBytes = VoiceIO.lastDataBytes;
  VoiceTask.peak = VoiceIO.peakLevel;
  VoiceTask.rms = VoiceIO.rmsLevel;
  AddCenterLog("op", "VoiceTask #" + String(VoiceTask.seq) + " 已生成：" + VoiceTask.audioFile);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceTaskSetText(String text, String source) {
  if (VoiceTask.seq == 0) {
    VoiceTask.seq = VoiceTaskSeqCounter++;
    VoiceTask.createdMs = millis();
  }
  VoiceTask.status = VOICE_TASK_TEXT_READY;
  VoiceTask.text = text;
  VoiceTask.source = source.length() ? source : VoiceTask.source;
  VoiceTask.asrReady = true;
  VoiceTask.reply = "文本已进入语音管线，V4.0 将接入白名单解析";
  VoiceTask.updatedMs = millis();
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceTaskSetCommand(String command, String intent) {
  if (VoiceTask.seq == 0) {
    VoiceTask.seq = VoiceTaskSeqCounter++;
    VoiceTask.createdMs = millis();
  }
  VoiceTask.status = VOICE_TASK_COMMAND_READY;
  VoiceTask.command = command;
  VoiceTask.intent = intent.length() ? intent : "manual_mock";
  VoiceTask.commandReady = command.length() > 0;
  VoiceTask.updatedMs = millis();
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceTaskSetReply(String reply) {
  if (VoiceTask.seq == 0) {
    VoiceTask.seq = VoiceTaskSeqCounter++;
    VoiceTask.createdMs = millis();
  }
  VoiceTask.status = VOICE_TASK_REPLIED;
  VoiceTask.reply = reply;
  VoiceTask.replyReady = reply.length() > 0;
  VoiceTask.updatedMs = millis();
}

// 函数说明：JSON 工具函数，负责构造或转义接口数据。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceIoStatusJson() {
  String json = "{";
  json += "\"ok\":true,";
  json += "\"input\":\"" + VoiceInputSourceName() + "\",";
  json += "\"output\":\"" + VoiceOutputTargetName() + "\",";
  json += "\"brain\":\"" + VoiceBrainModeName() + "\",";
  json += "\"mic_ready\":" + String(VoiceIO.micReady ? "true" : "false") + ",";
  json += "\"recording\":" + String(VoiceIO.recording ? "true" : "false") + ",";
  json += "\"ready\":" + String(VoiceIO.lastRecordReady ? "true" : "false") + ",";
  json += "\"file\":\"" + JsonEscape(VoiceIO.lastRecordFile) + "\",";
  json += "\"error\":\"" + JsonEscape(VoiceIO.lastError) + "\",";
  json += "\"sample_rate\":" + String(VoiceIO.sampleRate) + ",";
  json += "\"channels\":" + String(VoiceIO.channels) + ",";
  json += "\"bits\":" + String(VoiceIO.bitsPerSample) + ",";
  json += "\"mic_decode\":\"" + JsonEscape(VoiceMicDecodeModeName()) + "\",";
  json += "\"mic_decode_id\":" + String(VoiceMicDecodeMode) + ",";
  json += "\"peak\":" + String(VoiceIO.peakLevel) + ",";
  json += "\"rms\":" + String(VoiceIO.rmsLevel) + ",";
  json += "\"duration_ms\":" + String(VoiceIO.lastDurationMs) + ",";
  json += "\"bytes\":" + String(VoiceIO.lastDataBytes) + ",";
  json += "\"task\":" + VoiceTaskStatusJson();
  json += "}";
  return json;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceIoWriteWavHeader(File &file, uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels, uint32_t dataSize) {
  uint8_t h[44];
  memset(h, 0, sizeof(h));
  uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
  uint16_t blockAlign = channels * bitsPerSample / 8;
  uint32_t riffSize = 36 + dataSize;

  memcpy(h + 0, "RIFF", 4);
  h[4] = riffSize & 0xFF; h[5] = (riffSize >> 8) & 0xFF; h[6] = (riffSize >> 16) & 0xFF; h[7] = (riffSize >> 24) & 0xFF;
  memcpy(h + 8, "WAVE", 4);
  memcpy(h + 12, "fmt ", 4);
  h[16] = 16; h[17] = 0; h[18] = 0; h[19] = 0;
  h[20] = 1; h[21] = 0;
  h[22] = channels & 0xFF; h[23] = (channels >> 8) & 0xFF;
  h[24] = sampleRate & 0xFF; h[25] = (sampleRate >> 8) & 0xFF; h[26] = (sampleRate >> 16) & 0xFF; h[27] = (sampleRate >> 24) & 0xFF;
  h[28] = byteRate & 0xFF; h[29] = (byteRate >> 8) & 0xFF; h[30] = (byteRate >> 16) & 0xFF; h[31] = (byteRate >> 24) & 0xFF;
  h[32] = blockAlign & 0xFF; h[33] = (blockAlign >> 8) & 0xFF;
  h[34] = bitsPerSample & 0xFF; h[35] = (bitsPerSample >> 8) & 0xFF;
  memcpy(h + 36, "data", 4);
  h[40] = dataSize & 0xFF; h[41] = (dataSize >> 8) & 0xFF; h[42] = (dataSize >> 16) & 0xFF; h[43] = (dataSize >> 24) & 0xFF;
  file.write(h, sizeof(h));
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ES7210WriteReg(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  bool ok = (Wire.endTransmission() == 0);
  if (!ok) {
    Serial.printf("[ES7210] write addr=0x%02X reg=0x%02X val=0x%02X failed\n", addr, reg, val);
  }
  return ok;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
uint8_t ES7210ProbeAddress() {
  Wire.begin(BOARD_I2C_SDA_PIN, BOARD_I2C_SCL_PIN);
  Wire.setClock(400000);

  const uint8_t addrs[] = {ES7210_ADDR_00, ES7210_ADDR_01, ES7210_ADDR_10, ES7210_ADDR_11};
  for (uint8_t i = 0; i < sizeof(addrs); i++) {
    Wire.beginTransmission(addrs[i]);
    if (Wire.endTransmission() == 0) {
      Serial.printf("[ES7210] detected at 0x%02X\n", addrs[i]);
      return addrs[i];
    }
  }
  Serial.println("[ES7210] not detected on 0x40-0x43");
  return 0;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ES7210MicAdcInit(uint32_t sampleRate) {
  uint8_t addr = ES7210ProbeAddress();
  if (addr == 0) {
    VoiceIO.lastError = "ES7210 not detected: mic ADC is offline";
    return false;
  }

  uint8_t mainClk = 0xC1;
  uint8_t lrckH = 0x01;
  uint8_t lrckL = 0x00;
  uint8_t osr = 0x20;

  if (sampleRate == 32000) {
    mainClk = 0x03; lrckH = 0x01; lrckL = 0x80; osr = 0x20;
  } else if (sampleRate == 16000) {
    mainClk = 0xC1; lrckH = 0x01; lrckL = 0x00; osr = 0x20;
  } else if (sampleRate == 48000) {
    mainClk = 0xC1; lrckH = 0x01; lrckL = 0x00; osr = 0x20;
  } else {
    Serial.printf("[ES7210] unsupported sampleRate=%lu, fallback to 16k coeff\n", (unsigned long)sampleRate);
  }

  bool ok = true;
  ok &= ES7210WriteReg(addr, 0x00, 0xFF);
  delay(10);
  ok &= ES7210WriteReg(addr, 0x00, 0x32);
  delay(10);
  ok &= ES7210WriteReg(addr, 0x09, 0x30);
  ok &= ES7210WriteReg(addr, 0x0A, 0x30);
  ok &= ES7210WriteReg(addr, 0x23, 0x2A);
  ok &= ES7210WriteReg(addr, 0x22, 0x0A);
  ok &= ES7210WriteReg(addr, 0x21, 0x2A);
  ok &= ES7210WriteReg(addr, 0x20, 0x0A);
  ok &= ES7210WriteReg(addr, 0x11, 0x60);  
  ok &= ES7210WriteReg(addr, 0x12, 0x00);  
  ok &= ES7210WriteReg(addr, 0x40, 0xC3);
  ok &= ES7210WriteReg(addr, 0x41, ES7210_MIC_BIAS_2V87);
  ok &= ES7210WriteReg(addr, 0x42, ES7210_MIC_BIAS_2V87);
  ok &= ES7210WriteReg(addr, 0x43, ES7210_MIC_GAIN_REG_VALUE);
  ok &= ES7210WriteReg(addr, 0x44, ES7210_MIC_GAIN_REG_VALUE);
  ok &= ES7210WriteReg(addr, 0x45, ES7210_MIC_GAIN_REG_VALUE);
  ok &= ES7210WriteReg(addr, 0x46, ES7210_MIC_GAIN_REG_VALUE);
  ok &= ES7210WriteReg(addr, 0x47, 0x08);
  ok &= ES7210WriteReg(addr, 0x48, 0x08);
  ok &= ES7210WriteReg(addr, 0x49, 0x08);
  ok &= ES7210WriteReg(addr, 0x4A, 0x08);
  ok &= ES7210WriteReg(addr, 0x07, osr);
  ok &= ES7210WriteReg(addr, 0x02, mainClk);
  ok &= ES7210WriteReg(addr, 0x04, lrckH);
  ok &= ES7210WriteReg(addr, 0x05, lrckL);
  ok &= ES7210WriteReg(addr, 0x06, 0x04);
  ok &= ES7210WriteReg(addr, 0x4B, 0x0F);
  ok &= ES7210WriteReg(addr, 0x4C, 0x0F);
  ok &= ES7210WriteReg(addr, 0x1B, ES7210_ADC_VOLUME_0DB);
  ok &= ES7210WriteReg(addr, 0x1C, ES7210_ADC_VOLUME_0DB);
  ok &= ES7210WriteReg(addr, 0x1D, ES7210_ADC_VOLUME_0DB);
  ok &= ES7210WriteReg(addr, 0x1E, ES7210_ADC_VOLUME_0DB);
  ok &= ES7210WriteReg(addr, 0x00, 0x71);
  delay(10);
  ok &= ES7210WriteReg(addr, 0x00, 0x41);
  delay(10);

  if (!ok) {
    VoiceIO.lastError = "ES7210 mic ADC init failed";
    return false;
  }

  Serial.printf("[ES7210] mic ADC init OK, sampleRate=%lu\n", (unsigned long)sampleRate);
  return true;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool BoardMicI2SBegin(uint32_t sampleRate) {
#if ENABLE_VOICE_IO_FOUNDATION
  if (BoardAudioReady) {
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    BoardAudioReady = false;
  }

  i2s_config_t i2s_config;
  memset(&i2s_config, 0, sizeof(i2s_config));
  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  i2s_config.sample_rate = sampleRate;
  i2s_bits_per_sample_t micBits = VoiceMicI2SBits();
  i2s_config.bits_per_sample = micBits;  
  i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2s_config.dma_buf_count = 8;
  i2s_config.dma_buf_len = 256;
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = false;
  i2s_config.fixed_mclk = sampleRate * 256;

  esp_err_t err = i2s_driver_install(AUDIO_I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    VoiceIO.lastError = "I2S RX driver install failed: " + String((int)err);
    return false;
  }

  i2s_pin_config_t pin_config;
  memset(&pin_config, 0, sizeof(pin_config));
  pin_config.mck_io_num = AUDIO_I2S_MCLK_PIN;
  pin_config.bck_io_num = AUDIO_I2S_BCLK_PIN;
  pin_config.ws_io_num = AUDIO_I2S_LRCK_PIN;
  pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  pin_config.data_in_num = AUDIO_I2S_DIN_PIN;

  err = i2s_set_pin(AUDIO_I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    VoiceIO.lastError = "I2S RX set pin failed: " + String((int)err);
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    return false;
  }

  err = i2s_set_clk(AUDIO_I2S_PORT, sampleRate, micBits, I2S_CHANNEL_STEREO);
  if (err != ESP_OK) {
    VoiceIO.lastError = "I2S RX set clock failed: " + String((int)err);
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    return false;
  }

  
  if (!ES7210MicAdcInit(sampleRate)) {
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    return false;
  }

  i2s_zero_dma_buffer(AUDIO_I2S_PORT);
  VoiceIO.micReady = true;
  return true;
#else
  (void)sampleRate;
  VoiceIO.lastError = "ENABLE_VOICE_IO_FOUNDATION=0";
  return false;
#endif
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool BoardMicRecordWav(String path, uint32_t seconds) {
#if ENABLE_VOICE_IO_FOUNDATION
  if (VoiceIO.recording) {
    VoiceIO.lastError = "recording busy";
    return false;
  }
  if (seconds < 1) seconds = 1;
  if (seconds > VOICE_RECORD_MAX_SECONDS) seconds = VOICE_RECORD_MAX_SECONDS;

  if (!Voice.sdOnline) {
    VoiceInitSD();
  }
  if (!Voice.sdOnline) {
    VoiceIO.lastError = "SD offline, cannot record";
    return false;
  }

  if (!SD_MMC.exists(VOICE_RECORD_DIR)) {
    SD_MMC.mkdir(VOICE_RECORD_DIR);
  }
  if (SD_MMC.exists(path.c_str())) {
    SD_MMC.remove(path.c_str());
  }

  File file = SD_MMC.open(path.c_str(), FILE_WRITE);
  if (!file) {
    VoiceIO.lastError = "failed to create wav file";
    return false;
  }

  VoiceIO.recording = true;
  VoiceIO.lastRecordReady = false;
  VoiceIO.lastError = "";
  VoiceIO.lastRecordFile = path;
  VoiceIO.lastRecordMs = millis();
  VoiceIO.lastDurationMs = 0;
  VoiceIO.lastDataBytes = 0;
  VoiceIO.peakLevel = 0;
  VoiceIO.rmsLevel = 0;

  VoiceIoWriteWavHeader(file, VOICE_RECORD_SAMPLE_RATE, VOICE_RECORD_BITS_PER_SAMPLE, VOICE_RECORD_CHANNELS, 0);

  if (!BoardMicI2SBegin(VOICE_RECORD_SAMPLE_RATE)) {
    file.close();
    VoiceIO.recording = false;
    return false;
  }

  uint8_t* rxBuf = (uint8_t*)heap_caps_malloc(BOARD_AUDIO_BUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!rxBuf) rxBuf = (uint8_t*)malloc(BOARD_AUDIO_BUF_SIZE);
  int16_t* monoBuf = (int16_t*)heap_caps_malloc(BOARD_AUDIO_BUF_SIZE / 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!monoBuf) monoBuf = (int16_t*)malloc(BOARD_AUDIO_BUF_SIZE / 2);

  if (!rxBuf || !monoBuf) {
    if (rxBuf) free(rxBuf);
    if (monoBuf) free(monoBuf);
    file.close();
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    VoiceIO.recording = false;
    VoiceIO.lastError = "record buffer malloc failed";
    return false;
  }

  uint32_t startMs = millis();
  uint32_t endMs = startMs + seconds * 1000UL;
  uint32_t dataBytes = 0;
  int peak = 0;
  uint64_t sumSq = 0;
  uint32_t sampleCount = 0;

  bool vadEnabled = VoiceRecordVadEnabled;
  uint32_t vadMinMs = VoiceRecordVadMinDurationMs;
  uint32_t vadSilenceMs = VoiceRecordVadSilenceMs;
  int vadSpeechRms = VoiceRecordVadSpeechRmsThreshold;
  int vadSpeechPeak = VoiceRecordVadSpeechPeakThreshold;
  int vadSilenceRms = VoiceRecordVadSilenceRmsThreshold;
  int vadSilencePeak = VoiceRecordVadSilencePeakThreshold;
  bool vadHadSpeech = false;
  uint32_t vadFirstSpeechMs = 0;
  uint32_t vadLastSpeechMs = 0;
  uint32_t vadLastLogMs = 0;

  if (vadEnabled) {
    Serial.printf("[V4 FinalSeal LISTEN-ASR] true-stream record start max=%lus min=%lums silence=%lums speechRms=%d speechPeak=%d\n",
                  (unsigned long)seconds, (unsigned long)vadMinMs, (unsigned long)vadSilenceMs, vadSpeechRms, vadSpeechPeak);
  }

  while (millis() < endMs && !VoiceRecordStopRequested) {
    size_t bytesRead = 0;
    esp_err_t err = i2s_read(AUDIO_I2S_PORT, rxBuf, BOARD_AUDIO_BUF_SIZE, &bytesRead, 200 / portTICK_PERIOD_MS);
    if (err != ESP_OK) {
      VoiceIO.lastError = "I2S read failed: " + String((int)err);
      break;
    }
    if (bytesRead == 0) {
      delay(1);
      continue;
    }

    int chunkPeak = 0;
    uint64_t chunkSumSq = 0;
    uint32_t chunkSampleCount = 0;

    
    
    
    size_t frames = 0;
    i2s_bits_per_sample_t activeBits = VoiceMicI2SBits();

    if (activeBits == I2S_BITS_PER_SAMPLE_16BIT) {
      int16_t* samples16 = (int16_t*)rxBuf;
      size_t sample16Count = bytesRead / 2;
      if (VoiceMicDecodeMode == 7) {
        frames = sample16Count;
        if (frames > BOARD_AUDIO_BUF_SIZE / 4) frames = BOARD_AUDIO_BUF_SIZE / 4;
        for (size_t i = 0; i < frames; i++) {
          int32_t sample = samples16[i];
          int32_t amplified = sample * VOICE_RECORD_SOFTWARE_GAIN;
          if (amplified > 32767) amplified = 32767;
          if (amplified < -32768) amplified = -32768;
          monoBuf[i] = (int16_t)amplified;
          int32_t a = amplified < 0 ? -amplified : amplified;
          if (a > peak) peak = a;
          sumSq += (uint32_t)a * (uint32_t)a;
          if (a > chunkPeak) chunkPeak = a;
          chunkSumSq += (uint32_t)a * (uint32_t)a;
          chunkSampleCount++;
        }
      } else {
        frames = sample16Count / 2;
        if (frames > BOARD_AUDIO_BUF_SIZE / 4) frames = BOARD_AUDIO_BUF_SIZE / 4;
        for (size_t i = 0; i < frames; i++) {
          int32_t left = samples16[i * 2];
          int32_t right = samples16[i * 2 + 1];
          int32_t sample = (LingxiAbs32(right) > LingxiAbs32(left)) ? right : left;
          int32_t amplified = sample * VOICE_RECORD_SOFTWARE_GAIN;
          if (amplified > 32767) amplified = 32767;
          if (amplified < -32768) amplified = -32768;
          monoBuf[i] = (int16_t)amplified;
          int32_t a = amplified < 0 ? -amplified : amplified;
          if (a > peak) peak = a;
          sumSq += (uint32_t)a * (uint32_t)a;
          if (a > chunkPeak) chunkPeak = a;
          chunkSumSq += (uint32_t)a * (uint32_t)a;
          chunkSampleCount++;
        }
      }
    } else {
      int32_t* samples32 = (int32_t*)rxBuf;
      size_t sample32Count = bytesRead / 4;
      frames = sample32Count / 2;
      if (frames > BOARD_AUDIO_BUF_SIZE / 8) frames = BOARD_AUDIO_BUF_SIZE / 8;

      for (size_t i = 0; i < frames; i++) {
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

        monoBuf[i] = (int16_t)amplified;
        int32_t a = amplified < 0 ? -amplified : amplified;
        if (a > peak) peak = a;
        sumSq += (uint32_t)a * (uint32_t)a;
        if (a > chunkPeak) chunkPeak = a;
        chunkSumSq += (uint32_t)a * (uint32_t)a;
        chunkSampleCount++;
      }
    }

    size_t monoBytes = frames * sizeof(int16_t);
    if (monoBytes > 0) {
      file.write((uint8_t*)monoBuf, monoBytes);
      dataBytes += monoBytes;
      sampleCount += frames;
    }

    if (vadEnabled && chunkSampleCount > 0) {
      uint32_t nowMs = millis();
      uint32_t elapsedMs = nowMs - startMs;
      int chunkRms = (int)sqrt((double)chunkSumSq / (double)chunkSampleCount);
      bool speechChunk = (chunkRms >= vadSpeechRms) || (chunkPeak >= vadSpeechPeak);
      bool silenceChunk = (chunkRms <= vadSilenceRms) && (chunkPeak <= vadSilencePeak);

      if (speechChunk) {
        if (!vadHadSpeech) {
          vadFirstSpeechMs = nowMs;
          Serial.printf("[V4 FinalSeal LISTEN-ASR] speech detected at %lums rms=%d peak=%d\n",
                        (unsigned long)elapsedMs, chunkRms, chunkPeak);
        }
        vadHadSpeech = true;
        vadLastSpeechMs = nowMs;
      }

      if (nowMs - vadLastLogMs > 1000) {
        vadLastLogMs = nowMs;
        Serial.printf("[V4 FinalSeal LISTEN-ASR] chunk elapsed=%lums rms=%d peak=%d speech=%d\n",
                      (unsigned long)elapsedMs, chunkRms, chunkPeak, speechChunk ? 1 : 0);
      }

      if (vadHadSpeech && silenceChunk && elapsedMs >= vadMinMs && (nowMs - vadLastSpeechMs) >= vadSilenceMs) {
        Serial.printf("[V4 FinalSeal LISTEN-ASR] auto endpoint at %lums silence=%lums firstSpeech=%lums\n",
                      (unsigned long)elapsedMs, (unsigned long)(nowMs - vadLastSpeechMs),
                      (unsigned long)(vadFirstSpeechMs - startMs));
        break;
      }
    }
    delay(1);
  }

  free(rxBuf);
  free(monoBuf);

  uint32_t durationMs = millis() - startMs;
  file.seek(0);
  VoiceIoWriteWavHeader(file, VOICE_RECORD_SAMPLE_RATE, VOICE_RECORD_BITS_PER_SAMPLE, VOICE_RECORD_CHANNELS, dataBytes);
  file.close();
  i2s_driver_uninstall(AUDIO_I2S_PORT);
  BoardAudioReady = false;

  VoiceIO.recording = false;
  bool voiceTooShort = (VoiceRecordMinDurationMs > 0 && durationMs < VoiceRecordMinDurationMs);
  VoiceIO.lastRecordReady = dataBytes > 0 && VoiceIO.lastError.length() == 0 && !voiceTooShort;
  VoiceIO.lastDurationMs = durationMs;
  VoiceIO.lastDataBytes = dataBytes;
  VoiceIO.peakLevel = peak;
  VoiceIO.rmsLevel = sampleCount > 0 ? (int)sqrt((double)sumSq / (double)sampleCount) : 0;
  VoiceIO.micReady = true;

  if (voiceTooShort) {
    VoiceIO.lastError = "说话时间太短";
    AddCenterLog("sys", "中控麦克风录音忽略：说话时间太短 " + String(durationMs) + "ms");
    BemfaPublishStatus();
    return false;
  }

  if (VoiceIO.lastRecordReady) {
    VoiceTaskCreateFromRecord("board_mic");
    AddCenterLog("op", "中控麦克风录音完成：" + VoiceMicDecodeModeName() + " " + String(durationMs) + "ms peak=" + String(VoiceIO.peakLevel) + " rms=" + String(VoiceIO.rmsLevel) + " bytes=" + String(VoiceIO.lastDataBytes) + " /record/last.wav");
    BemfaPublishStatus();
    return true;
  }

  if (VoiceIO.lastError.length() == 0) VoiceIO.lastError = "recorded empty data";
  AddCenterLog("sys", "中控麦克风录音失败：" + VoiceIO.lastError);
  BemfaPublishStatus();
  return false;
#else
  (void)path;
  (void)seconds;
  VoiceIO.lastError = "ENABLE_VOICE_IO_FOUNDATION=0";
  return false;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceIoStatusApi() {
  SendCorsHeader();
  server.send(200, "application/json", VoiceIoStatusJson());
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceIoRecordApi() {
  uint32_t sec = server.hasArg("sec") ? (uint32_t)server.arg("sec").toInt() : 3;
  
  
  
  if (server.hasArg("fmt")) {
    int m = server.arg("fmt").toInt();
    if (m < 0) m = 0;
    if (m > 7) m = 7;
    VoiceMicDecodeMode = (uint8_t)m;
    AddCenterLog("sys", "麦克风格式临时探针：" + VoiceMicDecodeModeName());
  } else {
    VoiceMicDecodeMode = LINGXI_MIC_LOCK_FMT;
    AddCenterLog("sys", "麦克风格式统一锁定：" + VoiceMicDecodeModeName());
  }
  VoiceRecordStopRequested = false;
  VoiceRecordMinDurationMs = 0;
  bool ok = BoardMicRecordWav(VOICE_LAST_RECORD_PATH, sec);
  String json = VoiceIoStatusJson();
  if (!ok) {
    json.replace("\"ok\":true", "\"ok\":false");
  }
  SendCorsHeader();
  server.send(ok ? 200 : 500, "application/json", json);
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceIoAsyncRecordTask(void* param) {
#if ENABLE_VOICE_IO_FOUNDATION
  uint32_t maxSec = (uint32_t)(uintptr_t)param;
  if (maxSec < 1) maxSec = 1;
  if (maxSec > 60) maxSec = 60;
  BoardMicRecordWav(VOICE_LAST_RECORD_PATH, maxSec);
#endif
  VoiceRecordAsyncActive = false;
  VoiceRecordTaskHandle = nullptr;
  VoiceRecordMinDurationMs = 0;
  vTaskDelete(nullptr);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceIoStartApi() {
#if ENABLE_VOICE_IO_FOUNDATION
  SendCorsHeader();
  if (VoiceIO.recording || VoiceRecordAsyncActive) {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"recording already active\"}");
    return;
  }
  uint32_t maxSec = server.hasArg("max") ? (uint32_t)server.arg("max").toInt() : 60;
  if (maxSec < 1) maxSec = 1;
  if (maxSec > 60) maxSec = 60;
  VoiceMicDecodeMode = LINGXI_MIC_LOCK_FMT;
  VoiceRecordStopRequested = false;
  VoiceRecordAsyncActive = true;
  VoiceRecordAsyncStartMs = millis();
  VoiceRecordAsyncMaxSec = maxSec;
  VoiceRecordMinDurationMs = 1000;
  BaseType_t ok = xTaskCreatePinnedToCore(VoiceIoAsyncRecordTask, "VoiceIoRecord", 8192, (void*)(uintptr_t)maxSec, 2, &VoiceRecordTaskHandle, 1);
  if (ok != pdPASS) {
    VoiceRecordAsyncActive = false;
    VoiceRecordTaskHandle = nullptr;
    VoiceRecordMinDurationMs = 0;
    server.send(500, "application/json", "{\"ok\":false,\"error\":\"failed to create record task\"}");
    return;
  }
  AddCenterLog("op", "中控麦克风按住录音开始：" + VoiceMicDecodeModeName() + " max=" + String(maxSec) + "s");
  String startJson = "{\"ok\":true,\"recording\":true,\"max_sec\":" + String(maxSec) + "}";
  server.send(200, "application/json", startJson);
#else
  SendCorsHeader();
  server.send(503, "application/json", "{\"ok\":false,\"error\":\"Voice I/O disabled\"}");
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceIoStopApi() {
#if ENABLE_VOICE_IO_FOUNDATION
  SendCorsHeader();
  uint32_t heldMs = VoiceRecordAsyncStartMs ? (millis() - VoiceRecordAsyncStartMs) : 0;
  VoiceRecordStopRequested = true;
  uint32_t waitStart = millis();
  while (VoiceRecordAsyncActive && millis() - waitStart < 3500) {
    delay(20);
  }
  String json = "{";
  json += "\"ok\":" + String(!VoiceRecordAsyncActive && VoiceIO.lastRecordReady ? "true" : "false") + ",";
  json += "\"recording\":" + String(VoiceRecordAsyncActive ? "true" : "false") + ",";
  json += "\"ready\":" + String(VoiceIO.lastRecordReady ? "true" : "false") + ",";
  json += "\"held_ms\":" + String(heldMs) + ",";
  json += "\"duration_ms\":" + String(VoiceIO.lastDurationMs) + ",";
  json += "\"bytes\":" + String(VoiceIO.lastDataBytes) + ",";
  json += "\"peak\":" + String(VoiceIO.peakLevel) + ",";
  json += "\"rms\":" + String(VoiceIO.rmsLevel) + ",";
  json += "\"error\":\"" + JsonEscape(VoiceIO.lastError) + "\"";
  json += "}";
  server.send(VoiceRecordAsyncActive ? 202 : 200, "application/json", json);
#else
  SendCorsHeader();
  server.send(503, "application/json", "{\"ok\":false,\"error\":\"Voice I/O disabled\"}");
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceIoFileApi() {
#if ENABLE_VOICE_IO_FOUNDATION
  String path = VoiceIO.lastRecordFile.length() ? VoiceIO.lastRecordFile : String(VOICE_LAST_RECORD_PATH);
  if (!Voice.sdOnline || !SD_MMC.exists(path.c_str())) {
    SendCorsHeader();
    server.send(404, "text/plain", "record file not found");
    return;
  }
  File f = SD_MMC.open(path.c_str(), FILE_READ);
  if (!f) {
    SendCorsHeader();
    server.send(500, "text/plain", "failed to open record file");
    return;
  }
  SendCorsHeader();
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.streamFile(f, "audio/wav");
  f.close();
#else
  SendCorsHeader();
  server.send(503, "text/plain", "Voice I/O disabled");
#endif
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceIoDiagApi() {
#if ENABLE_VOICE_IO_FOUNDATION
  SendCorsHeader();
  String path = VoiceIO.lastRecordFile.length() ? VoiceIO.lastRecordFile : String(VOICE_LAST_RECORD_PATH);
  bool exists = Voice.sdOnline && SD_MMC.exists(path.c_str());
  uint32_t fileSize = 0;
  uint16_t wavChannels = 0;
  uint32_t wavSampleRate = 0;
  uint16_t wavBits = 0;
  uint32_t wavDataSize = 0;
  String header = "";
  if (exists) {
    File f = SD_MMC.open(path.c_str(), FILE_READ);
    if (f) {
      fileSize = f.size();
      uint8_t h[44];
      size_t n = f.read(h, sizeof(h));
      if (n == sizeof(h)) {
        header = String((char)h[0]) + String((char)h[1]) + String((char)h[2]) + String((char)h[3]) + "/" + String((char)h[8]) + String((char)h[9]) + String((char)h[10]) + String((char)h[11]);
        wavChannels = AudioReadLE16(h + 22);
        wavSampleRate = AudioReadLE32(h + 24);
        wavBits = AudioReadLE16(h + 34);
        wavDataSize = AudioReadLE32(h + 40);
      }
      f.close();
    }
  }
  String json = "{";
  json += "\"ok\":true,";
  json += "\"exists\":" + String(exists ? "true" : "false") + ",";
  json += "\"path\":\"" + JsonEscape(path) + "\",";
  json += "\"file_size\":" + String(fileSize) + ",";
  json += "\"header\":\"" + JsonEscape(header) + "\",";
  json += "\"wav_channels\":" + String(wavChannels) + ",";
  json += "\"wav_sample_rate\":" + String(wavSampleRate) + ",";
  json += "\"wav_bits\":" + String(wavBits) + ",";
  json += "\"wav_data_size\":" + String(wavDataSize) + ",";
  json += "\"mic_decode\":\"" + JsonEscape(VoiceMicDecodeModeName()) + "\",";
  json += "\"mic_decode_id\":" + String(VoiceMicDecodeMode) + ",";
  json += "\"duration_ms\":" + String(VoiceIO.lastDurationMs) + ",";
  json += "\"bytes\":" + String(VoiceIO.lastDataBytes) + ",";
  json += "\"peak\":" + String(VoiceIO.peakLevel) + ",";
  json += "\"rms\":" + String(VoiceIO.rmsLevel);
  json += "}";
  server.send(200, "application/json", json);
#else
  SendCorsHeader();
  server.send(503, "application/json", "{\"ok\":false,\"error\":\"Voice I/O disabled\"}");
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceIoPlayLastApi() {
#if ENABLE_VOICE_IO_FOUNDATION
  String path = VoiceIO.lastRecordFile.length() ? VoiceIO.lastRecordFile : String(VOICE_LAST_RECORD_PATH);
  if (!VoiceIO.lastRecordReady || !Voice.sdOnline || !SD_MMC.exists(path.c_str())) {
    VoiceIO.lastError = "no record file to play";
    String json = VoiceIoStatusJson();
    json.replace("\"ok\":true", "\"ok\":false");
    SendCorsHeader();
    server.send(404, "application/json", json);
    return;
  }
  bool shouldPlayBoard = (VoiceIO.outputTarget == VOICE_OUTPUT_BOARD_SPEAKER || VoiceIO.outputTarget == VOICE_OUTPUT_BOTH || VoiceIO.outputTarget == VOICE_OUTPUT_AUTO);
  bool ok = true;
  if (shouldPlayBoard) {
    ok = BoardAudioPlayWav(path);
    if (!ok && Voice.lastVoiceError.length()) VoiceIO.lastError = Voice.lastVoiceError;
  }
  String json = VoiceIoStatusJson();
  if (!ok) json.replace("\"ok\":true", "\"ok\":false");
  SendCorsHeader();
  server.send(ok ? 200 : 500, "application/json", json);
#else
  SendCorsHeader();
  server.send(503, "application/json", "{\"ok\":false,\"error\":\"Voice I/O disabled\"}");
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceIoSetApi() {
  if (server.hasArg("input")) {
    String v = server.arg("input");
    v.toLowerCase();
    if (v.indexOf("web") >= 0) VoiceIO.inputSource = VOICE_INPUT_WEBUI_MIC;
    else if (v.indexOf("board") >= 0 || v.indexOf("ctrl") >= 0 || v.indexOf("中控") >= 0) VoiceIO.inputSource = VOICE_INPUT_BOARD_MIC;
    else if (v.indexOf("auto") >= 0) VoiceIO.inputSource = VOICE_INPUT_AUTO;
    else VoiceIO.inputSource = VOICE_INPUT_WEBUI_MIC;
  }
  if (server.hasArg("output")) {
    String v = server.arg("output");
    v.toLowerCase();
    if (v.indexOf("web") >= 0) VoiceIO.outputTarget = VOICE_OUTPUT_WEBUI_SPEAKER;
    else if (v.indexOf("both") >= 0 || v.indexOf("dual") >= 0) VoiceIO.outputTarget = VOICE_OUTPUT_BOTH;
    else if (v.indexOf("auto") >= 0) VoiceIO.outputTarget = VOICE_OUTPUT_AUTO;
    else VoiceIO.outputTarget = VOICE_OUTPUT_BOARD_SPEAKER;
  }
  if (server.hasArg("fmt")) {
    
    
    AddCenterLog("sys", "已忽略语音设置里的 fmt 参数；正式录音保持 " + VoiceMicDecodeModeName());
  }
  if (server.hasArg("brain")) {
    String v = server.arg("brain");
    v.toLowerCase();
    if (v.indexOf("asr") >= 0) VoiceIO.brainMode = VOICE_BRAIN_CLOUD_ASR;
    else if (v.indexOf("doubao") >= 0 || v.indexOf("ai") >= 0) VoiceIO.brainMode = VOICE_BRAIN_DOUBAO_AI;
    else if (v.indexOf("sr") >= 0 || v.indexOf("local") >= 0) VoiceIO.brainMode = VOICE_BRAIN_LOCAL_ESPSR;
    else VoiceIO.brainMode = VOICE_BRAIN_RECORD_TEST;
  }
  AddCenterLog("op", "语音设置：input=" + VoiceInputSourceName() + ", output=" + VoiceOutputTargetName());
  SendCorsHeader();
  server.send(200, "application/json", VoiceIoStatusJson());
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AsrBridgeUrl() {
  return String(ASR_BRIDGE_URL);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceTaskExtractAsrText(String body) {
  body.trim();
  if (body.length() == 0) return "";

  
  
  String text = "";

  int utterPos = body.indexOf("\"utterances\"");
  int resultPos = body.indexOf("\"result\"");
  int payloadPos = body.indexOf("\"payload_msg\"");

  if (utterPos >= 0) text = JsonGetStringAfter(body, "text", utterPos);
  if (text.length() == 0 && resultPos >= 0) text = JsonGetStringAfter(body, "text", resultPos);
  if (text.length() == 0 && payloadPos >= 0) text = JsonGetStringAfter(body, "text", payloadPos);
  if (text.length() == 0) text = JsonGetString(body, "text");
  if (text.length() == 0) text = JsonGetString(body, "transcript");
  if (text.length() == 0) text = JsonGetString(body, "sentence");
  if (text.length() == 0) text = JsonGetString(body, "utterance");

  
  
  if (text.length() == 0) {
    String payload = JsonGetString(body, "payload_msg");
    payload = JsonUnescape(payload);
    payload.trim();
    if (payload.length()) {
      int pUtter = payload.indexOf("\"utterances\"");
      int pResult = payload.indexOf("\"result\"");
      if (pUtter < 0) pUtter = payload.indexOf("\"utterances\"");
      if (pResult < 0) pResult = payload.indexOf("\"result\"");
      if (pUtter >= 0) text = JsonGetStringAfter(payload, "text", pUtter);
      if (text.length() == 0 && pResult >= 0) text = JsonGetStringAfter(payload, "text", pResult);
      if (text.length() == 0) text = JsonGetString(payload, "text");
      if (text.length() == 0) text = JsonGetString(payload, "utterance");
      if (text.length() == 0) text = JsonGetString(payload, "sentence");
    }
  }

  if (text.length() == 0) {
    String resultObj = JsonGetString(body, "result");
    resultObj = JsonUnescape(resultObj);
    resultObj.trim();
    if (resultObj.length() && (resultObj.indexOf("{") >= 0 || resultObj.indexOf("[") >= 0)) {
      text = JsonGetString(resultObj, "text");
      if (text.length() == 0) text = JsonGetString(resultObj, "utterance");
      if (text.length() == 0) text = JsonGetString(resultObj, "sentence");
    }
  }

  if (text.length() == 0) {
    String result = JsonGetStringAfter(body, "result", 0);
    if (result.length() > 0 && result.indexOf("{") < 0 && result.indexOf("[") < 0) text = result;
  }

  
  if (text.length() == 0 && !body.startsWith("{") && !body.startsWith("[")) text = body;

  text = JsonUnescape(text);
  text.replace("\n", " ");
  text.replace("\r", " ");
  text.replace("\t", " ");
  text.trim();
  while (text.indexOf("  ") >= 0) text.replace("  ", " ");

  String low = text;
  low.toLowerCase();
  bool meta = false;
  if (text.startsWith("{") || text.startsWith("[") || text.endsWith("}")) meta = true;
  if (low.indexOf("additions") >= 0 || low.indexOf("duration") >= 0 || low.indexOf("audio_info") >= 0 || low.indexOf("payload_msg") >= 0) meta = true;
  if (low.indexOf("utterances") >= 0 || low.indexOf("\"result\"") >= 0) meta = true;
  if (meta) text = "";

  String compact = text;
  compact.replace(" ", "");
  compact.replace("，", "");
  compact.replace("。", "");
  compact.replace(",", "");
  compact.replace(".", "");
  if (compact.length() == 0) text = "";
  if (text.length() > 180) text = text.substring(0, 180);
  return text;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceTaskRunCloudAsr() {
#if ENABLE_CLOUD_ASR_BRIDGE
  if (WiFi.status() != WL_CONNECTED) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "STA WiFi 未连接，无法访问 ASR Bridge";
    VoiceTask.updatedMs = millis();
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }

  if (IsPlaceholderText(ASR_BRIDGE_URL)) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "ASR_BRIDGE_URL 未配置，请先填写云端中转地址";
    VoiceTask.updatedMs = millis();
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }

  String path = VoiceTask.audioFile.length() ? VoiceTask.audioFile : VoiceIO.lastRecordFile;
  if (!Voice.sdOnline || !SD_MMC.exists(path.c_str())) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "录音文件不存在，无法上传 ASR：" + path;
    VoiceTask.updatedMs = millis();
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }

  File file = SD_MMC.open(path.c_str(), FILE_READ);
  if (!file) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "录音文件打开失败：" + path;
    VoiceTask.updatedMs = millis();
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }

  String url = String(ASR_BRIDGE_URL);
  HTTPClient http;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  bool beginOk = false;

  if (url.startsWith("https://")) {
    secureClient.setInsecure();
    beginOk = http.begin(secureClient, url);
  } else {
    beginOk = http.begin(plainClient, url);
  }

  if (!beginOk) {
    file.close();
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "HTTP begin ASR Bridge 失败";
    VoiceTask.updatedMs = millis();
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }

  http.setTimeout(ASR_BRIDGE_TIMEOUT_MS);
  http.addHeader("Content-Type", "audio/wav");
  http.addHeader("X-Lingxi-Version", LINGXI_VERSION_CODE);
  http.addHeader("X-Voice-Seq", String(VoiceTask.seq));
  http.addHeader("X-Voice-Source", VoiceTask.source);
  http.addHeader("X-Voice-Duration", String(VoiceTask.durationMs));
  if (!IsPlaceholderText(ASR_BRIDGE_TOKEN)) {
    http.addHeader("Authorization", "Bearer " + String(ASR_BRIDGE_TOKEN));
  }

  VoiceTask.status = VOICE_TASK_RECORDED;
  VoiceTask.reply = "正在上传 ASR Bridge";
  VoiceTask.updatedMs = millis();
  AddCenterLog("op", "VoiceTask #" + String(VoiceTask.seq) + " 上传 ASR：" + path + " size=" + String(file.size()));

  int code = http.sendRequest("POST", &file, file.size());
  String body = http.getString();
  file.close();
  http.end();

  if (code < 200 || code >= 300) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "ASR Bridge HTTP " + String(code) + " " + body.substring(0, 160);
    VoiceTask.updatedMs = millis();
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }

  String text = VoiceTaskExtractAsrText(body);
  if (text.length() == 0) {
    VoiceTask.status = VOICE_TASK_ERROR;
    VoiceTask.error = "ASR Bridge 未返回有效 text";
    VoiceTask.updatedMs = millis();
    AddCenterLog("sys", VoiceTask.error);
    return false;
  }

  VoiceTaskSetText(text, "cloud_asr");
  VoiceTask.brain = "cloud_asr";
  VoiceTask.reply = "ASR 识别完成，等待白名单解析";
  AddCenterLog("op", "ASR识别文本：" + text);
  BemfaPublishStatus();
  return true;
#else
  VoiceTask.status = VOICE_TASK_ERROR;
  VoiceTask.error = "ENABLE_CLOUD_ASR_BRIDGE=0";
  VoiceTask.updatedMs = millis();
  return false;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceTextNormalize(String text) {
  text.trim();
  text.replace(" ", "");
  text.replace("，", "");
  text.replace("。", "");
  text.replace("！", "");
  text.replace("？", "");
  text.replace(",", "");
  text.replace(".", "");
  text.replace("!", "");
  text.replace("?", "");
  return text;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceTextHasAny(String text, const char* a, const char* b = nullptr, const char* c = nullptr, const char* d = nullptr, const char* e = nullptr, const char* f = nullptr, const char* g = nullptr, const char* h = nullptr) {
  if (a && text.indexOf(a) >= 0) return true;
  if (b && text.indexOf(b) >= 0) return true;
  if (c && text.indexOf(c) >= 0) return true;
  if (d && text.indexOf(d) >= 0) return true;
  if (e && text.indexOf(e) >= 0) return true;
  if (f && text.indexOf(f) >= 0) return true;
  if (g && text.indexOf(g) >= 0) return true;
  if (h && text.indexOf(h) >= 0) return true;
  return false;
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceTextHasExplicitControlVerb(String t) {
  return VoiceTextHasAny(t, "打开", "开启", "启动", "开一下") ||
         VoiceTextHasAny(t, "关闭", "关掉", "关上", "停止") ||
         VoiceTextHasAny(t, "切换", "进入", "退出", "调到") ||
         VoiceTextHasAny(t, "拉开", "合上", "解除报警", "取消报警");
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceTextLooksLikeQuestion(String t) {
  return VoiceTextHasAny(t, "什么", "怎么", "怎样", "哪", "推荐") ||
         VoiceTextHasAny(t, "吗", "呢", "嘛", "好看") ||
         VoiceTextHasAny(t, "为什么", "谁", "多少", "如何");
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceTextShouldStayChat(String userText) {
  String t = VoiceTextNormalize(userText);
  if (t.length() == 0) return false;
  
  bool isWeatherAsk = VoiceTextHasAny(t, "天气", "温度", "湿度", "冷不冷") || VoiceTextHasAny(t, "热不热", "下雨", "气温", "光照");
  if (isWeatherAsk) return false;
  
  if (VoiceTextLooksLikeQuestion(t) && !VoiceTextHasExplicitControlVerb(t)) return true;
  return false;
}

// 函数说明：自由对话函数，负责云端模型请求或回复处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool DoubaoShouldSuppressCommandsForText(String userText) {
  return VoiceTextShouldStayChat(userText);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String VoiceTextToCommand(String text, String &intent) {
  String t = VoiceTextNormalize(text);
  intent = "unknown";
  if (t.length() == 0) return "";

  if (VoiceTextShouldStayChat(t)) {
    intent = "chat";
    return "";
  }

  bool wantOn = VoiceTextHasAny(t, "打开", "开启", "启动", "开一下", "开灯", "开开");
  bool wantOff = VoiceTextHasAny(t, "关闭", "关掉", "关上", "停止", "关一下", "关了", "关灯");

  
  if (VoiceTextHasAny(t, "解除报警", "取消报警", "关闭报警", "停止报警")) { intent = "alarm"; return "alarm_reset"; }

  if (VoiceTextHasAny(t, "回家模式")) { intent = "scene"; return "scene_home"; }
  if (VoiceTextHasAny(t, "离家模式")) { intent = "scene"; return "scene_away"; }
  if (VoiceTextHasAny(t, "睡眠模式")) { intent = "scene"; return "scene_sleep"; }
  if (VoiceTextHasAny(t, "观影模式")) { intent = "scene"; return "scene_movie"; }
  if (VoiceTextHasAny(t, "办公模式")) { intent = "scene"; return "scene_office"; }
  if (VoiceTextHasAny(t, "舒适模式")) { intent = "scene"; return "scene_comfort"; }
  if (VoiceTextHasAny(t, "起夜模式")) { intent = "scene"; return "scene_night"; }
  if (VoiceTextHasAny(t, "报警模式")) { intent = "scene"; return "scene_alarm"; }
  if (VoiceTextHasAny(t, "退出场景", "关闭场景", "手动模式")) { intent = "scene"; return "scene_none"; }

  if ((VoiceTextHasAny(t, "切换", "换成", "换到") && VoiceTextHasAny(t, "守岸人")) || VoiceTextHasAny(t, "切换到守岸人", "换成守岸人", "换到守岸人")) { intent = "role"; return "set_role_shorekeeper"; }
  if ((VoiceTextHasAny(t, "切换", "换成", "换到") && VoiceTextHasAny(t, "卡提希娅", "卡提希亚")) || VoiceTextHasAny(t, "切换到卡提希娅", "换成卡提希娅", "换到卡提希娅", "切换到卡提希亚", "换成卡提希亚", "换到卡提希亚")) { intent = "role"; return "set_role_cartethyia"; }

  if (VoiceTextHasAny(t, "所有灯", "全部灯", "灯光全", "全屋灯")) {
    intent = "device_light";
    if (wantOff) return "light_all_off";
    if (wantOn) return "light_all_on";
  }
  bool mentionLivingLight = VoiceTextHasAny(t, "客厅灯", "客厅灯光") || (VoiceTextHasAny(t, "客厅") && VoiceTextHasAny(t, "灯", "灯光"));
  if (mentionLivingLight) {
    intent = "device_light";
    if (wantOff || VoiceTextHasAny(t, "关客厅灯", "客厅灯关", "客厅的灯关", "把客厅灯关")) return "light_living_off";
    if (wantOn || VoiceTextHasAny(t, "开客厅灯", "客厅灯开", "客厅的灯开", "把客厅灯开")) return "light_living_on";
  }
  bool mentionBedLight = VoiceTextHasAny(t, "卧室灯", "卧室灯光") || (VoiceTextHasAny(t, "卧室") && VoiceTextHasAny(t, "灯", "灯光"));
  if (mentionBedLight) {
    intent = "device_light";
    if (wantOff || VoiceTextHasAny(t, "关卧室灯", "卧室灯关", "卧室的灯关", "把卧室灯关")) return "light_bed_off";
    if (wantOn || VoiceTextHasAny(t, "开卧室灯", "卧室灯开", "卧室的灯开", "把卧室灯开")) return "light_bed_on";
  }
  
  if (VoiceTextHasAny(t, "灯", "灯光")) {
    intent = "device_light_default";
    if (wantOff) return "light_living_off";
    if (wantOn) return "light_living_on";
  }

  if (VoiceTextHasAny(t, "空调")) {
    intent = "device_relay";
    if (wantOff) return "relay_air_off";
    if (wantOn) return "relay_air_on";
  }
  if (VoiceTextHasAny(t, "电视")) {
    intent = "device_relay";
    if (wantOff) return "relay_tv_off";
    if (wantOn) return "relay_tv_on";
  }
  if (VoiceTextHasAny(t, "冰箱")) {
    intent = "device_relay";
    if (wantOff) return "relay_fridge_off";
    if (wantOn) return "relay_fridge_on";
  }
  if (VoiceTextHasAny(t, "热水器")) {
    intent = "device_relay";
    if (wantOff) return "relay_water_heater_off";
    if (wantOn) return "relay_water_heater_on";
  }

  if (VoiceTextHasAny(t, "所有窗帘", "全部窗帘", "窗帘全")) {
    intent = "device_curtain";
    if (wantOff || VoiceTextHasAny(t, "合上", "拉上", "关窗帘")) return "curtain_all_close";
    if (wantOn || VoiceTextHasAny(t, "拉开", "开窗帘")) return "curtain_all_open";
  }
  if (VoiceTextHasAny(t, "客厅窗帘")) {
    intent = "device_curtain";
    if (wantOff || VoiceTextHasAny(t, "合上", "拉上")) return "curtain_living_close";
    if (wantOn || VoiceTextHasAny(t, "拉开")) return "curtain_living_open";
  }
  if (VoiceTextHasAny(t, "卧室窗帘")) {
    intent = "device_curtain";
    if (wantOff || VoiceTextHasAny(t, "合上", "拉上")) return "curtain_bed_close";
    if (wantOn || VoiceTextHasAny(t, "拉开")) return "curtain_bed_open";
  }
  if (VoiceTextHasAny(t, "窗帘")) {
    
    intent = "device_curtain";
    if (wantOff || VoiceTextHasAny(t, "合上", "拉上")) return "curtain_living_close";
    if (wantOn || VoiceTextHasAny(t, "拉开")) return "curtain_living_open";
  }

  if (VoiceTextHasAny(t, "窗户")) {
    intent = "device_window";
    if (wantOff || VoiceTextHasAny(t, "关窗", "关上窗")) return "window_close";
    if (wantOn || VoiceTextHasAny(t, "开窗", "打开窗")) return "window_open";
  }

  return "";
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceTaskParseCurrentText(bool executeNow) {
  LingxiVoiceRouteResult route;
  if (ConfirmAgentTryRoute(VoiceTask.text, route)) {
    VoiceTask.intent = route.intent.length() ? route.intent : "confirm_agent";
    VoiceTask.command = route.command;
    VoiceTask.commandReady = route.command.length() > 0;
    VoiceTask.replyReady = true;
    VoiceTask.reply = route.reply.length() ? route.reply : "我理解到一个可能的设备动作，需要你确认后再执行。";
    VoiceTask.updatedMs = millis();

    if (executeNow && route.command.length() && !route.needConfirm) {
      String execReply = ExecuteUnifiedCommand(route.command, "VoiceRuleEngine");
      VoiceTask.status = VOICE_TASK_EXECUTED;
      VoiceTask.executed = true;
      VoiceTask.reply = execReply.length() ? execReply : VoiceTask.reply;
      VoiceTask.replyReady = true;
      LastCloudCommand = route.command;
      LastCloudReply = VoiceTask.reply;
      AddCenterLog("op", "语音规则引擎确认执行：" + route.command + " => " + VoiceTask.reply);
      BemfaPublishStatus();
      return true;
    }

    VoiceTask.status = VOICE_TASK_REPLIED;
    AddCenterLog("op", String(route.needConfirm ? "语音规则引擎等待确认：" : "语音规则引擎回复：") + VoiceTask.text);
    return true;
  }

  String intent = "";
  String cmd = VoiceTextToCommand(VoiceTask.text, intent);
  if (cmd.length() == 0) {
    VoiceTask.intent = VoiceTextShouldStayChat(VoiceTask.text) ? "chat" : "free_chat";
    VoiceTask.command = "";
    VoiceTask.commandReady = false;
    VoiceTask.replyReady = false;
    VoiceTask.reply = "";
    VoiceTask.updatedMs = millis();
    AddCenterLog("op", "语音文本未命中本地命令，交给自由对话：" + VoiceTask.text);
    return false;
  }

  VoiceTaskSetCommand(cmd, intent);
  AddCenterLog("op", "语音白名单解析：" + VoiceTask.text + " => " + cmd);

  if (!executeNow) return true;

  String reply = ExecuteUnifiedCommand(cmd, "VoiceASR");
  VoiceTask.status = VOICE_TASK_EXECUTED;
  VoiceTask.executed = true;
  VoiceTask.replyReady = true;
  VoiceTask.reply = reply;
  VoiceTask.updatedMs = millis();
  LastCloudCommand = cmd;
  LastCloudReply = reply;
  AddCenterLog("op", "语音命令执行：" + cmd + " => " + reply);
  BemfaPublishStatus();
  return true;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskAsrApi() {
  bool ok = VoiceTaskRunCloudAsr();
  String json = VoiceTaskStatusJson();
  if (!ok) json.replace("\"ok\":true", "\"ok\":false");
  SendCorsHeader();
  server.send(ok ? 200 : 500, "application/json", json);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskParseApi() {
  bool executeNow = false;
  if (server.hasArg("execute")) {
    String e = server.arg("execute");
    e.toLowerCase();
    executeNow = (e == "1" || e == "true" || e == "yes");
  }
  bool ok = VoiceTaskParseCurrentText(executeNow);
  String json = VoiceTaskStatusJson();
  if (!ok) json.replace("\"ok\":true", "\"ok\":false");
  SendCorsHeader();
  server.send(ok ? 200 : 422, "application/json", json);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceTaskAsrExecuteApi() {
  bool ok = VoiceTaskRunCloudAsr();
  if (ok) ok = VoiceTaskParseCurrentText(true);
  String json = VoiceTaskStatusJson();
  if (!ok) json.replace("\"ok\":true", "\"ok\":false");
  SendCorsHeader();
  server.send(ok ? 200 : 500, "application/json", json);
}

