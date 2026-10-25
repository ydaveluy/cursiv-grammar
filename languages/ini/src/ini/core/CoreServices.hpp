#pragma once

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "ini/core/IniReader.hpp"


namespace ini {

struct IniSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct IniCoreServices : cursiv::CoreServicesFor<IniSharedCoreServices> {
  std::unique_ptr<const IniReader> reader;

  explicit IniCoreServices(IniSharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}
};

void install_core_module(IniCoreServices& services);

}  // namespace ini
