// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/services/CoreServicesFor.hpp"
#include "cursiv/services/SharedCoreServices.hpp"
#include "sieve/validation/SieveValidator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace sieve {

struct SieveSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct SieveCoreServices
    : cursiv::CoreServicesFor<SieveSharedCoreServices> {
  std::unique_ptr<const SieveValidator> validator;

  using CoreServicesFor::CoreServicesFor;

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(SieveCoreServices& services);

}  // namespace sieve
