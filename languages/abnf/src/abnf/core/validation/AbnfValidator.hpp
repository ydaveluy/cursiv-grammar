#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "abnf/core/ast.hpp"

namespace abnf {

class AbnfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkRule(const ast::Rule&, cursiv::ValidationAcceptor&) const;
  void checkReference(const ast::RuleRef&, cursiv::ValidationAcceptor&) const;
  void checkRepeat(const ast::Repetition&, cursiv::ValidationAcceptor&) const;
  void checkNumber(const ast::NumVal&, cursiv::ValidationAcceptor&) const;
  void checkString(const ast::CharVal&, cursiv::ValidationAcceptor&) const;
  void checkProse(const ast::ProseVal&, cursiv::ValidationAcceptor&) const;
  void checkConcatenation(const ast::Concatenation&, cursiv::ValidationAcceptor&) const;
  void checkAlternation(const ast::Alternation&, cursiv::ValidationAcceptor&) const;
};

struct AbnfCoreServices;
void register_validation_checks(AbnfCoreServices& services);

}  // namespace abnf
