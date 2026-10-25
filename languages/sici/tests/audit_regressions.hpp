#pragma once

#include <cstdio>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "sici/core/CoreServices.hpp"
#include "sici/core/ast.hpp"

namespace audit_regressions {
inline bool run() {
  auto language = cursiv::create_language<sici::SiciCoreServices,
                                          sici::SiciSharedCoreServices,
                                          sici::install_core_module>();
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {
      std::fprintf(stderr, "sici regression: %s\n", message);
      ok = false;
    }
  };
  for (const std::string source :
       {"2434-561X(199412)45:10<737:TIODIM>2.3.TX;2-M",
        "0015-6914(19960101)157:1<>1.0.TX;2-V",
        "0277-786X()364<123:COIPDA>2.0.TX;2-S",
        "1234-5679(1996)<::INS-023456>3.0.CO;2-#",
        "0730-2312(199512)131:6:2<>1.0.TX;2-K",
        "0002-8231(199602)47:2<173:POPR:CCC-020173-04>3.0.TX;2-E",
        "0093-7673(19950910)+<>1.0.TX;2-R",
        "1064-6965(19950403)12:13<III.C.1:NAFS>2.0.CO;2-7"}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:sici", "sici", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(source);
    const auto& result = document.parseResult;
    const auto* s = cursiv::ast_cast<sici::ast::Sici>(result.value);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && s,
          "native syntax");
    if (!s) continue;
    check(!s->parent && s->item && s->item->parent == s && s->contribution &&
              s->contribution->parent == s && s->controls.size() == 1 &&
              s->controls[0]->parent == s,
          "segment parents");
    if (s->controls.size() != 1) continue;
    check(s->controls[0]->version == "2" &&
              s->controls[0]->check == source.substr(source.size() - 1) &&
              result.cst().text() == source,
          "version, check character and source");
    if (source.find("<::") != std::string::npos)
      check(!s->item->enumeration && s->contribution->location.empty() &&
                s->contribution->title.empty() &&
                s->contribution->localNumber == "INS-023456",
            "local identifier without location or title");
    if (source.find("CCC-") != std::string::npos)
      check(s->item->enumeration && s->item->enumeration->volume == "47" &&
                s->item->enumeration->issue == "2" &&
                s->contribution->location == "173" &&
                s->contribution->title == "POPR" &&
                s->contribution->localNumber == "CCC-020173-04",
            "three contribution fields");
    if (source.find("131:6:2") != std::string::npos)
      check(s->item->enumeration &&
                s->item->enumeration->additional.size() == 1 &&
                s->item->enumeration->additional[0] == "2",
            "compound enumeration");
    if (source.find(")+<") != std::string::npos)
      check(s->item->enumeration && s->item->enumeration->volume.empty() &&
                s->item->enumeration->supplement == "+",
            "supplement without volume");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    check(diagnostics.empty(), "valid segments have no runtime errors");
  }
  for (const std::string source :
       {"2434-5610(199412)45:10<737:TIODIM>2.3.TX;2-M",
        "0002-8231(199412)45:10<737:TIODIM>2.9.TX;2-M",
        "0002-8231(199412)45:10<737:TIODIM>2.3.QQ;2-M",
        "0002-8231(199412)45:10<737:TIODIM>2.3.TX;3-M",
        "0002-8231(199412)45:10<737:TIODIMS>2.3.TX;2-M"}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:sici-invalid", "sici", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(source);
    check(document.parseResult.accepted &&
              !document.parseResult.cst().recovered() &&
              document.parseResult.diagnostics.empty(),
          "semantic controls remain parseable");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    check(!diagnostics.empty(), "semantic controls have runtime diagnostics");
  }
  return ok;
}
}  // namespace audit_regressions
