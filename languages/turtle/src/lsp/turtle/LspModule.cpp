#include "turtle/LspModule.hpp"

#include <memory>
#include <utility>

#include "cursiv/services/CoreServices.hpp"
#include "cursiv/services/ServiceRegistry.hpp"
#include "cursiv/services/LspServices.hpp"
#include "turtle/CoreServices.hpp"

namespace turtle {

void install_lsp_module(TurtleServices& services) {
  cursiv::install_default_lsp_services(services);
}

// The three storeys in order: the framework's defaults, the language's
// core module, then its LSP module. The container is one object -- both
// bases derive `CoreServices` virtually -- so the providers read what the
// core module wired.
bool register_lsp_services(cursiv::SharedCoreServices& sharedServices) {
  auto& shared =
      dynamic_cast<TurtleSharedServices&>(sharedServices);
  auto services = std::make_unique<TurtleServices>(shared);
  cursiv::install_default_core_services(*services);
  install_core_module(*services);
  install_lsp_module(*services);
  shared.serviceRegistry->registerServices(std::move(services));
  return true;
}

}  // namespace turtle
