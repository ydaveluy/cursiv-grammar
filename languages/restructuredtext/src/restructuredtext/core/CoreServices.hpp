// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "restructuredtext/core/validation/RestructuredtextValidator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace restructuredtext {

struct RestructuredtextSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct RestructuredtextCoreServices
    : cursiv::CoreServicesFor<RestructuredtextSharedCoreServices> {
  std::unique_ptr<const RestructuredtextValidator> validator;

  explicit RestructuredtextCoreServices(
      RestructuredtextSharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(RestructuredtextCoreServices& services);

}  // namespace restructuredtext
