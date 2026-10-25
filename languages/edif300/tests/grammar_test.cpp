// The smoke test of the language: the upstream corpus document (empty.edf)
// parses clean and its header/library/design structure reads back correctly;
// coverage.edf (ours, not upstream) exercises constructs empty.edf does not
// reach: keyword levels, boolean expressions, numbers, nested comments,
// nets, instances, properties, a deliberately invalid document.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "edif300/core/CoreServices.hpp"
#include "edif300/core/ast.hpp"

namespace {

namespace ast = edif300::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "edif300: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const ast::Form* as_form(const ast::Value& v) {
  if (!std::holds_alternative<cursiv::AstNode*>(v)) return nullptr;
  const cursiv::AstNode* node = std::get<cursiv::AstNode*>(v);
  if (node == nullptr || node->type != &cursiv::ast_type_tag<ast::Form>)
    return nullptr;
  return static_cast<const ast::Form*>(node);
}

// Depth-first search for the first Form (anywhere below, args included)
// whose keyword matches -- good enough for a smoke test's navigation.
const ast::Form* find(const ast::Form* root, std::string_view keyword) {
  if (root == nullptr) return nullptr;
  for (const ast::Value& v : root->args) {
    const ast::Form* child = as_form(v);
    if (child == nullptr) continue;
    if (child->keyword == keyword) return child;
    if (const ast::Form* deep = find(child, keyword); deep != nullptr)
      return deep;
  }
  return nullptr;
}

std::string_view as_text(const ast::Value& v) {
  return std::holds_alternative<std::string_view>(v) ? std::get<std::string_view>(v)
                                                      : std::string_view{};
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      edif300::Edif300CoreServices, edif300::Edif300SharedCoreServices,
      edif300::install_core_module>();

  // -- the upstream corpus: empty.edf --
  const std::string corpusPath = std::string(CURSIV_EXAMPLE_DIR) + "/empty.edf";
  const std::string corpusText = read_file(corpusPath);
  if (corpusText.empty()) return fail("empty.edf: empty or unreadable");

  const cursiv::ParseResult corpus = language.language.parser->parse(corpusText);
  if (corpus.cst().recovered()) return fail("empty.edf: does not parse clean");
  if (!corpus.diagnostics.empty()) return fail("empty.edf: has diagnostics");

  const cursiv::AstNode* const corpusRoot = corpus.value;
  if (corpusRoot == nullptr ||
      corpusRoot->type != &cursiv::ast_type_tag<ast::Form>)
    return fail("empty.edf: built no Form");
  const auto& root = *static_cast<const ast::Form*>(corpusRoot);
  if (root.keyword != "(edif") return fail("empty.edf: root is not (edif ...)");
  if (root.args.empty() || as_text(root.args[0]) != "EXAMPLE_EMPTY")
    return fail("empty.edf: wrong edif name");

  const ast::Form* library = find(&root, "(library");
  if (library == nullptr || library->args.empty() ||
      as_text(library->args[0]) != "LIB")
    return fail("empty.edf: wrong library name");
  const ast::Form* cell = find(library, "(cell");
  if (cell == nullptr || cell->args.empty() || as_text(cell->args[0]) != "CELL")
    return fail("empty.edf: wrong cell name");

  const ast::Form* design = find(&root, "(design");
  if (design == nullptr || design->args.empty() ||
      as_text(design->args[0]) != "DESIGN")
    return fail("empty.edf: wrong design name");
  const ast::Form* cellRef = find(design, "(cellRef");
  if (cellRef == nullptr || cellRef->args.empty() ||
      as_text(cellRef->args[0]) != "CELL")
    return fail("empty.edf: design's cellRef names the wrong cell");

  const ast::Form* fonts = find(&root, "(unit");
  if (fonts == nullptr) return fail("empty.edf: no unit definition found");

  // -- coverage.edf (ours): keyword levels, expressions, nets, a nested
  // comment, a deliberately invalid document --
  const std::string covText = read_file(std::string(CURSIV_TEST_DIR) + "/coverage.edf");
  if (covText.empty()) return fail("coverage.edf: empty or unreadable");

  const cursiv::ParseResult cov = language.language.parser->parse(covText);
  if (cov.cst().recovered()) return fail("coverage.edf: does not parse clean");
  if (!cov.diagnostics.empty()) return fail("coverage.edf: has diagnostics");

  const auto& covRoot = *static_cast<const ast::Form*>(cov.value);
  const ast::Form* connView = find(&covRoot, "(connectivityView");
  if (connView == nullptr) return fail("coverage.edf: no connectivityView");
  const ast::Form* instance = find(connView, "(instance");
  if (instance == nullptr || instance->args.empty() ||
      as_text(instance->args[0]) != "U1")
    return fail("coverage.edf: wrong instance name");
  const ast::Form* net = find(connView, "(connectivityNet");
  if (net == nullptr) return fail("coverage.edf: no connectivityNet");
  const ast::Form* prop = find(connView, "(property");
  if (prop == nullptr || prop->args.empty() ||
      as_text(prop->args[0]) != "note")
    return fail("coverage.edf: wrong property name");
  // The property's typed value is (string "hello \"there\""): read the
  // wrapped decimal or string text back out to prove a converter ran, not
  // just that the shape parsed.
  const ast::Form* iprop = find(connView, "(integerParameterAssign");
  if (iprop == nullptr) return fail("coverage.edf: no integerParameterAssign");
  const ast::Form* sum = find(iprop, "(integerSum");
  if (sum == nullptr || sum->args.size() != 2)
    return fail("coverage.edf: integerSum should hold two operands");
  const ast::Form* neg = find(iprop, "(integerSubtract");
  if (neg == nullptr) return fail("coverage.edf: no integerSubtract");

  bool sawNegative = false;
  for (const ast::Value& v : neg->args)
    if (std::holds_alternative<double>(v) && std::get<double>(v) == -3.0)
      sawNegative = true;
  if (!sawNegative)
    return fail("coverage.edf: negative decimal literal read back wrong");

  const ast::Form* k3 = find(&covRoot, "(k3KeywordLevel");
  if (k3 == nullptr) return fail("coverage.edf: no k3KeywordLevel");

  std::puts("edif300: the corpus and coverage documents parse");
  return 0;
}
