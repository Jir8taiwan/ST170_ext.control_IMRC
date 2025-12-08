/*
 * IMRC 馬達控制器 - 非阻塞 (millis()) 程式碼
 * 目的: 模擬 BTS7960 的 "高電流湧浪 (Peak) + 低電流保持 (Hold)" 邏輯。
 * 額外功能: 模擬 IMRC 開啟時的 75 Ohm 電阻回饋訊號。
 */

// ST170 IMRC 馬達控制參數
// LM1949 Rt=150K, Ct=10uF 控制參數
// Rt*Ct = (150,000 Ohm) * (0.000010 F) = 1.5 秒
// Rs = 0.05r, 峰值電流Ip = 7.7A, 保持電流 Ih = 1.88A
const unsigned long T_PEAK_MS = 1500;      // 湧浪電流時間 (毫秒)，可調整 (50-100ms)
const int PWM_PEAK = 236;                 // 湧浪狀態 PWM (全開 255)，可調整以換取較小的電流和較溫和的啟動。
const int PWM_HOLD = 64;                 // 保持狀態 PWM (低電流)，可調整 (50-100)

// --- 軟體去彈跳 (Debounce) 參數 ---
const unsigned long DEBOUNCE_TIME_MS = 100; // 訊號必須持續穩定的時間 (毫秒)
unsigned long input_stable_time = 0; // 用於記錄訊號穩定的開始時間
bool last_pcm_request_open = false; // 記錄上一次 loop 讀取的請求狀態

// IMRC 訊號輸入腳位
const int IMRC_INPUT_PIN = 4;             // 連接 PCM IMRC Control Pin 1 (Active Low)

// BTS7960 驅動器腳位
const int RPWM = 5;                      // 開啟馬達方向 (PWM)
const int LPWM = 6;                      // 關閉方向 (保持 LOW)
const int L_EN = 7;                      // 啟用 L 側 (HIGH)
const int R_EN = 8;                      // 啟用 R 側 (HIGH)

// 感應電路模擬輸出腳位
// 請將這兩個腳位連接到您外部電路的適當位置，以實現 75 Ohm 模擬
const int SENSOR_LEFT_PIN = 9;           // 模擬左側開關 (用於 75 Ohm)
const int SENSOR_RIGHT_PIN = 10;         // 模擬右側開關 (用於 75 Ohm)

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
  TCCR1B = TCCR1B & B11111000 | B00000010;

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
  // 讀取 PCM 請求 (LOW = 請求開啟 IMRC)
  bool pcm_request_open = (digitalRead(IMRC_INPUT_PIN) == LOW);
  
  switch (current_state) {
    case STATE_OFF:
      // 狀態: 關閉
      // --- 軟體去彈跳邏輯 (Debounce Logic) ---
      if (pcm_request_open != last_pcm_request_open) {
        // 偵測到訊號變化，重設計時器
        input_stable_time = millis();
      }
      
      if (pcm_request_open&& (millis() - input_stable_time >= DEBOUNCE_TIME_MS)) {
        // 收到開啟請求 -> 進入湧浪狀態
        current_state = STATE_PEAK;
        peak_start_time = millis();
        
        // 動作: 開始高電流湧浪
        analogWrite(RPWM, PWM_PEAK);
        //analogWrite(LPWM, 0); 
        digitalWrite(LPWM, LOW);
        
        // 動作: 模擬感應電路為 IMRC 開啟狀態 (兩側開關閉合 -> 75 Ohm)
        //digitalWrite(SENSOR_LEFT_PIN, LOW);
        //digitalWrite(SENSOR_RIGHT_PIN, LOW);
        
        Serial.print("State Change: OFF -> PEAK. Time: "); // 除錯訊息
        Serial.println(millis());                         // 除錯訊息
        Serial.print("Action: Set RPWM to PEAK (");        // 除錯訊息
        Serial.print(PWM_PEAK);                            // 除錯訊息
        Serial.println("). Simulating 75 Ohm feedback.");  // 除錯訊息
      }
      // 更新上一次的狀態
      last_pcm_request_open = pcm_request_open;
      break;
      
    case STATE_PEAK:
      // 狀態: 湧浪高電流
      if (!pcm_request_open) {
        // 開啟請求中途取消 -> 立即關閉
        current_state = STATE_OFF;
        analogWrite(RPWM, 0);
        digitalWrite(LPWM, LOW);
        
        digitalWrite(SENSOR_LEFT_PIN, HIGH);
        digitalWrite(SENSOR_RIGHT_PIN, HIGH);

        // *重設 Debounce 變數，以便下次開啟*
        last_pcm_request_open = pcm_request_open;
        input_stable_time = millis(); // 離開時將計時器設為當前時間，以備下次訊號穩定
        
        Serial.print("State Change: PEAK -> OFF (Request Canceled). Time: "); // 除錯訊息
        Serial.println(millis());                                           // 除錯訊息
        Serial.println("Action: Motor Stopped. Feedback reset.");           // 除錯訊息
        break; // 跳出 case 
      }
      
      // 檢查湧浪時間是否已到
      if (millis() - peak_start_time >= T_PEAK_MS) {
        // 時間已到 -> 進入保持狀態
        current_state = STATE_HOLD;
        
        // 動作: 切換到低電流保持
        analogWrite(RPWM, PWM_HOLD);
        digitalWrite(LPWM, LOW);
        // 動作: 模擬感應電路為 IMRC 開啟狀態 (兩側開關閉合 -> 75 Ohm)
        digitalWrite(SENSOR_LEFT_PIN, LOW);
        digitalWrite(SENSOR_RIGHT_PIN, LOW);
        
        Serial.print("State Change: PEAK -> HOLD. Time: "); // 除錯訊息
        Serial.println(millis());                           // 除錯訊息
        Serial.print("Action: Set RPWM to HOLD (");         // 除錯訊息
        Serial.print(PWM_HOLD);                             // 除錯訊息
        Serial.println(").");                              // 除錯訊息
      }
      break;
      
    case STATE_HOLD:
      // 狀態: 保持低電流
      if (!pcm_request_open) {
        // 請求已移除 -> 關閉
        current_state = STATE_OFF;
        
        // 動作: 停止馬達
        analogWrite(RPWM, 0);
        digitalWrite(LPWM, LOW);
        
        // 1.5 秒動作: 模擬感應電路為 IMRC 關閉狀態 (兩側開關打開 -> 無限電阻)
        digitalWrite(SENSOR_LEFT_PIN, HIGH);
        digitalWrite(SENSOR_RIGHT_PIN, HIGH);

        // 重設 Debounce 變數
        last_pcm_request_open = pcm_request_open;
        input_stable_time = millis(); // 離開時將計時器設為當前時間
        
        Serial.print("State Change: HOLD -> OFF. Time: "); // 除錯訊息
        Serial.println(millis());                          // 除錯訊息
        Serial.println("Action: Motor Stopped. Feedback reset."); // 除錯訊息
      }
      // 否則，持續保持 analogWrite(RPWM, PWM_HOLD); 狀態
      break;
  }
}
