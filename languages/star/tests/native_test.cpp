#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "star/core/CoreServices.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>
int main() {
  const auto language = cursiv::create_language<star::StarCoreServices,
                                                star::StarSharedCoreServices,
                                                star::install_core_module>();
  unsigned cif = 0, nmr = 0;
  for (const auto &c : nativeCases) {
    auto fail = [&](const char *what) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), what);
      return 1;
    };
    std::ifstream in(std::string(CURSIV_TEST_DIR) + "/native-cases/" +
                         std::string(c.file),
                     std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(in)), {});
    const auto result = language.language.parser->parse(source);
    const auto *doc = cursiv::ast_cast<sa::Doc>(result.value);
    if (!doc || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    syntax = c.star2 ? star::StringSyntax::star2 : star::StringSyntax::cif1;
    if (!c.cif.empty()) {
      ++cif;
      if (doc_view(*doc).toJsonString({.pretty = false}) != c.cif)
        return fail("native CIF graph");
    }
    if (!c.nmr.empty()) {
      ++nmr;
      if (nmr_view(*doc).toJsonString({.pretty = false}) != c.nmr)
        return fail("native NMR graph");
    }
    bool owned = !doc->parent, raw = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto inside = [&](std::string_view v) {
      auto lo = reinterpret_cast<std::uintptr_t>(source.data()),
           p = reinterpret_cast<std::uintptr_t>(v.data());
      return p >= lo && p - lo <= source.size() &&
             v.size() <= source.size() - (p - lo);
    };
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto *s = cursiv::ast_cast<sa::StringLit>(&n))
        raw &= inside(s->raw);
      if (auto *s = cursiv::ast_cast<sa::Literal>(&n))
        raw &= inside(s->text);
      if (auto *k = cursiv::ast_cast<sa::KeyVal>(&n))
        raw &= inside(k->name);
      if (auto *b = cursiv::ast_cast<sa::DataBlock>(&n))
        raw &= inside(b->name);
      if (auto *f = cursiv::ast_cast<sa::SaveFrame>(&n))
        raw &= inside(f->name);
      if (auto *h = cursiv::ast_cast<sa::LoopHeader>(&n))
        for (auto &v : h->items)
          if (auto *s = std::get_if<std::string_view>(&v))
            raw &= inside(*s);
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     result.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *doc);
    if (!owned || !raw || seen.size() != result.nodes.size())
      return fail("ownership/source-backed fields");
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    std::vector<std::string> codes;
    for (auto &d : validation.diagnostics) {
      if (!d.code || !d.code->isString() || d.range.begin >= d.range.end ||
          d.range.end > source.size())
        return fail("diagnostic range/code");
      codes.push_back(d.code->string());
    }
    std::sort(codes.begin(), codes.end());
    std::string errors;
    for (const auto &s : codes) {
      if (!errors.empty())
        errors += '|';
      errors += s;
    }
    if (errors != c.errors)
      return fail("runtime diagnostics");
    if (c.name == "header-explicit-stop") {
      auto *b = std::get<sa::DataBlock *>(doc->blocks[0]);
      auto *l = std::get<sa::Loop *>(b->elements[0]);
      if (l->header->items.size() != 3 ||
          std::get<std::string_view>(l->header->items[0]) != "_outer" ||
          std::get<std::string_view>(l->header->items[2]) != "_after" ||
          l->values.size() != 4 ||
          !cursiv::ast_cast<sa::Stop>(l->values.back()))
        return fail("ordered nested header/body");
      auto *h = std::get<sa::LoopHeader *>(l->header->items[1]);
      if (!h->closed || h->items.size() != 1 ||
          std::get<std::string_view>(h->items[0]) != "_inner")
        return fail("explicit nested header close");
    }
    if (c.name == "global") {
      if (doc->blocks.size() != 3)
        return fail("global block order");
      auto *g = std::get<sa::Global *>(doc->blocks[1]);
      if (g->elements.size() != 1 ||
          std::get<sa::KeyVal *>(g->elements[0])->name != "_b")
        return fail("global owns content");
    }
    if (c.name == "original-example4") {
      auto *b = std::get<sa::DataBlock *>(doc->blocks[0]);
      auto *l = std::get<sa::Loop *>(b->elements[0]);
      unsigned stops = 0;
      for (auto *v : l->values)
        stops += cursiv::ast_cast<sa::Stop>(v) != nullptr;
      if (l->header->items.size() != 3 || !star::nested_header(*l->header) ||
          stops != 3)
        return fail("nested loop markers");
    }
  }
  std::printf("STAR: %zu sources, %u CIF/STAR2 graphs, %u NMR graphs\n",
              std::size(nativeCases), cif, nmr);
  return cif == 35 && nmr == 8 ? 0 : 1;
}
