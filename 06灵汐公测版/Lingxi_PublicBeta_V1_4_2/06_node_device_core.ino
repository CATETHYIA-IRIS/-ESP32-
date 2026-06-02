/*
 * 文件：06_node_device_core.ino
 * 标题：节点与端点管理核心
 * 说明：负责节点注册、端点模板、运行态节点和设备状态汇总。
 * 可改：可新增端点模板
 * 禁止：改已有 18 个标准端点 ID。
 */

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
static inline uint16_t AudioReadLE16(uint8_t* p) {
  return ((uint16_t)p[1] << 8) | p[0];
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
static inline uint32_t AudioReadLE32(uint8_t* p) {
  return ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | p[0];
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
static inline void AudioWriteLE32(uint8_t* p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)((v >> 8) & 0xFF);
  p[2] = (uint8_t)((v >> 16) & 0xFF);
  p[3] = (uint8_t)((v >> 24) & 0xFF);
}

typedef struct {
  uint16_t audioFormat;
  uint16_t channels;
  uint32_t sampleRate;
  uint16_t bitsPerSample;
  uint32_t dataOffset;
  uint32_t dataSize;
} WavInfo;


bool AudioParseWav(File &file, WavInfo &info);
bool AudioFixWavHeaderDataSize(String path);


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ES8311WriteReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ES8311_I2C_ADDR);
  Wire.write(reg);
  Wire.write(val);
  bool ok = (Wire.endTransmission() == 0);
  if (!ok) {
    Serial.printf("[ES8311] write reg 0x%02X failed\n", reg);
  }
  return ok;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
uint8_t ES8311ReadReg(uint8_t reg) {
  Wire.beginTransmission(ES8311_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFF;
  Wire.requestFrom(ES8311_I2C_ADDR, (uint8_t)1);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ES8311Probe() {
  Wire.begin(BOARD_I2C_SDA_PIN, BOARD_I2C_SCL_PIN);
  Wire.setClock(400000);
  Wire.beginTransmission(ES8311_I2C_ADDR);
  bool ok = (Wire.endTransmission() == 0);
  Serial.printf("[ES8311] probe addr 0x%02X: %s\n", ES8311_I2C_ADDR, ok ? "OK" : "FAIL");
  return ok;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool ES8311BasicInit(uint32_t sampleRate) {
  if (!ES8311Probe()) {
    Voice.lastVoiceError = "ES8311 I2C 探测失败：请确认板载 codec 地址/供电";
    return false;
  }

  
  IoExtSetPin(IOEXT_PA_CTRL_PIN, true);
  IoExtSetPin(IOEXT_SD_PA_SAFE_PIN, true);

  bool ok = true;

  
  
  ok &= ES8311WriteReg(0x00, 0x1F);  
  delay(20);
  ok &= ES8311WriteReg(0x00, 0x80);  
  delay(20);

  
  ok &= ES8311WriteReg(0x01, 0x3F);

  
  
  ok &= ES8311WriteReg(0x02, 0x00);  
  ok &= ES8311WriteReg(0x03, 0x10);  
  ok &= ES8311WriteReg(0x04, 0x10);  
  ok &= ES8311WriteReg(0x05, 0x11);  
  ok &= ES8311WriteReg(0x06, 0x04);  
  ok &= ES8311WriteReg(0x07, 0x00);  
  ok &= ES8311WriteReg(0x08, 0xFF);  

  
  ok &= ES8311WriteReg(0x09, 0x0C);  
  ok &= ES8311WriteReg(0x0A, 0x0C);  

  
  ok &= ES8311WriteReg(0x0B, 0x00);
  ok &= ES8311WriteReg(0x0C, 0x00);
  ok &= ES8311WriteReg(0x0D, 0x01);
  ok &= ES8311WriteReg(0x10, 0x1F);
  ok &= ES8311WriteReg(0x11, 0x7F);
  ok &= ES8311WriteReg(0x12, 0x00);
  ok &= ES8311WriteReg(0x13, 0x10);
  ok &= ES8311WriteReg(0x14, 0x1A);
  ok &= ES8311WriteReg(0x17, 0xD2);

  
  ok &= ES8311WriteReg(0x31, 0x00);
  ok &= ES8311WriteReg(0x32, BOARD_AUDIO_VOLUME);

  
  ok &= ES8311WriteReg(0x37, 0x08);
  ok &= ES8311WriteReg(0x00, 0x80);

  Serial.printf("[ES8311] init sampleRate=%lu result=%s\n", (unsigned long)sampleRate, ok ? "OK" : "FAIL");
  return ok;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool AudioI2SBegin(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample) {
  if (bitsPerSample != 16) {
    Voice.lastVoiceError = "当前硬件播放仅支持 16-bit PCM WAV";
    return false;
  }

  if (BoardAudioReady && BoardAudioSampleRate == sampleRate && BoardAudioChannels == channels && BoardAudioBits == bitsPerSample) {
    return true;
  }

  if (BoardAudioReady) {
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    BoardAudioReady = false;
  }

  i2s_config_t i2s_config;
  memset(&i2s_config, 0, sizeof(i2s_config));
  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  i2s_config.sample_rate = sampleRate;
  i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;  
  i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;  
  i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2s_config.dma_buf_count = 12;
  i2s_config.dma_buf_len = 512;
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = true;
  i2s_config.fixed_mclk = sampleRate * 256;

  esp_err_t err = i2s_driver_install(AUDIO_I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Voice.lastVoiceError = "I2S driver install failed: " + String((int)err);
    Serial.println("[I2S] driver install failed");
    return false;
  }

  i2s_pin_config_t pin_config;
  memset(&pin_config, 0, sizeof(pin_config));
  pin_config.mck_io_num = AUDIO_I2S_MCLK_PIN;
  pin_config.bck_io_num = AUDIO_I2S_BCLK_PIN;
  pin_config.ws_io_num = AUDIO_I2S_LRCK_PIN;
  pin_config.data_out_num = AUDIO_I2S_DOUT_PIN;
  pin_config.data_in_num = I2S_PIN_NO_CHANGE;

  err = i2s_set_pin(AUDIO_I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Voice.lastVoiceError = "I2S set pin failed: " + String((int)err);
    Serial.println("[I2S] set pin failed");
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    return false;
  }

  err = i2s_set_clk(AUDIO_I2S_PORT, sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
  if (err != ESP_OK) {
    Voice.lastVoiceError = "I2S set clock failed: " + String((int)err);
    Serial.println("[I2S] set clock failed");
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    return false;
  }

  i2s_zero_dma_buffer(AUDIO_I2S_PORT);

  if (!ES8311BasicInit(sampleRate)) {
    i2s_driver_uninstall(AUDIO_I2S_PORT);
    BoardAudioReady = false;
    return false;
  }

  BoardAudioReady = true;
  BoardAudioSampleRate = sampleRate;
  BoardAudioChannels = channels;
  BoardAudioBits = bitsPerSample;
  return true;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool BoardAudioInit(uint32_t sampleRate, uint8_t channels, uint8_t bitsPerSample) {
  return AudioI2SBegin(sampleRate, channels, bitsPerSample);
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool AudioFixWavHeaderDataSize(String path) {
  File file = SD_MMC.open(path.c_str(), "r+");
  if (!file) return false;

  uint8_t head[12];
  file.seek(0);
  if (file.read(head, 12) != 12 || memcmp(head, "RIFF", 4) != 0 || memcmp(head + 8, "WAVE", 4) != 0) {
    file.close();
    return false;
  }

  int32_t dataSizePos = -1;
  uint32_t dataOffset = 0;
  while (file.available()) {
    uint8_t hdr[8];
    if (file.read(hdr, 8) != 8) break;
    uint32_t chunkSize = AudioReadLE32(hdr + 4);
    uint32_t chunkStart = file.position();
    if (memcmp(hdr, "data", 4) == 0) {
      dataSizePos = (int32_t)chunkStart - 4;
      dataOffset = chunkStart;
      break;
    }
    uint32_t next = chunkStart + chunkSize + (chunkSize & 1);
    if (next <= chunkStart) break;
    file.seek(next);
  }

  uint32_t fileSize = file.size();
  if (dataSizePos < 0 || dataOffset >= fileSize || fileSize < 44) {
    file.close();
    return false;
  }

  uint32_t riffSize = fileSize - 8;
  uint32_t dataSize = fileSize - dataOffset;
  uint8_t le[4];
  AudioWriteLE32(le, riffSize);
  file.seek(4);
  file.write(le, 4);
  AudioWriteLE32(le, dataSize);
  file.seek(dataSizePos);
  file.write(le, 4);
  file.flush();
  file.close();
  Serial.printf("[Audio] WAV header fixed %s riff=%lu data=%lu\n", path.c_str(), (unsigned long)riffSize, (unsigned long)dataSize);
  return true;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool AudioParseWav(File &file, WavInfo &info) {
  memset(&info, 0, sizeof(info));
  uint8_t buf[12];

  file.seek(0);
  if (file.read(buf, 12) != 12) return false;
  if (memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0) return false;

  bool gotFmt = false;
  bool gotData = false;

  while (file.available()) {
    uint8_t hdr[8];
    if (file.read(hdr, 8) != 8) break;
    uint32_t chunkSize = AudioReadLE32(hdr + 4);
    uint32_t chunkStart = file.position();

    if (memcmp(hdr, "fmt ", 4) == 0) {
      uint8_t fmt[24];
      uint32_t toRead = chunkSize > sizeof(fmt) ? sizeof(fmt) : chunkSize;
      if (file.read(fmt, toRead) != (int)toRead) return false;
      info.audioFormat = AudioReadLE16(fmt + 0);
      info.channels = AudioReadLE16(fmt + 2);
      info.sampleRate = AudioReadLE32(fmt + 4);
      info.bitsPerSample = AudioReadLE16(fmt + 14);
      gotFmt = true;
    } else if (memcmp(hdr, "data", 4) == 0) {
      info.dataOffset = file.position();
      info.dataSize = chunkSize;
      gotData = true;
    }

    uint32_t next = chunkStart + chunkSize + (chunkSize & 1);
    file.seek(next);
    if (gotFmt && gotData) break;
  }

  if (!gotFmt || !gotData) return false;

  
  
  
  uint32_t fileSize = file.size();
  if (info.dataOffset < fileSize) {
    uint32_t maxData = fileSize - info.dataOffset;
    if (info.dataSize == 0xFFFFFFFFUL || info.dataSize == 0 || info.dataSize > maxData || (maxData > info.dataSize + 1024)) {
      Serial.printf("[Audio] fix dataSize %lu -> %lu, fileSize=%lu, dataOffset=%lu\n",
                    (unsigned long)info.dataSize,
                    (unsigned long)maxData,
                    (unsigned long)fileSize,
                    (unsigned long)info.dataOffset);
      info.dataSize = maxData;
    }
  }

  if (info.audioFormat != 1) return false;  
  if (info.bitsPerSample != 16) return false;
  if (info.channels < 1 || info.channels > 2) return false;
  if (info.sampleRate < 8000 || info.sampleRate > 48000) return false;
  return true;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool BoardAudioPlayWav(String path) {
#if ENABLE_BOARD_AUDIO_PLAYBACK
  File file = SD_MMC.open(path.c_str(), "r");
  if (!file) {
    Voice.lastVoiceError = "硬件播放失败：无法打开 " + path;
    return false;
  }

  WavInfo info;
  if (!AudioParseWav(file, info)) {
    Voice.lastVoiceError = "硬件播放失败：WAV格式不支持，仅支持 PCM/16bit/mono或stereo/8-48kHz";
    file.close();
    AddCenterLog("sys", Voice.lastVoiceError);
    return false;
  }

  Serial.printf("[Audio] WAV %s rate=%lu ch=%u bits=%u data=%lu\n",
                path.c_str(), (unsigned long)info.sampleRate, info.channels, info.bitsPerSample, (unsigned long)info.dataSize);

  
  if (!BoardAudioInit(info.sampleRate, 2, info.bitsPerSample)) {
    file.close();
    AddCenterLog("sys", Voice.lastVoiceError);
    return false;
  }

  file.seek(info.dataOffset);
  uint8_t* buf = (uint8_t*)heap_caps_malloc(BOARD_AUDIO_BUF_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!buf) buf = (uint8_t*)malloc(BOARD_AUDIO_BUF_SIZE);

  uint8_t* stereoBuf = nullptr;
  if (info.channels == 1) {
    stereoBuf = (uint8_t*)heap_caps_malloc(BOARD_AUDIO_BUF_SIZE * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!stereoBuf) stereoBuf = (uint8_t*)malloc(BOARD_AUDIO_BUF_SIZE * 2);
  }

  if (!buf || (info.channels == 1 && !stereoBuf)) {
    Voice.lastVoiceError = "硬件播放失败：音频缓冲区申请失败";
    if (buf) free(buf);
    if (stereoBuf) free(stereoBuf);
    file.close();
    return false;
  }

  uint32_t remain = info.dataSize;
  size_t written = 0;
  uint32_t totalWritten = 0;
  uint32_t totalRead = 0;

  while (remain > 0) {
    size_t maxNeed = (info.channels == 1) ? (BOARD_AUDIO_BUF_SIZE & ~1) : BOARD_AUDIO_BUF_SIZE;
    size_t need = remain > maxNeed ? maxNeed : remain;
    if (info.channels == 1) need &= ~1;
    if (need == 0) break;

    int r = file.read(buf, need);
    if (r <= 0) break;
    totalRead += r;

    uint8_t* writePtr = buf;
    size_t writeLen = r;

    if (info.channels == 1) {
      int samples = r / 2;
      int16_t* in = (int16_t*)buf;
      int16_t* out = (int16_t*)stereoBuf;
      for (int i = 0; i < samples; i++) {
        int16_t s = in[i];
        out[i * 2] = s;
        out[i * 2 + 1] = s;
      }
      writePtr = stereoBuf;
      writeLen = samples * 4;
    }

    esp_err_t err = i2s_write(AUDIO_I2S_PORT, writePtr, writeLen, &written, portMAX_DELAY);
    if (err != ESP_OK) {
      Voice.lastVoiceError = "I2S write failed: " + String((int)err);
      break;
    }

    remain -= r;
    totalWritten += written;
    
  }

  
  
  uint32_t silenceBytes = (info.sampleRate * 2UL * 2UL * 900UL) / 1000UL;
  uint32_t silenceWritten = 0;
  if (info.channels == 1 && stereoBuf) {
    memset(stereoBuf, 0, BOARD_AUDIO_BUF_SIZE * 2);
    while (silenceWritten < silenceBytes) {
      size_t n = min((uint32_t)(BOARD_AUDIO_BUF_SIZE * 2), silenceBytes - silenceWritten);
      i2s_write(AUDIO_I2S_PORT, stereoBuf, n, &written, 100 / portTICK_PERIOD_MS);
      silenceWritten += written;
      if (written == 0) break;
    }
  } else {
    memset(buf, 0, BOARD_AUDIO_BUF_SIZE);
    while (silenceWritten < silenceBytes) {
      size_t n = min((uint32_t)BOARD_AUDIO_BUF_SIZE, silenceBytes - silenceWritten);
      i2s_write(AUDIO_I2S_PORT, buf, n, &written, 100 / portTICK_PERIOD_MS);
      silenceWritten += written;
      if (written == 0) break;
    }
  }

  
  vTaskDelay(180 / portTICK_PERIOD_MS);

  if (stereoBuf) free(stereoBuf);
  free(buf);
  file.close();

  bool ok = (remain == 0);
  Serial.printf("[Audio] play %s, read=%lu/%lu, i2sWritten=%lu, result=%s\n",
                path.c_str(),
                (unsigned long)totalRead,
                (unsigned long)info.dataSize,
                (unsigned long)totalWritten,
                ok ? "OK" : "FAIL");
  return ok;
#else
  (void)path;
  return false;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceHardwarePlayPath(String path) {
#if ENABLE_BOARD_AUDIO_PLAYBACK
  bool ok = BoardAudioPlayWav(path);
  if (!ok && Voice.lastVoiceError.length() == 0) {
    Voice.lastVoiceError = "ES8311硬件播放失败";
  }
  return ok;
#else
  (void)path;
  return false;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool BoardAudioPlayTtsFileStable(String path, String source) {
#if ENABLE_BOARD_AUDIO_PLAYBACK
  if (!Voice.sdOnline) {
    Voice.lastVoiceError = "中控TTS播放失败：SD未挂载";
    AddCenterLog("sys", Voice.lastVoiceError);
    return false;
  }
  if (!SD_MMC.exists(path.c_str())) {
    Voice.lastVoiceError = "中控TTS播放失败：文件不存在 " + path;
    AddCenterLog("sys", Voice.lastVoiceError);
    return false;
  }
  AudioFixWavHeaderDataSize(path);
  File f = SD_MMC.open(path.c_str(), FILE_READ);
  size_t fileSize = f ? f.size() : 0;
  if (f) f.close();
  if (fileSize < 44) {
    Voice.lastVoiceError = "中控TTS播放失败：WAV文件过小 " + String(fileSize);
    AddCenterLog("sys", Voice.lastVoiceError);
    return false;
  }
  AddCenterLog("op", source + " 准备中控播放AI语音：" + path + " size=" + String(fileSize));
  
  vTaskDelay(500 / portTICK_PERIOD_MS);
  Voice.lastVoiceError = "";
  bool ok = BoardAudioPlayWav(path);
  AddCenterLog(ok ? "op" : "sys", ok ? (source + " 中控AI语音播放完成") : (source + " 中控AI语音播放失败：" + Voice.lastVoiceError));
  return ok;
#else
  (void)path;
  (void)source;
  return false;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskBoardTtsPlay(void *pvParameters) {
  (void)pvParameters;
  String path = BoardTtsPlayPath;
  String source = BoardTtsPlaySource.length() ? BoardTtsPlaySource : String("TTSAsync");
  BoardTtsPlayActive = true;
  BoardAudioPlayTtsFileStable(path, source);
  BoardTtsPlayActive = false;
  BoardTtsPlayTaskHandle = nullptr;
  vTaskDelete(nullptr);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool BoardAudioQueueTtsPlayback(String path, String source) {
#if ENABLE_BOARD_AUDIO_PLAYBACK
  if (BoardTtsPlayActive || BoardTtsPlayTaskHandle != nullptr) {
    AddCenterLog("sys", "中控AI语音播放队列正忙，忽略新的播放请求：" + path);
    return false;
  }
  BoardTtsPlayPath = path;
  BoardTtsPlaySource = source;
  BaseType_t ok = xTaskCreatePinnedToCore(TaskBoardTtsPlay, "BoardTtsPlay", 8192, nullptr, 1, &BoardTtsPlayTaskHandle, 1);
  if (ok != pdPASS) {
    BoardTtsPlayTaskHandle = nullptr;
    AddCenterLog("sys", "中控AI语音播放任务创建失败，退回同步播放");
    return BoardAudioPlayTtsFileStable(path, source + "SyncFallback");
  }
  AddCenterLog("op", "中控AI语音已加入异步播放队列：" + path);
  return true;
#else
  (void)path;
  (void)source;
  return false;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceRequestPlay(uint16_t id, const char* source) {
  if (id == 0) {
    Voice.lastVoiceError = "voice id is 0";
    return false;
  }

  String path = VoiceFindFileById(id, Voice.currentRole);
  if (path.length() == 0) {
    Voice.audioBusy = false;
    Voice.currentVoiceId = id;
    Voice.currentVoiceFile = "";
    Voice.lastVoiceError = "未找到语音文件 ID " + String(id) + " / " + VoiceRoleName();
    AddCenterLog("sys", Voice.lastVoiceError);
    BemfaPublishLog("voice", Voice.lastVoiceError);
    BemfaPublishStatus();
    return false;
  }

  Voice.currentVoiceId = id;
  Voice.currentVoiceFile = path;
  Voice.audioBusy = true;
  Voice.lastVoiceError = "";
  Voice.lastVoiceMs = millis();

  bool webuiOnly = (VoiceIO.outputTarget == VOICE_OUTPUT_WEBUI_SPEAKER);
  bool hw = true;
  if (webuiOnly) {
    AddCenterLog("op", String(source ? source : "Voice") + " 已定位语音 " + VoiceRoleName() + " #" + String(id) + "；当前扬声器=WebUI，不调用中控扬声器");
  } else {
    hw = VoiceHardwarePlayPath(path);
    if (hw) {
      AddCenterLog("op", String(source ? source : "Voice") + " 中控播报 " + VoiceRoleName() + " #" + String(id));
    } else {
      AddCenterLog("op", String(source ? source : "Voice") + " 已定位语音 " + VoiceRoleName() + " #" + String(id) + "；ES8311未成功出声，平板/浏览器仍可播放");
    }
  }

  Voice.audioBusy = false;
  BemfaPublishLog("voice", "play_voice_" + String(id) + " => " + path);
  BemfaPublishStatus();
  return true;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool VoiceRequestPlayOnBoard(uint16_t id, const char* source) {
  if (id == 0) {
    Voice.lastVoiceError = "voice id is 0";
    return false;
  }

  String path = VoiceFindFileById(id, Voice.currentRole);
  if (path.length() == 0) {
    Voice.currentVoiceId = id;
    Voice.currentVoiceFile = "";
    Voice.lastVoiceError = "未找到语音文件 ID " + String(id) + " / " + VoiceRoleName();
    AddCenterLog("sys", Voice.lastVoiceError);
    return false;
  }

  Voice.currentVoiceId = id;
  Voice.currentVoiceFile = path;
  Voice.audioBusy = true;
  Voice.lastVoiceError = "";
  Voice.lastVoiceMs = millis();

  bool hw = VoiceHardwarePlayPath(path);
  if (hw) {
    AddCenterLog("op", String(source ? source : "V4BoardVoice") + " 中控本机播报 " + VoiceRoleName() + " #" + String(id));
  } else {
    AddCenterLog("sys", String(source ? source : "V4BoardVoice") + " 中控本机播报失败：" + Voice.lastVoiceError);
  }

  Voice.audioBusy = false;
  BemfaPublishLog("voice", String(source ? source : "V4BoardVoice") + " => " + path);
  BemfaPublishStatus();
  return hw;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
uint16_t VoiceIdForCommand(String cmd, String reply) {
  bool offOrClose = ReplyMeansOffOrClose(reply);
  if (cmd == "light_living_on") return 101;
  if (cmd == "light_living_off") return 102;
  if (cmd == "light_bed_on") return 103;
  if (cmd == "light_bed_off") return 104;
  if (cmd == "light_all_on") return 101;
  if (cmd == "light_all_off") return 102;
  if (cmd == "relay_air" || cmd == "relay_air_on") return offOrClose ? 106 : 105;
  if (cmd == "relay_air_off") return 106;
  if (cmd == "relay_tv" || cmd == "relay_tv_on") return offOrClose ? 108 : 107;
  if (cmd == "relay_tv_off") return 108;
  if (cmd == "relay_fridge" || cmd == "relay_fridge_on") return offOrClose ? 110 : 109;
  if (cmd == "relay_fridge_off") return 110;
  if (cmd == "relay_water_heater" || cmd == "relay_water_heater_on") return offOrClose ? 112 : 111;
  if (cmd == "relay_water_heater_off") return 112;
  if (cmd == "relay_3" || cmd == "curtain_living_open") return offOrClose ? 114 : 113;
  if (cmd == "curtain_living_close") return 114;
  if (cmd == "relay_4" || cmd == "curtain_bed_open") return offOrClose ? 116 : 115;
  if (cmd == "curtain_bed_close") return 116;
  if (cmd == "curtain_all_open") return 113;
  if (cmd == "curtain_all_close") return 114;
  if (cmd == "window_toggle" || cmd == "window_open") return offOrClose ? 118 : 117;
  if (cmd == "window_close") return 118;
  if (cmd == "scene_home") return 201;
  if (cmd == "scene_away") return 202;
  if (cmd == "scene_sleep") return 203;
  if (cmd == "scene_movie") return 204;
  if (cmd == "scene_office") return 205;
  if (cmd == "scene_comfort") return 206;
  if (cmd == "scene_night") return 207;
  if (cmd == "scene_alarm") return 208;
  if (cmd == "scene_none") return 210;
  if (cmd == "alarm_reset") return 309;
  if (cmd == "cloud_report") return 18;
  return 0;
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void VoiceAutoFeedback(String cmd, String reply, const char* source) {
#if ENABLE_VOICE_AUTO_FEEDBACK
  if (source && String(source).indexOf("NoVoice") >= 0) return;
  if (cmd.startsWith("play_voice_")) return;
  if (cmd.startsWith("set_role_")) return;
  if (cmd == "audio_reload" || cmd == "audio_stop") return;
  uint16_t id = VoiceIdForCommand(cmd, reply);
  if (id > 0) {
    String src = source ? String(source) : "AutoVoice";
    if (src.indexOf("V4LocalVoice") >= 0 || src.indexOf("V4Wake") >= 0) {
      VoiceRequestPlayOnBoard(id, source ? source : "V4BoardAutoVoice");
    } else {
      VoiceRequestPlay(id, source ? source : "AutoVoice");
    }
  }
#else
  (void)cmd;
  (void)reply;
  (void)source;
#endif
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceStatusApi() {
  String json = "{";
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
  json += "\"VoiceLastRecordReady\":" + String(VoiceIO.lastRecordReady ? "true" : "false") + ",";
  json += "\"VoiceLastRecordFile\":\"" + JsonEscape(VoiceIO.lastRecordFile) + "\",";
  json += "\"HeadlessDisplayOff\":" + String(HeadlessDisplayIsOff ? "true" : "false") + ",";
  json += "\"BoardAudioEnabled\":" + String(ENABLE_BOARD_AUDIO_PLAYBACK ? "true" : "false") + ",";
  json += "\"BoardAudioReady\":" + String(BoardAudioReady ? "true" : "false") + ",";
  json += "\"BoardAudioSampleRate\":" + String(BoardAudioSampleRate) + ",";
  json += "\"VoiceInputSource\":\"" + VoiceInputSourceName() + "\",";
  json += "\"VoiceOutputTarget\":\"" + VoiceOutputTargetName() + "\",";
  json += "\"VoiceBrainMode\":\"" + VoiceBrainModeName() + "\",";
  json += "\"VoiceMicReady\":" + String(VoiceIO.micReady ? "true" : "false") + ",";
  json += "\"VoiceLastRecordReady\":" + String(VoiceIO.lastRecordReady ? "true" : "false") + ",";
  json += "\"VoiceLastRecordFile\":\"" + JsonEscape(VoiceIO.lastRecordFile) + "\"";
  json += "}";
  SendCorsHeader();
  server.send(200, "application/json", json);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceFileApi() {
#if ENABLE_SD_VOICE
  uint16_t id = server.hasArg("id") ? (uint16_t)server.arg("id").toInt() : Voice.currentVoiceId;
  uint8_t role = Voice.currentRole;
  String customDir = "";
  if (server.hasArg("role")) {
    String r = server.arg("role");
    String rid = RoleIdNormalize(r, "");
    if (rid.length()) customDir = String(VOICE_ROOT_DIR) + "/" + rid;
    r.toLowerCase();
    if (r.indexOf("cart") >= 0 || r.indexOf("kati") >= 0) role = ROLE_CARTETHYIA;
    if (r.indexOf("shore") >= 0 || r.indexOf("shou") >= 0) role = ROLE_SHOREKEEPER;
  }
  String path = customDir.length() ? VoiceFindFileByIdInDir(id, customDir) : "";
  if (path.length() == 0) path = VoiceFindFileById(id, role);
  if (path.length() == 0) {
    SendCorsHeader();
    server.send(404, "text/plain", "voice file not found");
    return;
  }
  File f = SD_MMC.open(path.c_str(), FILE_READ);
  if (!f) {
    SendCorsHeader();
    server.send(500, "text/plain", "failed to open voice file");
    return;
  }
  SendCorsHeader();
  server.streamFile(f, "audio/wav");
  f.close();
#else
  SendCorsHeader();
  server.send(503, "text/plain", "SD voice disabled");
#endif
}


// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoicePlayApi() {
  if (!server.hasArg("id")) {
    SendCorsHeader();
    server.send(400, "text/plain", "missing id");
    return;
  }
  uint16_t id = (uint16_t)server.arg("id").toInt();
  bool ok = VoiceRequestPlay(id, "WebUI");
  SendCorsHeader();
  server.send(ok ? 200 : 404, "text/plain", ok ? "voice play requested" : Voice.lastVoiceError);
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleVoiceListApi() {
#if ENABLE_SD_VOICE
  if (!Voice.sdOnline) {
    SendCorsHeader();
    server.send(503, "application/json", "{\"error\":\"sd offline\"}");
    return;
  }
  uint8_t role = Voice.currentRole;
  String customDir = "";
  if (server.hasArg("role")) {
    String r = server.arg("role");
    String rid = RoleIdNormalize(r, "");
    if (rid.length()) customDir = String(VOICE_ROOT_DIR) + "/" + rid;
    r.toLowerCase();
    if (r.indexOf("cart") >= 0) role = ROLE_CARTETHYIA;
    if (r.indexOf("shore") >= 0 || r.indexOf("shou") >= 0) role = ROLE_SHOREKEEPER;
  }
  String dirPath = customDir.length() ? customDir : VoiceRoleDir(role);
  File dir = SD_MMC.open(dirPath.c_str());
  String json = "[";
  bool first = true;
  if (dir && dir.isDirectory()) {
    File f = dir.openNextFile();
    while (f) {
      if (!f.isDirectory()) {
        String name = VoiceBasename(String(f.name()));
        String lower = name;
        lower.toLowerCase();
        if (lower.endsWith(".wav")) {
          if (!first) json += ",";
          json += "\"" + JsonEscape(name) + "\"";
          first = false;
        }
      }
      f.close();
      f = dir.openNextFile();
    }
    dir.close();
  }
  json += "]";
  SendCorsHeader();
  server.send(200, "application/json", json);
#else
  SendCorsHeader();
  server.send(503, "application/json", "[]");
#endif
}


