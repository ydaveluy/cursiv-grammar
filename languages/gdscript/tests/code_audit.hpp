#pragma once

#include <functional>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"

namespace code_audit {
namespace ast = gdscript::ast;
inline bool run() {
  const auto language =
      cursiv::create_language<gdscript::GdscriptCoreServices,
                              gdscript::GdscriptSharedCoreServices,
                              gdscript::install_core_module>();
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "gdscript code audit: %s\n", message);
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto result = language.language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "clean parse");
    check(result.value_as<ast::Program>() != nullptr, "Program root");
    std::unordered_set<const cursiv::AstNode*> reached;
    std::function<void(const cursiv::AstNode*, const cursiv::AstNode*)> visit =
        [&](const auto* n, const auto* parent) {
          if (!n) return;
          check(n->parent == parent, "AST parents");
          if (!reached.insert(n).second) {
            check(false, "unique ownership");
            return;
          }
          language.language.astReflection->visitMembers(
              *n, [&](auto, auto, auto& child) { visit(&child, n); });
        };
    visit(result.value, nullptr);
    check(reached.size() == result.nodes.size(),
          "all allocated nodes are reachable");
    return result;
  };
  auto validate = [&](std::string_view source) {
    cursiv::Document document(cursiv::TextDocument::create(
        "memory:gdscript", "gdscript", 0, std::string(source)));
    document.language = &language.language;
    document.parseResult = parse(source);
    if (document.parseResult.value)
      document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    return diagnostics;
  };
  const std::string source =
      readFile(std::filesystem::path(CURSIV_TEST_DIR) / "native_features.gd");
  const auto result = parse(source);
  const auto* program = result.value_as<ast::Program>();
  auto method = [&](std::string_view name) -> const ast::MethodDecl* {
    if (program)
      for (const auto* item : program->body)
        if (const auto* m = cursiv::ast_cast<ast::MethodDecl>(item);
            m && m->name == name)
          return m;
    return nullptr;
  };
  if (program && program->body.size() >= 6) {
    check(cursiv::ast_cast<ast::ToolDecl>(program->body[0]) &&
              cursiv::ast_cast<ast::ClassNameDecl>(program->body[1]) &&
              cursiv::ast_cast<ast::Inheritance>(program->body[2]),
          "headers retain their order and roles");
    const auto* signal = cursiv::ast_cast<ast::SignalDecl>(program->body[3]);
    const auto* exported =
        cursiv::ast_cast<ast::ClassVarDecl>(program->body[4]);
    const auto* rpc = cursiv::ast_cast<ast::ClassVarDecl>(program->body[5]);
    check(signal && signal->params.size() == 2 && signal->params[1] == "source",
          "signal trailing comma and arguments");
    check(exported && exported->isInferred && exported->exportDecl &&
              exported->exportDecl->args.size() == 2 &&
              cursiv::ast_cast<ast::UnaryOp>(exported->exportDecl->args[0]),
          "export bounds are expressions and := remains inferred");
    check(rpc && rpc->rpcMode == "remote" && rpc->name == "state",
          "RPC variable modifier");
  } else
    check(false, "native class declarations");
  bool triple = false, path = false, number = false, precedence = false,
       enumeration = false;
  if (program)
    for (const auto* item : program->body) {
      if (const auto* c = cursiv::ast_cast<ast::ConstDecl>(item)) {
        if (c->name == "TEXT") {
          const auto* s = cursiv::ast_cast<ast::StringLit>(c->value);
          triple = s && s->text == "\"\"\"first\n  # text\nlast\"\"\"";
        }
        if (c->name == "PATH") {
          const auto* s = cursiv::ast_cast<ast::NodePathLit>(c->value);
          path = s && s->text == "\"a/b:c\"";
        }
        if (c->name == "VALUE") {
          const auto* n = cursiv::ast_cast<ast::IntLit>(c->value);
          number = c->isInferred && n && n->text == "1_000";
        }
        if (c->name == "ORDER") {
          const auto* add = cursiv::ast_cast<ast::BinOp>(c->value);
          const auto* sub =
              add ? cursiv::ast_cast<ast::BinOp>(add->left) : nullptr;
          precedence = add && add->op == "+" && sub && sub->op == "-";
        }
      }
      if (const auto* e = cursiv::ast_cast<ast::EnumDecl>(item);
          e && e->name == "State")
        enumeration = e->items.size() == 3 && e->items[0]->name == "IDLE" &&
                      cursiv::ast_cast<ast::UnaryOp>(e->items[0]->value) &&
                      cursiv::ast_cast<ast::BinOp>(e->items[1]->value) &&
                      !e->items[2]->value;
    }
  check(triple && path && number && precedence && enumeration,
        "native constant kinds, lexemes and arithmetic precedence");
  const auto* typed = method("typed");
  check(typed && typed->returnType == "Outer.Inner" &&
            typed->params.size() == 1 &&
            typed->params[0]->typeHint == "Outer.Inner",
        "qualified parameter and return types");
  const auto* child = method("native_child");
  const auto* ret = child && child->body.size() == 1
                        ? cursiv::ast_cast<ast::ReturnStmt>(child->body[0])
                        : nullptr;
  const auto* getnode =
      ret ? cursiv::ast_cast<ast::GetNodeExpr>(ret->value) : nullptr;
  check(getnode && getnode->path == "Root/\"space name\"/Child",
        "mixed quoted node path is one lookup, not division");
  const auto* checked = method("checked");
  const auto* assertion =
      checked && checked->body.size() == 5
          ? cursiv::ast_cast<ast::AssertStmt>(checked->body[0])
          : nullptr;
  check(assertion && cursiv::ast_cast<ast::CallExpr>(assertion->msg),
        "assert message is an expression");
  const auto* delayed = method("delayed");
  const auto* local =
      delayed && !delayed->body.empty()
          ? cursiv::ast_cast<ast::LocalVarDecl>(delayed->body[0])
          : nullptr;
  const auto* yield =
      local ? cursiv::ast_cast<ast::CallExpr>(local->value) : nullptr;
  check(
      yield && yield->args.empty() && cursiv::ast_cast<ast::Name>(yield->func),
      "yield is also an expression with zero arguments");
  const auto* select = method("select");
  const auto* match = select && select->body.size() == 1
                          ? cursiv::ast_cast<ast::MatchStmt>(select->body[0])
                          : nullptr;
  if (match && match->cases.size() == 4) {
    const auto* dict =
        cursiv::ast_cast<ast::DictPattern>(match->cases[0]->patterns[0]);
    const auto* array =
        cursiv::ast_cast<ast::ArrayPattern>(match->cases[1]->patterns[0]);
    const auto* negative =
        cursiv::ast_cast<ast::LiteralPattern>(match->cases[2]->patterns[0]);
    check(dict && dict->openEnded && dict->items.size() == 2 &&
              cursiv::ast_cast<ast::IntLit>(dict->items[0]->key) &&
              cursiv::ast_cast<ast::BindingPattern>(dict->items[0]->value) &&
              !dict->items[1]->value && array && array->openEnded &&
              array->items.size() == 1 && negative &&
              cursiv::ast_cast<ast::UnaryOp>(negative->value),
          "match key, binding, absent value, negative and rest have distinct "
          "roles");
  } else
    check(false, "match branches");
  check(validate(source).empty(), "native fixture validates");
  {
    constexpr std::string_view text = "func f(): return 8.0 / 3 as int\n";
    const auto r = parse(text);
    const auto* p = r.value_as<ast::Program>();
    const auto* m = p && p->body.size() == 1
                        ? cursiv::ast_cast<ast::MethodDecl>(p->body[0])
                        : nullptr;
    const auto* ret = m && m->body.size() == 1
                          ? cursiv::ast_cast<ast::ReturnStmt>(m->body[0])
                          : nullptr;
    const auto* bin = ret ? cursiv::ast_cast<ast::BinOp>(ret->value) : nullptr;
    check(bin && bin->op == "/" && cursiv::ast_cast<ast::CastExpr>(bin->right),
          "as casts its operand before division");
  }
  for (std::string_view header : {"class_name InlineName extends Reference\n",
                                  "tool extends Reference\n"}) {
    const auto r = parse(header);
    const auto* p = r.value_as<ast::Program>();
    check(p && p->body.size() == 2 &&
              cursiv::ast_cast<ast::Inheritance>(p->body[1]),
          "inline class_name and tool keep the following extends declaration");
    check(validate(header).empty(), "inline native headers validate");
  }
  {
    const auto r = parse("static func f(): pass\n");
    const auto* p = r.value_as<ast::Program>();
    const auto* f = p && p->body.size() == 1
                        ? cursiv::ast_cast<ast::MethodDecl>(p->body[0])
                        : nullptr;
    check(f && f->isStatic, "static modifier retained");
  }
  for (std::string_view bad :
       {"func f(a, a): pass\n", "func f(a=1,b): pass\n", "func f(): break\n",
        "var value = 1\nextends Reference\n",
        "extends Reference\nextends Reference\n",
        "var value = 1\nvar value = 2\n", "var value = \"\\q\"\n",
        "const X = 1E3\n", "func f(): return yield(1)\n",
        "var value = preload()\n"}) {
    const auto diagnostics = validate(bad);
    check(!diagnostics.empty(),
          "semantic errors parse and are diagnosed at runtime");
    if (!diagnostics.empty())
      check(diagnostics[0].code &&
                diagnostics[0].range.end > diagnostics[0].range.begin,
            "runtime diagnostic has a code and source range");
  }
  return ok;
}
}  // namespace code_audit
