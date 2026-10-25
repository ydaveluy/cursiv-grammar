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
#include "recfile/core/CoreServices.hpp"

int main() {
  const auto language =
      cursiv::create_language<recfile::RecfileCoreServices,
                              recfile::RecfileSharedCoreServices,
                              recfile::install_core_module>();
  unsigned native = 0;
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
    const auto *doc = cursiv::ast_cast<recfile::ast::Document>(result.value);
    if (!doc || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    if (!c.native.empty()) {
      ++native;
      if (sets_view(*doc).toJsonString({.pretty = false}) != c.native)
        return fail("native sets, descriptors, records, fields and comments");
    }
    auto checkLine = [&](const recfile::ast::Line &line) {
      if (const auto *field = cursiv::ast_cast<recfile::ast::Field>(&line)) {
        std::string spelling =
            std::string(field->indentation) + std::string(field->key) + ":";
        for (const auto *part : field->parts) {
          std::string raw = part->join == recfile::ast::Join::newline  ? "\n+"
                            : part->join == recfile::ast::Join::splice ? "\\\n"
                                                                       : "";
          raw += part->text;
          if (raw != result.cst().text(part->cstNodeId))
            return false;
          spelling += raw;
        }
        if (field->terminated)
          spelling += '\n';
        return spelling == result.cst().text(field->cstNodeId);
      }
      if (const auto *comment =
              cursiv::ast_cast<recfile::ast::Comment>(&line)) {
        std::string spelling = std::string(comment->indentation) + "#";
        for (std::size_t i = 0; i < comment->lines.size(); ++i) {
          if (i)
            spelling += "\n#";
          spelling += comment->lines[i];
        }
        auto raw = result.cst().text(comment->cstNodeId);
        if (raw.ends_with('\n'))
          raw.remove_suffix(1);
        return raw == spelling;
      }
      if (const auto *invalid =
              cursiv::ast_cast<recfile::ast::RawLine>(&line)) {
        auto raw = result.cst().text(invalid->cstNodeId);
        if (raw.ends_with('\n'))
          raw.remove_suffix(1);
        return raw == invalid->text;
      }
      return false;
    };
    for (const auto &entry : doc->entries) {
      if (const auto *record = std::get_if<recfile::ast::Record *>(&entry);
          record && *record) {
        if ((*record)->lines.empty() ||
            !cursiv::ast_cast<recfile::ast::Field>((*record)->lines[0]))
          return fail("record starts with a field");
        for (const auto *line : (*record)->lines)
          if (!checkLine(*line))
            return fail("raw record line/value parts");
      } else if (const auto *comment =
                     std::get_if<recfile::ast::Comment *>(&entry);
                 comment && *comment) {
        if (!checkLine(**comment))
          return fail("raw top comment");
      } else if (const auto *raw = std::get_if<recfile::ast::RawLine *>(&entry);
                 raw && *raw) {
        if (!checkLine(**raw))
          return fail("raw invalid line");
      } else
        return fail("entry kind");
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
  std::printf("recfile: %zu source files, %u native databases\n",
              std::size(nativeCases), native);
  return native == 34 ? 0 : 1;
}
