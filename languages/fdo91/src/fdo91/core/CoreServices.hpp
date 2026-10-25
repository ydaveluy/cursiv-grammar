// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "fdo91/core/validation/Fdo91Validator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace fdo91 {

struct Fdo91SharedCoreServices : virtual cursiv::SharedCoreServices {};

struct Fdo91CoreServices
    : cursiv::CoreServicesFor<Fdo91SharedCoreServices> {
  std::unique_ptr<const Fdo91Validator> validator;

  explicit Fdo91CoreServices(
      Fdo91SharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(Fdo91CoreServices& services);

}  // namespace fdo91
