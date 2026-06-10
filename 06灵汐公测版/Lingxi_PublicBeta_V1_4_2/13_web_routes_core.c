/*
 * 文件：13_web_routes_core.ino
 * 标题：Web 路由注册表
 * 说明：集中注册 WebServer 路由，把 URL 分发到对应处理函数。
 * 可改：可新增独立 API
 * 禁止：删除主页、状态、控制、语音、节点基础路由。
 */

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void PrintSystemState() {
  xSemaphoreTake(StateMutex, portMAX_DELAY);

  Serial.println();
  Serial.println("========== SystemState ==========");
  Serial.print("temperature: "); Serial.println(State.temperature);
  Serial.print("humidity: "); Serial.println(State.humidity);
  Serial.print("GuangZhao: "); Serial.println(State.lightLevelValue);
  Serial.print("YanWuZhi: "); Serial.println(State.smokeValue);
  Serial.print("RanQiZhi: "); Serial.println(State.gasValue);
  Serial.print("livingPir: "); Serial.println(State.livingPirState);
  Serial.print("bedroomPir: "); Serial.println(State.bedroomPirState);
  Serial.print("DaMenCi: "); Serial.println(State.mainDoorContact);
  Serial.print("windowContact: "); Serial.println(State.windowContact);

  Serial.print("autoMode: "); Serial.println(State.autoMode);
  Serial.print("awayMode: "); Serial.println(State.awayMode);
  Serial.print("alarmMode: "); Serial.println(State.alarmMode);
  Serial.print("YanShiMoShi: "); Serial.println(State.previewMode);
  Serial.print("sensorBoardOnline: "); Serial.println(State.sensorBoardOnline);
  Serial.print("LastBaoXuHao: "); Serial.println(State.lastPacketSeq);

  Serial.println("---------- ControlCommand --------");
  Serial.print("lightCmd: "); Serial.println(CurrentCmd.lightCmd);
  Serial.print("lightLevel: "); Serial.println(CurrentCmd.lightLevel);
  Serial.print("relayCmd: "); Serial.println(CurrentCmd.relayCmd);
  Serial.print("servoCmd: "); Serial.println(CurrentCmd.servoCmd);
  Serial.print("motorCmd: "); Serial.println(CurrentCmd.motorCmd);
  Serial.print("buzzerCmd: "); Serial.println(CurrentCmd.buzzerCmd);
  Serial.print("sceneCmd: "); Serial.println(CurrentCmd.sceneCmd);
  Serial.println("==================================");

  xSemaphoreGive(StateMutex);
}


// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleSerialCommand(String cmd) {
  cmd.trim();
  if (V4HandleSerialCommand(cmd)) return;
  cmd.toLowerCase();

  if (cmd == "help") {
    Serial.println("Commands:");
    Serial.println("status");
    Serial.println("demo on / demo off");
    Serial.println("auto on / auto off");
    Serial.println("away on / away off");
    Serial.println("light on / light off");
    Serial.println("alarm reset");
    Serial.println("v4 status");
    Serial.println("v4 wake        // 串口模拟：你好灵汐 -> 录音 -> ASR -> 本地意图/聊天");
    Serial.println("v4 say 文本     // 只测试本地意图路由，不录音");
    Serial.println("v4 record 5    // 录音指定秒数后进入 ASR");
  }

  else if (cmd == "status") {
    PrintSystemState();
  }

  else if (cmd == "demo on") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.previewMode = true;
    xSemaphoreGive(StateMutex);
    Serial.println("Demo mode ON");
  }

  else if (cmd == "demo off") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.previewMode = false;
    xSemaphoreGive(StateMutex);
    Serial.println("Demo mode OFF");
  }

  else if (cmd == "auto on") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.autoMode = true;
    xSemaphoreGive(StateMutex);
    Serial.println("Auto mode ON");
  }

  else if (cmd == "auto off") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.autoMode = false;
    xSemaphoreGive(StateMutex);
    Serial.println("Auto mode OFF");
  }

  else if (cmd == "away on") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.awayMode = true;
    xSemaphoreGive(StateMutex);
    Serial.println("Away mode ON");
  }

  else if (cmd == "away off") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.awayMode = false;
    xSemaphoreGive(StateMutex);
    Serial.println("Away mode OFF");
  }

  else if (cmd == "light on") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.autoMode = false;
    State.lightState = 1;
    xSemaphoreGive(StateMutex);

    ControlCommand c = CurrentCmd;
    c.lightCmd = 1;
    c.lightLevel = 180;
    SetControlCommand(c);

    Serial.println("Manual light ON");
  }

  else if (cmd == "light off") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.autoMode = false;
    State.lightState = 0;
    xSemaphoreGive(StateMutex);

    ControlCommand c = CurrentCmd;
    c.lightCmd = 0;
    c.lightLevel = 0;
    SetControlCommand(c);

    Serial.println("Manual light OFF");
  }

  else if (cmd == "alarm reset") {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    State.alarmMode = false;
    xSemaphoreGive(StateMutex);

    ControlCommand c = CurrentCmd;
    c.lightCmd = 0;
    c.lightLevel = 0;
    c.relayCmd = 0;
    c.buzzerCmd = 0;
    c.sceneCmd = 0;
    SetControlCommand(c);

    Serial.println("Alarm reset");
  }

  else {
    Serial.println("Unknown command. Type help.");
  }
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskEspNowAndState(void *pvParameters) {
  while (1) {
    xSemaphoreTake(StateMutex, portMAX_DELAY);
    bool demo = State.previewMode;
    xSemaphoreGive(StateMutex);

    if (demo) {
      UpdateDemoData();
    }

    UpdateSystemStateFromSensor();

    xSemaphoreTake(StateMutex, portMAX_DELAY);
    if (millis() - State.LastSensorTime > 3000) {
      State.sensorBoardOnline = false;
    }
    xSemaphoreGive(StateMutex);

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskWebServer(void *pvParameters) {
  while (1) {
    server.handleClient();
    ApAlwaysOnLoop();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskRule(void *pvParameters) {
  while (1) {
    RunRuleEngine();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskCommandSend(void *pvParameters) {
  while (1) {
    unsigned long now = millis();

    if (CmdNeedSend || now - LastCmdSendTime > 2000) {
      CmdNeedSend = false;
      LastCmdSendTime = now;
      SendControlCommand();
    }

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// 函数说明：语音链路函数，负责录音、识别、合成或播报相关处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskV4LocalVoice(void *pvParameters) {
  while (1) {
    String type, text, source;
    uint32_t seconds = LINGXI_V4_RECORD_SECONDS;
    if (V4TakeQueuedRequest(type, text, seconds, source)) {
      if (type == "say") {
        Serial.println("[V4Task] route text: " + text);
        V4VoiceRouteText(text, source);
      } else if (type == "record" || type == "wake") {
        Serial.println("[V4 FinalSeal Task] board mic seconds=" + String(seconds) + " source=" + source);
        bool ok = V4VoiceRunOnceFromBoardMic(seconds, source);
        if (source.startsWith("ci1302_")) {
          CI1302SoftRearmAfterOneShot(ok ? "voice_done" : "voice_failed");
        }
      }
      Serial.println(V4VoiceStatusJson());
    }
    vTaskDelay(pdMS_TO_TICKS(30));
  }
}


uint8_t CI1302I2CAddr = CI1302_I2C_ADDR_FIXED;
uint32_t CI1302LastScanMs = 0;
uint32_t CI1302LastWakeMs = 0;
uint32_t CI1302LastPrintMs = 0;
uint8_t CI1302LastValue = 0xFF;
uint32_t CI1302LastValueMs = 0;

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String CI1302HexBytes(const uint8_t *data, int len) {
  const char *hex = "0123456789ABCDEF";
  String out = "";
  for (int i = 0; i < len; i++) {
    if (i) out += " ";
    out += hex[(data[i] >> 4) & 0x0F];
    out += hex[data[i] & 0x0F];
  }
  return out;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool CI1302IsKnownBoardI2CAddress(uint8_t addr) {
  if (addr == IOEXT_ADDR) return true;
  if (addr == ES8311_I2C_ADDR) return true;
  if (addr >= 0x40 && addr <= 0x43) return true;
  if (addr == 0x51) return true;
  if (addr == 0x14 || addr == 0x5D) return true;
  return false;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool CI1302ProbeAddress(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CI1302ScanI2CBus(bool verbose) {
  bool fixedFound = false;

  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    if (CI1302ProbeAddress(addr)) {
      if (addr == CI1302_I2C_ADDR_FIXED) fixedFound = true;
      if (verbose) {
        Serial.printf("[CI1302-I2C] found addr=0x%02X%s%s\n",
                      addr,
                      addr == CI1302_I2C_ADDR_FIXED ? " (CI1302)" : "",
                      CI1302IsKnownBoardI2CAddress(addr) ? " (board-known)" : "");
      }
    }
    delay(2);
  }

  if (fixedFound) {
    CI1302I2CAddr = CI1302_I2C_ADDR_FIXED;
    Serial.printf("[CI1302-I2C] use fixed addr=0x%02X\n", CI1302I2CAddr);
    AddCenterLog("sys", "CI1302 I2C地址确认 0x2B");
  } else if (verbose) {
    Serial.println("[CI1302-I2C] fixed addr 0x2B not found; check 5V/GND/SCL/SDA");
  }
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool CI1302WriteReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(CI1302I2CAddr);
  Wire.write(reg);
  Wire.write(value);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    Serial.printf("[CI1302-I2C] write reg 0x%02X=0x%02X failed err=%u\n", reg, value, err);
    return false;
  }
  return true;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool CI1302ReadReg(uint8_t reg, uint8_t *value) {
  if (!value) return false;

  Wire.beginTransmission(CI1302I2CAddr);
  Wire.write(reg);
  uint8_t err = Wire.endTransmission(false);
  if (err != 0) {
    Serial.printf("[CI1302-I2C] set read reg 0x%02X failed err=%u\n", reg, err);
    return false;
  }

  int n = Wire.requestFrom((int)CI1302I2CAddr, 1);
  if (n <= 0) return false;

  if (Wire.available()) {
    *value = (uint8_t)Wire.read();
    return true;
  }

  return false;
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool CI1302IsIdleValue(uint8_t v) {
  return v == CI1302_I2C_IDLE_FF || v == CI1302_I2C_IDLE_88;
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CI1302SoftRearmAfterOneShot(String reason) {
  (void)reason;
  
  
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CI1302TriggerWakeFromI2C(uint8_t value) {
  if (!ENABLE_LINGXI_V4_LOCAL_VOICE) return;
  uint32_t now = millis();
  if (now - CI1302LastWakeMs < CI1302_WAKE_DEBOUNCE_MS) {
    Serial.printf("[V4 CI1302] wake debounced value=0x%02X\n", value);
    return;
  }
  CI1302LastWakeMs = now;

  bool pending = false;
  if (V4ReqMutex) {
    xSemaphoreTake(V4ReqMutex, portMAX_DELAY);
    pending = V4Req.pending;
    xSemaphoreGive(V4ReqMutex);
  }
  if (pending || V4Voice.busy || VoiceIO.recording || VoiceRecordAsyncActive) {
    Serial.printf("[V4 CI1302] wake ignored busy=%d rec=%d async=%d pending=%d\n",
                  V4Voice.busy ? 1 : 0,
                  VoiceIO.recording ? 1 : 0,
                  VoiceRecordAsyncActive ? 1 : 0,
                  pending ? 1 : 0);
    return;
  }

  Serial.printf("[V4 CI1302] wake value=0x%02X -> one wake one file ASR -> record %d sec\n",
                value, CI1302_WAKE_RECORD_SECONDS);
  AddCenterLog("voice", "CI1302离线唤醒：单轮录音文件识别");
  V4QueueRequest("wake", "", CI1302_WAKE_RECORD_SECONDS, "ci1302_i2c_wake");
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CI1302HandleI2CValue(uint8_t value) {
  uint32_t now = millis();

  bool changed = (value != CI1302LastValue);

  
  if (CI1302IsIdleValue(value)) {
    if (changed || now - CI1302LastPrintMs > 10000) {
      CI1302LastPrintMs = now;
      Serial.printf("[CI1302-I2C] idle value=0x%02X\n", value);
    }
    CI1302LastValue = value;
    return;
  }

  
  
  
  
  
  
  
  
  
  
  if (changed || now - CI1302LastValueMs > 10000) {
    Serial.printf("[CI1302-I2C] read reg0x64 value=0x%02X dec=%u%s\n", value, value, changed ? " edge" : " hold");
    CI1302LastValueMs = now;
  }

  
  if (!changed) {
    return;
  }

  CI1302LastValue = value;

  
  if (value == CI1302_I2C_WAKE_VALUE_00 || value == CI1302_I2C_WAKE_VALUE_03) {
    CI1302TriggerWakeFromI2C(value);
    return;
  }

  
  if (value == 0x02 || value == 0x04 || value == 0x05 || value == CI1302_I2C_SLEEP_VALUE_6F) {
    Serial.printf("[CI1302-I2C] default/status value=0x%02X logged only\n", value);
    return;
  }

  Serial.printf("[CI1302-I2C] unknown value=0x%02X logged only\n", value);
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CI1302PollOneAddress(uint8_t addr) {
  if (addr == 0) return;
  if (V4Voice.busy || VoiceIO.recording || VoiceRecordAsyncActive) return;

  uint8_t value = 0xFF;
  if (CI1302ReadReg(CI1302_I2C_READ_REGISTER, &value)) {
    CI1302HandleI2CValue(value);
  }
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void CI1302I2CInit() {
#if ENABLE_CI1302_I2C_WAKE
  Wire.begin(BOARD_I2C_SDA_PIN, BOARD_I2C_SCL_PIN);
  Wire.setClock(100000);
  Serial.printf("[CI1302-I2C] init SDA=%d SCL=%d addr=0x%02X readReg=0x%02X poll=%dms\n",
                BOARD_I2C_SDA_PIN, BOARD_I2C_SCL_PIN, CI1302_I2C_ADDR_FIXED,
                CI1302_I2C_READ_REGISTER, CI1302_I2C_POLL_INTERVAL_MS);
  AddCenterLog("sys", "CI1302 I2C唤醒入口已启用");

  CI1302ScanI2CBus(true);

  bool initOk = CI1302WriteReg(CI1302_I2C_WRITE_REGISTER, CI1302_I2C_INIT_VALUE);
  Serial.printf("[CI1302-I2C] write init reg0x%02X=0x%02X %s\n",
                CI1302_I2C_WRITE_REGISTER, CI1302_I2C_INIT_VALUE, initOk ? "OK" : "FAIL");
#endif
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskCI1302I2CWake(void *pvParameters) {
#if ENABLE_CI1302_I2C_WAKE
  delay(2500);
  CI1302I2CInit();

  while (1) {
    uint32_t now = millis();

    if (now - CI1302LastScanMs > CI1302_I2C_SCAN_INTERVAL_MS) {
      CI1302LastScanMs = now;
      if (!CI1302ProbeAddress(CI1302_I2C_ADDR_FIXED)) {
        Serial.println("[CI1302-I2C] addr 0x2B lost, rescanning...");
        CI1302ScanI2CBus(false);
      }
    }

    CI1302PollOneAddress(CI1302I2CAddr);
    vTaskDelay(pdMS_TO_TICKS(CI1302_I2C_POLL_INTERVAL_MS));
  }
#else
  vTaskDelete(NULL);
#endif
}


// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskSerial(void *pvParameters) {
  while (1) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();

      if (TryAuthResetMaintCommand(cmd, "Serial")) {
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      }

      HandleSerialCommand(cmd);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// 函数说明：业务函数，负责本模块内的一项独立处理。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void TaskStatusPrint(void *pvParameters) {
  while (1) {
    PrintSystemState();
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}


