
#pragma once

#include "AnalyzerProcessor.h"
#include "embedded/Fonts.h"

#include <visage/widgets.h>
#include <visage/ui.h>
#include <magic_enum/magic_enum.hpp>

class PaletteColorWindow : public visage::ApplicationWindow {
  public:
    PaletteColorWindow(visage::Palette& palette) {
        setTitle("Spectrum Fill Color");
        const auto id = visage::GraphLine::LineFillColor;
        visage::Brush color;
        if (palette.color({}, id, color))
            mFillColorPicker.setColor(color.gradient().sample(0));

        mFillColorPicker.onColorChange().add([id, &palette](visage::Color c) {
            palette.setColor(id, c);
        });

        addChild(mFillColorPicker);
    }

    void resized() override { mFillColorPicker.setBounds(0, 0, width(), height()); }

  private:
    visage::ColorPicker mFillColorPicker;
};

class LabeledTextEditor : public visage::Frame {
  public:
    LabeledTextEditor(const std::string& label,
                      const std::function<void(const visage::String&)>& setValue) : mLabel(label) {
        mEditor.setFont({ 13, resources::fonts::DroidSansMono_ttf });
        mEditor.onEnterKey() = [this, setValue] { setValue(mEditor.text()); };
        mEditor.setAlphaTransparency(0.9);
        addChild(mEditor);
    }

    ~LabeledTextEditor() override { }

    void setValue(const visage::String& value) { mEditor.setText(value); }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(0xff000000);
        canvas.text(mLabel, { kLabelFontSize, resources::fonts::DroidSansMono_ttf },
                    visage::Font::kCenter, 0, 2, width(), kLabelFontSize);
    }

    void resized() override {
        constexpr auto kGap = 4;
        mEditor.setBounds(0, kLabelFontSize + kGap, width(), height() - kLabelFontSize - kGap);
    }

  private:
    static constexpr auto kLabelFontSize = 11;
    std::string mLabel;
    visage::TextEditor mEditor;
};

class SettingsFrame : public visage::Frame {
  public:
    SettingsFrame(AnalyzerProcessor& p) :
        mNumBands("Bands", [&p](const visage::String& t) { p.setTargetNumBands(t.toInt()); }),
        mFftSize("FFT Size", [&p](const visage::String& t) { p.setFftSize(t.toInt()); }),
        mMinFreq("Min Freq", [&p](const visage::String& t) { p.setMinFrequency(t.toFloat()); }),
        mMaxFreq("Max Freq", [&p](const visage::String& t) { p.setMaxFrequency(t.toFloat()); }),
        mFftHopSize("Hop Size", [&p](const visage::String& t) { p.setFftHopSize(t.toInt()); }),
        mAttack("Attack", [&p](const visage::String& t) { p.setAttackRate(t.toFloat()); }),
        mRelease("Release", [&p](const visage::String& t) { p.setReleaseRate(t.toFloat()); }),
        mDbPerOctave("dB/Octave",
                     [&p](const visage::String& t) { p.setWeightingDbPerOctave(t.toFloat()); }),
        mCenterFrequency("Fc",
                         [&p](const visage::String& t) {
                             p.setWeightingCenterFrequency(t.toFloat());
                         }),
        mMinDb("Min dB", [&p](const visage::String& t) { p.setMinDb(t.toFloat()); }),
        mSmoothingFactor("Smooth",
                         [&p](const visage::String& t) { p.setLineSmoothingFactor(t.toFloat()); }),
    mWindowMenuButton("", {10, resources::fonts::DroidSansMono_ttf}){
        addChild(mNumBands);
        addChild(mFftSize);
        addChild(mFftHopSize);
        addChild(mMinFreq);
        addChild(mMaxFreq);
        addChild(mAttack);
        addChild(mRelease);
        addChild(mDbPerOctave);
        addChild(mCenterFrequency);
        addChild(mMinDb);
        addChild(mSmoothingFactor);
        addChild(mWindowMenuButton);

        auto onParametersChanged = [this, &p] {
            mNumBands.setValue(p.bands().size());
            mFftSize.setValue(p.fftSize());
            mFftHopSize.setValue(p.fftHopSize());
            mMinFreq.setValue(p.minFrequency());
            mMaxFreq.setValue(p.maxFrequency());
            mAttack.setValue(p.attackRate());
            mRelease.setValue(p.releaseRate());
            mDbPerOctave.setValue(p.weightingDbPerOctave());
            mCenterFrequency.setValue(p.weightingCenterFrequency());
            mMinDb.setValue(p.minDb());
            mSmoothingFactor.setValue(p.lineSmoothingFactor());
            mWindowMenuButton.setText(std::string(magic_enum::enum_name(p.windowType())));
        };

        onParametersChanged();  // Set initial values
        p.onParametersChanged = std::move(onParametersChanged);

        mWindowMenuButton.setActionButton(true);
        mWindowMenuButton.onToggle() = [&p, this](visage::Button* /*button*/, bool /*toggled*/) {
            visage::PopupMenu menu;
            for (auto w : magic_enum::enum_entries<tb::WindowType>())
                menu.addOption(magic_enum::enum_index(w.first).value(), std::string(w.second));

            menu.onSelection() = [&p](int id) {
                p.setWindowType(magic_enum::enum_cast<tb::WindowType>(id).value());
            };

            menu.show(&mWindowMenuButton);
        };
    }

    ~SettingsFrame() override { }

    void draw(visage::Canvas& c) override {
        c.setColor(0xffffffff);
        c.roundedRectangle(0, 0, width(), height(), 10.0);
    }

    void resized() override {
        auto b = localBounds().reduced(1);
        for (auto* c : children()) {
            auto w = 85;
            if (c == &mWindowMenuButton)
                w = 118;

            c->setBounds(b.trimLeft(w).reduced(4));
        }
    }

  private:
    LabeledTextEditor mNumBands, mFftSize, mMinFreq, mMaxFreq, mFftHopSize, mAttack, mRelease,
        mDbPerOctave, mCenterFrequency, mMinDb, mSmoothingFactor;
    visage::UiButton mWindowMenuButton;
};