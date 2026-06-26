// ======================================================================
// Ford Focus MK1 ST170 — IMRC 外部主動訊號控制器
// Version 4.0 2026.06.26
// 硬體腳位定義：
//   D7  → Hall sensor / 點火訊號輸入（PCM 初級線圈下拉訊號）
//   D12 → 繼電器 IN 致能輸出（HIGH = 繼電器動作，LOW = 繼電器關閉）
//   OLED（SPI 7-pin SH1106）：
//     D0=13, D1=11, CS=10, DC=9, Reset=8
// ======================================================================
#include "U8glib.h"
U8GLIB_SH1106_128X64 u8g(13, 11, 10, 9, 8);

// 版本：
IMRCversion = "2026.06.26";
// -----------------------------------------------------------------------
// 腳位定義
// -----------------------------------------------------------------------
const int SIGNAL_PIN  = 7;   // 點火脈衝訊號輸入腳位
const int RELAY_PIN   = 12;  // 繼電器 IN 致能輸出腳位

// -----------------------------------------------------------------------
// 脈衝計數設定
// 說明：等到累積 hall_thresh 個脈衝後才計算一次 RPM
//       數值越大，計算越精確，但更新越慢
// -----------------------------------------------------------------------
const float HALL_THRESH = 400.0;

// -----------------------------------------------------------------------
// 脈衝係數設定（依實際接線調整）
// 四缸四行程，抓一個點火訊號：
//   若曲軸每轉 1 圈 D7 收到 1 個脈衝 → PULSE_PER_REV = 1.0
//   若曲軸每轉 1 圈 D7 收到 2 個脈衝 → PULSE_PER_REV = 2.0
// 實車測試時對比儀表板轉速表確認
// -----------------------------------------------------------------------
const float PULSE_PER_REV = 1.0;

// -----------------------------------------------------------------------
// IMRC 作動轉速門檻
// -----------------------------------------------------------------------
const int RPM_THRESHOLD = 6000;
const int RPM_ON_THRESHOLD  = RPM_THRESHOLD;  // 超過此值 → 繼電器 ON
const int RPM_OFF_THRESHOLD = RPM_THRESHOLD - 200;  // 低於此值 → 繼電器 OFF
// 5800 ~ 6000 之間保持上一次狀態

// -----------------------------------------------------------------------
// 全域狀態變數
// -----------------------------------------------------------------------
int   current_rpm    = 0;  // 當前計算 RPM（給 OLED 顯示用）
bool  relay_active   = false;  // 繼電器目前狀態（true = ON）

// ======================================================================
// setup()：開機初始化，只執行一次
// ======================================================================
void setup() {

  Serial.begin(115200);

  // 設定腳位方向
  pinMode(SIGNAL_PIN, INPUT);   // D7 點火訊號輸入
  pinMode(RELAY_PIN, OUTPUT);   // D12 繼電器輸出
  digitalWrite(RELAY_PIN, LOW); // 開機預設繼電器關閉

  delay(100);

  // OLED 開機畫面
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_helvB08);
    u8g.drawStr(3, 12, "GitHub: Jir8taiwan");
    u8g.drawStr(3, 32, "ST170 IMRC Controller");
    u8g.drawStr(3, 52, IMRCversion);
  } while (u8g.nextPage());

  delay(400);
}

// ======================================================================
// loop()：主迴圈，持續執行
// ======================================================================
void loop() {

  // --------------------------------------------------------------------
  // 第一步：量測 RPM
  // 原理：等待 HALL_THRESH 個脈衝，計算這段時間，換算轉速
  // PCM 下拉訊號：點火時為 LOW，待機時為 HIGH
  // --------------------------------------------------------------------
  float  hall_count = 0.0;
  float  start_time = micros();
  bool   in_pulse   = false;   // 防止同一個脈衝重複計數

  while (true) {
    if (digitalRead(SIGNAL_PIN) == LOW) {
      // 訊號為 LOW = 點火觸發中
      if (!in_pulse) {
        in_pulse = true;     // 標記：目前在脈衝內
        hall_count += 1.0;   // 計數加一
      }
    } else {
      // 訊號回到 HIGH = 脈衝結束，重置旗標準備下一次
      in_pulse = false;
    }

    // 累積夠了就跳出，開始計算
    if (hall_count >= HALL_THRESH) {
      break;
    }
  }

  // --------------------------------------------------------------------
  // 第二步：計算 RPM
  // 公式：RPM = (脈衝數 / 每轉脈衝數) / 經過秒數 × 60
  // --------------------------------------------------------------------
  float end_time     = micros();
  float time_passed  = (end_time - start_time) / 1000000.0;  // 轉換成秒
  current_rpm = (int)((hall_count / PULSE_PER_REV) / time_passed * 60.0);

  // 序列埠輸出供除錯監控
  Serial.print("Time Passed: ");
  Serial.print(time_passed, 3);
  Serial.print("s  |  RPM: ");
  Serial.println(current_rpm);

  // --------------------------------------------------------------------
  // 第三步：依 RPM 控制繼電器輸出
  // 邏輯：
  //   RPM >= RPM_THRESHOLD → 繼電器 ON（IMRC 開啟）
  //   RPM <  RPM_THRESHOLD → 繼電器 OFF（IMRC 關閉）
  // 注意：不使用 delay() 做二次確認，因為 delay() 期間
  //       RPM 不會更新，確認沒有實際意義。
  //       RPM 本身已是 HALL_THRESH 個脈衝的平均值，穩定性足夠。
  // --------------------------------------------------------------------
  if (current_rpm >= RPM_ON_THRESHOLD) {
    // 轉速超過 6000 RPM → 繼電器 ON
    digitalWrite(RELAY_PIN, HIGH);
    relay_active = true;
  } else {
    digitalWrite(RELAY_PIN, LOW);
    // 轉速低於 5800 RPM → 繼電器 OFF
    relay_active = false;
  }
  // ---- 序列埠輸出繼電器狀態 ----------------------------
  Serial.print("Relay: ");
  Serial.println(relay_active ? "ON" : "OFF");
  Serial.println("--------------------");

  // --------------------------------------------------------------------
  // 第四步：更新 OLED 顯示
  // 顯示：目前 RPM、繼電器狀態、設定門檻
  // --------------------------------------------------------------------
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_helvR12);

    // 第一行：RPM 數值
    u8g.drawStr(0, 15, "RPM:");
    u8g.setPrintPos(50, 15);
    u8g.print(current_rpm);

    // 第二行：繼電器狀態
    u8g.drawStr(0, 35, "IMRC:");
    u8g.setPrintPos(50, 35);
    u8g.print(relay_active ? "ON " : "OFF");

    // 第三行：設定門檻（確認目前設定值）
    u8g.drawStr(0, 55, "SET:");
    u8g.setPrintPos(50, 55);
    u8g.print(RPM_THRESHOLD);
    u8g.print("rpm");

  } while (u8g.nextPage());

  // 短暫延遲避免 OLED 更新過快造成閃爍
  delay(100);

} // loop() 結束
