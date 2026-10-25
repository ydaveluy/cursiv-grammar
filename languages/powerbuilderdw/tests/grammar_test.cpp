// The smoke test of the language: every example parses clean, and a few
// real field values come back out of one document's AST -- a converter that
// silently yields the wrong value would still pass a structural check alone.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "powerbuilderdw/core/CoreServices.hpp"
#include "powerbuilderdw/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "powerbuilderdw: %s\n", what);
  return 1;
}

std::string read_file(const char* path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

using Language =
    decltype(cursiv::create_language<powerbuilderdw::PowerbuilderdwCoreServices,
                                     powerbuilderdw::PowerbuilderdwSharedCoreServices,
                                     powerbuilderdw::install_core_module>());

// Parses `text` (kept alive by the caller for as long as the result is
// read) and checks the common shape every sample must have.
const powerbuilderdw::ast::Document* parse_clean(Language& language,
                                                  const std::string& text,
                                                  const char* label,
                                                  cursiv::ParseResult& out) {
  out = language.language.parser->parse(text);
  if (out.cst().recovered()) {
    std::fprintf(stderr, "powerbuilderdw: %s does not parse cleanly\n", label);
    return nullptr;
  }
  if (!out.diagnostics.empty()) {
    std::fprintf(stderr, "powerbuilderdw: %s carries diagnostics\n", label);
    return nullptr;
  }
  const cursiv::AstNode* const root = out.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<powerbuilderdw::ast::Document>) {
    std::fprintf(stderr, "powerbuilderdw: %s built no document\n", label);
    return nullptr;
  }
  return static_cast<const powerbuilderdw::ast::Document*>(root);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      powerbuilderdw::PowerbuilderdwCoreServices,
      powerbuilderdw::PowerbuilderdwSharedCoreServices,
      powerbuilderdw::install_core_module>();

  const char* const samples[] = {
      CURSIV_EXAMPLE_DIR "/d_example_detail_events.srd",
      CURSIV_TEST_DIR "/header_and_types.srd",
      CURSIV_TEST_DIR "/values.srd",
  };

  // Every text stays alive for the whole function: every string_view in
  // every tree points into one of these buffers.
  std::string texts[3];
  cursiv::ParseResult results[3];

  for (int i = 0; i < 3; ++i) {
    texts[i] = read_file(samples[i]);
    if (texts[i].empty()) return fail("a sample document is not readable");
    if (parse_clean(const_cast<Language&>(language), texts[i], samples[i],
                    results[i]) == nullptr)
      return fail("a sample failed the common shape check");
  }

  // header_and_types.srd: the header, the release clause, TABLE(...)'s two
  // attribute shapes, a nested attribute list, and the RETRIEVE/ARGUMENTS
  // escape handling.
  const auto& doc =
      *static_cast<const powerbuilderdw::ast::Document*>(results[1].value);
  if (doc.headerLines.size() != 2) return fail("header_and_types: header lines");
  if (!doc.release.has_value() || *doc.release != 12.0)
    return fail("header_and_types: RELEASE not read back as 12.0");
  if (doc.properties.size() != 4)
    return fail(
        "header_and_types: expected DATAWINDOW, TABLE, COLUMN, SIMPLETABLE");

  const auto* const table = static_cast<const powerbuilderdw::ast::TableProperty*>(
      doc.properties[1]);
  if (table->type != &cursiv::ast_type_tag<powerbuilderdw::ast::TableProperty>)
    return fail("header_and_types: second property is not TABLE(...)");
  if (table->items.size() != 3)
    return fail("header_and_types: TABLE(...) item count");

  const auto* const col0 =
      static_cast<const powerbuilderdw::ast::ColumnTypeAttr*>(table->items[0]);
  if (col0->type != &cursiv::ast_type_tag<powerbuilderdw::ast::ColumnTypeAttr>)
    return fail("header_and_types: first table item is not COLUMN=(TYPE=...)");
  if (col0->dataType != "CHAR" || col0->size != 30.0)
    return fail("header_and_types: COLUMN=(TYPE=CHAR(30)...) misread");

  const auto* const col1 =
      static_cast<const powerbuilderdw::ast::ColumnTypeAttr*>(table->items[1]);
  bool sawFilter = false;
  for (const powerbuilderdw::ast::Attr* const attr : col1->attrs) {
    if (attr->type != &cursiv::ast_type_tag<powerbuilderdw::ast::NestedAttr>)
      continue;
    const auto* const nested =
        static_cast<const powerbuilderdw::ast::NestedAttr*>(attr);
    if (nested->key == "FILTER" && nested->items.size() == 2) sawFilter = true;
  }
  if (!sawFilter) return fail("header_and_types: FILTER=(A=1 B=(...)) not read back");

  const auto* const retrieve =
      static_cast<const powerbuilderdw::ast::RetrieveAttr*>(table->items[2]);
  if (retrieve->type != &cursiv::ast_type_tag<powerbuilderdw::ast::RetrieveAttr>)
    return fail("header_and_types: third table item is not RETRIEVE=...");
  if (retrieve->retrieveText.find("\"X\"") == std::string_view::npos)
    return fail("header_and_types: RETRIEVE= tilde-escape not unescaped");
  if (retrieve->argsText != "\"ID\", INTEGER),(\"NAME\", STRING")
    return fail("header_and_types: ARGUMENTS= blob misread");

  std::puts("powerbuilderdw: the samples parse");
  return 0;
}
