#pragma once

#include <cmath>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"
#include "graphstreamdgs/core/CoreServices.hpp"
#include "graphstreamdgs/core/ast.hpp"

inline void dgs_regressions() {
  namespace ast = graphstreamdgs::ast;
  const auto language =
      cursiv::create_language<graphstreamdgs::GraphstreamDgsCoreServices,
                              graphstreamdgs::GraphstreamDgsSharedCoreServices,
                              graphstreamdgs::install_core_module>();
  const auto& services = language.language;
  {
    const std::string source =
        "DGS004\n  raw 00 header  \nAN 001 bool:TRUE quoted:\"true\" num:001 "
        "real:1.0e10 map:[flag,x:falſe] text:\"a\\\"b\"";
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<ast::Document>(result.value);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    CHECK(root && root->header == "  raw 00 header  " &&
          root->magic == "DGS004" && root->events.size() == 1);
    const auto* add = root && root->events.size() == 1
                          ? cursiv::ast_cast<ast::AddNode>(root->events[0])
                          : nullptr;
    CHECK(add && add->id == "001" && add->parent == root &&
          add->attributes.size() == 6);
    if (add && add->attributes.size() == 6) {
      bool valuesPresent = true;
      for (const auto* a : add->attributes) {
        CHECK(a && a->parent == add && a->values.size() == 1);
        valuesPresent &= a && a->values.size() == 1;
      }
      if (!valuesPresent) return;
      const auto* boolean = std::get_if<bool>(&add->attributes[0]->values[0]);
      const auto* quoted =
          std::get_if<std::string_view>(&add->attributes[1]->values[0]);
      const auto* integer =
          std::get_if<std::int64_t>(&add->attributes[2]->values[0]);
      const auto* real = std::get_if<double>(&add->attributes[3]->values[0]);
      const auto* mapValue =
          std::get_if<ast::Map*>(&add->attributes[4]->values[0]);
      const auto* text =
          std::get_if<std::string_view>(&add->attributes[5]->values[0]);
      CHECK(boolean && *boolean && quoted && *quoted == "true" && integer &&
            *integer == 1 && real && *real == 1e10);
      CHECK(text && *text == "a\"b");
      const auto* map = mapValue ? *mapValue : nullptr;
      CHECK(map && map->parent == add->attributes[4] &&
            map->entries.size() == 2);
      if (map && map->entries.size() == 2) {
        const auto* flag = std::get_if<bool>(&map->entries[0]->value);
        const auto* x = std::get_if<bool>(&map->entries[1]->value);
        CHECK(map->entries[0]->key == "flag" && flag && *flag && x && !*x);
        CHECK(map->entries[0]->parent == map && map->entries[1]->parent == map);
      }
      const auto span = result.cst()[add->cstNodeId];
      CHECK(span.begin == source.find("AN 001") && span.end == source.size());
      CHECK(result.nodes.size() == 11);
    }
  }
  {
    const std::string source = "DGS003\nx\nan +n\nan _n\nae 000 +n < _n\n# EOF";
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<ast::Document>(result.value);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    const auto* edge = root && root->events.size() == 3
                           ? cursiv::ast_cast<ast::AddEdge>(root->events[2])
                           : nullptr;
    CHECK(edge && edge->id == "000" && edge->source == "+n" &&
          edge->target == "_n" && edge->direction == ast::Direction::Backward &&
          edge->parent == root);
  }
  {
    const std::string source = "DGS004\n\nst .5\nst 0x1p2\nst \"1.5\"\nst NaN";
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<ast::Document>(result.value);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    CHECK(root && root->events.size() == 4);
    if (root && root->events.size() == 4) {
      const double expected[] = {.5, 4, 1.5};
      for (unsigned i = 0; i < 3; ++i) {
        const auto* step = cursiv::ast_cast<ast::Step>(root->events[i]);
        CHECK(step && step->time == expected[i] && step->parent == root);
      }
      const auto* step = cursiv::ast_cast<ast::Step>(root->events[3]);
      CHECK(step && std::isnan(step->time));
    }
  }
  {
    const std::string source =
        "DGS004\nh\ncn n\nce e\ncg\nan a nested:{{1,2}} duplicate:[a:1,a:2]";
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<ast::Document>(result.value);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    CHECK(root && root->events.size() == 4);
    if (root && root->events.size() == 4) {
      const auto* node = cursiv::ast_cast<ast::ChangeNode>(root->events[0]);
      const auto* edge = cursiv::ast_cast<ast::ChangeEdge>(root->events[1]);
      const auto* graph = cursiv::ast_cast<ast::ChangeGraph>(root->events[2]);
      const auto* add = cursiv::ast_cast<ast::AddNode>(root->events[3]);
      CHECK(node && node->id == "n" && node->attributes.empty());
      CHECK(edge && edge->id == "e" && edge->attributes.empty());
      CHECK(graph && graph->attributes.empty());
      CHECK(add && add->attributes.size() == 2);
      if (add && add->attributes.size() == 2 &&
          add->attributes[0]->values.size() == 1 &&
          add->attributes[1]->values.size() == 1) {
        const auto* outerValue =
            std::get_if<ast::Array*>(&add->attributes[0]->values[0]);
        const auto* outer = outerValue ? *outerValue : nullptr;
        CHECK(outer && outer->items.size() == 1 &&
              outer->parent == add->attributes[0]);
        if (outer && outer->items.size() == 1) {
          const auto* innerValue = std::get_if<ast::Array*>(&outer->items[0]);
          const auto* inner = innerValue ? *innerValue : nullptr;
          CHECK(inner && inner->items.size() == 2 && inner->parent == outer);
        }
        const auto* mapValue =
            std::get_if<ast::Map*>(&add->attributes[1]->values[0]);
        const auto* map = mapValue ? *mapValue : nullptr;
        CHECK(map && map->entries.size() == 2);
        if (map && map->entries.size() == 2) {
          const auto* first =
              std::get_if<std::int64_t>(&map->entries[0]->value);
          const auto* second =
              std::get_if<std::int64_t>(&map->entries[1]->value);
          CHECK(map->entries[0]->key == "a" && map->entries[1]->key == "a" &&
                first && *first == 1 && second && *second == 2);
        }
      }
    }
  }
  for (const auto [source, code] :
       {std::pair{"DGS006\nx\nan a", "dgs.version"},
        std::pair{"DGS004\nx\nan a -old:1", "dgs.remove-value"},
        std::pair{"DGS004\nx\nan a empty:{}", "dgs.empty-array"}}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:dgs", "dgs", 0, source));
    document.language = &services;
    document.parseResult = services.parser->parse(source);
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered() &&
          document.parseResult.diagnostics.empty());
    CHECK(diagnostics.size() == 1 && diagnostics[0].code &&
          *diagnostics[0].code == code);
    if (diagnostics.size() == 1 && std::string_view(code) == "dgs.version")
      CHECK(diagnostics[0].range.begin == 0 && diagnostics[0].range.end == 6);
  }
}
