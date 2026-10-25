#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "upnp/core/ast.hpp"
namespace upnp {
class UpnpValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkComparison(const ast::Comparison &,
                       cursiv::ValidationAcceptor &) const;
};
struct UpnpCoreServices;
void register_validation_checks(UpnpCoreServices &services);
} // namespace upnp
