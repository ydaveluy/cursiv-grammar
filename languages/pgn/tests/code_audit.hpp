#pragma once

#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "pgn/core/CoreServices.hpp"
#include "pgn/core/Values.hpp"

namespace code_audit {
namespace ast = pgn::ast;
inline bool run() {
  auto language =
      cursiv::create_language<pgn::PgnCoreServices, pgn::PgnSharedCoreServices,
                              pgn::install_core_module>();
  bool ok = true;
  auto check = [&](bool b, const char* message) {
    if (!b) {
      std::fprintf(stderr, "pgn code audit: %s\n", message);
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto r = language.language.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
    check(r.value_as<ast::Database>() != nullptr, "Database root");
    std::unordered_set<const cursiv::AstNode*> reached;
    std::function<void(const cursiv::AstNode*, const cursiv::AstNode*)> visit =
        [&](const auto* n, const auto* parent) {
          if (!n) return;
          check(n->parent == parent, "AST parents");
          if (!reached.insert(n).second) {
            check(false, "AST ownership");
            return;
          }
          language.language.astReflection->visitMembers(
              *n, [&](auto, auto, auto& child) { visit(&child, n); });
        };
    visit(r.value, nullptr);
    check(reached.size() == r.nodes.size(), "all allocated nodes reachable");
    return r;
  };
  {
    const auto r = parse("");
    const auto* db = r.value_as<ast::Database>();
    check(db && db->games.empty(), "empty database");
  }
  {
    const auto r = parse("1 . . . e4 *");
    const auto* db = r.value_as<ast::Database>();
    const auto* m =
        db && db->games.size() == 1 && !db->games[0]->elements.empty()
            ? cursiv::ast_cast<ast::MoveNumber>(db->games[0]->elements[0])
            : nullptr;
    check(m && m->number == "1" && std::ranges::count(m->dots, '.') == 3,
          "spaced move-number periods");
    const auto bare = parse("... e4 *");
    const auto* bd = bare.value_as<ast::Database>();
    const auto* bm =
        bd && bd->games.size() == 1 && !bd->games[0]->elements.empty()
            ? cursiv::ast_cast<ast::MoveNumber>(bd->games[0]->elements[0])
            : nullptr;
    check(bm && bm->number.empty() && bm->dots == "...",
          "bare ellipsis remains represented");
  }
  for (const char* source :
       {"1.e4 e5", "[Event \"Header only\"]", "{ Comment only }"}) {
    const auto r = parse(source);
    const auto* db = r.value_as<ast::Database>();
    check(db && db->games.size() == 1 && !db->games[0]->terminated,
          "game without explicit result");
  }
  {
    const auto r = parse("1.e4 e5\n\n1.d4 d5\n");
    const auto* db = r.value_as<ast::Database>();
    check(db && db->games.size() == 2 && !db->games[0]->terminated &&
              !db->games[1]->terminated,
          "blank lines separate unterminated games");
    const auto header = parse("[Event \"One\"]\n\n\n[Event \"Two\"]\n");
    const auto* hd = header.value_as<ast::Database>();
    check(hd && hd->games.size() == 2 && hd->games[0]->tags.size() == 1 &&
              hd->games[1]->tags.size() == 1 &&
              hd->games[0]->tags[0]->value == "One" &&
              hd->games[1]->tags[0]->value == "Two",
          "header-only games stay distinct");
  }
  {
    const auto r = parse("1.-- -- 2.Z0 Z0 3.0000 0000 4.@@@@ @@@@ *");
    std::vector<std::string_view> moves;
    for (const auto* n : r.nodes)
      if (const auto* m = cursiv::ast_cast<ast::SanMove>(n))
        moves.push_back(m->san);
    check(moves == std::vector<std::string_view>{"--", "--", "Z0", "Z0", "0000",
                                                 "0000", "@@@@", "@@@@"},
          "all native null move spellings");
  }
  {
    const std::string huge(100, '9');
    const std::string source =
        "1.e4! e5? 2.Nf3!! Nc6?? 3.Bb5!? a6?! $0005 $" + huge + " *";
    const auto r = parse(source);
    std::vector<std::string_view> nags;
    for (const auto* n : r.nodes)
      if (const auto* m = cursiv::ast_cast<ast::Nag>(n))
        nags.push_back(m->number);
    check(nags.size() == 8, "all NAGs preserved");
    if (nags.size() == 8)
      check(nags[0] == "1" && nags[1] == "2" && nags[2] == "3" &&
                nags[3] == "4" && nags[4] == "5" && nags[5] == "6" &&
                nags[6] == "0005" && nags[7] == huge,
            "symbolic and unbounded numeric NAG values");
    const std::string number(100, '8');
    const std::string numbered = number + ".e4 *";
    const auto big = parse(numbered);
    const auto* db = big.value_as<ast::Database>();
    const auto* m =
        db && db->games.size() == 1 && !db->games[0]->elements.empty()
            ? cursiv::ast_cast<ast::MoveNumber>(db->games[0]->elements[0])
            : nullptr;
    check(m && m->number == number && m->dots == ".",
          "move number lexeme has no integer limit");
  }
  {
    const auto r = parse(R"pgn([Event ""]
[Annotator "A \"Q\" \\ path \q"]

1.e4 *)pgn");
    const auto* db = r.value_as<ast::Database>();
    if (db && db->games.size() == 1) {
      const auto* g = db->games[0];
      check(g->tags.size() == 2 && g->tags[0]->value.empty() &&
                g->tags[1]->value == R"pgn(A \"Q\" \\ path \q)pgn",
            "native tag values retain their raw escapes");
      check(g->terminated && g->result == ast::Result::unknown,
            "explicit star differs from omitted result");
    } else
      check(false, "tagged game");
  }
  {
    const auto r = parse("[Event \"One\"]\n[Event \"Two\"]\n\n*");
    const auto* db = r.value_as<ast::Database>();
    check(db && db->games.size() == 1 && db->games[0]->tags.size() == 2 &&
              db->games[0]->tags[0]->value == "One" &&
              db->games[0]->tags[1]->value == "Two",
          "duplicate header values remain in the source AST");
  }
  {
    const auto r = parse(
        "{ start }1.e4 {main} ({alternative}1.d4 d5 (1...Nf6 {Indian})) e5 "
        "2.Nf3 {line\n\ninside} *");
    unsigned comments = 0, variations = 0;
    bool start = false, multiline = false;
    for (const auto* n : r.nodes) {
      if (const auto* c = cursiv::ast_cast<ast::Comment>(n)) {
        ++comments;
        start |= pgn::comment_value(c->raw) == "start";
        multiline |= pgn::comment_value(c->raw) == "line\n\ninside";
      }
      if (const auto* v = cursiv::ast_cast<ast::Variation>(n)) {
        ++variations;
        check(v->closed, "closed nested variation");
      }
    }
    check(comments == 5 && variations == 2 && start && multiline,
          "comments and nested variations retain values and order");
    check(pgn::comment_value("{  double  }") == " double " &&
              pgn::comment_value("{ tail ") == "tail ",
          "comment trimming follows native reader");
  }
  {
    const std::string source =
        "[Result \"1-0\"]\n\n1.e4 (1.d4))) 0-1\n\n((1.c4 *\n\n{ open comment ";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:pgn", "pgn", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    for (const auto* n : doc.parseResult.nodes) {
      if (const auto* p = cursiv::ast_cast<ast::Game>(n))
        language.language.validator->checkGame(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::Variation>(n))
        language.language.validator->checkVariation(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::Comment>(n))
        language.language.validator->checkComment(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::UnexpectedClose>(n))
        language.language.validator->checkUnexpectedClose(*p, accept);
    }
    check(errors == 6,
          "structural and conflicting-result errors validate at runtime");
  }
  // PGN1994 requires equality even when exactly one marker is '*'. The
  // forgiving python-chess reader does not itself diagnose all conflicts.
  for (const char* source : {"[Result \"1-0\"]\n\n1.e4 *", "[Result \"*\"]\n\n1.e4 1-0"}) {
    cursiv::Document doc(cursiv::TextDocument::create("memory:result", "pgn", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    for (const auto* node : doc.parseResult.nodes)
      if (const auto* game = cursiv::ast_cast<ast::Game>(node))
        language.language.validator->checkGame(*game, accept);
    check(errors==1, "star versus decisive Result mismatch reports a diagnostic");
  }
  {
    const auto r=parse("% initial\n\n% escaped\n1.e4\n% ignored e5\ne5 *");
    unsigned moves=0;
    for(const auto* node:r.nodes) moves+=cursiv::ast_cast<ast::SanMove>(node)!=nullptr;
    check(moves==2, "physical percent escape lines preserve following moves");
    const auto bad=language.language.parser->parse("1.e4 % garbage e5 *");
    check(!bad.accepted||bad.cst().recovered()||!bad.diagnostics.empty(),
          "inline percent cannot silently discard the remaining movetext");
  }
  return ok;
}
}  // namespace code_audit
