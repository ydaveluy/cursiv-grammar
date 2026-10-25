#include "audit_regressions.hpp"
#include "native_regressions.hpp"
// The grammar end to end: every document under example/ and tests/ parses
// clean (recovered() false, no diagnostics, the right root type), and a few
// real field values come back out of one document's AST.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "webidl/core/ast.hpp"
#include "webidl/core/CoreServices.hpp"
#include "webidl/generated/Webidl/Module.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "webidl: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      webidl::generated::Webidl::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const webidl::ast::Attribute* find_attribute(
    std::span<webidl::ast::Member* const> members, std::string_view name) {
  for (const webidl::ast::Member* const m : members) {
    if (m->type != &cursiv::ast_type_tag<webidl::ast::Attribute>) continue;
    const auto& attr = *static_cast<const webidl::ast::Attribute*>(m);
    if (attr.name == name) return &attr;
  }
  return nullptr;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<webidl::WebidlCoreServices,
      webidl::WebidlSharedCoreServices, webidl::install_core_module>();
  if (!native_regressions(language.language)) return 1;
  std::vector<fs::path> docs;
  // `coverage.webidl` lives beside this test, not in the corpus: it covers
  // the constructs the two shipped examples never reach.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".webidl")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path readback_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = engine().parse(text);
    if (!result.accepted) return fail(doc.string() + ": the parse was refused");
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<webidl::ast::Document>)
      return fail(doc.string() + ": the parse built no document");
    if (doc.filename() == "html_link_element.webidl") readback_doc = doc;
  }

  if (readback_doc.empty())
    return fail("html_link_element.webidl was not found among the examples");

  // Read real field values out of one document's AST: a converter or a
  // wrongly-shadowed member would still leave `recovered()` false and the
  // root type right, so only reading the fields back catches it.
  const std::string text = read_file(readback_doc);
  const cursiv::ParseResult result = engine().parse(text);
  const auto& document =
      *static_cast<const webidl::ast::Document*>(result.value);
  if (document.definitions.size() != 1)
    return fail("html_link_element.webidl: expected one definition");

  const webidl::ast::Definition* const def = document.definitions[0];
  if (def->type != &cursiv::ast_type_tag<webidl::ast::Interface>)
    return fail("html_link_element.webidl: expected an interface");
  const auto& iface = *static_cast<const webidl::ast::Interface*>(def);

  if (iface.name != "HTMLLinkElement" || iface.base != "HTMLElement" ||
      iface.partial_)
    return fail("html_link_element.webidl: wrong interface header");
  if (iface.attrs.size() != 2 || iface.attrs[0]->name != "Exposed" ||
      !iface.attrs[0]->rhs ||
      iface.attrs[0]->rhs->type != &cursiv::ast_type_tag<webidl::ast::IdentifierValue> ||
      static_cast<const webidl::ast::IdentifierValue*>(iface.attrs[0]->rhs)->text != "Window" ||
      iface.attrs[1]->name != "HTMLConstructor" ||
      iface.attrs[1]->rhs != nullptr)
    return fail("html_link_element.webidl: wrong interface extended attributes");
  if (iface.members.size() != 21)
    return fail("html_link_element.webidl: wrong member count");

  const webidl::ast::Attribute* const disabled =
      find_attribute(iface.members, "disabled");
  if (disabled == nullptr ||
      disabled->type->type != &cursiv::ast_type_tag<webidl::ast::PrimitiveType>)
    return fail("html_link_element.webidl: missing or mistyped 'disabled'");
  if (static_cast<const webidl::ast::PrimitiveType*>(disabled->type)->kind !=
      webidl::ast::Primitive::boolean_)
    return fail("html_link_element.webidl: 'disabled' is not boolean");
  if (disabled->attrs.size() != 2 || disabled->attrs[0]->name != "Reflect" ||
      disabled->attrs[1]->name != "Measure")
    return fail("html_link_element.webidl: wrong extended attributes on 'disabled'");

  const webidl::ast::Attribute* const href =
      find_attribute(iface.members, "href");
  if (href == nullptr ||
      href->type->type != &cursiv::ast_type_tag<webidl::ast::StringType>)
    return fail("html_link_element.webidl: missing or mistyped 'href'");
  if (static_cast<const webidl::ast::StringType*>(href->type)->name !=
      "USVString")
    return fail("html_link_element.webidl: 'href' is not a USVString");

  const webidl::ast::Attribute* const sheet =
      find_attribute(iface.members, "sheet");
  if (sheet == nullptr || !sheet->readonly_ || !sheet->type->nullable ||
      sheet->type->type != &cursiv::ast_type_tag<webidl::ast::NamedType>)
    return fail("html_link_element.webidl: 'sheet' is not a nullable named type");
  if (static_cast<const webidl::ast::NamedType*>(sheet->type)->name !=
      "StyleSheet")
    return fail("html_link_element.webidl: 'sheet' names the wrong type");

  std::printf("webidl: %zu documents parse clean\n", docs.size());
  return 0;
}
