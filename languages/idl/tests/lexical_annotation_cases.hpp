#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "idl/core/ast.hpp"

namespace idl_lexical_annotations {
inline bool run(const auto& services) {
  struct Case { const char* id; std::string_view source; bool valid; };
  const Case cases[] = {
      {"annotation-no-args", "@A struct S {long x;};", true},
      {"annotation-positional", "@A(1+2) struct S {long x;};", true},
      {"annotation-named", "@A(min=1+2,max=9) struct S {long x;};", true},
      {"annotation-single-named", "@A(value=1) struct S {long x;};", true},
      {"annotation-name-value", "@A(::Constants::value) struct S {long x;};", true},
      {"annotation-string", R"(@A("x" "y") struct S {long x;};)", true},
      {"escaped-char", R"(const char C = '\n';)", true},
      {"escaped-string", R"(const string S = "a\tb\rc\nd";)", true},
      {"escaped-wide", R"(const wstring S = L"\u3BC\n" L"x";)", true},
      {"empty-args", "@A() struct S {long x;};", false},
      {"two-positional", "@A(1,2) struct S {long x;};", false},
      {"positional-then-named", "@A(1,value=2) struct S {long x;};", false},
      {"named-then-positional", "@A(value=1,2) struct S {long x;};", false},
      {"trailing-comma", "@A(value=1,) struct S {long x;};", false},
      {"missing-argument-value", "@A(value=) struct S {long x;};", false},
      {"raw-LF-char", "const char C = '\n';", false},
      {"raw-CR-char", "const char C = '\r';", false},
      {"raw-tab-char", "const char C = '\t';", false},
      {"raw-LF-string", "const string S = \"a\nb\";", false},
      {"raw-CR-string", "const string S = \"a\rb\";", false},
      {"raw-tab-string", "const string S = \"a\tb\";", false},
      {"raw-LF-wide-char", "const wchar C = L'\n';", false},
      {"raw-LF-wide-string", "const wstring S = L\"a\nb\";", false},
      {"raw-C1-control", "const string S = \"a\xc2\x85" "b\";", false},
  };
  for (const auto& test : cases) {
    const auto r = services.parser->parse(test.source);
    const bool clean = r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "idl lexical/annotation: %s expected=%d accepted=%d recovered=%d diagnostics=%zu source=%.*s\n",
                   test.id, test.valid, r.accepted, r.cst().recovered(), r.diagnostics.size(),
                   int(test.source.size()), test.source.data());
      return false;
    }
    if (!clean) continue;
    if (!r.value || r.value->parent) {
      std::fprintf(stderr, "idl lexical/annotation: %s missing detached root\n", test.id);
      return false;
    }
    bool owned = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto describe = [&](const char* role, const cursiv::AstNode& node) {
      const auto type = services.astReflection->getTypeMetaData(node.type).name;
      std::fprintf(stderr, "idl lexical/annotation: %s %s type=%.*s cst=%u",
                   test.id, role, int(type.size()), type.data(), unsigned(node.cstNodeId));
      if (node.cstNodeId < r.cst().size()) {
        const auto& cst = r.cst()[node.cstNodeId];
        std::fprintf(stderr, " bytes=%u..%u", cst.begin, cst.end);
      }
      std::fputc('\n', stderr);
    };
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; describe("duplicate", node); return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view member, std::uint32_t index, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contained = node.cstNodeId < r.cst().size() && child.cstNodeId < r.cst().size() &&
                r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contained) {
              std::fprintf(stderr, "idl lexical/annotation: %s member=%.*s index=%u parent=%d contained=%d\n",
                           test.id, int(member.size()), member.data(), index, parent, contained);
              describe("owner", node); describe("child", child);
            }
            owned &= parent && contained;
            self(self, child);
          });
    };
    walk(walk, *r.value);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "idl lexical/annotation: %s ownership=%d seen=%zu arena=%zu\n",
                   test.id, owned, seen.size(), r.nodes.size());
      return false;
    }
    if (std::string_view(test.id).starts_with("annotation-")) {
      const auto* root = r.template value_as<idl::ast::Specification>();
      const auto* declaration = root && root->definitions.size() == 1
          ? cursiv::ast_cast<idl::ast::StructDecl>(root->definitions[0]) : nullptr;
      const auto* annotation = declaration && declaration->annotations.size() == 1
          ? declaration->annotations[0] : nullptr;
      const bool absent = std::string_view(test.id) == "annotation-no-args";
      const bool two = std::string_view(test.id) == "annotation-named";
      if (!annotation || annotation->name != "A" || annotation->hasArguments == absent ||
          annotation->arguments.size() != (absent ? 0u : two ? 2u : 1u)) {
        std::fprintf(stderr, "idl lexical/annotation: %s annotation presence/count AST lost\n", test.id);
        return false;
      }
      if (two && (annotation->arguments[0]->name != "min" || annotation->arguments[1]->name != "max" ||
                  !cursiv::ast_cast<idl::ast::BinaryExpr>(annotation->arguments[0]->value))) {
        std::fprintf(stderr, "idl lexical/annotation: %s named expression AST lost\n", test.id);
        return false;
      }
      if (std::string_view(test.id) == "annotation-positional" &&
          (!annotation->arguments[0]->name.empty() ||
           !cursiv::ast_cast<idl::ast::BinaryExpr>(annotation->arguments[0]->value))) {
        std::fprintf(stderr, "idl lexical/annotation: %s positional expression AST lost\n", test.id);
        return false;
      }
    }
  }
  return true;
}
}  // namespace idl_lexical_annotations
