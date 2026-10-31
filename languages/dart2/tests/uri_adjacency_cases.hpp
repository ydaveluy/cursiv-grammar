#pragma once

#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "dart2/core/ast.hpp"

namespace dart2_uri_adjacency {

inline bool run(const auto& language) {
  namespace a = dart2::ast;
  bool ok = true;
  auto check = [&](bool condition, const char* reason, std::string_view source) {
    if (!condition) {
      std::fprintf(stderr, "dart URI: %s; source=%.*s\n", reason, int(source.size()), source.data());
      ok = false;
    }
  };
  auto owned = [&](const cursiv::ParseResult& result) {
    if (!result.value) return false;
    bool valid = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *result.value);
    return valid && seen.size() == result.nodes.size();
  };
  auto strings = [](const a::StringValue* value,
                     std::initializer_list<std::string_view> expected,
                     bool rawFirst = false, bool multilineFirst = false) {
    std::vector<const a::StringLit*> parts;
    if (const auto* single = cursiv::ast_cast<a::StringLit>(value)) parts.push_back(single);
    else if (const auto* adjacent = cursiv::ast_cast<a::AdjacentStrings>(value))
      for (const auto* part : adjacent->parts) parts.push_back(part);
    if (parts.size() != expected.size() ||
        (expected.size() == 1 && !cursiv::ast_cast<a::StringLit>(value)) ||
        (expected.size() > 1 && !cursiv::ast_cast<a::AdjacentStrings>(value))) return false;
    unsigned i = 0;
    for (const auto text : expected) {
      const auto* literal = parts[i];
      if (!literal || literal->raw != (i == 0 && rawFirst) ||
          literal->multiline != (i == 0 && multilineFirst)) return false;
      std::string actual;
      for (const auto& part : literal->parts) {
        const auto* segment = std::get_if<std::string_view>(&part);
        if (!segment) return false;
        actual += *segment;
      }
      if (actual != text) return false;
      ++i;
    }
    return true;
  };
  struct Case { const char* name; std::string_view source; };
  constexpr Case positives[] = {
      {"import", "import 'a' '.dart';"},
      {"export", "export 'a' /*join*/ '.dart' show A;"},
      {"part", "part 'a' '.dart';"},
      {"partof", "part of 'a' '.dart';"},
      {"config", "import 'base.dart' if (dart.library.io == 'tr' 'ue') 'io' '.dart' as io;"},
      {"configs", "import 'a' '.dart' if(flag) 'b' '.dart' if(second == r'tr' 'ue') r'c' '.dart';"},
      {"multiline", "import '''a''' '.dart';"},
      {"single", "import 'single.dart';"},
      {"named", "part of a.b;"},
  };
  for (const auto& test : positives) {
    const auto result = language.parser->parse(test.source);
    const auto* unit = cursiv::ast_cast<a::CompilationUnit>(result.value);
    bool correct = result.accepted && !result.cst().recovered() &&
                   result.diagnostics.empty() && unit;
    const std::string_view name = test.name;
    if (unit && name == "export")
      correct &= unit->exports.size() == 1 && strings(unit->exports[0]->uri, {"a", ".dart"}) &&
                 unit->exports[0]->combinators.size() == 1 &&
                 !unit->exports[0]->combinators[0]->isHide &&
                 unit->exports[0]->combinators[0]->names.size() == 1 &&
                 unit->exports[0]->combinators[0]->names[0] == "A";
    else if (unit && name == "part")
      correct &= unit->parts.size() == 1 && strings(unit->parts[0]->uri, {"a", ".dart"});
    else if (unit && name == "partof")
      correct &= unit->partOf && unit->partOf->nameParts.empty() &&
                 strings(unit->partOf->uri, {"a", ".dart"});
    else if (unit && name == "named")
      correct &= unit->partOf && !unit->partOf->uri && unit->partOf->nameParts.size() == 2 &&
                 unit->partOf->nameParts[0] == "a" && unit->partOf->nameParts[1] == "b";
    else if (unit && unit->imports.size() == 1) {
      const auto* directive = unit->imports[0];
      if (name == "single") correct &= strings(directive->uri, {"single.dart"});
      else if (name == "multiline") correct &= strings(directive->uri, {"a", ".dart"}, false, true);
      else if (name == "config") {
        const auto* config = directive->configurations.size() == 1 ? directive->configurations[0] : nullptr;
        correct &= strings(directive->uri, {"base.dart"}) && directive->asName == "io" &&
                   config && config->testName.size() == 3 && config->testName[0] == "dart" &&
                   config->testName[1] == "library" && config->testName[2] == "io" &&
                   strings(config->testValue, {"tr", "ue"}) && strings(config->uri, {"io", ".dart"});
      } else if (name == "configs") {
        const auto* first = directive->configurations.size() == 2 ? directive->configurations[0] : nullptr;
        const auto* second = directive->configurations.size() == 2 ? directive->configurations[1] : nullptr;
        correct &= strings(directive->uri, {"a", ".dart"}) && first && second &&
                   first->testName.size() == 1 && first->testName[0] == "flag" &&
                   !first->testValue && strings(first->uri, {"b", ".dart"}) &&
                   second->testName.size() == 1 && second->testName[0] == "second" &&
                   strings(second->testValue, {"tr", "ue"}, true) &&
                   strings(second->uri, {"c", ".dart"}, true);
      } else correct &= name == "import" && strings(directive->uri, {"a", ".dart"});
    } else correct = false;
    check(correct, "URI/test segments, concrete single node, raw/multiline flags and directive fields", test.source);
    check(owned(result), "complete unique AST/CST ownership", test.source);
  }
  {
    constexpr std::string_view source = "void f(){ part('a' '.dart'); }";
    const auto result = language.parser->parse(source);
    const a::CallExpr* call = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<a::CallExpr>(node)) call = value;
    const auto* callee = call ? cursiv::ast_cast<a::Ident>(call->callee) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          callee && callee->name == "part" && call->args && call->args->items.size() == 1 &&
          strings(cursiv::ast_cast<a::StringValue>(call->args->items[0]->value), {"a", ".dart"}) &&
          owned(result), "part remains a contextual call name inside a body", source);
  }
  {
    constexpr std::string_view source = "part(value) {}";
    const auto result = language.parser->parse(source);
    const auto* unit = cursiv::ast_cast<a::CompilationUnit>(result.value);
    const auto* function = unit && unit->declarations.size() == 1
        ? cursiv::ast_cast<a::TopLevelFunction>(unit->declarations[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          function && function->name == "part" && owned(result),
          "part remains a contextual top-level function name", source);
  }
  unsigned negatives = 0;
  for (const std::string_view source : {
      "import 'a' + '.dart';", "export 'a' 1;", "part ('a' '.dart');",
      "part of ('a' '.dart');", "import 'a' if(flag == true) 'b';",
      "import 'a' if(flag == 'tr' 1) 'b';", "import 'a' if(flag) 1;",
      "import 'a' '.dart'"}) {
    const auto result = language.parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "URI grammar admits only adjacent literals, not arbitrary expressions", source);
    ++negatives;
  }
  std::printf("dart URI: 11 positives, %u negatives, distinct string nodes and ownership\n", negatives);
  return ok;
}

} // namespace dart2_uri_adjacency
