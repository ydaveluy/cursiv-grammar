// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "angelscript/core/validation/AngelscriptValidator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace angelscript {

struct AngelscriptSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct AngelscriptCoreServices
    : cursiv::CoreServicesFor<AngelscriptSharedCoreServices> {
  std::unique_ptr<const AngelscriptValidator> validator;

  explicit AngelscriptCoreServices(
      AngelscriptSharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(AngelscriptCoreServices& services);

}  // namespace angelscript
