#include <filesystem>
#include <memory>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "lisp/core/ast.hpp"
#include "lisp/generated/Lisp/Module.hpp"

namespace {

namespace fs = std::filesystem;

const cursiv::Parser &engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      lisp::generated::Lisp::create_parser();
  return *p;
}

void examples() {
  int seen = 0;
  for (const fs::directory_entry &entry :
       fs::directory_iterator(fs::path(CURSIV_TEST_DIR))) {
    if (!entry.is_regular_file() || entry.path().extension() != ".lisp")
      continue;
    ++seen;
    const std::string text = cursiv::test::read_file(entry.path().string());
    const cursiv::ParseResult r = engine().parse(text);
    CHECK(r.accepted && !r.cst().recovered());
    CHECK(r.value_as<lisp::ast::Doc>() != nullptr);
  }
  CHECK(seen > 0);
}

void reader_values() {
  namespace ast = lisp::ast;
  {
    const std::string text = "() (A0) (a b . c) 'foo `(a ,b ,@c) #(1 a) #3(1)";
    const auto result = engine().parse(text);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *doc = result.value_as<ast::Doc>();
    CHECK(doc && doc->expressions.size() == 7);
    if (doc && doc->expressions.size() == 7) {
      const auto *empty = cursiv::ast_cast<ast::List>(doc->expressions[0]);
      CHECK(empty && empty->items.empty() && !empty->tail);
      const auto *dotted = cursiv::ast_cast<ast::List>(doc->expressions[2]);
      CHECK(dotted && dotted->items.size() == 2 && dotted->tail);
      const auto *tail =
          dotted ? cursiv::ast_cast<ast::Symbol>(dotted->tail) : nullptr;
      CHECK(tail && tail->text == "c");
      const auto *quote = cursiv::ast_cast<ast::Prefix>(doc->expressions[3]);
      CHECK(quote && quote->marker == "'");
      const auto *vector = cursiv::ast_cast<ast::List>(doc->expressions[6]);
      CHECK(vector && vector->opener == "#3(" && vector->items.size() == 1);
    }
  }
  {
    const std::string text =
        R"SRC(42 9007199254740993 5/9 1d3 #xFF #16rFF #C(1 2) "A\n" #\λ #*1010 #:foo |a b| a\ b)SRC";
    const auto result = engine().parse(text);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *doc = result.value_as<ast::Doc>();
    CHECK(doc && doc->expressions.size() == 13);
    if (doc && doc->expressions.size() == 13) {
      const char *expected[] = {
          "42", "9007199254740993", "5/9", "1d3", "#xFF", "#16rFF"};
      for (std::size_t i = 0; i < 6; ++i) {
        const auto *number = cursiv::ast_cast<ast::Number>(doc->expressions[i]);
        CHECK(number && number->text == expected[i]);
      }
      const auto *complex = cursiv::ast_cast<ast::Prefix>(doc->expressions[6]);
      CHECK(complex && complex->marker == "#C");
      const auto *string = cursiv::ast_cast<ast::String>(doc->expressions[7]);
      CHECK(string && string->text == R"("A\n")");
      const auto *character =
          cursiv::ast_cast<ast::Character>(doc->expressions[8]);
      CHECK(character && character->text == R"(#\λ)");
      const auto *bits = cursiv::ast_cast<ast::BitVector>(doc->expressions[9]);
      CHECK(bits && bits->text == "#*1010");
      const auto *symbol = cursiv::ast_cast<ast::Symbol>(doc->expressions[10]);
      CHECK(symbol && symbol->text == "#:foo");
    }
  }
  {
    const std::string text =
        "#| a #| b |# c |# #+sbcl (yes) #-sbcl (no) #1=(a . #1#) ; final";
    const auto result = engine().parse(text);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *doc = result.value_as<ast::Doc>();
    CHECK(doc && doc->expressions.size() == 3);
    if (doc && doc->expressions.size() == 3) {
      const auto *conditional =
          cursiv::ast_cast<ast::Conditional>(doc->expressions[0]);
      CHECK(conditional && conditional->marker == "#+" &&
            conditional->feature && conditional->value);
      const auto *graph = cursiv::ast_cast<ast::Prefix>(doc->expressions[2]);
      const auto *list =
          graph ? cursiv::ast_cast<ast::List>(graph->value) : nullptr;
      CHECK(graph && graph->marker == "#1=" && list);
      CHECK(list && cursiv::ast_cast<ast::GraphRef>(list->tail));
    }
  }
  for (const std::string text : {"", " ; comment"}) {
    const auto result = engine().parse(text);
    CHECK(result.accepted && !result.cst().recovered());
    const auto *doc = result.value_as<ast::Doc>();
    CHECK(doc && doc->expressions.empty());
  }
}
} // namespace

int main() {
  examples();
  reader_values();
  return cursiv::test::report("lisp_grammar_test");
}
