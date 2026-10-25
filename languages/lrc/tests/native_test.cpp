#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>

#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "lrc/core/CoreServices.hpp"

std::string_view line_body(std::string_view text) {
  if (text.ends_with("\r\n"))
    text.remove_suffix(2);
  else if (text.ends_with('\n') || text.ends_with('\r'))
    text.remove_suffix(1);
  return text;
}

int main() {
  const auto language =
      cursiv::create_language<lrc::LrcCoreServices, lrc::LrcSharedCoreServices,
                              lrc::install_core_module>();
  unsigned equal = 0, different = 0;
  for (const auto &c : nativeCases) {
    const auto fail = [&](const char *what) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), what);
      return 1;
    };
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native-cases/" +
                           std::string(c.file),
                       std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto *doc = cursiv::ast_cast<lrc::ast::Document>(result.value);
    if (!doc || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    if (!c.native.empty()) {
      const auto view = lyric_view(*doc).toJsonString({.pretty = false});
      if (c.readerDifference) {
        ++different;
        if (view == c.native)
          return fail("reader difference unexpectedly disappeared");
        const auto *line = doc->lines.size() == 1
                               ? cursiv::ast_cast<lrc::ast::Line>(doc->lines[0])
                               : nullptr;
        if (!line || line->timestamps.size() != 1 ||
            line->timestamps[0]->minutes != "000" ||
            lrc::milliseconds(*line->timestamps[0]) != 1000)
          return fail("permissive raw minute spelling");
      } else {
        ++equal;
        if (view != c.native)
          return fail("native metadata, sorted times and lyric text");
      }
    }
    for (const auto *row : doc->lines) {
      std::string spelling;
      if (const auto *line = cursiv::ast_cast<lrc::ast::Line>(row)) {
        for (const auto *time : line->timestamps) {
          std::string stamp = "[" + std::string(time->minutes) + ":" +
                              std::string(time->seconds);
          if (time->fraction)
            stamp += "." + std::string(*time->fraction);
          stamp += ']';
          if (stamp != result.cst().text(time->cstNodeId))
            return fail("raw timestamp digits/delimiters");
          spelling += stamp;
          if (c.name == "minute-overflow" &&
              (time->minutes.size() != 45 ||
               time->minutes.find_first_not_of('9') != time->minutes.npos))
            return fail("raw minute overflow");
        }
        spelling += line->text;
      } else if (const auto *meta = cursiv::ast_cast<lrc::ast::Metadata>(row)) {
        spelling = "[" + std::string(meta->key) + ":" +
                   std::string(meta->value) + "]" + std::string(meta->tail);
        if (c.name == "overflow-offset" && meta->key == "offset" &&
            meta->value.size() != 400)
          return fail("raw offset overflow");
      } else if (const auto *raw = cursiv::ast_cast<lrc::ast::RawLine>(row))
        spelling = raw->text;
      else
        return fail("row kind");
      if (spelling != line_body(result.cst().text(row->cstNodeId)))
        return fail("raw row contents and order");
    }
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    if (validation.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty() &&
        (validation.diagnostics[0].code != c.error ||
         validation.diagnostics[0].range.begin >=
             validation.diagnostics[0].range.end ||
         validation.diagnostics[0].range.end > source.size()))
      return fail("runtime code/range");
    bool owned = doc->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) {
        owned = false;
        return;
      }
      language.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *doc);
    if (!owned || seen.size() != result.nodes.size())
      return fail("ownership");
  }
  std::printf("lrc: %zu files, %u equal native objects, %u preserved reader "
              "difference\n",
              std::size(nativeCases), equal, different);
  return equal == 36 && different == 1 ? 0 : 1;
}
