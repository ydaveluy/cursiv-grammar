#include "oncrpc/core/validation/OncrpcValidator.hpp"

#include <memory>
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "oncrpc/core/CoreServices.hpp"

namespace oncrpc {

namespace {

enum class NumberStatus { reference, valid, malformed, outOfRange };

NumberStatus numberStatus(std::string_view text, std::uint64_t positive,
                          std::uint64_t negative) {
  if (text.empty() || (text.front() != '+' && text.front() != '-' &&
                       (text.front() < '0' || text.front() > '9')))
    return NumberStatus::reference;
  const bool minus = text.front() == '-';
  if (text.front() == '+' || minus) text.remove_prefix(1);
  unsigned base = 10;
  if (text.starts_with("0x") || text.starts_with("0X")) {
    base = 16;
    text.remove_prefix(2);
  } else if (text.size() > 1 && text.front() == '0') base = 8;
  if (text.empty()) return NumberStatus::malformed;
  const auto limit = minus ? negative : positive;
  std::uint64_t value = 0;
  bool overflow = false;
  for (const char c : text) {
    unsigned digit;
    if (c >= '0' && c <= '9') digit = static_cast<unsigned>(c - '0');
    else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned>(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned>(c - 'A' + 10);
    else return NumberStatus::malformed;
    if (digit >= base) return NumberStatus::malformed;
    if (!overflow) {
      overflow = digit > limit || value > (limit - digit) / base;
      if (!overflow) value = value * base + digit;
    }
  }
  return overflow ? NumberStatus::outOfRange : NumberStatus::valid;
}

template <auto Member, class Node>
void checkNumber(const Node& node, cursiv::ValidationAcceptor& accept,
                 std::uint64_t positive, std::uint64_t negative,
                 bool bounded = true) {
  const auto status = numberStatus(node.*Member, positive, negative);
  if (status == NumberStatus::malformed)
    accept.error(node, "Invalid integer literal.", "oncrpc.number")
        .template property<Member>();
  else if (bounded && status == NumberStatus::outOfRange)
    accept.error(node, "Integer is outside the permitted range.", "oncrpc.range")
        .template property<Member>();
}

constexpr auto kUint32Max = std::numeric_limits<std::uint32_t>::max();

std::string_view definitionName(const ast::Definition& def) {
  if (def.type == &cursiv::ast_type_tag<ast::TypedefDef>) {
    const auto& d = static_cast<const ast::TypedefDef&>(def);
    if (d.decl != nullptr &&
        d.decl->type == &cursiv::ast_type_tag<ast::ValueDecl>)
      return static_cast<const ast::ValueDecl*>(d.decl)->name;
    return {};
  }
  if (def.type == &cursiv::ast_type_tag<ast::EnumDef>)
    return static_cast<const ast::EnumDef&>(def).name;
  if (def.type == &cursiv::ast_type_tag<ast::StructDef>)
    return static_cast<const ast::StructDef&>(def).name;
  if (def.type == &cursiv::ast_type_tag<ast::UnionDef>)
    return static_cast<const ast::UnionDef&>(def).name;
  if (def.type == &cursiv::ast_type_tag<ast::ConstantDef>)
    return static_cast<const ast::ConstantDef&>(def).name;
  if (def.type == &cursiv::ast_type_tag<ast::ProgramDef>)
    return static_cast<const ast::ProgramDef&>(def).name;
  return {};
}

}  // namespace

void OncrpcValidator::checkDeclaration(
    const ast::ValueDecl& node, cursiv::ValidationAcceptor& accept) const {
  const bool argument = cursiv::ast_cast<ast::ProcedureDef>(node.parent) != nullptr;
  const bool string = cursiv::ast_cast<ast::StringType>(node.type) != nullptr;
  const bool opaque = cursiv::ast_cast<ast::OpaqueType>(node.type) != nullptr;
  if (node.pointer && node.bound != ast::Bound::plain)
    accept.error(node, "Use a typedef for arrays of pointers.", "oncrpc.pointerArray")
        .property<&ast::ValueDecl::bound>();
  if (argument) {
    if (!node.extraNames.empty())
      accept.error(node, "An argument has more than one name.", "oncrpc.argumentName")
          .property<&ast::ValueDecl::extraNames>(0);
    if (opaque)
      accept.error(node, "Use a typedef for an opaque argument.", "oncrpc.argumentType")
          .property<&ast::ValueDecl::type>();
    if (string && node.pointer)
      accept.error(node, "A string argument cannot be a pointer.", "oncrpc.stringPointer")
          .property<&ast::ValueDecl::pointer>();
    if (node.bound != ast::Bound::plain &&
        !(string && node.bound == ast::Bound::variable))
      accept.error(node, "Use a typedef for an array argument.", "oncrpc.argumentArray")
          .property<&ast::ValueDecl::bound>();
  } else {
    if (opaque && node.bound == ast::Bound::plain)
      accept.error(node, "Opaque data needs an array bound.", "oncrpc.opaqueBound")
          .property<&ast::ValueDecl::type>();
    if (string && node.bound != ast::Bound::variable)
      accept.error(node, "Strings need a variable array bound.", "oncrpc.stringBound")
          .property<&ast::ValueDecl::bound>();
  }
  if (node.size) {
    const auto status = numberStatus(*node.size, kUint32Max, 0);
    if (status == NumberStatus::malformed || status == NumberStatus::outOfRange)
      accept.error(node, "Array bound must fit an unsigned 32-bit integer.", "oncrpc.bound")
          .property<&ast::ValueDecl::size>();
  }
}

void OncrpcValidator::checkVoid(
    const ast::VoidDecl& node, cursiv::ValidationAcceptor& accept) const {
  bool allowed = cursiv::ast_cast<ast::CaseSpec>(node.parent) != nullptr ||
                 cursiv::ast_cast<ast::ProcedureDef>(node.parent) != nullptr;
  if (const auto* body = cursiv::ast_cast<ast::UnionType>(node.parent))
    allowed |= body->defaultCase == &node;
  if (!allowed) accept.error(node, "Void is only valid in a union arm or procedure.", "oncrpc.void");
}

void OncrpcValidator::checkConstant(
    const ast::ConstantDef& node, cursiv::ValidationAcceptor& accept) const {
  checkNumber<&ast::ConstantDef::value>(node, accept, UINT64_MAX, UINT64_MAX, false);
}

void OncrpcValidator::checkEnumConstant(
    const ast::EnumConst& node, cursiv::ValidationAcceptor& accept) const {
  checkNumber<&ast::EnumConst::value>(node, accept, INT32_MAX, std::uint64_t{1} << 31);
}

void OncrpcValidator::checkCase(
    const ast::CaseLabel& node, cursiv::ValidationAcceptor& accept) const {
  checkNumber<&ast::CaseLabel::value>(node, accept, UINT64_MAX, UINT64_MAX, false);
  if (node.value.starts_with('\'') && node.value.size() != 3)
    accept.error(node, "A character label must contain one byte.", "oncrpc.character")
        .property<&ast::CaseLabel::value>();
}

void OncrpcValidator::checkProcedure(
    const ast::ProcedureDef& node, cursiv::ValidationAcceptor& accept) const {
  checkNumber<&ast::ProcedureDef::number>(node, accept, kUint32Max, 0);
  if (cursiv::ast_cast<ast::OpaqueType>(node.returnType))
    accept.error(node, "Use a typedef for an opaque result.", "oncrpc.result")
        .property<&ast::ProcedureDef::returnType>();
  if (node.arguments.size() > 1)
    for (std::size_t i = 0; i < node.arguments.size(); ++i)
      if (cursiv::ast_cast<ast::VoidDecl>(node.arguments[i]))
        accept.error(node, "Void must be the only argument.", "oncrpc.voidArgument")
            .property<&ast::ProcedureDef::arguments>(i);
}

void OncrpcValidator::checkVersion(
    const ast::VersionDef& node, cursiv::ValidationAcceptor& accept) const {
  checkNumber<&ast::VersionDef::number>(node, accept, kUint32Max, 0);
}

void OncrpcValidator::checkProgram(
    const ast::ProgramDef& node, cursiv::ValidationAcceptor& accept) const {
  checkNumber<&ast::ProgramDef::number>(node, accept, kUint32Max, 0);
}

void OncrpcValidator::checkTypedef(
    const ast::TypedefDef& node, cursiv::ValidationAcceptor& accept) const {
  const auto* value = cursiv::ast_cast<ast::ValueDecl>(node.decl);
  if (!value) return;
  static constexpr std::array reserved{
      std::string_view("array"), std::string_view("bytes"), std::string_view("destroy"),
      std::string_view("free"), std::string_view("getpos"), std::string_view("inline"),
      std::string_view("pointer"), std::string_view("reference"), std::string_view("setpos"),
      std::string_view("sizeof"), std::string_view("union"), std::string_view("vector"),
      std::string_view("string"), std::string_view("opaque")};
  if (std::find(reserved.begin(), reserved.end(), value->name) != reserved.end())
    accept.error(*value, "This name is reserved by the XDR runtime.", "oncrpc.reserved")
        .property<&ast::ValueDecl::name>();
}

void OncrpcValidator::checkUniqueNames(
    const ast::Specification& spec, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Definition*>>
      names;
  for (const ast::Definition* const def : spec.items) {
    if (def == nullptr) continue;
    const std::string_view name = definitionName(*def);
    if (!name.empty()) names[name].push_back(def);
  }

  for (const auto& [name, defs] : names) {
    if (defs.size() <= 1) continue;
    const std::string message = "Duplicate identifier: " + std::string(name);
    for (const ast::Definition* const def : defs)
      accept.error(*def, message);
  }
}

void register_validation_checks(OncrpcCoreServices& services) {
  using V = OncrpcValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkUniqueNames>(
          validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDeclaration>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkVoid>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkConstant>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkEnumConstant>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCase>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkProcedure>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkVersion>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkProgram>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTypedef>(validator)});
}

}  // namespace oncrpc
