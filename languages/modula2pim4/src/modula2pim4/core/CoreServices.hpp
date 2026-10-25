// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "modula2pim4/core/validation/Modula2pim4Validator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace modula2pim4 {

struct Modula2pim4SharedCoreServices : virtual cursiv::SharedCoreServices {};

struct Modula2pim4CoreServices
    : cursiv::CoreServicesFor<Modula2pim4SharedCoreServices> {
  std::unique_ptr<const Modula2pim4Validator> validator;

  explicit Modula2pim4CoreServices(
      Modula2pim4SharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(Modula2pim4CoreServices& services);

}  // namespace modula2pim4
