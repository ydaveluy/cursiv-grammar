// Object constraints that depend on decoded key identity.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "maml/ast.hpp"

namespace maml {

class MamlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkObject(const ast::Object&, cursiv::ValidationAcceptor&) const;
};

struct MamlCoreServices;
void register_validation_checks(MamlCoreServices& services);

}  // namespace maml
