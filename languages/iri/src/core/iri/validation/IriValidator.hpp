// The checks of this language.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "iri/ast.hpp"

namespace iri {

class IriValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkAuthority(const ast::Authority&, cursiv::ValidationAcceptor&) const;
};

struct IriCoreServices;
void register_validation_checks(IriCoreServices& services);

}  // namespace iri
