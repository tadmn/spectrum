
#pragma once

#include <choc_DisableAllWarnings.h>
#include <clap/helpers/plugin.hh>
#include <choc_ReenableAllWarnings.h>

using ClapPlugin =
    clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>;

class Plugin : public ClapPlugin
{
  public:
    static clap_plugin_descriptor descriptor;

    explicit Plugin(const clap_host *host);
};
