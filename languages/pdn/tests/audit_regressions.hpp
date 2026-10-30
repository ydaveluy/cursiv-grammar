#pragma once

#include <string>
#include <utility>
#include <vector>

#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"
#include "pdn/core/CoreServices.hpp"
#include "pdn/core/ast.hpp"

inline void pdn_regressions(const pdn::PdnCoreServices &services) {
  namespace ast = pdn::ast;
  const std::string source =
      "[A \"a\\\"b\\\\c\"]\n1... e2:f3:h5(!?) (1. 32-28 (1. 31-26)) "
      "$001 { raw\ncomment } /W:Wk50:B1,2/ 0-2\n[A \"second\"]\n1. 1-10 1-1";
  const auto result = services.parser->parse(source);
  CHECK(result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty());
  const auto *root = cursiv::ast_cast<ast::Document>(result.value);
  CHECK(root && root->games.size() == 2);
  if (root && root->games.size() == 2) {
    const auto *game = root->games[0];
    CHECK(game && game->parent == root && game->tags.size() == 1 &&
          game->items.size() == 5);
    CHECK(game->results.size() == 1 &&
          game->results[0] == ast::Outcome::BlackWin2);
    if (game->tags.size() == 1)
      CHECK(game->tags[0]->parent == game && game->tags[0]->name == "A" &&
            game->tags[0]->value == "a\\\"b\\\\c");
    if (game->items.size() == 5) {
      const auto *ply = cursiv::ast_cast<ast::Ply>(game->items[0]);
      const auto *variation = cursiv::ast_cast<ast::Variation>(game->items[1]);
      const auto *nag = cursiv::ast_cast<ast::Nag>(game->items[2]);
      const auto *comment = cursiv::ast_cast<ast::Comment>(game->items[3]);
      const auto *setup = cursiv::ast_cast<ast::Setup>(game->items[4]);
      CHECK(ply && ply->number == "1..." && ply->notation == "e2:f3:h5" &&
            ply->capture && ply->strength == "(!?)");
      CHECK(variation && variation->items.size() == 2);
      CHECK(nag && nag->value == "$001" && comment &&
            comment->text == " raw\ncomment " && setup &&
            setup->text == "W:Wk50:B1,2");
      for (const auto *item : game->items)
        CHECK(item->parent == game);
      if (variation && variation->items.size() == 2) {
        const auto *inner =
            cursiv::ast_cast<ast::Variation>(variation->items[1]);
        CHECK(inner && inner->parent == variation && inner->items.size() == 1 &&
              inner->items[0]->parent == inner);
      }
      if (ply) {
        const auto span = result.cst()[ply->cstNodeId];
        CHECK(span.begin == source.find("1...") &&
              span.end == source.find(" (1."));
      }
    }
    const auto *second = root->games[1];
    CHECK(second && second->results.size() == 1 &&
          second->results[0] == ast::Outcome::Draw2 &&
          second->items.size() == 1);
    if (second && second->items.size() == 1) {
      const auto *move = cursiv::ast_cast<ast::Ply>(second->items[0]);
      CHECK(move && move->notation == "1-10" && !move->capture);
    }
    CHECK(result.nodes.size() == 14);
  }
  for (const auto [text, code] :
       {std::pair{"", "pdn.empty"}, std::pair{"1. 32-28 () *", "pdn.variation"},
        std::pair{"1. 32-28 1-0 0-1", "pdn.results"},
        std::pair{"1. 32-28 [Event \"second\"] 1. 18-23 *", "pdn.separator"}}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:pdn", "pdn", 0, text));
    document.language = &services;
    document.parseResult = services.parser->parse(text);
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered() &&
          document.parseResult.diagnostics.empty());
    CHECK(diagnostics.size() == 1 && diagnostics[0].code &&
          *diagnostics[0].code == code);
    if (diagnostics.size() == 1 && std::string_view(code) == "pdn.variation")
      CHECK(diagnostics[0].range.begin == 9 && diagnostics[0].range.end == 11);
  }
}
