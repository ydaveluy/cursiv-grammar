#pragma once

#include <memory>

#include "cursiv/services/CoreServicesFor.hpp"
#include "cursiv/services/SharedCoreServices.hpp"
#include "ini/IniReader.hpp"

namespace ini {

struct IniSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct IniCoreServices : cursiv::CoreServicesFor<IniSharedCoreServices> {
  std::unique_ptr<const IniReader> reader;

  using CoreServicesFor::CoreServicesFor;

  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    cursiv::CoreServices::visitServices(yield);
    yield({"reader", true, reader != nullptr});
  }
};

void install_core_module(IniCoreServices& services);

}  // namespace ini
