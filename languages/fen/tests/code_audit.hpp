#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "fen/core/CoreServices.hpp"
#include "fen/core/CounterValue.hpp"

namespace code_audit {
namespace ast = fen::ast;
inline bool run() {
  auto language =
      cursiv::create_language<fen::FenCoreServices, fen::FenSharedCoreServices,
                              fen::install_core_module>();
  bool ok = true;
  auto check = [&](bool b, const char* message) {
    if (!b) {
      std::fprintf(stderr, "fen code audit: %s\n", message);
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto r = language.language.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
    check(r.value_as<ast::Fen>() != nullptr, "Fen root");
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
    check(reached.size() == r.nodes.size(), "all nodes reachable");
    return r;
  };
  constexpr std::string_view board =
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";
  for (const char* suffix : {"", " b", " b KQkq", " b KQkq a1", " b KQkq a1 45",
                             " b KQkq a1 45 12"}) {
    const std::string source = std::string(board) + suffix;
    const auto r = parse(source);
    const auto* n = r.value_as<ast::Fen>();
    check(n && n->placement && n->placement->ranks.size() == 8, "eight ranks");
    if (n) {
      check(n->color == (suffix[0] ? "b" : "w"), "active color default");
      check(n->castling == (source.find("KQkq") != source.npos ? "KQkq" : "-"),
            "castling default");
      check(n->enpassant == (source.find("a1") != source.npos ? "a1" : "-"),
            "en passant default");
      check(
          n->halfmoveClock == (source.find("45") != source.npos ? "45" : "0") &&
              n->fullmoveClock == (source.ends_with("12") ? "12" : "1"),
          "clock defaults");
    }
  }
  {
    const std::string source =
        "\u2003" + std::string(board) + "\u00a0w\x1cHAha\u0085-\t0\n1\u2029";
    const auto r = parse(source);
    const auto* n = r.value_as<ast::Fen>();
    check(n && n->color == "w" && n->castling == "HAha" &&
              n->halfmoveClock == "0" && n->fullmoveClock == "1",
          "Unicode whitespace and file castling rights");
  }
  {
    const auto r = parse("4k3/8/8/8/8/8/8/R~3K2R w QKqk - 0 0");
    const auto* n = r.value_as<ast::Fen>();
    if (n && n->placement && n->placement->ranks.size() == 8) {
      const auto& squares = n->placement->ranks[7]->squares;
      const auto* piece = squares.empty()
                              ? nullptr
                              : std::get_if<ast::PieceSquare*>(&squares[0]);
      check(piece && *piece && (*piece)->piece == "R" &&
                (*piece)->promotion == "~",
            "promoted piece marker");
      check(n->castling == "QKqk" && n->fullmoveClock == "0",
            "right order and zero spelling preserved");
    } else
      check(false, "promoted board");
  }
  {
    const std::string half(100, '9'), full(120, '8');
    const std::string source =
        std::string(board) + " w - - " + half + " " + full;
    const auto r = parse(source);
    const auto* n = r.value_as<ast::Fen>();
    check(n && n->halfmoveClock == half && n->fullmoveClock == full &&
              fen::counter_value(n->halfmoveClock) == half &&
              fen::counter_value(n->fullmoveClock) == full,
          "arbitrary precision counters retain all digits");
    check(fen::counter_value("+1_000") == "1000" &&
              fen::counter_value("-0") == "0" &&
              fen::counter_value("١٢") == "12" &&
              fen::counter_value("９９") == "99" && !fen::counter_value("-1") &&
              !fen::counter_value("1__0") && !fen::counter_value("1_") &&
              !fen::counter_value("1.5") &&
              !fen::counter_value("\xf8\x9d\x9f\x8e"),
          "native integer forms validate at runtime");
  }
  {
    const auto r = parse("8/8/8/8/8/8/8/r\t6K w - - 0 1");
    const auto* n = r.value_as<ast::Fen>();
    check(n && n->placement && n->placement->ranks.size() == 8 &&
              n->placement->ranks[7]->squares.size() == 1 && n->color == "6K",
          "whitespace cannot silently join board pieces");
  }
  {
    const std::string source = "99/P~~X x KK z9 -1 abc extra";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:fen", "fen", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    for (const auto* n : doc.parseResult.nodes) {
      if (const auto* p = cursiv::ast_cast<ast::Fen>(n)) {
        language.language.validator->checkFen(*p, accept);
        language.language.validator->checkCastling(*p, accept);
      }
      if (const auto* p = cursiv::ast_cast<ast::Placement>(n))
        language.language.validator->checkPlacement(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::Rank>(n))
        language.language.validator->checkRank(*p, accept);
      if (const auto* p = cursiv::ast_cast<ast::PieceSquare>(n))
        language.language.validator->checkPiece(*p, accept);
    }
    check(errors == 11,
          "invalid field and board values receive precise runtime errors");
  }
  return ok;
}
}  // namespace code_audit
