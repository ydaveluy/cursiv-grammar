#include <filesystem>
#include <memory>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "sexpression/core/CoreServices.hpp"
#include "sexpression/core/ast.hpp"
#include "sexpression/generated/Sexpression/Module.hpp"

namespace {
namespace ast = sexpression::ast;
const cursiv::Parser &engine() {
  static const auto parser =
      sexpression::generated::Sexpression::create_parser();
  return *parser;
}
void examples() {
  std::size_t seen = 0;
  for (const auto &dir : {std::filesystem::path(CURSIV_EXAMPLE_DIR) / "example",
                          std::filesystem::path(CURSIV_TEST_DIR)}) {
    for (const auto &file : std::filesystem::directory_iterator(dir)) {
      if (!file.is_regular_file() || file.path().extension() != ".sexpression")
        continue;
      ++seen;
      const std::string source = cursiv::test::read_file(file.path().string());
      const auto result = engine().parse(source);
      CHECK(result.accepted && !result.cst().recovered());
      CHECK(result.value_as<ast::Sexpr>() != nullptr);
    }
  }
  CHECK(seen >= 3);
}
void datums() {
  for (const char* newline : {"\n", "\r", "\r\n", "\u0085", "\u2028", "\u2029"}) {
    const auto result = engine().parse(std::string("a;comment") + newline + "b");
    const auto* root = result.value_as<ast::Sexpr>();
    CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    CHECK(root && root->items.size() == 2);
    if (root && root->items.size() == 2) {
      const auto* second = cursiv::ast_cast<ast::Atom>(root->items[1]);
      CHECK(second && second->text == "b");
    }
  }
  {
    const std::string source =
        "#lang htdp/bsl\n[define café 1] {+ café .5} '[] '( )";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *root = result.value_as<ast::Sexpr>();
    CHECK(root && root->items.size() == 5);
    if (!root || root->items.size() != 5)
      return;
    CHECK(cursiv::ast_cast<ast::Directive>(root->items[0]) != nullptr);
    const auto *define = cursiv::ast_cast<ast::List>(root->items[1]);
    CHECK(define && define->opener == "[" && define->closer == "]" &&
          define->items.size() == 3);
    if (define && define->items.size() == 3) {
      const auto *name = cursiv::ast_cast<ast::Atom>(define->items[1]);
      CHECK(name && name->text == "café");
    }
    const auto *quote = cursiv::ast_cast<ast::Prefix>(root->items[3]);
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
    const auto *root = result.value_as<ast::Sexpr>();
    CHECK(root && root->items.size() == 15);
    if (root && root->items.size() == 15) {
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
        const auto *n = cursiv::ast_cast<ast::Atom>(root->items[i]);
        CHECK(n && n->text == expected[i]);
      }
      CHECK(cursiv::ast_cast<ast::Atom>(root->items[13]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Atom>(root->items[14]) != nullptr);
    }
  }
  {
    const std::string source =
        R"SRC(|a b| a\ b foo#bar "A\n\u0042\U0001F600\101Z" #\newline #\λ #\u03bb #\space #\) #t #false #:|a b| #"bytes" #rx"\\d+" #px#"a")SRC";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *root = result.value_as<ast::Sexpr>();
    CHECK(root && root->items.size() == 15);
    if (root && root->items.size() == 15) {
      const auto *name = cursiv::ast_cast<ast::Atom>(root->items[0]);
      CHECK(name && name->text == "|a b|");
      const auto *string = cursiv::ast_cast<ast::String>(root->items[3]);
      CHECK(string && string->text == R"("A\n\u0042\U0001F600\101Z")");
      for (std::size_t i = 4; i < 9; ++i)
        CHECK(cursiv::ast_cast<ast::Character>(root->items[i]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Boolean>(root->items[9]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Keyword>(root->items[11]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Bytes>(root->items[12]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Regex>(root->items[13]) != nullptr);
      CHECK(cursiv::ast_cast<ast::Regex>(root->items[14]) != nullptr);
    }
  }
  {
    const std::string source =
        "#| outer #| nested |# tail |# '#;#;1 2 (a b . c) #;[ignored] #3(1 2) "
        "#hash((a . 1)) #0=(a . #0#)";
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *root = result.value_as<ast::Sexpr>();
    CHECK(root && root->items.size() == 5);
    if (root && root->items.size() == 5) {
      const auto *quote = cursiv::ast_cast<ast::Prefix>(root->items[0]);
      CHECK(quote && quote->discards.size() == 1);
      const auto *list =
          quote ? cursiv::ast_cast<ast::List>(quote->value) : nullptr;
      CHECK(list && list->items.size() == 2 && list->tail != nullptr);
      if (quote && quote->discards.size() == 1)
        CHECK(quote->discards[0]->discards.size() == 1);
      const auto *vector = cursiv::ast_cast<ast::List>(root->items[2]);
      CHECK(vector && vector->opener == "#3(" && vector->items.size() == 2);
      const auto *graph = cursiv::ast_cast<ast::Prefix>(root->items[4]);
      CHECK(graph && graph->marker == "#0=");
    }
  }
  for (const std::string source : {"", "#lang htdp/bsl\n", "(and #true)",
                                   "(arbitrary-native-form [x] {y})"}) {
    const auto result = engine().parse(source);
    CHECK(result.accepted && !result.cst().recovered());
    CHECK(result.value_as<ast::Sexpr>() != nullptr);
  }
}
void here_string() {
  const std::string source = "#<<END\nbody END\nλ\nEND\n42";
  const auto result = engine().parse(source);
  CHECK(result.accepted && !result.cst().recovered());
  const auto *root = result.value_as<ast::Sexpr>();
  CHECK(root && root->items.size() == 2);
  const auto *string = root && !root->items.empty()
                           ? cursiv::ast_cast<ast::String>(root->items[0])
                           : nullptr;
  CHECK(string && string->text == "#<<END\nbody END\nλ\nEND");
}
void numeric_kinds() {
  const std::string source = "inf.0 nan.0 #i+inf.0 #i-nan.0 #CI(AND #T #F)";
  const auto result = engine().parse(source);
  CHECK(result.accepted && !result.cst().recovered());
  const auto *root = result.value_as<ast::Sexpr>();
  CHECK(root && root->items.size() == 5);
  if (!root || root->items.size() != 5)
    return;
  const auto *symbol = cursiv::ast_cast<ast::Atom>(root->items[0]);
  CHECK(symbol && symbol->text == "inf.0");
  CHECK(cursiv::ast_cast<ast::Atom>(root->items[1]) != nullptr);
  const auto *number = cursiv::ast_cast<ast::Atom>(root->items[2]);
  CHECK(number && number->text == "#i+inf.0");
  number = cursiv::ast_cast<ast::Atom>(root->items[3]);
  CHECK(number && number->text == "#i-nan.0");
  const auto *prefix = cursiv::ast_cast<ast::Prefix>(root->items[4]);
  CHECK(prefix && prefix->marker == "#CI");
}
void additional_atoms() {
  const std::string source =
      "+INF.0 -NaN.0 1# #xA# 1.2t3 #%app #HASHEQ((a . 1)) #Fx3(1) 1+inf.0i";
  const auto result = engine().parse(source);
  CHECK(result.accepted && !result.cst().recovered());
  const auto *root = result.value_as<ast::Sexpr>();
  CHECK(root && root->items.size() == 9);
  if (!root || root->items.size() != 9)
    return;
  for (std::size_t i = 0; i < 5; ++i)
    CHECK(cursiv::ast_cast<ast::Atom>(root->items[i]) != nullptr);
  CHECK(cursiv::ast_cast<ast::Atom>(root->items[5]) != nullptr);
  const auto *hash = cursiv::ast_cast<ast::List>(root->items[6]);
  CHECK(hash && hash->opener == "#HASHEQ(");
  const auto *fx = cursiv::ast_cast<ast::List>(root->items[7]);
  CHECK(fx && fx->opener == "#Fx3(");
  const auto *complex = cursiv::ast_cast<ast::Atom>(root->items[8]);
  CHECK(complex && complex->text == "1+inf.0i");
}
void infix_list() {
  const std::string source = "(1 . + . 2 3)";
  const auto result = engine().parse(source);
  CHECK(result.accepted && !result.cst().recovered());
  const auto *root = result.value_as<ast::Sexpr>();
  const auto *list = root && root->items.size() == 1
                         ? cursiv::ast_cast<ast::List>(root->items[0])
                         : nullptr;
  CHECK(list && list->infix && list->items.size() == 1 &&
        list->suffix.size() == 2);
  const auto *head = list ? cursiv::ast_cast<ast::Atom>(list->tail) : nullptr;
  CHECK(head && head->text == "+");
}
void reader_constraints() {
  auto language = cursiv::create_language<sexpression::SexpressionCoreServices,
                                         sexpression::SexpressionSharedCoreServices,
                                         sexpression::install_core_module>();
  for (const auto* source : {"(a]", "[a)", "{a]", "(. a)",
                              "(#;ignored . a)", "(a . + .)",
                              "#(a . b)", "#s(a . b)", "#0(1)", "#01(1 2)"}) {
    const auto parsed = language.language.parser->parse(source);
    CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
    const auto validation = cursiv::test::validationHelper(language.language, source);
    CHECK(validation.diagnostics.size() == 1);
    if (validation.diagnostics.size() == 1)
      CHECK(validation.diagnostics[0].severity == cursiv::DiagnosticSeverity::Error);
  }
  for (const auto* source : {"[a {b}]", "(a . b)", "(a . + . b)",
                              "#(a b)", "#s(a . + . b)", "#hash((a . b))",
                              "#0()", "#02(1)", "#fl2(1.0)", "#fx2(1 2)"}) {
    const auto validation = cursiv::test::validationHelper(language.language, source);
    CHECK(validation.diagnostics.empty());
  }
  for (const auto* source : {"#t|name|", "#false\\name", "#<<\nbody\n\n"}) {
    const auto parsed = engine().parse(source);
    CHECK(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty());
  }
}
} // namespace
int main() {
  examples();
  datums();
  numeric_kinds();
  additional_atoms();
  infix_list();
  here_string();
  reader_constraints();
  return cursiv::test::report("sexpression_grammar_test");
}
