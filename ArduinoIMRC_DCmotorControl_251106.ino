/*
 * IMRC 馬達控制器 - 非阻塞 (millis()) 程式碼
 * 目的: 模擬 BTS7960 的 "高電流湧浪 (Peak) + 低電流保持 (Hold)" 邏輯。
 * 額外功能: 模擬 IMRC 開啟時的 75 Ohm 電阻回饋訊號。
 */

// ST170 IMRC 馬達控制參數
// LM1949 Rt=150K, Ct=10uF 控制參數
// Rt*Ct = (150,000 Ohm) * (0.000010 F) = 1.5 秒
// Rs = 0.05r, 峰值電流Ip = 7.7A, 保持電流 Ih = 1.88A
const unsigned long T_PEAK_MS = 1500*8;    // 1.5 秒 * 8ms = 12,000 // 湧浪電流時間 (毫秒)，可調整 (50-100ms)
const int PWM_PEAK = 230;                 // 湧浪狀態 PWM (全開 255)，可調整以換取較小的電流和較溫和的啟動。
const int PWM_HOLD = 51;                 // 保持狀態 PWM (低電流)，可調整 (50-100)

// --- 軟體去彈跳 (Debounce) 參數 ---
const unsigned long DEBOUNCE_TIME_MS = 300*8; // 0.3秒 * 8ms = 2,400 // 訊號必須持續穩定的時間 (毫秒)
unsigned long input_stable_time = 0; // 用於記錄訊號穩定的開始時間
bool last_pcm_request_open = false; // 記錄上一次 loop 讀取的請求狀態

// --- 新增：濾波後的穩定狀態 ---
bool stable_request_state = false;

const int IMRC_INPUT_PIN = 4;             // D4連接 PCM IMRC Control Pin 1 (Active Low)

// BTS7960 驅動器腳位
const int RPWM = 5;                      // D5開啟馬達方向 (PWM)
const int LPWM = 6;                      // D6關閉方向 (保持 LOW)
const int L_EN = 7;                      // D7啟用 L 側 (HIGH)
const int R_EN = 8;                      // D8啟用 R 側 (HIGH)

// 感應電路模擬輸出腳位
// 請將這兩個腳位連接到您外部電路的適當位置，以實現 75 Ohm 模擬
const int SENSOR_LEFT_PIN = 9;           // D9模擬左側開關 (用於 75 Ohm)
const int SENSOR_RIGHT_PIN = 10;         // D10模擬右側開關 (用於 75 Ohm)

// 狀態管理變數
enum MotorState {
  STATE_OFF,          // 關閉狀態
  STATE_PEAK,         // 湧浪高電流狀態
  STATE_HOLD          // 保持低電流狀態
};
MotorState current_state = STATE_OFF;
unsigned long peak_start_time = 0; // 用於記錄湧浪開始時間

void setup() {
  Serial.begin(9600);
  //Serial.begin(19200);
  Serial.println("--- IMRC Motor Controller Initializing ---"); // 除錯訊息
  // 提高 PWM 頻率
  // 設定 Timer0 (Pin 5/6) RPWM/LPWM分頻器設為約 62.5kHz
  //TCCR0B = TCCR0B & B11111000 | B00000001; //約 62.5kHz
  TCCR0B = TCCR0B & B11111000 | B00000010; //約 20kHz
  // 設定 Timer1 (Pin 9/10) PWM 頻率到 20kHz
  //TCCR1B = TCCR1B & B11111000 | B00000010;

  // 設置所有輸出腳位
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
  pinMode(SENSOR_LEFT_PIN, OUTPUT);
  pinMode(SENSOR_RIGHT_PIN, OUTPUT);
  
  // 設置 IMRC 輸入腳位 (使用上拉電阻，因為 PCM 訊號是 Active Low)
  pinMode(IMRC_INPUT_PIN, INPUT_PULLUP);
  
  // 初始狀態: 馬達關閉，H 橋啟用
  digitalWrite(R_EN, HIGH); 
  digitalWrite(L_EN, HIGH);
  analogWrite(RPWM, 0); 
  //analogWrite(LPWM, 0); 
  digitalWrite(LPWM, LOW); // 固定 LOW

  // 初始狀態: 模擬感應電路為 IMRC 關閉狀態 (兩側開關打開 -> 無限電阻)
  digitalWrite(SENSOR_LEFT_PIN, HIGH);
  digitalWrite(SENSOR_RIGHT_PIN, HIGH);

  Serial.println("Initial State: OFF. Ready to receive IMRC request."); // 除錯訊息
}

void loop() {
  // 1. 讀取原始訊號
  // 讀取 PCM 請求 (LOW = 請求開啟 IMRC)
  bool pcm_request_open = (digitalRead(IMRC_INPUT_PIN) == LOW);

  // 2. 雙向濾波邏輯：只有訊號維持穩定超過 DEBOUNCE_TIME_MS，才更新 stable_request_state
  if (pcm_request_open != last_pcm_request_open) {
    input_stable_time = millis(); // 只要訊號一跳動，重新計時
  }

  if ((millis() - input_stable_time) >= DEBOUNCE_TIME_MS) {
    // 訊號已經穩定足夠久了，更新穩定狀態
    if (pcm_request_open != stable_request_state) {
      stable_request_state = pcm_request_open;
      Serial.print("Signal Confirmed: ");
      Serial.println(stable_request_state ? "ON" : "OFF");
    }
  }
  last_pcm_request_open = pcm_request_open;

// 3. 狀態機切換 (使用穩定後的 stable_request_state)
  switch (current_state) {
    case STATE_OFF:
      if (stable_request_state) { // 使用過濾後的訊號
        current_state = STATE_PEAK;
        peak_start_time = millis();
        digitalWrite(LPWM, LOW);     // 1. 先確認低電位方向
        analogWrite(RPWM, PWM_PEAK); // 2. 直接啟動   
        Serial.println("State: OFF -> PEAK");
      }
      break;
      
    case STATE_PEAK:
      // 如果穩定狀態變為 OFF，才關閉馬達
      if (!stable_request_state) {
        current_state = STATE_OFF;
        analogWrite(RPWM, 0);  // 直接關閉，保護交給 TVS 二極體
        delay(2); // 加一個 delay(2)，進入下一次循環前，晶片有時間釋放能量
        digitalWrite(SENSOR_LEFT_PIN, HIGH);
        digitalWrite(SENSOR_RIGHT_PIN, HIGH);
        Serial.println("State: PEAK -> OFF (Signal Lost)");
        break;
      }
      
      if (millis() - peak_start_time >= T_PEAK_MS) {
        current_state = STATE_HOLD;
        // --- 平滑切換，不中斷電力 ---
        analogWrite(RPWM, PWM_HOLD);
        digitalWrite(SENSOR_LEFT_PIN, LOW);
        digitalWrite(SENSOR_RIGHT_PIN, LOW);
        Serial.println("State: PEAK -> HOLD");
      }
      break;
      
    case STATE_HOLD:
      // 如果穩定狀態變為 OFF，才關閉馬達
      if (!stable_request_state) {
        current_state = STATE_OFF;
        analogWrite(RPWM, 0);  // 直接關閉，保護交給 TVS 二極體
        delay(2); // 加一個 delay(2)，進入下一次循環前，晶片有時間釋放能量
        digitalWrite(SENSOR_LEFT_PIN, HIGH);
        digitalWrite(SENSOR_RIGHT_PIN, HIGH);
        Serial.println("State: HOLD -> OFF");
      }
      break;
  }
}
