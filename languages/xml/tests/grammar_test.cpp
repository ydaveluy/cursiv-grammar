#include "audit_regressions.hpp"
// The smoke test of the language: every file under example/ and tests/
// parses clean and builds a Document whose root is an Element. The exit
// code is the verdict.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "xml/core/CoreServices.hpp"
#include "xml/core/ast.hpp"

namespace {

constexpr const char* kExamples[] = {
    "books.xml",
    "declaration.xml",
    "underscore.xml",
    "unicode-names.xml",
    "web.xml",
};

void fail(const char* file, const char* what, bool& ok) {
  std::fprintf(stderr, "xml: %s: %s\n", file, what);
  ok = false;
}

// declaration.xml's own `<?xml ...?>` names three attributes: this is the
// one place a Declaration node's fields are read, not just its presence.
void check_declaration(const xml::ast::Document& doc, bool& ok) {
  if (doc.declaration == nullptr) {
    fail("declaration.xml", "no declaration node", ok);
    return;
  }
  const cursiv::vector<xml::ast::Attribute*>& attrs =
      doc.declaration->attributes;
  if (attrs.size() != 3) {
    fail("declaration.xml", "declaration does not hold three attributes",
         ok);
    return;
  }
  if (attrs[0]->name != "version" || attrs[0]->value != "1.0" ||
      attrs[1]->name != "encoding" || attrs[1]->value != "UTF-8" ||
      attrs[2]->name != "standalone" || attrs[2]->value != "yes")
    fail("declaration.xml", "declaration attributes read back wrong", ok);
}

// unicode-names.xml: element and attribute names outside ASCII -- U+66F8
// U+7C4D as the root, and an attribute name carrying the middle dot
// (U+00B7), which NameChar admits and NameStartChar does not.
void check_unicode_names(const xml::ast::Document& doc, bool& ok) {
  if (doc.root == nullptr) return;  // already failed above
  if (doc.root->name != "書籍") {
    fail("unicode-names.xml", "root element name read back wrong", ok);
    return;
  }
  if (doc.root->attributes.size() != 2 ||
      doc.root->attributes[1]->name != "版·号")
    fail("unicode-names.xml", "attribute name read back wrong", ok);
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      xml::XmlCoreServices, xml::XmlSharedCoreServices,
      xml::install_core_module>();

  bool ok = true;
  for (const char* name : kExamples) {
    const bool authored = std::string_view(name) == "declaration.xml" ||
                          std::string_view(name) == "unicode-names.xml";
    const std::string path = authored
        ? std::string(CURSIV_TEST_DIR) + "/" + name
        : std::string(CURSIV_EXAMPLE_DIR) + "/example/" + name;
    std::ifstream file(path);
    if (!file) {
      fail(name, "the sample document is not readable", ok);
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(name, "does not parse clean", ok);
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<xml::ast::Document>) {
      fail(name, "did not build a Document", ok);
      continue;
    }

    const auto& doc = *static_cast<const xml::ast::Document*>(root);
    if (doc.root == nullptr) {
      fail(name, "the document has no root element", ok);
      continue;
    }

    if (std::string_view(name) == "declaration.xml")
      check_declaration(doc, ok);
    if (std::string_view(name) == "unicode-names.xml")
      check_unicode_names(doc, ok);
  }

  if (ok) std::puts("xml: every example parses");
  return ok ? 0 : 1;
}
