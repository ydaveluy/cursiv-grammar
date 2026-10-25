// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check each tree -- and read
// a handful of field values back, since a converter that silently returns the
// wrong value would still pass a structural check.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "parkingsign/core/CoreServices.hpp"
#include "parkingsign/core/ast.hpp"

namespace {

using namespace parkingsign;

int fail(const std::string &path, const char *what) {
  std::fprintf(stderr, "parkingsign: %s: %s\n", path.c_str(), what);
  return 1;
}

template <class T> const T *as(const ast::Sign *sign) {
  if (sign == nullptr || sign->type != &cursiv::ast_type_tag<T>)
    return nullptr;
  return static_cast<const T *>(sign);
}

bool checkAmPm(const std::optional<ast::AmPm> &ampm, ast::AmPm want) {
  return ampm.has_value() && *ampm == want;
}

// Spot-checks against `example/example1.txt`, one per distinct sign kind plus
// the two permit variants: field values a structural check alone would miss.
int checkFields(const ast::Document &doc, const std::string &path) {
  if (doc.signs.size() != 27)
    return fail(path, "the document does not hold 27 signs");

  const auto *sweep0 = as<ast::StreetSweepingSign>(doc.signs[0]);
  if (sweep0 == nullptr || !sweep0->noParking || sweep0->day != ast::Day::Mon ||
      !sweep0->cleaning || sweep0->time->from->hour != "9" ||
      !checkAmPm(sweep0->time->from->ampm, ast::AmPm::Am) ||
      !sweep0->time->to->noon)
    return fail(path, "sign 0 (street sweeping) misreads");

  const auto *sweep2 = as<ast::StreetSweepingSign>(doc.signs[2]);
  if (sweep2 == nullptr || sweep2->noParking || sweep2->cleaning ||
      sweep2->day != ast::Day::Tue || sweep2->time->from->hour != "8" ||
      sweep2->time->to->hour != "10")
    return fail(path, "sign 2 (street sweeping, no prefix) misreads");

  const auto *noPark7 = as<ast::NoParkingSign>(doc.signs[7]);
  if (noPark7 == nullptr || !noPark7->noParking || !noPark7->anyTime ||
      !noPark7->except || noPark7->exceptDays != nullptr)
    return fail(path, "sign 7 (no parking, any time, except) misreads");

  const auto *stop8 = as<ast::NoStoppingSign>(doc.signs[8]);
  if (stop8 == nullptr || stop8->towAway || stop8->valetOnly ||
      stop8->times.size() != 2 || !stop8->except ||
      stop8->exceptDays == nullptr || !stop8->exceptDays->pair ||
      stop8->exceptDays->day1 != ast::Day::Sat ||
      stop8->exceptDays->day2 != ast::Day::Sun)
    return fail(path, "sign 8 (no stopping) misreads");

  const auto *stop10 = as<ast::NoStoppingSign>(doc.signs[10]);
  if (stop10 == nullptr || !stop10->towAway || !stop10->valetOnly ||
      stop10->times.size() != 1 || stop10->except ||
      stop10->exceptDays == nullptr || !stop10->exceptDays->nightly)
    return fail(path, "sign 10 (valet, tow-away, nightly) misreads");

  const auto *single12 = as<ast::SingleTimeLimitSign>(doc.signs[12]);
  if (single12 == nullptr || single12->amount != "4" || single12->minutes ||
      !single12->except || single12->exceptDays == nullptr ||
      single12->exceptDays->day1 != ast::Day::Sun)
    return fail(path, "sign 12 (4 hour parking) misreads");

  const auto *single16 = as<ast::SingleTimeLimitSign>(doc.signs[16]);
  if (single16 == nullptr || single16->amount != "15" || !single16->minutes ||
      single16->exceptDays == nullptr || !single16->exceptDays->school)
    return fail(path, "sign 16 (15 min parking, school days) misreads");

  const auto *dbl18 = as<ast::DoubleTimeLimitSign>(doc.signs[18]);
  if (dbl18 == nullptr || dbl18->amount != "2" || !dbl18->days1->range ||
      dbl18->days1->day1 != ast::Day::Mon ||
      dbl18->days1->day2 != ast::Day::Sat ||
      dbl18->days2->day1 != ast::Day::Sun || dbl18->t1->hour != "8" ||
      dbl18->t2->hour != "11" || dbl18->t3->hour != "11" ||
      dbl18->t4->hour != "8" || dbl18->exceptHolidays)
    return fail(path, "sign 18 (double time limit) misreads");

  const auto *dbl19 = as<ast::DoubleTimeLimitSign>(doc.signs[19]);
  if (dbl19 == nullptr || !dbl19->exceptHolidays)
    return fail(path, "sign 19 (double time limit, except holidays) misreads");

  const auto *permit24 = as<ast::PermitSign>(doc.signs[24]);
  if (permit24 == nullptr || permit24->vehiclesWith ||
      permit24->districtNo != "78" || permit24->exempted)
    return fail(path, "sign 24 (permit, exempt) misreads");

  const auto *permit25 = as<ast::PermitSign>(doc.signs[25]);
  if (permit25 == nullptr || !permit25->vehiclesWith ||
      permit25->districtNo != "78" || !permit25->exempted)
    return fail(path, "sign 25 (permit, vehicles with, exempted) misreads");

  const auto *permit26 = as<ast::PermitSign>(doc.signs[26]);
  if (permit26 == nullptr || permit26->districtNo != "34")
    return fail(path, "sign 26 (permit, no space before district) misreads");

  return 0;
}

int auditFields(const cursiv::Parser &parser) {
  {
    const std::string source = "NOPARKING6PMTO8AMNIGHTLY";
    const auto result = parser.parse(source);
    const auto *root = result.value_as<ast::Document>();
    const auto *sign = root && root->signs.size() == 1
                           ? as<ast::NoParkingSign>(root->signs[0])
                           : nullptr;
    if (!result.accepted || result.cst().recovered() || !sign || !sign->time ||
        sign->time->from->hour != "6" || sign->time->to->hour != "8" ||
        !sign->exceptDays || !sign->exceptDays->nightly ||
        sign->exceptDays->day1 || sign->exceptDays->day2 ||
        sign->time->parent != sign || sign->parent != root)
      return fail("concatenated", "native sign fields/parents differ");
  }
  {
    const std::string source =
        "DISTRICT NO. 99999999999999999999999999999999999999999999999999 "
        "PERMITS EXEMPT";
    const auto result = parser.parse(source);
    const auto *root = result.value_as<ast::Document>();
    const auto *sign = root && root->signs.size() == 1
                           ? as<ast::PermitSign>(root->signs[0])
                           : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !sign ||
        sign->districtNo !=
            "99999999999999999999999999999999999999999999999999")
      return fail("integer", "source digits lost");
  }
  {
    const std::string source = "0002 HOUR PARKING 0008:0030AM TO 6PM";
    const auto result = parser.parse(source);
    const auto *root = result.value_as<ast::Document>();
    const auto *sign = root && root->signs.size() == 1
                           ? as<ast::SingleTimeLimitSign>(root->signs[0])
                           : nullptr;
    if (!result.accepted || result.cst().recovered() || !sign ||
        sign->amount != "0002" || !sign->time ||
        sign->time->from->hour != "0008" || sign->time->from->minute != "0030")
      return fail("leading-zero", "clock/duration lexemes differ");
  }
  return 0;
}
} // namespace

int main() {
  std::vector<std::filesystem::path> files;
  for (const auto &entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.path().extension() == ".parkingsign")
      files.push_back(entry.path());
  if (files.empty())
    return fail(CURSIV_EXAMPLE_DIR, "no example found");

  const auto language =
      cursiv::create_language<parkingsign::ParkingsignCoreServices,
                              parkingsign::ParkingsignSharedCoreServices,
                              parkingsign::install_core_module>();

  if (const auto rc = auditFields(*language.language.parser))
    return rc;
  int checked = 0;
  for (const std::filesystem::path &path : files) {
    std::ifstream file(path);
    if (!file)
      return fail(path.string(), "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered())
      return fail(path.string(), "does not parse");
    if (!result.diagnostics.empty())
      return fail(path.string(), "parses with diagnostics");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Document>)
      return fail(path.string(), "built no document");

    const auto &doc = *static_cast<const ast::Document *>(root);
    if (const int rc = checkFields(doc, path.string()); rc != 0)
      return rc;
    ++checked;
  }

  std::printf("parkingsign: %d sample(s) parse\n", checked);
  return 0;
}
