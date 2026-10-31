#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "swift/core/CoreServices.hpp"
#include "swift/core/ast.hpp"

namespace swift_raw_escapes {

inline bool run(const swift::SwiftCoreServices& services) {
  bool ok = true;
  auto check = [&](bool value, const char* reason, std::string_view source) {
    if (!value) {
      std::fprintf(stderr, "swift raw escape: %s; source=%.*s\n", reason,
                   int(source.size()), source.data());
      ok = false;
    }
  };
  auto owned = [&](const cursiv::ParseResult& result) {
    if (!result.value) return false;
    bool valid = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *result.value);
    return valid && seen.size() == result.nodes.size();
  };
  struct Case { std::string_view source; bool valid; };
  constexpr Case cases[] = {
      {"let s = #\"\\q\"#\n", true},
      {"let s = #\"a\\#nb\"#\n", true},
      {"let s = ##\"\\#q\"##\n", true},
      {"let s = ##\"\\##n\"##\n", true},
      {"let s = ##\"\\q\"##\n", true},
      {"let s = #\"\\#u{1F600}\"#\n", true},
      {"let s = #\"\\#u{0}\"#\n", true},
      {"let s = ##\"\\#u{D800}\"##\n", true},
      {"let s = #\"\\#\\\\#q\"#\n", false},
      {"let s = #\"a\\#\"#b\"#\n", true},
      {"let s = #\"\"\"\nline\\#\nnext\n\"\"\"#\n", true},
      {"let s = ##\"\"\"\n\\##u{1F600}\n\"\"\"##\n", true},
      {"let s = #\"\\#(1)\"#\n", true},
      {"let s = #\"\\#\\#(\"#\n", true},
      {"let s = #\"\\#q\"#\n", false},
      {"let s = ##\"\\##q\"##\n", false},
      {"let s = #\"\\##n\"#\n", false},
      {"let s = ##\"\\###n\"##\n", false},
      {"let s = #\"\\#u{D800}\"#\n", false},
      {"let s = #\"\\#u{110000}\"#\n", false},
      {"let s = #\"\\#u{000000000}\"#\n", false},
      {"let s = #\"\\#u{}\"#\n", false},
      {"let s = #\"\\#u{G}\"#\n", false},
      {"let s = #\"\\#u0061\"#\n", false},
      {"let s = #\"\\#u{123\"#\n", false},
      {"let s = ##\"\"\"\n\\##q\n\"\"\"##\n", false},
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const swift::ast::RawStringLit* literal = nullptr;
    unsigned rawCount = 0;
    for (const auto* node : parsed.nodes)
      if (const auto* value = cursiv::ast_cast<swift::ast::RawStringLit>(node)) {
        literal = value;
        ++rawCount;
      }
    constexpr std::string_view prefix = "let s = ";
    auto spelling = test.source.substr(prefix.size());
    spelling.remove_suffix(1); // Every fixture adds one final source newline.
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
          literal && rawCount == 1 && literal->text == spelling && owned(parsed),
          "raw spelling and complete unique AST/CST ownership retained", test.source);
    const auto validation = cursiv::test::validationHelper(services, std::string(test.source));
    bool escape = false;
    for (const auto& diagnostic : validation.diagnostics)
      escape |= diagnostic.code == "swift.string-escape";
    check(test.valid ? validation.diagnostics.empty() : escape,
          "active escape validity uses matching hash count and Unicode scalar bounds", test.source);
  }
  std::printf("swift raw escape: 13 positives, 13 negatives, exact spelling and ownership\n");
  return ok;
}

} // namespace swift_raw_escapes
