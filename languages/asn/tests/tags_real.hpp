#pragma once

#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "asn/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace asn_tests {
namespace ast = asn::ast;

inline bool failed(std::string_view reason, std::string_view source) {
  std::fprintf(stderr, "asn tags/REAL: %.*s\nsource: %.*s\n",
               static_cast<int>(reason.size()), reason.data(),
               static_cast<int>(source.size()), source.data());
  return false;
}

inline bool clean(const cursiv::ParseResult& parsed, std::string_view source) {
  if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) return true;
  std::fprintf(stderr, "accepted=%d recovered=%d diagnostics=%zu\n", parsed.accepted,
               parsed.cst().recovered(), parsed.diagnostics.size());
  for (const auto& diagnostic : parsed.diagnostics)
    std::fprintf(stderr, "kind=%d begin=%u end=%u\n", static_cast<int>(diagnostic.kind),
                 diagnostic.begin, diagnostic.end);
  return failed("source did not parse clean", source);
}

inline bool parents(const auto& language, const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) return false;
  bool valid = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { valid = false; return; }
    language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node &&
                   parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return valid && seen.size() == parsed.nodes.size();
}

inline const ast::Decl* find(const ast::Module& module, std::string_view name) {
  for (const auto* declaration : module.assignments)
    if (declaration && declaration->name == name) return declaration;
  return nullptr;
}

inline const ast::Type* type(const ast::Module& module, std::string_view name) {
  const auto* assignment = cursiv::ast_cast<ast::TypeAssignment>(find(module, name));
  return assignment ? assignment->type : nullptr;
}

inline const ast::Value* value(const ast::Module& module, std::string_view name) {
  const auto* assignment = cursiv::ast_cast<ast::ValueAssignment>(find(module, name));
  return assignment ? assignment->value : nullptr;
}

inline bool integer(const ast::Value* value, std::string_view text) {
  const auto* number = cursiv::ast_cast<ast::IntegerValue>(value);
  return number && number->text == text;
}

inline bool tagsAndReal(const auto& language) {
  const std::string source = R"asn(
Other DEFINITIONS ::= BEGIN
tagNumber INTEGER ::= 7
m INTEGER ::= 1
b INTEGER ::= 2
e INTEGER ::= -4
END
M DEFINITIONS AUTOMATIC TAGS ::= BEGIN
IMPORTS tagNumber,m,b,e FROM Other;
tag INTEGER ::= 3
A ::= [0] INTEGER (1..9)
B ::= [APPLICATION 3] IMPLICIT OCTET STRING
C ::= [PRIVATE tag] EXPLICIT [TAG: 8] BOOLEAN
D ::= SEQUENCE {
  a [1] REAL DEFAULT { mantissa -25, base 10, exponent -1 },
  b [2] CHOICE { i INTEGER, n NULL } OPTIONAL
}
r REAL ::= { mantissa -25, base 10, exponent -1 }
binary [7] IMPLICIT REAL ::= { mantissa 1, base 2, exponent -1074 }
external [Other.tagNumber] REAL ::= { mantissa Other.m, base Other.b, exponent Other.e }
positive REAL ::= PLUS-INFINITY
negative REAL ::= MINUS-INFINITY
nan REAL ::= NOT-A-NUMBER
zero REAL ::= 0
minusZero REAL ::= -0
mantissa OBJECT IDENTIFIER ::= { iso(1) }
oid OBJECT IDENTIFIER ::= { mantissa 1 }
ordinary SEQUENCE { mantissa INTEGER, base INTEGER, exponent INTEGER } ::=
  { mantissa 1, base 3, exponent 0 }
END
)asn";
  const auto parsed = language.parser->parse(source);
  if (!clean(parsed, source) || !parents(language, parsed))
    return failed("complete ownership of typed values and tags", source);
  const auto* modules = cursiv::ast_cast<ast::Modules>(parsed.value);
  if (!modules || modules->modules.size() != 2) return failed("two modules", source);
  const auto& module = *modules->modules[1];
  if (module.tagDefault != ast::TagMode::automatic_ || module.assignments.size() != 16 ||
      modules->modules[0]->assignments.size() != 4) return failed("module default/assignments", source);
  const auto* a = cursiv::ast_cast<ast::TaggedType>(type(module, "A"));
  const auto* aInner = a ? cursiv::ast_cast<ast::IntegerType>(a->inner) : nullptr;
  const auto* b = cursiv::ast_cast<ast::TaggedType>(type(module, "B"));
  const auto* c = cursiv::ast_cast<ast::TaggedType>(type(module, "C"));
  const auto* cNumber = c ? cursiv::ast_cast<ast::NameValue>(c->number) : nullptr;
  const auto* inner = c ? cursiv::ast_cast<ast::TaggedType>(c->inner) : nullptr;
  if (!a || a->tagClass != ast::TagClass::contextSpecific ||
      a->mode != ast::TagMode::none_ || a->explicitEncodingReference ||
      !integer(a->number, "0") || !aInner || aInner->constraints.size() != 1 ||
      !b || b->tagClass != ast::TagClass::application ||
      b->mode != ast::TagMode::implicit_ || !integer(b->number, "3") ||
      !cursiv::ast_cast<ast::OctetStringType>(b->inner) ||
      !c || c->tagClass != ast::TagClass::private_ || c->mode != ast::TagMode::explicit_ ||
      !cNumber || cNumber->name != "tag" || !cNumber->moduleRef.empty() ||
      !inner || !inner->explicitEncodingReference || !integer(inner->number, "8") ||
      !cursiv::ast_cast<ast::BooleanType>(inner->inner))
    return failed("tag class/number/written mode/nesting/constraints", source);
  const auto* r = cursiv::ast_cast<ast::CompositeRealValue>(value(module, "r"));
  const auto* binary = cursiv::ast_cast<ast::CompositeRealValue>(value(module, "binary"));
  const auto* external = cursiv::ast_cast<ast::ValueAssignment>(find(module, "external"));
  const auto* externalType = external ? cursiv::ast_cast<ast::TaggedType>(external->type) : nullptr;
  const auto* externalTag = externalType ? cursiv::ast_cast<ast::NameValue>(externalType->number) : nullptr;
  const auto* externalValue = external ? cursiv::ast_cast<ast::CompositeRealValue>(external->value) : nullptr;
  const auto* externalMantissa = externalValue ? cursiv::ast_cast<ast::NameValue>(externalValue->mantissa) : nullptr;
  const auto* externalBase = externalValue ? cursiv::ast_cast<ast::NameValue>(externalValue->base) : nullptr;
  const auto* externalExponent = externalValue ? cursiv::ast_cast<ast::NameValue>(externalValue->exponent) : nullptr;
  if (!r || !integer(r->mantissa, "-25") || !integer(r->base, "10") ||
      !integer(r->exponent, "-1") || !binary || !integer(binary->base, "2") ||
      !integer(binary->exponent, "-1074") || !externalTag ||
      externalTag->moduleRef != "Other" || externalTag->name != "tagNumber" ||
      !externalMantissa || externalMantissa->moduleRef != "Other" || externalMantissa->name != "m" ||
      !externalBase || externalBase->moduleRef != "Other" || externalBase->name != "b" ||
      !externalExponent || externalExponent->moduleRef != "Other" || externalExponent->name != "e")
    return failed("exact REAL components and external defined-value names", source);
  const auto* sequence = cursiv::ast_cast<ast::SequenceType>(type(module, "D"));
  const auto* defaultValue = sequence && sequence->components.size() == 2
      ? cursiv::ast_cast<ast::CompositeRealValue>(sequence->components[0]->default_) : nullptr;
  const auto* choiceTag = sequence && sequence->components.size() == 2
      ? cursiv::ast_cast<ast::TaggedType>(sequence->components[1]->item->type) : nullptr;
  const auto* plus = cursiv::ast_cast<ast::SpecialRealValue>(value(module, "positive"));
  const auto* minus = cursiv::ast_cast<ast::SpecialRealValue>(value(module, "negative"));
  const auto* nan = cursiv::ast_cast<ast::SpecialRealValue>(value(module, "nan"));
  const auto* oid = cursiv::ast_cast<ast::ObjectIdentifierValue>(value(module, "oid"));
  const auto* ordinary = cursiv::ast_cast<ast::CompositeRealValue>(value(module, "ordinary"));
  if (!defaultValue || !integer(defaultValue->mantissa, "-25") ||
      !choiceTag || choiceTag->mode != ast::TagMode::none_ ||
      !cursiv::ast_cast<ast::ChoiceType>(choiceTag->inner) ||
      !plus || plus->kind != ast::SpecialReal::plusInfinity ||
      !minus || minus->kind != ast::SpecialReal::minusInfinity ||
      !nan || nan->kind != ast::SpecialReal::notANumber ||
      !integer(value(module, "zero"), "0") || !integer(value(module, "minusZero"), "-0") ||
      !ordinary || !integer(ordinary->base, "3") ||
      !oid || oid->components.size() != 2 || oid->components[0]->name != "mantissa" ||
      oid->components[1]->value != "1")
    return failed("defaults, default CHOICE mode, specials/zero and contextual OID names", source);
  if (!cursiv::test::validationHelper(language, source).diagnostics.empty())
    return failed("valid tags/REAL or ordinary base3 sequence received a diagnostic", source);

  for (const char* body : {
           "T ::= [TAG: APPLICATION 214748364812345678901234567890] INTEGER",
           "T ::= [0] IMPLICIT [1] EXPLICIT CHOICE { a INTEGER }",
           "r REAL ::= { mantissa 123456789012345678901234567890, base 10, exponent -999999999999999999999999999 }"}) {
    const std::string text = std::string("M DEFINITIONS EXPLICIT TAGS ::= BEGIN ") + body + " END";
    const auto result = language.parser->parse(text);
    if (!clean(result, text) || !parents(language, result) ||
        !cursiv::test::validationHelper(language, text).diagnostics.empty())
      return failed("unbounded native integer text or tagged CHOICE", text);
  }
  for (const char* body : {
           "T ::= [] INTEGER", "T ::= [-1] INTEGER", "T ::= [APPLICATION] INTEGER",
           "T ::= [CONTEXT 1] INTEGER", "T ::= [0] AUTOMATIC INTEGER",
           "T ::= [0 1] INTEGER", "T ::= [0]", "T ::= [0 IMPLICIT INTEGER",
           "v REAL ::= { mantissa 1, base 2, exponent }",
           "v REAL ::= { mantissa 1,, base 2, exponent 0 }",
           "v REAL ::= { mantissa 1, base 2, exponent 0, }",
           "v REAL ::= { mantissa 1, base 2, exp 0 }",
           "v REAL ::= { base 2, mantissa 1, exponent 0 }"}) {
    const std::string text = std::string("M DEFINITIONS ::= BEGIN ") + body + " END";
    const auto result = language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return failed("malformed tag/composite accepted clean", text);
  }
  const std::pair<const char*, const char*> invalidValues[] = {
      {"T ::= [UNIVERSAL 3] INTEGER", "asn.universal-tag"},
      {"T ::= [0] IMPLICIT CHOICE { a INTEGER }", "asn.implicit-choice"},
      {"v REAL ::= { mantissa 0, base 2, exponent 0 }", "asn.real-mantissa"},
      {"v [0] REAL ::= { mantissa -0, base 10, exponent 0 }", "asn.real-mantissa"},
      {"v REAL ::= { mantissa 1, base 3, exponent 0 }", "asn.real-base"},
      {"v REAL ::= { mantissa 1 base 2 exponent 0 }", "asn.real-value"},
      {"v REAL ::= TRUE", "asn.real-value"},
      {"T ::= SEQUENCE { a [0] REAL DEFAULT { mantissa 1, base 4, exponent 0 } }", "asn.real-base"},
      {"C ::= CLASS { &v REAL DEFAULT { mantissa 1, base 4, exponent 0 } }", "asn.real-base"}};
  for (const auto& [body, code] : invalidValues) {
    const std::string text = std::string("M DEFINITIONS ::= BEGIN ") + body + " END";
    const auto result = language.parser->parse(text);
    if (!clean(result, text) || !parents(language, result)) return false;
    bool found = false;
    for (const auto& diagnostic : cursiv::test::validationHelper(language, text).diagnostics)
      found |= diagnostic.code && diagnostic.code->isString() && diagnostic.code->string() == code;
    if (!found) return failed(std::string("missing diagnostic ") + code, text);
  }
  return true;
}
}  // namespace asn_tests
