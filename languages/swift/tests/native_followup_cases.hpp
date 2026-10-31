#pragma once
#include <cstdio>
#include <string>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "swift/core/CoreServices.hpp"
#include "swift/core/ast.hpp"

namespace native_followup {
inline bool run(const swift::SwiftCoreServices& services) {
  namespace ast = swift::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "swift followup: %s\n", message); ok = false; }
  };
  const auto clean = [](const auto& r) {
    return r.accepted && !r.cst().recovered() && r.diagnostics.empty() && r.value;
  };
  const auto owns = [&](const auto& r) {
    bool valid = r.value != nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      services.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        valid &= child.parent == &node;
        self(self, child);
      });
    };
    if (r.value) walk(walk, *r.value);
    return valid && seen.size() == r.nodes.size();
  };
  {
    const std::string text = R"SW(@Attr(1, "two \(value)", [3], {x}) func f() {}
)SW";
    const auto r = services.parser->parse(text);
    const auto* file = cursiv::ast_cast<ast::File>(r.value);
    const auto* fn = file && file->statements.size() == 1 ? cursiv::ast_cast<ast::FunctionDecl>(file->statements[0]) : nullptr;
    const auto* args = fn && fn->attributes.size() == 1 ? fn->attributes[0]->rawArgs : nullptr;
    const auto* str = args && args->children.size() == 7 ? cursiv::ast_cast<ast::StringLit>(args->children[2]->literal) : nullptr;
    check(clean(r) && owns(r) && args && args->children.size() == 7 && args->children[0]->text == "1" &&
          args->children[1]->text == "," && str && str->parts.size() == 2 && str->parts[0]->text == "two " &&
          str->parts[1]->interpolation && str->parts[1]->arguments.size() == 1 &&
          args->children[4]->text == "[" && args->children[4]->children.size() == 1 && args->children[4]->children[0]->children.size() == 1 &&
          args->children[4]->children[0]->children[0]->text == "3" && args->children[6]->text == "{" &&
          r.cst().text(args->children[2]->cstNodeId) == "\"two \\(value)\"",
          "balanced attribute values, delimiters, interpolation, CST and ownership");
  }
  {
    const auto r = services.parser->parse("enum E { case some(Int), none }\n");
    const auto* file = cursiv::ast_cast<ast::File>(r.value);
    const auto* e = file && file->statements.size() == 1 ? cursiv::ast_cast<ast::EnumDecl>(file->statements[0]) : nullptr;
    const auto* cases = e && e->members.size() == 1 ? cursiv::ast_cast<ast::EnumCaseDecl>(e->members[0]) : nullptr;
    check(clean(r) && owns(r) && cases && cases->cases.size() == 2 && cases->cases[0]->name == "some" &&
          cases->cases[0]->associatedValue && !cases->cases[0]->opaqueBase && cases->cases[1]->name == "none",
          "some is an enum case name, with an owned associated tuple");
  }
  for (const std::string& text : {
      std::string("func f() { return−value }\nlet x = true≠false\n"),
      std::string("@Attr(a = b.c / d, \\Root.path) func f() {}\n"),
      std::string("let x") + char(0) + "= 1\n",
      std::string("let `123` = 1\nlet `a b` = 2\nlet `a-b` = 3\nlet `a\u200Eb` = 4\n"),
      std::string(R"SW(let s = #"a\#"#b"#
)SW"),
      std::string(R"SW(let s = """
  value
  """
let raw = #"""
  value
  """#
)SW")}) {
    const auto r = services.parser->parse(text);
    check(clean(r) && owns(r) && cursiv::test::validationHelper(services, text).diagnostics.empty(),
          "source-confirmed lexical or multiline positive");
  }
  for (const std::string text : {"let `   ` = 1\n", "let `+` = 1\n", "let `a\tb` = 1\n",
                                  "let `a\u00A0b` = 1\n", "let `−\u0301` = 1\n",
                                  "let x = #\"a\nb\"#\n", "switch x { #elseif FLAG\ncase 1: break\n#endif }\n"}) {
    check(!clean(services.parser->parse(text)), "invalid raw identifier, raw newline or orphan elseif");
  }
  for (const std::string text : {"let x = \"\"\"same line\"\"\"\n", "let x = \"\"\"\nvalue\"\"\"\n",
                                  "let x = \"\"\"\n value\n  \"\"\"\n", "let x = #\"\"\"same line\"\"\"#\n"}) {
    const auto r = services.parser->parse(text);
    const auto v = cursiv::test::validationHelper(services, text);
    bool diagnosed = false;
    for (const auto& d : v.diagnostics) diagnosed |= d.code == "swift.multiline-string";
    check(clean(r) && diagnosed, "multiline opening, closing or indentation diagnostic");
  }
  return ok;
}
}  // namespace native_followup
