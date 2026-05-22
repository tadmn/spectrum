
#include "SpectrumPlugin.h"

#include "ui/MainFrame.h"

#include <tb_Core.h>
#include <clap/helpers/plugin.hxx>

const clap_plugin_descriptor* SpectrumPlugin::getDescriptor() {
    static const char* features[] = { CLAP_PLUGIN_FEATURE_STEREO, CLAP_PLUGIN_FEATURE_ANALYZER,
                                      "Free and Open Source", nullptr };

    static clap_plugin_descriptor desc = { CLAP_VERSION,
                                           "com.tadmn.spectrum",
                                           PLUGIN_NAME,
                                           "tadmn",
                                           "",
                                           "",
                                           "",
                                           PRODUCT_VERSION,
                                           "Buttery smooth audio spectrum analyzer",
                                           &features[0] };
    return &desc;
}

SpectrumPlugin::SpectrumPlugin(const clap_host* host) : ClapPlugin(getDescriptor(), host)
    , state_(analyzer_processor_, [this]{ _host.stateMarkDirty(); }) {}

SpectrumPlugin::~SpectrumPlugin() = default;

#ifdef __linux__
void SpectrumPlugin::onPosixFd(int fd, clap_posix_fd_flags_t flags) noexcept {
    if (gui_window_ && gui_window_->window())
        gui_window_->window()->processPluginFdEvents();
}
#endif

bool SpectrumPlugin::activate(double sampleRate, uint32_t /*minFrames*/, uint32_t maxFrames) noexcept {
    stereo_mix_buffer_.resize({.numChannels = 1, .numFrames = maxFrames});
    state_.setSampleRate(sampleRate);
    return true;
}

void SpectrumPlugin::deactivate() noexcept { }

void SpectrumPlugin::reset() noexcept { }

clap_process_status SpectrumPlugin::process(const clap_process* process) noexcept {
    auto in = cb::createChannelArrayView(process->audio_inputs->data32,
                                         process->audio_inputs->channel_count, process->frames_count);

    if (in.getNumChannels() != 2) {
        tb_assert(false); // Unsupported channel count
        return CLAP_PROCESS_ERROR;
    }

    {
        // Average the two input channels into a single buffer to be processed
        tb_assert(stereo_mix_buffer_.getNumFrames() >= in.getNumFrames());
        auto mix = stereo_mix_buffer_.getStart(in.getNumFrames());
        copy(mix, in.getChannel(0));
        add(mix, in.getChannel(1));
        applyGain(mix, 0.5f);
        analyzer_processor_.processAudio(mix);
    }

    // Hosts are allowed to out-of-place process even if we set `in_place_pair` in the port handling
    if (process->audio_inputs->data32 != process->audio_outputs->data32) {
        copy(cb::createChannelArrayView(process->audio_outputs->data32,
                                        process->audio_outputs->channel_count, process->frames_count),
             cb::createChannelArrayView(process->audio_inputs->data32,
                                        process->audio_inputs->channel_count, process->frames_count));
    }

    return CLAP_PROCESS_CONTINUE;
}

bool SpectrumPlugin::audioPortsInfo(uint32_t index, bool /*isInput*/,
                                    clap_audio_port_info* info) const noexcept {
    if (index != 0)
        return false;

    strncpy(info->name, "Main Input", sizeof(info->name));
    info->id = 0;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 2;
    info->in_place_pair = 0;
    info->port_type = CLAP_PORT_STEREO;

    return true;
}

bool SpectrumPlugin::stateSave(const clap_ostream* stream) noexcept {
    if (! stream || ! stream->write)
        return false;

    const auto json = state_.saveToJson();

    // CLAP streams may have size limitations, so we need to write in chunks
    const auto* buffer = json.data();

    auto remaining = json.size();
    while (remaining > 0) {
        // Try to write remaining bytes
        const auto written = stream->write(stream, buffer + (json.size() - remaining), remaining);

        if (written < 0)
            return false;  // Write error occurred

        remaining -= written;
    }

    return true;
}

bool SpectrumPlugin::stateLoad(const clap_istream* stream) noexcept {
    if (! stream || ! stream->read)
        return false;

    // Read the JSON string from the stream in chunks
    constexpr auto chunkSize = 4096;
    std::vector<char> buffer;
    char chunk[chunkSize];

    while (true) {
        int64_t bytesRead = stream->read(stream, chunk, chunkSize);

        if (bytesRead < 0)
            return false;  // Read error

        if (bytesRead == 0)
            break;  // End of stream

        buffer.insert(buffer.end(), chunk, chunk + bytesRead);
    }

    buffer.push_back('\0');  // Ensure buffer is null-terminated

    return state_.loadFromJson({ buffer.data(), buffer.size() });
}

bool SpectrumPlugin::guiIsApiSupported(char const* api, bool is_floating) noexcept {
    if (is_floating)
        return false;

#ifdef _WIN32
    if (strcmp(api, CLAP_WINDOW_API_WIN32) == 0)
        return true;
#elif __APPLE__
    if (strcmp(api, CLAP_WINDOW_API_COCOA) == 0)
        return true;
#elif __linux__
    if (strcmp(api, CLAP_WINDOW_API_X11) == 0)
        return true;
#endif

    return false;
}

bool SpectrumPlugin::guiCreate(char const* /*api*/, bool is_floating) noexcept {
    if (is_floating)
        return false;

    if (gui_window_)
        return true;

    const tb::ScopedSetter ss(notify_host_of_resize_, false);

    gui_window_ = std::make_unique<ApplicationWindow>();
    gui_window_->setMinimumDimensions(k_min_width, k_min_height);
    gui_window_->onResize() += [this] {
        for (auto* child : gui_window_->children())
            child->setBounds(0, 0, gui_window_->width(), gui_window_->height());
    };

    gui_window_->onWindowContentsResized() += [this] {
        if (notify_host_of_resize_)
            _host.guiRequestResize(pluginWidth(), pluginHeight());
    };

    gui_window_->addChild(std::make_unique<TopFrame>(state_, analyzer_processor_));

    // Default dimensions
    gui_window_->setWindowDimensions(k_default_height, k_default_width);

    return true;
}

void SpectrumPlugin::guiDestroy() noexcept {
#if __linux__
    if (gui_window_ && gui_window_->window() && _host.canUsePosixFdSupport())
        _host.posixFdSupportUnregister(gui_window_->window()->posixFd());
#endif

    gui_window_->close();
}

bool SpectrumPlugin::guiSetParent(clap_window const* window) noexcept {
    if (gui_window_ == nullptr)
        return false;

    gui_window_->show(window->ptr);

#if __linux__
    if (_host.canUsePosixFdSupport() && gui_window_->window()) {
        int fd_flags = CLAP_POSIX_FD_READ | CLAP_POSIX_FD_WRITE | CLAP_POSIX_FD_ERROR;
        return _host.posixFdSupportRegister(gui_window_->window()->posixFd(), fd_flags);
    }
#endif
    return true;
}

bool SpectrumPlugin::guiSetScale(double scale) noexcept
{
    if (! gui_window_)
        return false;

    gui_window_->setDpiScale(static_cast<float>(scale));

    return true;
}

bool SpectrumPlugin::guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept {
    if (! gui_window_)
        return false;

    const bool fixed_aspect_ratio = gui_window_->isFixedAspectRatio();
    tb_assert(! fixed_aspect_ratio);

    hints->can_resize_horizontally = true;
    hints->can_resize_vertically = true;
    hints->preserve_aspect_ratio = fixed_aspect_ratio;

    return true;
}

// This just tests the requested dimensions, doesn't actually set the window size
bool SpectrumPlugin::guiAdjustSize(uint32_t* width, uint32_t* height) noexcept {
    if (! gui_window_)
        return false;

    tb_assert(notify_host_of_resize_);
    const tb::ScopedSetter ss(notify_host_of_resize_, false);

    // This handles the minimums
    gui_window_->adjustWindowDimensions(width, height, true, true);

    // This handles the maximums. Maximums really only exist to prevent bugs / crashes when
    // CLI tests like pluginval set enormous sizes
    *width = std::min(*width, static_cast<uint32_t>(k_max_width * gui_window_->dpiScale()));
    *height = std::min(*height, static_cast<uint32_t>(k_max_height * gui_window_->dpiScale()));

    return true;
}

bool SpectrumPlugin::guiSetSize(uint32_t width, uint32_t height) noexcept {
    if (! gui_window_)
        return false;

    tb_assert(notify_host_of_resize_);
    const tb::ScopedSetter ss(notify_host_of_resize_, false);

    // These are really here for edge cases where hosts & testers (such as pluginval) don't call
    // `guiAdjustSize` first to check
    const auto dpi_scale = gui_window_->dpiScale();
    width = std::clamp(width, static_cast<uint32_t>(k_min_width * dpi_scale), static_cast<uint32_t>(k_max_width * dpi_scale));
    height = std::clamp(height, static_cast<uint32_t>(k_min_height * dpi_scale), static_cast<uint32_t>(k_max_height * dpi_scale));

#if __APPLE__
    gui_window_->setWindowDimensions(width, height);
#else
    gui_window_->setNativeWindowDimensions(width, height);
#endif

    return true;
}

bool SpectrumPlugin::guiGetSize(uint32_t* width, uint32_t* height) noexcept {
    if (! gui_window_)
        return false;

    *width = pluginWidth();
    *height = pluginHeight();
    return true;
}

int SpectrumPlugin::pluginWidth() const {
    if (! gui_window_)
        return 0;

#if __APPLE__
    return gui_window_->width();
#else
    return gui_window_->nativeWidth();
#endif
}

int SpectrumPlugin::pluginHeight() const {
    if (! gui_window_)
        return 0;

#if __APPLE__
    return gui_window_->height();
#else
    return gui_window_->nativeHeight();
#endif
}