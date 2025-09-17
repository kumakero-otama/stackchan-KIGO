#include "SG90Handler.h"

SG90Handler::SG90Handler() : is_moving(false), last_x(90), last_y(90) {
}

// 参考ファイルのイージング関数を実装
float SG90Handler::quadraticEaseInOut(float p) {
    if(p < 0.5) {
        return 2 * p * p;
    } else {
        return (-2 * p * p) + (4 * p) - 1;
    }
}

void SG90Handler::begin(int pin_x, int pin_y, int home_x, int home_y) {
    this->pin_x = pin_x;
    this->pin_y = pin_y;
    this->home_x = home_x;
    this->home_y = home_y;
    this->last_x = home_x;
    this->last_y = home_y;
    
    // ESP32Servoライブラリを使用
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    servo_x.setPeriodHertz(50);
    servo_y.setPeriodHertz(50);
    
    if (!servo_x.attach(pin_x, 500, 2400)) {  // 0.5ms-2.4msのパルス幅（SG90正式仕様）
        Serial.println("Error: Failed to attach servo X");
        return;
    }
    
    if (!servo_y.attach(pin_y, 500, 2400)) {  // 0.5ms-2.4msのパルス幅（SG90正式仕様）
        Serial.println("Error: Failed to attach servo Y");
        return;
    }
    
    // ホームポジションに移動
    servo_x.write(home_x);
    servo_y.write(home_y);
    
    Serial.printf("SG90Handler initialized: X=pin%d, Y=pin%d, Home=(%d,%d)\n", 
                  pin_x, pin_y, home_x, home_y);
}

void SG90Handler::moveX(int angle, uint32_t duration) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    is_moving = true;
    
    Serial.printf("Moving X servo from %d to %d degrees in %d ms\n", last_x, angle, duration);
    
    if (duration == 0) {
        // 即座に移動
        servo_x.write(angle);
        delay(100);
    } else {
        // 参考ファイルに基づくイージング実装（修正版）
        int increase_degree = angle - last_x;
        const int SERIAL_EASE_DIVISION = 20;
        uint32_t division_time = duration / SERIAL_EASE_DIVISION;
        if (division_time < 10) division_time = 10;
        
        Serial.printf("Easing: start=%d, target=%d, increase=%d, steps=%d, time_per_step=%d\n", 
                      last_x, angle, increase_degree, SERIAL_EASE_DIVISION, division_time);
        
        for (int i = 0; i <= SERIAL_EASE_DIVISION; i++) {
            float f = (float)i / (float)SERIAL_EASE_DIVISION;
            int target_angle = last_x + increase_degree * quadraticEaseInOut(f);
            Serial.printf("Step %d: f=%.2f, target_angle=%d\n", i, f, target_angle);
            servo_x.write(target_angle);
            delay(division_time);
        }
    }
    
    last_x = angle;
    is_moving = false;
}

void SG90Handler::moveY(int angle, uint32_t duration) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    is_moving = true;
    
    Serial.printf("Moving Y servo from %d to %d degrees in %d ms\n", last_y, angle, duration);
    
    if (duration == 0) {
        // 即座に移動
        servo_y.write(angle);
        delay(100);
    } else {
        // 参考ファイルに基づくイージング実装（修正版）
        int increase_degree = angle - last_y;
        const int SERIAL_EASE_DIVISION = 20;
        uint32_t division_time = duration / SERIAL_EASE_DIVISION;
        if (division_time < 10) division_time = 10;
        
        Serial.printf("Easing: start=%d, target=%d, increase=%d, steps=%d, time_per_step=%d\n", 
                      last_y, angle, increase_degree, SERIAL_EASE_DIVISION, division_time);
        
        for (int i = 0; i <= SERIAL_EASE_DIVISION; i++) {
            float f = (float)i / (float)SERIAL_EASE_DIVISION;
            int target_angle = last_y + increase_degree * quadraticEaseInOut(f);
            Serial.printf("Step %d: f=%.2f, target_angle=%d\n", i, f, target_angle);
            servo_y.write(target_angle);
            delay(division_time);
        }
    }
    
    last_y = angle;
    is_moving = false;
}

void SG90Handler::moveXY(int x, int y, uint32_t duration) {
    if (x < 0) x = 0;
    if (x > 180) x = 180;
    if (y < 0) y = 0;
    if (y > 180) y = 180;
    
    is_moving = true;
    
    Serial.printf("Moving XY servos from (%d,%d) to (%d,%d) degrees in %d ms\n", 
                  last_x, last_y, x, y, duration);
    
    if (duration == 0) {
        // 即座に移動
        servo_x.write(x);
        servo_y.write(y);
        delay(100);
    } else {
        // 参考ファイルに基づく同期イージング実装（修正版）
        int increase_x = x - last_x;
        int increase_y = y - last_y;
        const int SERIAL_EASE_DIVISION = 20;
        uint32_t division_time = duration / SERIAL_EASE_DIVISION;
        if (division_time < 10) division_time = 10;
        
        Serial.printf("XY Easing: start=(%d,%d), target=(%d,%d), increase=(%d,%d)\n", 
                      last_x, last_y, x, y, increase_x, increase_y);
        
        for (int i = 0; i <= SERIAL_EASE_DIVISION; i++) {
            float f = (float)i / (float)SERIAL_EASE_DIVISION;
            int target_x = last_x + increase_x * quadraticEaseInOut(f);
            int target_y = last_y + increase_y * quadraticEaseInOut(f);
            Serial.printf("XY Step %d: f=%.2f, target=(%d,%d)\n", i, f, target_x, target_y);
            servo_x.write(target_x);
            servo_y.write(target_y);
            delay(division_time);
        }
    }
    
    last_x = x;
    last_y = y;
    is_moving = false;
}

void SG90Handler::nod() {
    Serial.println("=== SG90: Easing Nod Test (Up-Down Pattern) ===");
    Serial.printf("Home position: X=%d, Y=%d\n", home_x, home_y);
    
    // Step 1: 上に45度（イージング付き）
    Serial.println("Step 1: Moving up 45 degrees with easing");
    moveY(home_y + 45, 1000);  // 上45度（135度）、1秒でイージング
    delay(500);
    
    // Step 2: 0度（センター位置）（イージング付き）
    Serial.println("Step 2: Moving to 0 degrees (center) with easing");
    moveY(home_y, 1000);  // センター位置（90度）、1秒でイージング
    delay(500);
    
    // Step 3: 上に45度（イージング付き）
    Serial.println("Step 3: Moving up 45 degrees again with easing");
    moveY(home_y + 45, 1000);  // 上45度（135度）、1秒でイージング
    delay(500);
    
    // Step 4: 0度（センター位置）（イージング付き）
    Serial.println("Step 4: Moving to 0 degrees (center) with easing");
    moveY(home_y, 1000);  // センター位置（90度）、1秒でイージング
    
    Serial.println("=== SG90: Easing Nod Complete ===");
}

void SG90Handler::shake() {
    Serial.println("=== SG90: Easing Head Shake Test ===");
    Serial.printf("Home position: X=%d, Y=%d\n", home_x, home_y);
    
    // Step 1: センター位置（イージング付き）
    Serial.println("Step 1: Moving to center with easing");
    moveX(home_x, 1000);  // センター位置（90度）、1秒でイージング
    delay(500);
    
    // Step 2: 右に振る（イージング付き）
    Serial.println("Step 2: Moving right with easing");
    moveX(home_x + 45, 1000);  // 右45度（135度）、1秒でイージング
    delay(500);
    
    // Step 3: 左に振る（イージング付き）
    Serial.println("Step 3: Moving left with easing");
    moveX(home_x - 45, 1500);  // 左45度（45度）、1.5秒でイージング
    delay(500);
    
    // Step 4: センター位置に戻る（イージング付き）
    Serial.println("Step 4: Moving back to center with easing");
    moveX(home_x, 1000);  // センター位置（90度）、1秒でイージング
    
    Serial.println("=== SG90: Easing Head Shake Complete ===");
}

void SG90Handler::greet() {
    Serial.println("=== SG90: Greeting Motion Start ===");
    Serial.printf("Home position: X=%d, Y=%d\n", home_x, home_y);
    Serial.printf("Step 1: Moving Y from %d to %d (up)\n", home_y, home_y + 25);
    moveY(home_y + 25, 1000);  // より大きく上に
    delay(400);
    Serial.printf("Step 2: Moving Y from %d to %d (down)\n", home_y + 25, home_y - 20);
    moveY(home_y - 20, 800);  // 下に
    delay(400);
    Serial.printf("Step 3: Moving Y from %d to %d (back to home)\n", home_y - 20, home_y);
    moveY(home_y, 1000);       // 元に戻る
    Serial.println("=== SG90: Greeting Motion Complete ===");
}

void SG90Handler::meditate() {
    Serial.println("=== SG90: Meditation Pose Start ===");
    Serial.printf("Home position: X=%d, Y=%d\n", home_x, home_y);
    Serial.printf("Moving to meditation pose: X=%d, Y=%d (slowly)\n", home_x, home_y - 35);
    moveXY(home_x, home_y - 35, 2500);  // より大きくゆっくりと下向きに（瞑想ポーズ）
    Serial.println("=== SG90: Meditation Pose Complete ===");
}

void SG90Handler::goHome(uint32_t duration) {
    Serial.println("SG90: Going home");
    moveXY(home_x, home_y, duration);
}
