// The parkingsign language: its AST. One class per sign kind, sharing a
// common `Sign` base so `Document::signs` needs no union.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace parkingsign::ast {

enum class Day : std::uint8_t { Mon, Tue, Wed, Thu, Fri, Sat, Sun };
enum class AmPm : std::uint8_t { Am, Pm };

// A field carries its hour only when neither `noon` nor `midnight` is set.
struct TimeOfDay : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> hour;
  optional<string_view> minute;
  optional<AmPm> ampm;
  boolean noon = false;
  boolean midnight = false;
};

struct TimeRange : cursiv::Extends<cursiv::AstNode> {
  TimeOfDay *from;
  TimeOfDay *to;
};

// Every day-range shape -- daily/nightly, school days, holidays, a pair
// joined by AND, a pair joined by TO, a single day marked ONLY, or a bare
// day -- collapses into one shape: at most a pair of days and which
// relation, if any, joins them.
struct DayRange : cursiv::Extends<cursiv::AstNode> {
  boolean daily = false;
  boolean nightly = false;
  boolean school = false;
  boolean holidays = false;
  boolean only = false;  // day1 ONLY
  boolean pair = false;  // day1 AND/& day2
  boolean range = false; // day1 TO/-/THRU day2
  optional<Day> day1;
  optional<Day> day2;
};

struct Sign : cursiv::Extends<cursiv::AstNode> {};

struct StreetSweepingSign : cursiv::Extends<Sign> {
  boolean noParking = false;
  TimeRange *time;
  Day day;
  boolean cleaning = false; // else STREET SWEEPING
};

struct NoParkingSign : cursiv::Extends<Sign> {
  boolean noParking = false;
  boolean anyTime = false;
  TimeRange *time; // null when anyTime
  boolean except = false;
  DayRange *exceptDays;
};

struct NoStoppingSign : cursiv::Extends<Sign> {
  boolean towAway = false;
  boolean valetOnly = false; // else NO STOPPING
  vector<TimeRange *> times;
  boolean except = false;
  DayRange *exceptDays;
};

struct PassengerLoadingSign : cursiv::Extends<Sign> {
  boolean towAway = false;
  vector<TimeRange *> times;
  boolean except = false;
  DayRange *exceptDays;
};

struct TemporaryNoParkingSign : cursiv::Extends<Sign> {
  boolean anyTime = false;
  TimeRange *time; // null when anyTime
  boolean except = false;
  DayRange *exceptDays;
};

struct SingleTimeLimitSign : cursiv::Extends<Sign> {
  string_view amount;
  boolean minutes = false; // else HOUR
  TimeRange *time;
  boolean except = false;
  DayRange *exceptDays;
};

// The sign reads two day ranges, then four times around a repeated TO
// (`t1 t2 TO TO t3 t4`), kept in that order: which pair belongs to which
// day range is not spelled by the language.
struct DoubleTimeLimitSign : cursiv::Extends<Sign> {
  string_view amount;
  DayRange *days1;
  DayRange *days2;
  TimeOfDay *t1;
  TimeOfDay *t2;
  TimeOfDay *t3;
  TimeOfDay *t4;
  boolean exceptHolidays = false;
};

struct PermitSign : cursiv::Extends<Sign> {
  boolean vehiclesWith = false;
  string_view districtNo;
  boolean exempted = false; // else EXEMPT
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Sign *> signs;
};

} // namespace parkingsign::ast
