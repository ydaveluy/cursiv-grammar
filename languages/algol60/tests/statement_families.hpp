#pragma once

#include <cstdio>
#include <initializer_list>
#include <string_view>
#include <unordered_set>
#include <utility>

#include "algol60/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/Check.hpp"

namespace algol60_tests {
namespace ast = algol60::ast;

inline bool clean(const cursiv::ParseResult& parsed, std::string_view source) {
  const bool ok = parsed.accepted && !parsed.cst().recovered() &&
                  parsed.diagnostics.empty();
  if (!ok) {
    std::fprintf(stderr, "algol60 statement source: %.*s\naccepted=%d recovered=%d diagnostics=%zu\n",
                 static_cast<int>(source.size()), source.data(), parsed.accepted,
                 parsed.cst().recovered(), parsed.diagnostics.size());
    for (const auto& diagnostic : parsed.diagnostics)
      std::fprintf(stderr, "kind=%d begin=%u end=%u\n",
                   static_cast<int>(diagnostic.kind), diagnostic.begin,
                   diagnostic.end);
  }
  return ok;
}

inline bool parents(const auto& language, const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) return false;
  bool valid = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) {
      valid = false;
      return;
    }
    language.astReflection->visitMembers(
        node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node &&
                   parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return valid && seen.size() == parsed.nodes.size();
}

inline const ast::Stmt* first(const ast::Block* block) {
  return block && !block->statements.empty() ? block->statements[0]->stmt
                                            : nullptr;
}

inline void statementFamilies(const auto& language) {
  // Revised Report1963 §§4.1/4.4: each separator creates another slot,
  // including a final dummy before end; labels can name that dummy.
  const std::pair<const char*, std::size_t> emptyCases[] = {
      {"begin end", 1}, {"begin ;;; end", 4},
      {"begin integer x; end", 1}, {"begin L:; M: N: end", 2}};
  for (const auto& [source, count] : emptyCases) {
    const auto parsed = language.parser->parse(source);
    CHECK(clean(parsed, source));
    CHECK(parents(language, parsed));
    const auto* block = cursiv::ast_cast<ast::Block>(parsed.value);
    CHECK(block && block->statements.size() == count);
    if (!block) continue;
    for (const auto* entry : block->statements)
      CHECK(entry && cursiv::ast_cast<ast::EmptyStmt>(entry->stmt));
    if (std::string_view(source) == "begin L:; M: N: end" &&
        block->statements.size() == 2) {
      CHECK(block->statements[0]->labels.size() == 1 &&
            block->statements[0]->labels[0] == "L");
      CHECK(block->statements[1]->labels.size() == 2 &&
            block->statements[1]->labels[0] == "M" &&
            block->statements[1]->labels[1] == "N");
    }
  }
  {
    const char* source = "begin if true then L: P: x:=1 else 12: begin end end";
    const auto parsed = language.parser->parse(source);
    CHECK(clean(parsed, source));
    CHECK(parents(language, parsed));
    const auto* branch = cursiv::ast_cast<ast::IfStmt>(
        first(cursiv::ast_cast<ast::Block>(parsed.value)));
    const auto* yes = branch ? cursiv::ast_cast<ast::StmtEntry>(branch->then_branch) : nullptr;
    const auto* no = branch ? cursiv::ast_cast<ast::StmtEntry>(branch->else_branch) : nullptr;
    CHECK(branch && branch->hasElse && yes && no);
    CHECK(yes && yes->labels.size() == 2 && yes->labels[0] == "L" &&
          yes->labels[1] == "P" && cursiv::ast_cast<ast::AssignStmt>(yes->stmt));
    CHECK(no && no->labels.size() == 1 && no->labels[0] == "12" &&
          cursiv::ast_cast<ast::Block>(no->stmt));
  }
  {
    const char* source = "begin for i:=1 do L: if true then P:q else E: end";
    const auto parsed = language.parser->parse(source);
    CHECK(clean(parsed, source));
    CHECK(parents(language, parsed));
    const auto* loop = cursiv::ast_cast<ast::ForStmt>(
        first(cursiv::ast_cast<ast::Block>(parsed.value)));
    const auto* body = loop ? cursiv::ast_cast<ast::StmtEntry>(loop->body) : nullptr;
    const auto* branch = body ? cursiv::ast_cast<ast::IfStmt>(body->stmt) : nullptr;
    const auto* yes = branch ? cursiv::ast_cast<ast::StmtEntry>(branch->then_branch) : nullptr;
    const auto* no = branch ? cursiv::ast_cast<ast::StmtEntry>(branch->else_branch) : nullptr;
    CHECK(body && body->labels.size() == 1 && body->labels[0] == "L" && branch);
    CHECK(yes && yes->labels.size() == 1 && yes->labels[0] == "P" &&
          cursiv::ast_cast<ast::CallStmt>(yes->stmt));
    CHECK(no && no->labels.size() == 1 && no->labels[0] == "E" &&
          cursiv::ast_cast<ast::EmptyStmt>(no->stmt));
  }
  {
    const char* source = "begin if true then Loop: for i:=1 do B:p end";
    const auto parsed = language.parser->parse(source);
    CHECK(clean(parsed, source));
    CHECK(parents(language, parsed));
    const auto* branch = cursiv::ast_cast<ast::IfStmt>(
        first(cursiv::ast_cast<ast::Block>(parsed.value)));
    const auto* label = branch ? cursiv::ast_cast<ast::StmtEntry>(branch->then_branch) : nullptr;
    const auto* loop = label ? cursiv::ast_cast<ast::ForStmt>(label->stmt) : nullptr;
    const auto* body = loop ? cursiv::ast_cast<ast::StmtEntry>(loop->body) : nullptr;
    CHECK(branch && !branch->hasElse && !branch->else_branch && label &&
          label->labels.size() == 1 && label->labels[0] == "Loop" && loop);
    CHECK(body && body->labels.size() == 1 && body->labels[0] == "B" &&
          cursiv::ast_cast<ast::CallStmt>(body->stmt));
  }
  {
    const char* source = "begin procedure p; P: for i:=1 do K:; p end";
    const auto parsed = language.parser->parse(source);
    CHECK(clean(parsed, source));
    CHECK(parents(language, parsed));
    const auto* block = cursiv::ast_cast<ast::Block>(parsed.value);
    const auto* proc = block && block->decls.size() == 1
                           ? cursiv::ast_cast<ast::ProcDecl>(block->decls[0]) : nullptr;
    const auto* label = proc ? cursiv::ast_cast<ast::StmtEntry>(proc->body) : nullptr;
    const auto* loop = label ? cursiv::ast_cast<ast::ForStmt>(label->stmt) : nullptr;
    const auto* body = loop ? cursiv::ast_cast<ast::StmtEntry>(loop->body) : nullptr;
    CHECK(label && label->labels.size() == 1 && label->labels[0] == "P");
    CHECK(body && body->labels.size() == 1 && body->labels[0] == "K" &&
          cursiv::ast_cast<ast::EmptyStmt>(body->stmt));
  }
  for (const char* source : {"begin if true then end", "begin if true then else end",
                             "begin for i:=1 do end", "begin procedure p; ; p end"}) {
    const auto parsed = language.parser->parse(source);
    CHECK(clean(parsed, source));
    CHECK(parents(language, parsed));
    const auto* block = cursiv::ast_cast<ast::Block>(parsed.value);
    if (const auto* branch = cursiv::ast_cast<ast::IfStmt>(first(block))) {
      CHECK(cursiv::ast_cast<ast::EmptyStmt>(branch->then_branch));
      CHECK(branch->hasElse ? cursiv::ast_cast<ast::EmptyStmt>(branch->else_branch) != nullptr
                            : branch->else_branch == nullptr);
    } else if (const auto* loop = cursiv::ast_cast<ast::ForStmt>(first(block))) {
      CHECK(cursiv::ast_cast<ast::EmptyStmt>(loop->body));
    } else {
      const auto* proc = block && block->decls.size() == 1
                             ? cursiv::ast_cast<ast::ProcDecl>(block->decls[0]) : nullptr;
      CHECK(proc && cursiv::ast_cast<ast::EmptyStmt>(proc->body));
    }
  }

  // §2.3: end comments are not merely one closing identifier. Words such
  // as friend/elseness must remain whole, and a later comment/quote cannot
  // swallow the first end-comment's semicolon or end/else terminator.
  const std::pair<const char*, std::size_t> comments[] = {
      {"begin begin end closing block, v1 / \"junk\"; p end", 2},
      {"begin begin end friend weekend elseness end", 1},
      {"begin begin end comment preserve this separator; p end", 2},
      {"begin comment end else ; p end", 1},
      {"begin begin end multiword\r\ncomment +[]{}\n end", 1},
      {"begin begin end \" a later quote cannot protect end", 1},
      {"begin p(\"end else; comment\"); begin end more text end", 2},
      {"begin if true then begin end arbitrary / text else Label:p end", 1}};
  for (const auto& [source, count] : comments) {
    const auto parsed = language.parser->parse(source);
    CHECK(clean(parsed, source));
    CHECK(parents(language, parsed));
    const auto* block = cursiv::ast_cast<ast::Block>(parsed.value);
    CHECK(block && block->statements.size() == count);
    if (std::string_view(source).find("else Label") != std::string_view::npos) {
      const auto* branch = cursiv::ast_cast<ast::IfStmt>(first(block));
      const auto* no = branch ? cursiv::ast_cast<ast::StmtEntry>(branch->else_branch) : nullptr;
      CHECK(branch && branch->hasElse && cursiv::ast_cast<ast::Block>(branch->then_branch));
      CHECK(no && no->labels.size() == 1 && no->labels[0] == "Label" &&
            cursiv::ast_cast<ast::CallStmt>(no->stmt));
    }
  }

  const std::pair<const char*, bool> nestedConditionals[] = {
      {"begin if true then begin if false then p else q end else r end", true},
      {"begin if true then for i:=1 do if false then p else q end", false}};
  for (const auto& [source, outerElse] : nestedConditionals) {
    const auto parsed = language.parser->parse(source);
    CHECK(clean(parsed, source));
    CHECK(parents(language, parsed));
    const auto* branch = cursiv::ast_cast<ast::IfStmt>(
        first(cursiv::ast_cast<ast::Block>(parsed.value)));
    CHECK(branch && branch->hasElse == outerElse);
    if (!outerElse) {
      const auto* loop = branch ? cursiv::ast_cast<ast::ForStmt>(branch->then_branch) : nullptr;
      const auto* inner = loop ? cursiv::ast_cast<ast::IfStmt>(loop->body) : nullptr;
      CHECK(inner && inner->hasElse && !branch->else_branch);
    }
  }

  // §4.5 permits an unconditional then-body, or a for-body without an
  // outer else. A nested conditional remains available inside begin/end.
  for (const char* source : {
           "begin if true then if false then p else q end",
           "begin if true then L: if false then p end",
           "begin if true then for i:=1 do p else q end",
           "begin if true then L: for i:=1 do p else q end",
           "begin if true then L: end else q",
           "begin L:: end"}) {
    const auto parsed = language.parser->parse(source);
    const bool rejected = !parsed.accepted || parsed.cst().recovered() ||
                          !parsed.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "algol60 invalid statement accepted: %s\n", source);
    CHECK(rejected);
  }
}
}  // namespace algol60_tests
