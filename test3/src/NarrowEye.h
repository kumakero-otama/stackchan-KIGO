#ifndef NARROWEYE_H
#define NARROWEYE_H

#include <Avatar.h>

using namespace m5avatar;

// 俳人用の細い横棒の目クラス
class NarrowEye : public Drawable {
private:
    bool isLeft;
    
public:
    NarrowEye(bool isLeft);
    void draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) override;
};

#endif
