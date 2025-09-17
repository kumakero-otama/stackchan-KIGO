#include "PoetFace.h"

PoetFace::PoetFace()
    : Face(
        // 口（中立表情ベース）
        new Mouth(50, 90, 4, 60), new BoundingRect(168, 163),
        // 右目（細い横棒の目）
        new NarrowEye(false), new BoundingRect(103, 80),
        // 左目（細い横棒の目）
        new NarrowEye(true), new BoundingRect(106, 240),
        // 右眉（中立表情ベース）
        new Eyeblow(15, 2, false), new BoundingRect(67, 96),
        // 左眉（中立表情ベース）
        new Eyeblow(15, 2, true), new BoundingRect(72, 230)
    ) {
}
