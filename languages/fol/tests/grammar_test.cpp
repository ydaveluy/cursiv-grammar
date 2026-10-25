// The smoke test of the language: parse every file in example/, and check each
// tree; functions.fol under tests/ is also read back field by field.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "fol/core/CoreServices.hpp"
#include "fol/core/ast.hpp"

#include "native_cases.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "fol: %s: %s\n", file.string().c_str(), what);
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      fol::FolCoreServices, fol::FolSharedCoreServices,
      fol::install_core_module>();

  if (!audit_regressions::run(language.language, *language.shared)) return 1;

  int total = 0;
  int failures = 0;
  for (const auto& entry :
      std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    ++total;
    const std::filesystem::path path = entry.path();
    std::ifstream file(path);
    if (!file) {
      fail(path, "not readable");
      ++failures;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path, "the parse was recovered");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<fol::ast::Condition>) {
      fail(path, "the parse built no Condition");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "fol: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;

  const std::filesystem::path funcPath =
      std::filesystem::path(CURSIV_TEST_DIR) / "functions.fol";
  std::ifstream funcFile(funcPath);
  if (!funcFile) {
    fail(funcPath, "not readable");
    return 1;
  }
  const std::string funcText((std::istreambuf_iterator<char>(funcFile)),
                             std::istreambuf_iterator<char>());
  const cursiv::ParseResult funcResult =
      language.language.parser->parse(funcText);
  if (funcResult.cst().recovered()) {
    fail(funcPath, "the parse was recovered");
    return 1;
  }
  fol::ast::Condition* const cond = as<fol::ast::Condition>(funcResult.value);
  if (cond == nullptr || cond->formulas.size() != 4) {
    fail(funcPath, "wrong formula count");
    return 1;
  }
  fol::ast::EqualityAtom* const eq = as<fol::ast::EqualityAtom>(cond->formulas[0]);
  fol::ast::FuncCall* const call =
      eq != nullptr ? as<fol::ast::FuncCall>(eq->left) : nullptr;
  if (call == nullptr || call->func != "presidentOf" || call->args.size() != 1) {
    fail(funcPath, "a function call term misread");
    return 1;
  }

  std::printf("fol: %d example(s) parse\n", total);
  return 0;
}
