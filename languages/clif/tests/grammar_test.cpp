// The grammar's smoke test: every document under example/ that is not
// expected to fail parses clean, and a few real field values read back
// correctly -- a converter that silently gave the wrong value would still
// pass a structural-only check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "clif/core/CoreServices.hpp"
#include "clif/core/ast.hpp"

namespace {

namespace ast = clif::ast;

int fail(const char* what) {
  std::fprintf(stderr, "clif: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

std::string_view name_of(const ast::Term* term) {
  const auto* name = as<ast::NameTerm>(term);
  return name != nullptr ? name->value : std::string_view{};
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      clif::ClifCoreServices, clif::ClifSharedCoreServices,
      clif::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  // module.clif: colore's own sample, wrapped in one named text.
  {
    const std::string text = read(CURSIV_MODULE_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("module.clif does not parse");
    if (!result.diagnostics.empty())
      return fail("module.clif carries a diagnostic");
    const auto* doc = as<ast::Document>(result.value);
    if (doc == nullptr) return fail("module.clif built no document");
    if (doc->items.size() != 1) return fail("module.clif: wrong item count");
    const auto* named = std::get_if<ast::NamedText*>(&doc->items[0]);
    if (named == nullptr || *named == nullptr)
      return fail("module.clif: the root item is not a named text");
    if ((*named)->name == nullptr ||
        name_of((*named)->name) != "http://colore.oor.net/algebra/module.clif")
      return fail("module.clif: the text name did not read back");
    if ((*named)->body == nullptr || (*named)->body->phrases.size() != 8)
      return fail("module.clif: wrong phrase count");
    if (as<ast::Module>((*named)->body->phrases[0]) == nullptr)
      return fail("module.clif: the first phrase is not a module");
    if (as<ast::QuantSentence>((*named)->body->phrases[2]) == nullptr)
      return fail("module.clif: the third phrase is not a quantified sentence");
  }

  // logic.clif: this port's own corpus, one of each remaining construct.
  {
    const std::string text = read(CURSIV_LOGIC_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("logic.clif does not parse");
    if (!result.diagnostics.empty())
      return fail("logic.clif carries a diagnostic");
    const auto* doc = as<ast::Document>(result.value);
    if (doc == nullptr) return fail("logic.clif built no document");
    if (doc->items.size() != 3) return fail("logic.clif: wrong item count");

    const auto* module = std::get_if<ast::Phrase*>(&doc->items[0]);
    const auto* geometry = module != nullptr ? as<ast::Module>(*module) : nullptr;
    if (geometry == nullptr || geometry->name == nullptr ||
        name_of(geometry->name) != "geometry")
      return fail("logic.clif: the module name did not read back");
    if (geometry->excludes.size() != 4) return fail("logic.clif: wrong excludes count");
    if (as<ast::NumeralTerm>(geometry->excludes[2]) == nullptr ||
        as<ast::NumeralTerm>(geometry->excludes[2])->value != "7")
      return fail("logic.clif: the numeral exclude did not read back");
    if (as<ast::StringTerm>(geometry->excludes[1]) == nullptr ||
        as<ast::StringTerm>(geometry->excludes[1])->value != "excluded shape")
      return fail("logic.clif: the quoted-string exclude did not unquote");

    const auto* named = std::get_if<ast::NamedText*>(&doc->items[1]);
    if (named == nullptr || *named == nullptr ||
        name_of((*named)->name) != "worked examples")
      return fail("logic.clif: the enclosed text name did not read back");

    bool found = false;
    for (const ast::Phrase* phrase : (*named)->body->phrases) {
      const auto* q = as<ast::QuantSentence>(phrase);
      if (q == nullptr || q->isExists || q->boundName != nullptr) continue;
      found = true;
      if (q->bounds == nullptr || q->bounds->items.size() != 2)
        return fail("logic.clif: wrong boundlist size");
      if (q->bounds->items[0]->sort == nullptr)
        return fail("logic.clif: the typed bound lost its sort");
      if (as<ast::SeqMarkTerm>(q->bounds->items[1]->name) == nullptr)
        return fail("logic.clif: the sequence marker did not read back");
    }
    if (!found) return fail("logic.clif: the forall was not found");
  }

  std::puts("clif: the samples parse");
  return 0;
}
