
#include "SpectrumPluginEntryImpl.h"

#include "SpectrumPlugin.h"

#include <clap/clap.h>
#include <clapwrapper/auv2.h>
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

const clap_plugin_info_as_vst3* getVst3Info(const clap_plugin_factory_as_vst3* /*f*/, uint32_t /*index*/) {
    return nullptr;
}

bool getAuV2Info(const clap_plugin_factory_as_auv2* /*factory*/, uint32_t index,
                        clap_plugin_info_as_auv2_t* info) {
    if (index >= 1)
        return false;

    strncpy(info->au_type, "aufx", 5);
    strncpy(info->au_subt, "Spct", 5);

    return true;
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

    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_INFO_VST3) == 0) {
        static const struct clap_plugin_factory_as_vst3 vst3Factory = { "tadmn", "", "", getVst3Info };

        return &vst3Factory;
    }

    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_INFO_AUV2) == 0) {
        static const clap_plugin_factory_as_auv2 auV2Factory = { "tadN",  // manu
                                                                 "tadmn", // manu name
                                                                 getAuV2Info };
        return &auV2Factory;
    }

    return nullptr;
}

bool clapInit(const char* /*p*/) {
    return true;
}

void clapDeinit() { }

}
