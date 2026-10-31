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
  struct BareText : rule<as<std::string_view>((many(ref<BOM>) + ref<BARE>) |
                                              some(ref<BOM>))> {};
  struct VARIABLE : terminal<lit<"@["> + many(cr<"^]">) + lit<"]">> {};
  struct STRING
      : terminal<as<unquote>(
            lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">)> {
  };
  struct Op : terminal<some(cr<"=<>!?">)> {};

  struct Scalar
      : rule<flag<&ast::Scalar::quoted>(
                 assign<&ast::Scalar::text>(ref<STRING>)) |
             assign<&ast::Scalar::text>(ref<VARIABLE> | ref<BareText>)> {};

  struct KeyValue;
  struct Implicit;
  struct Group;
  struct ParameterDefinition;

  // TextTape reads the parameter name and first body scalar without quoting.
  // A boundary at the first byte is itself a one-byte scalar.
  struct PARAMETER_NAME : terminal<some(bareRest) | byte> {};
  struct PARAMETER_VALUE
      : terminal<!ref<TRIVIA> + (some(bareRest) | byte)> {};
  struct ParameterScalar
      : rule<assign<&ast::Scalar::text>(ref<PARAMETER_VALUE>)> {};

  static constexpr auto arrayItem =
      ref<KeyValue> | ref<Group> | ref<Scalar>;
  static constexpr auto arrayItems = many(append<&ast::Group::items>(arrayItem) + gap);
  static constexpr auto objectItems =
      many(append<&ast::Group::items>(
               ref<KeyValue> | ref<Implicit> | ref<ParameterDefinition>) + gap) +
      arrayItems;
  static constexpr auto groupBody =
      lit<"{"> + gap +
      option((append<&ast::Group::items>(
                  ref<KeyValue> | ref<ParameterDefinition>) + gap + objectItems) |
             (append<&ast::Group::items>(ref<Group> | ref<Scalar>) + gap +
              arrayItems)) +
      (flag<&ast::Group::closed>(lit<"}">) | eof);

  struct Group : rule<groupBody> {};
  struct HeaderGroup
      : rule<assign<&ast::Group::header>(ref<Scalar>) + gap + groupBody> {};
  struct Value
      : rule<ref<HeaderGroup> | ref<Group> | ref<Scalar>> {};
  struct KeyValue
      : rule<assign<&ast::KeyValue::key>(ref<Scalar>) + gap +
             assign<&ast::KeyValue::op>(ref<Op>) + gap +
             assign<&ast::KeyValue::value>(ref<Value>)> {};
  struct Implicit
      : rule<assign<&ast::KeyValue::key>(ref<Scalar>) + gap +
             assign<&ast::KeyValue::value>(ref<Group>)> {};

  struct ParameterKeyValue
      : rule<assign<&ast::KeyValue::key>(ref<ParameterScalar>) + gap +
             assign<&ast::KeyValue::op>(ref<Op>) + gap +
             assign<&ast::KeyValue::value>(ref<Value>)> {};
  struct ParameterImplicit
      : rule<assign<&ast::KeyValue::key>(ref<ParameterScalar>) + gap +
             assign<&ast::KeyValue::value>(ref<Group>)> {};
  struct ParameterGroup
      : rule<((append<&ast::Group::items>(
                   ref<ParameterKeyValue> | ref<ParameterImplicit>) + gap +
               objectItems) |
              (append<&ast::Group::items>(ref<ParameterScalar>) + gap +
               !lit<"]"> +
               arrayItems)) +
             flag<&ast::Group::closed>(lit<"]">)> {};
  struct ParameterDefinition
      : rule<lit<"[["> + option(flag<&ast::ParameterDefinition::undefined>(lit<"!">)) +
             assign<&ast::ParameterDefinition::name>(ref<PARAMETER_NAME>) +
             lit<"]"> + gap +
             ((assign<&ast::ParameterDefinition::value>(ref<ParameterScalar>) +
               gap + lit<"]">) |
              assign<&ast::ParameterDefinition::value>(ref<ParameterGroup>))> {};

  struct Doc
      : rule<option(flag<&ast::Document::bom>(ref<BOM>)) + gap +
             many(append<&ast::Document::items>(
                 ref<KeyValue> | ref<Implicit> | ref<Group> |
                 ref<ParameterDefinition> | ref<Scalar>) + gap)> {};

  using entry = Doc;
};

}  // namespace stellaris
