// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/services/CoreServicesFor.hpp"
#include "cursiv/services/SharedCoreServices.hpp"
#include "json5/validation/Json5Validator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace json5 {

struct Json5SharedCoreServices : virtual cursiv::SharedCoreServices {};

struct Json5CoreServices
    : cursiv::CoreServicesFor<Json5SharedCoreServices> {
  std::unique_ptr<const Json5Validator> validator;

  using CoreServicesFor::CoreServicesFor;

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(Json5CoreServices& services);

}  // namespace json5
