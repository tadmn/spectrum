
#pragma once

#include <choc_DisableAllWarnings.h>
#include <clap/helpers/plugin.hh>
#include <choc_ReenableAllWarnings.h>

#include <FastFourier/FastFourier.h>
#include <farbot/RealtimeObject.hpp>

#include <visage/app.h>

using ClapPlugin =
    clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>;

class Plugin : public ClapPlugin
{
  public:
    static clap_plugin_descriptor descriptor;

    explicit Plugin(const clap_host *host);
    ~Plugin() override;

protected:
    bool activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept override;
    void deactivate() noexcept override;

    clap_process_status process(const clap_process *process) noexcept override;

    bool implementsGui() const noexcept override { return true; }
    bool guiIsApiSupported(const char* api, bool is_floating) noexcept override;
    bool guiCreate(const char* api, bool is_floating) noexcept override;
    void guiDestroy() noexcept override;
    bool guiSetParent(const clap_window* window) noexcept override;
    bool guiSetScale(double /*scale*/) noexcept override { return false; }
    bool guiCanResize() const noexcept override { return true; }
    bool guiGetResizeHints(clap_gui_resize_hints_t* hints) noexcept override;
    bool guiAdjustSize(uint32_t* width, uint32_t* height) noexcept override;
    bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
    bool guiGetSize(uint32_t* width, uint32_t* height) noexcept override;

private:
    static constexpr int kFftSize = 1024;
    FastFourier mFft;
    using FftComplexOutput = std::array<std::complex<float>, kFftSize / 2 + 1>;
    using RealtimeObject = farbot::RealtimeObject<FftComplexOutput, farbot::RealtimeObjectOptions::realtimeMutatable>;
    RealtimeObject mFftComplexOutput;

    std::unique_ptr<visage::ApplicationWindow> mApp;
};
