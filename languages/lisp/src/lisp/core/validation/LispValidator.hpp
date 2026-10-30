// Reader constraints that need the retained token text or AST context.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "lisp/core/ast.hpp"

namespace lisp {

class LispValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::Number&, cursiv::ValidationAcceptor&) const;
  void checkBits(const ast::BitVector&, cursiv::ValidationAcceptor&) const;
  void checkList(const ast::List&, cursiv::ValidationAcceptor&) const;
  void checkPrefix(const ast::Prefix&, cursiv::ValidationAcceptor&) const;
};

struct LispCoreServices;
void register_validation_checks(LispCoreServices& services);

}  // namespace lisp
