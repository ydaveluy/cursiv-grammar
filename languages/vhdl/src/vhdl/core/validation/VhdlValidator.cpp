#include "vhdl/core/validation/VhdlValidator.hpp"
#include <memory>
#include <string>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "vhdl/core/CoreServices.hpp"

namespace vhdl {
namespace {
char lower(char c) { return c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : c; }
bool equal(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  if (a.starts_with("\\") || b.starts_with("\\")) return a == b;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (lower(a[i]) != lower(b[i])) return false;
  return true;
}
bool logical(std::string_view op) {
  return equal(op,"and") || equal(op,"or") || equal(op,"nand") ||
         equal(op,"nor") || equal(op,"xor") || equal(op,"xnor");
}
bool shift(std::string_view op) {
  return equal(op,"sll") || equal(op,"srl") || equal(op,"sla") ||
         equal(op,"sra") || equal(op,"rol") || equal(op,"ror");
}
bool relation(std::string_view op) {
  return op=="=" || op=="/=" || op=="<" || op==">" || op=="<=" || op==">=" ||
         op=="?=" || op=="?/=" || op=="?<" || op=="?>" || op=="?<=" || op=="?>=";
}
void identifier(std::string_view name,const cursiv::AstNode& node,
                cursiv::ValidationAcceptor& accept) {
  if (name.empty() || name.starts_with("\"") || name.starts_with("'")) return;
  if (name.starts_with("\\")) {
    if (name.find_first_of("\r\n\t\f\v") != std::string_view::npos)
      accept.error(node,"An extended identifier cannot contain a line break or format character","vhdl.identifier");
    return;
  }
  if (name.ends_with("_") || name.find("__") != std::string_view::npos)
    accept.error(node,"Identifier underscores must separate letters or digits","vhdl.identifier");
}
std::string_view itemName(const ast::Item& n) {
#define VHDL_NAME(T) if (auto x=cursiv::ast_cast<ast::T>(&n)) return x->name
  VHDL_NAME(EntityDecl); VHDL_NAME(ArchitectureBody); VHDL_NAME(PackageDecl);
  VHDL_NAME(PackageBody); VHDL_NAME(ContextDecl); VHDL_NAME(ConfigurationDecl);
  VHDL_NAME(ComponentDecl); VHDL_NAME(TypeDecl); VHDL_NAME(SubtypeDecl);
  VHDL_NAME(InterfaceType); VHDL_NAME(InterfaceSubprogram); VHDL_NAME(InterfacePackage);
  VHDL_NAME(SubprogramSpec); VHDL_NAME(SubprogramInstDecl); VHDL_NAME(PackageInstDecl);
  VHDL_NAME(AliasDecl); VHDL_NAME(AttributeDecl); VHDL_NAME(UnitDecl);
  VHDL_NAME(GroupTemplateDecl); VHDL_NAME(GroupDecl);
#undef VHDL_NAME
  if (auto x=cursiv::ast_cast<ast::SubprogramBody>(&n))
    return x->spec ? x->spec->name : std::string_view{};
  if (cursiv::ast_cast<ast::PhysicalTypeDef>(&n) ||
      cursiv::ast_cast<ast::RecordTypeDef>(&n) ||
      cursiv::ast_cast<ast::ProtectedTypeDef>(&n))
    if (auto p=cursiv::ast_cast<ast::TypeDecl>(n.parent)) return p->name;
  if (cursiv::ast_cast<ast::GenBody>(&n)) {
    if (auto p=cursiv::ast_cast<ast::GenerateBranch>(n.parent)) return p->label;
    if (auto p=cursiv::ast_cast<ast::CaseGenAlt>(n.parent)) return p->label;
  }
  return {};
}
}  // namespace
void VhdlValidator::checkItem(const ast::Item& node,cursiv::ValidationAcceptor& accept) const {
  const auto name=itemName(node);
  identifier(name,node,accept);
  if (const auto* body=cursiv::ast_cast<ast::SubprogramBody>(&node);
      body && body->spec && !body->endKind.empty() &&
      !equal(body->endKind,body->spec->kind))
    accept.error(*body,"The closing subprogram kind must match its declaration","vhdl.subprogram-end")
        .property<&ast::SubprogramBody::endKind>();
  if (!node.endName.empty() && !equal(name,node.endName))
    accept.error(node,"The closing name must match the opening name","vhdl.closing-name")
        .property<&ast::Item::endName>();
  if (auto x=cursiv::ast_cast<ast::ObjectDecl>(&node))
    for (auto name:x->names) identifier(name,node,accept);
  if (auto x=cursiv::ast_cast<ast::InterfaceObject>(&node))
    for (auto name:x->names) identifier(name,node,accept);
  if (auto x=cursiv::ast_cast<ast::RecordElementDecl>(&node))
    for (auto name:x->names) identifier(name,node,accept);
}
void VhdlValidator::checkConcurrent(const ast::ConcStmt& node,cursiv::ValidationAcceptor& accept) const {
  std::string_view label;
  if (auto x=cursiv::ast_cast<ast::SignalAssign>(&node); x && (x->force || x->release))
    accept.error(node,"Force and release assignments must be sequential statements","vhdl.concurrent-force");
  if (auto x=cursiv::ast_cast<ast::ProcessStmt>(&node)) {
    label=x->label;
    if (x->endPostponed && !x->postponed)
      accept.error(*x,"Only a postponed process may end with postponed","vhdl.process-end")
          .property<&ast::ProcessStmt::endPostponed>();
  }
  if (auto x=cursiv::ast_cast<ast::BlockStmt>(&node)) label=x->label;
  if (auto x=cursiv::ast_cast<ast::GenerateStmt>(&node)) {
    label=x->label;
    if (label.empty()) accept.error(node,"A generate statement requires a label","vhdl.generate-label");
  }
  if (!node.endLabel.empty() && !equal(label,node.endLabel))
    accept.error(node,"The closing label must match the opening label","vhdl.closing-label")
        .property<&ast::ConcStmt::endLabel>();
}
void VhdlValidator::checkSequential(const ast::SeqStmt& node,cursiv::ValidationAcceptor& accept) const {
  std::string_view label;
  if (auto x=cursiv::ast_cast<ast::IfStmt>(&node)) label=x->label;
  if (auto x=cursiv::ast_cast<ast::CaseStmt>(&node)) {
    label=x->label;
    if (x->questioned != x->endQuestioned)
      accept.error(*x,"The opening and closing case matching markers must agree","vhdl.case-matching")
          .property<&ast::CaseStmt::endQuestioned>();
  }
  if (auto x=cursiv::ast_cast<ast::LoopStmt>(&node)) label=x->label;
  if (!node.endLabel.empty() && !equal(label,node.endLabel))
    accept.error(node,"The closing label must match the opening label","vhdl.closing-label")
        .property<&ast::SeqStmt::endLabel>();
}
void VhdlValidator::checkOperator(const ast::BinaryExpr& node,cursiv::ValidationAcceptor& accept) const {
  for (auto operand:{node.left,node.right}) {
    const auto* child=cursiv::ast_cast<ast::BinaryExpr>(operand);
    if (!child) continue;
    if (logical(node.op) && logical(child->op)) {
      if (!equal(node.op,child->op))
        accept.error(node,"Use parentheses when combining different logical operators","vhdl.logical-operators");
      else if (equal(node.op,"nand") || equal(node.op,"nor"))
        accept.error(node,"Use parentheses for consecutive nand or nor operators","vhdl.operator-chain");
    }
    if ((shift(node.op) && shift(child->op)) || (relation(node.op) && relation(child->op)))
      accept.error(node,"Use parentheses for consecutive shift or relational operators","vhdl.operator-chain");
  }
}
void VhdlValidator::checkLiteral(const ast::Literal& node,cursiv::ValidationAcceptor& accept) const {
  const auto text=node.text;
  if (text.empty() || text.front()=='\"' || text.front()=='\'') return;
  bool separator=false;
  for (std::size_t i=0;i<text.size();++i)
    if (text[i]=='_' && (i==0 || i+1==text.size() || text[i-1]=='_' || text[i+1]=='_' ||
        text[i-1]=='#' || text[i+1]=='#' || text[i-1]=='.' || text[i+1]=='.' ||
        text[i-1]=='\"' || text[i+1]=='\"')) separator=true;
  if (separator) accept.error(node,"Literal underscores must separate digits","vhdl.literal-separator");
  if (text.find('\"')!=std::string_view::npos) return;
  if (text.front()<'0' || text.front()>'9') return;
  std::string raw;
  for (char c:text) if (c!='_') raw+=c;
  const auto hash=raw.find('#');
  const auto last=raw.rfind('#');
  if (hash!=std::string::npos && last>hash) {
    unsigned base=0;
    for (std::size_t i=0;i<hash && base<=16;++i) base=base*10+unsigned(raw[i]-'0');
    bool bad=base<2 || base>16;
    for (std::size_t i=hash+1;i<last;++i) {
      if (raw[i]=='.') continue;
      const char c=lower(raw[i]);
      const unsigned digit=c>='0' && c<='9' ? unsigned(c-'0') : unsigned(c-'a'+10);
      bad |= digit>=base;
    }
    if (bad) accept.error(node,"A based literal needs a radix from 2 to 16 and digits within that radix","vhdl.literal-base");
  }
  const auto exponent=raw.find_first_of("eE",hash==std::string::npos?0:last+1);
  if (exponent!=std::string::npos) {
    auto exp=std::string_view(raw).substr(exponent+1);
    if (!exp.empty() && (exp.front()=='+' || exp.front()=='-')) exp.remove_prefix(1);
    bool bad=exp.empty();
    for (char c:exp) bad |= c<'0' || c>'9';
    const bool negativeInteger = exponent+1<raw.size() && raw[exponent+1]=='-' &&
        raw.substr(0,exponent).find('.')==std::string::npos;
    if (bad || negativeInteger)
      accept.error(node,negativeInteger ? "An integer literal cannot have a negative exponent"
                                        : "A literal exponent must be an integer",
                   "vhdl.literal-exponent");
  }
}
void VhdlValidator::checkIdentifier(const ast::Ident& node,cursiv::ValidationAcceptor& accept) const {
  identifier(node.name,node,accept);
}
void VhdlValidator::checkSubprogram(const ast::SubprogramSpec& node,cursiv::ValidationAcceptor& accept) const {
  if (equal(node.kind,"function") && !node.returnType)
    accept.error(node,"A function requires a return type","vhdl.subprogram-return");
  if (equal(node.kind,"procedure") && node.returnType)
    accept.error(node,"A procedure does not have a return type","vhdl.subprogram-return");
}
void VhdlValidator::checkFile(const ast::DesignFile& node,cursiv::ValidationAcceptor& accept) const {
  if (node.units.empty()) accept.error(node,"A design file requires a design unit","vhdl.design-file");
}
void register_validation_checks(VhdlCoreServices& services) {
  services.validator=std::make_unique<const VhdlValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&VhdlValidator::checkItem>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VhdlValidator::checkConcurrent>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VhdlValidator::checkSequential>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VhdlValidator::checkOperator>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VhdlValidator::checkLiteral>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VhdlValidator::checkIdentifier>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VhdlValidator::checkSubprogram>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&VhdlValidator::checkFile>(*services.validator)});
}
}  // namespace vhdl
