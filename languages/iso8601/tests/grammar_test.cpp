// The grammar test of this language: every file of example/ parses clean and
// builds a Document, and a few real field values are read back to catch a
// converter that silently returns the wrong value.

#include <cstdio>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "iso8601/core/CoreServices.hpp"
#include "iso8601/core/ast.hpp"

namespace {

namespace ast = iso8601::ast;
namespace fs = std::filesystem;

const cursiv::Parser &engine() {
  static const auto language =
      cursiv::create_language<iso8601::Iso8601CoreServices,
                              iso8601::Iso8601SharedCoreServices,
                              iso8601::install_core_module>();
  return *language.language.parser;
}

void every_example_parses_clean() {
  std::vector<fs::path> files;
  for (const auto &entry : fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.path().extension() == ".iso8601")
      files.push_back(entry.path());
  CHECK(!files.empty());

  for (const fs::path &path : files) {
    const std::string text = cursiv::test::read_file(path.string());
    const cursiv::ParseResult r = engine().parse(text);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) {
      std::printf("FAIL %s: accepted=%d recovered=%d diagnostics=%zu\n",
                  path.filename().string().c_str(), r.accepted,
                  r.cst().recovered(), r.diagnostics.size());
      ++cursiv::test::failures;
      continue;
    }
    ast::Document *doc = r.value_as<ast::Document>();
    if (doc == nullptr) {
      std::printf("FAIL %s: no Document built\n",
                  path.filename().string().c_str());
      ++cursiv::test::failures;
    }
  }
}

ast::Document *parse_document(std::string_view text) {
  static std::deque<std::string> kept;
  static std::deque<cursiv::ParseResult> results;
  kept.emplace_back(text);
  results.emplace_back(engine().parse(kept.back()));
  const cursiv::ParseResult &r = results.back();
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  return r.value_as<ast::Document>();
}

void calendar_date_extended_reads_back() {
  ast::Document *doc = parse_document("1985-04-12");
  CHECK(doc != nullptr);
  if (doc == nullptr)
    return;
  auto *date = std::get_if<ast::DateValue *>(&doc->value);
  CHECK(date != nullptr && *date != nullptr);
  if (date == nullptr || *date == nullptr)
    return;
  CHECK((*date)->year.has_value() && *(*date)->year == "1985");
  CHECK((*date)->month.has_value() && *(*date)->month == "04");
  CHECK((*date)->day.has_value() && *(*date)->day == "12");
  CHECK(!(*date)->century.has_value());
}

void datetime_with_zone_reads_back() {
  ast::Document *doc = parse_document("1985-04-12T23:20:00");
  CHECK(doc != nullptr);
  if (doc == nullptr)
    return;
  auto *dt = std::get_if<ast::DateTimeValue *>(&doc->value);
  CHECK(dt != nullptr && *dt != nullptr);
  if (dt == nullptr || *dt == nullptr)
    return;
  ast::TimeValue *time = (*dt)->time;
  CHECK(time != nullptr);
  if (time == nullptr)
    return;
  CHECK(time->hour == "23");
  CHECK(time->minute.has_value() && *time->minute == "20");
  CHECK(time->second.has_value() && *time->second == "00");
  CHECK(time->zone == nullptr);
}

void time_zone_offset_reads_back() {
  ast::Document *doc = parse_document("15:27:46+01:00");
  CHECK(doc != nullptr);
  if (doc == nullptr)
    return;
  auto *time = std::get_if<ast::TimeValue *>(&doc->value);
  CHECK(time != nullptr && *time != nullptr);
  if (time == nullptr || *time == nullptr)
    return;
  CHECK((*time)->hour == "15");
  CHECK((*time)->minute.has_value() && *(*time)->minute == "27");
  CHECK((*time)->second.has_value() && *(*time)->second == "46");
  ast::TimeZoneValue *zone = (*time)->zone;
  CHECK(zone != nullptr);
  if (zone == nullptr)
    return;
  CHECK(!zone->negative && !zone->utc);
  CHECK(zone->hour.has_value() && *zone->hour == "01");
  CHECK(zone->minute.has_value() && *zone->minute == "00");
}

void duration_reads_back() {
  ast::Document *doc = parse_document("P2Y10M15DT10H20M30S");
  CHECK(doc != nullptr);
  if (doc == nullptr)
    return;
  auto *dur = std::get_if<ast::Duration *>(&doc->value);
  CHECK(dur != nullptr && *dur != nullptr);
  if (dur == nullptr || *dur == nullptr)
    return;
  auto *interval = std::get_if<ast::Interval *>(&(*dur)->value);
  CHECK(interval != nullptr && *interval != nullptr);
  if (interval == nullptr || *interval == nullptr)
    return;
  auto *parts = std::get_if<ast::DurationParts *>(&(*interval)->value);
  CHECK(parts != nullptr && *parts != nullptr);
  if (parts == nullptr || *parts == nullptr)
    return;
  CHECK((*parts)->years.has_value() && *(*parts)->years == "2");
  CHECK((*parts)->months.has_value() && *(*parts)->months == "10");
  CHECK((*parts)->days.has_value() && *(*parts)->days == "15");
  CHECK((*parts)->hours.has_value() && *(*parts)->hours == "10");
  CHECK((*parts)->minutes.has_value() && *(*parts)->minutes == "20");
  CHECK((*parts)->seconds.has_value() && *(*parts)->seconds == "30");
}

// Shapes no corpus file exercises: a bare two digits is an hour (the
// reference tries `time` before `date`), a century needs its expanded (signed)
// spelling, and a duration whose final fractional unit is minutes.
void bare_hour_and_expanded_century_read_back() {
  ast::Document *plain = parse_document("19");
  CHECK(plain != nullptr);
  if (plain != nullptr) {
    auto *time = std::get_if<ast::TimeValue *>(&plain->value);
    CHECK(time != nullptr && *time != nullptr);
    if (time != nullptr && *time != nullptr) {
      CHECK((*time)->hour == "19");
      CHECK(!(*time)->minute.has_value());
    }
  }

  ast::Document *expanded = parse_document("+019");
  CHECK(expanded != nullptr);
  if (expanded != nullptr) {
    auto *date = std::get_if<ast::DateValue *>(&expanded->value);
    CHECK(date != nullptr && *date != nullptr);
    if (date != nullptr && *date != nullptr)
      CHECK((*date)->century.has_value() && *(*date)->century == "+019");
  }
}

void duration_minutes_fraction_reads_back() {
  ast::Document *doc = parse_document("PT5,5M");
  CHECK(doc != nullptr);
  if (doc == nullptr)
    return;
  auto *dur = std::get_if<ast::Duration *>(&doc->value);
  CHECK(dur != nullptr && *dur != nullptr);
  if (dur == nullptr || *dur == nullptr)
    return;
  auto *interval = std::get_if<ast::Interval *>(&(*dur)->value);
  CHECK(interval != nullptr && *interval != nullptr);
  if (interval == nullptr || *interval == nullptr)
    return;
  auto *parts = std::get_if<ast::DurationParts *>(&(*interval)->value);
  CHECK(parts != nullptr && *parts != nullptr);
  if (parts == nullptr || *parts == nullptr)
    return;
  CHECK((*parts)->minutes.has_value() && *(*parts)->minutes == "5,5");
}

void recurring_reads_back() {
  ast::Document *doc = parse_document("R2/P1Y6M");
  CHECK(doc != nullptr);
  if (doc == nullptr)
    return;
  auto *rec = std::get_if<ast::Recurring *>(&doc->value);
  CHECK(rec != nullptr && *rec != nullptr);
  if (rec == nullptr || *rec == nullptr)
    return;
  CHECK((*rec)->count.has_value() && *(*rec)->count == "2");
  CHECK((*rec)->body != nullptr);
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<iso8601::Iso8601CoreServices,
                                iso8601::Iso8601SharedCoreServices,
                                iso8601::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "PT0.123456789012345678901234567890123456789S");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  every_example_parses_clean();
  calendar_date_extended_reads_back();
  datetime_with_zone_reads_back();
  time_zone_offset_reads_back();
  duration_reads_back();
  bare_hour_and_expanded_century_read_back();
  duration_minutes_fraction_reads_back();
  recurring_reads_back();
  return cursiv::test::report("iso8601_grammar_test");
}
