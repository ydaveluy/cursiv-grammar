// The grammar's smoke test: every document of example/ parses clean (no
// recovery, no diagnostics) and builds a Model.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "alpaca/core/CoreServices.hpp"
#include "alpaca/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "alpaca: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      alpaca::AlpacaCoreServices, alpaca::AlpacaSharedCoreServices,
      alpaca::install_core_module>();

  // ALPACA 1.1's initial configuration is the entire remainder, and a
  // lowercase v denotes a relative state, including when used alone.
  {
    const auto result = language.language.parser->parse(
        "state Dead \" \" to v when 4294967296 Dead; "
        "state Alive \"*\" begin\n *\n**\n");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("<native configuration>", "did not parse cleanly");
    const auto* model = result.value_as<alpaca::ast::Model>();
    if (!model || model->states.size() != 2 || !model->beginTerminated ||
        model->initialConfiguration != "\n *\n**\n" ||
        model->states[0]->rules.size() != 1)
      return fail("<native configuration>", "configuration was lost");
    const auto* rule = model->states[0]->rules[0];
    const auto* target = cursiv::ast_cast<alpaca::ast::ArrowStateRef>(rule->target);
    const auto* guard = cursiv::ast_cast<alpaca::ast::AdjacencyPred>(rule->guard);
    if (!target || target->chain != "v" || !guard ||
        guard->count != "4294967296")
      return fail("<native configuration>", "state or count was changed");
  }

  int seen = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() != ".alp") continue;
    ++seen;

    std::ifstream file(entry.path());
    if (!file) return fail(entry.path(), "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result =
        language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path(), "did not parse clean");
    if (!result.diagnostics.empty())
      return fail(entry.path(), "left diagnostics behind");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<alpaca::ast::Model>)
      return fail(entry.path(), "built no Model");
  }
  if (seen == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  std::puts("alpaca: the examples parse");
  return 0;
}
