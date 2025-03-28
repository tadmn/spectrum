
#include "SpectrumPlugin.h"
#include "MainFrame.h"

#include <clap/helpers/plugin.hxx>

const clap_plugin_descriptor* SpectrumPlugin::getDescriptor() {
    static const char* features[] = { CLAP_PLUGIN_FEATURE_MONO, CLAP_PLUGIN_FEATURE_ANALYZER,
                                      "Free and Open Source", nullptr };

    static clap_plugin_descriptor desc = {
        CLAP_VERSION,
        "com.tadmn.spectrum",
        PRODUCT_NAME,
        "tadmn",
        "",
        "",
        "",
        PRODUCT_VERSION,
        "Buttery smooth audio spectrum analyzer",
        &features[0]};
    return &desc;
}

SpectrumPlugin::SpectrumPlugin(const clap_host* host) :
    ClapPlugin(getDescriptor(), host) { }

SpectrumPlugin::~SpectrumPlugin() = default;

bool SpectrumPlugin::activate(double sampleRate, uint32_t /*minFrameCount*/,
                              uint32_t /*maxFrameCount*/) noexcept {
    mAnalyzerProcessor.setSampleRate(sampleRate);
    return true;
}

void SpectrumPlugin::deactivate() noexcept { }

void SpectrumPlugin::reset() noexcept { }

clap_process_status SpectrumPlugin::process(const clap_process* process) noexcept {
    auto in = cb::createChannelArrayView(process->audio_inputs->data32,
                                         process->audio_inputs->channel_count, process->frames_count);

    mAnalyzerProcessor.process(in);

    // Hosts are allowed to out-of-place process even if we set `in_place_pair` in the port handling
    if (process->audio_inputs->data32 != process->audio_outputs->data32) {
        cb::copy(cb::createChannelArrayView(process->audio_outputs->data32,
                                            process->audio_outputs->channel_count, process->frames_count),
                 cb::createChannelArrayView(process->audio_inputs->data32,
                                            process->audio_inputs->channel_count, process->frames_count));
    }

    return CLAP_PROCESS_CONTINUE;
}

bool SpectrumPlugin::audioPortsInfo(uint32_t index, bool /*isInput*/, clap_audio_port_info* info) const noexcept {
    if (index != 0)
        return false;

    info->id = 0;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 1;
    info->in_place_pair = 0;
    info->port_type = CLAP_PORT_MONO;

    return true;
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

    if (mApp)
        return true;

    mApp = std::make_unique<visage::ApplicationWindow>();
    mApp->setWindowDimensions(120, 70);
    mApp->onDraw() = [this](visage::Canvas& c) {
        c.setColor(0xff000000);
        c.fill(0, 0, mApp->width(), mApp->height());
    };

    mApp->onWindowContentsResized() = [this] {
        _host.guiRequestResize(pluginWidth(), pluginHeight());
        mApp->children()[0]->setBounds(0, 0, mApp->width(), mApp->height());
    };

    mApp->addChild(std::make_unique<MainFrame>(mAnalyzerProcessor));

    return true;
}

void SpectrumPlugin::guiDestroy() noexcept {
#if __linux__
    if (mApp && mApp->window() && _host.canUsePosixFdSupport())
        _host.posixFdSupportUnregister(mApp->window()->posixFd());
#endif

    mApp->close();
}

bool SpectrumPlugin::guiSetParent(clap_window const* window) noexcept {
    if (mApp == nullptr)
        return false;

    mApp->show(window->ptr);

#if __linux__
    if (_host.canUsePosixFdSupport() && mApp->window()) {
        int fd_flags = CLAP_POSIX_FD_READ | CLAP_POSIX_FD_WRITE | CLAP_POSIX_FD_ERROR;
        return _host.posixFdSupportRegister(mApp->window()->posixFd(), fd_flags);
    }
#endif
    return true;
}

bool SpectrumPlugin::guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept {
    if (! mApp)
        return false;

    const bool fixed_aspect_ratio = mApp->isFixedAspectRatio();
    hints->can_resize_horizontally = true;
    hints->can_resize_vertically = true;
    hints->preserve_aspect_ratio = fixed_aspect_ratio;

    if (fixed_aspect_ratio) {
        hints->aspect_ratio_width = mApp->height() * mApp->aspectRatio();
        hints->aspect_ratio_height = mApp->width();
    }

    return true;
}

bool SpectrumPlugin::guiAdjustSize(uint32_t* width, uint32_t* height) noexcept {
    if (! mApp)
        return false;

    mApp->adjustWindowDimensions(width, height, true, true);
    return true;
}

bool SpectrumPlugin::guiSetSize(uint32_t width, uint32_t height) noexcept {
    if (! mApp)
        return false;

    setPluginDimensions(width, height);
    return true;
}

bool SpectrumPlugin::guiGetSize(uint32_t* width, uint32_t* height) noexcept {
    if (! mApp)
        return false;

    *width = pluginWidth();
    *height = pluginHeight();
    return true;
}

int SpectrumPlugin::pluginWidth() const {
    if (! mApp)
        return 0;

#if __APPLE__
    return mApp->width();
#else
    return mApp->nativeWidth();
#endif
}

int SpectrumPlugin::pluginHeight() const {
    if (! mApp)
        return 0;

#if __APPLE__
    return mApp->height();
#else
    return mApp->nativeHeight();
#endif
}

void SpectrumPlugin::setPluginDimensions(int width, int height) {
    if (! mApp)
        return;
#if __APPLE__
    mApp->setWindowDimensions(width, height);
#else
    mApp->setNativeWindowDimensions(width, height);
#endif
}