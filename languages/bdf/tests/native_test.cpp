#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "bdf/core/CoreServices.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

int main() {
  const auto language = cursiv::create_language<bdf::BdfCoreServices,
      bdf::BdfSharedCoreServices, bdf::install_core_module>();
  unsigned native = 0;
  for (const auto& c : nativeCases) {
    const auto fail = [&](const char* what) {
      std::fprintf(stderr, "%.*s: %s\n", static_cast<int>(c.name.size()), c.name.data(), what);
      return 1;
    };
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native-cases/" + std::string(c.file));
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto* font = cursiv::ast_cast<bdf::ast::Font>(result.value);
    if (!font || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    if (c.native) {
      ++native;
      if (native_view::font(*font).toJsonString({.pretty = false}) != c.nativeFont)
        return fail("native font/properties/metrics/bitmap");
    }
    std::vector<std::string_view> header, metrics;
    const bdf::ast::Properties* properties = nullptr;
    const bdf::ast::Char* glyph = nullptr;
    for (const auto& part : font->parts) {
      if (const auto* p = std::get_if<bdf::ast::Directive*>(&part); p && *p) header.push_back((*p)->keyword);
      if (const auto* p = std::get_if<bdf::ast::Properties*>(&part); p && *p) properties = *p;
      if (const auto* p = std::get_if<bdf::ast::Char*>(&part); p && *p) glyph = *p;
    }
    const std::vector<std::string_view> expectedHeader = c.name == "header-order"
        ? std::vector<std::string_view>{"FONTBOUNDINGBOX", "SIZE", "FONT", "CHARS"}
        : std::vector<std::string_view>{"FONT", "SIZE", "FONTBOUNDINGBOX", "CHARS"};
    if (header != expectedHeader || font->parts.size() != 6 || !properties || !glyph)
      return fail("declaration order/blocks");
    for (const auto& part : glyph->parts)
      if (const auto* p = std::get_if<bdf::ast::Directive*>(&part); p && *p) metrics.push_back((*p)->keyword);
    const std::vector<std::string_view> expectedMetrics = c.name == "glyph-order"
        ? std::vector<std::string_view>{"ENCODING", "DWIDTH", "SWIDTH", "BBX"}
        : std::vector<std::string_view>{"ENCODING", "SWIDTH", "DWIDTH", "BBX"};
    if (metrics != expectedMetrics || glyph->parts.size() != 5 ||
        bdf::argument(bdf::find_directive(glyph->parts, "SWIDTH"), 0) != "500")
      return fail("raw metric order/value");
    const auto* encoding = bdf::find_directive(glyph->parts, "ENCODING");
    if (!encoding || encoding->arguments.size() != (c.name == "secondary-encoding" ? 2u : 1u) ||
        bdf::argument(encoding, 0) != (c.name == "secondary-encoding" ? "-1" : "65") ||
        (c.name == "secondary-encoding" && bdf::argument(encoding, 1) != "65"))
      return fail("raw encoding");
    const auto rawAtom = c.name == "atom-spaces" ? "unquoted family name" :
                         c.name == "atom-quotes" ? "\"a\"\"b\"" : "\"Demo\"";
    if (properties->properties.size() != (c.name == "property-punctuation" ? 4u : 3u) ||
        properties->properties[2]->name != "FAMILY_NAME" || properties->properties[2]->value != rawAtom)
      return fail("raw property atom");
    std::vector<std::string_view> expectedRows{"80", "40"};
    if (c.name == "bpp2" || c.name == "wide-nine") expectedRows = {"8080", "4040"};
    if (c.name == "bpp4") expectedRows = {"12345678", "9ABCDEF0"};
    if (c.name == "zero-height") expectedRows.clear();
    if (c.name == "short-bitmap") expectedRows = {"80"};
    if (c.name == "short-row") expectedRows = {"8", "4"};
    if (c.name == "long-row") expectedRows = {"80AA", "40BB"};
    if (c.name == "nonhex-row") expectedRows = {"G0", "40"};
    const auto* bitmap = bdf::bitmap(*glyph);
    if (!bitmap || bitmap->rows.size() != expectedRows.size()) return fail("raw bitmap rows");
    for (std::size_t i = 0; i < expectedRows.size(); ++i)
      if (bitmap->rows[i] != expectedRows[i]) return fail("raw bitmap spelling");
    if (c.name == "properties-overflow" &&
        (properties->count.size() != 45 || properties->count.find_first_not_of('9') != properties->count.npos))
      return fail("raw unbounded count");
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (validation.diagnostics.size() != (c.error.empty() ? 0u : 1u)) return fail("runtime count");
    if (!c.error.empty() && (validation.diagnostics[0].code != c.error ||
        validation.diagnostics[0].range.begin >= validation.diagnostics[0].range.end ||
        validation.diagnostics[0].range.end > source.size())) return fail("runtime code/range");
    bool owned = font->parent == nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& n) -> void {
      if (!seen.insert(&n).second) { owned = false; return; }
      language.language.astReflection->visitMembers(n,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &n && result.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *font);
    if (!owned || seen.size() != result.nodes.size()) return fail("ownership");
  }
  std::printf("bdf: %zu source cases, %u native fonts with properties/metrics/bitmaps\n",
              std::size(nativeCases), native);
  return native == 19 ? 0 : 1;
}
