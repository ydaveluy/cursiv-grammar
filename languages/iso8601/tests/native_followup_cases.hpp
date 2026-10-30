#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "iso8601/core/CoreServices.hpp"
#include "iso8601/core/ast.hpp"

namespace iso8601_native_followup {
template<class Language> bool run(const Language& language) {
  struct Case { std::string_view source, error; };
  constexpr Case cases[] = {
    {"1985-04-12T23:20", ""},
    {"19850412T2320", ""},
    {"1985-04-12T2320", "iso8601.format"},
    {"19850412T23:20", "iso8601.format"},
    {"12:30+0130", "iso8601.format"},
    {"1985-04-12/19850413", "iso8601.format"},
    {"1985-102/103", ""},
    {"1985-W15-5/W16-1", ""},
    {"1985W155/W161", ""},
    {"1985/1986", ""},
    {"1985-04/05", ""},
    {"1985-04/13", "iso8601.month"},
    {"1985T23:20", "iso8601.datetime-date"},
    {"1985-04T23:20", "iso8601.datetime-date"},
    {"1985-W15T23:20", "iso8601.datetime-date"},
    {"P0000-12-30T24:60:60", ""},
    {"P0000-13-00T00:00:00", "iso8601.duration-date"},
    {"P0000-00-31T00:00:00", "iso8601.duration-date"},
    {"P0000-00-00T25:00:00", "iso8601.duration-clock"},
    {"P0000-00-00T00:61:00", "iso8601.duration-clock"},
    {"P0000-00-00T00:00:60.5", "iso8601.duration-clock"},
    {"P0000-00-00T00:00:00Z", "iso8601.duration-zone"},
    {"P0000-W01-1", "iso8601.duration-week-date"},
    {"P0000-00-00T00:00:00", ""},
    {"P99999Y13M31DT25H61M61S", ""},
  };
  for (const auto& c : cases) {
    const auto result = language.language.parser->parse(c.source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) {
      std::fprintf(stderr, "iso8601 native followup: unclean %.*s\n", int(c.source.size()), c.source.data());
      return false;
    }
    const auto validated = cursiv::test::validationHelper(language.language, std::string(c.source));
    if (validated.diagnostics.size() != (c.error.empty() ? 0u : 1u) ||
        (!c.error.empty() && validated.diagnostics[0].code != c.error)) {
      std::fprintf(stderr, "iso8601 native followup: validation %.*s\n", int(c.source.size()), c.source.data());
      return false;
    }
  }
  for (std::string_view source : {"1985-102/103", "1985-W15-5/W16-1", "1985-04/05", "1985/1986"}) {
    const auto result = language.language.parser->parse(source);
    bool endpoint = false;
    for (const auto* node : result.nodes)
      if (const auto* date = cursiv::ast_cast<iso8601::ast::DateValue>(node))
        endpoint |= (!date->year && (date->ordinalDay == "103" ||
                     (date->week == "16" && date->weekDay == "1") ||
                     (date->month == "05" && !date->day))) ||
                    (date->year == "1986" && !date->month && !date->day);
    if (!endpoint) {
      std::fprintf(stderr, "iso8601 native followup: concise ordinal/week endpoint fields lost\n");
      return false;
    }
  }
  const auto result = language.language.parser->parse("P0000-12-30T24:60:60");
  bool date = false, time = false;
  for (const auto* node : result.nodes) {
    if (const auto* d = cursiv::ast_cast<iso8601::ast::DateValue>(node))
      date |= d->year == "0000" && d->month == "12" && d->day == "30";
    if (const auto* t = cursiv::ast_cast<iso8601::ast::TimeValue>(node))
      time |= t->hour == "24" && t->minute == "60" && t->second == "60";
  }
  if (!date || !time) std::fprintf(stderr, "iso8601 native followup: raw alternative duration fields lost\n");
  return date && time;
}
} // namespace iso8601_native_followup
