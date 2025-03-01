
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

Plugin::Plugin(const clap_host *host) : ClapPlugin(&Plugin::descriptor, host)
{
}
