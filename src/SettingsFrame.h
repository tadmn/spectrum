
#pragma once

#include "AnalyzerProcessor.h"
#include "embedded/Fonts.h"
#include "visage/widgets.h"

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
        mEditor.setFont({ 30, resources::fonts::DroidSansMono_ttf });
        mEditor.onEnterKey() = [this, setValue] { setValue(mEditor.text()); };
        mEditor.setAlphaTransparency(0.9);
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
        mMinDb("Min dB", [&p](const visage::String& t) { p.setMinDb(t.toFloat()); }) {
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
    LabeledTextEditor mNumBands, mFftSize, mMinFreq, mMaxFreq, mFftHopSize, mAttack, mRelease,
        mDbPerOctave, mCenterFrequency, mMinDb;
};