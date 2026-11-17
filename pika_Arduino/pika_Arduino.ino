#include <SPI.h>
#include <Wire.h>
#include "MFRC522_SW.h"
#include <LiquidCrystal_I2C.h>
#include <Adafruit_INA219.h>
#include <Ethernet.h>
#include <ArduinoModbus.h>

#define SS_PIN   7
#define RST_PIN  5
#define TCA_ADDR 0x70
#define SLOT_N   6          // INA219 개수 (TCA 채널 0~5 = 슬롯 1~6)

// ===== 안정화 설정 =====
const uint32_t      I2C_CLOCK_HZ   = 50000;   // I2C 속도 50kHz (기본 100kHz보다 잡음에 강함)
const unsigned long LCD_REFRESH_MS = 10000;   // 현재 화면을 10초마다 다시 그림 (깨져도 자동 복구, 0이면 끔)
const unsigned long RFID_CHECK_MS  = 3000;    // RC522 상태 검사 주기

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_INA219 ina[SLOT_N];   // 전부 0x40, 멀티플렉서 채널로 구분

// 네트워크 / PLC
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress arduinoIP(192, 168, 137, 60);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress plcIP(192, 168, 137, 1);

EthernetClient ethClient;
ModbusTCPClient modbusClient(ethClient);

const int modbusPort  = 503;
const int HR_CARD     = 10;   // 카드 결제 신호
const int HR_SOC_BASE = 11;   // 슬롯1~6 잔량 = HR[11]~HR[16]       (아두이노 → PLC)
const int HR_PRESENT_BASE = 17;   // 슬롯1~6 상태 = HR[17]~HR[22]  (PLC → 아두이노, 0 = 비어 있음 / 1 = 꽂힘 / 2 = 불량)

unsigned long lastBeat = 0;
int lastSent[SLOT_N];
bool tcaOk = false;

// 배터리 설정 (모든 슬롯 공통)
const unsigned long SAMPLE_INTERVAL_MS = 100;     // 적산 주기 0.1초
const unsigned long PRINT_INTERVAL_MS  = 60000;   // 시리얼 출력 주기 (테스트할 땐 1000)
const unsigned long AVERAGE_PERIOD_MS  = 360000;  // 6분 평균

const float MAX_CAPACITY_MAH    = 1500.0;
const float CURRENT_SIGN        = 1.0;
const float CURRENT_DEADBAND_mA = 1.0;
const float R_COMP_OHM[SLOT_N] = {1.2, 1.2, 1.2, 1.2, 1.2, 1.2};

const float CHARGE_CURRENT_mA = 1000.0;
const float TERM_CURRENT_mA   = 100.0;
const float CV_VOLTAGE_V      = 4.10;
const float CV_START_PERCENT  = 85.0;

const float START_MIN_V         = 3.0;
const float START_MAX_V         = 4.25;
const float PRESENT_CURRENT_mA  = 20.0;
const int   START_CONFIRM_COUNT = 30;      // 3초
const float FULL_VOLTAGE_V      = 4.10;
const float FULL_CURRENT_mA     = 150.0;
const unsigned long FULL_HOLD_MS = 10000;
const float REMOVED_V           = 2.5;
const int   CONFIRM_COUNT       = 10;      // 1초

const unsigned long RAMP_INTERVAL_MS = 2000;

// ===== 빈 슬롯 오인식 방지 (추가) =====
// 실제 배터리는 전압이 거의 안 흔들리고, 배터리 없는 충전기 출력은 크게 출렁임
const float START_MAX_RIPPLE_V = 0.05;   // 시작 판정 3초 동안 전압 (최대-최소)가 이보다 크면 배터리로 인정 안 함
const float EMPTY_RIPPLE_V     = 0.15;   // 측정 중 1초 동안 이보다 크게 흔들리면 빈 슬롯 의심
const int   EMPTY_CONFIRM_SEC  = 3;      // 빈 슬롯 의심이 3초 연속이면 초기화
const int   RIPPLE_WINDOW_N    = 10;     // 흔들림 계산 구간 (0.1초 x 10 = 1초)
const int   PRESENT_DEBOUNCE   = 3;      // PLC '꽂힘' 신호가 3번(3초) 연속이어야 인정 (빠짐은 바로 반영)

// ===== 불량 판정 =====
const uint16_t FAULT_VALUE        = 110;      // 불량이면 잔량 대신 이 값을 보냄
const float OVER_V_LIMIT          = 4.30;     // 과전압 (빈 슬롯도 충전기 전압이 4.2V대로 읽히므로 그보다 높게)
const float OVER_I_LIMIT_mA       = 1300.0;   // 과전류
const float NEAR_FULL_V           = 4.05;     // 이 이상은 전류가 없어도 '충전 안 됨'으로 보지 않음 (완충 배터리)
const unsigned long LOW_V_MS      = 120000UL; // 과방전 2분
const unsigned long NO_CHARGE_MS  = 120000UL; // 충전 안 됨 2분
const unsigned long OVER_MS       = 1000UL;   // 과전압·과전류 1초
const unsigned long CHARGE_TIMEOUT_MS = 9000000UL;   // 충전 시간 초과 2.5시간

const float ocvV[]   = {3.00, 3.61, 3.69, 3.71, 3.73, 3.75, 3.77, 3.79, 3.80, 3.82, 3.84, 3.85, 3.87, 3.91, 3.95, 3.98, 4.02, 4.08, 4.11, 4.15, 4.20};
const float ocvPct[] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100};
const int OCV_N = sizeof(ocvV) / sizeof(ocvV[0]);

// 슬롯별 상태 (배터리 하나당 하나)
struct Slot {
  bool  ok;                       // INA219 인식 여부
  bool  started;
  float cap_mAh, startCap_mAh, dispPct;
  unsigned long lastRampMs;
  bool  fullReached, fullChecking;
  unsigned long fullStartMs;
  int   startCount, removeCount;
  float startVSum, startISum;
  float startVMin, startVMax;     // (추가) 시작 판정 3초 동안 전압 최소/최대
  // 출력용 누적
  float secV, secShunt, secI, secP;
  int   secN;
  // 6분 평균용 누적
  float sumLoadV, sumI, sumP;
  unsigned long sampleCount;
  float energy_mWh, charge_mAh;
  // 불량 여부
  bool  fault;
  unsigned long lowVSince, noChargeSince, overVSince, overISince;   // 조건이 시작된 시각 (0 = 조건 아님)
  int   goneCount;
  unsigned long chargeStartMs;
  bool  present;                  // PLC가 알려 준 배터리 유무
  int   presentCandCount;         // (추가) PLC '꽂힘' 연속 횟수
  // (추가) 1초 구간 전압 흔들림
  float winVMin, winVMax, lastRipple;
  int   winN, rippleCount;
};

Slot slots[SLOT_N];
unsigned long lastSampleMs = 0, lastPrintMs = 0, periodStartMs = 0;

void resetSlot(int i);

// ===================== I2C / 멀티플렉서 =====================
void i2cTune() {
  Wire.setClock(I2C_CLOCK_HZ);
  Wire.setWireTimeout(3000, true);   // I2C가 막혀도 보드가 멈추지 않게
}

bool tcaSelect(uint8_t ch) {
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << ch);
  return Wire.endTransmission() == 0;
}

void tcaOff() {
  if (!tcaOk) return;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(0);
  Wire.endTransmission();
}

// ===================== LCD (깨져도 스스로 복구) =====================
const __FlashStringHelper *scrL0 = nullptr;
const __FlashStringHelper *scrL1 = nullptr;
uint8_t scrC0 = 0, scrC1 = 0;
unsigned long lastLcdMs = 0;

void lcdDraw() {
  tcaOff();
  lcd.init();
  i2cTune();
  lcd.backlight();
  if (scrL0) { lcd.setCursor(scrC0, 0); lcd.print(scrL0); }
  if (scrL1) { lcd.setCursor(scrC1, 1); lcd.print(scrL1); }
  lastLcdMs = millis();
}

void lcdShow(uint8_t c0, const __FlashStringHelper *l0,
             uint8_t c1 = 0, const __FlashStringHelper *l1 = nullptr) {
  scrC0 = c0; scrL0 = l0;
  scrC1 = c1; scrL1 = l1;
  lcdDraw();
}

void lcdTask() {
  if (Wire.getWireTimeoutFlag()) {
    Wire.clearWireTimeoutFlag();
    Serial.println(F("I2C 타임아웃 → LCD 다시 그림"));
    lcdDraw();
    return;
  }
  if (LCD_REFRESH_MS > 0 && millis() - lastLcdMs >= LCD_REFRESH_MS) lcdDraw();
}

void showIdle() { lcdShow(1, F("Tap your card.")); }

// ===================== RC522 상태 검사 =====================
void rfidHealthTask() {
  static unsigned long last = 0;
  if (millis() - last < RFID_CHECK_MS) return;
  last = millis();

  byte ver = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  byte tx  = rfid.PCD_ReadRegister(MFRC522::TxControlReg);
  if (ver == 0x00 || ver == 0xFF || (tx & 0x03) != 0x03) {
    Serial.print(F("RC522 이상 (ver=0x")); Serial.print(ver, HEX);
    Serial.println(F(") → 재초기화"));
    rfid.PCD_Init();
  }
}

// ===================== PLC 통신 =====================
bool ensureConnected() {
  if (modbusClient.connected()) return true;
  modbusClient.stop();
  Serial.print(F("PLC 재접속... "));
  bool ok = modbusClient.begin(plcIP, modbusPort);
  Serial.println(ok ? F("성공") : F("실패"));
  return ok;
}

bool writeHR(int addr, uint16_t value) {
  if (!ensureConnected()) return false;
  if (!modbusClient.holdingRegisterWrite(addr, value)) {
    Serial.print(F("쓰기 실패 HR["));
    Serial.print(addr);
    Serial.print(F("] : "));
    Serial.println(modbusClient.lastError());
    return false;
  }
  return true;
}

uint16_t slotSoc(int i) {
  if (!slots[i].present) return 0;                 // PLC 기준 빈 슬롯
  if (slots[i].fault) return FAULT_VALUE;          // 불량이면 110
  if (!slots[i].started) return 0;
  uint16_t v = (uint16_t)(slots[i].dispPct + 0.5);
  return (v > 100) ? 100 : v;
}

// (변경) PLC 슬롯 상태 읽기 : 빠짐은 바로 반영, 꽂힘은 3초 연속일 때만 반영
void readPresent() {
  if (modbusClient.requestFrom(HOLDING_REGISTERS, HR_PRESENT_BASE, SLOT_N) != SLOT_N) return;

  for (int i = 0; i < SLOT_N; i++) {
    Slot &b = slots[i];
    long st = modbusClient.read();
    bool p = (st == 1 || st == 2);

    if (p == b.present) { b.presentCandCount = 0; continue; }

    if (p && ++b.presentCandCount < PRESENT_DEBOUNCE) continue;   // 꽂힘은 아직 확인 중

    Serial.print(F("[슬롯")); Serial.print(i + 1);
    Serial.print(p ? F("] PLC : 배터리 꽂힘 (상태값 ") : F("] PLC : 배터리 빠짐 (상태값 "));
    Serial.print(st); Serial.println(F(")"));
    resetSlot(i);
    b.present = p;
    b.presentCandCount = 0;
  }
}

// 1초마다: 하트비트 + 슬롯 상태 읽기 + 6개 잔량 전송
void plcTask() {
  if (millis() - lastBeat < 1000) return;
  lastBeat = millis();

  if (!ensureConnected()) return;

  if (modbusClient.holdingRegisterRead(0) < 0) {
    Serial.println(F("하트비트 실패 -> 재접속 예정"));
    modbusClient.stop();
    return;
  }

  // (변경) 상태를 먼저 읽고 나서 잔량 계산 → 빠진 슬롯에 옛 값이 1초 더 나가는 일 없음
  readPresent();

  uint16_t soc[SLOT_N];
  for (int i = 0; i < SLOT_N; i++) soc[i] = slotSoc(i);

  modbusClient.beginTransmission(HOLDING_REGISTERS, HR_SOC_BASE, SLOT_N);
  for (int i = 0; i < SLOT_N; i++) modbusClient.write(soc[i]);
  if (!modbusClient.endTransmission()) {
    Serial.print(F("잔량 전송 실패: "));
    Serial.println(modbusClient.lastError());
    return;
  }

  for (int i = 0; i < SLOT_N; i++) {
    if ((int)soc[i] != lastSent[i]) {
      Serial.print(F("PLC 전송: 슬롯")); Serial.print(i + 1);
      Serial.print(F(" HR[")); Serial.print(HR_SOC_BASE + i);
      Serial.print(F("] = ")); Serial.println(soc[i]);
      lastSent[i] = soc[i];
    }
  }
}

// ===================== 배터리 계산 =====================
float socFromVoltage(float v) {
  if (v <= ocvV[0]) return 0;
  if (v >= ocvV[OCV_N - 1]) return 100;
  for (int k = 1; k < OCV_N; k++) {
    if (v < ocvV[k]) {
      return ocvPct[k - 1] + (v - ocvV[k - 1]) * (ocvPct[k] - ocvPct[k - 1]) / (ocvV[k] - ocvV[k - 1]);
    }
  }
  return 100;
}

float socFromCvCurrent(float c) {
  float ratio = (CHARGE_CURRENT_mA - c) / (CHARGE_CURRENT_mA - TERM_CURRENT_mA);
  if (ratio < 0.0) ratio = 0.0;
  if (ratio > 1.0) ratio = 1.0;
  return CV_START_PERCENT + ratio * (100.0 - CV_START_PERCENT);
}

void startBattery(int i, float v, float c) {
  Slot &b = slots[i];
  float chargeI = (c > 0) ? c : 0;
  float vComp = v - (chargeI / 1000.0) * R_COMP_OHM[i];
  float pct = socFromVoltage(vComp);
  bool cvMode = false;

  if (vComp >= CV_VOLTAGE_V) {
    float pctI = socFromCvCurrent(c);
    if (pctI > pct) pct = pctI;
    cvMode = true;
  }
  if (pct > 99.0) pct = 99.0;

  b.cap_mAh      = pct / 100.0 * MAX_CAPACITY_MAH;
  b.startCap_mAh = b.cap_mAh;
  b.dispPct      = pct;
  b.fullReached  = false;
  b.fullChecking = false;
  b.started      = true;
  b.chargeStartMs = millis();
  b.rippleCount  = 0;

  Serial.print(F("**** [슬롯")); Serial.print(i + 1); Serial.print(F("] 보간: "));
  Serial.print(v, 3); Serial.print(F("V (배터리 ")); Serial.print(vComp, 3);
  Serial.print(F("V), ")); Serial.print(c, 1); Serial.print(F("mA → "));
  Serial.print(cvMode ? F("CV 전류 보간 ") : F("전압 보간 "));
  Serial.print(pct, 1); Serial.println(F("% ****"));
}

void resetSlot(int i) {
  Slot &b = slots[i];
  b.started = false;
  b.removeCount = 0;
  b.cap_mAh = b.startCap_mAh = b.dispPct = 0;
  b.fullReached = b.fullChecking = false;
  b.startCount = 0; b.startVSum = 0; b.startISum = 0;
  b.startVMin = b.startVMax = 0;
  b.fault = false;
  b.lowVSince = b.noChargeSince = b.overVSince = b.overISince = 0;
  b.winN = 0; b.rippleCount = 0;
  Serial.print(F("**** [슬롯")); Serial.print(i + 1);
  Serial.println(F("] 초기화 → 다시 측정 ****"));
}

static bool heldFor(bool cond, unsigned long &since, unsigned long now, unsigned long ms) {
  if (!cond) { since = 0; return false; }
  if (since == 0) since = (now == 0) ? 1 : now;
  return now - since >= ms;
}

void checkFault(int i, float busV, float cur, unsigned long now) {
  Slot &b = slots[i];
  bool fault = false;

  if (!b.present || busV < REMOVED_V) {
    b.lowVSince = b.noChargeSince = b.overVSince = b.overISince = 0;
  }
  else {
    float vBat = busV - ((cur > 0) ? cur : 0) / 1000.0 * R_COMP_OHM[i];

    if (heldFor(vBat > OVER_V_LIMIT,    b.overVSince, now, OVER_MS)) fault = true;
    if (heldFor(cur  > OVER_I_LIMIT_mA, b.overISince, now, OVER_MS)) fault = true;

    if (!b.started) {
      if (heldFor(busV < START_MIN_V, b.lowVSince, now, LOW_V_MS)) fault = true;

      bool noCharge = (busV >= START_MIN_V && busV < NEAR_FULL_V && cur < PRESENT_CURRENT_mA);
      if (heldFor(noCharge, b.noChargeSince, now, NO_CHARGE_MS)) fault = true;
    }
    else {
      b.lowVSince = b.noChargeSince = 0;
      if (cur < PRESENT_CURRENT_mA) {
        b.chargeStartMs = now;
      }
      else if (!b.fullReached && now - b.chargeStartMs >= CHARGE_TIMEOUT_MS) {
        fault = true;
      }
    }
  }

  if (fault != b.fault) {
    Serial.print(fault ? F("!!!! [슬롯") : F("[슬롯")); Serial.print(i + 1);
    Serial.println(fault ? F("] 불량 조건 → 110 전송 !!!!") : F("] 불량 조건 해제"));
  }
  b.fault = fault;
}

// (추가) 1초 구간마다 전압 흔들림(최대-최소) 계산. 구간이 끝났으면 true
bool updateRipple(Slot &b, float busV) {
  if (b.winN == 0) { b.winVMin = b.winVMax = busV; }
  if (busV < b.winVMin) b.winVMin = busV;
  if (busV > b.winVMax) b.winVMax = busV;
  if (++b.winN < RIPPLE_WINDOW_N) return false;
  b.lastRipple = b.winVMax - b.winVMin;
  b.winN = 0;
  return true;
}

void updateSlot(int i, float busV, float shunt, float cur, float pwr, float dt, unsigned long now) {
  checkFault(i, busV, cur, now);
  Slot &b = slots[i];
  bool windowDone = updateRipple(b, busV);   // (추가)

  b.secV += busV; b.secShunt += shunt; b.secI += cur; b.secP += pwr; b.secN++;
  b.energy_mWh += pwr * dt / 3600.0;
  b.charge_mAh += cur * dt / 3600.0;

  // 시작 / 제거 판정
  if (!b.started) {
    bool charging = (busV >= START_MIN_V && busV <= START_MAX_V && cur >= PRESENT_CURRENT_mA);
    bool restFull = (busV >= NEAR_FULL_V && busV <= START_MAX_V && fabs(cur) < PRESENT_CURRENT_mA);
    if (b.present && !b.fault && (charging || restFull)) {
      if (b.startCount == 0) { b.startVMin = b.startVMax = busV; }   // (추가)
      if (busV < b.startVMin) b.startVMin = busV;
      if (busV > b.startVMax) b.startVMax = busV;
      b.startVSum += busV; b.startISum += (cur > 0) ? cur : 0;

      if (++b.startCount >= START_CONFIRM_COUNT) {
        float ripple = b.startVMax - b.startVMin;
        if (ripple <= START_MAX_RIPPLE_V) {
          startBattery(i, b.startVSum / b.startCount, b.startISum / b.startCount);
        } else {
          // (추가) 3초 동안 전압이 크게 흔들림 → 빈 충전기 출력으로 보고 시작 안 함
          Serial.print(F("[슬롯")); Serial.print(i + 1);
          Serial.print(F("] 전압 흔들림 ")); Serial.print(ripple, 3);
          Serial.println(F("V → 배터리로 인정 안 함"));
        }
        b.startCount = 0; b.startVSum = 0; b.startISum = 0;
      }
    } else {
      b.startCount = 0; b.startVSum = 0; b.startISum = 0;
    }
    return;
  }

  // (추가) 측정 중인데 전압이 계속 크게 출렁이면 배터리가 빠진 것으로 보고 초기화
  //        (빈 슬롯은 충전기 전압 때문에 2.5V 아래로 안 떨어져서 기존 제거 판정이 안 걸림)
  if (windowDone) {
    if (b.lastRipple > EMPTY_RIPPLE_V) {
      if (++b.rippleCount >= EMPTY_CONFIRM_SEC) {
        Serial.print(F("[슬롯")); Serial.print(i + 1);
        Serial.print(F("] 전압 흔들림 ")); Serial.print(b.lastRipple, 3);
        Serial.println(F("V 지속 → 빈 슬롯으로 판단"));
        resetSlot(i);
        return;
      }
    } else {
      b.rippleCount = 0;
    }
  }

  if (busV < REMOVED_V) {
    if (++b.removeCount >= CONFIRM_COUNT) resetSlot(i);
    return;
  }
  b.removeCount = 0;

  // 쿨롱 적산
  float c = (fabs(cur) < CURRENT_DEADBAND_mA) ? 0.0 : cur;
  b.cap_mAh += (c * dt) / 3600.0;

  float limit = b.fullReached ? MAX_CAPACITY_MAH : MAX_CAPACITY_MAH * 0.99;
  if (b.cap_mAh > limit) b.cap_mAh = limit;
  if (b.cap_mAh < 0.0)   b.cap_mAh = 0.0;

  // 완충 판정
  float vBat = busV - ((cur > 0) ? cur : 0) / 1000.0 * R_COMP_OHM[i];
  if (vBat >= FULL_VOLTAGE_V && cur <= FULL_CURRENT_mA) {
    if (!b.fullChecking) {
      b.fullChecking = true;
      b.fullStartMs = now;
    } else if (!b.fullReached && now - b.fullStartMs >= FULL_HOLD_MS) {
      b.fullReached = true;
      b.cap_mAh = MAX_CAPACITY_MAH;
      b.startCap_mAh = MAX_CAPACITY_MAH;
      Serial.print(F("**** [슬롯")); Serial.print(i + 1); Serial.println(F("] 완충 ****"));
    }
  } else {
    b.fullChecking = false;
  }

  // 표시 퍼센트 1%씩 따라가기
  float target = b.cap_mAh / MAX_CAPACITY_MAH * 100.0;
  if (target <= b.dispPct + 1.0) {
    b.dispPct = target;
  } else if (now - b.lastRampMs >= RAMP_INTERVAL_MS) {
    b.dispPct += 1.0;
    b.lastRampMs = now;
  }
}

void batteryTask() {
  unsigned long now = millis();

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    float dt = (now - lastSampleMs) / 1000.0;
    lastSampleMs = now;

    for (int i = 0; i < SLOT_N; i++) {
      if (!slots[i].ok) continue;
      if (!tcaSelect(i)) continue;

      float busV  = ina[i].getBusVoltage_V();
      float shunt = ina[i].getShuntVoltage_mV() * CURRENT_SIGN;
      float cur   = ina[i].getCurrent_mA() * CURRENT_SIGN;
      float pwr   = ina[i].getPower_mW();

      updateSlot(i, busV, shunt, cur, pwr, dt, now);
    }
    tcaOff();
  }

  // 시리얼 출력 (슬롯당 한 줄)
  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    Serial.print(F("[경과 ")); Serial.print((now - periodStartMs) / 1000); Serial.println(F("초]"));

    for (int i = 0; i < SLOT_N; i++) {
      Slot &b = slots[i];
      Serial.print(F("슬롯")); Serial.print(i + 1); Serial.print(F(" : "));

      if (!b.ok)       { Serial.println(F("INA219 없음")); continue; }
      if (b.secN == 0) { Serial.println(F("측정값 없음")); continue; }

      float v  = b.secV / b.secN;
      float sh = b.secShunt / b.secN;
      float I  = b.secI / b.secN;
      float P  = b.secP / b.secN;
      float loadV = v + sh / 1000.0;
      b.secV = b.secShunt = b.secI = b.secP = 0; b.secN = 0;

      b.sumLoadV += loadV; b.sumI += I; b.sumP += P; b.sampleCount++;

      Serial.print(v, 3);  Serial.print(F("V  "));
      Serial.print(I, 1);  Serial.print(F("mA  "));
      Serial.print(P, 0);  Serial.print(F("mW  "));
      Serial.print(F("흔들림 ")); Serial.print(b.lastRipple, 3); Serial.print(F("V  "));   // (추가)
      Serial.print(b.present ? F("[PLC 꽂힘] ") : F("[PLC 비어있음] "));                // (추가)
      if (b.fault) {
        Serial.print(F("[불량] 110 전송 중"));
      }
      else if (b.started) {
        uint16_t s = slotSoc(i);
        Serial.print(s); Serial.print(F("%"));
        if (b.fullReached && s < 100) Serial.print(F(" [완충 반영 중]"));
        else if (b.fullReached)       Serial.print(F(" [완충]"));
        else if (b.fullChecking)      Serial.print(F(" [완충 확인 중]"));
        else                          Serial.print(F(" [적산 중]"));
      } else {
        Serial.print(F("배터리 확인 중"));
        if (b.startCount > 0) {
          Serial.print(F(" (충전 감지 ")); Serial.print(b.startCount / 10.0, 1); Serial.print(F("초)"));
        }
        if (b.noChargeSince) {
          Serial.print(F(" (충전 안 됨 ")); Serial.print((now - b.noChargeSince) / 1000); Serial.print(F("초 / 120초)"));
        }
        if (b.lowVSince) {
          Serial.print(F(" (과방전 ")); Serial.print((now - b.lowVSince) / 1000); Serial.print(F("초 / 120초)"));
        }
      }
      Serial.println();
    }
    Serial.println(F("-----------------------------------------"));
  }

  // 6분 평균
  if (now - periodStartMs >= AVERAGE_PERIOD_MS) {
    Serial.println(F("========== 6분 평균 =========="));
    for (int i = 0; i < SLOT_N; i++) {
      Slot &b = slots[i];
      if (!b.ok || b.sampleCount == 0) continue;
      Serial.print(F("슬롯")); Serial.print(i + 1); Serial.print(F(" : "));
      Serial.print(b.sumLoadV / b.sampleCount, 3); Serial.print(F("V  "));
      Serial.print(b.sumI / b.sampleCount, 1);     Serial.print(F("mA  "));
      Serial.print(b.energy_mWh, 2);               Serial.print(F("mWh  "));
      Serial.print(b.charge_mAh, 2);               Serial.println(F("mAh"));
      b.sumLoadV = b.sumI = b.sumP = 0;
      b.sampleCount = 0;
      b.energy_mWh = b.charge_mAh = 0;
    }
    Serial.println(F("=============================="));
    periodStartMs = now;
  }
}

void waitWithTasks(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    batteryTask();
    plcTask();
    lcdTask();
  }
}

// ===================== RFID =====================
bool isCardPresent() {
  byte atqa[2];
  byte size = sizeof(atqa);
  if (rfid.PICC_WakeupA(atqa, &size) == MFRC522::STATUS_OK) {
    rfid.PICC_HaltA();
    return true;
  }
  return false;
}

void waitForCardRemoval() {
  byte missCount = 0;
  while (missCount < 3) {
    if (isCardPresent()) missCount = 0;
    else missCount++;
    waitWithTasks(100);
  }
}

// ===================== setup / loop =====================
void setup() {
  Serial.begin(9600);

  pinMode(53, OUTPUT);      digitalWrite(53, HIGH);
  pinMode(SS_PIN, OUTPUT);  digitalWrite(SS_PIN, HIGH);
  pinMode(17, OUTPUT);      digitalWrite(17, HIGH);
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW);  delay(10);
  digitalWrite(16, HIGH); delay(200);

  SPI.begin();

  Ethernet.init(17);
  Ethernet.begin(mac, arduinoIP, gateway, gateway, subnet);
  delay(1500);

  Serial.print(F("아두이노 IP: "));
  Serial.println(Ethernet.localIP());
  if (Ethernet.hardwareStatus() == EthernetNoHardware) Serial.println(F("이더넷 칩 인식 실패"));
  if (Ethernet.linkStatus() == LinkOFF)                Serial.println(F("랜 케이블 링크 없음"));

  Wire.begin();
  i2cTune();

  Wire.beginTransmission(TCA_ADDR);
  tcaOk = (Wire.endTransmission() == 0);
  if (!tcaOk) Serial.println(F("TCA9548A(0x70)를 찾을 수 없음 → 배터리 측정 없이 계속"));

  for (int i = 0; i < SLOT_N; i++) {
    memset(&slots[i], 0, sizeof(Slot));
    lastSent[i] = -1;

    slots[i].ok = tcaOk && tcaSelect(i) && ina[i].begin();
    if (slots[i].ok) ina[i].setCalibration_32V_2A();

    Serial.print(F("슬롯")); Serial.print(i + 1);
    Serial.print(F(" (채널 ")); Serial.print(i); Serial.print(F(") INA219 : "));
    Serial.println(slots[i].ok ? F("OK") : F("없음"));
  }
  tcaOff();
  i2cTune();

  rfid.PCD_Init();
  delay(4);
  rfid.PCD_DumpVersionToSerial();

  modbusClient.setTimeout(500);
  ensureConnected();

  lastSampleMs = lastPrintMs = periodStartMs = millis();

  showIdle();
}

void loop() {
  plcTask();
  batteryTask();
  lcdTask();
  rfidHealthTask();

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.print(F("UID:"));
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? F(" 0") : F(" "));
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  lcdShow(1, F("Processing...."));
  waitWithTasks(2000);

  if (!writeHR(HR_CARD, 1)) {
    lcdShow(0, F("PLC Comm Error"));
    waitWithTasks(2000);
    showIdle();
    return;
  }
  Serial.println(F("HR[10] = 1 전송 완료"));

  lcdShow(4, F("Paid! :)"), 0, F("Have a nice day!"));

  waitForCardRemoval();
  writeHR(HR_CARD, 0);
  Serial.println(F("HR[10] = 0 전송 완료"));
  Serial.println(F("Card removed"));

  showIdle();
}