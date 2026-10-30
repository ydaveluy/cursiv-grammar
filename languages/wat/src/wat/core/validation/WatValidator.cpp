#include "wat/core/validation/WatValidator.hpp"

#include <algorithm>
#include <charconv>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "wat/core/CoreServices.hpp"
#include "wat/core/WatGrammar.hpp"
#include "wat/core/validation/OpcodeNames.hpp"

namespace wat {
namespace {
std::optional<std::uint64_t> natural(std::string_view text) {
  std::string digits;
  for (char c : text)
    if (c != '_') digits += c;
  std::string_view value = digits;
  int base = 10;
  if (value.starts_with("0x")) {
    base = 16;
    value.remove_prefix(2);
  }
  std::uint64_t result = 0;
  auto parsed =
      std::from_chars(value.data(), value.data() + value.size(), result, base);
  if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
    return {};
  return result;
}
void string(const cursiv::AstNode& node, std::string_view property,
            std::size_t index, cursiv::ValidationAcceptor& accept) {
  auto doc = node.getDocument();
  auto span = accept.featureSpan(node, property, index);
  if (!doc || !span) return;
  auto text = doc->textDocument().getText().substr(span->begin,
                                                   span->end - span->begin);
  if (text.empty() || text[0] != '"') return;
  for (std::size_t i = 1; i + 1 < text.size(); ++i) {
    auto start = i;
    bool invalid = static_cast<unsigned char>(text[i]) < 0x20 || text[i] == '\x7f';
    if (text[i] == '\\') {
      if (++i + 1 >= text.size())
        invalid = true;
      else if (std::string_view("nrt\"'\\").find(text[i]) !=
               std::string_view::npos)
        continue;
      else if (hexDigit(text[i]) >= 0 && i + 2 < text.size() &&
               hexDigit(text[i + 1]) >= 0) {
        ++i;
        continue;
      } else if (text[i] == 'u' && i + 1 < text.size() && text[i + 1] == '{') {
        auto end = text.find('}', i + 2);
        invalid = end == text.npos ||
                  !unicodeScalar(text.substr(i + 2, end - i - 2));
        if (end != text.npos) i = end;
      } else
        invalid = true;
    }
    if (invalid)
      accept.error(node, "Invalid text string escape or newline", "wat.string")
          .range({static_cast<cursiv::TextOffset>(span->begin + start),
                  static_cast<cursiv::TextOffset>(span->begin + i + 1)});
  }
}
void strings(const auto& node, std::string_view property, const auto& values,
             cursiv::ValidationAcceptor& accept) {
  for (std::size_t i = 0; i < values.size(); ++i)
    string(node, property, i, accept);
}
bool validUtf8(std::string_view text) {
  for (std::size_t i = 0; i < text.size();) {
    const auto lead = static_cast<unsigned char>(text[i]);
    if (lead < 0x80) { ++i; continue; }
    const unsigned width = lead >= 0xc2 && lead <= 0xdf ? 2 :
                           lead >= 0xe0 && lead <= 0xef ? 3 :
                           lead >= 0xf0 && lead <= 0xf4 ? 4 : 0;
    if (!width || width > text.size() - i) return false;
    for (unsigned j = 1; j < width; ++j)
      if ((static_cast<unsigned char>(text[i + j]) & 0xc0) != 0x80) return false;
    const auto next = static_cast<unsigned char>(text[i + 1]);
    if ((lead == 0xe0 && next < 0xa0) || (lead == 0xed && next >= 0xa0) ||
        (lead == 0xf0 && next < 0x90) || (lead == 0xf4 && next >= 0x90)) return false;
    i += width;
  }
  return true;
}
void name(const cursiv::AstNode& node, std::string_view property,
          std::string_view value, std::size_t index,
          cursiv::ValidationAcceptor& accept) {
  string(node, property, index, accept);
  if (!validUtf8(value))
    accept.error(node, "A text name must decode to valid UTF-8", "wat.name-utf8")
        .range(accept.featureSpan(node, property, index).value_or(accept.spanOf(node)));
}
void names(const auto& node, std::string_view property, const auto& values,
           cursiv::ValidationAcceptor& accept) {
  for (std::size_t i = 0; i < values.size(); ++i)
    name(node, property, values[i], i, accept);
}
void limits(const auto& node, cursiv::ValidationAcceptor& accept) {
  auto lo = node.min ? natural(*node.min) : std::nullopt,
       hi = node.max ? natural(*node.max) : std::nullopt;
  if (lo && hi && *lo > *hi)
    accept
        .error(node, "The maximum must not be smaller than the minimum",
               "wat.limits")
        .range(
            accept.featureSpan(node, "max", 0).value_or(accept.spanOf(node)));
  if (node.shared && !node.max)
    accept.error(node, "A shared limit needs an explicit maximum",
                 "wat.shared-limit");
}
}  // namespace
void WatValidator::checkInstruction(const ast::Instr& node,
                                    cursiv::ValidationAcceptor& accept) const {
  static constexpr std::string_view clauses[] = {
      "param",    "result", "type",   "memory",    "table",     "then",
      "else",     "do",     "catch",  "catch_all", "catch_ref", "catch_all_ref",
      "delegate", "item",   "before", "after",     "ref"};
  bool clause = std::find(std::begin(clauses), std::end(clauses), node.op) !=
                std::end(clauses);
  if (!clause && !node.op.starts_with('@') &&
      !std::binary_search(std::begin(opcodeNames), std::end(opcodeNames),
                          node.op))
    accept.error(node, "Unknown instruction mnemonic", "wat.opcode")
        .property<&ast::Instr::op>();
  if (node.endLabel && node.endLabel != node.label)
    accept
        .error(node, "The end label must match the opening label",
               "wat.end-label")
        .property<&ast::Instr::endLabel>();
  if (node.op == "else" && node.label)
    if (auto parent = cursiv::ast_cast<ast::Instr>(node.parent);
        parent && parent->label != node.label)
      accept
          .error(node, "The else label must match the if label",
                 "wat.else-label")
          .property<&ast::Instr::label>();
  unsigned immediates = 0;
  for (std::size_t i = 0; i < node.args.size(); ++i)
    if (auto value = std::get_if<std::string_view>(&node.args[i])) {
      ++immediates;
      if (value->starts_with("align=")) {
        auto alignment = natural(value->substr(6));
        if (!alignment || *alignment == 0 || (*alignment & (*alignment - 1)))
          accept
              .error(node, "Alignment must be a power of two", "wat.alignment")
              .property<&ast::Instr::args>(i);
      }
      if (node.op.starts_with('@')) string(node, "args", i, accept);
    }
  static constexpr std::string_view one[] = {
      "br",        "br_if",     "call",       "return_call", "local.get",
      "local.set", "local.tee", "global.get", "global.set",  "ref.func",
      "ref.null",  "throw",     "rethrow",    "data.drop",   "elem.drop",
      "delegate",  "i32.const", "i64.const",  "f32.const",   "f64.const"};
  if (std::find(std::begin(one), std::end(one), node.op) != std::end(one) &&
      immediates != 1)
    accept.error(node, "This instruction needs exactly one immediate operand",
                 "wat.immediate");
  static constexpr std::string_view gcOne[] = {
      "struct.new",    "struct.new_default", "array.new",   "array.new_default",
      "array.get",     "array.get_s",        "array.get_u", "array.set",
      "array.fill",    "ref.cast",           "ref.test",    "br_on_null",
      "br_on_non_null"};
  static constexpr std::string_view gcTwo[] = {
      "struct.get",      "struct.get_s",   "struct.get_u",   "struct.set",
      "array.new_fixed", "array.new_data", "array.new_elem", "array.copy",
      "array.init_data", "array.init_elem"};
  static constexpr std::string_view gcZero[] = {
      "array.len",         "ref.i31", "i31.get_s",
      "i31.get_u",         "ref.eq",  "any.convert_extern",
      "extern.convert_any"};
  std::optional<unsigned> expected;
  const auto contains = [&](const auto& names) {
    return std::find(std::begin(names), std::end(names), node.op) !=
           std::end(names);
  };
  if (contains(gcOne)) expected = 1;
  if (contains(gcTwo)) expected = 2;
  if (contains(gcZero)) expected = 0;
  if (node.op == "br_on_cast" || node.op == "br_on_cast_fail") expected = 3;
  if (expected && immediates != *expected)
    accept.error(node, "Incorrect number of GC instruction immediate operands",
                 "wat.gc-immediate");
  if (node.op == "br_table" && immediates == 0)
    accept.error(node, "br_table needs a default target", "wat.immediate");
}
void WatValidator::checkType(const ast::TypeDef& node,
                             cursiv::ValidationAcceptor& accept) const {
  if (node.kind == "array" && node.fields.size() != 1)
    accept.error(node, "An array type needs exactly one storage field",
                 "wat.array-fields");
  if (node.supertypes.size() > 1)
    accept.error(node, "A subtype has at most one supertype", "wat.supertypes");
  if (node.supertypes.empty()) return;
  const cursiv::AstNode* scope = node.parent;
  while (scope && !cursiv::ast_cast<ast::Module>(scope) &&
         !cursiv::ast_cast<ast::Script>(scope))
    scope = scope->parent;
  std::vector<const ast::TypeDef*> types;
  const auto collect = [&](const auto& fields) {
    for (const auto& field : fields) {
      if (const auto* t = std::get_if<ast::TypeDef*>(&field))
        types.push_back(*t);
      else if (const auto* group = std::get_if<ast::RecGroup*>(&field))
        for (const auto* t : (*group)->types) types.push_back(t);
    }
  };
  if (const auto* module = cursiv::ast_cast<ast::Module>(scope))
    collect(module->fields);
  if (const auto* script = cursiv::ast_cast<ast::Script>(scope))
    collect(script->fields);
  for (std::size_t i = 0; i < node.supertypes.size(); ++i) {
    const auto name = node.supertypes[i];
    const ast::TypeDef* parent = nullptr;
    if (name.starts_with('$')) {
      for (const auto* t : types)
        if (t->name == name) {
          parent = t;
          break;
        }
    } else if (const auto index = natural(name);
               index && *index < types.size()) {
      parent = types[*index];
    }
    if (parent && parent->final_.value_or(true))
      accept
          .error(node, "A final type cannot be used as a supertype",
                 "wat.final-supertype")
          .property<&ast::TypeDef::supertypes>(i);
  }
}
void WatValidator::checkMemory(const ast::Memory& node,
                               cursiv::ValidationAcceptor& accept) const {
  limits(node, accept);
  names(node, "exports", node.exports, accept);
  strings(node, "inlineData", node.inlineData, accept);
  if (node.pageSize) {
    auto value = natural(*node.pageSize);
    if (!value || !*value || *value > UINT32_MAX || (*value & (*value - 1)))
      accept
          .error(node, "Page size must be a positive 32-bit power of two",
                 "wat.page-size")
          .property<&ast::Memory::pageSize>();
  }
}
void WatValidator::checkTable(const ast::Table& node,
                              cursiv::ValidationAcceptor& accept) const {
  limits(node, accept);
  names(node, "exports", node.exports, accept);
}
void WatValidator::checkImport(const ast::Import& node,
                               cursiv::ValidationAcceptor& accept) const {
  limits(node, accept);
  name(node, "module_", node.module_, 0, accept);
  name(node, "name", node.name, 0, accept);
}
void WatValidator::checkExport(const ast::Export& node,
                               cursiv::ValidationAcceptor& accept) const {
  name(node, "name", node.name, 0, accept);
}
void WatValidator::checkFunction(const ast::FunctionDecl& node,
                                 cursiv::ValidationAcceptor& accept) const {
  names(node, "exports", node.exports, accept);
}
void WatValidator::checkGlobal(const ast::Global& node,
                               cursiv::ValidationAcceptor& accept) const {
  names(node, "exports", node.exports, accept);
}
void WatValidator::checkData(const ast::Data& node,
                             cursiv::ValidationAcceptor& accept) const {
  strings(node, "strings", node.strings, accept);
}
void WatValidator::checkBinary(const ast::BinQuoteModule& node,
                               cursiv::ValidationAcceptor& accept) const {
  strings(node, "strings", node.strings, accept);
}
void WatValidator::checkAction(const ast::Action& node,
                               cursiv::ValidationAcceptor& accept) const {
  name(node, "name", node.name, 0, accept);
}
void WatValidator::checkAssertion(const ast::Assertion& node,
                                  cursiv::ValidationAcceptor& accept) const {
  if (node.message) name(node, "message", *node.message, 0, accept);
}
void WatValidator::checkRegister(const ast::Register& node,
                                 cursiv::ValidationAcceptor& accept) const {
  name(node, "name", node.name, 0, accept);
}
void register_validation_checks(WatCoreServices& services) {
  services.validator = std::make_unique<const WatValidator>(services);
  using V = WatValidator;
  services.validationRegistry->registerChecks({
#define CHECK(Name) \
  cursiv::ValidationRegistry::makeValidationCheck<&V::Name>(*services.validator)
      CHECK(checkInstruction),
      CHECK(checkType),
      CHECK(checkMemory),
      CHECK(checkTable),
      CHECK(checkImport),
      CHECK(checkExport),
      CHECK(checkFunction),
      CHECK(checkGlobal),
      CHECK(checkData),
      CHECK(checkBinary),
      CHECK(checkAction),
      CHECK(checkAssertion),
      CHECK(checkRegister),

#undef CHECK
  });
}
}  // namespace wat
