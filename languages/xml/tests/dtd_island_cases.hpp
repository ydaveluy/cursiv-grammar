#pragma once
#include <cstdio>
#include <string_view>
#include "cursiv/core/parser/Parser.hpp"
#include "xml/core/ast.hpp"
namespace xml_dtd_islands {
inline bool run(const cursiv::Parser& parser) {
  struct Case { const char* id; const char* source; bool valid; };
  const Case cases[] = {
    {"comment", "<!DOCTYPE a [<!-- plain -->]><a/>", true},
    {"comment-delimiters", "<!DOCTYPE a [<!-- ] > ' \"> ?> -->]><a/>", true},
    {"pi-delimiters", "<!DOCTYPE a [<?p ] > ' \"> ?>]><a/>", true},
    {"pi-no-data", "<!DOCTYPE a [<?p?>]><a/>", true},
    {"pi-prefix-not-reserved", "<!DOCTYPE a [<?xml-stylesheet?>]><a/>", true},
    {"quoted-islands", "<!DOCTYPE a [<!ENTITY x '<?xml?> <!-- a--b -->'>]><a/>", true},
    {"mixed-declarations", "<!DOCTYPE a [<!-- ok --><?p ] ?><!ELEMENT a EMPTY>]><a/>", true},
    {"comment-double-hyphen", "<!DOCTYPE a [<!--a--b-->]><a/>", false},
    {"comment-ending-hyphen", "<!DOCTYPE a [<!--a--->]><a/>", false},
    {"comment-unclosed", "<!DOCTYPE a [<!--a ]><a/>", false},
    {"pi-xml-reserved", "<!DOCTYPE a [<?xml?>]><a/>", false},
    {"pi-XML-reserved", "<!DOCTYPE a [<?XmL?>]><a/>", false},
    {"pi-empty", "<!DOCTYPE a [<?\?>]><a/>", false},
    {"pi-leading-space", "<!DOCTYPE a [<? p?>]><a/>", false},
    {"pi-bad-name", "<!DOCTYPE a [<?1?>]><a/>", false},
    {"pi-unclosed", "<!DOCTYPE a [<?p ]><a/>", false},
  };
  for (const auto& test : cases) {
    const auto result = parser.parse(test.source);
    const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "xml DTD island: %s expected=%d clean=%d: %s\n",
                   test.id, test.valid, clean, test.source);
      return false;
    }
    if (!test.valid) continue;
    const auto* document = cursiv::ast_cast<xml::ast::Document>(result.value);
    const std::string_view source(test.source);
    const auto rootAt = source.rfind("<a/>");
    if (!document || document->doctypes.size() != 1 ||
        document->doctypes[0] != source.substr(0, rootAt) ||
        !document->root || document->root->name != "a" ||
        !document->root->children.empty() || document->root->parent != document ||
        !result.cst().contains(document->cstNodeId, document->root->cstNodeId)) {
      std::fprintf(stderr, "xml DTD island: %s raw DTD/root/ownership lost\n", test.id);
      return false;
    }
  }
  return true;
}
}  // namespace xml_dtd_islands
