// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "unrealangelscript/core/validation/UnrealAngelscriptValidator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace unrealangelscript {

struct UnrealAngelscriptSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct UnrealAngelscriptCoreServices
    : cursiv::CoreServicesFor<UnrealAngelscriptSharedCoreServices> {
  std::unique_ptr<const UnrealAngelscriptValidator> validator;

  explicit UnrealAngelscriptCoreServices(
      UnrealAngelscriptSharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(UnrealAngelscriptCoreServices& services);

}  // namespace unrealangelscript
