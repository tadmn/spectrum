
#pragma once

#include "AnalyzerProcessor.h"

#include <choc_SampleBuffers.h>
#include <clap/helpers/plugin.hh>
#include <visage/app.h>

#if NDEBUG
using ClapPlugin = clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Ignore, clap::helpers::CheckingLevel::Minimal>;
#else
using ClapPlugin = clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>;
#endif

namespace cb = choc::buffer;

class SpectrumPlugin : public ClapPlugin {
  public:
    static const clap_plugin_descriptor* getDescriptor();

    explicit SpectrumPlugin(const clap_host* host);
    ~SpectrumPlugin() override;

  protected:
    bool activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept override;
    void deactivate() noexcept override;
    void reset() noexcept override;

    clap_process_status process(const clap_process* process) noexcept override;

    bool implementsAudioPorts() const noexcept override { return true; }

    uint32_t audioPortsCount(bool /*isInput*/) const noexcept override { return 1; }

    bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info* info) const noexcept override;

    bool implementsState() const noexcept override { return true; }
    bool stateSave(const clap_ostream* stream) noexcept override;
    bool stateLoad(const clap_istream* stream) noexcept override;

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
    int pluginWidth() const;
    int pluginHeight() const;
    void setPluginDimensions(int width, int height);

    choc::buffer::ChannelArrayBuffer<float> mStereoMixBuffer;
    AnalyzerProcessor mAnalyzerProcessor;

    std::unique_ptr<visage::ApplicationWindow> mApp;
};
