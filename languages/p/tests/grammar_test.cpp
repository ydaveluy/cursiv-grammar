// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "p/core/CoreServices.hpp"
#include "p/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "p: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      p::PCoreServices, p::PSharedCoreServices, p::install_core_module>();

  {
    const auto result = language.language.parser->parse("R(λ(R))λ");
    const auto* root = result.value_as<p::ast::Prog>();
    const auto* first = root && root->symbols.size() == 3
                            ? cursiv::ast_cast<p::ast::Atom>(root->symbols[0]) : nullptr;
    const auto* group = root && root->symbols.size() == 3
                            ? cursiv::ast_cast<p::ast::Iterate>(root->symbols[1]) : nullptr;
    const auto* lambda = group && group->symbols.size() == 2
                            ? cursiv::ast_cast<p::ast::Atom>(group->symbols[0]) : nullptr;
    const auto* inner = group && group->symbols.size() == 2
                            ? cursiv::ast_cast<p::ast::Iterate>(group->symbols[1]) : nullptr;
    const auto* nested = inner && inner->symbols.size() == 1
                            ? cursiv::ast_cast<p::ast::Atom>(inner->symbols[0]) : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !first || first->kind != p::ast::AtomKind::r ||
        !lambda || lambda->kind != p::ast::AtomKind::l ||
        !nested || nested->kind != p::ast::AtomKind::r ||
        first->parent != root || group->parent != root ||
        lambda->parent != group || inner->parent != group || nested->parent != inner)
      return fail("AST", "atom values, nesting, order or parents differ");
  }
  for (const auto* invalid : {"", "()", "(R", "R)", "L", "r"}) {
    const auto result = language.language.parser->parse(invalid);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail(invalid, "outside the retained p.g4 text profile");
  }

  int total = 0;
  int failures = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const std::filesystem::path& file = entry.path();
    ++total;

    std::ifstream in(file);
    if (!in) {
      failures += fail(file, "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<p::ast::Prog>)
      failures += fail(file, "the parse built no Prog");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("p: %d example(s) parse\n", total);
  return 0;
}
