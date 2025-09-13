#ifndef TEXTANIMATOR_H
#define TEXTANIMATOR_H

#include <Arduino.h>
#include <Avatar.h>
#include <M5Unified.h>

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
    
    // 改行機能用変数
    String segments[10];                          // 改行で分割されたセグメント（最大10セグメント）
    int segment_count = 0;                        // セグメント数
    int current_segment_index = 0;                // 現在処理中のセグメント番号
    bool segment_pause_enabled = false;           // セグメント間一時停止状態
    unsigned long segment_pause_start_time = 0;   // セグメント間一時停止開始時刻
    const unsigned long segment_pause_duration = 500;  // セグメント間一時停止時間（0.5秒）
    
    // タイミング設定
    const unsigned long char_interval = 100;      // 0.1秒間隔
    const unsigned long clear_delay = 2000;       // 2秒後に消去
    const unsigned long mouth_close_delay = 100;  // 0.1秒後に口を閉じる
    const unsigned long beep_duration = 50;       // 0.05秒ビープ音
    int beep_frequency = 1000;                    // ビープ音周波数（表情により変更）
    
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
    void segmentText(const String& text);         // テキストを改行で分割
    
public:
    TextAnimator(Avatar* avatarInstance);
    
    // パブリックメソッド
    void startAnimation(const String& text);
    void update();
    void stop();
    void clear();
    bool isAnimating() const;
    void setBeepFrequency(int frequency);  // 表情に応じたビープ音周波数設定
};

#endif
