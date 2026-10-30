#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "molecule/core/ast.hpp"

namespace molecule {

class MoleculeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkStructure(const ast::Structure&, cursiv::ValidationAcceptor&) const;
  void checkPart(const ast::Part&, cursiv::ValidationAcceptor&) const;
};

struct MoleculeCoreServices;
void register_validation_checks(MoleculeCoreServices& services);

}  // namespace molecule
