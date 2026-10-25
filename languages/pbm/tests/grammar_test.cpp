#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "native_cases.hpp"
#include "pbm/core/Bitmap.hpp"
#include "pbm/core/CoreServices.hpp"
#include "pbm/generated/Pbm/Module.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
namespace ast = pbm::ast;
std::string source(const std::filesystem::path &file) {
  std::ifstream in(file, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), {});
}
int main() {
  const auto language =
      cursiv::create_language<pbm::PbmCoreServices, pbm::PbmSharedCoreServices,
                              pbm::install_core_module>();
  const auto factory = pbm::generated::Pbm::create_parser();
  bool ok = true;
  const auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "pbm: %s\n", message);
      ok = false;
    }
  };
  for (const auto &c : pbm_native::cases) {
    if (std::ranges::find(pbm_native::gaps, std::string_view(c.file)) !=
        pbm_native::gaps.end())
      continue;
    const auto text =
        source(std::filesystem::path(CURSIV_TEST_DIR) / "native" / c.file);
    const auto result = language.language.parser->parse(text);
    const auto other = factory->parse(text);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty(),
          c.file);
    check(result.references && result.references->handles.empty() &&
              other.references && other.references->handles.empty(),
          "bitmap has an empty reference table");
    const auto *doc = result.value_as<ast::Document>();
    const auto *second = other.value_as<ast::Document>();
    check(doc && second && result.cst().text(doc->cstNodeId) == text, c.file);
    if (!doc || !second)
      continue;
    check(doc->images.size() == second->images.size() &&
              doc->trailing == second->trailing,
          c.file);
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      check(seen.insert(&node).second, c.file);
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
        cursiv::test::validationHelper(language.language, text);
    if (*c.code) {
      bool found = false;
      for (const auto &d : validation.diagnostics)
        found |= d.code && d.code->isString() && d.code->string() == c.code;
      for (const auto &d : validation.diagnostics) {
        const std::string_view name(c.file);
        if (name == "magic.pbm")
          check(d.range.begin == 0 && d.range.end == 2,
                "magic diagnostic span");
        if (name == "width.pbm")
          check(d.range.begin == 3 && d.range.end == 4,
                "width diagnostic span");
        if (name == "pixel.pbm" || name == "short-binary.pbm")
          check(d.range.begin == 7 && d.range.end == 8,
                "raster diagnostic span");
        if (name == "trailing-pixel.pbm")
          check(d.range.begin == 8 && d.range.end == 9,
                "trailing diagnostic span");
      }
      check(found, c.file);
    } else {
      check(validation.diagnostics.empty() && doc->trailing.empty(), c.file);
      check(doc->images.size() == c.images.size(), c.file);
      for (std::size_t i = 0; i < doc->images.size() && i < c.images.size();
           ++i) {
        const auto &expected = c.images[i];
        const auto &image = *doc->images[i];
        const auto value = pbm::read_bitmap(image);
        std::string pixels;
        for (bool bit : value.pixels)
          pixels += bit ? '1' : '0';
        check(value && image.magic == expected.magic &&
                  value.width == expected.width &&
                  value.height == expected.height && pixels == expected.pixels,
              c.file);
      }
    }
    for (std::size_t i = 0; i < doc->images.size() && i < second->images.size();
         ++i) {
      const auto &a = *doc->images[i];
      const auto &b = *second->images[i];
      check(a.magic == b.magic && a.width == b.width && a.height == b.height &&
                a.separator == b.separator && a.raster == b.raster,
            c.file);
      for (const auto field : {std::pair{"magic", a.magic},
                               {"width", a.width},
                               {"height", a.height},
                               {"separator", a.separator},
                               {"raster", a.raster}}) {
        bool found = false;
        for (auto id = result.cst().subtree_begin(a.cstNodeId);
             id < a.cstNodeId; ++id)
          if (result.cst().grammar()->recordWrites(result.cst()[id],
                                                   field.first)) {
            found = true;
            check(result.cst().text(id) == field.second, c.file);
          }
        for (const auto &d : validation.diagnostics) {
          const std::string_view name(c.file);
          if (name == "magic.pbm")
            check(d.range.begin == 0 && d.range.end == 2,
                  "magic diagnostic span");
          if (name == "width.pbm")
            check(d.range.begin == 3 && d.range.end == 4,
                  "width diagnostic span");
          if (name == "pixel.pbm" || name == "short-binary.pbm")
            check(d.range.begin == 7 && d.range.end == 8,
                  "raster diagnostic span");
          if (name == "trailing-pixel.pbm")
            check(d.range.begin == 8 && d.range.end == 9,
                  "trailing diagnostic span");
        }
        check(found || field.second.empty(), c.file);
      }
    }
  }
  for (const char *dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto &entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".pbm") {
        const auto text = source(entry.path());
        const auto result = language.language.parser->parse(text);
        check(result.accepted && !result.cst().recovered() && result.value &&
                  cursiv::test::validationHelper(language.language, text)
                      .diagnostics.empty(),
              "included PBM");
      }
  std::printf("pbm: %zu native cases\n", pbm_native::cases.size());
  return !ok;
}
