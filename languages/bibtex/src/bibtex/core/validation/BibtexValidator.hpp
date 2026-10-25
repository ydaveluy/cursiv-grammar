#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "bibtex/core/ast.hpp"

namespace bibtex {
class BibtexValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkUniqueCitekeys(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkEntry(const ast::Entry&, cursiv::ValidationAcceptor&) const;
  void checkString(const ast::StringDef&, cursiv::ValidationAcceptor&) const;
  void checkPreamble(const ast::Preamble&, cursiv::ValidationAcceptor&) const;
  void checkComment(const ast::Comment&, cursiv::ValidationAcceptor&) const;
  void checkField(const ast::Field&, cursiv::ValidationAcceptor&) const;
  void checkValue(const ast::Value&, cursiv::ValidationAcceptor&) const;
};
struct BibtexCoreServices;
void register_validation_checks(BibtexCoreServices&);
}  // namespace bibtex
