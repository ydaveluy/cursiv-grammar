#include "iso8601/LspModule.hpp"

#include <memory>
#include <utility>

#include "cursiv/services/CoreServices.hpp"
#include "cursiv/services/ServiceRegistry.hpp"
#include "cursiv/services/LspServices.hpp"
#include "iso8601/CoreServices.hpp"

namespace iso8601 {

void install_lsp_module(Iso8601Services& services) {
  cursiv::install_default_lsp_services(services);
}

// The three storeys in order: the framework's defaults, the language's
// core module, then its LSP module. The container is one object -- both
// bases derive `CoreServices` virtually -- so the providers read what the
// core module wired.
bool register_lsp_services(cursiv::SharedCoreServices& sharedServices) {
  auto& shared =
      dynamic_cast<Iso8601SharedServices&>(sharedServices);
  auto services = std::make_unique<Iso8601Services>(shared);
  cursiv::install_default_core_services(*services);
  install_core_module(*services);
  install_lsp_module(*services);
  shared.serviceRegistry->registerServices(std::move(services));
  return true;
}

}  // namespace iso8601
