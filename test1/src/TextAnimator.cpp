#include "TextAnimator.h"

TextAnimator::TextAnimator(Avatar* avatarInstance) : avatar(avatarInstance) {
}

void TextAnimator::startAnimation(const String& text) {
    // テキストを改行で分割
    segmentText(text);
    
    // 初期化
    display_text = "";
    is_animating = true;
    auto_clear_enabled = false;
    mouth_close_enabled = false;
    current_char_index = 0;
    display_start_index = 0;
    current_segment_index = 0;
    segment_pause_enabled = false;
    last_update_time = millis();
    
    if (segment_count > 0 && segments[0].length() > 0) {
        // 最初のセグメントの最初の文字を表示
        animation_text = segments[0];
        String firstChar = getCharAtIndex(animation_text, 0);
        display_text = firstChar;
        avatar->setSpeechText(display_text.c_str());
        avatar->setMouthOpenRatio(getMouthRatioForChar(firstChar));
        M5.Speaker.tone(beep_frequency, beep_duration);
    }
}

void TextAnimator::update() {
    // セグメント間一時停止処理
    if (segment_pause_enabled) {
        if (millis() - segment_pause_start_time >= segment_pause_duration) {
            // 一時停止終了、次のセグメントに進む
            current_segment_index++;
            if (current_segment_index < segment_count) {
                // 次のセグメントを開始
                animation_text = segments[current_segment_index];
                display_text = "";
                display_start_index = 0;
                current_char_index = 0;
                segment_pause_enabled = false;
                last_update_time = millis();
                
                // 最初の文字を表示
                if (animation_text.length() > 0) {
                    String firstChar = getCharAtIndex(animation_text, 0);
                    display_text = firstChar;
                    avatar->setSpeechText(display_text.c_str());
                    avatar->setMouthOpenRatio(getMouthRatioForChar(firstChar));
                    M5.Speaker.tone(beep_frequency, beep_duration);
                }
                
                Serial.printf("Starting segment %d: %s\n", current_segment_index, animation_text.c_str());
            } else {
                // 全セグメント完了
                is_animating = false;
                segment_pause_enabled = false;
                auto_clear_enabled = true;
                clear_start_time = millis();
                mouth_close_enabled = true;
                mouth_close_start_time = millis();
            }
        }
        return;
    }
    
    // テキストアニメーション処理
    if (is_animating && (millis() - last_update_time >= char_interval)) {
        current_char_index++;
        int total_chars = getCharCount(animation_text);
        
        if (current_char_index >= total_chars) {
            // 現在セグメント完了
            if (current_segment_index < segment_count - 1) {
                // 次のセグメントがある場合、一時停止開始
                segment_pause_enabled = true;
                segment_pause_start_time = millis();
                Serial.printf("Segment %d completed, pausing for 0.5s\n", current_segment_index);
            } else {
                // 全セグメント完了
                is_animating = false;
                mouth_close_enabled = true;
                mouth_close_start_time = millis();
                auto_clear_enabled = true;
                clear_start_time = millis();
                Serial.println("All segments completed");
            }
        } else {
            // 次の文字を取得
            String nextChar = getCharAtIndex(animation_text, current_char_index);
            
            // スクロール機能
            int current_display_chars = getCharCount(display_text);
            if (current_display_chars >= MAX_DISPLAY_CHARS) {
                // スクロール処理
                display_start_index++;
                display_text = getScrolledText(animation_text, display_start_index, current_char_index);
            } else {
                // 通常の追加表示
                display_text += nextChar;
            }
            
            avatar->setSpeechText(display_text.c_str());
            avatar->setMouthOpenRatio(getMouthRatioForChar(nextChar));
            last_update_time = millis();
            M5.Speaker.tone(beep_frequency, beep_duration);
        }
    }
    
    // 0.1秒後に口を閉じる処理
    if (mouth_close_enabled && (millis() - mouth_close_start_time >= mouth_close_delay)) {
        avatar->setMouthOpenRatio(0.0);
        mouth_close_enabled = false;
    }
    
    // 2秒後の自動クリア処理
    if (auto_clear_enabled && (millis() - clear_start_time >= clear_delay)) {
        avatar->setSpeechText("");
        auto_clear_enabled = false;
    }
}

void TextAnimator::stop() {
    is_animating = false;
    auto_clear_enabled = false;
    mouth_close_enabled = false;
}

void TextAnimator::clear() {
    avatar->setSpeechText("");
    avatar->setMouthOpenRatio(0.0);
    stop();
}

bool TextAnimator::isAnimating() const {
    return is_animating;
}

// UTF-8文字処理関数の実装
String TextAnimator::getCharAtIndex(const String& text, int index) {
    int currentIndex = 0;
    int byteIndex = 0;
    
    while (byteIndex < text.length() && currentIndex <= index) {
        if (currentIndex == index) {
            int charLength = 1;
            uint8_t firstByte = text[byteIndex];
            
            if (firstByte >= 0xC0) {
                if (firstByte < 0xE0) charLength = 2;
                else if (firstByte < 0xF0) charLength = 3;
                else charLength = 4;
            }
            
            return text.substring(byteIndex, byteIndex + charLength);
        }
        
        // 次の文字へ進む
        uint8_t firstByte = text[byteIndex];
        if (firstByte >= 0xC0) {
            if (firstByte < 0xE0) byteIndex += 2;
            else if (firstByte < 0xF0) byteIndex += 3;
            else byteIndex += 4;
        } else {
            byteIndex += 1;
        }
        currentIndex++;
    }
    
    return "";
}

int TextAnimator::getCharCount(const String& text) {
    int count = 0;
    int byteIndex = 0;
    
    while (byteIndex < text.length()) {
        uint8_t firstByte = text[byteIndex];
        if (firstByte >= 0xC0) {
            if (firstByte < 0xE0) byteIndex += 2;
            else if (firstByte < 0xF0) byteIndex += 3;
            else byteIndex += 4;
        } else {
            byteIndex += 1;
        }
        count++;
    }
    
    return count;
}

float TextAnimator::getMouthRatioForChar(const String& character) {
    // 日本語文字に基づく簡単な口の形推定
    if (character == "あ" || character == "は" || character == "か" || character == "が" || character == "な" || character == "ま") return 1.0;
    if (character == "い" || character == "き" || character == "ぎ" || character == "に" || character == "み") return 0.3;
    if (character == "う" || character == "く" || character == "ぐ" || character == "す" || character == "ず" || character == "む") return 0.8;
    if (character == "え" || character == "け" || character == "げ" || character == "せ" || character == "ぜ" || character == "ね" || character == "め") return 0.5;
    if (character == "お" || character == "こ" || character == "ご" || character == "そ" || character == "ぞ" || character == "の" || character == "も") return 0.8;
    if (character == "ん") return 0.1;
    if (character == "っ" || character == "ッ") return 0.2;
    // デフォルトは中程度の開き
    return 0.5;
}

// スクロール用のテキスト取得関数
String TextAnimator::getScrolledText(const String& text, int start_index, int end_index) {
    String result = "";
    int char_count = 0;
    
    for (int i = start_index; i <= end_index && char_count < MAX_DISPLAY_CHARS; i++) {
        String character = getCharAtIndex(text, i);
        if (character.length() > 0) {
            result += character;
            char_count++;
        }
    }
    
    return result;
}

// 表情に応じたビープ音周波数設定
void TextAnimator::setBeepFrequency(int frequency) {
    beep_frequency = frequency;
    Serial.printf("Beep frequency set to: %d Hz\n", frequency);
}

// テキスト分割関数
void TextAnimator::segmentText(const String& text) {
    segment_count = 0;
    
    // 初期化
    for (int i = 0; i < 10; i++) {
        segments[i] = "";
    }
    
    if (text.length() == 0) {
        return;
    }
    
    int start = 0;
    int pos = 0;
    
    // 改行コードで分割
    while (pos < text.length() && segment_count < 10) {
        if (text[pos] == '\n') {
            // セグメントを抽出
            String segment = text.substring(start, pos);
            if (segment.length() > 0) {
                segments[segment_count] = segment;
                segment_count++;
                Serial.printf("Segment %d: %s\n", segment_count - 1, segment.c_str());
            }
            start = pos + 1; // 改行コード後から開始
        }
        pos++;
    }
    
    // 最後のセグメント（改行コードがない場合も含む）
    if (start < text.length() && segment_count < 10) {
        String segment = text.substring(start);
        if (segment.length() > 0) {
            segments[segment_count] = segment;
            segment_count++;
            Serial.printf("Segment %d: %s\n", segment_count - 1, segment.c_str());
        }
    }
    
    Serial.printf("Total segments: %d\n", segment_count);
}
