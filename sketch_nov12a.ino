// ================= 腳位與硬體設定 =================
const int RED_PIN    = 9;   // 紅色腳位 (PWM)
const int GREEN_PIN  = 10;  // 綠色腳位 (PWM)
const int BLUE_PIN   = 11;  // 藍色腳位 (PWM)
const int BUTTON_PIN = 2;   // 連接按鈕模組的 Input 腳位

// 已設定為共陽極 RGB LED (最長腳接 5V)
const bool COMMON_ANODE = true;

// ================= 顏色庫設定 =================
const int NUM_COLORS = 7;
const byte COLORS[NUM_COLORS][3] = {
  {255, 0,   0  }, // 0: 紅色 (Red)
  {0,   255, 0  }, // 1: 綠色 (Green)
  {0,   0,   255}, // 2: 藍色 (Blue)
  {255, 180, 0  }, // 3: 黃色 (Yellow)
  {0,   255, 255}, // 4: 青色 (Cyan)
  {255, 0,   255}, // 5: 紫色 (Magenta)
  {255, 255, 255}  // 6: 白色 (White)
};
const char* COLOR_NAMES[NUM_COLORS] = {
  "Red", "Green", "Blue", "Yellow", "Cyan", "Magenta", "White"
};

// ================= 狀態變數 =================
// 模式: 0=恆亮, 1=閃爍, 2=呼吸燈, 3=彩虹(進階), 4=心跳(進階雙擊)
int currentMode = 0;
int previousMode = 0;
int colorIndex = 0;

// 按鈕偵測變數 (未按下為 HIGH，按下為 LOW)
int lastReading = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long pressStartTime = 0;
unsigned long lastReleaseTime = 0;
bool longPressHandled = false;
int clickCount = 0;

const unsigned long DEBOUNCE_DELAY   = 30;   // 彈跳消除時間 (ms)
const unsigned long LONG_PRESS_TIME  = 600;  // 長按判定時間 (ms)
const unsigned long DOUBLE_CLICK_GAP = 250;  // 雙擊最大間隔 (ms)

// 燈效計時變數
unsigned long lastEffectTime = 0;
bool blinkState = true;
byte rainbowHue = 0;

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  
  // 按鈕模組本身已有上拉電阻，直接設為 INPUT 即可
  pinMode(BUTTON_PIN, INPUT); 

  // 開機先關閉所有燈光避免閃爍
  setRGB(0, 0, 0);

  Serial.begin(9600);
  Serial.println("=== Light Stick Ready (Common Anode) ===");
  printStatus();
}

void loop() {
  handleButton();
  updateLightEffect();
}

// ================= 按鈕互動邏輯 =================
void handleButton() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        // 按鈕剛被按下 (LOW)
        pressStartTime = millis();
        longPressHandled = false;
      } else {
        // 按鈕剛被放開 (回到 HIGH)
        if (!longPressHandled) {
          clickCount++;
          lastReleaseTime = millis();
        }
      }
    }
  }

  // 1. 判定長按（按住超過 600ms 切換模式）
  if (buttonState == LOW && !longPressHandled) {
    if (millis() - pressStartTime >= LONG_PRESS_TIME) {
      longPressHandled = true;
      clickCount = 0;

      if (currentMode == 4) {
        currentMode = 0;
      } else {
        currentMode = (currentMode + 1) % 4;
      }
      printStatus();
    }
  }

  // 2. 判定短按與雙擊
  if (clickCount > 0 && buttonState == HIGH) {
    if (clickCount >= 2) {
      // 雙擊：切換進階「心跳燈效模式」
      clickCount = 0;
      if (currentMode != 4) {
        previousMode = currentMode;
        currentMode = 4;
      } else {
        currentMode = previousMode;
      }
      printStatus();
    } else if (millis() - lastReleaseTime > DOUBLE_CLICK_GAP) {
      // 單次短按：切換顏色
      clickCount = 0;
      colorIndex = (colorIndex + 1) % NUM_COLORS;
      printStatus();
    }
  }

  lastReading = reading;
}

// ================= 燈光模式控制 =================
void updateLightEffect() {
  byte r = COLORS[colorIndex][0];
  byte g = COLORS[colorIndex][1];
  byte b = COLORS[colorIndex][2];

  switch (currentMode) {
    case 0: // 恆亮模式 (Constant Light)
      setRGB(r, g, b);
      break;

    case 1: // 閃爍模式 (Blink - 每 300ms 切換明暗)
      if (millis() - lastEffectTime >= 300) {
        lastEffectTime = millis();
        blinkState = !blinkState;
      }
      if (blinkState) {
        setRGB(r, g, b);
      } else {
        setRGB(0, 0, 0);
      }
      break;

    case 2: { // 呼吸燈模式 (Breathing Light - 週期 2 秒)
      float phase = (millis() % 2000) / 2000.0;
      float brightness = (sin(phase * 2.0 * PI - PI / 2.0) + 1.0) / 2.0;
      setRGB(r * brightness, g * brightness, b * brightness);
      break;
    }

    case 3: // 進階模式：彩虹漸變 (Rainbow Mode)
      if (millis() - lastEffectTime >= 15) {
        lastEffectTime = millis();
        rainbowHue++;
      }
      setWheelColor(rainbowHue);
      break;

    case 4: { // 進階模式：心跳燈效 (Heartbeat Mode - 週期 1.2 秒)
      unsigned long t = millis() % 1200;
      float beat = 0.05;
      if (t < 150) {
        beat = 1.0;
      } else if (t >= 250 && t < 400) {
        beat = 0.85;
      }
      setRGB(r * beat, g * beat, b * beat);
      break;
    }
  }
}

// ================= 輸出 RGB 訊號 =================
void setRGB(byte r, byte g, byte b) {
  if (COMMON_ANODE) {
    // 共陽極反相計算：0 為全亮，255 為全暗
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

// 彩虹色輪轉換函式 (0~255)
void setWheelColor(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    setRGB(255 - pos * 3, 0, pos * 3);
  } else if (pos < 170) {
    pos -= 85;
    setRGB(0, pos * 3, 255 - pos * 3);
  } else {
    pos -= 170;
    setRGB(pos * 3, 255 - pos * 3, 0);
  }
}

// 序列埠狀態顯示
void printStatus() {
  const char* modeNames[] = {
    "Constant Light (恆亮)",
    "Blink (閃爍)",
    "Breathing Light (呼吸燈)",
    "Rainbow Mode [進階:彩虹]",
    "Heartbeat Mode [進階:雙擊心跳]"
  };
  Serial.print("Mode: ");
  Serial.print(modeNames[currentMode]);
  Serial.print(" | Color: ");
  Serial.println(COLOR_NAMES[colorIndex]);
}