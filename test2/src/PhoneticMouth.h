#ifndef PHONETIC_MOUTH_H
#define PHONETIC_MOUTH_H

#include <Arduino.h>
#include <Avatar.h>
#include <Mouth.h>

using namespace m5avatar;

// aiueo音素別の口形状パラメータ
struct MouthShape {
  uint16_t minWidth;
  uint16_t maxWidth; 
  uint16_t minHeight;
  uint16_t maxHeight;
  String description;
};

class PhoneticMouth {
private:
    // aiueo音素別の口形状定義
    static const MouthShape VOWEL_SHAPES[5];
    
    // デフォルト形状
    static const MouthShape DEFAULT_SHAPE;
    
    // 現在の口オブジェクト
    Mouth* currentMouth;
    
    // 音素から口形状を取得
    MouthShape getShapeForPhoneme(const String& phoneme);
    
public:
    PhoneticMouth();
    ~PhoneticMouth();
    
    // 音素に基づいて口形状を設定
    void setPhoneme(const String& phoneme);
    
    // カスタム口形状を設定（外部から幅・高さ指定可能）
    void setCustomShape(uint16_t minWidth, uint16_t maxWidth, 
                       uint16_t minHeight, uint16_t maxHeight);
    
    // 現在の口形状パラメータを取得
    MouthShape getCurrentShape() const;
    
    // Avatarに口を適用
    void applyToAvatar(Avatar* avatar);
    
    // 口形状をシリアル出力（デバッグ用）
    void printCurrentShape() const;
    
    // aiueo音素一覧を表示
    static void printAllShapes();
};

#endif // PHONETIC_MOUTH_H
