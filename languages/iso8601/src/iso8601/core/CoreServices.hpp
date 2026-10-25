// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "iso8601/core/validation/Iso8601Validator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace iso8601 {

struct Iso8601SharedCoreServices : virtual cursiv::SharedCoreServices {};

struct Iso8601CoreServices
    : cursiv::CoreServicesFor<Iso8601SharedCoreServices> {
  std::unique_ptr<const Iso8601Validator> validator;

  explicit Iso8601CoreServices(
      Iso8601SharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(Iso8601CoreServices& services);

}  // namespace iso8601
