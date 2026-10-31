#include "fortran/core/validation/FortranValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "fortran/core/CoreServices.hpp"

namespace fortran {
namespace {
bool sameName(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; };
  for (std::size_t i = 0; i < a.size(); ++i)
    if (lower(a[i]) != lower(b[i])) return false;
  return true;
}
std::string_view labelValue(std::string_view label) {
  const auto first = label.find_first_not_of('0');
  return first == std::string_view::npos ? std::string_view{} : label.substr(first);
}
}  // namespace

void FortranValidator::checkUnit(const ast::ProgramUnit& unit,
                                 cursiv::ValidationAcceptor& accept) const {
  std::string_view name;
  bool procedure = false;
  if (const auto* node = cursiv::ast_cast<ast::MainProgramUnit>(&unit)) name = node->name;
  if (const auto* node = cursiv::ast_cast<ast::ModuleUnit>(&unit)) name = node->name;
  if (const auto* node = cursiv::ast_cast<ast::BlockDataUnit>(&unit)) name = node->name;
  if (const auto* node = cursiv::ast_cast<ast::FunctionUnit>(&unit)) {
    name = node->name; procedure = true;
  }
  if (const auto* node = cursiv::ast_cast<ast::SubroutineUnit>(&unit)) {
    name = node->name; procedure = true;
  }
  if (!unit.endName.empty() && !sameName(name, unit.endName))
    accept.error(unit, "The closing name must match its program-unit name.",
                 "fortran.end-name").property<&ast::ProgramUnit::endName>();
  if (procedure && !unit.endHasKind &&
      cursiv::ast_cast<ast::ProgramUnit>(unit.parent))
    accept.error(unit, "An internal or module procedure needs its kind after END.",
                 "fortran.end-kind");
}

void FortranValidator::checkStatement(const ast::Stmt& statement,
                                      cursiv::ValidationAcceptor& accept) const {
  if (const auto* node = cursiv::ast_cast<ast::BlockIfStmt>(&statement)) {
    if (!sameName(node->name, node->endName))
      accept.error(*node, "A named IF needs the same name on END IF.",
                   "fortran.construct-name").property<&ast::BlockIfStmt::endName>();
    if (!node->elseName.empty() && !sameName(node->name, node->elseName))
      accept.error(*node, "ELSE must use its IF construct's name.",
                   "fortran.construct-name").property<&ast::BlockIfStmt::elseName>();
    for (const auto* branch : node->elseIfs)
      if (branch && !branch->name.empty() && !sameName(node->name, branch->name))
        accept.error(*branch, "ELSE IF must use its IF construct's name.",
                     "fortran.construct-name").property<&ast::ElseIfClause::name>();
  }
  if (const auto* node = cursiv::ast_cast<ast::DoStmt>(&statement)) {
    if (!sameName(node->name, node->endName))
      accept.error(*node, "A named DO needs the same name on END DO.",
                   "fortran.construct-name").property<&ast::DoStmt::endName>();
    if ((node->endsWithContinue && node->endLabel.empty()) ||
        (!node->endLabel.empty() &&
         labelValue(node->endLabel) != labelValue(node->terminalLabel)))
      accept.error(*node, "A labelled block DO needs its matching terminal label; an unlabelled DO needs END DO.",
                   "fortran.do-end");
  }
  if (const auto* node = cursiv::ast_cast<ast::SelectCaseStmt>(&statement)) {
    if (!sameName(node->name, node->endName))
      accept.error(*node, "A named SELECT needs the same name on END SELECT.",
                   "fortran.construct-name").property<&ast::SelectCaseStmt::endName>();
    for (const auto* clause : node->cases)
      if (clause && !clause->name.empty() && !sameName(node->name, clause->name))
        accept.error(*clause, "CASE must use its SELECT construct's name.",
                     "fortran.construct-name").property<&ast::CaseClause::name>();
  }
  if (const auto* node = cursiv::ast_cast<ast::DerivedTypeDecl>(&statement))
    if (!node->endName.empty() && !sameName(node->name, node->endName))
      accept.error(*node, "END TYPE must use its derived type's name.",
                   "fortran.end-name").property<&ast::DerivedTypeDecl::endName>();
  if (const auto* node = cursiv::ast_cast<ast::InterfaceDecl>(&statement))
    if (!node->endName.empty() && !sameName(node->name, node->endName))
      accept.error(*node, "END INTERFACE must use its interface's name.",
                   "fortran.end-name").property<&ast::InterfaceDecl::endName>();
}

void register_validation_checks(FortranCoreServices& services) {
  services.validator = std::make_unique<const FortranValidator>(services);
  using V = FortranValidator;
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkUnit>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkStatement>(validator)});
}

}  // namespace fortran
