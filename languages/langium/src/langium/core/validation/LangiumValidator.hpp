#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "langium/core/ast.hpp"

namespace langium {

class LangiumValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkKeyword(const ast::Keyword&, cursiv::ValidationAcceptor&) const;
  void checkCharacterRange(const ast::CharacterRange&,
                           cursiv::ValidationAcceptor&) const;
  void checkParserRule(const ast::AbstractParserRule&,
                       cursiv::ValidationAcceptor&) const;
  void checkTerminalRule(const ast::TerminalRule&,
                         cursiv::ValidationAcceptor&) const;
  void checkUnorderedGroup(const ast::UnorderedGroup&,
                           cursiv::ValidationAcceptor&) const;
  void checkRegexToken(const ast::RegexToken&,
                       cursiv::ValidationAcceptor&) const;
};

struct LangiumCoreServices;
void register_validation_checks(LangiumCoreServices& services);

}  // namespace langium
