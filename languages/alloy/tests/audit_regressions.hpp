#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "alloy/core/CoreServices.hpp"
#include "alloy/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"

namespace audit_regressions {
inline void run() {
  namespace ast = alloy::ast;
  const auto language = cursiv::create_language<alloy::AlloyCoreServices,
                                                alloy::AlloySharedCoreServices,
                                                alloy::install_core_module>();
  const std::string source =
      "module native[exactly M]\nprivate open util/ordering[M] as ord\n"
      "some private sig A {,private disjoint x,y: A,z=A,}\n"
      "private enum Color {Red,Blue}\nprivate pred p[] {}\n"
      "private fun f():set Int {}\nprivate let macro[a,b] = a+b\n"
      "fact test { a->b->c a<:b:>c a[b].c[d].e a[] (αβ') \"a\\n\" }\n"
      "first: run {} for exactly 1..5:2 A expect 1 => check {}";
  cursiv::Document document(
      cursiv::TextDocument::create("memory:alloy", "alloy", 0, source));
  document.language = &language.language;
  document.parseResult =
      language.language.parser->parse(document.textDocument().getText());
  const auto &result = document.parseResult;
  CHECK(result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty());
  const auto *root = cursiv::ast_cast<ast::Module>(result.value);
  CHECK(root && root->moduleDecl && root->moduleDecl->names.size() == 1 &&
        root->moduleDecl->names[0]->exactly_);
  CHECK(root && root->imports.size() == 1 && root->imports[0]->private_ &&
        root->imports[0]->args.size() == 1 && root->imports[0]->args[0] == "M");
  if (!root || root->paragraphs.size() != 7) {
    CHECK(false);
    return;
  }
  const auto *sig = cursiv::ast_cast<ast::SigDecl>(root->paragraphs[0]);
  CHECK(sig && sig->qualifiers.size() == 2 && sig->decls.size() == 2);
  if (sig && sig->decls.size() == 2) {
    CHECK(sig->decls[0]->private_ && sig->decls[0]->namesDisjoint &&
          sig->decls[0]->names.size() == 2);
    CHECK(sig->decls[1]->defined && sig->decls[1]->names[0] == "z");
    CHECK(sig->decls[0]->parent == sig);
  }
  const auto *fun = cursiv::ast_cast<ast::FunDecl>(root->paragraphs[3]);
  CHECK(fun && fun->private_ && fun->params.empty() && fun->body &&
        fun->body->exprs.empty());
  const auto *macro = cursiv::ast_cast<ast::MacroDecl>(root->paragraphs[4]);
  CHECK(macro && macro->private_ && macro->params.size() == 2 &&
        macro->name == "macro");
  const auto *fact = cursiv::ast_cast<ast::FactDecl>(root->paragraphs[5]);
  CHECK(fact && fact->body && fact->body->exprs.size() == 6);
  if (fact && fact->body && fact->body->exprs.size() == 6) {
    const auto *arrow = cursiv::ast_cast<ast::ArrowExpr>(fact->body->exprs[0]);
    CHECK(arrow && cursiv::ast_cast<ast::ArrowExpr>(arrow->right) &&
          arrow->right->parent == arrow);
    CHECK(arrow && cursiv::ast_cast<ast::NameExpr>(arrow->left));
    const auto *domain =
        cursiv::ast_cast<ast::BinaryExpr>(fact->body->exprs[1]);
    const auto *range =
        domain ? cursiv::ast_cast<ast::BinaryExpr>(domain->right) : nullptr;
    CHECK(domain && domain->op == ast::BinOp::domain_restrict && range &&
          range->op == ast::BinOp::range_restrict);
    const auto *chain = cursiv::ast_cast<ast::BinaryExpr>(fact->body->exprs[2]);
    CHECK(chain && chain->op == ast::BinOp::join &&
          cursiv::ast_cast<ast::BoxJoinExpr>(chain->left));
    const auto *empty =
        cursiv::ast_cast<ast::BoxJoinExpr>(fact->body->exprs[3]);
    CHECK(empty && empty->args.empty());
    const auto *name = cursiv::ast_cast<ast::NameExpr>(fact->body->exprs[4]);
    CHECK(name && name->name == "αβ'");
    const auto *string =
        cursiv::ast_cast<ast::StringExpr>(fact->body->exprs[5]);
    CHECK(string && string->raw == "\"a\\n\"");
  }
  const auto *command = cursiv::ast_cast<ast::CmdDecl>(root->paragraphs[6]);
  CHECK(command && command->name == "first" && command->expects == 1 &&
        command->next && command->next->parent == command);
  if (command && command->scope && command->scope->typescopes.size() == 1) {
    const auto *scope = command->scope->typescopes[0];
    CHECK(scope->count == 1 && scope->end == 5 && scope->step == 2 &&
          scope->exactly_ && scope->type == "A");
  } else
    CHECK(false);
  std::vector<cursiv::Diagnostic> diagnostics;
  language.language.documentValidator->validateDocument(document, diagnostics);
  CHECK(diagnostics.empty());

  {
    const std::string text = "run p for 4 Person expect 1";
    const auto parsed = language.language.parser->parse(text);
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    const auto *module = cursiv::ast_cast<ast::Module>(parsed.value);
    CHECK(module && module->paragraphs.size() == 1);
    const auto *cmd =
        module && module->paragraphs.size() == 1
            ? cursiv::ast_cast<ast::CmdDecl>(module->paragraphs[0])
            : nullptr;
    CHECK(cmd && cmd->scope && !cmd->scope->overall &&
          cmd->scope->typescopes.size() == 1 && cmd->expects == 1);
    if (cmd && cmd->scope && cmd->scope->typescopes.size() == 1)
      CHECK(cmd->scope->typescopes[0]->type == "Person" &&
            cmd->scope->typescopes[0]->count == 4);
  }
  {
    const std::string text = R"(assert "a\n" {})";
    const auto parsed = language.language.parser->parse(text);
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    const auto *module = cursiv::ast_cast<ast::Module>(parsed.value);
    const auto *assertion =
        module && module->paragraphs.size() == 1
            ? cursiv::ast_cast<ast::AssertDecl>(module->paragraphs[0])
            : nullptr;
    CHECK(assertion && assertion->name == "\"a\n\"");
  }

  for (const char* source : {R"als(assert "bad\q" {})als",
                             R"als(fact "bad\t" {})als",
                             R"als(assert "" {})als",
                             "assert \"a\nb\" {}"}) {
    const auto parsed = language.language.parser->parse(source);
    CHECK(!parsed.accepted || parsed.cst().recovered() ||
          !parsed.diagnostics.empty());
  }
  for (std::string_view empty : {"", "module empty", "open util/ordering[A]"}) {
    const auto parsed = language.language.parser->parse(empty);
    CHECK(parsed.accepted && !parsed.cst().recovered() &&
          parsed.diagnostics.empty());
    CHECK(cursiv::ast_cast<ast::Module>(parsed.value));
  }
  struct Negative {
    const char *source;
    const char *code;
    const char *anchor;
  };
  for (const Negative item :
       {Negative{"one one sig A {}", "alloy.qualifier", "one"},
        {"enum Empty {}", "alloy.empty-enum", "enum Empty {}"},
        {"sig A {disj x=A}", "alloy.defined-disjoint", "disj x=A"},
        {"fact {2147483648=0}", "alloy.integer-range", "2147483648"},
        {"sig $A {}", "alloy.name", "$A"},
        {"sig A {} sig A {}", "alloy.duplicate-signature", "A"},
        {"fact {\"\"}", "alloy.empty-string", "\"\""},
        {"run {} for 5..2 A", "alloy.scope-range", "2"},
        {"run {} for 2147483648 A", "alloy.integer-range", "2147483648"},
        {"sig A {} open util/ordering[A]", "alloy.import-order",
         "open util/ordering[A]"}}) {
    const std::string text = item.source;
    cursiv::Document invalid(
        cursiv::TextDocument::create("memory:negative", "alloy", 0, text));
    invalid.language = &language.language;
    invalid.parseResult =
        language.language.parser->parse(invalid.textDocument().getText());
    CHECK(invalid.parseResult.accepted &&
          !invalid.parseResult.cst().recovered() &&
          invalid.parseResult.diagnostics.empty());
    diagnostics.clear();
    language.language.documentValidator->validateDocument(invalid, diagnostics);
    bool found = false;
    for (const auto &d : diagnostics)
      if (d.code && d.code->string() == item.code) {
        found = true;
        CHECK(d.range.begin < d.range.end && d.range.end <= text.size());
        if (text.substr(d.range.begin, d.range.end - d.range.begin) !=
            item.anchor)
          std::fprintf(stderr, "%s: [%zu,%zu) '%.*s' expected '%s'\n",
                       item.code, std::size_t(d.range.begin),
                       std::size_t(d.range.end),
                       int(d.range.end - d.range.begin),
                       text.data() + d.range.begin, item.anchor);
        CHECK(text.substr(d.range.begin, d.range.end - d.range.begin) ==
              item.anchor);
      }
    CHECK(found);
  }
}
} // namespace audit_regressions
