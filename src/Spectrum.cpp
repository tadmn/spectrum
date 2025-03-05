
#include "Spectrum.h"
#include "MainGuiFrame.h"

#include <choc_DisableAllWarnings.h>
#include <clap/helpers/plugin.hxx>
#include <choc_ReenableAllWarnings.h>

clap_plugin_descriptor Spectrum::descriptor = {.clap_version = CLAP_VERSION,
                                               .id = "com.tadmn.spectrum",
                                               .name = "Spectrum",
                                               .vendor = "tadmn",
                                               .url = "",
                                               .manual_url = "",
                                               .support_url = "",
                                               .version = "0.0.0",
                                               .description = "Spectrum"};

Spectrum::Spectrum(const clap_host *host)
    : ClapPlugin(&Spectrum::descriptor, host), mFifoBuffer(1, kFftSize), mFft(kFftSize)
{
    mFftInBuffer.resize({.numChannels = 1, .numFrames = kFftSize});
    updateWindowingValues();
}

Spectrum::~Spectrum() = default;

bool Spectrum::activate(double /*sampleRate*/, uint32_t /*minFrameCount*/, uint32_t /*maxFrameCount*/) noexcept
{
    return true;
}

void Spectrum::deactivate() noexcept
{
}

void Spectrum::reset() noexcept
{
    mFifoBuffer.clear();
}

clap_process_status Spectrum::process(const clap_process *process) noexcept
{
    auto in = cb::createChannelArrayView(process->audio_inputs->data32, process->audio_inputs->channel_count,
                                         process->frames_count);

    while (in.getNumFrames() > 0) {
        in = mFifoBuffer.push(in);
        if (mFifoBuffer.isFull()) {
            // Make a copy of the accumulated samples, so that when we window them we don't affect the overlapping
            // samples in the next chunk.
            cb::copy(mFftInBuffer, mFifoBuffer.getBuffer());

            // Apply windowing.
            cb::applyGainPerFrame(mFftInBuffer, [this](auto i) { return mWindow[i]; });

            {
                RealtimeObject::ScopedAccess<farbot::ThreadType::realtime> fftOut(mFftComplexOutput);
                mFft.forward(mFftInBuffer.getIterator(0).sample, fftOut->data());
            }

            mFifoBuffer.pop(kFftHopSize);
        }
    }

    // Hosts are allowed to out-of-place process even if we set `in_place_pair` in the port handling.
    if (process->audio_inputs->data32 != process->audio_outputs->data32) {
        cb::copy(cb::createChannelArrayView(process->audio_outputs->data32, process->audio_outputs->channel_count,
                                            process->frames_count),
                 cb::createChannelArrayView(process->audio_inputs->data32, process->audio_inputs->channel_count,
                                            process->frames_count));
    }

    return CLAP_PROCESS_CONTINUE;
}

bool Spectrum::audioPortsInfo(uint32_t index, bool /*isInput*/, clap_audio_port_info *info) const noexcept
{
    if (index != 0)
        return false;

    info->id = 0;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = 1;
    info->in_place_pair = 0;

    return true;
}

bool Spectrum::guiIsApiSupported(char const *api, bool is_floating) noexcept
{
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

bool Spectrum::guiCreate(char const */*api*/, bool is_floating) noexcept
{
    if (is_floating)
        return false;

    if (mApp != nullptr)
        return true;

    mApp = std::make_unique<visage::ApplicationWindow>();
    mApp->setWindowDimensions(50, 50);

    mApp->onDraw() = [this](visage::Canvas& canvas) {
        canvas.setColor(0xff000000);
        canvas.fill(0, 0, mApp->width(), mApp->height());
    };

    mApp->onWindowContentsResized() = [this] {
        _host.guiRequestResize(static_cast<uint32_t>(mApp->logicalWidth()),
                               static_cast<uint32_t>(mApp->logicalHeight()));

        for (auto * child : mApp->children())
            child->setBounds(0, 0, mApp->width(), mApp->height());
    };

    mApp->addChild(std::make_unique<MainGuiFrame>(*this));

    return true;
}

void Spectrum::guiDestroy() noexcept
{
#if __linux__
    if (mApp && mApp->window() && _host.canUsePosixFdSupport())
        _host.posixFdSupportUnregister(mApp->window()->posixFd());
#endif

    mApp->close();
}

bool Spectrum::guiSetParent(clap_window const *window) noexcept
{
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

bool Spectrum::guiGetResizeHints(clap_gui_resize_hints_t *hints) noexcept
{
    if (mApp == nullptr)
        return false;

    bool fixed_aspect_ratio = mApp->isFixedAspectRatio();
    hints->can_resize_horizontally = true;
    hints->can_resize_vertically = true;
    hints->preserve_aspect_ratio = fixed_aspect_ratio;

    if (fixed_aspect_ratio) {
        hints->aspect_ratio_width  = static_cast<uint32_t>(mApp->height() * mApp->aspectRatio());
        hints->aspect_ratio_height = static_cast<uint32_t>(mApp->width());
    }
    return true;
}

bool Spectrum::guiAdjustSize(uint32_t *width, uint32_t *height) noexcept
{
    if (mApp == nullptr)
        return false;

    mApp->adjustWindowDimensions(width, height, true, true);
    return true;
}

bool Spectrum::guiSetSize(uint32_t width, uint32_t height) noexcept
{
    if (mApp == nullptr)
        return false;

    mApp->setWindowDimensions(static_cast<int>(width), static_cast<int>(height));
    return true;
}

bool Spectrum::guiGetSize(uint32_t *width, uint32_t *height) noexcept
{
    if (mApp == nullptr)
        return false;

    *width = static_cast<uint32_t>(mApp->logicalWidth());
    *height = static_cast<uint32_t>(mApp->logicalHeight());
    return true;
}

void Spectrum::updateWindowingValues()
{
    // Hann window.
    for (size_t i = 0; i < mWindow.size(); ++i) {
        mWindow[i] = static_cast<float>(0.5 - std::cos((2.0 * i * M_PI) / (mWindow.size() - 1)) / 2);
    }
}
