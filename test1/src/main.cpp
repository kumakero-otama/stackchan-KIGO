#include <Arduino.h>
#include <M5Unified.h>
#include <Avatar.h>
#include "TextAnimator.h"
#include "config.h"

using namespace m5avatar;

Avatar avatar;
TextAnimator textAnimator(&avatar);

int currentLyricsIndex = 0;
String lastPlayedSpeech = "";  // セリフリピート用バッファ

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
  
  // 日本語フォントを設定（文字化け対策）
  avatar.setSpeechFont(&fonts::lgfxJapanGothicP_16);
  
  // セリフをデバッグ出力
  Serial.printf("Loaded %d lyrics:\n", LYRICS_COUNT);
  for (int i = 0; i < LYRICS_COUNT; i++) {
    Serial.printf("  [%d]: %s (%d chars)\n", i, LYRICS[i].c_str(), LYRICS[i].length());
  }
  
  Serial.println("Avatar initialized!");
  Serial.println("Speech balloon ready with text wrapping support.");
  Serial.println("Press buttons:");
  Serial.println("  Button A: Say speech with text wrapping animation");
  Serial.println("  Button B: Clear text and stop animation");
  Serial.println("  Button C: Change expression");
}

void loop() {
  M5.update();
  
  // TextAnimatorの更新処理（常に呼び出す）
  textAnimator.update();
  
  // Aボタンが押されたらセリフをアニメーション表示
  if (M5.BtnA.wasPressed()) {
    if (LYRICS_COUNT > 0 && !textAnimator.isAnimating()) {
      String currentLyrics = LYRICS[currentLyricsIndex];
      Serial.printf("Button A pressed - Animating speech: %s\n", currentLyrics.c_str());
      
      // セリフをバッファに保存（リピート用）
      lastPlayedSpeech = currentLyrics;
      
      // TextAnimatorを使用して一文字ずつアニメーション表示（スクロール機能付き）
      textAnimator.startAnimation(currentLyrics);
      
      // 次のセリフに進む（循環）
      currentLyricsIndex = (currentLyricsIndex + 1) % LYRICS_COUNT;
    }
  }
  
  // Bボタンが押されたら前のセリフをリピート再生
  if (M5.BtnB.wasPressed()) {
    if (lastPlayedSpeech.length() > 0 && !textAnimator.isAnimating()) {
      Serial.printf("Button B pressed - Repeat speech: %s\n", lastPlayedSpeech.c_str());
      textAnimator.startAnimation(lastPlayedSpeech);
    } else if (lastPlayedSpeech.length() == 0) {
      Serial.println("Button B pressed - No speech to repeat");
    }
  }
  
  // Cボタンが押されたら表情を順次変更
  if (M5.BtnC.wasPressed()) {
    Serial.printf("Button C pressed - Change expression to: %s\n", EXPRESSION_NAMES[expression_index]);
    avatar.setExpression(expressions[expression_index]);
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
