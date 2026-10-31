#include "scala/core/validation/ScalaValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "scala/core/CoreServices.hpp"
#include "scala/core/UnicodeSource.hpp"
#include "cursiv/core/workspace/Document.hpp"

namespace scala {
namespace {
bool physical_line(std::string_view text) {
  return text.find_first_of("\r\n") != text.npos;
}
std::string_view original(const cursiv::AstNode& node) {
  const auto* document = node.getDocument();
  if (!document) return {};
  const auto& cst = document->parseResult.cst();
  return node.cstNodeId < cst.size() ? cst.text(node.cstNodeId) : std::string_view{};
}
bool surrogate(unsigned unit) { return unit >= 0xd800 && unit <= 0xdfff; }
// Only ordinary literals and quoted names interpret backslash escapes. A
// retained UTF16 unit stands for a character, not another backslash to decode.
void check_retained_units(std::string_view text, const cursiv::AstNode& node,
                          cursiv::ValidationAcceptor& accept,
                          std::string_view member = "text", std::size_t index = 0) {
  if (text.find('\\') == text.npos) return;
  const auto* document = node.getDocument();
  if (!document) return;
  const auto source = document->textDocument().getText();
  const auto originalRange = accept.featureSpan(node, member, index).value_or(accept.spanOfName(text, node));
  const auto* translation = document->parseResult.cst().translation();
  const auto start = translation ? translation->translated(originalRange.begin)
                                : std::optional<cursiv::TextOffset>{originalRange.begin};
  const auto translated = translation ? translation->text() : source;
  if (!start || *start > translated.size() || text.size() > translated.size() - *start ||
      translated.substr(*start, text.size()) != text) return;
  std::size_t slashes = 0;
  for (std::size_t at = 0; at < text.size();) {
    if (text[at] != '\\') { slashes = 0; ++at; continue; }
    // Inspect one candidate, irrespective of its textual slash prefix. Its
    // eligibility in the original source distinguishes a retained Char unit.
    const auto unit = unicode_escape_at(text.substr(at), 0);
    if (unit && unit->valid && surrogate(unit->unit)) {
      const cursiv::TextRange target{
          static_cast<cursiv::TextOffset>(*start + at),
          static_cast<cursiv::TextOffset>(*start + at + unit->end)};
      const auto origin = translation ? translation->original(target) : target;
      const auto raw = unicode_escape_at(source, origin.begin);
      const bool retained = raw && raw->valid && surrogate(raw->unit) &&
          raw->end == origin.end && origin.begin <= origin.end && origin.end <= source.size() &&
          source.substr(origin.begin, origin.end - origin.begin) == text.substr(at, unit->end);
      if (retained ? slashes % 2 != 0 : slashes % 2 == 0) {
        accept.error(node, "Unicode substitution does not recursively decode an escape.",
                     "scala.unicode-escape-origin").range(origin);
        return;
      }
      if (retained) { at += unit->end; slashes = 0; continue; }
    }
    ++slashes; ++at;
  }
}
void check_name(std::string_view name, const cursiv::AstNode& node,
                cursiv::ValidationAcceptor& accept,
                std::string_view member = "name", std::size_t index = 0) {
  if (name.empty()) return;
  const bool quoted = name.front() == '`';
  const bool line = physical_line(name);
  const bool slash = name.find('\\') != name.npos;
  if (!line && !slash) return;
  if (quoted) check_retained_units(name, node, accept, member, index);
  const auto* document = node.getDocument();
  if (!document) return;
  const auto source = document->textDocument().getText();
  const auto range = accept.featureSpan(node, member, index).value_or(accept.spanOfName(name, node));
  if (range.begin > range.end || range.end > source.size()) return;
  if (line && physical_line(source.substr(range.begin, range.end - range.begin)))
    accept.error(node, "A quoted identifier cannot contain a physical line ending.",
                 "scala.physical-name-newline").range(range);
  if (!quoted && slash) {
    // A retained surrogate must not turn into an unquoted backslash operator.
    // A generated backslash instead maps to a non-surrogate source escape.
    for (std::size_t at = range.begin; at < range.end; ++at) {
      const auto unit = unicode_escape_at(source, at);
      if (unit && unit->valid && surrogate(unit->unit)) {
        accept.error(node, "A UTF16 surrogate is not an unquoted identifier character.",
                     "scala.unquoted-surrogate").range({static_cast<cursiv::TextOffset>(at),
                                                      static_cast<cursiv::TextOffset>(unit->end)});
        break;
      }
    }
  }
}
template<class Values>
void check_names(const Values& values, const cursiv::AstNode& node,
                 cursiv::ValidationAcceptor& accept, std::string_view member) {
  std::size_t index = 0;
  for (const auto value : values) check_name(value, node, accept, member, index++);
}
template<class... Ts>
void check_modifiers(const cursiv::AstNode& node, cursiv::ValidationAcceptor& accept) {
  ([&] {
    if (const auto* n = cursiv::ast_cast<Ts>(&node)) check_names(n->modifiers, node, accept, "modifiers");
  }(), ...);
}
}  // namespace

void ScalaValidator::checkSpelling(const cursiv::AstNode& node,
                                   cursiv::ValidationAcceptor& accept) const {
  bool literal = cursiv::ast_cast<ast::CharLit>(&node) != nullptr;
  if (const auto* n = cursiv::ast_cast<ast::CharLit>(&node)) check_retained_units(n->text, node, accept);
  if (const auto* n = cursiv::ast_cast<ast::StringLit>(&node)) {
    literal = !n->multiline;
    if (!n->multiline) check_retained_units(n->text, node, accept);
  }
  if (const auto* n = cursiv::ast_cast<ast::InterpolationPart>(&node);
      n && !n->text.empty()) {
    if (const auto* owner = cursiv::ast_cast<ast::InterpolatedString>(n->parent))
      literal = !owner->multiline;
    if (const auto* owner = cursiv::ast_cast<ast::InterpolatedPattern>(n->parent))
      literal = !owner->multiline;
  }
  if (literal && physical_line(original(node)))
    accept.error(node, "This literal cannot contain a physical line ending.",
                 "scala.physical-literal-newline");

  if (node.type && node.type->readName) check_name(node.type->readName(node), node, accept);
  if (const auto* n = cursiv::ast_cast<ast::SymbolLit>(&node);
      n && !n->text.empty()) check_name(n->text.substr(1), node, accept, "");
  if (const auto* n = cursiv::ast_cast<ast::StableId>(&node)) {
    check_name(n->qualifierPrefix, node, accept, "qualifierPrefix");
    check_name(n->superQualifier, node, accept, "superQualifier");
    check_name(n->root, node, accept, "root");
    check_names(n->segments, node, accept, "segments");
  }
  if (const auto* n = cursiv::ast_cast<ast::TypeProjection>(&node)) check_name(n->member, node, accept, "member");
  if (const auto* n = cursiv::ast_cast<ast::BinaryType>(&node)) check_name(n->op, node, accept, "op");
  if (const auto* n = cursiv::ast_cast<ast::MemberExpr>(&node)) check_name(n->field, node, accept, "field");
  if (const auto* n = cursiv::ast_cast<ast::PostfixExpr>(&node)) check_name(n->postfixOp, node, accept, "postfixOp");
  if (const auto* n = cursiv::ast_cast<ast::InfixExpr>(&node)) check_name(n->op, node, accept, "op");
  if (const auto* n = cursiv::ast_cast<ast::InfixPattern>(&node)) check_name(n->op, node, accept, "op");
  if (const auto* n = cursiv::ast_cast<ast::ExtractorPattern>(&node)) check_name(n->varargsBindName, node, accept, "varargsBindName");
  if (const auto* n = cursiv::ast_cast<ast::ValDcl>(&node)) check_names(n->names, node, accept, "names");
  if (const auto* n = cursiv::ast_cast<ast::ImportSelector>(&node)) check_name(n->renameTo, node, accept, "renameTo");
  if (const auto* n = cursiv::ast_cast<ast::ImportExpr>(&node)) check_name(n->singleName, node, accept, "singleName");
  if (const auto* n = cursiv::ast_cast<ast::QualId>(&node)) check_names(n->parts, node, accept, "parts");
  if (const auto* n = cursiv::ast_cast<ast::InterpolatedString>(&node)) check_name(n->prefix, node, accept, "prefix");
  if (const auto* n = cursiv::ast_cast<ast::InterpolatedPattern>(&node)) check_name(n->prefix, node, accept, "prefix");
  if (const auto* n = cursiv::ast_cast<ast::ClassDef>(&node)) check_names(n->ctorModifiers, node, accept, "ctorModifiers");
  check_modifiers<ast::Dcl, ast::ClassParam, ast::PatDef, ast::DefDef,
                  ast::SecondaryCtorDef, ast::TypeDef, ast::ClassDef,
                  ast::TraitDef, ast::ObjectDef>(node, accept);
}

void register_validation_checks(ScalaCoreServices& services) {
  services.validator = std::make_unique<const ScalaValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&ScalaValidator::checkSpelling>(*services.validator)});
}

}  // namespace scala
