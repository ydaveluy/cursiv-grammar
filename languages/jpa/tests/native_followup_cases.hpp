#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "jpa/core/ast.hpp"
namespace jpa_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = jpa::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "jpa native: %s\n", message); ok = false; }
  };
  struct Numeric { const char* source; double value; };
  for (const auto& c : {
      Numeric{"SELECT e FROM Employee e WHERE e.x = .5", .5},
      Numeric{"SELECT e FROM Employee e WHERE e.x = 12L", 12},
      Numeric{"SELECT e FROM Employee e WHERE e.x = 1.e2D", 100},
      Numeric{"SELECT e FROM Employee e WHERE e.x = .25F", .25}}) {
    const auto result = language.language.parser->parse(c.source);
    const auto* document = result.template value_as<a::Document>();
    const auto* select = document ? cursiv::ast_cast<a::SelectStatement>(document->statement) : nullptr;
    const auto* relation = select ? cursiv::ast_cast<a::BinaryExpr>(select->where) : nullptr;
    const auto* number = relation ? cursiv::ast_cast<a::NumberLit>(relation->right) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          number && number->parent == relation &&
          number->value == c.value,
          "Java/SQL numeric literal shape and numeric value");
  }
  const auto parseCase = [&](std::string_view source, auto inspect) {
    const auto result = language.language.parser->parse(source);
    const auto* document = result.template value_as<a::Document>();
    const auto* select = document ? cursiv::ast_cast<a::SelectStatement>(document->statement) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && select,
          "native character/parameter case parses clean");
    if (select) inspect(*select);
  };
  for (const std::string_view alias : {
      "é", "Ⅻ", "￥", "‿", "a\u0301", "a\u0903", "a\u0661",
      "a\u200c", "a\x01", "SELECTé", "FROM\u0301"}) {
    const std::string source = "SELECT " + std::string(alias) +
        " FROM Société " + std::string(alias) + " WHERE " +
        std::string(alias) + ".naïve = :" + std::string(alias);
    parseCase(source, [&](const auto& select) {
      const auto* range = select.from.size() == 1
          ? cursiv::ast_cast<a::IdVarDecl>(select.from[0]) : nullptr;
      const auto* relation = cursiv::ast_cast<a::BinaryExpr>(select.where);
      const auto* path = relation ? cursiv::ast_cast<a::PathExpr>(relation->left) : nullptr;
      const auto* parameter = relation ? cursiv::ast_cast<a::InputParam>(relation->right) : nullptr;
      check(range && range->range && range->range->schemaName == "Société" &&
            range->range->alias == alias && range->range->parent == range &&
            path && path->segments.size() == 2 && path->segments[0] == alias &&
            path->segments[1] == "naïve" && path->parent == relation &&
            parameter && parameter->text == alias && !parameter->positional &&
            parameter->parent == relation,
            "Java identifier categories, keyword boundaries and preserved spelling");
    });
  }
  for (const std::string_view alias : {"\u0301a", "\u0661a", "😀", "a+"}) {
    const std::string source = "SELECT e FROM Employee " + std::string(alias);
    const auto result = language.language.parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "non-Java identifier start or part is rejected");
  }
  parseCase("SELECT e FROM Employee e WHERE e.name LIKE 'a' ESCAPE :escape", [&](const auto& select) {
    const auto* like = cursiv::ast_cast<a::LikeExpr>(select.where);
    const auto* parameter = like ? cursiv::ast_cast<a::InputParam>(like->escape) : nullptr;
    check(parameter && !parameter->positional && parameter->text == "escape" && parameter->parent == like,
          "ESCAPE character-valued input parameter");
  });
  parseCase("SELECT e FROM Employee e WHERE e.name LIKE 'a' ESCAPE ''''", [&](const auto& select) {
    const auto* like = cursiv::ast_cast<a::LikeExpr>(select.where);
    const auto* character = like ? cursiv::ast_cast<a::CharLit>(like->escape) : nullptr;
    check(character && character->value == "'", "doubled quote is a single native character");
  });
  parseCase(R"jpql(SELECT e FROM Employee e WHERE TRIM('\' FROM e.name) = e.name)jpql", [&](const auto& select) {
    const auto* relation = cursiv::ast_cast<a::BinaryExpr>(select.where);
    const auto* trim = relation ? cursiv::ast_cast<a::TrimExpr>(relation->left) : nullptr;
    const auto* character = trim ? cursiv::ast_cast<a::CharLit>(trim->trimChar) : nullptr;
    check(character && character->value == "\\", "backslash is literal in JPQL strings");
  });
  parseCase("SELECT $e FROM Employee $e WHERE TRIM(BOTH :trim FROM $e.name) = $e.name", [&](const auto& select) {
    const auto* relation = cursiv::ast_cast<a::BinaryExpr>(select.where);
    const auto* trim = relation ? cursiv::ast_cast<a::TrimExpr>(relation->left) : nullptr;
    const auto* parameter = trim ? cursiv::ast_cast<a::InputParam>(trim->trimChar) : nullptr;
    const auto* range = select.from.size() == 1 ? cursiv::ast_cast<a::IdVarDecl>(select.from[0]) : nullptr;
    check(parameter && parameter->text == "trim" && trim->spec == a::TrimSpec::both &&
          parameter->parent == trim && range && range->range && range->range->alias == "$e",
          "TRIM input parameter and Java dollar identifier");
  });
  for (const auto source : {"SELECT e FROM Employee e WHERE e.x = ?0",
                            "SELECT e FROM Employee e WHERE e.x = ?000",
                            "SELECT e FROM Employee e WHERE e.x = ?1 AND e.y = :y"}) {
    const auto validation = cursiv::test::validationHelper(language.language, source);
    check(validation.document->parseResult.accepted && !validation.document->parseResult.cst().recovered() &&
          validation.diagnostics.size() == 1 &&
          validation.diagnostics[0].code == (std::string_view(source).find(":y") != std::string_view::npos
              ? "jpa.parameter-style" : "jpa.parameter-index"), "parameter constraints independent of mappings");
  }
  const auto valid = cursiv::test::validationHelper(language.language,
      "SELECT e FROM Employee e WHERE e.x = ?01 OR e.x = ?1");
  check(valid.document->parseResult.accepted && !valid.document->parseResult.cst().recovered() &&
        valid.diagnostics.empty(), "one-based repeated positional parameter");
  const auto reserved = cursiv::test::validationHelper(language.language,
      "SELECT unknown FROM Employee unknown");
  check(reserved.document->parseResult.accepted && !reserved.document->parseResult.cst().recovered() &&
        reserved.diagnostics.size() == 1 && reserved.diagnostics[0].code == "jpa.alias-reserved",
        "case-insensitive reserved alias needs no entity mapping");
  const auto bad = language.language.parser->parse("SELECT e FROM Employee e WHERE e.x = 1.2L");
  check(!bad.accepted || bad.cst().recovered() || !bad.diagnostics.empty(), "long suffix only on integer syntax");
  return ok;
}
} // namespace jpa_native_followup
