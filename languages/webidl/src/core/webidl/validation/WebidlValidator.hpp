#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "webidl/ast.hpp"

namespace webidl {

class WebidlValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkName(const cursiv::AstNode &, cursiv::ValidationAcceptor &) const;
  void checkType(const ast::Type &, cursiv::ValidationAcceptor &) const;
  void checkMember(const ast::Member &, cursiv::ValidationAcceptor &) const;
  void checkArgument(const ast::Argument &, cursiv::ValidationAcceptor &) const;
  void checkField(const ast::DictionaryMember &,
                  cursiv::ValidationAcceptor &) const;
  void checkInterface(const ast::Interface &,
                      cursiv::ValidationAcceptor &) const;
  void checkDictionary(const ast::Dictionary &,
                       cursiv::ValidationAcceptor &) const;
  void checkExtendedAttribute(const ast::ExtendedAttribute &,
                              cursiv::ValidationAcceptor &) const;
};

struct WebidlCoreServices;
void register_validation_checks(WebidlCoreServices &services);

} // namespace webidl
