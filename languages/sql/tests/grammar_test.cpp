#include "native_followup.hpp"
#include "audit_regressions.hpp"
#include "code_audit.hpp"
// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Script at the root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/parser/Parser.hpp"
#include "cursiv/services/CoreLanguage.hpp"
#include "cursiv/syntax-tree/AstNode.hpp"
#include "cursiv/syntax-tree/ParseResult.hpp"
#include "sqlite/CoreServices.hpp"
#include "sqlite/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "sqlite: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  if (!native_followup::run() || !audit_regressions::run() || !code_audit::run()) return 1;
  const auto language = cursiv::create_language<
      sqlite::SqliteCoreServices, sqlite::SqliteSharedCoreServices,
      sqlite::install_core_module>();

  {
    const auto parsed = language.language.parser->parse(
        "CREATE TABLE flags(id INTEGER PRIMARY KEY) STRICT, WITHOUT ROWID, STRICT;");
    const sqlite::ast::CreateTableStmt* table = nullptr;
    for (const auto* node : parsed.nodes())
      if (const auto* n = cursiv::ast_cast<sqlite::ast::CreateTableStmt>(node)) table = n;
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        !table || !table->strict() || !table->withoutRowid() || table->options.size() != 3 ||
        table->options[0] != sqlite::ast::TableOption::strict ||
        table->options[1] != sqlite::ast::TableOption::withoutRowid ||
        table->options[2] != sqlite::ast::TableOption::strict)
      return fail("table options", "all repeated options retained in order");
  }

  int failures = 0;
  int count = 0;
  for (const char* const dir :
       {CURSIV_EXAMPLE_DIR "/example", CURSIV_TEST_DIR})
  for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".sql")
      continue;
    ++count;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path(), "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(entry.path(), "the document carries diagnostics");
      continue;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<sqlite::ast::Script>) {
      failures += fail(entry.path(), "the parse built no Script");
      continue;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "sqlite: no .sql file found under example/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("sqlite: %d example documents parse clean\n", count);
  return 0;
}
