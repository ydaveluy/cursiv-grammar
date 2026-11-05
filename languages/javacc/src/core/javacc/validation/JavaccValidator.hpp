// The checks of this language: what the grammar accepts but the language
// does not.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "javacc/ast.hpp"

namespace javacc {

class JavaccValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkParserNamesMatch(const ast::JavaccInput& input,
                             cursiv::ValidationAcceptor& accept) const;
  void checkOption(const ast::OptionBinding&, cursiv::ValidationAcceptor&) const;
  void checkLookahead(const ast::LocalLookahead&, cursiv::ValidationAcceptor&) const;
  void checkRange(const ast::RegexGroupUnit&, cursiv::ValidationAcceptor&) const;
  void checkCharacter(const ast::CharDescriptor&, cursiv::ValidationAcceptor&) const;
  void checkTry(const ast::TryExpansionUnit&, cursiv::ValidationAcceptor&) const;
};

struct JavaccCoreServices;
void register_validation_checks(JavaccCoreServices& services);

}  // namespace javacc
