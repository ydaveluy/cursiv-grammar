#pragma once

#include "antlr/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace antlr {

class AntlrValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkGrammar(const ast::Grammar &, cursiv::ValidationAcceptor &) const;
  void checkParserRule(const ast::ParserRule &,
                       cursiv::ValidationAcceptor &) const;
  void checkLexerRule(const ast::LexerRule &,
                      cursiv::ValidationAcceptor &) const;
  void checkMode(const ast::Mode &, cursiv::ValidationAcceptor &) const;
  void checkAlt(const ast::Alt &, cursiv::ValidationAcceptor &) const;
  void checkElement(const ast::Element &, cursiv::ValidationAcceptor &) const;
  void checkOption(const ast::Option &, cursiv::ValidationAcceptor &) const;
};

struct AntlrCoreServices;
void register_validation_checks(AntlrCoreServices &services);

} // namespace antlr
