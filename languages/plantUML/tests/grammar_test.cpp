
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "plantuml/core/CoreServices.hpp"
#include "plantuml/core/ast.hpp"
// The smoke test of the language: every document under example/ parses
// clean -- no recovery, no diagnostics, and the root is a Document.

#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "plantuml: %s: %s\n", file, what);
  return 1;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<plantuml::PlantUMLCoreServices,
                              plantuml::PlantUMLSharedCoreServices,
                              plantuml::install_core_module>();
  {
    const auto r =
        language.language.parser->parse("@startuml\ninterface Foo\n@enduml");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  static constexpr const char* kFiles[] = {
      "x1.plantuml",          "x2.plantuml",  "x3.plantuml",
      "x4.plantuml",          "x5.plantuml",  "x6.plantuml",
      "x7.plantuml",          "constructs.plantuml",
      "no_connector.plantuml", "arrowhead.plantuml",
  };

  const auto language = cursiv::create_language<
      plantuml::PlantUMLCoreServices, plantuml::PlantUMLSharedCoreServices,
      plantuml::install_core_module>();

  for (const char* name : kFiles) {
    const bool authored = std::string_view(name) == "constructs.plantuml" ||
                          std::string_view(name) == "no_connector.plantuml" ||
                          std::string_view(name) == "arrowhead.plantuml";
    const std::string path = (authored ? std::string(CURSIV_TEST_DIR)
                                       : std::string(CURSIV_EXAMPLE_DIR)) +
                             "/" + name;
    std::ifstream file(path);
    if (!file) return fail(name, "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(name, "the document does not parse");
    if (!result.diagnostics.empty()) return fail(name, "the document carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<plantuml::ast::Document>)
      return fail(name, "the parse built no document");

    const auto& doc = *static_cast<const plantuml::ast::Document*>(root);
    if (doc.elements.empty()) return fail(name, "the document declares no element");

    if (std::string_view(name) == "no_connector.plantuml" ||
        std::string_view(name) == "arrowhead.plantuml") {
      if (doc.elements.size() != 1) return fail(name, "expected one element");
      const plantuml::ast::Element* const el = doc.elements[0];
      if (el == nullptr ||
          el->type != &cursiv::ast_type_tag<plantuml::ast::Association>)
        return fail(name, "expected an association");
      const auto& assoc = *static_cast<const plantuml::ast::Association*>(el);
      const std::string_view op =
          std::string_view(name) == "arrowhead.plantuml" ? "<" : "";
      if (assoc.left != "Left" || assoc.right != "Right" || assoc.op != op)
        return fail(name, "the association was misread");
    }
  }

  std::puts("plantuml: every example parses");
  return 0;
}
