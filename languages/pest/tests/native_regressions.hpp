#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "pest/core/ast.hpp"

inline bool native_regressions(const auto& services) {
  namespace ast = pest::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {std::fprintf(stderr, "pest: %s\n", message);ok = false;}
  };
  const std::string source = R"(
    //! Grammar documentation.
    /// A tagged rule.
    r = !{ | #PUSH_tag = !&PUSH_LITERAL("\xFF\u{123}")? ~ PEEK ~ PEEK[-0001..02]
           | ^ /* retained comment */ "Case" ~ 'α' /* gap */ .. '\u{10FFFF}' }
    limits = { "a"{0002} ~ "b"{0,} ~ "c"{,4} ~ "d"{3,2} }
  )";
  const auto result = services.parser->parse(source);
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(), "native forms parse cleanly");
  const auto* doc = cursiv::ast_cast<ast::Document>(result.value);
  check(doc && doc->rules.size() == 2, "both rules retained");
  if (doc && doc->rules.size() == 2) {
    const auto* r = doc->rules[0];
    check(r->modifier == ast::Modifier::non_atomic && r->expression &&
          r->expression->alternatives.size() == 2, "modifier and leading-choice alternatives");
    if (r->expression && r->expression->alternatives.size() == 2) {
      const auto& first = r->expression->alternatives[0]->terms;
      const auto& second = r->expression->alternatives[1]->terms;
      check(first.size() == 3 && second.size() == 2, "sequence terms");
      if (first.size() == 3) {
        const auto* push = cursiv::ast_cast<ast::PushLiteral>(first[0]->item);
        const auto* peek = cursiv::ast_cast<ast::RuleRef>(first[1]->item);
        const auto* slice = cursiv::ast_cast<ast::PeekSlice>(first[2]->item);
        check(first[0]->tag && *first[0]->tag == "PUSH_tag" && first[0]->prefixes.size() == 2 &&
              first[0]->prefixes[0] == ast::PrefixOp::negative_predicate &&
              first[0]->prefixes[1] == ast::PrefixOp::positive_predicate,
              "tag and ordered predicates retained");
        check(push && push->text == R"("\xFF\u{123}")" && first[0]->postfixes.size() == 1 &&
              std::holds_alternative<ast::Optional_*>(first[0]->postfixes[0]), "literal push and optional postfix");
        check(peek && peek->name == "PEEK", "bare PEEK is a reference");
        check(slice && slice->start && *slice->start == "-0001" && slice->end && *slice->end == "02",
              "signed indices retain spelling");
      }
      if (second.size() == 2) {
        const auto* text = cursiv::ast_cast<ast::InsensitiveStringLit>(second[0]->item);
        const auto* range = cursiv::ast_cast<ast::CharRange>(second[1]->item);
        check(text && text->text == "\"Case\"", "insensitive string excludes interstitial trivia");
        check(range && range->start == "'α'" && range->end == R"('\u{10FFFF}')", "range bounds remain separate literals");
      }
    }
    const auto* limits = doc->rules[1]->expression;
    check(limits && limits->alternatives.size() == 1 && limits->alternatives[0]->terms.size() == 4,
          "all counted repetition shapes");
    if (limits && limits->alternatives.size() == 1 && limits->alternatives[0]->terms.size() == 4) {
      const auto* exact = std::get<ast::RepeatExact*>(limits->alternatives[0]->terms[0]->postfixes[0]);
      check(exact && exact->count == "0002", "count retains leading zeroes");
    }
  }
  for (std::string text : {"", "//! documentation\n/// rule docs\n", "r={|\"a\"|\"b\"}",
                          "r={\"line\nnext\"}", "r={ '\\xFF' .. '\\u{10FFFF}' }",
                          "r={PEEK[-2147483648..2147483647]}", "r={PUSH_LITERAL(\"a\")~PEEK}",
                          "SYMBOL={\"a\"} r={SYMBOL~XID_START~EMOJI}",
                          "r={\"a\"{4294967295}~\"b\"{0,}~\"c\"{3,2}}"}) {
    const auto valid = cursiv::test::validationHelper(services, text);
    check(valid.diagnostics.empty(), "valid form has no false runtime error");
  }
  for (std::string text : {R"(r={"\q"})", R"(r={"\X41"})", R"(r={"\U{41}"})",
                          R"(r={"\u{1}"})", R"(r={"\u{D800}"})", R"(r={"\u{110000}"})",
                          "r={''..'z'}", "r={'ab'..'z'}", "r={PEEK[-0..]}",
                          "r={PEEK[2147483648..]}", "r={\"a\"{0}}", "r={\"a\"{,0}}",
                          "r={\"a\"{2,0}}", "r={\"a\"{4294967296}}",
                          "r={\"a\"} r={\"b\"}", "r={missing}", "PUSH_name={\"a\"}",
                          "ANY={\"a\"}"}) {
    const auto parsed = services.parser->parse(text);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(), "invalid value reaches runtime validation");
    const auto invalid = cursiv::test::validationHelper(services, text);
    check(invalid.diagnostics.size() == 1, "one precise runtime error");
    if (invalid.diagnostics.size() == 1)
      check(invalid.diagnostics[0].range.begin < invalid.diagnostics[0].range.end &&
            invalid.diagnostics[0].range.end <= text.size(), "diagnostic selects source text");
  }
  return ok;
}
