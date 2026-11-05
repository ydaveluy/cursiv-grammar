#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "smiles/ast.hpp"

namespace smiles {

class SmilesValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkAtom(const ast::Atom&, cursiv::ValidationAcceptor&) const;
  void checkRings(const ast::Smiles&, cursiv::ValidationAcceptor&) const;
};

struct SmilesCoreServices;
void register_validation_checks(SmilesCoreServices& services);

}  // namespace smiles
