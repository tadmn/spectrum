
#pragma once

#include "AnalyzerProcessor.h"
#include "embedded/Fonts.h"
#include "visage/widgets.h"

class LabeledTextEditor : public visage::Frame {
  public:
    LabeledTextEditor(const std::string& label,
                      const std::function<void(const visage::String&)>& setValue) : mLabel(label) {
        mEditor.setFont({ 30, resources::fonts::DroidSansMono_ttf });
        mEditor.onEnterKey() = [this, setValue] { setValue(mEditor.text()); };
        addChild(mEditor);
    }

    ~LabeledTextEditor() override { }

    void setValue(const visage::String& value) { mEditor.setText(value); }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(0xff000000);
        canvas.text(mLabel, { kFontSize, resources::fonts::DroidSansMono_ttf },
                    visage::Font::kCenter, 0, 0, width(), kFontSize);
    }

    void resized() override { mEditor.setBounds(0, kFontSize, width(), height() - kFontSize); }

  private:
    static constexpr auto kFontSize = 25;
    std::string mLabel;
    visage::TextEditor mEditor;
};

class SettingsFrame : public visage::Frame {
  public:
    SettingsFrame(AnalyzerProcessor& p) :
    mFftSize("FFT Size", [&p](const visage::String& t) { p.setFftSize(t.toInt()); }),
    mFftHopSize("Hop Size", [&p](const visage::String& t) { p.setFftHopSize(t.toInt()); }),
    mAttack("Attack", [&p](const visage::String& t) { p.setAttackRate(t.toFloat()); }),
    mRelease("Release", [&p](const visage::String& t) { p.setReleaseRate(t.toFloat()); }) {
        addChild(mFftSize);
        addChild(mFftHopSize);
        addChild(mAttack);
        addChild(mRelease);

        auto onParametersChanged = [this, &p] {
            mFftSize.setValue(p.fftSize());
            mFftHopSize.setValue(p.fftHopSize());
            mAttack.setValue(p.attackRate());
            mRelease.setValue(p.releaseRate());
        };

        onParametersChanged();  // Set initial values
        p.onParametersChanged = std::move(onParametersChanged);
    }

    ~SettingsFrame() override { }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(0xffffffff);
        canvas.fill(0, 0, width(), height());
    }

    void resized() override {
        const auto w = width() / children().size();
        for (int i = 0; i < children().size(); ++i)
            children()[i]->setBounds(i * w, 0, w, height());
    }

  private:
    LabeledTextEditor mFftSize, mFftHopSize, mAttack, mRelease;
};