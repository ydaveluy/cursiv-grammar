// The smoke test of the language: every file in example/ parses clean and
// builds a Program.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "audit_regressions.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "datalog/core/CoreServices.hpp"
#include "datalog/core/ast.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "datalog: %s: %s\n", path.c_str(), what);
  return 1;
}

std::vector<std::string> split(const std::string& s, char sep) {
  std::vector<std::string> parts;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, sep))
    if (!item.empty()) parts.push_back(item);
  return parts;
}

}  // namespace

int main() {
  const auto language =
      cursiv::create_language<datalog::DatalogCoreServices,
                              datalog::DatalogSharedCoreServices,
                              datalog::install_core_module>();
  if (datalog_regressions(language.language)) return 1;

  for (const std::string& path : split(CURSIV_SAMPLES, ',')) {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path, "the sample does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<datalog::ast::Program>)
      return fail(path, "the parse built no Program");

    std::printf("datalog: %s parses\n", path.c_str());
  }

  return 0;
}
