
#include "SpectrumPluginEntryImpl.h"
#include "SpectrumPlugin.h"

#include <clap/clap.h>
#include <clapwrapper/vst3.h>

namespace {

uint32_t getPluginCount(const clap_plugin_factory* /*f*/) {
    return 1;
}

const clap_plugin_descriptor* getPluginDescriptor(const clap_plugin_factory* /*f*/, uint32_t /*w*/) {
    return SpectrumPlugin::getDescriptor();
}

const clap_plugin* createPlugin(const clap_plugin_factory* /*factory*/, const clap_host* host,
                                const char* plugin_id) {
    if (strcmp(plugin_id, SpectrumPlugin::getDescriptor()->id) != 0)
        return nullptr;

    auto p = new SpectrumPlugin(host);
    return p->clapPlugin();
}

const clap_plugin_info_as_vst3* clapGetVst3Info(const clap_plugin_factory_as_vst3* /*f*/, uint32_t /*index*/) {
    return nullptr;
}

}

namespace tadmn::spectrum {

const void* getFactory(const char* factory_id) {
    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
        static constexpr clap_plugin_factory clapPluginFactory = {
            getPluginCount,
            getPluginDescriptor,
            createPlugin,
        };

        return &clapPluginFactory;
    }

    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_INFO_VST3) == 0) {
        static const struct clap_plugin_factory_as_vst3 vst3Factory = {
            "tadmn", "", "", clapGetVst3Info};

        return &vst3Factory;
    }

    return nullptr;
}

bool clapInit(const char* /*p*/) { return true; }
void clapDeinit() { }

}
