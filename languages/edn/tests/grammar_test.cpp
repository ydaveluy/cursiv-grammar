// The grammar end to end: every corpus file under example/ parses clean, a
// handful of accept/refuse probes pin the token boundaries, and one small,
// construct-rich document is walked down to its scalars.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "edn/core/CoreServices.hpp"
#include "edn/core/ast.hpp"

namespace {

namespace ast = edn::ast;
namespace fs = std::filesystem;

int failures = 0;

void check(bool ok, const char *what) {
  if (ok)
    return;
  std::fprintf(stderr, "edn: FAIL %s\n", what);
  ++failures;
}

std::string read_file(const fs::path &path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

bool clean(const cursiv::ParseResult &r) {
  return r.accepted && !r.cst().recovered();
}

template <class T> T *as(ast::Value *v) {
  return v != nullptr && v->type == &cursiv::ast_type_tag<T>
             ? static_cast<T *>(v)
             : nullptr;
}
template <class T> T *as(const ast::Val &v) {
  auto *const *n = std::get_if<ast::Value *>(&v);
  return n != nullptr ? as<T>(*n) : nullptr;
}

} // namespace

void reader_values(const cursiv::Parser &parser) {
  const std::string scalars = R"edn([:nil :true :false :123 :foo/bar
    1.0M 1e3M 1. -2.5E-2M "A\n\u0042\uD83D\uDE00\101Z"] )edn";
  const auto scalar_result = parser.parse(scalars);
  check(clean(scalar_result) && scalar_result.diagnostics.empty(),
        "reader scalars parse clean");
  const auto *scalar_doc = scalar_result.value_as<ast::Program>();
  auto *values = scalar_doc && scalar_doc->values.size() == 1
                     ? as<ast::Vector>(scalar_doc->values[0])
                     : nullptr;
  check(values && values->items.size() == 10, "reader scalar count");
  if (values && values->items.size() == 10) {
    const char *names[] = {"nil", "true", "false", "123", "foo/bar"};
    for (std::size_t i = 0; i < std::size(names); ++i) {
      const auto *keyword = as<ast::Keyword>(values->items[i]);
      check(keyword && keyword->name == names[i],
            "keyword names retain literal spellings");
    }
    const char *numbers[] = {"1.0M", "1e3M", "1.", "-2.5E-2M"};
    for (std::size_t i = 0; i < std::size(numbers); ++i) {
      const auto *number = as<ast::Number>(values->items[i + 5]);
      check(number && number->text == numbers[i],
            "bigdecimal is a single number");
    }
    const auto *text = std::get_if<std::string_view>(&values->items[9]);
    check(text && *text == "A\nB😀AZ", "string escapes are decoded");
  }
  const std::string source = "{#_ :skip :k #_ 1 2 #_ :tail}";
  const auto parsed = parser.parse(source);
  const auto *doc = parsed.value_as<ast::Program>();
  const auto *map =
      doc && doc->values.size() == 1 ? as<ast::Map>(doc->values[0]) : nullptr;
  check(clean(parsed) && parsed.diagnostics.empty() && map &&
            map->entries.size() == 1 && map->discards.size() == 2,
        "discarded forms do not shift map pairs");
  if (map && map->entries.size() == 1) {
    const auto *key = as<ast::Keyword>(map->entries[0]->key);
    const auto *value = as<ast::Number>(map->entries[0]->value);
    check(key && key->name == "k" && value && value->text == "2" &&
              map->entries[0]->discards.size() == 1,
          "map keeps the real key/value and discarded source");
  }
  const std::string nested = "#_ #_ 1 2 3";
  const auto nested_result = parser.parse(nested);
  const auto *nested_doc = nested_result.value_as<ast::Program>();
  check(clean(nested_result) && nested_doc && nested_doc->values.size() == 2,
        "nested discard consumes the next non-discarded form");
  if (nested_doc && nested_doc->values.size() == 2) {
    const auto *discard = as<ast::Discard>(nested_doc->values[0]);
    const auto *consumed = discard ? as<ast::Number>(discard->value) : nullptr;
    const auto *kept = as<ast::Number>(nested_doc->values[1]);
    check(discard && discard->discards.size() == 1 && consumed &&
              consumed->text == "2" && kept && kept->text == "3",
          "nested discard keeps both discarded sources and surviving value");
  }
  const std::string namespace_source = "#:foo{:a 1 :_/b 2}";
  const auto namespaced = parser.parse(namespace_source);
  const auto *namespace_doc = namespaced.value_as<ast::Program>();
  const auto *namespace_map = namespace_doc && namespace_doc->values.size() == 1
                                  ? as<ast::Map>(namespace_doc->values[0])
                                  : nullptr;
  check(clean(namespaced) && namespace_map &&
            namespace_map->namespace_ == "foo" &&
            namespace_map->entries.size() == 2,
        "namespaced map retains its prefix");
}

void native_reader_constraints(const edn::EdnCoreServices& services) {
  const auto& parser = *services.parser;
  for (const char* source : {"1abc", "1+2", "1.0Mfoo", "\\newlineX", "# foo 1", R"("\c")"}) {
    const auto result = parser.parse(source);
    check(!clean(result) || !result.diagnostics.empty(), "reader rejects invalid token boundary or escape");
  }
  for (const char* source : {"foo:", "::foo", ":foo::bar", R"("\400")"}) {
    const auto result = parser.parse(source);
    check(!result.diagnostics.empty(), "reader reports invalid token or octal value");
  }
  {
    const auto result = parser.parse(R"("\377")");
    const auto* document = result.value_as<ast::Program>();
    const auto* value = document && document->values.size() == 1
        ? std::get_if<std::string_view>(&document->values[0]) : nullptr;
    check(clean(result) && result.diagnostics.empty() && value && *value == "ÿ",
          "octal character U+00FF is encoded as UTF-8");
  }
  for (const auto& [source, code] : {
      std::pair{"{:a 1 :a 2}", "edn.duplicate-key"},
      std::pair{"#:foo{:a 1 :foo/a 2}", "edn.duplicate-key"},
      std::pair{"#{:a #_ :discard :a}", "edn.duplicate-element"},
      std::pair{"\\o400", "edn.character"},
      std::pair{"\\uD800", "edn.character"}}) {
    cursiv::Document document(cursiv::TextDocument::create("memory:edn", "edn", 0, source));
    document.language = &services;
    document.parseResult = parser.parse(source);
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    check(clean(document.parseResult) && document.parseResult.diagnostics.empty(),
          "reader constraint is checked after structural parsing");
    check(diagnostics.size() == 1 && diagnostics[0].code &&
              diagnostics[0].code->isString() && diagnostics[0].code->string() == code,
          "native reader constraint produces its diagnostic");
  }
}

int main() {
  const auto language =
      cursiv::create_language<edn::EdnCoreServices, edn::EdnSharedCoreServices,
                              edn::install_core_module>();
  const cursiv::Parser &parser = *language.language.parser;
  reader_values(parser);
  native_reader_constraints(language.language);

  const fs::path exampleDir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  bool sawAny = false;
  for (const fs::path &dir : {exampleDir, fs::path(CURSIV_TEST_DIR)})
    for (const fs::directory_entry &entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".edn")
        continue;
      sawAny = true;
      const std::string text = read_file(entry.path());
      const cursiv::ParseResult r = parser.parse(text);
      if (!clean(r) || !r.diagnostics.empty()) {
        std::fprintf(stderr, "edn: FAIL %s does not parse clean\n",
                     entry.path().filename().string().c_str());
        ++failures;
        continue;
      }
      check(r.value != nullptr &&
                r.value->type == &cursiv::ast_type_tag<ast::Program>,
            "root is a Program");
    }
  check(sawAny, "example/ holds at least one .edn file");

  // Token-boundary probes.
  check(clean(parser.parse("nil true false 1 1.0 :kw sym")),
        "top-level values");
  {
    // `nil` is a whole word: `nilfoo` is one symbol, not the keyword nil
    // followed by a dangling `foo`.
    const cursiv::ParseResult r = parser.parse("nilfoo");
    check(clean(r), "nilfoo parses");
    ast::Program *prog =
        r.value != nullptr &&
                r.value->type == &cursiv::ast_type_tag<ast::Program>
            ? static_cast<ast::Program *>(r.value)
            : nullptr;
    ast::Symbol *sym = prog != nullptr && prog->values.size() == 1
                           ? as<ast::Symbol>(prog->values[0])
                           : nullptr;
    check(sym != nullptr && sym->name == "nilfoo", "nilfoo is one symbol");
  }
  check(!clean(parser.parse("[1 2")), "an unterminated vector recovers");
  check(clean(parser.parse("nil ; a comment, then a newline\n")),
        "a comment ends in a newline");
  check(clean(parser.parse("[1, 2, 3]")), "a comma is whitespace");
  check(clean(parser.parse("{:a 1 :b 2}")), "a map pair has no separator");
  check(clean(parser.parse("+5 -3 +foo -bar .5")),
        "a sign before a digit is a number, before a letter a symbol");
  {
    // `+5` ties a Number and a Symbol reading at the same length; this
    // port breaks the tie toward the number, the terminal declared first.
    const cursiv::ParseResult r = parser.parse("+5");
    ast::Program *prog =
        r.value != nullptr &&
                r.value->type == &cursiv::ast_type_tag<ast::Program>
            ? static_cast<ast::Program *>(r.value)
            : nullptr;
    check(prog != nullptr && prog->values.size() == 1 &&
              as<ast::Number>(prog->values[0]) != nullptr,
          "+5 reads as a number, not a symbol");
  }

  // A small, construct-rich document, walked to its scalars.
  {
    const cursiv::ParseResult r = parser.parse(
        "[:keyword :keyword/with-namespace symbol symbol/with-namespace "
        "nil true false \"string literal\" 1 1N 1.0 5M \\A "
        "#tagged \"value\" #tagged/with-namespace \"value\" "
        "#{\"some\" \"set\"} {\"some\" \"map\"} (\"a\" \"list\" \"of\" "
        "\"things\")]");
    check(clean(r) && r.diagnostics.empty(), "the rich document parses clean");
    ast::Program *prog =
        r.value != nullptr &&
                r.value->type == &cursiv::ast_type_tag<ast::Program>
            ? static_cast<ast::Program *>(r.value)
            : nullptr;
    check(prog != nullptr && prog->values.size() == 1, "one top-level vector");
    ast::Vector *vec = prog != nullptr && !prog->values.empty()
                           ? as<ast::Vector>(prog->values[0])
                           : nullptr;
    // Each tag and its value form a single element.
    check(vec != nullptr && vec->items.size() == 18, "eighteen elements");
    if (vec != nullptr && vec->items.size() == 18) {
      ast::Keyword *kw = as<ast::Keyword>(vec->items[0]);
      check(kw != nullptr, "a plain keyword");
      if (kw != nullptr) {
        check(kw->name == "keyword", "its name");
      }
      ast::Keyword *nskw = as<ast::Keyword>(vec->items[1]);
      if (nskw != nullptr) {
        check(nskw->name == "keyword/with-namespace",
              "a namespaced keyword keeps the slash");
      }
      ast::Symbol *sym = as<ast::Symbol>(vec->items[2]);
      check(sym != nullptr && sym->name == "symbol", "a plain symbol");
      check(std::holds_alternative<std::monostate>(vec->items[4]), "nil");
      check(std::holds_alternative<bool>(vec->items[5]) &&
                std::get<bool>(vec->items[5]),
            "true");
      check(std::holds_alternative<bool>(vec->items[6]) &&
                !std::get<bool>(vec->items[6]),
            "false");
      check(std::holds_alternative<std::string_view>(vec->items[7]) &&
                std::get<std::string_view>(vec->items[7]) == "string literal",
            "a string, quotes stripped");
      ast::Number *intLit = as<ast::Number>(vec->items[8]);
      check(intLit != nullptr && intLit->text == "1", "an integer");
      ast::Number *bigint = as<ast::Number>(vec->items[9]);
      check(bigint != nullptr && bigint->text == "1N",
            "an arbitrary-precision integer keeps its N");
      ast::Number *floatLit = as<ast::Number>(vec->items[10]);
      check(floatLit != nullptr && floatLit->text == "1.0", "a float");
      ast::Number *bigdec = as<ast::Number>(vec->items[11]);
      check(bigdec != nullptr && bigdec->text == "5M",
            "a bigdecimal keeps its M");
      ast::Character *ch = as<ast::Character>(vec->items[12]);
      check(ch != nullptr && ch->text == "A", "a character literal");
      ast::Tag *tag = as<ast::Tag>(vec->items[13]);
      check(tag != nullptr && tag->name == "tagged", "a tag");
      check(tag && std::holds_alternative<std::string_view>(tag->value) &&
                std::get<std::string_view>(tag->value) == "value",
            "a tag owns its payload");
      ast::Tag *nstag = as<ast::Tag>(vec->items[14]);
      check(nstag != nullptr && nstag->name == "tagged/with-namespace",
            "a namespaced tag");
      ast::Set *set = as<ast::Set>(vec->items[15]);
      check(set != nullptr && set->items.size() == 2, "a set literal");
      ast::Map *map = as<ast::Map>(vec->items[16]);
      check(map != nullptr && map->entries.size() == 1, "a map literal");
      ast::List *list = as<ast::List>(vec->items[17]);
      check(list != nullptr && list->items.size() == 4, "a list literal");
    }
  }

  if (failures != 0) {
    std::fprintf(stderr, "edn_grammar_test: %d failure(s)\n", failures);
    return 1;
  }
  std::puts("edn_grammar_test: all checks passed");
  return 0;
}
