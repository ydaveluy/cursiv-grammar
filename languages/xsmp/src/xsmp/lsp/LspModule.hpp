#pragma once

#include <memory>
#include <string>

#include "cursiv/core/workspace/FileSystemProvider.hpp"
#include "xsmp/generated/Xsmp/Module.hpp"
#include "xsmp/lsp/LspServices.hpp"

namespace xsmp {

void install_lsp_module(XsmpServices& services);

std::unique_ptr<XsmpServices> create_lsp_services(
    XsmpSharedServices& sharedServices,
    std::string languageId = std::string(xsmp::generated::Xsmp::kLanguageId));

// The same one call, one storey up.
struct XsmpLanguage {
  std::unique_ptr<XsmpSharedServices> shared;
  XsmpServices& language;
};

XsmpLanguage create_lsp_services(
    std::unique_ptr<cursiv::FileSystemProvider> files = nullptr);

bool register_lsp_services(cursiv::SharedCoreServices& sharedServices);

}  // namespace xsmp
