// The ttm conformance suite: every document under example/ must parse without
// recovery and root a Program.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "ttm/core/CoreServices.hpp"
#include "ttm/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

bool check(const fs::path& file) {
  std::ifstream stream(file);
  if (!stream) {
    std::fprintf(stderr, "ttm: %s: not readable\n", file.string().c_str());
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(stream)),
                         std::istreambuf_iterator<char>());

  const auto language =
      cursiv::create_language<ttm::TtmCoreServices, ttm::TtmSharedCoreServices,
                              ttm::install_core_module>();

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "ttm: %s: does not parse clean\n",
                 file.string().c_str());
    return false;
  }

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<ttm::ast::Program>) {
    std::fprintf(stderr, "ttm: %s: did not build a Program\n",
                 file.string().c_str());
    return false;
  }
  return true;
}

}  // namespace

int main() {
  bool ok = true;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".ttm")
        continue;
      ++count;
      ok &= check(entry.path());
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "ttm: no examples found under %s\n",
                 CURSIV_EXAMPLE_DIR);
    return 1;
  }
  if (!ok) return 1;

  std::printf("ttm: %d examples parse clean\n", count);
  return 0;
}
