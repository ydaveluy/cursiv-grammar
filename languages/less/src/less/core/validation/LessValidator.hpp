// Context-sensitive checks retained by the permissive syntax.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "less/core/ast.hpp"

namespace less {

class LessValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkVariableDecl(const ast::VariableDecl&,
                         cursiv::ValidationAcceptor&) const;
};

struct LessCoreServices;
void register_validation_checks(LessCoreServices& services);

}  // namespace less
