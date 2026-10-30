#include "clf/core/validation/ClfValidator.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <memory>
#include <string_view>

#include "clf/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace clf {
namespace {
bool digits(std::string_view value) {
  if (value.empty()) return false;
  for (const char c : value)
    if (c < '0' || c > '9') return false;
  return true;
}
int number(std::string_view value) {
  int result = -1;
  if (!digits(value)) return -1;
  const auto conversion =
      std::from_chars(value.data(), value.data() + value.size(), result);
  return conversion.ec == std::errc{} ? result : -1;
}
bool integer(std::string_view value) {
  if (value == "-") return true;
  return digits(value) && (value.size() == 1 || value.front() != '0');
}
bool date(std::string_view text) {
  if (text.size() < 11 || text[2] != '/' || text[6] != '/') return false;
  static constexpr std::array<std::string_view, 12> months = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  unsigned month = 0;
  for (unsigned i = 0; i < months.size(); ++i)
    if (text.substr(3, 3) == months[i]) month = i + 1;
  const int year = number(text.substr(7)), day = number(text.substr(0, 2));
  return year >= 1 && year <= 9999 && day >= 1 &&
         std::chrono::year_month_day(std::chrono::year(year),
                                     std::chrono::month(month),
                                     std::chrono::day(unsigned(day)))
             .ok();
}
bool time(std::string_view text) {
  if (text.size() != 8 || text[2] != ':' || text[5] != ':') return false;
  const int hour = number(text.substr(0, 2)),
            minute = number(text.substr(3, 2)),
            second = number(text.substr(6, 2));
  return hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 &&
         second < 60;
}
bool timezone(std::string_view text) {
  if (text.size() != 5 || (text.front() != '+' && text.front() != '-'))
    return false;
  const int hour = number(text.substr(1, 2)),
            minute = number(text.substr(3, 2));
  return hour >= 0 && hour < 24 && minute >= 0 && minute < 60;
}
}  // namespace

void ClfValidator::checkFields(const ast::Line& line,
                               cursiv::ValidationAcceptor& accept) const {
  if (line.statuscode != "-" &&
      (line.statuscode.size() != 3 || !digits(line.statuscode)))
    accept.error(line, "Expected a three-digit status or '-'.", "clf.status")
        .property<&ast::Line::statuscode>();
  if (!integer(line.bytes))
    accept.error(line, "Expected a decimal byte count or '-'.", "clf.bytes")
        .property<&ast::Line::bytes>();
  if (!date(line.date))
    accept.error(line, "Invalid calendar date.", "clf.date")
        .property<&ast::Line::date>();
  if (!time(line.time))
    accept.error(line, "Invalid time of day.", "clf.time")
        .property<&ast::Line::time>();
  if (!timezone(line.tz))
    accept.error(line, "Invalid timezone offset.", "clf.timezone")
        .property<&ast::Line::tz>();
}

void register_validation_checks(ClfCoreServices& services) {
  using V = ClfValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkFields>(
          *services.validator)});
}

}  // namespace clf
