
#include <cstdio>

#include "cursiv/parser/Parser.hpp"
#include "cursiv/services/CoreLanguage.hpp"
#include "native_regressions.hpp"
#include "resume_regressions.hpp"
#include "native_followup.hpp"
#include "objc/CoreServices.hpp"
#include "objc/ast.hpp"
// The smoke test of the language: parse every file under example/, the way a
// workspace would build one.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/syntax-tree/AstNode.hpp"

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
  {
    const auto parsed = language.language.parser->parse(
        "IBOutlet IBOutletCollection(View) IBOutlet id views;"
        "void f(int a[const static volatile 4]);");
    const objc::ast::Specifiers* outlets = nullptr;
    const objc::ast::ArraySuffix* array = nullptr;
    for (const auto* node : parsed.nodes()) {
      if (const auto* n = cursiv::ast_cast<objc::ast::Specifiers>(node);
          n && !n->ibOutlets.empty()) outlets = n;
      if (const auto* n = cursiv::ast_cast<objc::ast::ArraySuffix>(node)) array = n;
    }
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        !outlets || outlets->ibOutlets.size() != 2 || !outlets->ibOutlet() ||
        !outlets->ibOutletCollection() || outlets->ibOutletCollectionTypes.size() != 1 ||
        outlets->ibOutletCollectionTypes[0] != "View" || !array ||
        !array->isStatic || array->qualifiers.size() != 2)
      return fail("repeatable outlet specifiers and static array qualifiers");
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

  if (!objc_native::run(language.language)) return fail("native helper failed");
  if (!objc_resume::run(language.language)) return fail("resume helper failed");
  if (!objc_followup::run(language.language)) return fail("follow-up helper failed");

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
