#pragma once
#include <cstdio>
#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "ocl/core/CoreServices.hpp"
#include "ocl/core/OclGrammar.hpp"

namespace ocl_tuple_cases {
struct Case { const char* name; std::string_view source; bool valid; unsigned literals; unsigned types; bool duplicate; };
inline constexpr Case cases[] = {
  {"inferred", R"ocl(context C inv: Tuple{x=1, y=2})ocl", true, 1, 0, false},
  {"typed-and-inferred", R"ocl(context C inv: Tuple{a:Integer=1, b='z'})ocl", true, 1, 0, false},
  {"quoted-name", R"ocl(context C inv: Tuple{_'if'=1, _'a b':String='x'})ocl", true, 1, 0, false},
  {"nested-literal", R"ocl(context C inv: Tuple{left=Tuple{x=1}, right=2})ocl", true, 2, 0, false},
  {"postfix", R"ocl(context C inv: Tuple{x=1}.x = 1)ocl", true, 1, 0, false},
  {"collection-field", R"ocl(context C inv: Tuple{a:Collection(Integer)=Set{1,2}})ocl", true, 1, 0, false},
  {"conditional-field", R"ocl(context C inv: Tuple{x=if true then 1 else 2 endif})ocl", true, 1, 0, false},
  {"let-type", R"ocl(context C inv: let t:Tuple(x:Integer,y:String)=Tuple{x=1,y='a'} in t.x = 1)ocl", true, 1, 1, false},
  {"empty-type", R"ocl(context C::x : Tuple() derive: invalid)ocl", true, 0, 1, false},
  {"nested-type", R"ocl(context C::x : Tuple(a:Tuple(b:Integer)) derive: Tuple{a=Tuple{b=1}})ocl", true, 2, 2, false},
  {"type-in-collection", R"ocl(context C::x : Set(Tuple(a:Integer)) derive: Set{Tuple{a=1}})ocl", true, 1, 1, false},
  {"duplicate-literal", R"ocl(context C inv: Tuple{x=1,x=2})ocl", true, 1, 0, true},
  {"duplicate-decoded-literal", R"ocl(context C inv: Tuple{x=1, _'\u0078'=2})ocl", true, 1, 0, true},
  {"duplicate-type", R"ocl(context C::x : Tuple(x:Integer,x:String) derive: invalid)ocl", true, 0, 1, true},
  {"empty-literal", R"ocl(context C inv: Tuple{})ocl", false, 0, 0, false},
  {"missing-value", R"ocl(context C inv: Tuple{x})ocl", false, 0, 0, false},
  {"missing-initializer", R"ocl(context C inv: Tuple{x:Integer})ocl", false, 0, 0, false},
  {"missing-type", R"ocl(context C inv: Tuple{x:=1})ocl", false, 0, 0, false},
  {"trailing-comma", R"ocl(context C inv: Tuple{x=1,})ocl", false, 0, 0, false},
  {"untyped-type", R"ocl(context C::x : Tuple(x) derive: invalid)ocl", false, 0, 0, false},
  {"initialized-type", R"ocl(context C::x : Tuple(x:Integer=1) derive: invalid)ocl", false, 0, 0, false},
  {"trailing-type", R"ocl(context C::x : Tuple(x:Integer,) derive: invalid)ocl", false, 0, 0, false},
};
inline bool run(const ocl::OclCoreServices& services) {
  namespace a = ocl::ast;
  unsigned failures = 0;
  auto check = [&](bool ok, const char* name, const char* what) {
    if (!ok) { std::fprintf(stderr, "ocl tuple %s: %s\n", name, what); ++failures; }
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean == test.valid, test.name, "tuple concrete syntax verdict");
    if (!clean || !test.valid) continue;
    check(parsed.cst().text() == test.source, test.name, "source retained exactly");
    unsigned literals = 0, types = 0;
    const a::TupleLiteral* literal = nullptr;
    for (const auto* node : parsed.nodes) {
      if (const auto* value = cursiv::ast_cast<a::TupleLiteral>(node)) { ++literals; literal = value; }
      if (cursiv::ast_cast<a::TupleType>(node)) ++types;
      if (const auto* value = cursiv::ast_cast<a::TuplePart>(node))
        check(value->value && value->value->parent == value && (!value->type || value->type->parent == value),
              test.name, "initializer and optional type roles owned");
    }
    check(literals == test.literals && types == test.types, test.name, "literal/type counts distinct");
    if (std::string_view(test.name) == "typed-and-inferred" && literal) {
      const auto* type = literal->parts.size() == 2 ? cursiv::ast_cast<a::NamedType>(literal->parts[0]->type) : nullptr;
      const auto* number = literal->parts.size() == 2 ? cursiv::ast_cast<a::IntLit>(literal->parts[0]->value) : nullptr;
      const auto* string = literal->parts.size() == 2 ? cursiv::ast_cast<a::StringLit>(literal->parts[1]->value) : nullptr;
      check(type && number && string && literal->parts[0]->name == "a" && type->name == "Integer" &&
            number->value == 1 && literal->parts[1]->name == "b" && !literal->parts[1]->type && string->value == "z",
            test.name, "ordered names/types/values and omitted-type distinction");
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false, test.name, "unique AST ownership"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() &&
              child.cstNodeId < parsed.cst().size() && parsed.cst().contains(node->cstNodeId, child.cstNodeId),
              test.name, "immediate parents and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent, test.name, "complete tree reachability");
    auto validated = cursiv::test::validationHelper(services, std::string(test.source));
    check(validated.diagnostics.size() == (test.duplicate ? 1u : 0u), test.name, "local uniqueness diagnostic count");
    if (test.duplicate && validated.diagnostics.size() == 1)
      check(validated.diagnostics[0].code && *validated.diagnostics[0].code == "ocl.tuple-duplicate",
            test.name, "exact decoded-name duplicate diagnostic");
    validated.dispose();
  }
  std::pmr::monotonic_buffer_resource memory;
  for (std::string_view source : {"", "a", "'", "'a", "a'", "'a\"", R"ocl('a\')ocl",
                                R"ocl('\u12')ocl", R"ocl('\xG0')ocl", R"ocl('\q')ocl"}) {
    const auto value = ocl::unescape(source, &memory);
    check(!value && !value.error.message.empty(), "converter-totality", "malformed input reports bounded conversion error");
  }
  for (const auto& pair : {
      std::pair<std::string_view, std::string_view>{"''", ""}, {R"ocl('\u0041')ocl", "A"},
      {R"ocl('\x41')ocl", "A"}, {R"ocl('\101')ocl", "A"},
      {R"ocl('\400')ocl", " 0"}, {R"ocl('\777')ocl", "?7"}}) {
    const auto value = ocl::unescape(pair.first, &memory);
    check(value && *value.value == pair.second, "converter-decoding", "escape widths follow declared terminal");
    const std::string source = "context C inv: " + std::string(pair.first);
    const auto parsed = services.parser->parse(source);
    const a::StringLit* string = nullptr;
    for (const auto* node : parsed.nodes) if (const auto* v = cursiv::ast_cast<a::StringLit>(node)) string = v;
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
          string && string->value == pair.second, "converter-decoding", "real parsed AST keeps exact decoded value");
  }
  for (std::string_view source : {"context C inv: Tuple{x='unterminated}", "context C inv: '"}) {
    const auto parsed = services.parser->parse(source);
    check(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty(),
          "malformed-string", "broken source does not parse clean");
  }
  return !failures;
}
} // namespace ocl_tuple_cases
