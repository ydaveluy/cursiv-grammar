// Semantic constraints are checked on the lossless, permissive AST.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "semver/core/ast.hpp"

namespace semver {

class SemverValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkVersion(const ast::Semver&, cursiv::ValidationAcceptor&) const;
};

struct SemverCoreServices;
void register_validation_checks(SemverCoreServices& services);

}  // namespace semver
