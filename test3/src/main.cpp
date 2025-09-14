#include <Arduino.h>
#include <M5Unified.h>
#include <Avatar.h>
#include <ESP32Servo.h>
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
