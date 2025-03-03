
#include "Plugin.h"

#include <choc_DisableAllWarnings.h>
#include <clap/helpers/plugin.hxx>
#include <choc_ReenableAllWarnings.h>

clap_plugin_descriptor Plugin::descriptor = {.clap_version = CLAP_VERSION,
                                               .id = "com.tadmn.spectrum",
                                               .name = "Spectrum",
                                               .vendor = "tadmn",
                                               .url = "",
                                               .manual_url = "",
                                               .support_url = "",
                                               .version = "0.0.0",
                                               .description = "Spectrum"};

Plugin::Plugin(const clap_host *host) : ClapPlugin(&Plugin::descriptor, host), mFft(kFftSize)
{
}

Plugin::~Plugin() = default;

bool Plugin::activate(double /*sampleRate*/, uint32_t /*minFrameCount*/, uint32_t /*maxFrameCount*/) noexcept
{
    return true;
}

void Plugin::deactivate() noexcept
{
}

clap_process_status Plugin::process(const clap_process */*process*/) noexcept
{
    std::array<float, 1024> in;

    {
        RealtimeObject::ScopedAccess<farbot::ThreadType::realtime> fftOut(mFftComplexOutput);
        mFft.forward(in.data(), fftOut->data());
    }

    return CLAP_PROCESS_CONTINUE;
}

bool Plugin::guiIsApiSupported(char const *api, bool is_floating) noexcept
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

bool Plugin::guiCreate(char const */*api*/, bool is_floating) noexcept
{
    if (is_floating)
        return false;

    if (mApp != nullptr)
        return true;

    mApp = std::make_unique<visage::ApplicationWindow>();
    mApp->setWindowDimensions(50, 50);

    mApp->onDraw() = [this](visage::Canvas& canvas) {
        canvas.setColor(0xff000066);
        canvas.fill(0, 0, mApp->width(), mApp->height());

        float circle_radius = mApp->height() * 0.1f;
        float x = mApp->width() * 0.5f - circle_radius;
        float y = mApp->height() * 0.5f - circle_radius;
        canvas.setColor(0xff00ffff);
        canvas.circle(x, y, 2.0f * circle_radius);
    };

    mApp->onWindowContentsResized() = [this] {
        _host.guiRequestResize(static_cast<uint32_t>(mApp->logicalWidth()),
                               static_cast<uint32_t>(mApp->logicalHeight()));
    };

    return true;
}

void Plugin::guiDestroy() noexcept
{
#if __linux__
    if (mApp && mApp->window() && _host.canUsePosixFdSupport())
        _host.posixFdSupportUnregister(mApp->window()->posixFd());
#endif

    mApp->close();
}

bool Plugin::guiSetParent(clap_window const *window) noexcept
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

bool Plugin::guiGetResizeHints(clap_gui_resize_hints_t *hints) noexcept
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

bool Plugin::guiAdjustSize(uint32_t *width, uint32_t *height) noexcept
{
    if (mApp == nullptr)
        return false;

    mApp->adjustWindowDimensions(width, height, true, true);
    return true;
}

bool Plugin::guiSetSize(uint32_t width, uint32_t height) noexcept
{
    if (mApp == nullptr)
        return false;

    mApp->setWindowDimensions(static_cast<int>(width), static_cast<int>(height));
    return true;
}

bool Plugin::guiGetSize(uint32_t *width, uint32_t *height) noexcept
{
    if (mApp == nullptr)
        return false;

    *width  = static_cast<uint32_t>(mApp->logicalWidth());
    *height = static_cast<uint32_t>(mApp->logicalHeight());
    return true;
}
