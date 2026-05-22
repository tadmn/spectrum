
#pragma once

#include "Common.h"

class Shelf : public Frame {
public:
    Shelf() {
        setIgnoresMouseEvents(true, false);
    }

    void draw(Canvas& canvas) override {
        const auto h = height();

        Path path;
        path.moveTo(0, h);
        path.lineTo(13, 7);
        path.bezierTo(16, 3, 19, 2, 25, 2);
        path.lineTo(429, 2);
        path.bezierTo(435, 2, 438, 3, 441, 7);
        path.lineTo(454, h);

        const auto strokePath = path.stroke(1);

        path.close();

        canvas.setBrush(Brush::linear(0xff202020, 0xff313131, { 0, 0 }, { 0, h }));
        canvas.fill(path);

        // Border (slightly lighter)
        canvas.setColor(0xff454545);
        canvas.fill(strokePath);
    }

private:
    VISAGE_LEAK_CHECKER(Shelf)
};