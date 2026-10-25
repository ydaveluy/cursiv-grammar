// The grammar's smoke test: every document of tests/ parses clean, with
// the root type the grammar builds. The upstream corpus ships no examples/
// at all, so every document is this port's own.

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "infosapient/core/ast.hpp"
#include "infosapient/generated/Infosapient/Module.hpp"

namespace {

namespace ast = infosapient::ast;

const cursiv::Parser &engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      infosapient::generated::Infosapient::create_parser();
  return *p;
}

std::vector<std::filesystem::path> examples() {
  std::vector<std::filesystem::path> paths;
  for (const auto &entry : std::filesystem::directory_iterator(CURSIV_TEST_DIR))
    if (entry.path().extension() == ".infosapient")
      paths.push_back(entry.path());
  return paths;
}

void corpus() {
  const std::vector<std::filesystem::path> paths = examples();
  CHECK(!paths.empty());
  for (const std::filesystem::path &path : paths) {
    const std::string src = cursiv::test::read_file(path.string());
    CHECK(!src.empty());

    const cursiv::ParseResult r = engine().parse(src);
    CHECK(r.accepted);
    if (!r.accepted)
      continue;
    CHECK(!r.cst().recovered());

    const cursiv::AstNode *const root = r.value;
    CHECK(root != nullptr &&
          root->type == &cursiv::ast_type_tag<ast::ConditionalRule>);
  }
}

} // namespace

int main() {
  {
    const auto result =
        engine().parse("R if a s x and (b s y or c s z) then d s v;");
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    const auto *rule = cursiv::ast_cast<ast::ConditionalRule>(result.value);
    const auto *outer =
        rule ? cursiv::ast_cast<ast::LogicalExpr>(rule->premise) : nullptr;
    CHECK(outer && cursiv::ast_cast<ast::LogicalExpr>(outer->right));
  }

  corpus();
  return cursiv::test::report("infosapient_grammar_test");
}
