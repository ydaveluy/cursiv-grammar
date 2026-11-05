#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "rfc1035/ast.hpp"

namespace rfc1035 {

class Rfc1035Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDomain(const ast::Domain& domain,
                   cursiv::ValidationAcceptor& accept) const;
};

struct Rfc1035CoreServices;
void register_validation_checks(Rfc1035CoreServices& services);

}  // namespace rfc1035
