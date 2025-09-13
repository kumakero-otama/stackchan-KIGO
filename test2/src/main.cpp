#include <Arduino.h>
#include <M5Unified.h>
#include <Avatar.h>
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

void setup() {
  // M5Stack initialization
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(30);
  M5.Display.clear();
  
  Serial.begin(115200);
  Serial.println("Stackchan Test1 - Speech Balloon Test with Text Wrapping");
  Serial.println("Starting initialization...");
  
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
  Serial.println("Speech balloon ready with text wrapping support.");
  Serial.println("PhoneticMouth system ready for detailed mouth shape control!");
  Serial.println("");
  Serial.println("=== 口形状制御機能 ===");
  Serial.println("PhoneticMouth::setPhoneme(\"あ\") - aiueo音素別口形状");
  Serial.println("PhoneticMouth::setCustomShape(minW, maxW, minH, maxH) - カスタム幅・高さ指定");
  Serial.println("PhoneticMouth::applyToAvatar(&avatar) - Avatarに適用");
  Serial.println("例: phoneticMouth->setCustomShape(10, 50, 5, 25); // 小さく細い口");
  Serial.println("例: phoneticMouth->setCustomShape(30, 100, 15, 45); // 大きく開いた口");
  Serial.println("===================");
  Serial.println("");
  Serial.println("Press buttons:");
  Serial.println("  Button A: Say speech with phonetic mouth control");
  Serial.println("  Button B: Repeat last speech");
  Serial.println("  Button C: Change expression (includes Poet mode)");
}

void loop() {
  M5.update();
  
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
  
  // Bボタンが押されたら前のセリフをリピート再生（発音文字列含む）
  if (M5.BtnB.wasPressed()) {
    if (lastPlayedSpeech.length() > 0 && !textAnimator.isAnimating()) {
      Serial.printf("Button B pressed - Repeat speech: %s (phonetic: %s)\n", 
                    lastPlayedSpeech.c_str(), lastPlayedPhonetic.c_str());
      textAnimator.startAnimation(lastPlayedSpeech, lastPlayedPhonetic);
    } else if (lastPlayedSpeech.length() == 0) {
      Serial.println("Button B pressed - No speech to repeat");
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
  
  delay(100);
}
