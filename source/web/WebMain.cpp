// Entry point for the browser build. Runs the plugin's analyzer and UI in a page, with audio handed
// over from JavaScript (see web/spectrum.js). Everything runs on the browser's main thread: audio
// blocks get pushed into the analyzers as they arrive, and each animation frame analyzes the latest
// audio, the same as the plugin's GUI does.

#include <emscripten.h>
#include <visage/app.h>

#include "AnalyzeAudio.h"
#include "AnalyzerProcessor.h"
#include "State.h"
#include "ui/MainFrame.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

namespace {

// The most frames that JavaScript hands over at once. Audio worklets deliver blocks of 128.
constexpr uint32_t k_max_block_frames = 4'096;
constexpr uint32_t k_max_channels = 2;

class WebApp {
  public:
    WebApp() : state_(analyzer_processors_, k_max_channels, [this] { saveState(); }) {
        mix_buffer_.resize({ .numChannels = 1, .numFrames = k_max_block_frames });
        loadState();

        window_.onResize() += [this] {
            for (auto* child : window_.children())
                child->setBounds(0, 0, window_.width(), window_.height());
        };

        auto top_frame = std::make_unique<TopFrame>(state_, analyzer_processors_);
        top_frame_ = top_frame.get();
        top_frame_->onFullScreenClicked([] { EM_ASM({ Module.spectrumToggleFullScreen(); }); });
        window_.addChild(std::move(top_frame));

        // The page's own controls hide along with the analyzer's
        state_listener_ = state_.addListener([this] { updatePageControls(); });
        updatePageControls();
    }

    // Never returns, as the browser drives the event loop from here on
    void run() {
        window_.showMaximized();
        window_.runEventLoop();
    }

    float* inputBuffer(int channel) { return input_buffers_[static_cast<size_t>(channel)].data(); }

    void setSampleRate(double sample_rate) { state_.setSampleRate(sample_rate); }

    // Analyzes the frames that JavaScript has copied into the input buffers
    void process(int channel_count, int frame_count) {
        const auto num_channels = static_cast<uint32_t>(std::clamp(channel_count, 1, static_cast<int>(k_max_channels)));
        const auto num_frames = static_cast<uint32_t>(std::clamp(frame_count, 0, static_cast<int>(k_max_block_frames)));

        // Follows the input device, as switching between a mono and a stereo microphone changes it
        if (num_channels != state_.channel_count())
            state_.setChannelCount(num_channels);

        std::array<float*, k_max_channels> channels {};
        for (uint32_t i = 0; i < num_channels; ++i)
            channels[i] = input_buffers_[i].data();

        analyzeAudio(choc::buffer::createChannelArrayView(channels.data(), num_channels, num_frames), state_,
                     analyzer_processors_, mix_buffer_);
    }

    void setFullScreen(bool full_screen) { top_frame_->setFullScreen(full_screen); }

  private:
    void updatePageControls() const {
        EM_ASM({ Module.spectrumSetControlsHidden($0 !== 0); }, state_.hide_controls() ? 1 : 0);
    }

    // Settings are kept in the browser, so that they're still there on the next visit
    void saveState() const {
        const auto json = state_.saveToJson();
        EM_ASM({
            try {
                localStorage.setItem('spectrum-state', UTF8ToString($0));
            } catch (e) {
            }
        }, json.c_str());
    }

    void loadState() {
        auto* json = static_cast<char*>(EM_ASM_PTR({
            try {
                const json = localStorage.getItem('spectrum-state');
                return json ? stringToNewUTF8(json) : 0;
            } catch (e) {
                return 0;
            }
        }));

        if (json == nullptr)
            return;

        state_.loadFromJson(json);
        std::free(json);
    }

    AnalyzerProcessors analyzer_processors_;
    State state_;

    std::array<std::array<float, k_max_block_frames>, k_max_channels> input_buffers_ {};
    choc::buffer::ChannelArrayBuffer<float> mix_buffer_;

    ApplicationWindow window_;
    TopFrame* top_frame_ = nullptr;

    std::unique_ptr<State::Listener> state_listener_;
};

// Lives for as long as the page does. It can't be a local in main(), as the browser's event loop
// unwinds main()'s stack without destroying anything on it, and it can't be constructed before main()
// either, as the window needs the runtime to be up
std::optional<WebApp> app;

} // namespace

// Called from JavaScript
extern "C" {

float* spectrumInputBuffer(int channel);
int spectrumMaxBlockFrames();
void spectrumSetSampleRate(double sample_rate);
void spectrumProcess(int channel_count, int frame_count);
void spectrumSetFullScreen(int full_screen);

EMSCRIPTEN_KEEPALIVE float* spectrumInputBuffer(int channel) {
    if (!app || channel < 0 || channel >= static_cast<int>(k_max_channels))
        return nullptr;

    return app->inputBuffer(channel);
}

EMSCRIPTEN_KEEPALIVE int spectrumMaxBlockFrames() { return static_cast<int>(k_max_block_frames); }

EMSCRIPTEN_KEEPALIVE void spectrumSetSampleRate(double sample_rate) {
    if (app && sample_rate > 0.0)
        app->setSampleRate(sample_rate);
}

EMSCRIPTEN_KEEPALIVE void spectrumProcess(int channel_count, int frame_count) {
    if (app)
        app->process(channel_count, frame_count);
}

EMSCRIPTEN_KEEPALIVE void spectrumSetFullScreen(int full_screen) {
    if (app)
        app->setFullScreen(full_screen != 0);
}

}

int main() {
    app.emplace();
    EM_ASM({ Module.spectrumReady(); });
    app->run();
    return 0;
}
