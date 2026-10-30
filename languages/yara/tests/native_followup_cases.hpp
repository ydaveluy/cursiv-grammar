#pragma once

#include <cstdio>
#include <string_view>

namespace yara_followup {
template <class Language> bool run(const Language& language) {
  const auto& parser = *language.language.parser;
  constexpr std::string_view invalid[] = {
      "rule true {condition:true}", "rule rule {condition:true}",
      "rule uint32be {condition:true}", "rule int8be {condition:true}",
      "rule r {meta:true=1 condition:true}", "rule r:and {condition:true}",
      "rule r {condition:for any all in (1,2):(all>0)}",
      "rule r {meta:s=\"", "include \"",
  };
  for (const auto source : invalid) {
    const auto result = parser.parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty()) {
      std::fprintf(stderr, "yara: native-invalid name/string accepted: %.*s\n",
                   int(source.size()), source.data());
      return false;
    }
  }
  constexpr std::string_view valid[] = {
      "rule include {condition:true}", "rule then {condition:true}",
      "rule true1 {condition:true}", "rule uint32be_x {condition:true}",
      "private private rule r {condition:true}",
      "global global rule r {condition:true}",
      "rule r {strings:$true=\"a\" condition:$true}",
  };
  for (const auto source : valid) {
    const auto result = parser.parse(source);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !result.value) {
      std::fprintf(stderr, "yara: native-valid identifier rejected: %.*s\n",
                   int(source.size()), source.data());
      return false;
    }
  }
  return true;
}
}  // namespace yara_followup
