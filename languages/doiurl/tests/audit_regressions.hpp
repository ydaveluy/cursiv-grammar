#pragma once

#include <algorithm>
#include <cstdio>
#include <optional>
#include <string_view>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreServices.hpp"
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
  return failures;
}
