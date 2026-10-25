#pragma once

#include <string>
#include <utility>
#include <vector>

#include "clf/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"

inline void clf_regressions() {
  namespace ast = clf::ast;
  const auto language =
      cursiv::create_language<clf::ClfCoreServices, clf::ClfSharedCoreServices,
                              clf::install_core_module>();
  const auto& services = language.language;
  {
    const std::string source =
        "::1 - alice smith [29/Feb/2024:23:59:59 +0060] "
        "\"GET /a\\\"b\\x00 HTTP/1.1\" 200 - \"\" \"x\\\\y\\t\"";
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<ast::Log>(result.value);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    CHECK(root && root->lines.size() == 1 && result.nodes.size() == 2);
    if (root && root->lines.size() == 1) {
      const auto* line = root->lines[0];
      std::string request = "GET /a\"b";
      request.push_back('\0');
      request += " HTTP/1.1";
      CHECK(line->host == "::1" && line->logname == "-" &&
            line->username == "alice smith");
      CHECK(line->request == request && line->date == "29/Feb/2024" &&
            line->time == "23:59:59" && line->tz == "+0060");
      CHECK(line->bytes == "-" && line->referer && line->referer->empty() &&
            line->useragent && *line->useragent == "x\\y\t" &&
            line->parent == root);
      const auto span = result.cst()[line->cstNodeId];
      CHECK(span.begin == 0 && span.end == source.size());
    }
  }
  {
    const std::string source = "   [10/Oct/2000:13:55:36 -0700] \"\" - 0";
    const auto result = services.parser->parse(source);
    const auto* root = cursiv::ast_cast<ast::Log>(result.value);
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    CHECK(root && root->lines.size() == 1);
    if (root && root->lines.size() == 1)
      CHECK(root->lines[0]->host.empty() && root->lines[0]->logname.empty() &&
            root->lines[0]->username.empty() &&
            root->lines[0]->request.empty() && !root->lines[0]->referer &&
            !root->lines[0]->useragent);
  }
  for (const auto [field, code] :
       {std::pair{"29/Feb/2023:13:55:36 -0700", "clf.date"},
        std::pair{"10/Oct/2000:24:55:36 -0700", "clf.time"},
        std::pair{"10/Oct/2000:13:55:36 +2400", "clf.timezone"}}) {
    const std::string source =
        std::string("host - - [") + field + "] \"GET /\" 200 1";
    cursiv::Document document(
        cursiv::TextDocument::create("memory:clf", "clf", 0, source));
    document.language = &services;
    document.parseResult = services.parser->parse(source);
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered() &&
          document.parseResult.diagnostics.empty());
    CHECK(diagnostics.size() == 1 && diagnostics[0].code &&
          *diagnostics[0].code == code);
  }
  {
    const std::string source =
        "host - - [10/Oct/2000:13:55:36 -0700] \"GET /\" 2xx 01";
    cursiv::Document document(
        cursiv::TextDocument::create("memory:clf", "clf", 0, source));
    document.language = &services;
    document.parseResult = services.parser->parse(source);
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered() &&
          document.parseResult.diagnostics.empty() && diagnostics.size() == 2);
    for (const auto& diagnostic : diagnostics) {
      CHECK(diagnostic.code && (*diagnostic.code == "clf.status" ||
                                *diagnostic.code == "clf.bytes"));
      if (diagnostic.code && *diagnostic.code == "clf.status")
        CHECK(diagnostic.range.begin == source.find("2xx") &&
              diagnostic.range.end == source.find("2xx") + 3);
    }
  }
}
