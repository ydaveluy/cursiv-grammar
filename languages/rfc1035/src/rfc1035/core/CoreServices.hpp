// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "rfc1035/core/validation/Rfc1035Validator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace rfc1035 {

struct Rfc1035SharedCoreServices : virtual cursiv::SharedCoreServices {};

struct Rfc1035CoreServices
    : cursiv::CoreServicesFor<Rfc1035SharedCoreServices> {
  std::unique_ptr<const Rfc1035Validator> validator;

  explicit Rfc1035CoreServices(
      Rfc1035SharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(Rfc1035CoreServices& services);

}  // namespace rfc1035
