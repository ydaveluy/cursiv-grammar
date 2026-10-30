#pragma once
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"
#include "iri/core/CoreServices.hpp"
#include "iri/core/ast.hpp"

namespace audit_regressions {
inline void run() {
  const auto language =
      cursiv::create_language<iri::IriCoreServices, iri::IriSharedCoreServices,
                              iri::install_core_module>();
  for (const std::string source :
       {"x://[1:2:3:4:5:6:0.0.0.1]/?#", "x://[::1.2.3.4]/?#",
        "x://[1:2:3:4:5:6:000.0.0.1]/?#", "x://[::001.002.003.004]/?#",
        "x://[::256.0.0.1]/?#"}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:iri", "iri", 0, source));
    document.language = &language.language;
    document.parseResult =
        language.language.parser->parse(document.textDocument().getText());
    const auto& result = document.parseResult;
    CHECK(result.accepted && !result.cst().recovered() &&
          result.diagnostics.empty());
    const auto* root = cursiv::ast_cast<iri::ast::Iri>(result.value);
    CHECK(root && root->scheme == "x" && root->authority &&
          root->authority->parent == root && root->path == "/" &&
          root->has_query && root->query.empty() && root->has_fragment &&
          root->fragment.empty());
    if (root && root->authority)
      CHECK(root->authority->raw ==
            std::string_view(source).substr(4, source.size() - 7));
    CHECK(result.nodes.size() == 2);
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    if (source == "x://[1:2:3:4:5:6:0.0.0.1]/?#" ||
        source == "x://[::1.2.3.4]/?#")
      CHECK(diagnostics.empty());
    else {
      CHECK(diagnostics.size() == 1);
      if (diagnostics.size() == 1) {
        const auto& d = diagnostics[0];
        CHECK(d.code && d.code->string() == "iri.address");
        CHECK(root && root->authority &&
              source.substr(d.range.begin, d.range.end - d.range.begin) == root->authority->raw);
      }
    }
  }
}
}  // namespace audit_regressions
