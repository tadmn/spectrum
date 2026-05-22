
#pragma once

#include "common/Common.h"
#include "common/FrameHelpers.h"
#include "common/UrlFrame.h"

#include "embedded/Shaders.h"

namespace overlay {

class LogoButton : public Button {
public:
    LogoButton() = default;

    void draw(Canvas& canvas, float hoverAmount) override {
        canvas.setColor(Color(0xffffff).withAlpha(0.45f + hoverAmount * 0.35f));
        spacedText(canvas, "Spectrum", 21, 7, 0, 0, height());
    }

private:
    VISAGE_LEAK_CHECKER(LogoButton)
};

class Overlay : public Frame {
public:
    Overlay() {
        url_.setUrl("github.com/tadmn");
        addChild(url_);
    }

  void resized() override {
        url_.setBounds(0, 111, width(), 24);
  }

  void draw(Canvas& canvas) override {
        const auto rounding = 25;

        // Background
        canvas.setBrush(Brush::linear(0xff282828, 0xff393939, { 0.3f * width(), 0 },
                                              { 0.66f * width(), height() }));
        canvas.roundedRectangle(0, 0, width(), height(), rounding);

        // Text
        canvas.setColor(0xffc6c6c6);
        spacedText(canvas, "Spectrum", 22, 8, 59, 19, 29);

        const Font font(12, resources::fonts::NotoSans_Regular_ttf);
        canvas.text(PRODUCT_VERSION, font.withSize(10), Font::kCenter, 0, 48, 285, 24);

        canvas.setColor(0xffffffff);
        canvas.text("Design & DSP by Tad Nicol", font, Font::kCenter, 0, 83, width(), 24);
        // canvas.text("github.com/tadmn", font, Font::kCenter, 0, 111, width(), 24);
    }

private:
    UrlFrame url_;

    VISAGE_LEAK_CHECKER(Overlay)
};

class OverlayFrame : public Frame {
public:
    OverlayFrame() :
        animation_(160.f, Animation<float>::kLinear, Animation<float>::kLinear),
        shader_zoom_(resources::shaders::vs_overlay, resources::shaders::fs_overlay) {
        addChild(overlay_);
        animation_.setTargetValue(1.f);
        setPostEffect(&shader_zoom_);

        overlay_.onMouseDown() += [this](const MouseEvent&) { dismiss(); };
        onMouseDown() += [this](const MouseEvent&) { dismiss(); };
    }

    void resized() override {
        const auto w = 285;
        const auto h = 156;
        overlay_.setBounds(width() / 2 - w / 2, 0.3 * height(), w, h);
    }

    void draw(Canvas& /*canvas*/) override {
        float overlay_amount = animation_.update();
        if (! animation_.isTargeting() && overlay_amount == 0.f)
            setVisible(false);

        shader_zoom_.setUniformValue("u_zoom", 0.075f * (1.0f - overlay_amount) + 1.0f);
        shader_zoom_.setUniformValue("u_alpha", overlay_amount * overlay_amount);

        on_animate_.callback(overlay_amount);

        if (animation_.isAnimating())
            redraw();
    }

    void dismiss() {
        animation_.target(false);
        redraw();
    }

    void visibilityChanged() override { animation_.target(isVisible()); }

    auto& onAnimate() { return on_animate_; }

private:
    Overlay overlay_;

    Animation<float> animation_;
    CallbackList<void(float)> on_animate_;
    ShaderPostEffect shader_zoom_;

    VISAGE_LEAK_CHECKER(OverlayFrame)
};

}