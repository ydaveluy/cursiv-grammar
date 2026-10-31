#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "lpc/core/CoreServices.hpp"
#include "lpc/core/ast.hpp"

namespace lpc_heredoc_cases {
struct Case {
  const char* name;
  std::string_view source;
  bool valid;
  bool array;
  std::string_view literal;
  std::vector<std::string_view> pieces;
};
inline const Case cases[] = {
  {"text-basic", "mixed f() { return @END\nhello\nworld\nEND; }\n", true, false, "@END\nhello\nworld\nEND", {"@END\nhello\nworld\nEND"}},
  {"text-empty", "mixed f() { return @END\nEND; }\n", true, false, "@END\nEND", {"@END\nEND"}},
  {"text-crlf", "mixed f() { return @END\r\nfirst\r\nsecond\r\nEND; }\n", true, false, "@END\r\nfirst\r\nsecond\r\nEND", {"@END\r\nfirst\r\nsecond\r\nEND"}},
  {"text-longer-id", "mixed f() { return @END\nENDING\nEND_\nEND0\nEND; }\n", true, false, "@END\nENDING\nEND_\nEND0\nEND", {"@END\nENDING\nEND_\nEND0\nEND"}},
  {"text-raw", "mixed f() { return @RAW\n\"quoted\" \\n \\t /* raw */\nRAW; }\n", true, false, "@RAW\n\"quoted\" \\n \\t /* raw */\nRAW", {"@RAW\n\"quoted\" \\n \\t /* raw */\nRAW"}},
  {"text-tag-digits", "mixed f() { return @123_x\nbody\n123_x; }\n", true, false, "@123_x\nbody\n123_x", {"@123_x\nbody\n123_x"}},
  {"text-header-space", "mixed f() { return @END \t\v\f\r\nbody\nEND; }\n", true, false, "@END \t\v\f\r\nbody\nEND", {"@END \t\v\f\r\nbody\nEND"}},
  {"text-trailing-blank", "mixed f() { return @END\n\nline\n\nEND; }\n", true, false, "@END\n\nline\n\nEND", {"@END\n\nline\n\nEND"}},
  {"array-basic", "mixed f() { return @@END\none\ntwo\nEND; }\n", true, true, "@@END\none\ntwo\nEND", {}},
  {"array-empty", "mixed f() { return @@END\nEND; }\n", true, true, "@@END\nEND", {}},
  {"array-crlf", "mixed f() { return @@END\r\none\r\ntwo\r\nEND; }\n", true, true, "@@END\r\none\r\ntwo\r\nEND", {}},
  {"array-raw", "mixed f() { return @@END\n\"quoted\"\n\\n\nEND; }\n", true, true, "@@END\n\"quoted\"\n\\n\nEND", {}},
  {"array-blank-lines", "mixed f() { return @@END\n\na\n\nEND; }\n", true, true, "@@END\n\na\n\nEND", {}},
  {"array-longer-id", "mixed f() { return @@END\nENDING\nEND_\nEND; }\n", true, true, "@@END\nENDING\nEND_\nEND", {}},
  {"text-adjacent", "mixed f() { return \"before\" @END\nbody\nEND \"after\"; }\n", true, false, "@END\nbody\nEND", {"\"before\"", "@END\nbody\nEND", "\"after\""}},
  {"array-call-argument", "mixed f() { return sizeof(@@END\na\nb\nEND); }\n", true, true, "@@END\na\nb\nEND", {}},
  {"missing-close", "mixed f() { return @END\nbody\n; }\n", false, false, "@END\nbody\n", {}},
  {"wrong-close", "mixed f() { return @END\nbody\nOTHER; }\n", false, false, "@END\nbody\nOTHER", {}},
  {"indented-close", "mixed f() { return @END\nbody\n END; }\n", false, false, "@END\nbody\n END", {}},
  {"case-close", "mixed f() { return @@End\na\nEND; }\n", false, false, "@@End\na\nEND", {}},
  {"empty-tag", "mixed f() { return @\nbody\nEND; }\n", false, false, "@\nbody\nEND", {}},
};
inline bool run(const lpc::LpcCoreServices& services) {
  unsigned failures = 0;
  auto check = [&](bool ok, const char* name, const char* contract) {
    if (!ok) { std::fprintf(stderr, "lpc heredoc %s: %s\n", name, contract); ++failures; }
  };
  for (const auto& test : cases) {
    if (std::string_view(test.name).find("crlf") != std::string_view::npos)
      check(test.source.find("\r\n") != std::string_view::npos &&
            test.literal.find("\r\n") != std::string_view::npos,
            test.name, "runtime input really retains CRLF, not compiler-normalized LF");
    if (std::string_view(test.name) == "text-header-space")
      check(test.source.find("\t\v\f\r\n") != std::string_view::npos,
            test.name, "all header whitespace bytes really reach the parser");
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() &&
        parsed.diagnostics.empty() && parsed.value;
    check(clean == test.valid, test.name, "native delimiter verdict");
    if (!test.valid || !clean) continue;
    check(parsed.cst().text() == test.source, test.name, "complete source retained");
    const lpc::ast::ArrayHeredocExpr* array = nullptr;
    const lpc::ast::StringLit* string = nullptr;
    unsigned arrayCount = 0, stringCount = 0;
    for (const auto* node : parsed.nodes) {
      if (const auto* value = cursiv::ast_cast<lpc::ast::ArrayHeredocExpr>(node)) {
        array = value; ++arrayCount;
      }
      if (const auto* value = cursiv::ast_cast<lpc::ast::StringLit>(node)) {
        string = value; ++stringCount;
      }
    }
    if (test.array) {
      check(arrayCount == 1 && array && array->text == test.literal && !stringCount,
            test.name, "array lexical node preserves exact opener/body/closer");
      check(array && (cursiv::ast_cast<lpc::ast::ReturnStmt>(array->parent) ||
            cursiv::ast_cast<lpc::ast::Arg>(array->parent)),
            test.name, "array belongs to its expression context");
    } else {
      check(!arrayCount && stringCount == 1 && string && string->pieces.size() == test.pieces.size(),
            test.name, "text pieces remain ordinary concatenated string literals");
      if (string && string->pieces.size() == test.pieces.size())
        for (std::size_t i = 0; i < test.pieces.size(); ++i)
          check(string->pieces[i] == test.pieces[i], test.name, "piece spelling exact, without escape evaluation");
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false, test.name, "unique ownership"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() &&
            child.cstNodeId < parsed.cst().size() &&
            parsed.cst().contains(node->cstNodeId, child.cstNodeId),
            test.name, "immediate parent and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent,
          test.name, "every allocated node reachable from root exactly once");
  }
  return !failures;
}
} // namespace lpc_heredoc_cases
