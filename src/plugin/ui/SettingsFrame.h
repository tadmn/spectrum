
#pragma once

#include "AnalyzerProcessor.h"
#include "embedded/Fonts.h"

#include <magic_enum/magic_enum.hpp>
#include <visage/ui.h>
#include <visage/widgets.h>

template<typename EnumType>
class LabeledPopupMenu : public visage::Frame {
  public:
    LabeledPopupMenu(const std::string& label, const std::function<void(EnumType)>& setValue) :
        mLabel(label) {
        mMenuButton.setFont({ 11, resources::fonts::DroidSansMono_ttf });
        mMenuButton.setActionButton(true);
        mMenuButton.onToggle() = [this, setValue](visage::Button* /*button*/, bool /*toggled*/) {
            visage::PopupMenu menu;
            for (auto e : magic_enum::enum_entries<EnumType>())
                menu.addOption(magic_enum::enum_index(e.first).value(), std::string(e.second));

            menu.onSelection() = [setValue](int id) {
                setValue(magic_enum::enum_cast<EnumType>(id).value());
            };

            menu.show(&mMenuButton);
        };

        addChild(mMenuButton);
    }

    ~LabeledPopupMenu() override { }

    void setValue(EnumType value) {
        mMenuButton.setText(std::string(magic_enum::enum_name(value)));
    }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(0xff000000);
        canvas.text(mLabel, { 11, resources::fonts::DroidSansMono_ttf }, visage::Font::kCenter,
                    mLabelBounds.x(), mLabelBounds.y(), mLabelBounds.width(), mLabelBounds.height());
    }

    void resized() override {
        auto b = localBounds();
        mLabelBounds = b.trimTop(13);
        b.trimTop(2); // Gap
        mMenuButton.setBounds(b);
    }

  private:
    std::string mLabel;
    visage::Bounds mLabelBounds;
    visage::UiButton mMenuButton;
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
        canvas.text(mLabel, { 11, resources::fonts::DroidSansMono_ttf }, visage::Font::kCenter,
                    mLabelBounds.x(), mLabelBounds.y(), mLabelBounds.width(), mLabelBounds.height());
    }

    void resized() override {
        auto b = localBounds();
        mLabelBounds = b.trimTop(13);
        b.trimTop(2); // Gap
        mEditor.setBounds(b);
    }

  private:
    std::string mLabel;
    visage::Bounds mLabelBounds;
    visage::TextEditor mEditor;
};

class SettingsFrame : public visage::Frame {
  public:
    SettingsFrame(AnalyzerProcessor& p) :
        mProcessor(p),
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
    mWindowTypeMenu("Windowing", [&p](tb::WindowType t){ p.setWindowType(t); }) {
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
        addChild(mWindowTypeMenu);

        updateSettings(); // Set initial values
    }

    ~SettingsFrame() override { }

    void updateSettings() {
        const auto& p = mProcessor;
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
        mWindowTypeMenu.setValue(p.windowType());
    }

    void draw(visage::Canvas& c) override {
        c.setColor(0xffffffff);
        c.roundedRectangle(0, 0, width(), height(), 10.0);
    }

    void resized() override {
        auto b = localBounds().reduced(1);
        for (auto* c : children()) {
            auto w = 85;
            if (c == &mWindowTypeMenu)
                w = 111;

            c->setBounds(b.trimLeft(w).reduced(4));
        }
    }

  private:
    AnalyzerProcessor& mProcessor;

    LabeledTextEditor mNumBands, mFftSize, mMinFreq, mMaxFreq, mFftHopSize, mAttack, mRelease,
        mDbPerOctave, mCenterFrequency, mMinDb, mSmoothingFactor;
    LabeledPopupMenu<tb::WindowType> mWindowTypeMenu;
};