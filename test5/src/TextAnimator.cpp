#include "TextAnimator.h"

TextAnimator::TextAnimator(Avatar* avatarInstance) : avatar(avatarInstance), debugModePtr(nullptr) {
}

void TextAnimator::startAnimation(const String& displayText, const String& phoneticText) {
    // 発音文字列が指定されている場合はそれを使用、なければ表示文字列を使用
    String phoneticForAnimation = (phoneticText.length() > 0) ? phoneticText : displayText;
    
    // 表示用と発音用のテキストを別々に分割
    segmentTexts(displayText, phoneticForAnimation);
    
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
    
    if (segment_count > 0 && display_segments[0].length() > 0) {
        // 最初のセグメントの最初の文字を表示
        String firstDisplayChar = getCharAtIndex(display_segments[0], 0);
        String firstPhoneticChar = getCharAtIndex(phonetic_segments[0], 0);
        display_text = firstDisplayChar;
        avatar->setSpeechText(display_text.c_str());
        avatar->setMouthOpenRatio(getMouthRatioForChar(firstPhoneticChar)); // 発音文字で口形制御
        M5.Speaker.setVolume(beep_volume);
        M5.Speaker.tone(beep_frequency, beep_duration);
        
        // 現在のアニメーション用テキストを設定（発音制御用）
        animation_text = phonetic_segments[0];
        
        // 最初のセグメント開始ログを出力
        Serial.printf("Starting segment 0: %s\n", animation_text.c_str());
    }
}

void TextAnimator::update() {
    // セグメント間一時停止処理
    if (segment_pause_enabled) {
        if (millis() - segment_pause_start_time >= segment_pause_duration) {
            // 一時停止終了、次のセグメントに進む
            current_segment_index++;
            if (current_segment_index < segment_count) {
                // 次のセグメントを開始（発音制御用）
                animation_text = phonetic_segments[current_segment_index];
                display_text = "";
                display_start_index = 0;
                current_char_index = 0;
                segment_pause_enabled = false;
                last_update_time = millis();
                
                // 最初の文字を表示（表示用）と発音制御（発音用）
                if (animation_text.length() > 0) {
                    String firstDisplayChar = getCharAtIndex(display_segments[current_segment_index], 0);
                    String firstPhoneticChar = getCharAtIndex(phonetic_segments[current_segment_index], 0);
                    display_text = firstDisplayChar;
                    avatar->setSpeechText(display_text.c_str());
                    avatar->setMouthOpenRatio(getMouthRatioForChar(firstPhoneticChar)); // 発音制御
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
        int phonetic_chars_total = getCharCount(animation_text);
        int display_chars_total = getCharCount(display_segments[current_segment_index]);
        
        // より長い方を基準にアニメーション終了を判定
        int max_chars = max(phonetic_chars_total, display_chars_total);
        
        if (current_char_index >= max_chars) {
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
            // 発音文字で制御（ビープ音・口形用）
            String nextPhoneticChar = getCharAtIndex(phonetic_segments[current_segment_index], current_char_index);
            
            // 表示文字の進行を計算（より長い方を基準にして、短い方も完全に表示）
            int display_chars_total = getCharCount(display_segments[current_segment_index]);
            int phonetic_chars_total = getCharCount(phonetic_segments[current_segment_index]);
            
            int display_char_index;
            if (phonetic_chars_total > 0) {
                // 発音文字数が0でない場合の計算
                if (display_chars_total >= phonetic_chars_total) {
                    // 表示文字数の方が多い場合：発音の進行に合わせて表示を進める
                    display_char_index = (current_char_index * display_chars_total) / phonetic_chars_total;
                } else {
                    // 発音文字数の方が多い場合：表示文字を早めに完了させる
                    display_char_index = min(current_char_index, display_chars_total - 1);
                }
            } else {
                // 発音文字数が0の場合：表示文字のインデックスをそのまま使用
                display_char_index = min(current_char_index, display_chars_total - 1);
            }
            
            // 表示文字列を更新
            display_text = "";
            int display_start = (display_char_index >= MAX_DISPLAY_CHARS) ? (display_char_index - MAX_DISPLAY_CHARS + 1) : 0;
            int display_end = display_char_index;
            
            for (int i = display_start; i <= display_end; i++) {
                String displayChar = getCharAtIndex(display_segments[current_segment_index], i);
                if (displayChar.length() > 0) {
                    display_text += displayChar;
                }
            }
            
            avatar->setSpeechText(display_text.c_str());
            avatar->setMouthOpenRatio(getMouthRatioForChar(nextPhoneticChar)); // 発音制御
            last_update_time = millis();
            M5.Speaker.setVolume(beep_volume);
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

// テキスト分割関数（表示・発音分離版）
void TextAnimator::segmentTexts(const String& displayText, const String& phoneticText) {
    segment_count = 0;
    
    // 初期化
    for (int i = 0; i < 20; i++) {
        display_segments[i] = "";
        phonetic_segments[i] = "";
    }
    
    if (displayText.length() == 0) {
        return;
    }
    
    // 表示用テキストを分割
    int start = 0;
    int pos = 0;
    while (pos < displayText.length() && segment_count < 20) {
        if (displayText[pos] == '\n') {
            String segment = displayText.substring(start, pos);
            if (segment.length() > 0) {
                display_segments[segment_count] = segment;
                segment_count++;
            }
            start = pos + 1;
        }
        pos++;
    }
    if (start < displayText.length() && segment_count < 20) {
        String segment = displayText.substring(start);
        if (segment.length() > 0) {
            display_segments[segment_count] = segment;
            segment_count++;
        }
    }
    
    // 発音用テキストを分割
    int phonetic_segment_count = 0;
    start = 0;
    pos = 0;
    while (pos < phoneticText.length() && phonetic_segment_count < 20) {
        if (phoneticText[pos] == '\n') {
            String segment = phoneticText.substring(start, pos);
            if (segment.length() > 0) {
                phonetic_segments[phonetic_segment_count] = segment;
                phonetic_segment_count++;
            }
            start = pos + 1;
        }
        pos++;
    }
    if (start < phoneticText.length() && phonetic_segment_count < 20) {
        String segment = phoneticText.substring(start);
        if (segment.length() > 0) {
            phonetic_segments[phonetic_segment_count] = segment;
            phonetic_segment_count++;
        }
    }
    
    Serial.printf("Display segments: %d, Phonetic segments: %d\n", segment_count, phonetic_segment_count);
    for (int i = 0; i < segment_count; i++) {
        Serial.printf("Segment %d: Display='%s', Phonetic='%s'\n", i, display_segments[i].c_str(), phonetic_segments[i].c_str());
    }
}

// テキスト分割関数（旧版・互換性用）
void TextAnimator::segmentText(const String& text) {
    segment_count = 0;
    
    // 初期化
    for (int i = 0; i < 10; i++) {
        display_segments[i] = "";
        phonetic_segments[i] = "";
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
                display_segments[segment_count] = segment;
                phonetic_segments[segment_count] = segment; // 同じ内容
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
            display_segments[segment_count] = segment;
            phonetic_segments[segment_count] = segment; // 同じ内容
            segment_count++;
            Serial.printf("Segment %d: %s\n", segment_count - 1, segment.c_str());
        }
    }
    
    Serial.printf("Total segments: %d\n", segment_count);
}
