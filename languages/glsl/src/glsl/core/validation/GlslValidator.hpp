#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "glsl/core/ast.hpp"
namespace glsl {
class GlslValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkType(const ast::TypeRef &, cursiv::ValidationAcceptor &) const;
  void checkNumber(const ast::NumberLit &, cursiv::ValidationAcceptor &) const;
  void checkString(const ast::StringLit &, cursiv::ValidationAcceptor &) const;
  void checkAssignment(const ast::AssignExpr &,
                       cursiv::ValidationAcceptor &) const;
  void checkPrefix(const ast::PrefixOpExpr &,
                   cursiv::ValidationAcceptor &) const;
  void checkPostfix(const ast::PostfixOpExpr &,
                    cursiv::ValidationAcceptor &) const;
  void checkBreak(const ast::BreakStmt &, cursiv::ValidationAcceptor &) const;
  void checkContinue(const ast::ContinueStmt &,
                     cursiv::ValidationAcceptor &) const;
  void checkStruct(const ast::StructSpecifier &,
                   cursiv::ValidationAcceptor &) const;
  void checkLayout(const ast::LayoutQualifierId &,
                   cursiv::ValidationAcceptor &) const;
  void checkDirectives(const ast::TranslationUnit &,
                       cursiv::ValidationAcceptor &) const;
};
struct GlslCoreServices;
void register_validation_checks(GlslCoreServices &services);
} // namespace glsl
