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
#define LINE_HEIGHT 18  // 日本語フォント用に高さを調整
#define MAX_LINES (DISPLAY_HEIGHT / LINE_HEIGHT)

// 受信データバッファ
String receivedData = "";
String displayLines[MAX_LINES];
int currentLine = 0;

// UTF-8文字処理用
bool isUTF8Sequence = false;
int utf8BytesExpected = 0;
int utf8BytesReceived = 0;
String utf8Buffer = "";

void setup() {
  // M5Stack初期化
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(200);
  M5.Display.clear();
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(WHITE, BLACK);
  
  // 日本語フォント設定
  M5.Display.setFont(&fonts::lgfxJapanGothic_12);
  
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
  M5.Display.println("Baud: 9600");
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
    uint8_t incomingByte = UartPortC.read();
    
    // デバッグ: 受信した生バイトを表示
    if (debugMode) {
      Serial.printf("Raw byte: 0x%02X (%d) '%c'\n", incomingByte, incomingByte, 
                    isPrintable(incomingByte) ? incomingByte : '?');
    }
    
    // UTF-8文字処理
    bool processComplete = false;
    
    if (incomingByte == '\n' || incomingByte == '\r') {
      // 改行文字を受信した場合、完成した行を処理
      if (receivedData.length() > 0) {
        processComplete = true;
      }
    } else if (incomingByte < 0x80) {
      // ASCII文字（1バイト）
      receivedData += (char)incomingByte;
      if (receivedData.length() > 200) {
        processComplete = true;
      }
    } else {
      // UTF-8マルチバイト文字の処理
      if (!isUTF8Sequence) {
        // UTF-8シーケンスの開始
        if ((incomingByte & 0xE0) == 0xC0) {
          // 2バイト文字
          utf8BytesExpected = 2;
        } else if ((incomingByte & 0xF0) == 0xE0) {
          // 3バイト文字（日本語ひらがな・カタカナ・漢字）
          utf8BytesExpected = 3;
        } else if ((incomingByte & 0xF8) == 0xF0) {
          // 4バイト文字
          utf8BytesExpected = 4;
        } else {
          // 無効なUTF-8開始バイト
          if (debugMode) {
            Serial.printf("Invalid UTF-8 start byte: 0x%02X\n", incomingByte);
          }
          // 無効なバイトは無視して次のバイトを待つ
          utf8BytesExpected = 0;
        }
        
        if (utf8BytesExpected > 0) {
          isUTF8Sequence = true;
          utf8BytesReceived = 1;
          utf8Buffer = "";
          utf8Buffer += (char)incomingByte;
          
          if (debugMode) {
            Serial.printf("UTF-8 sequence start: %d bytes expected\n", utf8BytesExpected);
          }
        }
      } else {
        // UTF-8シーケンスの継続
        if ((incomingByte & 0xC0) == 0x80) {
          utf8Buffer += (char)incomingByte;
          utf8BytesReceived++;
          
          if (utf8BytesReceived >= utf8BytesExpected) {
            // UTF-8文字完成
            receivedData += utf8Buffer;
            if (debugMode) {
              Serial.printf("UTF-8 character complete: %s\n", utf8Buffer.c_str());
            }
            
            // リセット
            isUTF8Sequence = false;
            utf8BytesExpected = 0;
            utf8BytesReceived = 0;
            utf8Buffer = "";
            
            if (receivedData.length() > 200) {
              processComplete = true;
            }
          }
        } else {
          // 無効なUTF-8継続バイト
          if (debugMode) {
            Serial.printf("Invalid UTF-8 continuation byte: 0x%02X\n", incomingByte);
          }
          // リセット
          isUTF8Sequence = false;
          utf8BytesExpected = 0;
          utf8BytesReceived = 0;
          utf8Buffer = "";
        }
      }
    }
    
    // 完成した行を処理
    if (processComplete) {
      Serial.println("UART RX: " + receivedData);
      addLineToDisplay("RX: " + receivedData);
      receivedData = "";
      
      // UTF-8シーケンスもリセット
      isUTF8Sequence = false;
      utf8BytesExpected = 0;
      utf8BytesReceived = 0;
      utf8Buffer = "";
    }
  }
  
  // ボタンA: 日本語テストデータ送信
  if (M5.BtnA.wasPressed()) {
    static int testIndex = 0;
    String testMessages[] = {
      "{\"message\":\"こんにちは\",\"type\":\"greeting\"}",
      "{\"sensor\":\"温度\",\"value\":25.6,\"unit\":\"℃\"}",
      "{\"status\":\"正常\",\"device\":\"センサー1\"}",
      "{\"text\":\"日本語テスト\",\"encoding\":\"UTF-8\"}",
      "{\"data\":\"ひらがな・カタカナ・漢字\",\"test\":true}"
    };
    
    String testData = testMessages[testIndex];
    testIndex = (testIndex + 1) % 5;
    
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
    M5.Display.println("Baud: 9600");
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
