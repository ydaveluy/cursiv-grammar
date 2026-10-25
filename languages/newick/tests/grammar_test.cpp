
#include <cstdio>

#include "code_audit.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "newick/core/CoreServices.hpp"
#include "newick/core/LengthValue.hpp"
#include "newick/core/ast.hpp"
// Newick end to end: the tree accepted (and refused) at the CST, the AST a
// clade builds -- name, length, children -- and every corpus file in
// example/, parsed clean.

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/CstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "newick/generated/Newick/Module.hpp"

namespace {

namespace ast = newick::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      newick::generated::Newick::create_parser();
  return *p;
}

bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

void accepted_texts() {
  CHECK(accepts("A;"));        // a bare leaf, no enclosing parens
  CHECK(accepts(";"));         // the fully anonymous single leaf
  CHECK(accepts("A:1.5;"));    // a bare leaf with a length
  CHECK(accepts("(,,(,));"));  // anonymous leaves throughout
  CHECK(accepts("(A,B);"));
  CHECK(accepts("(A,B)C;"));
  CHECK(accepts("((A,B),C);"));
  CHECK(accepts(" \t\r\n (A,B) ; \r\n"));  // trivia around the whole text
}

void refused_texts() {
  CHECK(!accepts(""));          // no terminating ';' at all
  CHECK(!accepts("A"));         // missing the terminating ';'
  CHECK(!accepts("(A,B;"));     // unbalanced
  CHECK(!accepts("(A,B))C;"));  // unbalanced the other way
  CHECK(accepts("1A;"));        // labels may start with a digit
  CHECK(!accepts("A::1.5;"));   // one colon only
  CHECK(!accepts("(A B);"));    // siblings need a comma between them
}

void ast_shape() {
  const cursiv::ParseResult r =
      engine().parse("(A:0.1,B:0.2,(C:0.3,D:0.4)E:0.5)F;");
  CHECK(r.accepted && !r.cst().recovered());
  CHECK(static_cast<bool>(r) && r.diagnostics.empty());

  ast::Clade* const root = r.value_as<ast::Clade>();
  CHECK(root != nullptr);
  if (root == nullptr) return;
  // The entry adopts the clade: its record is the whole document, `;`
  // included.
  CHECK(r.cst()[root->cstNodeId].begin == 0 &&
        r.cst()[root->cstNodeId].end == 34);
  CHECK(root->name == "F");
  CHECK(!root->length.has_value());
  CHECK(root->children.size() == 3);
  if (root->children.size() != 3) return;

  CHECK(root->children[0]->name == "A");
  CHECK(root->children[0]->length.has_value() &&
        newick::length_value(*root->children[0]->length) == 0.1);
  CHECK(root->children[0]->children.empty());
  CHECK(root->children[0]->parent == root);

  ast::Clade* const internal = root->children[2];
  CHECK(internal->name == "E");
  CHECK(internal->length.has_value() &&
        newick::length_value(*internal->length) == 0.5);
  CHECK(internal->children.size() == 2);
  CHECK(internal->children.size() == 2 && internal->children[0]->name == "C");
}

// The textbook Newick example: `(,)` is one comma, so two anonymous leaves.
void ast_anonymous_leaves() {
  const cursiv::ParseResult r = engine().parse("(,,(,));");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Clade* const root = r.value_as<ast::Clade>();
  CHECK(root != nullptr && root->children.size() == 3);
  if (root == nullptr || root->children.size() != 3) return;
  CHECK(root->children[0]->name.empty() && root->children[0]->children.empty());
  CHECK(root->children[1]->name.empty());
  CHECK(root->children[2]->children.size() == 2);
  CHECK(root->children[2]->children.size() == 2 &&
        root->children[2]->children[0]->name.empty() &&
        root->children[2]->children[1]->name.empty());
}

void corpus() {
  namespace fs = std::filesystem;
  int seen = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const auto& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".nwk")
        continue;
      ++seen;
      const std::string path = entry.path().string();
      const std::string text = cursiv::test::read_file(path);
      const cursiv::ParseResult r = engine().parse(text);
      if (!r.accepted || r.cst().recovered()) {
        std::printf("FAIL %s: did not parse clean\n", path.c_str());
        ++failures;
        continue;
      }
      if (r.value_as<ast::Clade>() == nullptr) {
        std::printf("FAIL %s: root is not a Clade\n", path.c_str());
        ++failures;
      }
    }
  }
  CHECK(seen == 9);
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<newick::NewickCoreServices,
                              newick::NewickSharedCoreServices,
                              newick::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "('Homo sapiens':-1e-2,Pan_troglodytes[comment]);");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* c = r.value_as<newick::ast::Clade>();
    if (!c || c->children.size() != 2 ||
        c->children[0]->name != "Homo sapiens" ||
        c->children[1]->name != "Pan troglodytes" || !c->children[0]->length ||
        newick::length_value(*c->children[0]->length) != -0.01)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression() || !code_audit::run()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  accepted_texts();
  refused_texts();
  ast_shape();
  ast_anonymous_leaves();
  corpus();
  return cursiv::test::report("newick_grammar_test");
}
