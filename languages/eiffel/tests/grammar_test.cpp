// The grammar end to end: every document under example/ parses clean
// (recovered() false, no diagnostics, the right root type), and a few real
// field values come back out of one document's AST.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "eiffel/core/ast.hpp"
#include "eiffel/generated/Eiffel/Module.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "eiffel: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      eiffel::generated::Eiffel::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  std::vector<fs::path> docs;
  // `features.e` and `inspect.e` live beside this test, not in the corpus:
  // they are written here for the constructs the corpus never reaches.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".e")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path readback_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = engine().parse(text);
    if (!result.accepted)
      return fail(doc.string() + ": the parse was refused");
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<eiffel::ast::ClassDecl>)
      return fail(doc.string() + ": the parse built no class declaration");
    if (doc.filename() == "application.e" &&
        doc.parent_path().filename() == "100_doors")
      readback_doc = doc;
  }

  if (readback_doc.empty())
    return fail("100_doors/application.e was not found among the examples");

  // Read a few real field values out of one document's AST: a converter or a
  // link that silently misfires would still leave `recovered()` false and
  // the root type right, so only reading the fields back catches it.
  const std::string text = read_file(readback_doc);
  const cursiv::ParseResult result = engine().parse(text);
  const auto& cls = *static_cast<const eiffel::ast::ClassDecl*>(result.value);
  if (cls.name != "APPLICATION")
    return fail("100_doors/application.e: wrong class name");
  if (cls.creators.size() != 1 || cls.creators[0]->procedures.size() != 1 ||
      cls.creators[0]->procedures[0] != "make")
    return fail("100_doors/application.e: wrong creation clause");
  if (cls.features.empty())
    return fail("100_doors/application.e: no feature clauses");
  bool found_initialize = false;
  for (const eiffel::ast::FeatureClause* const clause : cls.features)
    for (const eiffel::ast::FeatureDeclaration* const decl : clause->features)
      for (const eiffel::ast::NewFeatureName* const name : decl->names)
        if (name->name == "initialize_closed_doors") found_initialize = true;
  if (!found_initialize)
    return fail("100_doors/application.e: missing initialize_closed_doors");

  std::printf("eiffel: %zu documents parse clean\n", docs.size());
  return 0;
}
