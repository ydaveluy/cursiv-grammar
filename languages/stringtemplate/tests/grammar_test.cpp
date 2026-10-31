#include "audit_regressions.hpp"
#include "native_cases.hpp"
#include "builtin_cases.hpp"
// The grammar itself: the upstream corpus parses clean, a hand-written
// document exercises every construct the language accepts, and a few field
// values are read back out of the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "stringtemplate/core/CoreServices.hpp"
#include "stringtemplate/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "stringtemplate: %s\n", what);
  return 1;
}

std::string read_file(const char* path) {
  std::ifstream file(path);
  if (!file) return {};
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      stringtemplate::StringtemplateCoreServices,
      stringtemplate::StringtemplateSharedCoreServices,
      stringtemplate::install_core_module>();

  if(native_regressions::run(language)) return 1;
  if (!stringtemplate_builtins::run(language.language)) return 1;

  // The upstream corpus: one file, `<title>`.
  const std::string corpus = read_file(CURSIV_EXAMPLE_DIR "/example1.st");
  if (corpus.empty()) return fail("the upstream example is not readable");

  {
    const cursiv::ParseResult result = language.language.parser->parse(corpus);
    if (result.cst().recovered()) return fail("the upstream example does not parse");
    if (!result.diagnostics.empty())
      return fail("the upstream example raised a diagnostic");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<stringtemplate::ast::Template>)
      return fail("the upstream example built no template");

    const auto& tmpl = *static_cast<const stringtemplate::ast::Template*>(root);
    if (tmpl.elements.size() != 2 || tmpl.elements[1]->type != &cursiv::ast_type_tag<stringtemplate::ast::Text> || static_cast<const stringtemplate::ast::Text*>(tmpl.elements[1])->value != "\n\n")
      return fail("the upstream example must keep its expression tag and trailing newlines");
    const stringtemplate::ast::Element* el = tmpl.elements[0];
    if (el == nullptr ||
        el->type != &cursiv::ast_type_tag<stringtemplate::ast::ExprTag>)
      return fail("<title> did not build an ExprTag");
    const auto& tag = *static_cast<const stringtemplate::ast::ExprTag*>(el);
    const auto* value = tag.value;
    if (value == nullptr ||
        value->type != &cursiv::ast_type_tag<stringtemplate::ast::MapExpr>)
      return fail("the expression tag did not hold a map expression");
    const auto& map = *static_cast<const stringtemplate::ast::MapExpr*>(value);
    if (map.sources.size() != 1 ||
        map.sources[0]->type !=
            &cursiv::ast_type_tag<stringtemplate::ast::NameExpr>)
      return fail("<title> did not read back as a bare name");
    const auto& name =
        *static_cast<const stringtemplate::ast::NameExpr*>(map.sources[0]);
    if (name.name != "title") return fail("the name did not read back as title");
  }

  // A document of this port's own: every construct the language accepts
  // that the one-file upstream corpus does not exercise.
  const std::string full = read_file(CURSIV_TEST_DIR "/full.st");
  if (full.empty()) return fail("full.st is not readable");

  {
    const cursiv::ParseResult result = language.language.parser->parse(full);
    if (result.cst().recovered()) return fail("full.st does not parse");
    if (!result.diagnostics.empty())
      return fail("full.st raised a diagnostic");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<stringtemplate::ast::Template>)
      return fail("full.st built no template");

    const auto& tmpl = *static_cast<const stringtemplate::ast::Template*>(root);
    bool sawIf = false, sawRegion = false;
    for (const stringtemplate::ast::Element* el : tmpl.elements) {
      if (el == nullptr) continue;
      if (el->type == &cursiv::ast_type_tag<stringtemplate::ast::IfStat>) {
        const auto& ifs = *static_cast<const stringtemplate::ast::IfStat*>(el);
        if (ifs.branches.size() != 2 || !ifs.hasElse || ifs.elseBody.empty())
          return fail("the if/elseif/else did not read back");
        sawIf = true;
      }
      if (el->type == &cursiv::ast_type_tag<stringtemplate::ast::Region>) {
        const auto& region = *static_cast<const stringtemplate::ast::Region*>(el);
        if (region.name != "header")
          return fail("the region name did not read back");
        sawRegion = true;
      }
    }
    if (!sawIf) return fail("full.st did not build an IfStat");
    if (!sawRegion) return fail("full.st did not build a Region");
  }

  std::puts("stringtemplate: the corpus and the hand-written sample parse");
  return 0;
}
