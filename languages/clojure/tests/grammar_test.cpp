// The smoke test of the language: parse every document under example/ and
// check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "clojure/core/CoreServices.hpp"
#include "clojure/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "clojure: %s\n", what.c_str());
  return 1;
}

// No RTTI in the AST: a node is identified by its tag, not a dynamic_cast.
template <class T> const T *as(const clojure::ast::Form *form) {
  if (form == nullptr || form->type != &cursiv::ast_type_tag<T>)
    return nullptr;
  return static_cast<const T *>(form);
}

std::string read_file(const std::filesystem::path &path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

int reader_values(const cursiv::Parser &parser) {
  namespace ast = clojure::ast;
  const std::string literals =
      R"clj([+1 +1.25 1. 16rff -0xFF -0xFFN \uABCD \o101 ##Inf ##-Inf ##NaN])clj";
  const auto numbers = parser.parse(literals);
  const auto *file = numbers.value_as<ast::File>();
  const auto *values = file && file->forms.size() == 1
                           ? as<ast::VectorForm>(file->forms[0])
                           : nullptr;
  if (!numbers.accepted || numbers.cst().recovered() ||
      !numbers.diagnostics.empty() || !values || values->items.size() != 11 ||
      !as<ast::RadixLit>(values->items[3]) ||
      !as<ast::CharLit>(values->items[6]) ||
      !as<ast::CharLit>(values->items[7]) ||
      !as<ast::FloatLit>(values->items[8]))
    return fail("reader numeric forms were lost");
  const std::string source = R"clj(["A\n\u0042" #"\d+\\w"])clj";
  const auto parsed = parser.parse(source);
  const auto *doc = parsed.value_as<ast::File>();
  const auto *vector = doc && doc->forms.size() == 1
                           ? as<ast::VectorForm>(doc->forms[0])
                           : nullptr;
  if (!vector || vector->items.size() != 2)
    return fail("string/regex forms were lost");
  const auto *text = as<ast::StringLit>(vector->items[0]);
  const auto *regex = as<ast::RegexForm>(vector->items[1]);
  if (!text || text->text != "A\nB" || !regex || !regex->pattern ||
      regex->pattern->text != R"(\d+\\w)")
    return fail("strings and regex escapes were conflated");
  const std::string metadata = "#^:private foo";
  const auto meta_result = parser.parse(metadata);
  const auto *meta_doc = meta_result.value_as<ast::File>();
  const auto *meta = meta_doc && meta_doc->forms.size() == 1
                         ? as<ast::MetaDataForm>(meta_doc->forms[0])
                         : nullptr;
  const auto *key = meta ? as<ast::Keyword>(meta->meta) : nullptr;
  const auto *target = meta ? as<ast::Symbol>(meta->form) : nullptr;
  if (!key || key->name != "private" || !target || target->text != "foo")
    return fail("old metadata spelling lost its metadata or target");
  const std::string forms = "foo;tail\nbar foo'bar foo#bar foo%bar";
  const auto symbols = parser.parse(forms);
  const auto *symbol_doc = symbols.value_as<ast::File>();
  if (!symbol_doc || symbol_doc->forms.size() != 5)
    return fail("symbol delimiters changed the form count");
  const char *expected[] = {"foo", "bar", "foo'bar", "foo#bar", "foo%bar"};
  for (std::size_t i = 0; i < std::size(expected); ++i) {
    const auto *symbol = as<ast::Symbol>(symbol_doc->forms[i]);
    if (!symbol || symbol->text != expected[i])
      return fail("symbol spelling changed");
  }
  const std::string map_source = "{#_ :skip :k #_ 1 2}";
  const auto maps = parser.parse(map_source);
  const auto *map_doc = maps.value_as<ast::File>();
  const auto *map = map_doc && map_doc->forms.size() == 1
                        ? as<ast::MapForm>(map_doc->forms[0])
                        : nullptr;
  if (!map || map->entries.size() != 1 || map->discards.size() != 1 ||
      map->entries[0]->discards.size() != 1 ||
      !as<ast::LongLit>(map->entries[0]->value) ||
      as<ast::LongLit>(map->entries[0]->value)->text != "2")
    return fail("discard shifted a map key/value pair");
  const std::string quote_source = "' #_ 1 2 #_ #_ 3 4 5";
  const auto quoted = parser.parse(quote_source);
  const auto *quote_doc = quoted.value_as<ast::File>();
  const auto *quote = quote_doc && quote_doc->forms.size() == 3
                          ? as<ast::QuoteForm>(quote_doc->forms[0])
                          : nullptr;
  const auto *outer = quote_doc && quote_doc->forms.size() == 3
                          ? as<ast::DiscardForm>(quote_doc->forms[1])
                          : nullptr;
  if (!quote || quote->discards.size() != 1 || !as<ast::LongLit>(quote->form) ||
      as<ast::LongLit>(quote->form)->text != "2" || !outer ||
      outer->discards.size() != 1 || !as<ast::LongLit>(outer->form) ||
      as<ast::LongLit>(outer->form)->text != "4")
    return fail("reader macros did not skip discarded operands");
  const std::string namespaced = "#:: {:a 1} [#?@(:clj [1 2])]";
  const auto conditional = parser.parse(namespaced);
  bool auto_namespace = false, splicing = false;
  for (const auto *node : conditional.nodes) {
    if (const auto *m = cursiv::ast_cast<ast::MapForm>(node))
      auto_namespace = m->autoNamespace && m->entries.size() == 1;
    if (const auto *c = cursiv::ast_cast<ast::ConditionalForm>(node))
      splicing = c->splicing && c->form;
  }
  return auto_namespace && splicing && !conditional.cst().recovered()
             ? 0
             : fail("namespace map or reader conditional was lost");
}

} // namespace

int main() {
  const auto language =
      cursiv::create_language<clojure::ClojureCoreServices,
                              clojure::ClojureSharedCoreServices,
                              clojure::install_core_module>();
  if (reader_values(*language.language.parser))
    return 1;

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result = language.language.parser->parse("[22/7 1.0M]");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      std::fprintf(stderr, "audit regression: parse failed\n");
      return 1;
    }
    bool ratio = false, decimal = false;
    for (const auto *node : result.nodes) {
      if (const auto *value = cursiv::ast_cast<clojure::ast::RatioLit>(node))
        ratio |= value->text == "22/7";
      if (const auto *value = cursiv::ast_cast<clojure::ast::DecimalLit>(node))
        decimal |= value->text == "1.0M";
    }
    if (!ratio || !decimal)
      return 1;
  }

  // Every document under example/ parses clean: no recovery, no diagnostics,
  // and the root is a File. metadata.clj and reader-macros.clj live beside
  // this test, not in the corpus.
  std::vector<std::filesystem::path> paths;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto &entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".clj")
        paths.push_back(entry.path());

  if (paths.empty())
    return fail("no example documents found");

  // The text of every document stays alive for the whole run: every
  // string_view in a tree points into it.
  std::vector<std::string> texts;
  texts.reserve(paths.size());
  std::vector<cursiv::ParseResult> results;
  results.reserve(paths.size());

  for (const auto &path : paths) {
    texts.push_back(read_file(path));
    cursiv::ParseResult result = language.language.parser->parse(texts.back());
    if (result.cst().recovered())
      return fail(path.string() + ": the document does not parse");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": unexpected diagnostics");
    if (result.value == nullptr ||
        result.value->type != &cursiv::ast_type_tag<clojure::ast::File>)
      return fail(path.string() + ": the parse built no file");
    results.push_back(std::move(result));
  }

  // The corpus sample: `(ns helloworld.core)` then a `defn` with a doc
  // string, an empty param vector and a call.
  const clojure::ast::File *sample = nullptr;
  for (std::size_t i = 0; i < paths.size(); ++i)
    if (paths[i].filename() == "example1.clj")
      sample = static_cast<const clojure::ast::File *>(results[i].value);
  if (sample == nullptr)
    return fail("the corpus sample was not found");
  if (sample->forms.size() != 2)
    return fail("the corpus sample does not hold two top-level forms");

  const auto *ns_form = as<clojure::ast::ListForm>(sample->forms[0]);
  if (ns_form == nullptr || ns_form->items.size() != 2)
    return fail("(ns helloworld.core) did not parse as a two-item list");
  const auto *ns_sym = as<clojure::ast::Symbol>(ns_form->items[1]);
  if (ns_sym == nullptr || ns_sym->text != "helloworld.core")
    return fail("the ns form does not name helloworld.core");

  const auto *defn_form = as<clojure::ast::ListForm>(sample->forms[1]);
  if (defn_form == nullptr || defn_form->items.size() != 5)
    return fail("the defn form does not hold five items");
  const auto *doc = as<clojure::ast::StringLit>(defn_form->items[2]);
  // A converter that silently mis-decoded the string would still pass a
  // structural check: read the text back.
  if (doc == nullptr || doc->text != "I can say 'Hello World'.")
    return fail("the doc string did not decode to the written text");
  const auto *params = as<clojure::ast::VectorForm>(defn_form->items[3]);
  if (params == nullptr || !params->items.empty())
    return fail("the param vector is not empty");

  std::puts("clojure: every example parses");
  return 0;
}
