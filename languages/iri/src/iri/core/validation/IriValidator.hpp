// The checks of this language.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "iri/core/ast.hpp"

namespace iri {

class IriValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkAuthority(const ast::Authority&, cursiv::ValidationAcceptor&) const;
};

struct IriCoreServices;
void register_validation_checks(IriCoreServices& services);

}  // namespace iri
