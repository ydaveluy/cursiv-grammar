#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "szf/core/ast.hpp"

namespace szf {
inline constexpr auto unbracket = [](std::string_view text) {
  return text.size() >= 2 ? text.substr(1, text.size() - 2)
                          : std::string_view{};
};
inline constexpr auto after_equals = [](std::string_view text) {
  return text.empty() ? std::string_view{} : text.substr(1);
};

struct Szf : cursiv::GrammarSpec {
  static constexpr auto identifier = cr<"a-zA-Z0-9_">;
  static constexpr auto raw_identifier = identifier | cr<"$">;
  static constexpr auto hws = some(cr<R"( \t)">);
  static constexpr auto comment = lit<"//"> | lit<"/*">;
  static constexpr auto define_start = lit<"#define", !identifier>;
  static constexpr auto include_start = lit<"#include", !identifier>;
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCKCOMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct NAME : terminal<some(identifier)> {};
  struct KEY : terminal<!(lit<"base64data"> + !raw_identifier) +
                        some(raw_identifier)> {};
  struct HEADER
      : terminal<as<unbracket>(lit<"<"> + many(cr<R"(^>\r\n)">) + lit<">">)> {};
  struct PATH
      : terminal<as<unbracket>(lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">)> {
  };
  static constexpr auto next_opcode =
      identifier + many(raw_identifier) + lit<"=">;
  static constexpr auto value_end =
      cr<R"(\r\n)"> | lit<"<"> | comment |
      (hws + (eof | cr<R"(\r\n)"> | comment | lit<"<"> | define_start |
              include_start | next_opcode));
  struct VALUE
      : terminal<as<after_equals>(through(lit<"=">, &(value_end | eof)))> {};
  struct BASE64
      : terminal<as<after_equals>(through(lit<"=">, &(cr<"<#"> | eof)))> {};
  struct DEFINITION_VALUE : terminal<some(!comment + cr<R"(^ \t\r\n)">)> {};
  struct UNKNOWN
      : terminal<lit<"#"> + !lit<"define", !identifier> +
                 !lit<"include", !identifier> + many(cr<R"(^\r\n)">)> {};
  struct Header : rule<assign<&ast::Header::name>(ref<HEADER>)> {};
  struct Opcode
      : rule<(assign<&ast::Opcode::key>(lit<"base64data">) + gap::none +
              assign<&ast::Opcode::value>(ref<BASE64>)) |
             (assign<&ast::Opcode::key>(ref<KEY>) + gap::none +
              assign<&ast::Opcode::value>(ref<VALUE>))> {};
  struct Definition
      : rule<define_start + gap::is(many(cr<R"( \t)">)) + lit<"$"> + gap::none +
             assign<&ast::Definition::name>(ref<NAME>) +
             option(gap::is(many(cr<R"( \t)">)) +
                    assign<&ast::Definition::value>(ref<DEFINITION_VALUE> |
                                                    lit<"base64data">))> {};
  struct Include : rule<include_start + gap::is(many(cr<R"( \t)">)) +
                            assign<&ast::Include::path>(ref<PATH>)> {};
  struct Unknown
      : rule<assign<&ast::UnknownDirective::text>(ref<UNKNOWN>)> {};
  struct Element
      : rule<ref<Header> | ref<Opcode> | ref<Definition> |
             ref<Include> | ref<Unknown>> {};
  struct File : rule<create<ast::File> +
                         many(append<&ast::File::elements>(ref<Element>))> {
  };
  using entry = File;
  using ignored = WS;
  using hidden = list<COMMENT, BLOCKCOMMENT>;
};
}  // namespace szf
