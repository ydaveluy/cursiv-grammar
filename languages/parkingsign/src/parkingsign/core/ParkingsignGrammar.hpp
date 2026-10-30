// The parkingsign language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "parkingsign/core/ast.hpp"

namespace parkingsign {

struct Parkingsign : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n.)">)> {};

  struct INT : terminal<some(d)> {};

  struct MON : terminal<as<ast::Day::Mon>(lit<"MONDAY"> | lit<"MON">)> {};
  struct TUE : terminal<as<ast::Day::Tue>(lit<"TUESDAY"> | lit<"TUE">)> {};
  struct WED : terminal<as<ast::Day::Wed>(lit<"WEDNESDAY"> | lit<"WED">)> {};
  struct THU : terminal<as<ast::Day::Thu>(lit<"THURSDAY"> | lit<"THU">)> {};
  struct FRI : terminal<as<ast::Day::Fri>(lit<"FRIDAY"> | lit<"FRI">)> {};
  struct SAT : terminal<as<ast::Day::Sat>(lit<"SATURDAY"> | lit<"SAT">)> {};
  struct SUN : terminal<as<ast::Day::Sun>(lit<"SUNDAY"> | lit<"SUN">)> {};
  static constexpr auto DayChoice = ref<MON> | ref<TUE> | ref<WED> | ref<THU> |
                                    ref<FRI> | ref<SAT> | ref<SUN>;

  struct AM : terminal<as<ast::AmPm::Am>(lit<"AM"> | lit<"A.M.">)> {};
  struct PM : terminal<as<ast::AmPm::Pm>(lit<"PM"> | lit<"P.M.">)> {};

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

  struct TimeRange
      : rule<assign<&ast::TimeRange::from>(ref<TimeOfDayRule>) + ToSep +
             assign<&ast::TimeRange::to>(ref<TimeOfDayRule>)> {};

  struct DayRange
      : rule<flag<&ast::DayRange::daily>(lit<"DAILY">) |
             flag<&ast::DayRange::nightly>(lit<"NIGHTLY">) |
             flag<&ast::DayRange::school>(lit<"SCHOOL"> + lit<"DAYS">) |
             flag<&ast::DayRange::holidays>(lit<"HOLIDAYS">) |
             (assign<&ast::DayRange::day1>(DayChoice) +
              option(((flag<&ast::DayRange::pair>(AndSep) |
                       flag<&ast::DayRange::range>(ToSep)) +
                      assign<&ast::DayRange::day2>(DayChoice)) |
                     flag<&ast::DayRange::only>(lit<"ONLY">)))> {};

  struct StreetSweepingSign
      : rule<option(
                 flag<&ast::StreetSweepingSign::noParking>(NoParkingPhrase)) +
             assign<&ast::StreetSweepingSign::time>(ref<TimeRange>) +
             assign<&ast::StreetSweepingSign::day>(DayChoice) + lit<"STREET"> +
             (lit<"SWEEPING"> |
              flag<&ast::StreetSweepingSign::cleaning>(lit<"CLEANING">))> {};

  struct NoParkingSign
      : rule<option(flag<&ast::NoParkingSign::noParking>(NoParkingPhrase)) +
             (flag<&ast::NoParkingSign::anyTime>(AnyTimePhrase) |
              assign<&ast::NoParkingSign::time>(ref<TimeRange>)) +
             option(flag<&ast::NoParkingSign::except>(lit<"EXCEPT">)) +
             option(
                 assign<&ast::NoParkingSign::exceptDays>(ref<DayRange>))> {
  };

  struct NoStoppingSign
      : rule<option(flag<&ast::NoStoppingSign::towAway>(TowAwayPhrase)) +
             ((lit<"NO"> + lit<"STOPPING">) |
              flag<&ast::NoStoppingSign::valetOnly>(
                  lit<"VALET"> + lit<"PARKING"> + lit<"ONLY">)) +
             some(append<&ast::NoStoppingSign::times>(ref<TimeRange>)) +
             option(flag<&ast::NoStoppingSign::except>(lit<"EXCEPT">)) +
             option(
                 assign<&ast::NoStoppingSign::exceptDays>(ref<DayRange>))> {
  };

  struct PassengerLoadingSign
      : rule<option(flag<&ast::PassengerLoadingSign::towAway>(TowAwayPhrase)) +
             lit<"PASSENGER"> + lit<"LOADING"> + lit<"ONLY"> +
             some(append<&ast::PassengerLoadingSign::times>(
                 ref<TimeRange>)) +
             option(flag<&ast::PassengerLoadingSign::except>(lit<"EXCEPT">)) +
             option(assign<&ast::PassengerLoadingSign::exceptDays>(
                 ref<DayRange>))> {};

  struct TemporaryNoParkingSign
      : rule<TowAwayPhrase + lit<"TEMPORARY"> + NoParkingPhrase +
             (flag<&ast::TemporaryNoParkingSign::anyTime>(AnyTimePhrase) |
              assign<&ast::TemporaryNoParkingSign::time>(ref<TimeRange>)) +
             option(flag<&ast::TemporaryNoParkingSign::except>(lit<"EXCEPT">)) +
             option(assign<&ast::TemporaryNoParkingSign::exceptDays>(
                 ref<DayRange>))> {};

  struct SingleTimeLimitSign
      : rule<assign<&ast::SingleTimeLimitSign::amount>(ref<INT>) +
             (lit<"HOUR"> | flag<&ast::SingleTimeLimitSign::minutes>(
                                lit<"MIN"> | lit<"MINUTE">)) +
             lit<"PARKING"> +
             assign<&ast::SingleTimeLimitSign::time>(ref<TimeRange>) +
             option(flag<&ast::SingleTimeLimitSign::except>(lit<"EXCEPT">)) +
             option(assign<&ast::SingleTimeLimitSign::exceptDays>(
                 ref<DayRange>))> {};

  struct DoubleTimeLimitSign
      : rule<assign<&ast::DoubleTimeLimitSign::amount>(ref<INT>) + lit<"HOUR"> +
             lit<"PARKING"> +
             assign<&ast::DoubleTimeLimitSign::days1>(ref<DayRange>) +
             assign<&ast::DoubleTimeLimitSign::days2>(ref<DayRange>) +
             assign<&ast::DoubleTimeLimitSign::t1>(ref<TimeOfDayRule>) +
             assign<&ast::DoubleTimeLimitSign::t2>(ref<TimeOfDayRule>) +
             lit<"TO"> + lit<"TO"> +
             assign<&ast::DoubleTimeLimitSign::t3>(ref<TimeOfDayRule>) +
             assign<&ast::DoubleTimeLimitSign::t4>(ref<TimeOfDayRule>) +
             option(flag<&ast::DoubleTimeLimitSign::exceptHolidays>(
                 lit<"EXCEPT"> + lit<"HOLIDAYS">))> {};

  struct PermitSign
      : rule<option(flag<&ast::PermitSign::vehiclesWith>(lit<"VEHICLES"> +
                                                         lit<"WITH">)) +
             lit<"DISTRICT"> + lit<"NO"> +
             assign<&ast::PermitSign::districtNo>(ref<INT>) + lit<"PERMITS"> +
             (lit<"EXEMPT"> |
              flag<&ast::PermitSign::exempted>(lit<"EXEMPTED">))> {};

  struct ParkingSign
      : rule<ref<StreetSweepingSign> | ref<NoParkingSign> |
             ref<NoStoppingSign> | ref<PassengerLoadingSign> |
             ref<SingleTimeLimitSign> | ref<DoubleTimeLimitSign> |
             ref<TemporaryNoParkingSign> | ref<PermitSign>> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::signs>(ref<ParkingSign>))> {};

  using entry = Document;
  using ignored = WS;
};

} // namespace parkingsign
