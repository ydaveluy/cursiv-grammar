// The smoke test of the language: build the container the way a workspace
// does, read the sample, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "dice/core/CoreServices.hpp"
#include "dice/core/ast.hpp"

#include "audit_regressions.hpp"

namespace {

int fail(const char *what) {
  std::fprintf(stderr, "dice: %s\n", what);
  return 1;
}

} // namespace

int main() {
  std::ifstream file(CURSIV_SAMPLE);
  if (!file)
    return fail("the sample document is not readable");
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<dice::DiceCoreServices,
                                                dice::DiceSharedCoreServices,
                                                dice::install_core_module>();

  dice_regressions(language.language);

  const cursiv::ParseResult result = language.language.parser->parse(text);
  // `recovered()` is set when the parser had to repair the input to keep
  // going: a clean document leaves it false.
  if (result.cst().recovered())
    return fail("the sample does not parse");

  // The sample is "(1d6+2)*3-1d4": tightest first, its root is the outermost
  // "-".
  const cursiv::AstNode *const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<dice::ast::BinaryNotation>)
    return fail("the parse built no binary notation at its root");

  const auto &sub = *static_cast<const dice::ast::BinaryNotation *>(root);
  if (sub.op != dice::ast::Operator::subtract)
    return fail("the root operator is not '-'");

  if (sub.left == nullptr ||
      sub.left->type != &cursiv::ast_type_tag<dice::ast::BinaryNotation>)
    return fail("the left side is not the '(1d6+2)*3' product");
  const auto &product =
      *static_cast<const dice::ast::BinaryNotation *>(sub.left);
  if (product.op != dice::ast::Operator::multiply)
    return fail("the left side operator is not '*'");

  if (sub.right == nullptr ||
      sub.right->type != &cursiv::ast_type_tag<dice::ast::DiceRoll>)
    return fail("the right side is not a dice roll");
  const auto &roll = *static_cast<const dice::ast::DiceRoll *>(sub.right);
  if (roll.negative || !roll.count.has_value() || *roll.count != "1" ||
      roll.sides != "4")
    return fail("the '1d4' roll does not hold what the sample declares");

  std::puts("dice: the sample parses");
  return cursiv::test::report("dice_grammar_test");
}
