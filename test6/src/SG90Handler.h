#ifndef SG90_HANDLER_H
#define SG90_HANDLER_H

#include <Arduino.h>
#include <ESP32Servo.h>

class SG90Handler {
private:
    Servo servo_x;  // X軸（左右）
    Servo servo_y;  // Y軸（上下）
    int pin_x, pin_y;
    int home_x, home_y;   // ホームポジション
    bool is_moving;
    int last_x, last_y;   // 前回の位置を記録
    
    // 参考ファイルのイージング関数
    float quadraticEaseInOut(float p);
    
public:
    SG90Handler();
    
    // 初期化
    void begin(int pin_x, int pin_y, int home_x = 90, int home_y = 90);
    
    // 基本動作（イージング付き）
    void moveX(int angle, uint32_t duration = 1000);
    void moveY(int angle, uint32_t duration = 1000);
    void moveXY(int x, int y, uint32_t duration = 1000);
    
    // 定義済みモーション
    void nod();          // うなづき
    void shake();        // 首振り
    void greet();        // 挨拶
    void meditate();     // 瞑想（俳人用）
    
    // ホームポジションに戻る
    void goHome(uint32_t duration = 1000);
    
    // 状態確認
    bool isMoving() { return is_moving; }
};

#endif // SG90_HANDLER_H
