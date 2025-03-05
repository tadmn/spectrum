
#include "Spectrum.h"

namespace
{

uint32_t getPluginCount(const clap_plugin_factory * /*f*/)
{
    return 1;
}

const clap_plugin_descriptor *getPluginDescriptor(const clap_plugin_factory * /*f*/, uint32_t /*w*/)
{
    return &Spectrum::descriptor;
}

const clap_plugin *createPlugin(const clap_plugin_factory * /*factory*/, const clap_host *host, const char *plugin_id)
{
    if (strcmp(plugin_id, Spectrum::descriptor.id) != 0)
        return nullptr;

    auto p = new Spectrum(host);
    return p->clapPlugin();
}

const void *getFactory(const char *factory_id)
{
    static constexpr clap_plugin_factory va_clap_plugin_factory = {
        getPluginCount,
        getPluginDescriptor,
        createPlugin,
    };
    return (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID)) ? &va_clap_plugin_factory : nullptr;
}

bool clapInit(const char * /*p*/)
{
    return true;
}
void clapDeinit()
{
}

} // namespace

extern "C"
{
    extern const CLAP_EXPORT clap_plugin_entry clap_entry = {CLAP_VERSION, clapInit, clapDeinit, getFactory};
}