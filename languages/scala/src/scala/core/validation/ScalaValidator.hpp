// Lexical constraints whose spelling matters after Unicode source translation.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "scala/core/ast.hpp"

namespace scala {

class ScalaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkSpelling(const cursiv::AstNode&, cursiv::ValidationAcceptor&) const;
};

struct ScalaCoreServices;
void register_validation_checks(ScalaCoreServices& services);

}  // namespace scala
