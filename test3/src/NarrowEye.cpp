#include "NarrowEye.h"

NarrowEye::NarrowEye(bool isLeft) : isLeft(isLeft) {
}

void NarrowEye::draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) {
    uint16_t cx = rect.getCenterX();
    uint16_t cy = rect.getCenterY();
    
    // 色の取得
    ColorPalette *cp = ctx->getColorPalette();
    uint16_t primaryColor = cp->get(COLOR_PRIMARY);
    
    // 視線の取得（左右で異なる）
    Gaze gaze = isLeft ? ctx->getLeftGaze() : ctx->getRightGaze();
    
    // 視線に応じた位置調整
    int16_t offsetX = gaze.getHorizontal() * 3;  // 視線による横方向のオフセット
    int16_t offsetY = gaze.getVertical() * 2;    // 視線による縦方向のオフセット
    
    // 細い横棒の描画パラメータ
    int16_t lineWidth = 20;   // 横棒の長さ
    int16_t lineHeight = 2;   // 横棒の太さ
    
    // 細い横棒を描画（俳人の細い目）
    int16_t x1 = cx - lineWidth / 2 + offsetX;
    int16_t y1 = cy + offsetY;
    int16_t x2 = cx + lineWidth / 2 + offsetX;
    int16_t y2 = cy + offsetY;
    
    // 横線を描画（太さ分だけ複数ライン描画）
    for (int i = 0; i < lineHeight; i++) {
        spi->drawLine(x1, y1 + i, x2, y2 + i, primaryColor);
    }
}
