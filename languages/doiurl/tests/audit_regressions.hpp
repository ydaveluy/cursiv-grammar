#pragma once

#include <algorithm>
#include <cstdio>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreServices.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "doiurl/core/ast.hpp"

inline int doiurl_regressions(const cursiv::CoreServices& services) {
  struct Case {
    std::string_view source;
    std::vector<std::string_view> suffix;
    std::optional<std::string_view> query;
    std::optional<std::string_view> fragment;
  };
  int failures = 0;
  for (const Case& c : std::vector<Case>{
           {"doi:10.1000/a", {"a"}, {}, {}},
           {"doi:10.1000/a?", {"a"}, "", {}},
           {"doi:10.1000/a#", {"a"}, {}, ""},
           {"doi:10.1000/a?#", {"a"}, "", ""},
           {"doi:10.1000//a/", {"", "a", ""}, {}, {}},
           {"doi:10.1000/", {""}, {}, {}},
           {"DOI:10.1000/a/%2f?x/y?z#%2f/", {"a", "%2f"}, "x/y?z", "%2f/"}}) {
    const auto result = services.parser->parse(c.source);
    const auto* uri = cursiv::ast_cast<doiurl::ast::DoiUri>(result.value);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !uri || uri->prefix != "10.1000" ||
        !std::equal(uri->suffix.begin(), uri->suffix.end(), c.suffix.begin(),
                    c.suffix.end()) ||
        uri->query != c.query || uri->fragment != c.fragment || uri->parent ||
        result.nodes.size() != 1 || result.cst().text() != c.source) {
      std::fprintf(stderr, "doiurl regression: %.*s\n", int(c.source.size()),
                   c.source.data());
      ++failures;
    }
  }
  for (const auto text : {"doi :10.1000/a", "doi:10.1000 /a",
                          "doi:10.1000/a /b", "doi:10.1000/a? x"}) {
    const auto result = services.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) {
      std::fprintf(stderr, "doiurl: whitespace split a URI token sequence\n");
      ++failures;
    }
  }
  for (const auto text : {"doi:10.1000/a?\n", "doi:10.1000/a#\r\n",
                          "doi:10.1000/a?#\n"}) {
    const auto result = services.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) {
      std::fprintf(stderr, "doiurl: empty URI tail rejected file whitespace\n");
      ++failures;
    }
  }
  for (const auto& [text, code] :
       {std::pair{"doi:/a", "doi.prefix"},
        std::pair{"doi:10.1000/", "doi.suffix"}}) {
    const auto validation = cursiv::test::validationHelper(services, text);
    if (validation.diagnostics.size() != 1 ||
        !validation.diagnostics[0].code ||
        !validation.diagnostics[0].code->isString() ||
        validation.diagnostics[0].code->string() != code) {
      std::fprintf(stderr, "doiurl: empty DOI component lacks validation\n");
      ++failures;
    }
  }
  return failures;
}
