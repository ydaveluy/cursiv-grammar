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
#include "gff3/core/CoreServices.hpp"

int main() {
  const auto language = cursiv::create_language<gff3::Gff3CoreServices,
                                                gff3::Gff3SharedCoreServices,
                                                gff3::install_core_module>();
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
    const auto *domain = cursiv::ast_cast<gff3::ast::Document>(result.value);
    if (!domain || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    if (!c.native.empty()) {
      ++native;
      if (document_view(*domain).toJsonString({.pretty = false}) != c.native)
        return fail(
            "native feature fields, ordered directives, attributes and FASTA");
    }
    for (const auto &line : domain->lines) {
      if (const auto *f = std::get_if<gff3::ast::Feature *>(&line); f && *f) {
        for (const auto *attribute : (*f)->attributes) {
          std::string spelling(attribute->key);
          if (attribute->hasEquals) {
            spelling += '=';
            for (std::size_t i = 0; i < attribute->values.size(); ++i) {
              if (i)
                spelling += ',';
              spelling += attribute->values[i];
            }
          }
          if (spelling != result.cst().text(attribute->cstNodeId))
            return fail("raw attribute spelling");
        }
        if (c.name == "long-position" &&
            ((*f)->start != "0000000000000000000000001" ||
             (*f)->end != "99999999999999999999999999999999999"))
          return fail("raw unbounded coordinates");
        if (c.name == "eight-columns" && (*f)->hasAttributeColumn)
          return fail("missing ninth column");
        if (c.name == "missing-attrs" && !(*f)->attributesMissing)
          return fail("missing attribute marker");
        if (c.name == "extra-columns" &&
            ((*f)->extra.size() != 2 || (*f)->extra[1] != "unknown data"))
          return fail("unknown columns");
      }
    }
    if (c.name == "fasta" &&
        (!domain->fasta || !domain->fasta->marked ||
         domain->fasta->sequences.size() != 2 ||
         domain->fasta->sequences[0]->description != "first record" ||
         domain->fasta->sequences[0]->lines.size() != 2))
      return fail("FASTA header, line boundaries and description");
    if (c.name == "bare-fasta" && (!domain->fasta || domain->fasta->marked))
      return fail("optional FASTA marker");
    if (c.name == "fasta-leading-comment" &&
        (!domain->fasta || domain->fasta->leadingComments.size() != 1 ||
         domain->fasta->leadingComments[0] != ";before sequence"))
      return fail("raw leading FASTA comment");
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    if (validation.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty() &&
        (validation.diagnostics[0].code != c.error ||
         validation.diagnostics[0].range.begin >
             validation.diagnostics[0].range.end ||
         validation.diagnostics[0].range.end > source.size()))
      return fail("runtime code/range");
    bool owned = domain->parent == nullptr;
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
    walk(walk, *domain);
    if (!owned || seen.size() != result.nodes.size())
      return fail("ownership");
  }
  std::printf("gff3: %zu source files, %u native documents\n",
              std::size(nativeCases), native);
  return native == 38 ? 0 : 1;
}
