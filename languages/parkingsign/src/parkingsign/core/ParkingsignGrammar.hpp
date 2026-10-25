// The parkingsign language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "parkingsign/core/ast.hpp"

namespace parkingsign {

struct Parkingsign : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n.)">)> {};

  struct INT : terminal<some(d)> {};

  struct MON : terminal<lit<"MONDAY"> | lit<"MON">, value<ast::Day::Mon>> {};
  struct TUE : terminal<lit<"TUESDAY"> | lit<"TUE">, value<ast::Day::Tue>> {};
  struct WED : terminal<lit<"WEDNESDAY"> | lit<"WED">, value<ast::Day::Wed>> {};
  struct THU : terminal<lit<"THURSDAY"> | lit<"THU">, value<ast::Day::Thu>> {};
  struct FRI : terminal<lit<"FRIDAY"> | lit<"FRI">, value<ast::Day::Fri>> {};
  struct SAT : terminal<lit<"SATURDAY"> | lit<"SAT">, value<ast::Day::Sat>> {};
  struct SUN : terminal<lit<"SUNDAY"> | lit<"SUN">, value<ast::Day::Sun>> {};
  static constexpr auto DayChoice = ref<MON> | ref<TUE> | ref<WED> | ref<THU> |
                                    ref<FRI> | ref<SAT> | ref<SUN>;

  struct AM : terminal<lit<"AM"> | lit<"A.M.">, value<ast::AmPm::Am>> {};
  struct PM : terminal<lit<"PM"> | lit<"P.M.">, value<ast::AmPm::Pm>> {};

  struct NOON : terminal<lit<"NOON">> {};
  struct MIDNIGHT : terminal<lit<"MIDNIGHT">> {};

  struct DASH : terminal<lit<"-">> {};
  struct AMPERSAND : terminal<lit<"&">> {};

  static constexpr auto ToSep = lit<"TO"> | ref<DASH> | lit<"THRU">;
  static constexpr auto AndSep = lit<"AND"> | ref<AMPERSAND>;
  static constexpr auto TowAwayPhrase =
      lit<"TOW"> + option(ref<DASH>) + lit<"AWAY">;
  static constexpr auto NoParkingPhrase = lit<"NO"> + lit<"PARKING">;
  static constexpr auto AnyTimePhrase =
      lit<"ANYTIME"> | (lit<"ANY"> + lit<"TIME">);

  // A bare INT already reads as a TimeOfDay with no minute or am/pm
  // suffix, so a range pairing a bare INT with a full time (`10 TO 6PM`)
  // needs no rule of its own.
  struct TimeOfDayRule
      : rule<(assign<&ast::TimeOfDay::hour>(ref<INT>) +
              option(lit<":"> + assign<&ast::TimeOfDay::minute>(ref<INT>)) +
              option(assign<&ast::TimeOfDay::ampm>(ref<AM> | ref<PM>))) |
             flag<&ast::TimeOfDay::noon>(option(lit<"12">) + ref<NOON>) |
             flag<&ast::TimeOfDay::midnight>(option(lit<"12">) +
                                             ref<MIDNIGHT>)> {};

  struct TimeRangeRule
      : rule<assign<&ast::TimeRange::from>(ref<TimeOfDayRule>) + ToSep +
             assign<&ast::TimeRange::to>(ref<TimeOfDayRule>)> {};

  struct DayRangeRule
      : rule<flag<&ast::DayRange::daily>(lit<"DAILY">) |
             flag<&ast::DayRange::nightly>(lit<"NIGHTLY">) |
             flag<&ast::DayRange::school>(lit<"SCHOOL"> + lit<"DAYS">) |
             flag<&ast::DayRange::holidays>(lit<"HOLIDAYS">) |
             (assign<&ast::DayRange::day1>(DayChoice) +
              option(((flag<&ast::DayRange::pair>(AndSep) |
                       flag<&ast::DayRange::range>(ToSep)) +
                      assign<&ast::DayRange::day2>(DayChoice)) |
                     flag<&ast::DayRange::only>(lit<"ONLY">)))> {};

  struct StreetSweepingSignRule
      : rule<option(
                 flag<&ast::StreetSweepingSign::noParking>(NoParkingPhrase)) +
             assign<&ast::StreetSweepingSign::time>(ref<TimeRangeRule>) +
             assign<&ast::StreetSweepingSign::day>(DayChoice) + lit<"STREET"> +
             (lit<"SWEEPING"> |
              flag<&ast::StreetSweepingSign::cleaning>(lit<"CLEANING">))> {};

  struct NoParkingSignRule
      : rule<option(flag<&ast::NoParkingSign::noParking>(NoParkingPhrase)) +
             (flag<&ast::NoParkingSign::anyTime>(AnyTimePhrase) |
              assign<&ast::NoParkingSign::time>(ref<TimeRangeRule>)) +
             option(flag<&ast::NoParkingSign::except>(lit<"EXCEPT">)) +
             option(
                 assign<&ast::NoParkingSign::exceptDays>(ref<DayRangeRule>))> {
  };

  struct NoStoppingSignRule
      : rule<option(flag<&ast::NoStoppingSign::towAway>(TowAwayPhrase)) +
             ((lit<"NO"> + lit<"STOPPING">) |
              flag<&ast::NoStoppingSign::valetOnly>(
                  lit<"VALET"> + lit<"PARKING"> + lit<"ONLY">)) +
             some(append<&ast::NoStoppingSign::times>(ref<TimeRangeRule>)) +
             option(flag<&ast::NoStoppingSign::except>(lit<"EXCEPT">)) +
             option(
                 assign<&ast::NoStoppingSign::exceptDays>(ref<DayRangeRule>))> {
  };

  struct PassengerLoadingSignRule
      : rule<option(flag<&ast::PassengerLoadingSign::towAway>(TowAwayPhrase)) +
             lit<"PASSENGER"> + lit<"LOADING"> + lit<"ONLY"> +
             some(append<&ast::PassengerLoadingSign::times>(
                 ref<TimeRangeRule>)) +
             option(flag<&ast::PassengerLoadingSign::except>(lit<"EXCEPT">)) +
             option(assign<&ast::PassengerLoadingSign::exceptDays>(
                 ref<DayRangeRule>))> {};

  struct TemporaryNoParkingSignRule
      : rule<TowAwayPhrase + lit<"TEMPORARY"> + NoParkingPhrase +
             (flag<&ast::TemporaryNoParkingSign::anyTime>(AnyTimePhrase) |
              assign<&ast::TemporaryNoParkingSign::time>(ref<TimeRangeRule>)) +
             option(flag<&ast::TemporaryNoParkingSign::except>(lit<"EXCEPT">)) +
             option(assign<&ast::TemporaryNoParkingSign::exceptDays>(
                 ref<DayRangeRule>))> {};

  struct SingleTimeLimitSignRule
      : rule<assign<&ast::SingleTimeLimitSign::amount>(ref<INT>) +
             (lit<"HOUR"> | flag<&ast::SingleTimeLimitSign::minutes>(
                                lit<"MIN"> | lit<"MINUTE">)) +
             lit<"PARKING"> +
             assign<&ast::SingleTimeLimitSign::time>(ref<TimeRangeRule>) +
             option(flag<&ast::SingleTimeLimitSign::except>(lit<"EXCEPT">)) +
             option(assign<&ast::SingleTimeLimitSign::exceptDays>(
                 ref<DayRangeRule>))> {};

  struct DoubleTimeLimitSignRule
      : rule<assign<&ast::DoubleTimeLimitSign::amount>(ref<INT>) + lit<"HOUR"> +
             lit<"PARKING"> +
             assign<&ast::DoubleTimeLimitSign::days1>(ref<DayRangeRule>) +
             assign<&ast::DoubleTimeLimitSign::days2>(ref<DayRangeRule>) +
             assign<&ast::DoubleTimeLimitSign::t1>(ref<TimeOfDayRule>) +
             assign<&ast::DoubleTimeLimitSign::t2>(ref<TimeOfDayRule>) +
             lit<"TO"> + lit<"TO"> +
             assign<&ast::DoubleTimeLimitSign::t3>(ref<TimeOfDayRule>) +
             assign<&ast::DoubleTimeLimitSign::t4>(ref<TimeOfDayRule>) +
             option(flag<&ast::DoubleTimeLimitSign::exceptHolidays>(
                 lit<"EXCEPT"> + lit<"HOLIDAYS">))> {};

  struct PermitSignRule
      : rule<option(flag<&ast::PermitSign::vehiclesWith>(lit<"VEHICLES"> +
                                                         lit<"WITH">)) +
             lit<"DISTRICT"> + lit<"NO"> +
             assign<&ast::PermitSign::districtNo>(ref<INT>) + lit<"PERMITS"> +
             (lit<"EXEMPT"> |
              flag<&ast::PermitSign::exempted>(lit<"EXEMPTED">))> {};

  struct ParkingSignRule
      : rule<ref<StreetSweepingSignRule> | ref<NoParkingSignRule> |
             ref<NoStoppingSignRule> | ref<PassengerLoadingSignRule> |
             ref<SingleTimeLimitSignRule> | ref<DoubleTimeLimitSignRule> |
             ref<TemporaryNoParkingSignRule> | ref<PermitSignRule>> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::signs>(ref<ParkingSignRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
};

} // namespace parkingsign
