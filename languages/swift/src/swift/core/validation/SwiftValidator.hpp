#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "swift/core/ast.hpp"

namespace swift {

class SwiftValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkString(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkPrefix(const ast::PrefixExpr&, cursiv::ValidationAcceptor&) const;
  void checkAccessor(const ast::Accessor&, cursiv::ValidationAcceptor&) const;
  void checkCondition(const ast::Condition&, cursiv::ValidationAcceptor&) const;
  void checkInitializer(const ast::InitializerDecl&, cursiv::ValidationAcceptor&) const;
  void checkFile(const ast::File&, cursiv::ValidationAcceptor&) const;
};

struct SwiftCoreServices;
void register_validation_checks(SwiftCoreServices& services);

}  // namespace swift
