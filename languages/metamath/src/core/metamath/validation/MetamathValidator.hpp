#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "metamath/ast.hpp"

namespace metamath {

class MetamathValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDatabase(const ast::Database &, cursiv::ValidationAcceptor &) const;
  void checkMath(const ast::MathStatement &,
                 cursiv::ValidationAcceptor &) const;
  void checkStep(const ast::ProofStep &, cursiv::ValidationAcceptor &) const;
  void checkPlain(const ast::UncompressedProof &,
                  cursiv::ValidationAcceptor &) const;
  void checkCompressed(const ast::CompressedProof &,
                       cursiv::ValidationAcceptor &) const;
};

struct MetamathCoreServices;
void register_validation_checks(MetamathCoreServices &services);

} // namespace metamath
