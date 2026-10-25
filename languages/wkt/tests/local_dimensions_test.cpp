#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "geometry_view.hpp"
#include "local_dimension_cases.hpp"
#include "wkt/core/CoreServices.hpp"
#include "wkt/generated/Wkt/Module.hpp"
#include <cstdio>
#include <unordered_set>

int main() {
  const auto language =
      cursiv::create_language<wkt::WktCoreServices, wkt::WktSharedCoreServices,
                              wkt::install_core_module>();
  auto factory = wkt::generated::Wkt::create_parser();
  for (const auto &c : wkt_local_cases::cases) {
    const std::string_view source(c.source);
    const auto result = language.language.parser->parse(source);
    const auto other = factory->parse(source);
    const bool positive = *c.expected;
    if (positive || c.markerError) {
      CHECK(result.accepted && !result.cst().recovered() &&
            result.diagnostics.empty());
      CHECK(other.accepted && !other.cst().recovered() &&
            other.diagnostics.empty());
    }
    const auto *doc = result.value_as<wkt::ast::Document>();
    CHECK(doc && doc->geometries.size() == 1);
    if (!doc || doc->geometries.size() != 1)
      continue;
    CHECK(result.cst().text() == source);
    std::unordered_set<const cursiv::AstNode *> reached;
    std::string markers;
    const auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      CHECK(reached.insert(&node).second);
      auto dim = wkt::ast::Dim::none;
      if (const auto *p = cursiv::ast_cast<wkt::ast::PointList>(&node))
        dim = p->dim;
      if (const auto *p = cursiv::ast_cast<wkt::ast::BarePolygon>(&node))
        dim = p->dim;
      if (const auto *p = cursiv::ast_cast<wkt::ast::MultiPointItem>(&node))
        dim = p->dim;
      if (dim != wkt::ast::Dim::none)
        markers += char('0' + static_cast<unsigned>(dim));
      language.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            CHECK(child.parent == &node &&
                  result.cst().contains(node.cstNodeId, child.cstNodeId));
            self(self, child);
          });
    };
    walk(walk, *doc);
    CHECK(reached.size() == result.nodes.size());
    const auto validation =
        cursiv::test::validationHelper(language.language, std::string(source));
    const wkt::Dimensions dimensions(*doc);
    if (positive) {
      CHECK(markers == c.markers);
      CHECK(validation.diagnostics.empty() && dimensions.issues().empty());
      const auto projected = wkt_test::geometry(*doc->geometries[0], dimensions)
                                 .toJsonString({.pretty = false});
      if (projected != c.expected)
        std::fprintf(stderr, "WKT native AST mismatch: %s\n", c.name);
      CHECK(projected == c.expected);
      const auto *second = other.value_as<wkt::ast::Document>();
      CHECK(second && second->geometries.size() == 1);
      if (second && second->geometries.size() == 1)
        CHECK(
            wkt_test::geometry(*second->geometries[0], wkt::Dimensions(*second))
                .toJsonString({.pretty = false}) == c.expected);
    } else if (c.markerError) {
      CHECK(markers == c.markers);
      CHECK(validation.diagnostics.size() == 1 &&
            validation.diagnostics[0].code &&
            validation.diagnostics[0].code->string() == "wkt.dimension-marker");
      if (validation.diagnostics.size() == 1) {
        const auto &range = validation.diagnostics[0].range;
        const auto marker = source.substr(range.begin, range.end - range.begin);
        CHECK(marker == "Z" || marker == "M" || marker == "ZM");
      }
    } else
      CHECK(result.cst().recovered() || !result.diagnostics.empty() ||
            !validation.diagnostics.empty());
  }
  const std::string extension =
      "GEOMETRYCOLLECTION Z(TIN(((0 0 1,1 0 1,0 1 1,0 0 1))))";
  const auto retained = language.language.parser->parse(extension);
  CHECK(retained.accepted && !retained.cst().recovered() &&
        retained.diagnostics.empty());
  CHECK(cursiv::test::validationHelper(language.language, extension)
            .diagnostics.empty());
  return cursiv::test::report("wkt_local_dimensions_test");
}
