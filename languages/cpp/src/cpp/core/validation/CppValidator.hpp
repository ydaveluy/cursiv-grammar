#pragma once

#include "cpp/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace cpp {

class CppValidator final : public cursiv::DefaultCoreService {
 public:
  void checkStringLiteral(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkUserDefinedLiteral(const ast::UserDefinedLit&, cursiv::ValidationAcceptor&) const;
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkReservedNames(const ast::TranslationUnit& unit,
                          cursiv::ValidationAcceptor& accept) const;
  void checkClass(const ast::ClassType& type,
                  cursiv::ValidationAcceptor& accept) const;
  void checkPureSpecifier(const ast::MemberDeclarator &,
                          cursiv::ValidationAcceptor &) const;
};

struct CppCoreServices;
void register_validation_checks(CppCoreServices& services);

}  // namespace cpp
