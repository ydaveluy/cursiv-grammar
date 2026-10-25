#include "capnproto/core/validation/CapnprotoValidator.hpp"

#include <charconv>
#include <memory>
#include <unordered_set>

#include "capnproto/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
namespace capnproto {
namespace {
bool number(std::string_view text, std::uint64_t& value) {
  int base = 10;
  if (text.starts_with("0x")) {
    base = 16;
    text.remove_prefix(2);
  } else if (text.size() > 1 && text[0] == '0')
    base = 8;
  const auto r =
      std::from_chars(text.data(), text.data() + text.size(), value, base);
  return r.ec == std::errc{} && r.ptr == text.data() + text.size();
}
bool ordinal(const cursiv::AstNode* n) {
  return cursiv::ast_cast<ast::FieldDef>(n) ||
         cursiv::ast_cast<ast::EnumContent>(n) ||
         cursiv::ast_cast<ast::FunctionDef>(n) ||
         cursiv::ast_cast<ast::UnionDef>(n);
}
bool generic(const ast::Declaration* n) {
  return cursiv::ast_cast<ast::StructDef>(n) ||
         cursiv::ast_cast<ast::InterfaceDef>(n) ||
         cursiv::ast_cast<ast::EnumDef>(n) ||
         cursiv::ast_cast<ast::ConstDef>(n) ||
         cursiv::ast_cast<ast::AnnotationDef>(n) ||
         cursiv::ast_cast<ast::UsingAlias>(n);
}
std::string_view target_name(const ast::Expression* n) {
  if (const auto* name = cursiv::ast_cast<ast::NameExpr>(n)) return name->name;
  if (const auto* member = cursiv::ast_cast<ast::MemberExpr>(n))
    return member->name;
  if (const auto* app = cursiv::ast_cast<ast::ApplicationExpr>(n))
    return target_name(app->function);
  return {};
}
void duplicates(std::span<ast::Declaration* const> content,
                cursiv::ValidationAcceptor& a) {
  std::unordered_set<std::string_view> names;
  for (const auto* d : content) {
    const auto* alias = cursiv::ast_cast<ast::UsingAlias>(d);
    const auto name =
        d->name.empty() && alias ? target_name(alias->target) : d->name;
    if (name.empty() && !cursiv::ast_cast<ast::UnionDef>(d)) continue;
    if (!names.insert(name).second)
      a.error(*d, "this name is already defined in this scope")
          .property<&ast::Declaration::name>();
  }
}
}  // namespace
void CapnprotoValidator::checkDocument(const ast::Document& n,
                                       cursiv::ValidationAcceptor& a) const {
  unsigned ids = 0;
  for (const auto* d : n.content)
    ids += cursiv::ast_cast<ast::FileId>(d) != nullptr;
  if (ids != 1) a.error(n, "a schema file must declare exactly one ID");
  duplicates({n.content.data(), n.content.size()}, a);
}
void CapnprotoValidator::checkDeclaration(const ast::Declaration& n,
                                          cursiv::ValidationAcceptor& a) const {
  const auto* p = n.parent;
  const bool inFile = cursiv::ast_cast<ast::Document>(p);
  const bool inStruct = cursiv::ast_cast<ast::StructDef>(p) ||
                        cursiv::ast_cast<ast::GroupDef>(p) ||
                        cursiv::ast_cast<ast::UnionDef>(p);
  const bool inInterface = cursiv::ast_cast<ast::InterfaceDef>(p);
  const bool inEnum = cursiv::ast_cast<ast::EnumDef>(p);
  const bool allowed =
      (generic(&n) && (inFile || inStruct || inInterface)) ||
      ((cursiv::ast_cast<ast::FileId>(&n) ||
        cursiv::ast_cast<ast::AnnotationStmt>(&n)) &&
       inFile) ||
      ((cursiv::ast_cast<ast::FieldDef>(&n) ||
        cursiv::ast_cast<ast::GroupDef>(&n) ||
        cursiv::ast_cast<ast::UnionDef>(&n)) &&
       inStruct) ||
      (cursiv::ast_cast<ast::FunctionDef>(&n) && inInterface) ||
      (cursiv::ast_cast<ast::EnumContent>(&n) && inEnum);
  if (!allowed) a.error(n, "this declaration is not valid in this scope");
  if (ordinal(&n) && !cursiv::ast_cast<ast::UnionDef>(&n) && !n.id)
    a.error(n, "this declaration needs an ordinal");
  if (const auto* u = cursiv::ast_cast<ast::UnionDef>(&n);
      u && u->id && !u->legacy)
    a.error(n, "a legacy union ordinal must be followed by !");
  if (const auto* alias = cursiv::ast_cast<ast::UsingAlias>(&n)) {
    if (n.name.empty() && !cursiv::ast_cast<ast::MemberExpr>(alias->target))
      a.error(n, "using without = must name a declaration in another scope");
    if (!n.name.empty()) {
      const auto target = target_name(alias->target);
      const bool lower =
          !target.empty() && target[0] >= 'a' && target[0] <= 'z';
      if (lower ? n.name[0] < 'a' || n.name[0] > 'z'
                : n.name[0] < 'A' || n.name[0] > 'Z')
        a.error(n, lower
                       ? "non-type aliases must begin with a lower-case letter"
                       : "type aliases must begin with a capital letter")
            .property<&ast::Declaration::name>();
    }
  }
  if (!n.name.empty()) {
    if (n.name.find('_') != std::string_view::npos)
      a.error(n, "declaration names must not contain underscores")
          .property<&ast::Declaration::name>();
    const bool type = cursiv::ast_cast<ast::StructDef>(&n) ||
                      cursiv::ast_cast<ast::InterfaceDef>(&n) ||
                      cursiv::ast_cast<ast::EnumDef>(&n);
    if (!cursiv::ast_cast<ast::UsingAlias>(&n) &&
        (type ? n.name[0] < 'A' || n.name[0] > 'Z'
              : n.name[0] < 'a' || n.name[0] > 'z'))
      a.error(n, type ? "type names must begin with a capital letter"
                      : "non-type names must begin with a lower-case letter")
          .property<&ast::Declaration::name>();
  }
  duplicates({n.content.data(), n.content.size()}, a);
}
void CapnprotoValidator::checkId(const ast::Id& n,
                                 cursiv::ValidationAcceptor& a) const {
  std::uint64_t v = 0;
  if (!number(n.text, v)) {
    a.error(n, "this ID is not a valid unsigned 64-bit integer");
    return;
  }
  if (ordinal(n.parent) ? v > 65535 : v < (std::uint64_t{1} << 63))
    a.error(n, ordinal(n.parent) ? "ordinals must not exceed 65535"
                                 : "unique IDs must have their high bit set");
}
void CapnprotoValidator::checkInteger(const ast::IntegerLiteral& n,
                                      cursiv::ValidationAcceptor& a) const {
  std::uint64_t v = 0;
  if (!number(n.text, v))
    a.error(n, "integer literals must fit in an unsigned 64-bit magnitude");
}
void CapnprotoValidator::checkBytes(const ast::BytesLiteral& n,
                                    cursiv::ValidationAcceptor& a) const {
  if (n.value.empty())
    a.error(n, "hex binary literals must contain at least one byte");
}
void CapnprotoValidator::checkNegative(const ast::NegativeExpr& n,
                                       cursiv::ValidationAcceptor& a) const {
  const auto* name = cursiv::ast_cast<ast::NameExpr>(n.operand);
  if (!cursiv::ast_cast<ast::IntegerLiteral>(n.operand) &&
      !cursiv::ast_cast<ast::FloatLiteral>(n.operand) &&
      !(name && !name->absolute && name->name == "inf"))
    a.error(n, "only a numeric literal or inf can be negated");
}
void CapnprotoValidator::checkTargets(const ast::AnnotationDef& n,
                                      cursiv::ValidationAcceptor& a) const {
  constexpr std::string_view valid[] = {
      "file",  "const",      "enum",  "enumerant", "struct",
      "field", "union",      "group", "interface", "method",
      "param", "annotation", "*"};
  std::unordered_set<std::string_view> seen;
  for (auto t : n.targets) {
    bool found = false;
    for (auto v : valid) found |= v == t;
    if (!found) a.error(n, "unknown annotation target");
    if (!seen.insert(t).second) a.error(n, "duplicate annotation target");
    if (t == "*" && n.targets.size() != 1)
      a.error(n, "a wildcard annotation target must stand alone");
  }
}
void register_validation_checks(CapnprotoCoreServices& services) {
  using V = CapnprotoValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDeclaration>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkId>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkInteger>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBytes>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkNegative>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTargets>(v)});
}
}  // namespace capnproto
