#ifndef TEXTANIMATOR_H
#define TEXTANIMATOR_H

#include <Arduino.h>
#include <Avatar.h>
#include <M5Unified.h>
#include "config.h"

using namespace m5avatar;

class TextAnimator {
private:
    Avatar* avatar;
    
    // アニメーション制御用変数
    bool is_animating = false;
    bool auto_clear_enabled = false;
    bool mouth_close_enabled = false;
    int current_char_index = 0;
    unsigned long last_update_time = 0;
    unsigned long clear_start_time = 0;
    unsigned long mouth_close_start_time = 0;
    
    // 改行機能用変数（表示と発音を分離）
    String display_segments[10];                 // 表示用セグメント（最大10セグメント）
    String phonetic_segments[10];                // 発音用セグメント（最大10セグメント）
    int segment_count = 0;                        // セグメント数
    int current_segment_index = 0;                // 現在処理中のセグメント番号
    bool segment_pause_enabled = false;           // セグメント間一時停止状態
    unsigned long segment_pause_start_time = 0;   // セグメント間一時停止開始時刻
    const unsigned long segment_pause_duration = SEGMENT_PAUSE_DURATION;  // セグメント間一時停止時間（config.hから）
    
    // タイミング設定（config.hから取得）
    const unsigned long char_interval = CHAR_DISPLAY_INTERVAL;    // 文字表示間隔
    const unsigned long clear_delay = TEXT_CLEAR_DELAY;           // セリフ自動消去
    const unsigned long mouth_close_delay = MOUTH_CLOSE_DELAY;    // 口を閉じるタイミング
    const unsigned long beep_duration = BEEP_SOUND_DURATION;      // ビープ音の長さ
    int beep_frequency = 1000;                    // ビープ音周波数（表情により変更）
    const int beep_volume = BEEP_VOLUME;          // ビープ音量（0~100、config.hから）
    
    // スクロール設定
    static const int MAX_DISPLAY_CHARS = 9;       // 最大表示文字数
    
    // アニメーション用文字列
    String animation_text = "";
    String display_text = "";
    int display_start_index = 0;                  // 表示開始インデックス
    
    // UTF-8文字処理関数
    String getCharAtIndex(const String& text, int index);
    int getCharCount(const String& text);
    float getMouthRatioForChar(const String& character);
    String getScrolledText(const String& text, int start_index, int max_chars);
    
    // 改行機能用関数
    void segmentText(const String& text);         // テキストを改行で分割（旧版）
    void segmentTexts(const String& displayText, const String& phoneticText); // 表示・発音分離版
    
public:
    TextAnimator(Avatar* avatarInstance);
    
    // パブリックメソッド
  void startAnimation(const String& displayText, const String& phoneticText = "");
    void update();
    void stop();
    void clear();
    bool isAnimating() const;
    void setBeepFrequency(int frequency);  // 表情に応じたビープ音周波数設定
};

#endif
