#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// セリフ設定（改行機能テスト用）
const String LYRICS[] = {
  "こんにちは",
  "元気ですか？",
  "私はスタックチャンです",
  "よろしくお願いします",
  "ありがとうございます",
  "今日はとても良いお天気ですね",
  "テキストスクロール機能をテストしています",
  "長いセリフも問題なく表示できるはずです",
  "９文字以上の文字はスクロールします",
  "改行コードを使うと\n分割表示もできます",
  "古池や\n蛙飛びこむ\n水の音"
};

const int LYRICS_COUNT = sizeof(LYRICS) / sizeof(LYRICS[0]);

// 表情設定
const char* EXPRESSION_NAMES[] = {"中立", "嬉しい", "怒り", "悲しい", "疑問", "眠い"};
const int EXPRESSION_COUNT = 6;

// TextAnimator設定
const int MAX_CHARS_PER_LINE = 10;  // 1行あたりの最大文字数
const int MAX_LINES = 3;            // 最大行数

// フォント設定
const bool USE_JAPANESE_FONT = true;

// 表情別ビープ音周波数設定
const int BEEP_FREQUENCIES[] = {
  1000,  // 中立 (Neutral) - 標準
  1400,  // 嬉しい (Happy) - 高め (ポジティブ)
  600,   // 怒り (Angry) - 低め (ネガティブ)
  500,   // 悲しい (Sad) - とても低め (ネガティブ)
  1100,  // 疑問 (Doubt) - やや高め
  800    // 眠い (Sleepy) - やや低め
};

#endif
