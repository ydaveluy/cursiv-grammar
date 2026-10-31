// The grammar end to end: every file of `example/` parses clean, and one
// document's AST is read back to check a converter actually ran.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "elixir/core/CoreServices.hpp"
#include "elixir/core/ast.hpp"
#include "audit_regressions.hpp"
#include "modern_cases.hpp"
#include "heredoc_lines.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "elixir: %s: %s\n", file.string().c_str(), what);
  return 1;
}

std::string read(const fs::path& file) {
  std::ifstream in(file);
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

// These fixtures are rejected by the native parser.
const std::set<std::string>& knownFailing() {
  static const std::set<std::string> files = {
      "expression_doBlockExpr.ex",
      "expression_operatorExpr.ex",
      "expression_rarrowExpr.ex",
      "expression_variablesExpr.ex",
  };
  return files;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      elixir::ElixirCoreServices, elixir::ElixirSharedCoreServices,
      elixir::install_core_module>();
  if (!elixir_audit::modern_cases(language)) return 1;
  if (!elixir_heredoc_lines::run(language.language)) return 1;
  if (elixir::tests::audit_regressions(*language.language.parser)) return 1;
  for (const std::string_view text : {std::string_view{R"(def join(a, b \\ nil))"}, std::string_view{"&+/2"}}) {
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("function head", "bodyless definition or operator capture failed");
  }
  for (const std::string_view text : {std::string_view{R"("#{
1
}")"}, std::string_view{R"("#{a = 1; a + 1}")"},
       std::string_view{R"("#{%{a: "x #{1}"}}")"}, std::string_view{R"("\#{literal}")"}, std::string_view{R"("#{}")"}}) {
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("interpolation", "nested or multiline interpolation failed");
  }


  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result =
        language.language.parser->parse("\"left #{foo(\"inner\")} right\"");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      std::fprintf(stderr, "audit regression: parse failed\n");
      return 1;
    }
    bool interpolation = false, inner = false;
    for (const auto *node : result.nodes)
      if (const auto *string = cursiv::ast_cast<elixir::ast::StringLit>(node)) {
        interpolation |=
            string->texts.size() == 2 && string->texts[0] == "left " &&
            string->texts[1] == " right" && string->holes.size() == 1;
        inner |= string->texts.size() == 1 && string->texts[0] == "inner" &&
                 string->holes.empty();
      }
    if (!interpolation || !inner)
      return 1;
  }

  const fs::path exampleDir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  int total = 0;
  int expectedInvalid = 0;
  for (const fs::directory_entry& entry : fs::directory_iterator(exampleDir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".ex")
      continue;
    ++total;
    const std::string text = read(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const bool unclean =
        result.cst().recovered() || !result.diagnostics.empty();
    const bool expectedToFail =
        knownFailing().contains(entry.path().filename().string());
    if (expectedToFail) {
      ++expectedInvalid;
      continue;
    }
    if (unclean) {
      return fail(entry.path(), "the native-confirmed document does not parse clean");
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<elixir::ast::Block>)
      return fail(entry.path(), "the parse built no block");
  }
  if (total == 0) {
    std::fprintf(stderr, "elixir: no example found under %s\n",
                 exampleDir.string().c_str());
    return 1;
  }

  // A converter that silently keeps the wrong value passes every structural
  // check above: read a few real fields back out.
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("1 + 2\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("1 + 2", "does not parse clean");
    const auto* const block = result.value_as<elixir::ast::Block>();
    if (block == nullptr || block->body.size() != 1)
      return fail("1 + 2", "the block does not hold one statement");
    const auto* const bin =
        static_cast<const elixir::ast::BinaryExpr*>(block->body[0]);
    if (bin->type != &cursiv::ast_type_tag<elixir::ast::BinaryExpr>)
      return fail("1 + 2", "the statement is not a binary expression");
    if (bin->op != elixir::ast::Operator::add)
      return fail("1 + 2", "the operator is not add");
    const auto* const left =
        static_cast<const elixir::ast::IntegerLit*>(bin->left);
    if (left->type != &cursiv::ast_type_tag<elixir::ast::IntegerLit> ||
        left->text != "1")
      return fail("1 + 2", "the left operand is not the integer 1");
  }

  {
    const cursiv::ParseResult result =
        language.language.parser->parse(":atom\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail(":atom", "does not parse clean");
    const auto* const block = result.value_as<elixir::ast::Block>();
    const auto* const atom =
        static_cast<const elixir::ast::AtomLit*>(block->body[0]);
    if (atom->type != &cursiv::ast_type_tag<elixir::ast::AtomLit> ||
        atom->text != "atom")
      return fail(":atom", "the atom text is not unquoted correctly");
  }

  // `++ -- +++ --- .. <>`, `=`, `|` and `::` are right-associative in
  // Elixir: a right operand nests, a left one does not.
  const auto isBinary = [](const cursiv::AstNode* n) {
    return n->type == &cursiv::ast_type_tag<elixir::ast::BinaryExpr>;
  };
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("a ++ b ++ c\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a ++ b ++ c", "does not parse clean");
    const auto* const bin = static_cast<const elixir::ast::BinaryExpr*>(
        result.value_as<elixir::ast::Block>()->body[0]);
    if (bin->op != elixir::ast::Operator::concat || isBinary(bin->left) ||
        !isBinary(bin->right))
      return fail("a ++ b ++ c", "++ is not right-associative");
  }
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("a = b = c\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a = b = c", "does not parse clean");
    const auto* const bin = static_cast<const elixir::ast::BinaryExpr*>(
        result.value_as<elixir::ast::Block>()->body[0]);
    if (bin->op != elixir::ast::Operator::pattern_match || isBinary(bin->left) ||
        !isBinary(bin->right))
      return fail("a = b = c", "= is not right-associative");
  }
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("a | b | c\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a | b | c", "does not parse clean");
    const auto* const bin = static_cast<const elixir::ast::BinaryExpr*>(
        result.value_as<elixir::ast::Block>()->body[0]);
    if (bin->op != elixir::ast::Operator::prepend || isBinary(bin->left) ||
        !isBinary(bin->right))
      return fail("a | b | c", "| is not right-associative");
  }
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("a :: b :: c\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a :: b :: c", "does not parse clean");
    const auto* const bin = static_cast<const elixir::ast::BinaryExpr*>(
        result.value_as<elixir::ast::Block>()->body[0]);
    if (bin->op != elixir::ast::Operator::type_op || isBinary(bin->left) ||
        !isBinary(bin->right))
      return fail("a :: b :: c", ":: is not right-associative");
  }
  // `|>`, `<<<`/`>>>` and the tilde-arrow operators are one precedence
  // level in Elixir, not three separate ones.
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("a <<< b |> c\n");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a <<< b |> c", "does not parse clean");
    const auto* const bin = static_cast<const elixir::ast::BinaryExpr*>(
        result.value_as<elixir::ast::Block>()->body[0]);
    if (bin->op != elixir::ast::Operator::pipe_op || !isBinary(bin->left) ||
        isBinary(bin->right))
      return fail("a <<< b |> c",
                   "<<< and |> are not one left-associative level");
  }

  std::printf("elixir: %d examples parse; %d expected invalid fixture%s\n",
              total - expectedInvalid, expectedInvalid,
              expectedInvalid == 1 ? "" : "s");
  return 0;
}
