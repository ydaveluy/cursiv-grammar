// Type modifiers are parsed as a list and checked after AST construction.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "acme/ast.hpp"

namespace acme {

class AcmeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  template<class T> void checkModifiers(const T&, cursiv::ValidationAcceptor&) const;

};

struct AcmeCoreServices;
void register_validation_checks(AcmeCoreServices& services);

}  // namespace acme
