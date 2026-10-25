#include "ini/lsp/LspModule.hpp"

#include <memory>
#include <utility>

#include "cursiv/core/services/ServiceRegistry.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "cursiv/core/workspace/EmptyFileSystemProvider.hpp"
#include "cursiv/core/workspace/FileSystemProvider.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "ini/core/CoreServices.hpp"

namespace ini {

void install_lsp_module(IniServices& services) {
  cursiv::install_default_lsp_services(services);
}

std::unique_ptr<IniServices> create_lsp_services(
    IniSharedServices& sharedServices, std::string languageId) {
  auto services = std::make_unique<IniServices>(sharedServices);
  services->languageMetaData.languageId = std::move(languageId);
  cursiv::install_default_core_services(*services);
  install_core_module(*services);
  install_lsp_module(*services);
  return services;
}

IniLanguage create_lsp_services(
    std::unique_ptr<cursiv::FileSystemProvider> files) {
  auto shared = std::make_unique<IniSharedServices>();
  shared->fileSystemProvider =
      files != nullptr ? std::move(files)
                       : std::make_unique<cursiv::EmptyFileSystemProvider>();
  cursiv::install_default_shared_lsp_services(*shared);
  auto services = create_lsp_services(*shared);
  IniServices& language = *services;
  shared->serviceRegistry->registerServices(std::move(services));
  return {std::move(shared), language};
}

bool register_lsp_services(cursiv::SharedCoreServices& sharedServices) {
  sharedServices.serviceRegistry->registerServices(
      create_lsp_services(dynamic_cast<IniSharedServices&>(sharedServices)));
  return true;
}

}  // namespace ini
