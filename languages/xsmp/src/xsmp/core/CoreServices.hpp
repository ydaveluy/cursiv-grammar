#pragma once

#include "cursiv/core/services/CoreServicesFor.hpp"
#include "cursiv/core/services/SharedCoreServices.hpp"

// What this language owns beyond what every language owns, and the module that
// fills its container. The pair of types is empty: it exists so the container
// is built from the language's own shared type, and a service added later
// lands in a container that already has a home.

namespace xsmp {

struct XsmpSharedCoreServices : virtual cursiv::SharedCoreServices {};

struct XsmpCoreServices : cursiv::CoreServicesFor<XsmpSharedCoreServices> {
  explicit XsmpCoreServices(XsmpSharedCoreServices& sharedServices)
      : cursiv::CoreServices(sharedServices), CoreServicesFor(sharedServices) {}
};

void install_core_module(XsmpCoreServices& services);

}  // namespace xsmp
