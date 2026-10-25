#pragma once

#include <memory>
#include <string>

#include "cursiv/core/workspace/FileSystemProvider.hpp"
#include "ini/generated/INI/Module.hpp"
#include "ini/lsp/LspServices.hpp"

namespace ini {

void install_lsp_module(IniServices& services);

std::unique_ptr<IniServices> create_lsp_services(
    IniSharedServices& sharedServices,
    std::string languageId = std::string(ini::generated::INI::kLanguageId));

// The same one call, one storey up.
struct IniLanguage {
  std::unique_ptr<IniSharedServices> shared;
  IniServices& language;
};

IniLanguage create_lsp_services(
    std::unique_ptr<cursiv::FileSystemProvider> files = nullptr);

bool register_lsp_services(cursiv::SharedCoreServices& sharedServices);

}  // namespace ini
