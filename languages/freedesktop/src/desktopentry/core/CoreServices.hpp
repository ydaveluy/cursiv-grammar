// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "desktopentry/core/validation/DesktopEntryValidator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace desktopentry {

struct DesktopEntrySharedCoreServices : virtual cursiv::SharedCoreServices {};

struct DesktopEntryCoreServices
    : cursiv::CoreServicesFor<DesktopEntrySharedCoreServices> {
  std::unique_ptr<const DesktopEntryValidator> validator;

  explicit DesktopEntryCoreServices(
      DesktopEntrySharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(DesktopEntryCoreServices& services);

}  // namespace desktopentry
