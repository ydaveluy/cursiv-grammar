// Object constraints that depend on decoded key identity.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "maml/core/ast.hpp"

namespace maml {

class MamlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkObject(const ast::Object&, cursiv::ValidationAcceptor&) const;
};

struct MamlCoreServices;
void register_validation_checks(MamlCoreServices& services);

}  // namespace maml
