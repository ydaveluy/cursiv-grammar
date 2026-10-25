#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"
#include "tinyc/core/CoreServices.hpp"
#include "tinyc/core/ast.hpp"
#include <string>
#include <vector>
namespace audit_regressions {
inline void run() {
  namespace ast = tinyc::ast;
  const auto language = cursiv::create_language<tinyc::TinycCoreServices,
                                                tinyc::TinycSharedCoreServices,
                                                tinyc::install_core_module>();
  const std::string text = "{if(a)if(b)c;else d; do e=e+1;while(e<3); {;{}} "
                           "a=b=9223372036854775808;}";
  const auto parsed = language.language.parser->parse(text);
  CHECK(parsed.accepted && !parsed.cst().recovered() &&
        parsed.diagnostics.empty());
  const auto *program = cursiv::ast_cast<ast::Program>(parsed.value);
  const auto *block =
      program && program->statements.size() == 1
          ? cursiv::ast_cast<ast::BlockStmt>(program->statements[0])
          : nullptr;
  CHECK(block && block->statements.size() == 4 && block->parent == program);
  if (block && block->statements.size() == 4) {
    const auto *outer = cursiv::ast_cast<ast::IfStmt>(block->statements[0]);
    const auto *inner =
        outer ? cursiv::ast_cast<ast::IfStmt>(outer->thenBranch) : nullptr;
    CHECK(outer && !outer->elseBranch && inner && inner->elseBranch &&
          inner->parent == outer);
    const auto *loop = cursiv::ast_cast<ast::DoStmt>(block->statements[1]);
    const auto *less =
        loop ? cursiv::ast_cast<ast::BinaryExpr>(loop->cond) : nullptr;
    CHECK(loop && less && less->op == "<" && less->parent == loop &&
          loop->body);
    const auto *nested = cursiv::ast_cast<ast::BlockStmt>(block->statements[2]);
    CHECK(nested && nested->statements.size() == 2);
    if (nested && nested->statements.size() == 2) {
      const auto *empty =
          cursiv::ast_cast<ast::ExprStmt>(nested->statements[0]);
      const auto *emptyBlock =
          cursiv::ast_cast<ast::BlockStmt>(nested->statements[1]);
      CHECK(empty && !empty->value && emptyBlock &&
            emptyBlock->statements.empty());
    }
    const auto *statement =
        cursiv::ast_cast<ast::ExprStmt>(block->statements[3]);
    const auto *first =
        statement ? cursiv::ast_cast<ast::Assign>(statement->value) : nullptr;
    const auto *second =
        first ? cursiv::ast_cast<ast::Assign>(first->value) : nullptr;
    const auto *number =
        second ? cursiv::ast_cast<ast::IntLit>(second->value) : nullptr;
    CHECK(first && first->target && first->target->name == "a" && second &&
          second->target && second->target->name == "b");
    CHECK(number && number->value == "9223372036854775808" &&
          number->parent == second);
  }
  {
    const auto zeros = language.language.parser->parse("0000001;");
    CHECK(zeros.accepted && !zeros.cst().recovered() &&
          zeros.diagnostics.empty());
    const auto *root = cursiv::ast_cast<ast::Program>(zeros.value);
    const auto *s = root && root->statements.size() == 1
                        ? cursiv::ast_cast<ast::ExprStmt>(root->statements[0])
                        : nullptr;
    const auto *n = s ? cursiv::ast_cast<ast::IntLit>(s->value) : nullptr;
    CHECK(n && n->value == "0000001");
  }
  struct Invalid {
    const char *source;
    const char *code;
    const char *anchor;
  };
  for (const Invalid item : {Invalid{"foo=1;", "tinyc.variable", "foo"},
                             {"a;b;", "tinyc.program", "b;"},
                             {"", "tinyc.program", ""}}) {
    const std::string source = item.source;
    cursiv::Document document(
        cursiv::TextDocument::create("memory:tinyc", "tinyc", 0, source));
    document.language = &language.language;
    document.parseResult =
        language.language.parser->parse(document.textDocument().getText());
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered() &&
          document.parseResult.diagnostics.empty());
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    bool found = false;
    for (const auto &d : diagnostics)
      if (d.code && d.code->string() == item.code) {
        found = true;
        CHECK(source.substr(d.range.begin, d.range.end - d.range.begin) ==
              item.anchor);
      }
    CHECK(found);
  }
}
} // namespace audit_regressions
