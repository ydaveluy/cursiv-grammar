// The smoke test of the language: every file in example/ parses clean and
// builds a KotlinFile.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "kotlin/core/CoreServices.hpp"
#include "kotlin/core/ast.hpp"
#include "modern_cases.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "kotlin: %s: %s\n", path.c_str(), what);
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

bool ends_with(const std::string& path, const char* suffix) {
  const std::string s(suffix);
  return path.size() >= s.size() &&
         path.compare(path.size() - s.size(), s.size(), s) == 0;
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      kotlin::KotlinCoreServices, kotlin::KotlinSharedCoreServices,
      kotlin::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("class C { fun f() {} }\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      kotlin::KotlinCoreServices, kotlin::KotlinSharedCoreServices,
      kotlin::install_core_module>();

  if (!kotlin_audit::modern_cases(*language.language.parser))
    return 1;

  std::vector<std::string> paths = split(CURSIV_SAMPLES, ',');
  const std::vector<std::string> testPaths = split(CURSIV_TEST_SAMPLES, ',');
  paths.insert(paths.end(), testPaths.begin(), testPaths.end());

  for (const std::string& path : paths) {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);

    // fun_on_same_line.kt has an upstream .errors sibling: two top-level
    // functions with no NEWLINE/';' between them, refused, not merely
    // tolerated.
    if (ends_with(path, "fun_on_same_line.kt")) {
      if (!result.cst().recovered() && result.diagnostics.empty())
        return fail(path, "expected this sample to be refused");
      std::printf("kotlin: %s is correctly refused\n", path.c_str());
      continue;
    }

    if (result.cst().recovered()) return fail(path, "the sample does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<kotlin::ast::KotlinFile>)
      return fail(path, "the parse built no KotlinFile");

    std::printf("kotlin: %s parses\n", path.c_str());
  }

  return 0;
}
