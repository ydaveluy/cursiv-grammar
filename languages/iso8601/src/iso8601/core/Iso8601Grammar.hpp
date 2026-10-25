#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "iso8601/core/ast.hpp"

namespace iso8601 {
struct Iso8601 : cursiv::GrammarSpec {
  struct Newline : terminal<some(cr<"\r\n">)> {};
  struct TMark : terminal<cr<"Tt">> {};
  struct ZMark : terminal<cr<"Zz">> {};
  struct WMark : terminal<cr<"Ww">> {};
  struct PMark : terminal<cr<"Pp">> {};
  struct YMark : terminal<cr<"Yy">> {};
  struct MMark : terminal<cr<"Mm">> {};
  struct DMark : terminal<cr<"Dd">> {};
  struct HMark : terminal<cr<"Hh">> {};
  struct SMark : terminal<cr<"Ss">> {};
  struct RMark : terminal<cr<"Rr">> {};
  struct Digit : terminal<d> {};
  struct Fraction : terminal<cr<",."> + some(d)> {};
  struct Int_ : rule<some(ref<Digit>)> {};
  struct Dec : rule<some(ref<Digit>) + option(ref<Fraction>)> {};
  struct Int2 : rule<repeat<2>(ref<Digit>)> {};
  struct Int3 : rule<repeat<3>(ref<Digit>)> {};
  struct Int4 : rule<repeat<4>(ref<Digit>)> {};
  struct SInt2p : rule<cr<"+-"> + repeat<2, cursiv::kUnbounded>(ref<Digit>)> {};
  struct SInt4p : rule<cr<"+-"> + repeat<4, cursiv::kUnbounded>(ref<Digit>)> {};
  struct Dec2 : rule<repeat<2>(ref<Digit>) + option(ref<Fraction>)> {};

  static constexpr auto YearNum = ref<Int4> | ref<SInt4p>;
  static constexpr auto LocalTimeBody =
      (assign<&ast::TimeValue::hour>(ref<Int2>) +
       ((assign<&ast::TimeValue::minute>(ref<Int2>) +
         assign<&ast::TimeValue::second>(ref<Dec2>)) |
        (lit<":"> + assign<&ast::TimeValue::minute>(ref<Int2>) + lit<":"> +
         assign<&ast::TimeValue::second>(ref<Dec2>)))) |
      (assign<&ast::TimeValue::hour>(ref<Int2>) + option(lit<":">) +
       assign<&ast::TimeValue::minute>(ref<Dec2>)) |
      assign<&ast::TimeValue::hour>(ref<Dec2>);
  static constexpr auto TimeZoneBody =
      ((flag<&ast::TimeZoneValue::negative>(lit<"-">) |
        lit<"+">)+assign<&ast::TimeZoneValue::hour>(ref<Int2>) +
       option(option(lit<":">) +
              assign<&ast::TimeZoneValue::minute>(ref<Int2>))) |
      flag<&ast::TimeZoneValue::utc>(ref<ZMark>);
  static constexpr auto ZonedTimeBody =
      LocalTimeBody + option(assign<&ast::TimeValue::zone>(
                          create<ast::TimeZoneValue>(TimeZoneBody)));
  struct TimeRule
      : rule<create<ast::TimeValue>(option(ref<TMark>) + ZonedTimeBody)> {};

  static constexpr auto CalendarBody =
      assign<&ast::DateValue::year>(YearNum) +
      ((assign<&ast::DateValue::month>(ref<Int2>) +
        assign<&ast::DateValue::day>(ref<Int2>)) |
       (lit<"-"> + assign<&ast::DateValue::month>(ref<Int2>) + lit<"-"> +
        assign<&ast::DateValue::day>(ref<Int2>)));
  static constexpr auto OrdinalBody =
      assign<&ast::DateValue::year>(YearNum) + option(lit<"-">) +
      assign<&ast::DateValue::ordinalDay>(ref<Int3>);
  static constexpr auto WeekBody =
      assign<&ast::DateValue::year>(YearNum) +
      ((ref<WMark> + assign<&ast::DateValue::week>(ref<Int2>) +
        option(assign<&ast::DateValue::weekDay>(ref<Digit>))) |
       (lit<"-"> + ref<WMark> + assign<&ast::DateValue::week>(ref<Int2>) +
        option(lit<"-"> + assign<&ast::DateValue::weekDay>(ref<Digit>))));
  static constexpr auto YearMonthBody =
      assign<&ast::DateValue::year>(YearNum) + lit<"-"> +
      assign<&ast::DateValue::month>(ref<Int2>);
  static constexpr auto DateBody =
      CalendarBody | OrdinalBody | WeekBody | YearMonthBody |
      assign<&ast::DateValue::year>(YearNum) |
      assign<&ast::DateValue::century>(ref<SInt2p>);
  struct DateRule : rule<create<ast::DateValue>(DateBody)> {};
  static constexpr auto DateTimeBody =
      assign<&ast::DateTimeValue::date>(create<ast::DateValue>(DateBody)) +
      ref<TMark> +
      assign<&ast::DateTimeValue::time>(create<ast::TimeValue>(ZonedTimeBody));
  struct DateTimeRule : rule<create<ast::DateTimeValue>(DateTimeBody)> {};

  static constexpr auto ConciseDateBody =
      (assign<&ast::DateValue::month>(ref<Int2>) + option(lit<"-">) +
       assign<&ast::DateValue::day>(ref<Int2>)) |
      assign<&ast::DateValue::day>(ref<Int2>);
  static constexpr auto EndDateBody =
      CalendarBody | OrdinalBody | WeekBody | YearMonthBody | ConciseDateBody;
  static constexpr auto EndDateTimeBody =
      assign<&ast::DateTimeValue::date>(create<ast::DateValue>(EndDateBody)) +
      ref<TMark> +
      assign<&ast::DateTimeValue::time>(create<ast::TimeValue>(ZonedTimeBody));
  struct StartPointRule
      : rule<create<ast::Point>(assign<&ast::Point::value>(
            ref<DateTimeRule> | ref<DateRule> | ref<TimeRule>))> {};
  struct EndPointRule
      : rule<create<ast::Point>(assign<&ast::Point::value>(
            create<ast::DateTimeValue>(EndDateTimeBody) |
            create<ast::DateValue>(EndDateBody) | ref<TimeRule>))> {};

  static constexpr auto DurationPartsBody =
      option(assign<&ast::DurationParts::years>(ref<Dec>) + ref<YMark>) +
      option(assign<&ast::DurationParts::months>(ref<Dec>) + ref<MMark>) +
      option(assign<&ast::DurationParts::weeks>(ref<Dec>) + ref<WMark>) +
      option(assign<&ast::DurationParts::days>(ref<Dec>) + ref<DMark>) +
      option(
          flag<&ast::DurationParts::hasTime>(ref<TMark>) +
          option(assign<&ast::DurationParts::hours>(ref<Dec>) + ref<HMark>) +
          option(assign<&ast::DurationParts::minutes>(ref<Dec>) + ref<MMark>) +
          option(assign<&ast::DurationParts::seconds>(ref<Dec>) + ref<SMark>));
  struct IntervalRule
      : rule<ref<PMark> + assign<&ast::Interval::value>(
                              create<ast::DurationParts>(DurationPartsBody) |
                              ref<StartPointRule> | ref<EndPointRule>)> {};
  struct DurationRangeRule
      : rule<(assign<&ast::DurationRange::start>(ref<StartPointRule>) +
              lit<"/"> +
              assign<&ast::DurationRange::stop>(ref<IntervalRule> |
                                                ref<EndPointRule>)) |
             (assign<&ast::DurationRange::start>(ref<IntervalRule>) + lit<"/"> +
              assign<&ast::DurationRange::stop>(ref<StartPointRule>))> {};
  static constexpr auto DurationBody =
      assign<&ast::Duration::value>(ref<DurationRangeRule> | ref<IntervalRule>);
  struct DurationRule : rule<DurationBody> {};
  struct RecurringRule
      : rule<ref<RMark> + option(assign<&ast::Recurring::count>(ref<Int_>)) +
             lit<"/"> +
             assign<&ast::Recurring::body>(
                 create<ast::Duration>(DurationBody))> {};
  struct IsoRule : rule<create<ast::Document>(assign<&ast::Document::value>(
                            ref<DateTimeRule> | ref<DateRule> | ref<TimeRule> |
                            ref<DurationRule> | ref<RecurringRule>)) +
                        option(ref<Newline>) + eof> {};
  using entry = IsoRule;
};
} // namespace iso8601
