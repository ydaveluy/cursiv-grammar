// The smoke test of the language: build the container the way a workspace
// does, parse every document under `example/`, and check each tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "lark/core/CoreServices.hpp"
#include "lark/core/ast.hpp"
#include "native_regressions.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "lark: %s\n", what.c_str());
  return 1;
}

std::string read(const std::filesystem::path &path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<lark::LarkCoreServices,
                                lark::LarkSharedCoreServices,
                                lark::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "%extend NUMBER: /0x[0-9a-f]+/\n%override name: \"x\"\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool token = false, rule = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *d = cursiv::ast_cast<lark::ast::OverrideStmt>(node)) {
        token = token || (d->extend && d->declaration &&
                          d->declaration->terminal);
        rule = rule || (!d->extend && d->declaration &&
                        !d->declaration->terminal);
      }
    if (!token || !rule)
      return 1;
  }

  const auto language = cursiv::create_language<lark::LarkCoreServices,
                                                lark::LarkSharedCoreServices,
                                                lark::install_core_module>();

  if (!native_regressions(language.language)) return 1;

  const std::filesystem::path dir = CURSIV_EXAMPLE_DIR;
  std::vector<std::filesystem::path> files;
  for (const auto &entry : std::filesystem::directory_iterator(dir))
    if (entry.is_regular_file())
      files.push_back(entry.path());
  if (files.empty())
    return fail("no example documents found");

  for (const std::filesystem::path &path : files) {
    const std::string text = read(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.filename().string() + ": did not parse clean");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<lark::ast::Start>)
      return fail(path.filename().string() + ": built no Start node");
  }

  std::printf("lark: %zu example documents parse\n", files.size());
  return 0;
}
