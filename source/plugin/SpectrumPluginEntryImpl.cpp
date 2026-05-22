
#include "SpectrumPluginEntryImpl.h"
#include "SpectrumPlugin.h"

#include <clap/clap.h>

namespace {

uint32_t getPluginCount(const clap_plugin_factory* /*f*/) { return 1; }

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

}

namespace tadmn::spectrum {

const void* getFactory(const char* factory_id) {
    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0) {
        static constexpr clap_plugin_factory clapFactory = {
            getPluginCount,
            getPluginDescriptor,
            createPlugin,
        };

        return &clapFactory;
    }

    return nullptr;
}

bool clapInit(const char* /*p*/) { return true; }
void clapDeinit() { }

}
