// The smoke test of the language: parse every sample under example/ and
// check each one built a clean Graph.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "dot/core/CoreServices.hpp"
#include "dot/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "dot: %s\n", what);
  return 1;
}

bool parse_one(
    const cursiv::CoreLanguage<dot::DotCoreServices, dot::DotSharedCoreServices>&
        language,
    const char* path) {
  std::ifstream file(path);
  if (!file) {
    std::fprintf(stderr, "dot: %s is not readable\n", path);
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "dot: %s did not parse clean\n", path);
    return false;
  }
  if (!result.diagnostics.empty()) {
    std::fprintf(stderr, "dot: %s carries diagnostics\n", path);
    return false;
  }

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<dot::ast::Graph>) {
    std::fprintf(stderr, "dot: %s did not build a Graph\n", path);
    return false;
  }
  return true;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      dot::DotCoreServices,
      dot::DotSharedCoreServices,
      dot::install_core_module>();
  {
    const auto result = language.language.parser->parse(
        R"DOT(digraph "a" + /* " */ "b" { é [label="x\"" + "y"]; })DOT");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* graph = result.value_as<dot::ast::Graph>();
    if (!graph || !graph->id || *graph->id != "ab") return 1;
    bool label = false, unicode = false;
    for (const auto* node : result.nodes) {
      if (const auto* attr = cursiv::ast_cast<dot::ast::AttrPair>(node))
        label |= attr->name == "label" && attr->value && *attr->value == "x\"y";
      if (const auto* id = cursiv::ast_cast<dot::ast::NodeId>(node)) unicode |= id->id == "é";
    }
    if (!label || !unicode) return 1;
  }


  static constexpr const char* kExamples[] = {
      CURSIV_EXAMPLE_DIR "/crazy.dot",
      CURSIV_EXAMPLE_DIR "/cluster.dot",
      CURSIV_EXAMPLE_DIR "/dg.dot",
      CURSIV_EXAMPLE_DIR "/JavaParser.variableInitializer.dot",
      CURSIV_TEST_DIR "/features.dot",
  };

  bool ok = true;
  for (const char* const path : kExamples) ok = parse_one(language, path) && ok;
  if (!ok) return fail("some example did not parse clean");

  std::puts("dot: every example parses");
  return 0;
}
