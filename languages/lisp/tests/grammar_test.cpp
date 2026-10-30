#include <filesystem>
#include <memory>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "lisp/core/ast.hpp"
#include "lisp/generated/Lisp/Module.hpp"
#include "lisp/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

namespace fs = std::filesystem;

const auto& services() {
  static const auto language = cursiv::create_language<
      lisp::LispCoreServices, lisp::LispSharedCoreServices, lisp::install_core_module>();
  return language.language;
}
const cursiv::Parser &engine() { return *services().parser; }

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
void native_reader_constraints() {
  struct Case { const char* text; const char* code; };
  for (const auto& item : {
           Case{"1/0", "lisp.number"}, {"#b102", "lisp.number"},
           {"#x1.2", "lisp.number"}, {"#1r0", "lisp.number"},
           {"#37r1", "lisp.number"}, {"#r10", "lisp.number"},
           {"#*012", "lisp.bit-vector"}, {"#2*101", "lisp.bit-vector"},
           {"#1*", "lisp.bit-vector"}, {"#2*", "lisp.bit-vector"},
           {"#(a . b)", "lisp.dotted-list"}, {"(. a)", "lisp.dotted-list"},
           {"#2(1 2 3)", "lisp.vector-length"}, {"#3()", "lisp.vector-length"},
           {"#A(1 2)", "lisp.array-rank"},
           {"#C(1)", "lisp.complex"}, {"#C(1 x)", "lisp.complex"},
           {",x", "lisp.comma"}, {"',x", "lisp.comma"}, {"`(,,x)", "lisp.comma"}}) {
    const auto parsed = engine().parse(item.text);
    CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
    bool found = false;
    const auto validated = cursiv::test::validationHelper(services(), item.text);
    for (const auto& diagnostic : validated.diagnostics)
      found |= diagnostic.code && *diagnostic.code == item.code;
    CHECK(found);
  }
  for (const auto* text : {"-5/9", "#b101 #o77 #xFF #16rFF #36rZ", "#2b1",
                           "#4*10", "#*", "#0*", "#0()", "#3(1)",
                           "#0A1", "#1A(1 2)", "#C(1 -2)", "`(,x ,@y)", "``(,,x)",
                           "#+unknown-reader-feature 1/0",
                           "#2(#+unknown-reader-feature 1 2 3)",
                           "#C(#+unknown-reader-feature 0 1 2)"})
    CHECK(cursiv::test::validationHelper(services(), text).diagnostics.empty());
}

} // namespace

int main() {
  examples();
  reader_values();
  native_reader_constraints();
  return cursiv::test::report("lisp_grammar_test");
}
