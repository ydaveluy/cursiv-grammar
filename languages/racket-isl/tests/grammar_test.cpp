#include <filesystem>
#include "reader_boundaries.hpp"
#include <memory>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "racketisl/core/CoreServices.hpp"
#include "racketisl/core/ast.hpp"
#include "racketisl/generated/RacketIsl/Module.hpp"

namespace {
namespace ast = racketisl::ast;
const cursiv::Parser &engine() {
  static const auto parser = racketisl::generated::RacketIsl::create_parser();
  return *parser;
}
void examples() {
  std::size_t seen = 0;
  for (const auto &dir : {std::filesystem::path(CURSIV_EXAMPLE_DIR) / "example",
                          std::filesystem::path(CURSIV_TEST_DIR)}) {
    for (const auto &file : std::filesystem::directory_iterator(dir)) {
      if (!file.is_regular_file() ||
          (file.path().extension() != ".rkt" &&
           file.path().extension() != ".racketisl"))
        continue;
      ++seen;
      const std::string source = cursiv::test::read_file(file.path().string());
      const auto result = engine().parse(source);
      CHECK(result.accepted && !result.cst().recovered());
      CHECK(result.value_as<ast::Program>() != nullptr);
    }
  }
  CHECK(seen >= 2);
}
void datums() {
  {
    const std::string source =
        "#lang htdp/isl\n[define café 1] {+ café .5} '[] '( )";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *root = result.value_as<ast::Program>();
    CHECK(root && root->forms.size() == 5);
    if (!root || root->forms.size() != 5)
      return;
    CHECK(cursiv::ast_cast<ast::Directive>(root->forms[0]) != nullptr);
    const auto *define = cursiv::ast_cast<ast::List>(root->forms[1]);
    CHECK(define && define->opener == "[" && define->closer == "]" &&
          define->items.size() == 3);
    if (define && define->items.size() == 3) {
      const auto *name = cursiv::ast_cast<ast::Atom>(define->items[1]);
      CHECK(name && name->text == "café");
    }
    const auto *quote = cursiv::ast_cast<ast::Prefix>(root->forms[3]);
    CHECK(quote && quote->marker == "'");
    const auto *empty =
        quote ? cursiv::ast_cast<ast::List>(quote->value) : nullptr;
    CHECK(empty && empty->items.empty() && empty->opener == "[");
  }
  {
    const std::string source =
        "#e1.25 #xFF .5 +inf.0 +nan.0 1+2i 9007199254740993 +i 1@2 #e#x10 "
        "#x#e10 5/9 1e-2 1+ 2x";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *root = result.value_as<ast::Program>();
    CHECK(root && root->forms.size() == 15);
    if (root && root->forms.size() == 15) {
      const char *expected[] = {"#e1.25",
                                "#xFF",
                                ".5",
                                "+inf.0",
                                "+nan.0",
                                "1+2i",
                                "9007199254740993",
                                "+i",
                                "1@2",
                                "#e#x10",
                                "#x#e10",
                                "5/9",
                                "1e-2"};
      for (std::size_t i = 0; i < 13; ++i) {
        const auto *n = cursiv::ast_cast<ast::Atom>(root->forms[i]);
        CHECK(n && n->text == expected[i]);
      }
      CHECK(cursiv::ast_cast<ast::Atom>(root->forms[13]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Atom>(root->forms[14]) != nullptr);
    }
  }
  {
    const std::string source =
        R"SRC(|a b| a\ b foo#bar "A\n\u0042\U0001F600\101Z" #\newline #\λ #\u03bb #\space #\) #t #false #:|a b| #"bytes" #rx"\\d+" #px#"a")SRC";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *root = result.value_as<ast::Program>();
    CHECK(root && root->forms.size() == 15);
    if (root && root->forms.size() == 15) {
      const auto *name = cursiv::ast_cast<ast::Atom>(root->forms[0]);
      CHECK(name && name->text == "|a b|");
      const auto *string = cursiv::ast_cast<ast::String>(root->forms[3]);
      CHECK(string && string->text == R"("A\n\u0042\U0001F600\101Z")");
      for (std::size_t i = 4; i < 9; ++i)
        CHECK(cursiv::ast_cast<ast::Character>(root->forms[i]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Boolean>(root->forms[9]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Keyword>(root->forms[11]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Bytes>(root->forms[12]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Regex>(root->forms[13]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Regex>(root->forms[14]) != nullptr);
    }
  }
  {
    const std::string source =
        "#| outer #| nested |# tail |# '#;#;1 2 (a b . c) #;[ignored] #3(1 2) "
        "#hash((a . 1)) #0=(a . #0#)";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *root = result.value_as<ast::Program>();
    CHECK(root && root->forms.size() == 5);
    if (root && root->forms.size() == 5) {
      const auto *quote = cursiv::ast_cast<ast::Prefix>(root->forms[0]);
      CHECK(quote && quote->discards.size() == 1);
      const auto *list =
          quote ? cursiv::ast_cast<ast::List>(quote->value) : nullptr;
      CHECK(list && list->items.size() == 2 && list->tail != nullptr);
      if (quote && quote->discards.size() == 1)
        CHECK(quote->discards[0]->discards.size() == 1);
      const auto *vector = cursiv::ast_cast<ast::List>(root->forms[2]);
      CHECK(vector && vector->opener == "#3(" && vector->items.size() == 2);
      const auto *graph = cursiv::ast_cast<ast::Prefix>(root->forms[4]);
      CHECK(graph && graph->marker == "#0=");
    }
  }
  for (const std::string source : {"", "#lang htdp/isl\n", "(and #true)",
                                   "(arbitrary-native-form [x] {y})"}) {
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    CHECK(result.value_as<ast::Program>() != nullptr);
  }
}
void here_string() {
  const std::string source = "#<<END\nbody END\nλ\nEND\n42";
  const auto result = engine().parse(source);
  CHECK(result.accepted && !result.cst().recovered());
  const auto *root = result.value_as<ast::Program>();
  CHECK(root && root->forms.size() == 2);
  const auto *string = root && !root->forms.empty()
                           ? cursiv::ast_cast<ast::String>(root->forms[0])
                           : nullptr;
  CHECK(string && string->text == "#<<END\nbody END\nλ\nEND");
}
void validation() {
  auto language =
      cursiv::create_language<racketisl::RacketIslCoreServices,
                              racketisl::RacketIslSharedCoreServices,
                              racketisl::install_core_module>();
  for (const std::string source :
       {"(and #true)", "(or)", "'(and)", "(quote (and))", "(and #true #false)",
        "#&(and)"}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:reader", "racketisl", 0, source));
    document.parseResult = engine().parse(source);
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered());
    int errors = 0;
    auto sink = [&](cursiv::Diagnostic &&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, document, source);
    const auto *program = document.parseResult.value_as<ast::Program>();
    if (program)
      language.language.validator->checkProgram(*program, accept);
    CHECK(errors == ((source == "(and #true)" || source == "(or)") ? 1 : 0));
  }
}
void numeric_kinds() {
  const std::string source = "inf.0 nan.0 #i+inf.0 #i-nan.0 #CI(AND #T #F)";
  const auto result = engine().parse(source);
  CHECK(result.accepted && !result.cst().recovered());
  const auto *root = result.value_as<ast::Program>();
  CHECK(root && root->forms.size() == 5);
  if (!root || root->forms.size() != 5)
    return;
  const auto *symbol = cursiv::ast_cast<ast::Atom>(root->forms[0]);
  CHECK(symbol && symbol->text == "inf.0");
  CHECK(cursiv::ast_cast<ast::Atom>(root->forms[1]) != nullptr);
  const auto *number = cursiv::ast_cast<ast::Atom>(root->forms[2]);
  CHECK(number && number->text == "#i+inf.0");
  number = cursiv::ast_cast<ast::Atom>(root->forms[3]);
  CHECK(number && number->text == "#i-nan.0");
  const auto *prefix = cursiv::ast_cast<ast::Prefix>(root->forms[4]);
  CHECK(prefix && prefix->marker == "#CI");
}
void additional_atoms() {
  const std::string source =
      "+INF.0 -NaN.0 1# #xA# 1.2t3 #%app #HASHEQ((a . 1)) #Fx3(1) 1+inf.0i";
  const auto result = engine().parse(source);
  CHECK(result.accepted && !result.cst().recovered());
  const auto *root = result.value_as<ast::Program>();
  CHECK(root && root->forms.size() == 9);
  if (!root || root->forms.size() != 9)
    return;
  for (std::size_t i = 0; i < 5; ++i)
    CHECK(cursiv::ast_cast<ast::Atom>(root->forms[i]) != nullptr);
  CHECK(cursiv::ast_cast<ast::Atom>(root->forms[5]) != nullptr);
  const auto *hash = cursiv::ast_cast<ast::List>(root->forms[6]);
  CHECK(hash && hash->opener == "#HASHEQ(");
  const auto *fx = cursiv::ast_cast<ast::List>(root->forms[7]);
  CHECK(fx && fx->opener == "#Fx3(");
  const auto *complex = cursiv::ast_cast<ast::Atom>(root->forms[8]);
  CHECK(complex && complex->text == "1+inf.0i");
}
void character_boundaries() {
  const std::string source =
      "#\\unknown #\\uabcdx #\\U0001F6001 #\\1234 #\\a1 #\\SPACE1";
  const auto result = engine().parse(source);
  CHECK(result.accepted && !result.cst().recovered());
  const auto *root = result.value_as<ast::Program>();
  CHECK(root && root->forms.size() == 12);
  if (!root || root->forms.size() != 12)
    return;
  const char *characters[] = {"#\\u",   "#\\uabcd", "#\\U0001F600",
                              "#\\123", "#\\a",     "#\\SPACE"};
  const char *atoms[] = {"nknown", "x", "1", "4", "1", "1"};
  for (std::size_t i = 0; i < 6; ++i) {
    const auto *character =
        cursiv::ast_cast<ast::Character>(root->forms[2 * i]);
    const auto *atom = cursiv::ast_cast<ast::Atom>(root->forms[2 * i + 1]);
    CHECK(character && character->text == characters[i] &&
          character->parent == root);
    CHECK(atom && atom->text == atoms[i] && atom->parent == root);
  }
}
void native_reader_rejections() {
  auto language = cursiv::create_language<racketisl::RacketIslCoreServices,
      racketisl::RacketIslSharedCoreServices, racketisl::install_core_module>();
  {
    const auto parsed = engine().parse("#lang htdp/isl (define x 1)");
    const auto *program = parsed.value_as<ast::Program>();
    CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
          program && program->forms.size() == 2);
    if (program && program->forms.size() == 2) {
      const auto *header = cursiv::ast_cast<ast::Directive>(program->forms[0]);
      const auto *form = cursiv::ast_cast<ast::List>(program->forms[1]);
      CHECK(header && header->text == "#lang htdp/isl");
      CHECK(form && form->items.size() == 3);
    }
  }
  for (const auto *source : {"#lang htdp/isl(define x 1)", "#lang  htdp/isl", "#!x\n1", "#!\tbin\n1"}) {
    const auto parsed = engine().parse(source);
    CHECK(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty());
  }
  for (const auto *source : {"\"\\q\"", "\"\\x\"", "\"\\uD800\"", "\"\\U00110000\"",
       "#\"\\u0041\"", "#\"λ\"", "#xG", "#e", "#x", "#iabc", "#d1e", "#e+INF.0",
       "1/0", "#e1/0", "#fx(1)", "#fl(1.0)", "#hash((a 1))", "(a . b)", "#0#", "#0=(a)"})
    CHECK(!cursiv::test::validationHelper(language.language, source).diagnostics.empty());
  for (const auto *source : {"\"\\x41\"", "\"\\u1\"", "\"\\uD83D\\uDE00\"", "#\"é\"", "#\"\\377\"",
       "#e#x10", "#xA#", "#hash((a . 1))", "#2(1)"})
    CHECK(cursiv::test::validationHelper(language.language, source).diagnostics.empty());
}
void line_comment_boundaries() {
  for (const std::string_view ending : {"\n", "\r", "\r\n", "\xc2\x85", "\xe2\x80\xa8", "\xe2\x80\xa9"}) {
    const std::string source = "a ; hidden" + std::string(ending) + "b";
    const auto result = engine().parse(source);
    const auto *root = result.value_as<ast::Program>();
    const bool endsComment = ending == "\n" || ending == "\r\n";
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty() && root &&
          root->forms.size() == (endsComment ? 2 : 1));
    if (root && endsComment && root->forms.size() == 2) {
      const auto *last = cursiv::ast_cast<ast::Atom>(root->forms[1]);
      CHECK(last && last->text == "b" && last->parent == root);
    }
  }
}
} // namespace
int main() {
  {
    auto language = cursiv::create_language<racketisl::RacketIslCoreServices,
        racketisl::RacketIslSharedCoreServices, racketisl::install_core_module>();
    racketisl_reader_boundaries::run(language.language);
  }

  character_boundaries();
  native_reader_rejections();
  line_comment_boundaries();
  examples();
  datums();
  numeric_kinds();
  additional_atoms();
  validation();
  here_string();
  return cursiv::test::report("racketisl_grammar_test");
}
