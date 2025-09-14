#include <Arduino.h>
#include <M5Unified.h>
#include <HardwareSerial.h>

// Port C UART設定 (M5Stack Core2)
// Port C: GPIO13 (RX), GPIO14 (TX)
#define UART_RX_PIN 13
#define UART_TX_PIN 14
#define UART_BAUD_RATE 115200

// UART2を使用（Serial2）
HardwareSerial UartPortC(2);

// デバッグ用フラグ
bool debugMode = true;

// ディスプレイ設定
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240
#define LINE_HEIGHT 16
#define MAX_LINES (DISPLAY_HEIGHT / LINE_HEIGHT)

// 受信データバッファ
String receivedData = "";
String displayLines[MAX_LINES];
int currentLine = 0;

void setup() {
  // M5Stack初期化
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(200);
  M5.Display.clear();
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE, BLACK);
  
  // シリアル初期化
  Serial.begin(115200);
  Serial.println("=== M5Stack Core2 UART Test (Port C) ===");
  Serial.println("Starting UART communication test...");
  
  // Port C UART初期化
  Serial.println("Initializing UART...");
  
  // UARTを一度終了してから再初期化
  UartPortC.end();
  delay(100);
  
  // UART初期化（ピンを明示的に指定）
  UartPortC.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.printf("UART initialized: RX=GPIO%d, TX=GPIO%d, Baud=%d\n", 
                UART_RX_PIN, UART_TX_PIN, UART_BAUD_RATE);
  
  // UART設定確認
  Serial.printf("UART timeout: %d ms\n", UartPortC.getTimeout());
  
  // 少し待機してから開始
  delay(500);
  
  // ディスプレイ初期表示
  M5.Display.setCursor(0, 0);
  M5.Display.println("UART Port C Monitor");
  M5.Display.println("RX: GPIO13, TX: GPIO14");
  M5.Display.println("Baud: 115200");
  M5.Display.println("Waiting for data...");
  M5.Display.println("-------------------");
  
  // 表示行配列初期化
  for (int i = 0; i < MAX_LINES; i++) {
    displayLines[i] = "";
  }
  
  Serial.println("Setup complete. Waiting for UART data...");
}

void addLineToDisplay(const String& line) {
  // 新しい行を配列に追加
  displayLines[currentLine] = line;
  currentLine = (currentLine + 1) % MAX_LINES;
  
  // ディスプレイを更新
  M5.Display.clear();
  M5.Display.setCursor(0, 0);
  M5.Display.println("UART Port C Monitor");
  M5.Display.println("RX: GPIO13, TX: GPIO14");
  M5.Display.println("Baud: 115200");
  M5.Display.println("-------------------");
  
  // 受信データを表示（最新のものから順に）
  int displayLine = 4; // ヘッダー分をスキップ
  for (int i = 0; i < MAX_LINES - 4; i++) {
    int index = (currentLine + i) % MAX_LINES;
    if (displayLines[index].length() > 0) {
      M5.Display.setCursor(0, displayLine * LINE_HEIGHT);
      // 長い行は切り詰める
      String displayText = displayLines[index];
      if (displayText.length() > 40) {
        displayText = displayText.substring(0, 37) + "...";
      }
      M5.Display.println(displayText);
      displayLine++;
    }
  }
}

void loop() {
  M5.update();
  
  // UART受信データをチェック
  if (UartPortC.available()) {
    char incomingByte = UartPortC.read();
    
    // デバッグ: 受信した生バイトを表示
    if (debugMode) {
      Serial.printf("Raw byte: 0x%02X (%d) '%c'\n", incomingByte, incomingByte, 
                    isPrintable(incomingByte) ? incomingByte : '?');
    }
    
    if (incomingByte == '\n' || incomingByte == '\r') {
      // 改行文字を受信した場合、完成した行を処理
      if (receivedData.length() > 0) {
        // シリアルに出力
        Serial.println("UART RX: " + receivedData);
        
        // ディスプレイに追加
        addLineToDisplay("RX: " + receivedData);
        
        // バッファをクリア
        receivedData = "";
      }
    } else if (isPrintable(incomingByte)) {
      // 印刷可能文字を受信した場合、バッファに追加
      receivedData += incomingByte;
      
      // バッファが長すぎる場合は強制的に行を完成させる
      if (receivedData.length() > 200) {
        Serial.println("UART RX (long): " + receivedData);
        addLineToDisplay("RX: " + receivedData);
        receivedData = "";
      }
    } else {
      // 非印刷可能文字もログに記録
      if (debugMode) {
        Serial.printf("Non-printable byte: 0x%02X\n", incomingByte);
      }
    }
  }
  
  // ボタンA: テストデータ送信
  if (M5.BtnA.wasPressed()) {
    String testData = "{\"test\":\"data\",\"timestamp\":" + String(millis()) + "}";
    UartPortC.println(testData);
    Serial.println("UART TX: " + testData);
    
    // 送信データも表示に追加
    addLineToDisplay("TX: " + testData);
  }
  
  // ボタンB: ディスプレイクリア
  if (M5.BtnB.wasPressed()) {
    for (int i = 0; i < MAX_LINES; i++) {
      displayLines[i] = "";
    }
    currentLine = 0;
    
    M5.Display.clear();
    M5.Display.setCursor(0, 0);
    M5.Display.println("UART Port C Monitor");
    M5.Display.println("RX: GPIO13, TX: GPIO14");
    M5.Display.println("Baud: 115200");
    M5.Display.println("Display cleared");
    M5.Display.println("-------------------");
    
    Serial.println("Display cleared");
  }
  
  // ボタンC: 統計情報表示
  if (M5.BtnC.wasPressed()) {
    Serial.println("=== UART Statistics ===");
    Serial.printf("Available bytes: %d\n", UartPortC.available());
    Serial.printf("Current buffer: %s\n", receivedData.c_str());
    Serial.printf("Buffer length: %d\n", receivedData.length());
    Serial.println("=====================");
  }
  
  delay(10); // CPU負荷軽減
}
