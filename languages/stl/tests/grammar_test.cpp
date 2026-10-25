#include "audit_regressions.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "native_cases.hpp"
#include "stl/core/Coordinates.hpp"
#include "stl/core/CoreServices.hpp"
#include "stl/generated/Stl/Module.hpp"
#include <bit>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
namespace ast = stl::ast;
std::string bits(double value) {
  if (std::isnan(value))
    return std::signbit(value) ? "-nan" : "nan";
  char text[17];
  std::snprintf(
      text, sizeof text, "%016llx",
      static_cast<unsigned long long>(std::bit_cast<std::uint64_t>(value)));
  return text;
}
std::string read(const std::filesystem::path &file) {
  std::ifstream in(file, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), {});
}
int main() {
  if (!audit_regressions::run())
    return 1;
  const auto language =
      cursiv::create_language<stl::StlCoreServices, stl::StlSharedCoreServices,
                              stl::install_core_module>();
  auto factory = stl::generated::Stl::create_parser();
  bool ok = true;
  const auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "stl: %s\n", message);
      ok = false;
    }
  };
  for (const auto &c : stl_native::cases) {
    if (c.binary)
      continue;
    const auto source =
        read(std::filesystem::path(CURSIV_TEST_DIR) / "native" / c.file);
    const auto result = language.language.parser->parse(source);
    const auto other = factory->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          c.file);
    const auto *doc = result.value_as<ast::Document>();
    const auto *second = other.value_as<ast::Document>();
    check(doc && second, c.file);
    if (!doc || !second)
      continue;
    check(doc->binary == c.binary && doc->binary == second->binary &&
              doc->solids.size() == second->solids.size(),
          c.file);
    check(result.references && other.references &&
              result.references->handles.empty() &&
              other.references->handles.empty(),
          "empty reference table");
    check(result.cst().text() == source, c.file);
    const auto rootText = result.cst().text(doc->cstNodeId);
    check(source.starts_with(rootText) &&
              std::string_view(source)
                      .substr(rootText.size())
                      .find_first_not_of(" \t\r\n\v\f") ==
                  std::string_view::npos,
          "root span and trailing trivia");
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) {
        check(false, c.file);
        return;
      }
      language.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            check(child.parent == &node &&
                      result.cst().contains(node.cstNodeId, child.cstNodeId),
                  c.file);
            self(self, child);
          });
    };
    walk(walk, *doc);
    check(seen.size() == result.nodes.size(), c.file);
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    if (*c.code) {
      bool found = false;
      for (const auto &d : validation.diagnostics)
        found |= d.code && d.code->isString() && d.code->string() == c.code;
      check(found, c.file);
    } else {
      check(validation.diagnostics.empty(), c.file);
      std::size_t index = 0;
      for (const auto *solid : doc->solids)
        for (const auto *facet : solid->facets) {
          check(index < c.facets.size(), c.file);
          if (index >= c.facets.size())
            continue;
          const auto &expected = c.facets[index++];
          const auto normal =
              facet->normal ? stl::coordinates(*facet->normal) : std::nullopt;
          check(normal && facet->vertices.size() == 3 &&
                    facet->attribute == expected.attribute,
                c.file);
          if (normal)
            for (unsigned j = 0; j < 3; ++j)
              check(bits((*normal)[j]) == expected.normal[j], c.file);
          for (unsigned v = 0; v < facet->vertices.size() && v < 3; ++v) {
            const auto point = stl::coordinates(*facet->vertices[v]);
            check(point.has_value(), c.file);
            if (point)
              for (unsigned j = 0; j < 3; ++j)
                check(bits((*point)[j]) == expected.vertices[v][j], c.file);
          }
        }
      check(index == c.facets.size(), c.file);
    }
    for (const auto *node : result.nodes)
      if (const auto *triple = cursiv::ast_cast<ast::Triple>(node)) {
        const auto text = result.cst().text(triple->cstNodeId);
        check(text.find(triple->i) != text.npos &&
                  text.find(triple->j) != text.npos &&
                  text.find(triple->k) != text.npos,
              "coordinate source captures");
      }
    if (doc->binary) {
      check(doc->header ==
                source.substr(0, std::min(std::size_t(80), source.size())),
            "binary header bytes");
      check(doc->countBytes == std::string_view(source).substr(
                                   std::min(std::size_t(80), source.size()), 4),
            "binary count bytes");
      for (const auto *node : result.nodes)
        if (const auto *triple = cursiv::ast_cast<ast::Triple>(node)) {
          check(triple->binary && triple->i.size() == 4 &&
                    triple->j.size() == 4 && triple->k.size() == 4,
                "binary scalar bytes");
        }
    } else
      for (const auto *solid : doc->solids) {
        if (std::string_view(c.file) == "ascii-spaces.stl")
          check(solid->name == "my mesh 2026", "full solid name");
        if (std::string_view(c.file) == "ascii-close-different.stl")
          check(solid->name == "open" && solid->endName == "other" &&
                    solid->hasEnd,
                "independent closing label");
        if (std::string_view(c.file) == "ascii-close-omitted.stl")
          check(solid->hasEnd && solid->endName.empty(),
                "optional closing label");
      }
  }
  for (std::string_view number : {"nan()", "nan(123)", "NaN(payload_42)",
                                  "-nan(0xAF)", "+NaN(a)", "nan(123abc)"}) {
    const std::string source =
        "solid review\nfacet normal 0 0 2\nouter loop\nvertex " +
        std::string(number) +
        " 0 0\nvertex 0 1 0\nvertex 0 0 1\nendloop\nendfacet\nendsolid\n";
    const auto result = language.language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          "NaN payload parses");
    const auto *doc = result.value_as<ast::Document>();
    if (!doc || doc->solids.size() != 1 || doc->solids[0]->facets.size() != 1 ||
        doc->solids[0]->facets[0]->vertices.size() != 3) {
      check(false, "NaN payload facet");
      continue;
    }
    const auto *point = doc->solids[0]->facets[0]->vertices[0];
    const auto value = stl::coordinate(point->i, point->binary);
    check(point->i == number && value && std::isnan(*value) &&
              std::signbit(*value) == number.starts_with('-'),
          "NaN payload source and sign");
    check(cursiv::test::validationHelper(language.language, source)
              .diagnostics.empty(),
          "NaN payload validation");
  }
  for (const char *dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    if (!std::filesystem::exists(dir))
      continue;
    for (const auto &entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".stl") {
        const auto source = read(entry.path());
        const auto result = language.language.parser->parse(source);
        check(result.accepted && !result.cst().recovered() && result.value &&
                  cursiv::test::validationHelper(language.language, source)
                      .diagnostics.empty(),
              "included STL");
      }
  }
  std::printf("stl: %zu native cases\n", stl_native::cases.size());
  return !ok;
}
