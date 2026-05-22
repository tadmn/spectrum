
#pragma once

#include <tb_Math.h>
#include <functional>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <nlohmann/json.hpp>
#include <ranges>
#include <utility>

#include "common/Common.h"

class State {
public:
    class Listener {
    public:
        Listener(State& s, size_t id) : state_(s), id_(id) { }
        ~Listener() { state_.callbacks_.erase(id_); }

    private:
        State& state_;
        size_t id_;
    };

    State(AnalyzerProcessor& p, std::function<void()> notify_host_handler) :
        analyzer_processor_(p), notify_host_handler_(std::move(notify_host_handler)) { }

    void setSampleRate(double sample_rate) {
        tb_assert(sample_rate > 0.0);
        non_realtime_params_.sample_rate = sample_rate;
        stateChanged();
        syncAnalyzer();
    }

    double sample_rate() const noexcept { return non_realtime_params_.sample_rate; }

    void setTargetNumBands(int num_bands) {
        num_bands = std::clamp(num_bands, 5, 600);
        non_realtime_params_.target_num_bands = num_bands;
        stateChanged();
        asyncUpdateAnalyzer();
    }

    int target_num_bands() const noexcept { return non_realtime_params_.target_num_bands; }

    void setFftSize(int fft_size) {
        tb_assert(tb::isPowerOf2(fft_size));
        fft_size = std::max(4096, tb::closestPowerOf2(fft_size));
        non_realtime_params_.fft_size = fft_size;
        stateChanged();
        asyncUpdateAnalyzer();
    }

    int fft_size() const noexcept { return non_realtime_params_.fft_size; }

    void setWeightingDbPerOctave(float dB_per_octave) {
        dB_per_octave = std::clamp(dB_per_octave, 0.0f, 6.0f);
        non_realtime_params_.weighting_db_per_octave = dB_per_octave;
        stateChanged();
        asyncUpdateAnalyzer();
    }

    float weighting_db_per_octave() const noexcept { return non_realtime_params_.weighting_db_per_octave; }

    void setWeightingCenterFrequency(float center_freq) {
        center_freq = std::clamp(center_freq, 10.0f, 20'000.0f);
        non_realtime_params_.weighting_center_frequency = center_freq;
        stateChanged();
        asyncUpdateAnalyzer();
    }

    float weighting_center_frequency() const noexcept { return non_realtime_params_.weighting_center_frequency; }

    void setLineSmoothingInterpolationSteps(int num_steps) {
        num_steps = std::clamp(num_steps, 0, 10);
        non_realtime_params_.line_interpolation_steps = num_steps;
        stateChanged();
        asyncUpdateAnalyzer();
    }

    int line_smoothing_interpolation_steps() const noexcept { return non_realtime_params_.line_interpolation_steps; }

    void setWindowType(tb::WindowType type) {
        non_realtime_params_.window_type = type;
        stateChanged();
        asyncUpdateAnalyzer();
    }

    tb::WindowType window_type() const noexcept { return non_realtime_params_.window_type; }

    void setMinDb(float min_dB) {
        min_dB = std::clamp(min_dB, -125.0f, -40.0f);
        analyzer_processor_.setMinDb(min_dB);
        stateChanged();
    }

    float min_dB() const noexcept { return analyzer_processor_.minDb(); }

    void setMaxDb(float max_dB) {
        max_dB = std::clamp(max_dB, -35.0f, 20.0f);
        analyzer_processor_.setMaxDb(max_dB);
        stateChanged();
    }

    float max_dB() const noexcept { return analyzer_processor_.maxDb(); }

    void setAttackRate(float rate) {
        rate = std::clamp(rate, 0.0f, 30.0f);
        analyzer_processor_.setAttackRate(rate);
        stateChanged();
    }

    float attack_rate() const noexcept { return analyzer_processor_.attackRate(); }

    void setReleaseRate(float rate) {
        rate = std::clamp(rate, 0.0f, 15.0f);
        analyzer_processor_.setReleaseRate(rate);
        stateChanged();
    }

    float release_rate() const noexcept { return analyzer_processor_.releaseRate(); }

    void setHideControls(bool hide) {
        hide_controls_ = hide;
        stateChanged();
    }

    bool hide_controls() const noexcept { return hide_controls_; }

    bool loadFromJson(const std::string& json_state) {
        try {
            nlohmann::json j = nlohmann::json::parse(json_state);

            {
                // Mute listener callbacks until we get everything loaded (ensures that host
                // state doesn't get marked as dirty)
                tb::ScopedSetter ss(notify_listeners_, false);

                // Apply all parameters from the loaded JSON at once
                setFftSize(j["fft_size"].get<int>());
                setTargetNumBands(j["target_num_bands"].get<int>());
                setWeightingDbPerOctave(j["weighting_db_per_octave"].get<float>());
                setWeightingCenterFrequency(j["weighting_center_frequency"].get<float>());
                setLineSmoothingInterpolationSteps(j["line_smoothing_factor"].get<int>());

                const auto windowType = magic_enum::enum_cast<tb::WindowType>(j["window_type"].get<std::string>());
                if (windowType.has_value())
                    setWindowType(windowType.value());

                setAttackRate(j["attack"].get<float>());
                setReleaseRate(j["release"].get<float>());
                setMinDb(j["min_db"].get<float>());
                setMaxDb(j["max_db"].get<float>());
                setHideControls(j["hide_controls"].get<bool>());
            }

            stateChanged();

            return true;
        } catch (std::exception& e) {
            std::cerr << "Spectrum: Failed to load state: " << e.what() << std::endl;
            return false;
        }
    }

    std::string saveToJson() const {
        nlohmann::json j;

        // Store all parameters in the JSON object
        j["fft_size"] = fft_size();
        j["target_num_bands"] = target_num_bands();
        j["weighting_db_per_octave"] = weighting_db_per_octave();
        j["weighting_center_frequency"] = weighting_center_frequency();
        j["line_smoothing_factor"] = line_smoothing_interpolation_steps();
        j["window_type"] = std::string(magic_enum::enum_name(window_type()));
        j["attack"] = attack_rate();
        j["release"] = release_rate();
        j["min_db"] = min_dB();
        j["max_db"] = max_dB();
        j["hide_controls"] = hide_controls();

        return j.dump();
    }

    std::unique_ptr<Listener> addListener(std::function<void()> on_state_change) {
        auto id = callbacks_.size();
        while (callbacks_.contains(id))
            id++;

        callbacks_[id] = std::move(on_state_change);
        return std::make_unique<Listener>(*this, id);
    }

    void resetToDefaults() {
        const auto sample_rate = non_realtime_params_.sample_rate;
        non_realtime_params_ = {};
        non_realtime_params_.sample_rate = sample_rate;
        non_realtime_params_.min_frequency = k_min_frequency;
        non_realtime_params_.max_frequency = k_max_frequency;
        setAttackRate(AnalyzerProcessor::k_default_attack);
        setReleaseRate(AnalyzerProcessor::k_default_release);
        setMinDb(AnalyzerProcessor::k_default_min_dB);
        setMaxDb(AnalyzerProcessor::k_default_max_dB);
        stateChanged();
        syncAnalyzer();
    }

private:
    void stateChanged() {
        if (! notify_listeners_)
            return;

        for (auto& listenerCallback : callbacks_ | std::views::values)
            listenerCallback();

        if (notify_host_handler_)
            notify_host_handler_();
    }

    void syncAnalyzer() {
        analyzer_processor_.setNonRealtimeParameters(non_realtime_params_);
        timer_.stopTimer();
    }

    void asyncUpdateAnalyzer() {
        timer_.stopTimer();
        timer_.onTimerCallback() = [this] { syncAnalyzer(); };
        timer_.startTimer(120);
    }

    AnalyzerProcessor& analyzer_processor_;

    bool notify_listeners_ = true;
    std::function<void()> notify_host_handler_;
    std::map<size_t, std::function<void()>> callbacks_;

    AnalyzerProcessor::NonRealtimeParameters non_realtime_params_;
    bool hide_controls_ = false;
    EventTimer timer_;
};