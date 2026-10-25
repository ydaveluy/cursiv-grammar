
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "native_regressions.hpp"
#include "resume_regressions.hpp"
#include "objc/core/CoreServices.hpp"
#include "objc/core/ast.hpp"
// The smoke test of the language: parse every file under example/, the way a
// workspace would build one.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "objc: %s\n", what.c_str());
  return 1;
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<objc::ObjcCoreServices,
                                                objc::ObjcSharedCoreServices,
                                                objc::install_core_module>();
  {
    const auto r = language.language.parser->parse("int f(int);");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* unit = r.value_as<objc::ast::TranslationUnit>();
    const auto* declaration =
        unit && unit->items.size() == 1
            ? cursiv::ast_cast<objc::ast::Declaration>(unit->items[0])
            : nullptr;
    const auto* declarator = declaration && declaration->declarators.size() == 1
                                 ? declaration->declarators[0]->declarator
                                 : nullptr;
    const auto* function = declarator && declarator->suffixes.size() == 1
                               ? cursiv::ast_cast<objc::ast::FunctionSuffix>(
                                     declarator->suffixes[0])
                               : nullptr;
    if (!declarator || declarator->name != "f" || !function ||
        function->params.size() != 1 || !function->params[0]->specifiers ||
        function->params[0]->declarator)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.path().extension() == ".m") files.push_back(entry.path());
  if (files.empty()) return fail("no example files found");

  const auto language = cursiv::create_language<objc::ObjcCoreServices,
                                                objc::ObjcSharedCoreServices,
                                                objc::install_core_module>();

  if (!objc_native::run(language.language)) return 1;
  if (!objc_resume::run(language.language)) return 1;

  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<objc::ast::TranslationUnit>)
      return fail(path.string() + ": the parse built no translation unit");
  }

  std::printf("objc: %zu example(s) parse clean\n", files.size());
  return 0;
}
