
#include "SpectrumPluginEntryImpl.h"

#include <clap/clap.h>

extern "C" {
    extern const CLAP_EXPORT clap_plugin_entry clap_entry = {
        CLAP_VERSION,
        tadmn::spectrum::clapInit,
        tadmn::spectrum::clapDeinit,
        tadmn::spectrum::getFactory
    };
}