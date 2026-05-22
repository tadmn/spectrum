
#pragma once

#include "Common.h"
#include "FrameHelpers.h"

class FadeFrame : public Frame {
public:
    FadeFrame() {
        animation_.setTargetValue(1.0f);
        animation_.setAnimationTime(333);
    }

    void visibilityChanged() override {
        if (! isVisible())
            animation_.target(false, true);
    }

    void makeVisible(bool visible) {
        animation_.target(visible);
        if (visible)
            setVisible(true);

        redraw();
    }

    void draw(Canvas& canvas) final {
        const auto alpha = animation_.update();

        drawBackground(canvas, alpha);

        setFrameRecursive(*this, [alpha](Frame& f) {
            f.setAlphaTransparency(alpha);
        });

        if (animation_.isAnimating()) {
            redraw();
        } else {
            if (! animation_.isTargeting()) {
                setVisible(false);
            }
        }
    }

    virtual void drawBackground(Canvas& /*canvas*/, float /*hoverAmount*/) {}

private:
    Animation<float> animation_;

    VISAGE_LEAK_CHECKER(FadeFrame)
};
