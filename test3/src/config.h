#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// セリフ設定（表示文字列と発音文字列のペア）
struct SpeechData {
  String display;      // 表示文字列
  String phonetic;     // 発音文字列（空の場合は表示文字列を使用）
};

const SpeechData LYRICS[] = {
  {"こんにちは", ""},
  {"元気ですか？", "げんきですか？"},
  {"私はスタックチャンです", "わたしはすたっくちゃんです"},
  {"よろしくお願いします", "よろしくおねがいします"},
  {"ありがとうございます", ""},
  {"今日はとても良いお天気ですね", "きょうはとてもよいおてんきですね"},
  {"テキストスクロール機能をテストしています", "てきすとすくろーるきのうをてすとしています"},
  {"長いセリフも問題なく表示できるはずです", "ながいせりふももんだいなくひょうじできるはずです"},
  {"９文字以上の文字はスクロールします", "きゅうもじいじょうのもじはすくろーるします"},
  {"改行コードを使うと\n分割表示もできます", "かいぎょうこーどをつかうと\nぶんかつひょうじもできます"},
  {"古池や\n蛙飛びこむ\n水の音", "ふるいけや\nかわずとびこむ\nみずのおと"}
};

const int LYRICS_COUNT = sizeof(LYRICS) / sizeof(LYRICS[0]);

// 表情設定（俳人追加）
extern const char* EXPRESSION_NAMES[];
extern const int EXPRESSION_COUNT;

// TextAnimator設定
extern const int MAX_CHARS_PER_LINE;  // 1行あたりの最大文字数
extern const int MAX_LINES;           // 最大行数

// フォント設定
extern const bool USE_JAPANESE_FONT;

// 表情別ビープ音周波数設定
extern const int BEEP_FREQUENCIES[];

// ========== ビープ音量設定 ==========

// ビープ音の音量（0~100）
const int BEEP_VOLUME = 5;  // 50% = 標準音量

// ========================================

// ========== しゃべる速さ調節設定 ==========

// 文字表示間隔（小さいほど速くしゃべる）
const unsigned long CHAR_DISPLAY_INTERVAL = 100;  // 100ms = 0.1秒間隔（標準）

// ビープ音の長さ
const unsigned long BEEP_SOUND_DURATION = 50;     // 50ms = 0.05秒

// 口を閉じるタイミング
const unsigned long MOUTH_CLOSE_DELAY = 100;      // 100ms = 0.1秒後に口を閉じる

// セリフ自動消去タイミング
const unsigned long TEXT_CLEAR_DELAY = 2000;      // 2000ms = 2秒後に消去

// セグメント間（改行間）の一時停止時間
const unsigned long SEGMENT_PAUSE_DURATION = 500;  // 500ms = 0.5秒間停止

// ========== 速度プリセット例 ==========
// 高速： CHAR_DISPLAY_INTERVAL = 50   (0.05秒間隔)
// 標準： CHAR_DISPLAY_INTERVAL = 100  (0.1秒間隔) 
// ゆっくり： CHAR_DISPLAY_INTERVAL = 200  (0.2秒間隔)
// 非常にゆっくり： CHAR_DISPLAY_INTERVAL = 300  (0.3秒間隔)

// ========================================

// ========== サーボピン設定 ==========

// サーボ制御ピン（M5Stack Core2）
const int SERVO_X_PIN = 33;  // X軸サーボピン（GPIO33 = Port A）
const int SERVO_Y_PIN = 32;  // Y軸サーボピン（GPIO32 = Port A）

// ピン設定例：
// Port A: GPIO32, GPIO33 (推奨)
// Port B: GPIO26, GPIO36
// Port C: GPIO13, GPIO14

// ========================================

// ========== サーボ初期位置設定 ==========

// サーボ初期位置（度数）
const int SERVO_HOME_X = 82;   // X軸初期位置（右に10度）90 - 10 = 80
const int SERVO_HOME_Y = 75;   // Y軸初期位置（上に15度）90 - 15 = 75

// サーボ動作範囲
const int SERVO_RIGHT_POSITION = 0;    // 右90度位置（左右反転）
const int SERVO_UP_POSITION = 0;       // 上90度位置

// うなずき・首振り動作設定
const int SERVO_NOD_DOWN = 90;         // うなずき下向き位置（15度下）75 + 15 = 90
const int SERVO_SHAKE_RIGHT = 60;      // 首振り右位置（20度右）80 - 20 = 60
const int SERVO_SHAKE_LEFT = 100;      // 首振り左位置（20度左）80 + 20 = 100

// ========================================

#endif
