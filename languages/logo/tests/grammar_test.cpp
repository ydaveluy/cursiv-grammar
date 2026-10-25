// The grammar end to end: every corpus document under example/ parses (or
// fails exactly where its own name declares it will), and a few field
// values are read back out of the tree -- a converter that silently yields
// the wrong value passes every structural check alone.

#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/Check.hpp"
#include "logo/core/CoreServices.hpp"
#include "logo/core/ast.hpp"

namespace {

namespace ast = logo::ast;

std::string read_file(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return {};
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<const T*>(n)
             : nullptr;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      logo::LogoCoreServices, logo::LogoSharedCoreServices,
      logo::install_core_module>();

  const std::string dir = CURSIV_EXAMPLE_DIR;

  // Every corpus document; `true` marks the one the corpus itself declares
  // as failing (a non-empty `.errors` sibling).
  const std::vector<std::pair<std::string, bool>> corpus = {
      {"example1.logo", false},
      {"example2.logo", false},
      {"example3.logo", false},
      {"example4.logo", false},
      {"expression.logo", false},
      {"flower.logo", false},
      {"fractal.logo", false},
      {"make.logo", false},
      {"octagon.logo", false},
      {"procedure1.logo", false},
      {"procedure2.logo", false},
      {"random.logo", false},
      {"recursive1.logo", false},
      {"repeat.logo", false},
      {"spiral.logo", false},
      {"spiral2.logo", false},
      {"tree.logo", false},
      {"tree2.logo", false},
      {"logo_fail_1.logo", false},
      {"logo_fail_2.logo", true},
      {"logo_feature_butfail.logo", false},
      // The corpus's own `.errors` sibling calls this one a token
      // recognition error on two raw replacement-character bytes; they are
      // well-formed UTF-8 (U+FFFD), accepted as raw bytes by QuotedString.
      // This document therefore parses clean.
      {"logo_feature_butfail_2.logo", false},
  };

  std::vector<std::string> texts;
  texts.reserve(corpus.size());
  for (const auto& [name, wants_diagnostic] : corpus) {
    texts.push_back(read_file(dir + "/" + name));
    const std::string& text = texts.back();
    CHECK(!text.empty());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    CHECK(result.diagnostics.empty() != wants_diagnostic);
    if (!wants_diagnostic) {
      CHECK(!result.cst().recovered());
      CHECK(as<ast::Program>(result.value) != nullptr);
    }
  }

  // example3.logo: `make "first_programmer "Ada_Lovelace` then
  // `print :first_programmer` -- the leading `"` is stripped from both
  // STRINGLITERAL reads, and the deref carries the plain name.
  {
    const std::string text = read_file(dir + "/example3.logo");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& program = *as<ast::Program>(result.value);
    CHECK(program.lines.size() == 2);

    const auto& line0 = *as<ast::CommandLine>(program.lines[0]);
    CHECK(line0.commands.size() == 1);
    const auto& make = *as<ast::MakeCmd>(line0.commands[0]);
    CHECK(make.name == "first_programmer");
    CHECK(std::holds_alternative<std::string_view>(make.value));
    CHECK(std::get<std::string_view>(make.value) == "Ada_Lovelace");

    const auto& line1 = *as<ast::PrintLine>(program.lines[1]);
    CHECK(std::holds_alternative<ast::Expression*>(line1.value));
    const auto& deref = *as<ast::Deref>(std::get<ast::Expression*>(line1.value));
    CHECK(deref.name == "first_programmer");
  }

  // expression.logo: `print 2*3` -- the infix table and the operator
  // converters both read back the right shape and the right constant.
  {
    const std::string text = read_file(dir + "/expression.logo");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& program = *as<ast::Program>(result.value);
    const auto& line1 = *as<ast::PrintLine>(program.lines[1]);
    const auto& bin =
        *as<ast::BinaryExpression>(std::get<ast::Expression*>(line1.value));
    CHECK(bin.op == ast::Op::multiply);
    CHECK(as<ast::NumberLiteral>(bin.left)->value == 2);
    CHECK(as<ast::NumberLiteral>(bin.right)->value == 3);
  }

  return cursiv::test::report("logo_grammar_test");
}
