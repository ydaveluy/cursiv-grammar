#include <cstdio>

#include "code_audit.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "pgn/core/CoreServices.hpp"
#include "pgn/core/ast.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every document under tests/, and check each tree.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "pgn: %s: %s\n", path.c_str(), what);
  return 1;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<pgn::PgnCoreServices, pgn::PgnSharedCoreServices,
                              pgn::install_core_module>();
  {
    const auto r = language.language.parser->parse("1... e5 *");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* db = r.value_as<pgn::ast::Database>();
    if (!db || db->games.empty() || db->games[0]->elements.empty()) return 1;
    const auto* m =
        cursiv::ast_cast<pgn::ast::MoveNumber>(db->games[0]->elements[0]);
    if (!m || m->dots != "...") return 1;
  }
  return 0;
}

int main() {
  if (!code_audit::run()) return 1;
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(CURSIV_TEST_DIR))
    if (entry.path().extension() == ".pgn") files.push_back(entry.path());
  if (files.empty()) return fail(CURSIV_TEST_DIR, "no example found");

  const auto language =
      cursiv::create_language<pgn::PgnCoreServices, pgn::PgnSharedCoreServices,
                              pgn::install_core_module>();

  int checked = 0;
  for (const std::filesystem::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string(), "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) return fail(path.string(), "does not parse");
    if (!result.diagnostics.empty())
      return fail(path.string(), "parses with diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<pgn::ast::Database>)
      return fail(path.string(), "built no database");

    const auto& database = *static_cast<const pgn::ast::Database*>(root);
    if (database.games.empty())
      return fail(path.string(), "the database holds no game");
    ++checked;
  }

  std::printf("pgn: %d sample(s) parse\n", checked);
  return 0;
}
