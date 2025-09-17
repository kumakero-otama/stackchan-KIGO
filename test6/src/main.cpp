#include <Arduino.h>
#include <M5Unified.h>
#include <Avatar.h>
#include <ESP32Servo.h>
#include <HardwareSerial.h>
#include "TextAnimator.h"
#include "config.h"
#include "NarrowEye.h"
#include "PoetFace.h"
#include "PhoneticMouth.h"

using namespace m5avatar;

// config.hのextern宣言に対応する実際の定義
const char* EXPRESSION_NAMES[] = {"中立", "嬉しい", "怒り", "悲しい", "疑問", "眠い", "俳人"};
const int EXPRESSION_COUNT = 7;
const int MAX_CHARS_PER_LINE = 10;  // 1行あたりの最大文字数
const int MAX_LINES = 3;            // 最大行数
const bool USE_JAPANESE_FONT = true;
const int BEEP_FREQUENCIES[] = {
  1000,  // 中立 (Neutral) - 標準
  1400,  // 嬉しい (Happy) - 高め (ポジティブ)
  600,   // 怒り (Angry) - 低め (ネガティブ)
  500,   // 悲しい (Sad) - とても低め (ネガティブ)
  1100,  // 疑問 (Doubt) - やや高め
  800,   // 眠い (Sleepy) - やや低め
  700    // 俳人 (Poet) - 低め・落ち着いた音色
};

// Servo objects
Servo ServoX;  // X軸（左右）
Servo ServoY;  // Y軸（上下）

// Pin definitions (from config.h)
// サーボピンはconfig.hで設定されています

// Servo positions (from config.h)
#define HOME_POSITION_X SERVO_HOME_X
#define HOME_POSITION_Y SERVO_HOME_Y
#define RIGHT_POSITION SERVO_RIGHT_POSITION
#define UP_POSITION SERVO_UP_POSITION
#define NOD_DOWN_POSITION SERVO_NOD_DOWN
#define SHAKE_RIGHT_POSITION SERVO_SHAKE_RIGHT
#define SHAKE_LEFT_POSITION SERVO_SHAKE_LEFT

// Easing parameters
#define REFRESH_INTERVAL_MILLIS 20

// Current servo positions
int currentX = HOME_POSITION_X;
int currentY = HOME_POSITION_Y;

// Movement state
bool isMovingX = false;
bool isMovingY = false;
unsigned long moveStartTime = 0;
int startAngleX, targetAngleX, startAngleY, targetAngleY;
unsigned long moveDuration = 0;

Avatar avatar;
TextAnimator textAnimator(&avatar);

int currentLyricsIndex = 0;
String lastPlayedSpeech = "";   // セリフリピート用バッファ（表示文字列）
String lastPlayedPhonetic = ""; // セリフリピート用バッファ（発音文字列）

// 表情切り替え制御用
int expression_index = 0;
const Expression expressions[] = {
  Expression::Neutral,
  Expression::Happy,
  Expression::Angry,
  Expression::Sad,
  Expression::Doubt,
  Expression::Sleepy
};

// カスタム俳人フェイス
PoetFace* poetFace = nullptr;
Face* originalFace = nullptr;
bool isPoetMode = false;

// PhoneticMouth（音素別口形状制御）
PhoneticMouth* phoneticMouth = nullptr;
int phoneticIndex = 0;  // aiueo音素インデックス
const String phonemes[] = {"あ", "い", "う", "え", "お"};
const int PHONEME_COUNT = 5;

// 動作パターン切り替え用
bool isNodTurn = true;  // true: うなずき, false: 首振り

// UART Port C設定 (M5Stack Core2)
// Port C: GPIO13 (RX), GPIO14 (TX)
#define UART_RX_PIN 13
#define UART_TX_PIN 14
#define UART_BAUD_RATE 115200

// UART2を使用（Serial2）
HardwareSerial UartPortC(2);

// UART受信データバッファ（大容量対応）
String receivedData = "";
const int MAX_BUFFER_SIZE = 4096;  // 4KBまで対応（超長文メッセージ対応）

// UTF-8文字処理用
bool isUTF8Sequence = false;
int utf8BytesExpected = 0;
int utf8BytesReceived = 0;
String utf8Buffer = "";

// デバッグ用フラグ
bool debugMode = true;  // UART受信デバッグを有効化

// デバッグ用カウンタ
unsigned long totalBytesReceived = 0;
unsigned long lastDebugTime = 0;
const unsigned long DEBUG_INTERVAL = 1000; // 1秒間隔でデバッグ情報表示

// 改行文字重複処理防止用
uint8_t lastNewlineChar = 0;  // 最後に受信した改行文字を記録

// メッセージ重複処理防止用
String lastProcessedMessage = "";  // 最後に処理したメッセージ
unsigned long lastProcessedTime = 0;  // 最後に処理した時刻
const unsigned long MESSAGE_COOLDOWN = 3000;  // 3秒間のクールダウン

// Cubic easing function (based on ServoEasing library)
float easeCubicInOut(float t) {
  if (t < 0.5) {
    return 4 * t * t * t;
  } else {
    float f = ((2 * t) - 2);
    return 1 + f * f * f / 2;
  }
}

// Start easing movement for X servo
void startEaseToX(int targetAngle, unsigned long duration) {
  if (targetAngle < 0) targetAngle = 0;
  if (targetAngle > 180) targetAngle = 180;
  
  startAngleX = currentX;
  targetAngleX = targetAngle;
  moveDuration = duration;
  moveStartTime = millis();
  isMovingX = true;
  
  Serial.printf("Starting X movement: %d -> %d degrees in %lu ms\n", 
                startAngleX, targetAngleX, duration);
}

// Start easing movement for Y servo
void startEaseToY(int targetAngle, unsigned long duration) {
  if (targetAngle < 0) targetAngle = 0;
  if (targetAngle > 180) targetAngle = 180;
  
  startAngleY = currentY;
  targetAngleY = targetAngle;
  moveDuration = duration;
  moveStartTime = millis();
  isMovingY = true;
  
  Serial.printf("Starting Y movement: %d -> %d degrees in %lu ms\n", 
                startAngleY, targetAngleY, duration);
}

// Update servo positions (call this regularly in loop)
bool updateServos() {
  bool anyMoving = false;
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - moveStartTime;
  
  if (isMovingX && elapsed < moveDuration) {
    float progress = (float)elapsed / (float)moveDuration;
    float easedProgress = easeCubicInOut(progress);
    int newAngle = startAngleX + (targetAngleX - startAngleX) * easedProgress;
    
    ServoX.write(newAngle);
    currentX = newAngle;
    anyMoving = true;
  } else if (isMovingX) {
    // Movement complete
    ServoX.write(targetAngleX);
    currentX = targetAngleX;
    isMovingX = false;
    Serial.printf("X movement complete at %d degrees\n", targetAngleX);
  }
  
  if (isMovingY && elapsed < moveDuration) {
    float progress = (float)elapsed / (float)moveDuration;
    float easedProgress = easeCubicInOut(progress);
    int newAngle = startAngleY + (targetAngleY - startAngleY) * easedProgress;
    
    ServoY.write(newAngle);
    currentY = newAngle;
    anyMoving = true;
  } else if (isMovingY) {
    // Movement complete
    ServoY.write(targetAngleY);
    currentY = targetAngleY;
    isMovingY = false;
    Serial.printf("Y movement complete at %d degrees\n", targetAngleY);
  }
  
  return anyMoving;
}

// うなずき動作（1秒で下に15度、1秒で戻る）
void performNod() {
  Serial.println("=== Starting Nod Movement ===");
  
  // Step 1: 下に15度（1秒）
  Serial.println("Nodding down 15 degrees");
  startEaseToY(NOD_DOWN_POSITION, 1000); // 1秒でイージング
  while (isMovingY) {
    updateServos();
    delay(REFRESH_INTERVAL_MILLIS);
  }
  
  // Step 2: 元の位置に戻る（1秒）
  Serial.println("Nodding back to center");
  startEaseToY(HOME_POSITION_Y, 1000); // 1秒でイージング
  while (isMovingY) {
    updateServos();
    delay(REFRESH_INTERVAL_MILLIS);
  }
  
  Serial.println("=== Nod Movement Complete ===");
}

// 首振り動作（0.5秒で右20度、1秒で左20度、0.5秒でセンター）
void performHeadShake() {
  Serial.println("=== Starting Head Shake Movement ===");
  
  // Step 1: 右に20度（0.5秒）
  Serial.println("Shaking right 20 degrees");
  startEaseToX(SHAKE_RIGHT_POSITION, 500); // 0.5秒でイージング
  while (isMovingX) {
    updateServos();
    delay(REFRESH_INTERVAL_MILLIS);
  }
  
  // Step 2: 左に20度（1秒）
  Serial.println("Shaking left 20 degrees");
  startEaseToX(SHAKE_LEFT_POSITION, 1000); // 1秒でイージング
  while (isMovingX) {
    updateServos();
    delay(REFRESH_INTERVAL_MILLIS);
  }
  
  // Step 3: センターに戻る（0.5秒）
  Serial.println("Shaking back to center");
  startEaseToX(HOME_POSITION_X, 500); // 0.5秒でイージング
  while (isMovingX) {
    updateServos();
    delay(REFRESH_INTERVAL_MILLIS);
  }
  
  Serial.println("=== Head Shake Movement Complete ===");
}

// 表情を名前で設定する関数
void setExpressionByName(const String& expressionName) {
  Serial.println("Setting expression: " + expressionName);
  
  if (expressionName == "Neutral" || expressionName == "中立") {
    avatar.setExpression(Expression::Neutral);
    textAnimator.setBeepFrequency(BEEP_FREQUENCIES[0]);
    if (isPoetMode) {
      avatar.setIsAutoBlink(true);
      avatar.setEyeOpenRatio(1.0);
      isPoetMode = false;
    }
  } else if (expressionName == "Happy" || expressionName == "嬉しい") {
    avatar.setExpression(Expression::Happy);
    textAnimator.setBeepFrequency(BEEP_FREQUENCIES[1]);
    if (isPoetMode) {
      avatar.setIsAutoBlink(true);
      avatar.setEyeOpenRatio(1.0);
      isPoetMode = false;
    }
  } else if (expressionName == "Angry" || expressionName == "怒り") {
    avatar.setExpression(Expression::Angry);
    textAnimator.setBeepFrequency(BEEP_FREQUENCIES[2]);
    if (isPoetMode) {
      avatar.setIsAutoBlink(true);
      avatar.setEyeOpenRatio(1.0);
      isPoetMode = false;
    }
  } else if (expressionName == "Sad" || expressionName == "悲しい") {
    avatar.setExpression(Expression::Sad);
    textAnimator.setBeepFrequency(BEEP_FREQUENCIES[3]);
    if (isPoetMode) {
      avatar.setIsAutoBlink(true);
      avatar.setEyeOpenRatio(1.0);
      isPoetMode = false;
    }
  } else if (expressionName == "Doubt" || expressionName == "疑問") {
    avatar.setExpression(Expression::Doubt);
    textAnimator.setBeepFrequency(BEEP_FREQUENCIES[4]);
    if (isPoetMode) {
      avatar.setIsAutoBlink(true);
      avatar.setEyeOpenRatio(1.0);
      isPoetMode = false;
    }
  } else if (expressionName == "Sleepy" || expressionName == "眠い") {
    avatar.setExpression(Expression::Sleepy);
    textAnimator.setBeepFrequency(BEEP_FREQUENCIES[5]);
    if (isPoetMode) {
      avatar.setIsAutoBlink(true);
      avatar.setEyeOpenRatio(1.0);
      isPoetMode = false;
    }
  } else if (expressionName == "Poet" || expressionName == "俳人") {
    avatar.setExpression(Expression::Neutral);
    avatar.setIsAutoBlink(false);
    avatar.setEyeOpenRatio(0.0);
    textAnimator.setBeepFrequency(BEEP_FREQUENCIES[6]);
    isPoetMode = true;
  } else {
    Serial.println("Unknown expression: " + expressionName);
    return;
  }
  
  Serial.println("Expression set successfully: " + expressionName);
}

// モーションを名前で実行する関数
void performMotionByName(const String& motionName) {
  Serial.println("Performing motion: " + motionName);
  
  if (!isMovingX && !isMovingY) {
    if (motionName == "nod" || motionName == "うなずき") {
      performNod();
    } else if (motionName == "shake" || motionName == "首振り") {
      performHeadShake();
    } else {
      Serial.println("Unknown motion: " + motionName);
      return;
    }
    Serial.println("Motion completed: " + motionName);
  } else {
    Serial.println("Servo is moving, motion skipped: " + motionName);
  }
}

// 単一JSONメッセージを処理する関数
void processSingleMessage(const String& message) {
  Serial.println("Processing single message: " + message);
  
  // JSON形式のメッセージを解析
  String displayText = "";
  String phoneticText = "";
  String expressionStr = "";
  String motionStr = "";
  
  // JSON形式かどうかをチェック
  if (message.startsWith("{") && message.endsWith("}")) {
    // デバッグコマンドをチェック
    if (message.indexOf("\"command\"") != -1) {
      // コマンド形式: {"command": "debug_on"} または {"command": "debug_off"}
      int cmdStart = message.indexOf("\"command\"");
      int cmdQuoteStart = message.indexOf('"', cmdStart + 9);
      int cmdQuoteEnd = message.indexOf('"', cmdQuoteStart + 1);
      
      if (cmdQuoteStart != -1 && cmdQuoteEnd != -1) {
        String command = message.substring(cmdQuoteStart + 1, cmdQuoteEnd);
        
        if (command == "debug_on") {
          debugMode = true;
          Serial.println("Debug mode: ON");
        } else if (command == "debug_off") {
          debugMode = false;
          Serial.println("Debug mode: OFF");
        } else {
          if (debugMode) {
            Serial.println("Unknown command: " + command);
          }
        }
      }
      return;
    }
    // 新しいJSON形式をチェック（message/expression/motion）
    else if (message.indexOf("\"message\"") != -1) {
      // 新形式: {"message": ["表示", "発音"], "expression": "Doubt", "motion": "nod"}
      
      // messageの配列を抽出
      int messageStart = message.indexOf("\"message\"");
      int arrayStart = message.indexOf("[", messageStart);
      int arrayEnd = message.indexOf("]", arrayStart);
      
      if (arrayStart != -1 && arrayEnd != -1) {
        String messageArray = message.substring(arrayStart + 1, arrayEnd);
        
        // 配列内の最初の文字列（表示用）
        int firstQuote = messageArray.indexOf('"');
        int firstEnd = messageArray.indexOf('"', firstQuote + 1);
        if (firstQuote != -1 && firstEnd != -1) {
          displayText = messageArray.substring(firstQuote + 1, firstEnd);
        }
        
        // 配列内の2番目の文字列（発音用）
        int secondQuote = messageArray.indexOf('"', firstEnd + 1);
        int secondEnd = messageArray.indexOf('"', secondQuote + 1);
        if (secondQuote != -1 && secondEnd != -1) {
          phoneticText = messageArray.substring(secondQuote + 1, secondEnd);
        }
      }
      
      // expressionを抽出
      int expStart = message.indexOf("\"expression\"");
      if (expStart != -1) {
        int expQuoteStart = message.indexOf('"', expStart + 12);
        int expQuoteEnd = message.indexOf('"', expQuoteStart + 1);
        if (expQuoteStart != -1 && expQuoteEnd != -1) {
          expressionStr = message.substring(expQuoteStart + 1, expQuoteEnd);
        }
      }
      
      // motionを抽出
      int motionStart = message.indexOf("\"motion\"");
      if (motionStart != -1) {
        int motionQuoteStart = message.indexOf('"', motionStart + 8);
        int motionQuoteEnd = message.indexOf('"', motionQuoteStart + 1);
        if (motionQuoteStart != -1 && motionQuoteEnd != -1) {
          motionStr = message.substring(motionQuoteStart + 1, motionQuoteEnd);
        }
      }
      
      if (debugMode) {
        Serial.println("New JSON format parsed:");
        Serial.println("  Display: " + displayText);
        Serial.println("  Phonetic: " + phoneticText);
        Serial.println("  Expression: " + expressionStr);
        Serial.println("  Motion: " + motionStr);
      }
      
    } else {
      // 旧形式: {"表示テキスト", "発音テキスト"}
      int firstQuote = message.indexOf('"');
      int firstComma = message.indexOf(',');
      int lastQuote = message.lastIndexOf('"');
      
      if (firstQuote != -1 && firstComma != -1 && lastQuote != -1) {
        // 最初の文字列（表示用）を抽出
        int firstEnd = message.indexOf('"', firstQuote + 1);
        if (firstEnd != -1) {
          displayText = message.substring(firstQuote + 1, firstEnd);
        }
        
        // 2番目の文字列（発音用）を抽出
        int secondStart = message.indexOf('"', firstComma);
        int secondEnd = message.indexOf('"', secondStart + 1);
        if (secondStart != -1 && secondEnd != -1) {
          phoneticText = message.substring(secondStart + 1, secondEnd);
        }
        
        Serial.println("Legacy JSON format parsed:");
        Serial.println("  Display: " + displayText);
        Serial.println("  Phonetic: " + phoneticText);
      } else {
        Serial.println("JSON parsing failed, using raw data");
        displayText = message;
        phoneticText = "";
      }
    }
  } else {
    // 通常のテキストの場合
    displayText = message;
    phoneticText = ""; // 空の場合はdisplayTextを使用
    Serial.println("Plain text mode: " + displayText);
  }
  
  // エスケープシーケンス（\n）を実際の改行文字に変換
  displayText.replace("\\n", "\n");
  displayText.replace("\\r", "\r");
  displayText.replace("\\t", "\t");
  
  if (phoneticText.length() > 0) {
    phoneticText.replace("\\n", "\n");
    phoneticText.replace("\\r", "\r");
    phoneticText.replace("\\t", "\t");
  }
  
  // 表情制御処理
  if (expressionStr.length() > 0) {
    Serial.println("Processing expression: " + expressionStr);
    setExpressionByName(expressionStr);
  }
  
  // モーション制御処理
  if (motionStr.length() > 0) {
    Serial.println("Processing motion: " + motionStr);
    performMotionByName(motionStr);
  }
  
  // TextAnimatorを使用してアニメーション表示（発音・折り返し・スクロール対応）
  // 表示用テキストのみを使用（JSON全体ではなく）
  if (displayText.length() > 0) {
    if (textAnimator.isAnimating()) {
      if (debugMode) {
        Serial.println("TextAnimator is busy, message ignored");
      }
    } else {
      Serial.println("Starting TextAnimator with display: " + displayText);
      Serial.printf("Display text length: %d chars\n", displayText.length());
      if (phoneticText.length() > 0) {
        Serial.println("Using phonetic: " + phoneticText);
        Serial.printf("Phonetic text length: %d chars\n", phoneticText.length());
        
        // セグメント数の事前チェック
        int displaySegments = 1;
        int phoneticSegments = 1;
        for (int i = 0; i < displayText.length(); i++) {
          if (displayText.charAt(i) == '\n') displaySegments++;
        }
        for (int i = 0; i < phoneticText.length(); i++) {
          if (phoneticText.charAt(i) == '\n') phoneticSegments++;
        }
        Serial.printf("Expected segments - Display: %d, Phonetic: %d\n", displaySegments, phoneticSegments);
        
        textAnimator.startAnimation(displayText, phoneticText);
      } else {
        Serial.println("Using display text for phonetic");
        textAnimator.startAnimation(displayText, displayText);
      }
    }
  }
}

void setup() {
  // M5Stack initialization
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(200);
  M5.Display.clear();
  
  Serial.begin(115200);
  Serial.println("Stackchan Test3 - Avatar with Servo Control");
  Serial.println("Starting initialization...");
  
  // Configure ESP32 PWM timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // Attach servos to pins
  Serial.println("Attaching servos...");
  ServoX.setPeriodHertz(50);
  ServoY.setPeriodHertz(50);
  
  if (ServoX.attach(SERVO_X_PIN, 500, 2400) == -1) {
    Serial.println("Error: Failed to attach servo X");
  } else {
    Serial.printf("X-axis servo attached: GPIO%d\n", SERVO_X_PIN);
  }
  
  if (ServoY.attach(SERVO_Y_PIN, 500, 2400) == -1) {
    Serial.println("Error: Failed to attach servo Y");
  } else {
    Serial.printf("Y-axis servo attached: GPIO%d\n", SERVO_Y_PIN);
  }
  
  // Move to home position
  ServoX.write(HOME_POSITION_X);
  ServoY.write(HOME_POSITION_Y);
  
  // Avatar initialization
  avatar.init();
  
  // オリジナルフェイスを保存
  originalFace = avatar.getFace();
  
  // 俳人フェイスを作成
  poetFace = new PoetFace();
  
  // PhoneticMouthを初期化
  phoneticMouth = new PhoneticMouth();
  
  // 日本語フォントを設定（文字化け対策）
  avatar.setSpeechFont(&fonts::lgfxJapanGothicP_16);
  
  // UART Port C初期化
  Serial.println("Initializing UART Port C...");
  UartPortC.end();
  delay(100);
  UartPortC.begin(UART_BAUD_RATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  Serial.printf("UART Port C initialized: RX=GPIO%d, TX=GPIO%d, Baud=%d\n", 
                UART_RX_PIN, UART_TX_PIN, UART_BAUD_RATE);
  
  // aiueo音素別口形状一覧を表示
  PhoneticMouth::printAllShapes();
  
  // セリフをデバッグ出力
  Serial.printf("Loaded %d lyrics:\n", LYRICS_COUNT);
  for (int i = 0; i < LYRICS_COUNT; i++) {
    Serial.printf("  [%d]: %s (%d chars)\n", i, LYRICS[i].display.c_str(), LYRICS[i].display.length());
  }
  
  Serial.println("Avatar initialized!");
  Serial.println("Servo control ready!");
  Serial.println("Speech balloon ready with text wrapping support.");
  Serial.println("PhoneticMouth system ready for detailed mouth shape control!");
  Serial.println("");
  Serial.println("Press buttons:");
  Serial.println("  Button A: Say speech with phonetic mouth control");
  Serial.println("  Button B: Alternating movement (Nod <-> Head Shake)");
  Serial.println("  Button C: Change expression (includes Poet mode)");
}


void loop() {
  M5.update();
  
  // サーボ位置を常に更新
  updateServos();
  
  // TextAnimatorの更新処理（常に呼び出す）
  textAnimator.update();
  
  // 俳人モード中は目を閉じたままに維持
  if (isPoetMode) {
    avatar.setEyeOpenRatio(0.0);   // 目を閉じる
  }
  
  // UART受信データをチェック
  if (UartPortC.available()) {
    uint8_t incomingByte = UartPortC.read();
    totalBytesReceived++;
    
    // デバッグ: 受信統計と内容を表示
    if (debugMode && (totalBytesReceived % 50 == 0 || incomingByte == '\n' || incomingByte == '\r')) {
      Serial.printf("UART RX: %lu bytes received, buffer: %d chars\n", 
                    totalBytesReceived, receivedData.length());
      if (receivedData.length() > 0) {
        Serial.printf("Buffer content: '%s'\n", receivedData.c_str());
      }
    }
    
    // UTF-8文字処理
    bool processComplete = false;
    
    if (incomingByte == '\n' || incomingByte == '\r') {
      // \r\n の重複処理を防ぐ（Windows形式改行対応）
      if (lastNewlineChar != 0 && 
          ((lastNewlineChar == '\r' && incomingByte == '\n') || 
           (lastNewlineChar == '\n' && incomingByte == '\r'))) {
        // 前の改行文字と組み合わせの場合はスキップ
        lastNewlineChar = 0;  // リセット
        // この1バイトの処理をスキップして次のバイトへ
      } else {
        lastNewlineChar = incomingByte;  // 現在の改行文字を記録
        // 改行文字を受信した場合、UTF-8シーケンスが完了していることを確認
        if (isUTF8Sequence) {
          // UTF-8シーケンスが未完了の場合、残りのバッファを追加
          if (utf8Buffer.length() > 0) {
            receivedData += utf8Buffer;
            Serial.printf("Incomplete UTF-8 sequence completed: %s\n", utf8Buffer.c_str());
          }
          // UTF-8シーケンスをリセット
          isUTF8Sequence = false;
          utf8BytesExpected = 0;
          utf8BytesReceived = 0;
          utf8Buffer = "";
        }
        
        // 完成した行を処理
        if (receivedData.length() > 0) {
          processComplete = true;
        }
      }
    } else if (incomingByte < 0x80) {
      // ASCII文字（1バイト）
      lastNewlineChar = 0;  // 改行文字以外を受信したらリセット
      receivedData += (char)incomingByte;
      
      // JSONメッセージの終了を検出
      if (incomingByte == '}' && receivedData.indexOf('{') != -1) {
        // JSONの開始と終了が揃った場合、処理を開始
        processComplete = true;
        Serial.println("JSON message complete, processing data");
      } else if (receivedData.length() >= MAX_BUFFER_SIZE) {  // バッファサイズ制限
        processComplete = true;
        Serial.println("Buffer size limit reached, processing data");
      }
    } else {
      // UTF-8マルチバイト文字の処理
      if (!isUTF8Sequence) {
        // UTF-8シーケンスの開始
        if ((incomingByte & 0xE0) == 0xC0) {
          utf8BytesExpected = 2;
        } else if ((incomingByte & 0xF0) == 0xE0) {
          utf8BytesExpected = 3;
        } else if ((incomingByte & 0xF8) == 0xF0) {
          utf8BytesExpected = 4;
        } else {
          utf8BytesExpected = 0;
        }
        
        if (utf8BytesExpected > 0) {
          isUTF8Sequence = true;
          utf8BytesReceived = 1;
          utf8Buffer = "";
          utf8Buffer += (char)incomingByte;
        }
      } else {
        // UTF-8シーケンスの継続
        if ((incomingByte & 0xC0) == 0x80) {
          utf8Buffer += (char)incomingByte;
          utf8BytesReceived++;
          
          if (utf8BytesReceived >= utf8BytesExpected) {
            // UTF-8文字完成
            receivedData += utf8Buffer;
            
            // リセット
            isUTF8Sequence = false;
            utf8BytesExpected = 0;
            utf8BytesReceived = 0;
            utf8Buffer = "";
            
            if (receivedData.length() >= MAX_BUFFER_SIZE) {
              processComplete = true;
              Serial.println("Buffer size limit reached during UTF-8 processing");
            }
          }
        } else {
          // 無効なUTF-8継続バイト - リセット
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
      
      // 複数のJSONメッセージが連結されている場合を処理
      String remainingData = receivedData;
      receivedData = "";  // 元のバッファをクリア
      
      while (remainingData.length() > 0) {
        // JSONメッセージの開始を探す
        int jsonStart = remainingData.indexOf('{');
        if (jsonStart == -1) {
          // JSONが見つからない場合は残りを破棄
          Serial.println("No JSON found in remaining data, discarding");
          break;
        }
        
        // JSONメッセージの終了を探す（ネストした{}に対応）
        int braceCount = 0;
        int jsonEnd = -1;
        for (int i = jsonStart; i < remainingData.length(); i++) {
          if (remainingData.charAt(i) == '{') {
            braceCount++;
          } else if (remainingData.charAt(i) == '}') {
            braceCount--;
            if (braceCount == 0) {
              jsonEnd = i;
              break;
            }
          }
        }
        
        if (jsonEnd == -1) {
          // 完全なJSONが見つからない場合は残りをバッファに戻す
          Serial.println("Incomplete JSON found, keeping in buffer");
          receivedData = remainingData;
          break;
        }
        
        // 完全なJSONメッセージを抽出
        String singleMessage = remainingData.substring(jsonStart, jsonEnd + 1);
        Serial.println("Processing single JSON: " + singleMessage);
        
        // メッセージ重複チェック
        unsigned long currentTime = millis();
        if (singleMessage == lastProcessedMessage && 
            (currentTime - lastProcessedTime) < MESSAGE_COOLDOWN) {
          if (debugMode) {
            Serial.printf("Duplicate message ignored (cooldown: %lu ms remaining)\n", 
                          MESSAGE_COOLDOWN - (currentTime - lastProcessedTime));
          }
        } else {
          // 新しいメッセージとして記録・処理
          lastProcessedMessage = singleMessage;
          lastProcessedTime = currentTime;
          
          // 単一メッセージを実際に処理
          processSingleMessage(singleMessage);
        }
        
        // 残りのデータを更新
        remainingData = remainingData.substring(jsonEnd + 1);
      }
      
      // JSON形式のメッセージを解析
      String displayText = "";
      String phoneticText = "";
      String expressionStr = "";
      String motionStr = "";
      
      // JSON形式かどうかをチェック
      if (receivedData.startsWith("{") && receivedData.endsWith("}")) {
        // デバッグコマンドをチェック
        if (receivedData.indexOf("\"command\"") != -1) {
          // コマンド形式: {"command": "debug_on"} または {"command": "debug_off"}
          int cmdStart = receivedData.indexOf("\"command\"");
          int cmdQuoteStart = receivedData.indexOf('"', cmdStart + 9);
          int cmdQuoteEnd = receivedData.indexOf('"', cmdQuoteStart + 1);
          
          if (cmdQuoteStart != -1 && cmdQuoteEnd != -1) {
            String command = receivedData.substring(cmdQuoteStart + 1, cmdQuoteEnd);
            
            if (command == "debug_on") {
              debugMode = true;
              Serial.println("Debug mode: ON");
            } else if (command == "debug_off") {
              debugMode = false;
              Serial.println("Debug mode: OFF");
            } else {
              if (debugMode) {
                Serial.println("Unknown command: " + command);
              }
            }
          }
          
          // コマンド処理後はリセットして終了
          receivedData = "";
          isUTF8Sequence = false;
          utf8BytesExpected = 0;
          utf8BytesReceived = 0;
          utf8Buffer = "";
          return;
        }
        // 新しいJSON形式をチェック（message/expression/motion）
        else if (receivedData.indexOf("\"message\"") != -1) {
          // 新形式: {"message": ["表示", "発音"], "expression": "Doubt", "motion": "nod"}
          
          // messageの配列を抽出
          int messageStart = receivedData.indexOf("\"message\"");
          int arrayStart = receivedData.indexOf("[", messageStart);
          int arrayEnd = receivedData.indexOf("]", arrayStart);
          
          if (arrayStart != -1 && arrayEnd != -1) {
            String messageArray = receivedData.substring(arrayStart + 1, arrayEnd);
            
            // 配列内の最初の文字列（表示用）
            int firstQuote = messageArray.indexOf('"');
            int firstEnd = messageArray.indexOf('"', firstQuote + 1);
            if (firstQuote != -1 && firstEnd != -1) {
              displayText = messageArray.substring(firstQuote + 1, firstEnd);
            }
            
            // 配列内の2番目の文字列（発音用）
            int secondQuote = messageArray.indexOf('"', firstEnd + 1);
            int secondEnd = messageArray.indexOf('"', secondQuote + 1);
            if (secondQuote != -1 && secondEnd != -1) {
              phoneticText = messageArray.substring(secondQuote + 1, secondEnd);
            }
          }
          
          // expressionを抽出
          int expStart = receivedData.indexOf("\"expression\"");
          if (expStart != -1) {
            int expQuoteStart = receivedData.indexOf('"', expStart + 12);
            int expQuoteEnd = receivedData.indexOf('"', expQuoteStart + 1);
            if (expQuoteStart != -1 && expQuoteEnd != -1) {
              expressionStr = receivedData.substring(expQuoteStart + 1, expQuoteEnd);
            }
          }
          
          // motionを抽出
          int motionStart = receivedData.indexOf("\"motion\"");
          if (motionStart != -1) {
            int motionQuoteStart = receivedData.indexOf('"', motionStart + 8);
            int motionQuoteEnd = receivedData.indexOf('"', motionQuoteStart + 1);
            if (motionQuoteStart != -1 && motionQuoteEnd != -1) {
              motionStr = receivedData.substring(motionQuoteStart + 1, motionQuoteEnd);
            }
          }
          
          if (debugMode) {
            Serial.println("New JSON format parsed:");
            Serial.println("  Display: " + displayText);
            Serial.println("  Phonetic: " + phoneticText);
            Serial.println("  Expression: " + expressionStr);
            Serial.println("  Motion: " + motionStr);
          }
          
        } else {
          // 旧形式: {"表示テキスト", "発音テキスト"}
          int firstQuote = receivedData.indexOf('"');
          int firstComma = receivedData.indexOf(',');
          int lastQuote = receivedData.lastIndexOf('"');
          
          if (firstQuote != -1 && firstComma != -1 && lastQuote != -1) {
            // 最初の文字列（表示用）を抽出
            int firstEnd = receivedData.indexOf('"', firstQuote + 1);
            if (firstEnd != -1) {
              displayText = receivedData.substring(firstQuote + 1, firstEnd);
            }
            
            // 2番目の文字列（発音用）を抽出
            int secondStart = receivedData.indexOf('"', firstComma);
            int secondEnd = receivedData.indexOf('"', secondStart + 1);
            if (secondStart != -1 && secondEnd != -1) {
              phoneticText = receivedData.substring(secondStart + 1, secondEnd);
            }
            
            Serial.println("Legacy JSON format parsed:");
            Serial.println("  Display: " + displayText);
            Serial.println("  Phonetic: " + phoneticText);
          } else {
            Serial.println("JSON parsing failed, using raw data");
            displayText = receivedData;
            phoneticText = "";
          }
        }
      } else {
        // 通常のテキストの場合
        displayText = receivedData;
        phoneticText = ""; // 空の場合はdisplayTextを使用
        Serial.println("Plain text mode: " + displayText);
      }
      
      // エスケープシーケンス（\n）を実際の改行文字に変換
      displayText.replace("\\n", "\n");
      displayText.replace("\\r", "\r");
      displayText.replace("\\t", "\t");
      
      if (phoneticText.length() > 0) {
        phoneticText.replace("\\n", "\n");
        phoneticText.replace("\\r", "\r");
        phoneticText.replace("\\t", "\t");
      }
      
      // 表情制御処理
      if (expressionStr.length() > 0) {
        Serial.println("Processing expression: " + expressionStr);
        setExpressionByName(expressionStr);
      }
      
      // モーション制御処理
      if (motionStr.length() > 0) {
        Serial.println("Processing motion: " + motionStr);
        performMotionByName(motionStr);
      }
      
      // TextAnimatorを使用してアニメーション表示（発音・折り返し・スクロール対応）
      // 表示用テキストのみを使用（JSON全体ではなく）
      if (displayText.length() > 0) {
        if (textAnimator.isAnimating()) {
          if (debugMode) {
            Serial.println("TextAnimator is busy, message ignored");
          }
        } else {
        Serial.println("Starting TextAnimator with display: " + displayText);
        Serial.printf("Display text length: %d chars\n", displayText.length());
        if (phoneticText.length() > 0) {
          Serial.println("Using phonetic: " + phoneticText);
          Serial.printf("Phonetic text length: %d chars\n", phoneticText.length());
          
          // セグメント数の事前チェック
          int displaySegments = 1;
          int phoneticSegments = 1;
          for (int i = 0; i < displayText.length(); i++) {
            if (displayText.charAt(i) == '\n') displaySegments++;
          }
          for (int i = 0; i < phoneticText.length(); i++) {
            if (phoneticText.charAt(i) == '\n') phoneticSegments++;
          }
          Serial.printf("Expected segments - Display: %d, Phonetic: %d\n", displaySegments, phoneticSegments);
          
          textAnimator.startAnimation(displayText, phoneticText);
        } else {
          Serial.println("Using display text for phonetic");
          textAnimator.startAnimation(displayText, displayText);
        }
        }
      }
      
      receivedData = "";
      
      // UTF-8シーケンスもリセット
      isUTF8Sequence = false;
      utf8BytesExpected = 0;
      utf8BytesReceived = 0;
      utf8Buffer = "";
    }
  }
  
  // Aボタンが押されたらセリフをアニメーション表示
  if (M5.BtnA.wasPressed()) {
    if (LYRICS_COUNT > 0 && !textAnimator.isAnimating()) {
      SpeechData currentSpeech = LYRICS[currentLyricsIndex];
      Serial.printf("Button A pressed - Animating speech: %s\n", currentSpeech.display.c_str());
      
      // セリフをバッファに保存（リピート用：表示・発音両方）
      lastPlayedSpeech = currentSpeech.display;
      lastPlayedPhonetic = currentSpeech.phonetic;
      
      // TextAnimatorを使用（表示文字列と発音文字列を指定）
      textAnimator.startAnimation(currentSpeech.display, currentSpeech.phonetic);
      
      // 次のセリフに進む（循環）
      currentLyricsIndex = (currentLyricsIndex + 1) % LYRICS_COUNT;
    }
  }
  
  // Bボタンが押されたらうなずき・首振りを交互に実行
  if (M5.BtnB.wasPressed()) {
    if (!isMovingX && !isMovingY) {
      if (isNodTurn) {
        // うなずき動作
        Serial.println("Button B pressed - Performing Nod");
        avatar.setSpeechText("うなずき");
        performNod();
        isNodTurn = false;  // 次は首振り
      } else {
        // 首振り動作
        Serial.println("Button B pressed - Performing Head Shake");
        avatar.setSpeechText("首振り");
        performHeadShake();
        isNodTurn = true;   // 次はうなずき
      }
      
      // 動作完了後、吹き出しを消去
      delay(1000);
      avatar.setSpeechText("");
    } else {
      Serial.println("Servo is moving, please wait");
    }
  }
  
  // Cボタンが押されたら表情を順次変更
  if (M5.BtnC.wasPressed()) {
    Serial.printf("Button C pressed - Change expression to: %s\n", EXPRESSION_NAMES[expression_index]);
    
    if (expression_index == 6) {  // 俳人表情（7番目）
      // 中立表情にして目を閉じる（俳人らしい表現）
      avatar.setExpression(Expression::Neutral);
      avatar.setIsAutoBlink(false);  // 自動まばたきを無効化
      avatar.setEyeOpenRatio(0.0);   // 目を閉じる
      isPoetMode = true;
      Serial.println("Switched to Poet Expression (中立＋目を閉じる)");
    } else {
      // 通常の表情に切り替え
      if (isPoetMode) {
        // 目の開き具合を通常に戻す
        avatar.setIsAutoBlink(true);   // 自動まばたきを再有効化
        avatar.setEyeOpenRatio(1.0);
        isPoetMode = false;
        Serial.println("Switched back to Normal Eye Opening");
      }
      avatar.setExpression(expressions[expression_index]);
    }
    
    avatar.setSpeechText(EXPRESSION_NAMES[expression_index]);
    avatar.setMouthOpenRatio(0.0); // 口を閉じる
    
    // 表情に応じたビープ音周波数を設定
    textAnimator.setBeepFrequency(BEEP_FREQUENCIES[expression_index]);
    Serial.printf("Expression: %s, Beep frequency: %d Hz\n", EXPRESSION_NAMES[expression_index], BEEP_FREQUENCIES[expression_index]);
    
    expression_index = (expression_index + 1) % EXPRESSION_COUNT;
    textAnimator.stop(); // テキストアニメーション停止
  }
  
  delay(50);
}
