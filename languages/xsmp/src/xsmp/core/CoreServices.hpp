#pragma once

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"
#include "xsmp/core/validation/XsmpValidator.hpp"
#include <memory>

// The language adds its local validator to the default shared/core services.

namespace xsmp {

struct XsmpSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct XsmpCoreServices : cursiv::CoreServicesFor<XsmpSharedCoreServices> {
  std::unique_ptr<const XsmpValidator> validator;
  explicit XsmpCoreServices(XsmpSharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}
};

void install_core_module(XsmpCoreServices& services);

}  // namespace xsmp
