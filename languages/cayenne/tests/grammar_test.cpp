// The cayenne conformance suite: every document under tests/ must parse
// without recovery and root an Expr.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cayenne/core/CoreServices.hpp"
#include "cayenne/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

bool check(const fs::path& file) {
  std::ifstream stream(file);
  if (!stream) {
    std::fprintf(stderr, "cayenne: %s: not readable\n", file.string().c_str());
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(stream)),
                         std::istreambuf_iterator<char>());

  const auto language = cursiv::create_language<
      cayenne::CayenneCoreServices, cayenne::CayenneSharedCoreServices,
      cayenne::install_core_module>();

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "cayenne: %s: does not parse clean\n",
                file.string().c_str());
    return false;
  }

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      !root->type->derivesFrom(&cursiv::ast_type_tag<cayenne::ast::Expr>)) {
    std::fprintf(stderr, "cayenne: %s: did not build an Expr\n",
                file.string().c_str());
    return false;
  }
  return true;
}

}  // namespace

int main() {
  bool ok = true;
  int count = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_TEST_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".cayenne")
      continue;
    ++count;
    ok &= check(entry.path());
  }

  if (count == 0) {
    std::fprintf(stderr, "cayenne: no examples found under %s\n",
                CURSIV_TEST_DIR);
    return 1;
  }
  if (!ok) return 1;

  std::printf("cayenne: %d examples parse clean\n", count);
  return 0;
}
