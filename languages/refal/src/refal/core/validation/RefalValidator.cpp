#include "refal/core/validation/RefalValidator.hpp"
#include <memory>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "refal/core/CoreServices.hpp"
namespace refal {
namespace {
bool identifier(std::string_view s) {
  const auto letter=[](char c) {return (c>='a'&&c<='z')||(c>='A'&&c<='Z');};
  if (s.empty() || !letter(s[0])) return false;
  for (char c:s) if (!letter(c) && !(c>='0'&&c<='9') && c!='_' && c!='-') return false;
  return true;
}
bool macrodigit(std::string_view s) {
  if (s.empty()) return false;
  std::uint64_t n=0;
  for (char c:s) {
    if (c<'0'||c>'9') return false;
    n=n*10+static_cast<unsigned>(c-'0');
    if(n>0xFFFFFFFFULL) return false;
  }
  return true;
}
}
void RefalValidator::checkNumber(const ast::NumberSymbol& n, cursiv::ValidationAcceptor& a) const {
  if(!macrodigit(n.text)) a.error(n,"a Refal-5 macrodigit must be at most 4294967295");
}
void RefalValidator::checkFunction(const ast::FunctionDef& n, cursiv::ValidationAcceptor& a) const {
  if(!identifier(n.name)) a.error(n,"a function name must be an identifier");
}
void RefalValidator::checkExternal(const ast::ExternalDecl& n, cursiv::ValidationAcceptor& a) const {
  for(auto name:n.names) if(!identifier(name)) a.error(n,"an external name must be an identifier");
}
void RefalValidator::checkVariable(const ast::Variable& n, cursiv::ValidationAcceptor& a) const {
  if(!identifier(n.index)&&!macrodigit(n.index)) a.error(n,"a variable index must be an identifier or macrodigit");
}
void RefalValidator::checkCall(const ast::Call& n, cursiv::ValidationAcceptor& a) const {
  const auto* head=n.body.empty()?nullptr:cursiv::ast_cast<ast::Symbol>(n.body[0]);
  if(!head||(!identifier(head->text)&&head->text!="+"&&head->text!="-"&&head->text!="*"&&head->text!="/"))
    a.error(n,"an activation requires an identifier or arithmetic function name");
}
void register_validation_checks(RefalCoreServices& services) {
  using V=RefalValidator;
  services.validator=std::make_unique<const V>(services);
  const auto& v=*services.validator;
  services.validationRegistry->registerChecks({
    cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(v),
    cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(v),
    cursiv::ValidationRegistry::makeValidationCheck<&V::checkExternal>(v),
    cursiv::ValidationRegistry::makeValidationCheck<&V::checkVariable>(v),
    cursiv::ValidationRegistry::makeValidationCheck<&V::checkCall>(v)});
}
} // namespace refal
