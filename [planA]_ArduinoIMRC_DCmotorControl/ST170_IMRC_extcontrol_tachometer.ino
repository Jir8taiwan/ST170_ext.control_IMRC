// ============================================================
// Ford Focus MK1 ST170 — IMRC Active Signal Controller
// Version 4.0  2026.06.26
//
// 功能說明：
//   讀取點火線圈 Hall 感測器脈衝 → 計算 RPM
//   RPM >= 6000 → 繼電器 ON  (pin 12 HIGH)
//   RPM <  5800 → 繼電器 OFF (pin 12 LOW)
//   5800 <= RPM < 6000 → 保持上一次狀態（遲滯保護，防止臨界點抖動）
//
// 接線說明：
//   pin  D7  — Hall 感測器輸入（點火線圈脈衝信號）
//   pin D12  — 繼電器線圈控制輸出（HIGH = 繼電器 ON）
// ============================================================

// ------ 腳位定義 --------------------------------------------
const int HALL_PIN  = 7;   // Hall 感測器輸入腳位
const int RELAY_PIN = 12;  // 繼電器控制輸出腳位

// ------ RPM 計算參數 ----------------------------------------
// 每次計算 RPM 所需收集的脈衝數
// 數值越大精度越高，但更新速度越慢；後續可自行微調
const float HALL_THRESH    = 400.0;

// 每轉對應的脈衝數（ST170 點火線圈信號：每轉 1 次脈衝）
//   若曲軸每轉 1 圈 D7 收到 1 個脈衝 → PULSE_PER_REV = 1.0
//   若曲軸每轉 1 圈 D7 收到 2 個脈衝 → PULSE_PER_REV = 2.0
const float PULSE_PER_REV  = 1.0;

// ------ 繼電器控制閾值（遲滯保護）--------------------------
const int RPM_ON_THRESHOLD  = 6000;  // 超過此值 → 繼電器 ON
const int RPM_OFF_THRESHOLD = 5800;  // 低於此值 → 繼電器 OFF
// 5800 ~ 6000 之間保持上一次狀態

// ------ 全域狀態變數 ----------------------------------------
int  current_rpm   = 0;    // 目前計算出的 RPM
bool relay_active  = false; // 繼電器目前狀態（false = OFF）

// ============================================================
// setup：只執行一次
// ============================================================
void setup() {
  Serial.begin(115200);             // 初始化序列埠，鮑率 115200
  pinMode(HALL_PIN,  INPUT);        // Hall 感測器腳位設為輸入
  pinMode(RELAY_PIN, OUTPUT);       // 繼電器腳位設為輸出
  digitalWrite(RELAY_PIN, LOW);     // 初始狀態：繼電器 OFF
  delay(500);                       // 等待穩定
  Serial.println("IMRC Controller Ready");
}

// ============================================================
// loop：主迴圈，持續執行
// ============================================================
void loop() {

  // ---- 1. 收集脈衝，計時 ----------------------------------
  float hall_count = 1.0;           // 從 1 開始避免除以 0
  float start_time = micros();      // 記錄開始時間（微秒）
  bool  on_state   = false;         // 用來避免同一次脈衝重複計數

  while (true) {
    if (digitalRead(HALL_PIN) == LOW) {
      // 偵測到 LOW（脈衝觸發）
      if (on_state == false) {
        on_state = true;            // 標記為「已在脈衝中」
        hall_count += 1.0;         // 計數 +1
      }
    } else {
      on_state = false;             // 脈衝結束，重置旗標
    }

    // 收集到足夠脈衝數後跳出
    if (hall_count >= HALL_THRESH) {
      break;
    }
  }

  // ---- 2. 計算 RPM ----------------------------------------
  // 公式：RPM = (脈衝數 / 每轉脈衝數) / 經過秒數 × 60
  float end_time    = micros();
  float time_passed = (end_time - start_time) / 1000000.0;  // 轉換成秒
  current_rpm = (int)((hall_count / PULSE_PER_REV) / time_passed * 60.0);

  // ---- 3. 序列埠輸出，供監控與除錯使用 --------------------
  Serial.print("Time Passed: ");
  Serial.print(time_passed, 3);     // 顯示小數點後 3 位
  Serial.println(" s");
  Serial.print("RPM: ");
  Serial.println(current_rpm);

  // ---- 4. 遲滯判斷，控制繼電器 ----------------------------
  if (current_rpm >= RPM_ON_THRESHOLD) {
    // 轉速超過 6000 RPM → 繼電器 ON
    relay_active = true;

  } else if (current_rpm < RPM_OFF_THRESHOLD) {
    // 轉速低於 5800 RPM → 繼電器 OFF
    relay_active = false;

  }
  // 5800 <= RPM < 6000：不更新 relay_active，保持上一次狀態

  // ---- 5. 輸出繼電器動作 ----------------------------------
  if (relay_active) {
    digitalWrite(RELAY_PIN, HIGH);  // 繼電器 ON
  } else {
    digitalWrite(RELAY_PIN, LOW);   // 繼電器 OFF
  }

  // ---- 6. 序列埠輸出繼電器狀態 ----------------------------
  Serial.print("Relay: ");
  Serial.println(relay_active ? "ON" : "OFF");
  Serial.println("--------------------");

  delay(1);  // 短暫延遲，維持穩定性
}
