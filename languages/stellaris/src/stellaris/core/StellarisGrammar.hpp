#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "stellaris/core/ast.hpp"

namespace stellaris {

inline constexpr auto unquote = [](std::string_view text) {
  return text.substr(1, text.size() - 2);
};

struct Stellaris : cursiv::GrammarSpec {
  struct TRIVIA
      : terminal<some(cr<R"( \t\f\v\r\n;)"> |
                      (lit<"#"> + many(cr<R"(^\n)">)))> {};
  static constexpr auto gap = option(ref<TRIVIA>);
  struct BOM : terminal<lit<"\xef\xbb\xbf">> {};
  static constexpr auto bareRest = cr<R"(^ \t\f\v\r\n{}\[\]<>!=#;)">;
  struct BARE
      : terminal<!ref<BOM> +
                 ((cr<R"(^ \t\f\v\r\n{}\[\]<>!=#;")"> + many(bareRest)) |
                  lit<"]">)> {};
  struct BareText : rule<(many(ref<BOM>) + ref<BARE>) | some(ref<BOM>),
                         std::string_view> {};
  struct VARIABLE : terminal<lit<"@["> + many(cr<"^]">) + lit<"]">> {};
  struct STRING
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">,
                 cursiv::text_conv<unquote>> {};
  struct Op : terminal<some(cr<"=<>!?">)> {};

  struct ScalarRule
      : rule<flag<&ast::Scalar::quoted>(
                 assign<&ast::Scalar::text>(ref<STRING>)) |
             assign<&ast::Scalar::text>(ref<VARIABLE> | ref<BareText>)> {};

  struct KeyValueRule;
  struct ImplicitRule;
  struct GroupRule;

  static constexpr auto arrayItem =
      ref<KeyValueRule> | ref<GroupRule> | ref<ScalarRule>;
  static constexpr auto arrayItems = many(append<&ast::Group::items>(arrayItem) + gap);
  static constexpr auto groupBody =
      lit<"{"> + gap +
      option((append<&ast::Group::items>(ref<KeyValueRule>) + gap +
              many(append<&ast::Group::items>(ref<KeyValueRule> | ref<ImplicitRule>) + gap) +
              arrayItems) |
             (append<&ast::Group::items>(ref<GroupRule> | ref<ScalarRule>) + gap +
              arrayItems)) +
      (flag<&ast::Group::closed>(lit<"}">) | eof);

  struct GroupRule : rule<groupBody> {};
  struct HeaderGroupRule
      : rule<assign<&ast::Group::header>(ref<ScalarRule>) + gap + groupBody> {};
  struct ValueRule
      : rule<ref<HeaderGroupRule> | ref<GroupRule> | ref<ScalarRule>> {};
  struct KeyValueRule
      : rule<assign<&ast::KeyValue::key>(ref<ScalarRule>) + gap +
             assign<&ast::KeyValue::op>(ref<Op>) + gap +
             assign<&ast::KeyValue::value>(ref<ValueRule>)> {};
  struct ImplicitRule
      : rule<assign<&ast::KeyValue::key>(ref<ScalarRule>) + gap +
             assign<&ast::KeyValue::value>(ref<GroupRule>)> {};

  struct DocRule
      : rule<option(flag<&ast::Document::bom>(ref<BOM>)) + gap +
             many(append<&ast::Document::items>(
                 ref<KeyValueRule> | ref<ImplicitRule> | ref<GroupRule> |
                 ref<ScalarRule>) + gap)> {};

  using entry = DocRule;
};

}  // namespace stellaris
