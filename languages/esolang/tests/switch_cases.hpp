#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "lolcode/core/CoreServices.hpp"
#include "lolcode/core/LolcodeGrammar.hpp"

namespace lolcode_switch_cases {
namespace a = lolcode::ast;

struct Case {
  const char* name;
  std::string_view source;
  bool syntax;
  std::string_view validationCode;
};

// Original witnesses, shared with the separately executed official parser
// oracle in lolcode-switch-native-inputs28.json. ::{ is a documented lci
// divergence; duplicate labels/interpolation are local validation failures.
inline constexpr Case cases[] = {
    {"minimal-empty-case", R"lol(HAI 1.2
1
WTF?
OMG 1
OIC
KTHXBYE
)lol", true, ""},
    {"fallthrough-and-default", R"lol(HAI 1.2
"fall"
WTF?
OMG "first"
VISIBLE "first"
GTFO
OMG "fall"
VISIBLE "fall"
OMG "last"
VISIBLE "last"
GTFO
OMGWTF
VISIBLE "default"
OIC
KTHXBYE
)lol", true, ""},
    {"comma-separated", R"lol(HAI 1.2
WIN, WTF?, OMG WIN, VISIBLE "yes", GTFO, OMGWTF, OIC
KTHXBYE
)lol", true, ""},
    {"negative-numbar", R"lol(HAI 1.2
-1.25
WTF?
OMG -1.25
VISIBLE "match"
GTFO
OMG 2.5
OIC
KTHXBYE
)lol", true, ""},
    {"troof-labels", R"lol(HAI 1.2
WIN
WTF?
OMG WIN
GTFO
OMG FAIL
OIC
KTHXBYE
)lol", true, ""},
    {"quoted-colon-escape", R"lol(HAI 1.2
"a:"b"
WTF?
OMG "a:"b"
GTFO
OIC
KTHXBYE
)lol", true, ""},
    {"escaped-colon-before-brace", R"lol(HAI 1.2
"::{x}"
WTF?
OMG "::{x}"
GTFO
OIC
KTHXBYE
)lol", true, ""},
    {"hex-escape-constant", R"lol(HAI 1.2
"A"
WTF?
OMG ":(41)"
GTFO
OIC
KTHXBYE
)lol", true, ""},
    {"named-escape-constant", R"lol(HAI 1.2
"A"
WTF?
OMG ":[LATIN CAPITAL LETTER A]"
GTFO
OIC
KTHXBYE
)lol", true, ""},
    {"nested-switch", R"lol(HAI 1.2
1
WTF?
OMG 1
2
WTF?
OMG 2
GTFO
OIC
VISIBLE "outer"
GTFO
OIC
KTHXBYE
)lol", true, ""},
    {"nested-if", R"lol(HAI 1.2
1
WTF?
OMG 1
WIN
O RLY?
YA RLY
VISIBLE "yes"
NO WAI
VISIBLE "no"
OIC
GTFO
OIC
KTHXBYE
)lol", true, ""},
    {"switch-in-loop", R"lol(HAI 1.2
IM IN YR outer
1
WTF?
OMG 1
GTFO
OIC
GTFO
IM OUTTA YR outer
KTHXBYE
)lol", true, ""},
    {"switch-in-function", R"lol(HAI 1.2
HOW IZ I choose YR x
x
WTF?
OMG 1
FOUND YR "one"
OMGWTF
GTFO
OIC
IF U SAY SO
KTHXBYE
)lol", true, ""},
    {"empty-default", R"lol(HAI 1.2
1
WTF?
OMG 1
OMGWTF
OIC
KTHXBYE
)lol", true, ""},
    {"comments-and-blank-lines", R"lol(HAI 1.2
1
WTF? BTW opener

OMG 1 BTW label
BTW body comment
VISIBLE "one" BTW output
GTFO

OMGWTF
BTW default comment
OIC
KTHXBYE
)lol", true, ""},
    {"duplicate-numbr", R"lol(HAI 1.2
1
WTF?
OMG 1
OMG 1
OIC
KTHXBYE
)lol", true, "lolcode.switch-duplicate"},
    {"duplicate-numbar", R"lol(HAI 1.2
1.25
WTF?
OMG 1.25
OMG 1.25
OIC
KTHXBYE
)lol", true, "lolcode.switch-duplicate"},
    {"duplicate-yarn", R"lol(HAI 1.2
"a"
WTF?
OMG "a"
OMG "a"
OIC
KTHXBYE
)lol", true, "lolcode.switch-duplicate"},
    {"duplicate-troof", R"lol(HAI 1.2
WIN
WTF?
OMG WIN
OMG WIN
OIC
KTHXBYE
)lol", true, "lolcode.switch-duplicate"},
    {"variable-label", R"lol(HAI 1.2
I HAS A x ITZ 1
1
WTF?
OMG x
OIC
KTHXBYE
)lol", false, ""},
    {"operator-label", R"lol(HAI 1.2
1
WTF?
OMG SUM OF 1 AN 2
OIC
KTHXBYE
)lol", false, ""},
    {"call-label", R"lol(HAI 1.2
HOW IZ I f
FOUND YR 1
IF U SAY SO
1
WTF?
OMG I IZ f MKAY
OIC
KTHXBYE
)lol", false, ""},
    {"interpolated-yarn-label", R"lol(HAI 1.2
I HAS A x ITZ 1
"1"
WTF?
OMG ":{x}"
OIC
KTHXBYE
)lol", true, "lolcode.switch-interpolation"},
    {"default-not-last", R"lol(HAI 1.2
1
WTF?
OMG 1
OMGWTF
OMG 2
OIC
KTHXBYE
)lol", false, ""},
    {"duplicate-default", R"lol(HAI 1.2
1
WTF?
OMG 1
OMGWTF
OMGWTF
OIC
KTHXBYE
)lol", false, ""},
    {"missing-command-separator", R"lol(HAI 1.2
1
WTF? OMG 1
OIC
KTHXBYE
)lol", false, ""},
    {"missing-oic", R"lol(HAI 1.2
1
WTF?
OMG 1
VISIBLE "one"
KTHXBYE
)lol", false, ""},
    {"explicit-selector-not-allowed", R"lol(HAI 1.2
1
WTF? 1
OMG 1
OIC
KTHXBYE
)lol", false, ""},
};

inline bool run(const lolcode::LolcodeCoreServices& services) {
  bool good = true;
  const auto check = [&](bool condition, const char* name, const char* message) {
    if (!condition) {
      good = false;
      std::fprintf(stderr, "lolcode switch [%s]: %s\n", name, message);
    }
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() &&
                       parsed.diagnostics.empty();
    check(clean == test.syntax, test.name, "syntax expectation");
    if (clean != test.syntax)
      for (const auto& diagnostic : parsed.diagnostics)
        std::fprintf(stderr, "kind=%u element=%u bytes=%u..%u\n",
                     unsigned(diagnostic.kind), unsigned(diagnostic.element),
                     unsigned(diagnostic.begin), unsigned(diagnostic.end));
    if (!test.syntax || !clean)
      continue;

    check(parsed.cst().text() == test.source, test.name, "CST source retained");
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) {
        check(false, test.name, "duplicate AST edge");
        return;
      }
      check(node.cstNodeId < parsed.cst().size(), test.name, "node CST span");
      services.astReflection->visitMembers(node, [&](auto, auto, auto& child) {
        check(child.parent == &node, test.name, "exact member parent");
        check(node.cstNodeId < parsed.cst().size() &&
                  child.cstNodeId < parsed.cst().size() &&
                  parsed.cst().contains(node.cstNodeId, child.cstNodeId),
              test.name, "child CST containment");
        self(self, child);
      });
    };
    if (parsed.value) {
      check(parsed.value->parent == nullptr, test.name, "root parent");
      walk(walk, *parsed.value);
    }
    check(seen.size() == parsed.nodes.size(), test.name,
          "every allocated node reachable exactly once");

    const auto* program = parsed.value_as<a::Program>();
    check(program != nullptr, test.name, "typed Program");
    const a::SwitchStmt* first = nullptr;
    unsigned switches = 0;
    for (const auto* node : parsed.nodes)
      if (const auto* statement = cursiv::ast_cast<a::SwitchStmt>(node)) {
        if (!first) first = statement;
        ++switches;
      }
    check(first && switches == (std::string_view(test.name) == "nested-switch" ? 2u : 1u),
          test.name, "switch count and typed node");
    if (first) {
      check(!first->cases.empty(), test.name, "at least one literal case");
      const std::string_view name = test.name;
      if (name == "minimal-empty-case")
        check(first->cases.size() == 1 && first->cases[0]->literal == "1" &&
                  first->cases[0]->body.empty() && !first->hasDefault &&
                  first->defaultBody.empty(),
              test.name, "empty case and absent default");
      if (name == "empty-default" || name == "comma-separated")
        check(first->hasDefault && first->defaultBody.empty(), test.name,
              "present empty default retained");
      if (name == "fallthrough-and-default") {
        const bool sizes = first->cases.size() == 3 &&
                           first->cases[0]->body.size() == 2 &&
                           first->cases[1]->body.size() == 1 &&
                           first->cases[2]->body.size() == 2;
        check(sizes && first->hasDefault && first->defaultBody.size() == 1,
              test.name, "ordered case bodies and default body");
        if (sizes) {
          check(first->cases[0]->literal == "\"first\"" &&
                    first->cases[1]->literal == "\"fall\"" &&
                    first->cases[2]->literal == "\"last\"",
                test.name, "raw quoted labels keep their order");
          check(cursiv::ast_cast<a::BreakStmt>(first->cases[0]->body[1]) &&
                    cursiv::ast_cast<a::PrintBlock>(first->cases[1]->body[0]) &&
                    cursiv::ast_cast<a::BreakStmt>(first->cases[2]->body[1]),
                test.name, "GTFO and fallthrough are ordinary ordered statements");
        }
      }
      if (name == "negative-numbar")
        check(first->cases.size() == 2 && first->cases[0]->literal == "-1.25" &&
                  first->cases[1]->literal == "2.5",
              test.name, "signed decimal label spelling retained");
      if (name == "troof-labels")
        check(first->cases.size() == 2 && first->cases[0]->literal == "WIN" &&
                  first->cases[1]->literal == "FAIL",
              test.name, "boolean label spelling retained");
      if (name == "escaped-colon-before-brace")
        check(first->cases[0]->literal == "\"::{x}\"", test.name,
              "quoted colon is constant text, unlike active interpolation");
    }

    auto validated = cursiv::test::validationHelper(services, std::string(test.source));
    if (test.validationCode.empty()) {
      check(validated.diagnostics.empty(), test.name, "no validation issues");
    } else {
      check(validated.diagnostics.size() == 1 &&
                validated.diagnostics[0].code &&
                *validated.diagnostics[0].code == test.validationCode,
            test.name, "one precise local validation diagnostic");
    }
    validated.dispose();
  }

  // Switch labels and ordinary numeric expressions both preserve raw text.
  // Distinct int64 spellings must never become duplicate labels through an
  // approximate numeric comparison.
  const std::string precise =
      "HAI 1.2\nWTF?\nOMG 9007199254740992\nOMG 9007199254740993\nOIC\nKTHXBYE\n";
  const auto parsed = services.parser->parse(precise);
  const auto* program = parsed.value_as<a::Program>();
  const auto* statement = program && program->statements.size() == 1
      ? cursiv::ast_cast<a::SwitchStmt>(program->statements[0]) : nullptr;
  check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
            statement && statement->cases.size() == 2 &&
            statement->cases[0]->literal == "9007199254740992" &&
            statement->cases[1]->literal == "9007199254740993",
        "raw-integer-labels", "integer graphies remain distinct beyond double precision");
  auto validated = cursiv::test::validationHelper(services, precise);
  check(validated.diagnostics.empty(), "raw-integer-labels", "no approximate duplicate error");
  validated.dispose();

  // Converter totality is a direct contract. These malformed fragments do
  // not claim that any particular recovery path supplies them at runtime.
  for (std::string_view text : {"", "\"", "a", "abc", "\"a", "a\"", "'a'"}) {
    const auto result = lolcode::unquote(text);
    check(!result && result.error.kind == cursiv::DiagnosticKind::conversion_error &&
              !result.error.message.empty(),
          "unquote-direct", "malformed fragment returns conversion_error");
  }
  for (const auto& [source, expected] : {
           std::pair<std::string_view, std::string_view>{R"lol("")lol", ""},
           {R"lol("a:"b")lol", R"lol(a:"b)lol"},
           {R"lol("::{x}")lol", "::{x}"},
           {R"lol("a\b")lol", R"lol(a\b)lol"}}) {
    const auto result = lolcode::unquote(source);
    check(result && *result.value == expected, "unquote-direct", "interior spelling retained");
  }
  for (std::string_view source : {"HAI 1.2\nVISIBLE \"\nKTHXBYE\n",
                                  "HAI 1.2\nWTF?\nOMG \"unterminated\nKTHXBYE\n",
                                  "HAI 1.2\nWTF?\nOMG\nOIC\nKTHXBYE\n"}) {
    const auto result = services.parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "malformed-string-recovery", "broken document does not parse clean");
  }
  std::puts("lolcode switch: 28 witnesses, raw labels, ownership, validation and converter contracts checked");
  return good;
}
} // namespace lolcode_switch_cases
