// What this language owns beyond what every language owns, and the module
// that fills its container.
#pragma once

#include <memory>

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "evmbytecode/core/validation/EvmBytecodeValidator.hpp"

// Two containers per language: the shared one holds what is workspace-wide
// (the document store, the index), the other holds one language's
// services. Add a service of your own here, filled in `install_core_module`.

namespace evmbytecode {

struct EvmBytecodeSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct EvmBytecodeCoreServices
    : cursiv::CoreServicesFor<EvmBytecodeSharedCoreServices> {
  std::unique_ptr<const EvmBytecodeValidator> validator;

  explicit EvmBytecodeCoreServices(
      EvmBytecodeSharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}

  // One row per service, for whoever asks what a container holds: the
  // language's own rows come after the defaults.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"validator", true, validator != nullptr});
  }
};

void install_core_module(EvmBytecodeCoreServices& services);

}  // namespace evmbytecode
