#include "cookie/core/CoreServices.hpp"
#include "cursiv/core/parser/Parser.hpp"
#pragma once
#include "cookie/core/Value.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include <array>
#include <cstdio>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace audit_regressions {
inline bool run(const cookie::CookieCoreServices& services) {
  struct Case {
    const char* source;
    std::vector<std::pair<std::string, std::optional<std::string>>> pairs;
  };
  const std::array cases = {
    Case{"a=b; c=d", {{"a","b"},{"c","d"}}},
    Case{"a+b=one+two; Path=/x+y; Secure", {{"a+b","one+two"},{"Path","/x+y"},{"Secure",std::nullopt}}},
    Case{"id=; HttpOnly", {{"id",""},{"HttpOnly",std::nullopt}}},
    Case{"id=\"hello; world\"; SameSite=Lax", {{"id","hello; world"},{"SameSite","Lax"}}},
    Case{"id=\"say\\\"hello\"; Path=/", {{"id","say\"hello"},{"Path","/"}}},
    Case{"id=\"a\\141b\"; Expires=Wed, 09 Jun 2027 10:18:14 GMT", {{"id","aab"},{"Expires","Wed, 09 Jun 2027 10:18:14 GMT"}}},
    Case{"a!#$%&'`|~:=v[]{}?@/+=", {{"a!#$%&'`|~:","v[]{}?@/+="}}},
    Case{"id=abc; Partitioned", {{"id","abc"},{"Partitioned",std::nullopt}}},
    Case{"Path=/", {{"Path","/"}}},
    Case{"Secure=token; HttpOnly", {{"Secure","token"},{"HttpOnly",std::nullopt}}},
    Case{"SameSite=Lax; Domain=example.com", {{"SameSite","Lax"},{"Domain","example.com"}}},
  };
  for (const auto& test : cases) {
    const std::string source(test.source);
    const auto result = services.parser->parse(source);
    const auto* doc = result.value_as<cookie::ast::Document>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !doc ||
        doc->pairs.size() != test.pairs.size())
      return false;
    if (!cursiv::test::validationHelper(services, source).diagnostics.empty())
      return false;
    for (std::size_t i = 0; i < test.pairs.size(); ++i) {
      const auto* pair = doc->pairs[i];
      if (pair->name != test.pairs[i].first ||
          cookie::decoded_value(*pair) != test.pairs[i].second ||
          pair->parent != doc)
        return false;
    }
  }
  for (const auto& test : std::array{
      std::pair{"na/me=1", "cookie.name"},
      std::pair{"id=x; Path", "cookie.value"},
      std::pair{"Secure", "cookie.attribute"},
      std::pair{R"(id="a\012b")", "cookie.control"}}) {
    const std::string source(test.first);
    const auto result = services.parser->parse(source);
    if (!result.accepted || result.cst().recovered())
      return false;
    const auto validation = cursiv::test::validationHelper(services, source);
    bool found = false;
    for (const auto& d : validation.diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == test.second;
    if (!found)
      return false;
  }
  return true;
}
}  // namespace audit_regressions
