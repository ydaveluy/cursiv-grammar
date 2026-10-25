// The smoke test of the language: the one upstream corpus document parses
// clean and its escapes/params read back correctly; coverage.ics (ours, not
// upstream) exercises every component kind, folding, lists, periods,
// durations and a trigger, since one corpus file cannot.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "icalendar/core/CoreServices.hpp"
#include "icalendar/core/ast.hpp"
#include "icalendar/core/Parameters.hpp"
#include "native_regressions.hpp"

namespace {

namespace ast = icalendar::ast;

int fail(const std::string& what) {
  std::fprintf(stderr, "icalendar: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

bool ieq(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (std::tolower(static_cast<unsigned char>(a[i])) !=
        std::tolower(static_cast<unsigned char>(b[i])))
      return false;
  return true;
}

const ast::Property* find_property(const cursiv::vector<ast::Property*>& props,
                                   std::string_view name) {
  for (const ast::Property* const p : props)
    if (p != nullptr && ieq(p->name, name)) return p;
  return nullptr;
}

const ast::Component* find_component(const cursiv::vector<ast::Component*>& comps,
                                     std::string_view name) {
  for (const ast::Component* const c : comps)
    if (c != nullptr && ieq(c->name, name)) return c;
  return nullptr;
}

}  // namespace

int main() {
  {
    const auto language = cursiv::create_language<icalendar::IcalendarCoreServices,
        icalendar::IcalendarSharedCoreServices, icalendar::install_core_module>();
    const auto invalid = cursiv::test::validationHelper(language.language,
        "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:x\r\nBEGIN:X-A\r\nEND:X-B\r\nEND:VCALENDAR\r\n");
    if (invalid.diagnostics.size() != 1 || invalid.diagnostics[0].message.find("different component") == std::string::npos) return 1;
  }

  {
    const auto audit_language = cursiv::create_language<
      icalendar::IcalendarCoreServices, icalendar::IcalendarSharedCoreServices,
      icalendar::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//test//EN\r\nBEGIN:X-FOO\r\nEND:X-BAR\r\nEND:VCALENDAR\r\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool found = false;
    for (const auto* node : audit_result.nodes)
      if (const auto* c = cursiv::ast_cast<icalendar::ast::Component>(node); c && c->name == "X-FOO") found = c->closeName == "X-BAR";
    if (!found) return 1;
  }

  const auto language = cursiv::create_language<
      icalendar::IcalendarCoreServices, icalendar::IcalendarSharedCoreServices,
      icalendar::install_core_module>();

  // -- the upstream corpus: example1.ics --
  const std::string corpusPath =
      std::string(CURSIV_EXAMPLE_DIR) + "/example1.ics";
  const std::string corpusText = read_file(corpusPath);
  if (corpusText.empty()) return fail("example1.ics: empty or unreadable");

  const cursiv::ParseResult corpus = language.language.parser->parse(corpusText);
  if (corpus.cst().recovered()) return fail("example1.ics: does not parse clean");
  if (!corpus.diagnostics.empty()) return fail("example1.ics: has diagnostics");

  const cursiv::AstNode* const corpusRoot = corpus.value;
  if (corpusRoot == nullptr ||
      corpusRoot->type != &cursiv::ast_type_tag<ast::Document>)
    return fail("example1.ics: built no Document");
  const auto& corpusDoc = *static_cast<const ast::Document*>(corpusRoot);
  if (corpusDoc.components.size() != 1)
    return fail("example1.ics: expected one VCALENDAR");
  const ast::Component& cal = *corpusDoc.components[0];
  if (cal.subcomponents.size() != 1)
    return fail("example1.ics: expected one component");

  const ast::Property* prodid = find_property(cal.properties, "PRODID");
  if (prodid == nullptr || prodid->value != "-//xyz Corp//NONSGML PDA Calendar Version 1.0//EN")
    return fail("example1.ics: wrong PRODID value");

  const ast::Component& event = *cal.subcomponents[0];
  if (!ieq(event.name, "VEVENT")) return fail("example1.ics: expected a VEVENT");

  const ast::Property* summary = find_property(event.properties, "SUMMARY");
  if (summary == nullptr || summary->value != "Networld+Interop Conference")
    return fail("example1.ics: wrong SUMMARY value");

  // DESCRIPTION folds across three physical lines; the fold bytes (CRLF plus
  // one leading WSP) must vanish from the joined value, escapes stay raw.
  const ast::Property* description = find_property(event.properties, "DESCRIPTION");
  if (description == nullptr) return fail("example1.ics: no DESCRIPTION");
  const std::string_view descValue = description->value;
  if (descValue.find("\r\n") != std::string_view::npos ||
      descValue.find('\n') != std::string_view::npos)
    return fail("example1.ics: DESCRIPTION keeps a fold artifact");
  if (descValue !=
      "Networld+Interop Conference and Exhibit\\nAtlanta World Congress "
      "Center\\nAtlanta\\, Georgia")
    return fail("example1.ics: DESCRIPTION did not unfold correctly");

  const ast::Property* attach = find_property(event.properties, "ATTACH");
  if (attach == nullptr || attach->params.size() != 3)
    return fail("example1.ics: ATTACH should carry three parameters");
  bool sawEncoding = false;
  for (const ast::Param* const p : attach->params)
    if (p != nullptr && ieq(p->name, "ENCODING")) {
      sawEncoding = true;
      if (icalendar::parameter_values(p->rawValue) != std::vector<std::string>{"BASE64"})
        return fail("example1.ics: ENCODING param has the wrong value");
    }
  if (!sawEncoding) return fail("example1.ics: ATTACH missing ENCODING param");

  // -- coverage.ics (ours): every component kind, folding, lists, periods --
  const std::string covText =
      read_file(std::string(CURSIV_TEST_DIR) + "/coverage.ics");
  if (covText.empty()) return fail("coverage.ics: empty or unreadable");

  const cursiv::ParseResult cov = language.language.parser->parse(covText);
  if (cov.cst().recovered()) return fail("coverage.ics: does not parse clean");
  if (!cov.diagnostics.empty()) return fail("coverage.ics: has diagnostics");

  const auto& covDoc = *static_cast<const ast::Document*>(cov.value);
  if (covDoc.components.size() != 1) return fail("coverage.ics: expected one calendar");
  const ast::Component& covCal = *covDoc.components[0];
  if (covCal.subcomponents.size() != 5)
    return fail("coverage.ics: expected VTIMEZONE/VEVENT/VTODO/VFREEBUSY/VJOURNAL");

  const ast::Component* tz = find_component(covCal.subcomponents, "VTIMEZONE");
  if (tz == nullptr || tz->subcomponents.size() != 2)
    return fail("coverage.ics: VTIMEZONE should hold STANDARD and DAYLIGHT");
  if (!find_component(tz->subcomponents, "STANDARD") ||
      !find_component(tz->subcomponents, "DAYLIGHT"))
    return fail("coverage.ics: VTIMEZONE subcomponents misnamed");

  const ast::Component* ev = find_component(covCal.subcomponents, "VEVENT");
  if (ev == nullptr || ev->subcomponents.size() != 1 ||
      !ieq(ev->subcomponents[0]->name, "VALARM"))
    return fail("coverage.ics: VEVENT should hold one VALARM");

  // The folded SUMMARY: two leading bytes on the continuation line are
  // "one WSP the fold consumes, one WSP that is real content" -- the joined
  // value keeps exactly one space at the fold point.
  const ast::Property* covSummary = find_property(ev->properties, "SUMMARY");
  if (covSummary == nullptr ||
      covSummary->value !=
          "Long description folded across two physical lines to test line "
          "unfolding")
    return fail("coverage.ics: SUMMARY did not unfold correctly");

  const ast::Property* categories = find_property(ev->properties, "CATEGORIES");
  if (categories == nullptr || categories->value != "WORK,MEETING,IMPORTANT")
    return fail("coverage.ics: wrong CATEGORIES value");

  const ast::Property* geo = find_property(ev->properties, "GEO");
  if (geo == nullptr) return fail("coverage.ics: no GEO");
  {
    const std::string geoText(geo->value);
    const std::size_t scol = geoText.find(';');
    if (scol == std::string::npos)
      return fail("coverage.ics: GEO value has no separator");
    const double lat = std::stod(geoText.substr(0, scol));
    const double lon = std::stod(geoText.substr(scol + 1));
    if (lat < 37.3 || lat > 37.4 || lon > -122.0 || lon < -122.2)
      return fail("coverage.ics: GEO value parsed to the wrong numbers");
  }

  const ast::Component* fb = find_component(covCal.subcomponents, "VFREEBUSY");
  if (fb == nullptr) return fail("coverage.ics: no VFREEBUSY");
  const ast::Property* freebusy = find_property(fb->properties, "FREEBUSY");
  if (freebusy == nullptr ||
      freebusy->value !=
          "20240101T090000Z/20240101T100000Z,20240101T140000Z/PT1H")
    return fail("coverage.ics: wrong FREEBUSY value");

  const ast::Component* todo = find_component(covCal.subcomponents, "VTODO");
  if (todo == nullptr) return fail("coverage.ics: no VTODO");
  const ast::Property* due = find_property(todo->properties, "DUE");
  if (due == nullptr || due->value != "20240201" || due->params.size() != 1)
    return fail("coverage.ics: wrong DUE value/params");

  const ast::Component* jr = find_component(covCal.subcomponents, "VJOURNAL");
  if (jr == nullptr) return fail("coverage.ics: no VJOURNAL");

  const ast::Property* attendee = find_property(ev->properties, "ATTENDEE");
  if (attendee == nullptr || attendee->params.size() != 3)
    return fail("coverage.ics: ATTENDEE should carry three parameters");

  // A calendar missing VERSION/PRODID, and an event missing DTSTAMP/UID, are
  // syntactically valid but not legal RFC 5545 -- the parser alone would not
  // see it, so this runs the validation pass.
  const cursiv::test::ValidationResult noVersion = cursiv::test::validationHelper(
      language.language,
      "BEGIN:VCALENDAR\r\nPRODID:-//t//t//EN\r\nBEGIN:VEVENT\r\n"
      "UID:x@example.com\r\nDTSTAMP:20240101T000000Z\r\n"
      "END:VEVENT\r\nEND:VCALENDAR\r\n");
  if (noVersion.diagnostics.empty())
    return fail("<missing VERSION>: the validator did not object");

  const cursiv::test::ValidationResult noUid = cursiv::test::validationHelper(
      language.language,
      "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//t//t//EN\r\n"
      "BEGIN:VEVENT\r\nDTSTAMP:20240101T000000Z\r\n"
      "END:VEVENT\r\nEND:VCALENDAR\r\n");
  if (noUid.diagnostics.empty())
    return fail("<missing UID>: the validator did not object");

  if (const int code = native_regressions(language.language))
    return fail("native regression " + std::to_string(code));
  std::puts("icalendar: the corpus and coverage documents parse");
  return 0;
}
