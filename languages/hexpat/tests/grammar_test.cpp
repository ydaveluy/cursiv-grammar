// The grammar lane: every file of example/ parses clean, and a few field
// values read back out of one document confirm the AST is not just
// structurally right but holds what the source actually wrote.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "hexpat/core/CoreServices.hpp"
#include "hexpat/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "hexpat: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const hexpat::ast::Document* parse_document(
    const cursiv::CoreLanguage<hexpat::HexpatCoreServices,
                               hexpat::HexpatSharedCoreServices>& language,
    const std::string& text, const std::string& label, bool* ok) {
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "hexpat: %s does not parse clean\n", label.c_str());
    *ok = false;
    return nullptr;
  }
  if (!result.diagnostics.empty()) {
    std::fprintf(stderr, "hexpat: %s carries %zu diagnostic(s)\n",
                 label.c_str(), result.diagnostics.size());
    *ok = false;
    return nullptr;
  }
  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<hexpat::ast::Document>) {
    std::fprintf(stderr, "hexpat: %s built no document\n", label.c_str());
    *ok = false;
    return nullptr;
  }
  return static_cast<const hexpat::ast::Document*>(root);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      hexpat::HexpatCoreServices, hexpat::HexpatSharedCoreServices,
      hexpat::install_core_module>();

  const std::filesystem::path exampleDir = CURSIV_EXAMPLE_DIR;
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(exampleDir))
    if (entry.is_regular_file()) files.push_back(entry.path());
  if (files.empty()) return fail("no example files found");

  bool ok = true;
  const hexpat::ast::Document* dexDoc = nullptr;
  std::string dexText;
  for (const auto& path : files) {
    std::string text = read_file(path);
    const hexpat::ast::Document* doc =
        parse_document(language, text, path.filename().string(), &ok);
    if (path.filename() == "dex.hexpat") {
      dexText = std::move(text);
      dexDoc = doc;
    }
  }
  // A construct the three-file corpus never exercises (for/ternary/try-catch/
  // sizeof/typenameof/io direction/section-name/generic using) still has to
  // parse: this fixture is ours, so it lives under tests/, not example/, and
  // never counts toward the corpus totals.
  {
    const std::filesystem::path coverage =
        std::filesystem::path(CURSIV_TEST_DIR) / "fixtures/coverage.hexpat";
    const std::string text = read_file(coverage);
    parse_document(language, text, "fixtures/coverage.hexpat", &ok);
  }
  if (!ok) return 1;

  // Read real field values back out of dex.hexpat: the first import, and the
  // first struct's name and declarators, catching a converter that silently
  // yields the wrong span even though the tree shape is right.
  if (dexDoc == nullptr) return fail("dex.hexpat did not parse for readback");

  const hexpat::ast::Import* import = nullptr;
  const hexpat::ast::Struct* headerItem = nullptr;
  for (const hexpat::ast::Item* item : dexDoc->items) {
    if (import == nullptr &&
        item->type == &cursiv::ast_type_tag<hexpat::ast::Import>)
      import = static_cast<const hexpat::ast::Import*>(item);
    if (headerItem == nullptr &&
        item->type == &cursiv::ast_type_tag<hexpat::ast::Struct>) {
      const auto* s = static_cast<const hexpat::ast::Struct*>(item);
      if (s->name == "header_item") headerItem = s;
    }
  }

  if (import == nullptr || import->parts.size() != 2 ||
      import->parts[0] != "type" || import->parts[1] != "leb128")
    return fail("the import's dotted parts do not read back right");

  if (headerItem == nullptr)
    return fail("struct header_item was not found");
  if (headerItem->body == nullptr || headerItem->body->items.size() != 23)
    return fail("header_item does not hold the 23 fields it declares");

  const hexpat::ast::Item* first = headerItem->body->items[0];
  if (first->type != &cursiv::ast_type_tag<hexpat::ast::StructMember>)
    return fail("header_item's first field is not a StructMember");
  const auto* member = static_cast<const hexpat::ast::StructMember*>(first);
  if (member->type == nullptr || member->type->name != "u8" ||
      member->declarators.size() != 1 ||
      member->declarators[0]->name != "magic" ||
      !member->declarators[0]->hasBrackets)
    return fail("header_item's `u8 magic[8];` does not read back right");

  std::puts("hexpat: every example parses clean");
  return 0;
}
