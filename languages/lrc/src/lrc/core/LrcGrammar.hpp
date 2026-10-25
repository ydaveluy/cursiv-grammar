#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lrc/core/ast.hpp"

namespace lrc {

struct Lrc : cursiv::GrammarSpec {
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct NUM : terminal<some(d)> {};
  struct SECOND : terminal<some(cr<R"(^.]\r\n)">)> {};
  struct FRACTION : terminal<some(cr<R"(^]\r\n)">)> {};
  struct KEY : terminal<cr<"A-Za-z"> + many(cr<R"(^:]\r\n)">)> {};
  struct META_VALUE : terminal<some(cr<R"(^]\r\n)">)> {};
  struct REST : terminal<some(cr<R"(^\r\n)">)> {};
  struct LineEnd : rule<ref<EOL> | eof> {};
  static constexpr auto TimeShape =
      lit<"["> + some(d) + lit<":"> + many(cr<R"(^]\r\n)">) + lit<"]">;
  static constexpr auto MetaShape =
      lit<"["> + ref<KEY> + lit<":"> + many(cr<R"(^]\r\n)">) + lit<"]">;
  static constexpr auto FollowingTimeShape =
      lit<"["> + some(d) + lit<":"> + option(cr<"0-5">) + d +
      option(lit<"."> + some(d)) + lit<"]">;
  struct LYRIC_TEXT : terminal<!FollowingTimeShape + some(cr<R"(^\r\n)">)> {};
  struct RAW_TEXT : terminal<!(TimeShape | MetaShape) + some(cr<R"(^\r\n)">)> {
  };

  struct FractionRule : rule<option(ref<FRACTION>)> {};
  struct TimestampRule
      : rule<create<ast::Timestamp>(
            lit<"["> + assign<&ast::Timestamp::minutes>(ref<NUM>) + lit<":"> +
            option(assign<&ast::Timestamp::seconds>(ref<SECOND>)) +
            option(lit<"."> +
                   assign<&ast::Timestamp::fraction>(ref<FractionRule>)) +
            lit<"]">)> {};
  struct LineRule
      : rule<create<ast::Line>(
            append<&ast::Line::timestamps>(ref<TimestampRule>) +
            many((&FollowingTimeShape) +
                 append<&ast::Line::timestamps>(ref<TimestampRule>)) +
            option(assign<&ast::Line::text>(ref<LYRIC_TEXT>)) + ref<LineEnd>)> {
  };
  struct MetadataRule
      : rule<create<ast::Metadata>(
            lit<"["> + assign<&ast::Metadata::key>(ref<KEY>) + lit<":"> +
            option(assign<&ast::Metadata::value>(ref<META_VALUE>)) + lit<"]"> +
            option(assign<&ast::Metadata::tail>(ref<REST>)) + ref<LineEnd>)> {};
  struct RawLineRule
      : rule<create<ast::RawLine>(
            (assign<&ast::RawLine::text>(ref<RAW_TEXT>) + ref<LineEnd>) |
            ref<EOL>)> {};
  struct DocumentRule
      : rule<create<ast::Document>(many(append<&ast::Document::lines>(
            ref<LineRule> | ref<MetadataRule> | ref<RawLineRule>)))> {};

  using entry = DocumentRule;
};

} // namespace lrc
