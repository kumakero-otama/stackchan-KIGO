#include "PhoneticMouth.h"

// aiueo音素別の口形状定義
const MouthShape PhoneticMouth::VOWEL_SHAPES[5] = {
  // あ (a) - 大きく開いた口
  {20, 80, 10, 40, "あ(a): 大きく開いた口"},
  
  // い (i) - 横に広い口
  {30, 90, 5, 15, "い(i): 横に広い口"},
  
  // う (u) - 小さく丸い口
  {15, 40, 8, 25, "う(u): 小さく丸い口"},
  
  // え (e) - やや開いた口
  {25, 70, 8, 30, "え(e): やや開いた口"},
  
  // お (o) - 丸い口
  {20, 60, 15, 35, "お(o): 丸い口"}
};

// デフォルト形状
const MouthShape PhoneticMouth::DEFAULT_SHAPE = {
  30, 60, 4, 20, "デフォルト: 中立"
};

PhoneticMouth::PhoneticMouth() : currentMouth(nullptr) {
  // デフォルト形状で初期化
  currentMouth = new Mouth(DEFAULT_SHAPE.minWidth, DEFAULT_SHAPE.maxWidth,
                          DEFAULT_SHAPE.minHeight, DEFAULT_SHAPE.maxHeight);
}

PhoneticMouth::~PhoneticMouth() {
  if (currentMouth) {
    delete currentMouth;
    currentMouth = nullptr;
  }
}

MouthShape PhoneticMouth::getShapeForPhoneme(const String& phoneme) {
  // 音素の最初の文字で判定
  if (phoneme.length() > 0) {
    String firstChar = phoneme.substring(0, 1);
    
    // あ行音素
    if (firstChar == "あ" || firstChar == "か" || firstChar == "が" || 
        firstChar == "さ" || firstChar == "ざ" || firstChar == "た" || 
        firstChar == "だ" || firstChar == "な" || firstChar == "は" || 
        firstChar == "ば" || firstChar == "ぱ" || firstChar == "ま" || 
        firstChar == "や" || firstChar == "ら" || firstChar == "わ") {
      return VOWEL_SHAPES[0]; // あ
    }
    
    // い行音素
    else if (firstChar == "い" || firstChar == "き" || firstChar == "ぎ" || 
             firstChar == "し" || firstChar == "じ" || firstChar == "ち" || 
             firstChar == "ぢ" || firstChar == "に" || firstChar == "ひ" || 
             firstChar == "び" || firstChar == "ぴ" || firstChar == "み" || 
             firstChar == "り") {
      return VOWEL_SHAPES[1]; // い
    }
    
    // う行音素
    else if (firstChar == "う" || firstChar == "く" || firstChar == "ぐ" || 
             firstChar == "す" || firstChar == "ず" || firstChar == "つ" || 
             firstChar == "づ" || firstChar == "ぬ" || firstChar == "ふ" || 
             firstChar == "ぶ" || firstChar == "ぷ" || firstChar == "む" || 
             firstChar == "ゆ" || firstChar == "る") {
      return VOWEL_SHAPES[2]; // う
    }
    
    // え行音素
    else if (firstChar == "え" || firstChar == "け" || firstChar == "げ" || 
             firstChar == "せ" || firstChar == "ぜ" || firstChar == "て" || 
             firstChar == "で" || firstChar == "ね" || firstChar == "へ" || 
             firstChar == "べ" || firstChar == "ぺ" || firstChar == "め" || 
             firstChar == "れ") {
      return VOWEL_SHAPES[3]; // え
    }
    
    // お行音素
    else if (firstChar == "お" || firstChar == "こ" || firstChar == "ご" || 
             firstChar == "そ" || firstChar == "ぞ" || firstChar == "と" || 
             firstChar == "ど" || firstChar == "の" || firstChar == "ほ" || 
             firstChar == "ぼ" || firstChar == "ぽ" || firstChar == "も" || 
             firstChar == "よ" || firstChar == "ろ" || firstChar == "を") {
      return VOWEL_SHAPES[4]; // お
    }
    
    // ん - 口を閉じる
    else if (firstChar == "ん") {
      return {10, 30, 2, 8, "ん: 口を閉じる"};
    }
    
    // っ - 促音、口を小さく
    else if (firstChar == "っ" || firstChar == "ッ") {
      return {15, 35, 3, 10, "っ: 促音、小さく"};
    }
  }
  
  // 未対応の音素はデフォルト
  return DEFAULT_SHAPE;
}

void PhoneticMouth::setPhoneme(const String& phoneme) {
  MouthShape shape = getShapeForPhoneme(phoneme);
  
  // 現在の口オブジェクトを削除
  if (currentMouth) {
    delete currentMouth;
  }
  
  // 新しい口オブジェクトを作成
  currentMouth = new Mouth(shape.minWidth, shape.maxWidth,
                          shape.minHeight, shape.maxHeight);
  
  Serial.printf("口形状設定: %s -> %s\n", phoneme.c_str(), shape.description.c_str());
  Serial.printf("  幅: %d-%d, 高さ: %d-%d\n", 
                shape.minWidth, shape.maxWidth, 
                shape.minHeight, shape.maxHeight);
}

void PhoneticMouth::setCustomShape(uint16_t minWidth, uint16_t maxWidth, 
                                  uint16_t minHeight, uint16_t maxHeight) {
  // 現在の口オブジェクトを削除
  if (currentMouth) {
    delete currentMouth;
  }
  
  // カスタム形状で新しい口オブジェクトを作成
  currentMouth = new Mouth(minWidth, maxWidth, minHeight, maxHeight);
  
  Serial.printf("カスタム口形状設定: 幅=%d-%d, 高さ=%d-%d\n", 
                minWidth, maxWidth, minHeight, maxHeight);
}

MouthShape PhoneticMouth::getCurrentShape() const {
  // 現在のMouthオブジェクトからパラメータを取得することは
  // できないため、最後に設定された形状を返す
  // (実際の実装では内部状態を保持する必要がある)
  return DEFAULT_SHAPE;
}

void PhoneticMouth::applyToAvatar(Avatar* avatar) {
  if (avatar && currentMouth) {
    // 現在のFaceを取得
    Face* currentFace = avatar->getFace();
    if (currentFace) {
      // Faceに口を設定
      currentFace->setMouth(currentMouth);
      Serial.println("口形状をFaceに適用しました");
    } else {
      Serial.println("現在のFaceが取得できませんでした");
    }
  }
}

void PhoneticMouth::printCurrentShape() const {
  if (currentMouth) {
    Serial.println("現在の口形状パラメータ:");
    Serial.println("  (Mouthオブジェクトからの取得は非対応)");
  } else {
    Serial.println("口オブジェクトが設定されていません");
  }
}

void PhoneticMouth::printAllShapes() {
  Serial.println("=== aiueo音素別口形状一覧 ===");
  for (int i = 0; i < 5; i++) {
    const MouthShape& shape = VOWEL_SHAPES[i];
    Serial.printf("%d. %s\n", i+1, shape.description.c_str());
    Serial.printf("   幅: %d-%d, 高さ: %d-%d\n", 
                  shape.minWidth, shape.maxWidth, 
                  shape.minHeight, shape.maxHeight);
  }
  Serial.printf("デフォルト: %s\n", DEFAULT_SHAPE.description.c_str());
  Serial.printf("   幅: %d-%d, 高さ: %d-%d\n", 
                DEFAULT_SHAPE.minWidth, DEFAULT_SHAPE.maxWidth, 
                DEFAULT_SHAPE.minHeight, DEFAULT_SHAPE.maxHeight);
  Serial.println("===============================");
}
