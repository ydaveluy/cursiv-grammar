#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "yara/core/ast.hpp"
namespace yara {
class YaraValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document &, cursiv::ValidationAcceptor &) const;
  void checkRule(const ast::RuleDecl &, cursiv::ValidationAcceptor &) const;
  void checkString(const ast::StringDef &, cursiv::ValidationAcceptor &) const;
  void checkModifier(const ast::StringModifier &,
                     cursiv::ValidationAcceptor &) const;
  void checkInteger(const ast::IntegerLiteral &,
                    cursiv::ValidationAcceptor &) const;
  void checkMeta(const ast::MetaEntry &, cursiv::ValidationAcceptor &) const;
  void checkLiteral(const ast::Literal &, cursiv::ValidationAcceptor &) const;
  void checkOf(const ast::OfExpr &, cursiv::ValidationAcceptor &) const;
  void checkFor(const ast::ForExpr &, cursiv::ValidationAcceptor &) const;
  void checkRange(const ast::Range &, cursiv::ValidationAcceptor &) const;
  void checkHex(const ast::HexPattern &, cursiv::ValidationAcceptor &) const;
  void checkBranch(const ast::AltBranch &, cursiv::ValidationAcceptor &) const;
  void checkJump(const ast::Jump &, cursiv::ValidationAcceptor &) const;
};
struct YaraCoreServices;
void register_validation_checks(YaraCoreServices &services);
} // namespace yara
