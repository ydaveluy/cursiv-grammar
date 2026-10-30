#pragma once
#include "abb/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace abb {
inline constexpr auto unquote = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text.size() < 3 || text.front() != '\'' || text.back() != '\'')
    return Result::err("Incomplete character literal.");
  return Result::ok(text.substr(1, text.size() - 2));
};
// RAPID doubles quotes and backslashes, and uses \HH for character codes.
// RobotWare 7 represents codes above ASCII as their UTF-8 characters.
inline cursiv::ConversionResult<std::string_view>
string_value(std::string_view text, std::pmr::memory_resource *memory) {
  using Result = cursiv::ConversionResult<std::string_view>;
  // Recovery can supply an incomplete token; do not assume the terminal's
  // delimiters or escape widths survived the repair.
  if (text.size() < 2 || text.front() != '"' || text.back() != '"')
    return Result::err("Incomplete string literal.");
  const auto inner = text.substr(1, text.size() - 2);
  if (inner.find_first_of("\"\\") == inner.npos)
    return Result::ok(inner);
  const auto room = cursiv::text_room(memory, inner.size());
  const auto hex = [](char c) -> int {
    return c >= '0' && c <= '9' ? c - '0'
           : c >= 'a' && c <= 'f' ? c - 'a' + 10
           : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                 : -1;
  };
  std::size_t size = 0;
  for (std::size_t i = 0; i < inner.size(); ++i) {
    const char c = inner[i];
    if (c == '"') {
      if (i + 1 == inner.size() || inner[i + 1] != '"')
        return Result::err("A string quote must be doubled.");
      room[size++] = c;
      ++i;
    } else if (c == '\\') {
      if (i + 1 == inner.size())
        return Result::err("Incomplete string escape.");
      if (inner[i + 1] == '\\') {
        room[size++] = c;
        ++i;
        continue;
      }
      if (i + 2 >= inner.size() || hex(inner[i + 1]) < 0 ||
          hex(inner[i + 2]) < 0)
        return Result::err("A character code needs two hexadecimal digits.");
      const auto value = hex(inner[i + 1]) * 16 + hex(inner[i + 2]);
      i += 2;
      if (value >= 0x80)
        room[size++] = char(0xc0 | (value >> 6));
      room[size++] = char(value < 0x80 ? value : 0x80 | (value & 0x3f));
    } else {
      room[size++] = c;
    }
  }
  return Result::ok({room.data(), size});
}
struct Abb : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LINE_COMMENT : terminal<lit<"!"> + many(cr<R"(^\n\r)">)> {};
  struct IDENTIFIER : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z0-9_">)> {};
  static constexpr auto control =
      ikw<"MODULE"> | ikw<"ENDMODULE"> | ikw<"PROC"> | ikw<"ENDPROC"> |
      ikw<"FUNC"> | ikw<"ENDFUNC"> | ikw<"TRAP"> | ikw<"ENDTRAP"> |
      ikw<"RECORD"> | ikw<"ENDRECORD"> | ikw<"VAR"> | ikw<"PERS"> |
      ikw<"CONST"> | ikw<"LOCAL"> | ikw<"TASK"> | ikw<"IF"> | ikw<"THEN"> |
      ikw<"ELSEIF"> | ikw<"ELSE"> | ikw<"ENDIF"> | ikw<"WHILE"> | ikw<"DO"> |
      ikw<"ENDWHILE"> | ikw<"FOR"> | ikw<"ENDFOR"> | ikw<"TEST"> | ikw<"CASE"> |
      ikw<"DEFAULT"> | ikw<"ENDTEST"> | ikw<"RETURN"> | ikw<"CONNECT"> |
      ikw<"ERROR"> | ikw<"UNDO"> | ikw<"BACKWARD">;
  struct CALL_NAME : terminal<!control + ref<IDENTIFIER>> {};
  struct BOOLLITERAL
      : terminal<as<[](std::string_view text) { return text.size() == 4; }>(
            ikw<"TRUE"> | ikw<"FALSE">)> {};
  static constexpr auto escape = lit<"\\"> + any;
  struct CHARLITERAL : terminal<as<unquote>(
                           lit<"'"> + (escape | cr<R"(^'\\\r\n)">)+lit<"'">)> {
  };
  struct STRINGLITERAL
      : terminal<as<string_value>(
            lit<"\""> +
            many(lit<"\"\""> | lit<"\\\\"> |
                 (lit<"\\"> + repeat<2>(cr<"0-9a-fA-F">)) |
                 cr<R"(^"\\\r\n)">) +
            lit<"\"">)> {};
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto sign = option(cr<"+-">);
  struct FLOATLITERAL
      : terminal<sign + ((some(d) + lit<"."> + many(d) + option(exponent)) |
                         (lit<"."> + some(d) + option(exponent)) |
                         (some(d) + exponent))> {};
  struct INTLITERAL
      : terminal<sign +
                 (some(d) |
                  (lit<"'"> + cr<"hH"> + some(cr<"0-9A-Fa-f">) + lit<"'">) |
                  (lit<"'"> + cr<"bB"> + some(cr<"01">) + lit<"'">))> {};
  struct Integer : rule<assign<&ast::Integer::text>(ref<INTLITERAL>)> {};
  struct Float : rule<assign<&ast::FloatLiteral::text>(ref<FLOATLITERAL>)> {
  };
  struct Reference : rule<assign<&ast::Reference::name>(ref<IDENTIFIER>)> {
  };
  struct Array;
  static constexpr auto primitive = ref<BOOLLITERAL> | ref<CHARLITERAL> |
                                    ref<STRINGLITERAL> | ref<Float> |
                                    ref<Integer>;
  static constexpr auto expression =
      ref<Array> | primitive | ref<Reference>;
  struct Array
      : rule<lit<"["> + many(append<&ast::Array::items>(expression), lit<",">) +
             lit<"]">> {};
  struct Wildcard : rule<assign<&ast::Reference::name>(lit<"*">)> {};
  struct Dimensions
      : rule<create<ast::Array>(
            lit<"{"> +
            many(append<&ast::Array::items>(expression | ref<Wildcard>),
                 lit<",">) +
            lit<"}">)> {};
  static constexpr auto init_kind =
      as<ast::InitKind::Const>(ikw<"CONST">) | as<ast::InitKind::Pers>(ikw<"PERS">) | as<ast::InitKind::Var>(ikw<"VAR">);
  struct Declaration
      : rule<option(flag<&ast::Declaration::local>(ikw<"LOCAL">) |
                    flag<&ast::Declaration::task>(ikw<"TASK">)) +
             assign<&ast::Declaration::kind>(init_kind) +
             assign<&ast::Declaration::type>(ref<IDENTIFIER>) +
             assign<&ast::Declaration::name>(ref<IDENTIFIER>) +
             option(assign<&ast::Declaration::dimensions>(ref<Dimensions>)) +
             option(lit<":="> + assign<&ast::Declaration::value>(expression)) +
             lit<";">> {};
  struct OptionalArgument
      : rule<flag<&ast::BindingArgument::isOptional>(lit<"\\">) +
             assign<&ast::BindingArgument::name>(ref<IDENTIFIER>) +
             option(lit<":="> +
                    assign<&ast::BindingArgument::value>(expression))> {};
  struct NamedArgument
      : rule<assign<&ast::BindingArgument::name>(ref<IDENTIFIER>) + lit<":="> +
             assign<&ast::BindingArgument::value>(expression)> {};
  static constexpr auto argument =
      ref<OptionalArgument> | ref<NamedArgument> | expression;
  struct Call
      : rule<assign<&ast::Call::name>(ref<CALL_NAME>) +
             option(append<&ast::Call::arguments>(argument)) +
             many((lit<","> + append<&ast::Call::arguments>(argument)) |
                  append<&ast::Call::arguments>(ref<OptionalArgument>)) +
             lit<";">> {};
  struct Statement;
  struct Assignment
      : rule<assign<&ast::Assignment::target>(ref<Reference>) + lit<":="> +
             assign<&ast::Assignment::value>(expression) + lit<";">> {};
  struct Return
      : rule<ikw<"RETURN"> + option(assign<&ast::Return::value>(expression)) +
             lit<";">> {};
  struct Branch
      : rule<assign<&ast::Branch::condition>(expression) + ikw<"THEN"> +
             many(append<&ast::Branch::statements>(ref<Statement>))> {};
  struct If
      : rule<ikw<"IF"> + append<&ast::If::branches>(ref<Branch>) +
             many(ikw<"ELSEIF"> + append<&ast::If::branches>(ref<Branch>)) +
             option(
                 flag<&ast::If::hasElse>(ikw<"ELSE">) +
                 many(append<&ast::If::elseStatements>(ref<Statement>))) +
             ikw<"ENDIF">> {};
  struct While
      : rule<ikw<"WHILE"> + assign<&ast::While::condition>(expression) +
             ikw<"DO"> +
             many(append<&ast::While::statements>(ref<Statement>)) +
             ikw<"ENDWHILE">> {};
  struct Statement : rule<ref<Assignment> | ref<Return> |
                              ref<If> | ref<While> | ref<Call>> {};
  struct Parameter
      : rule<option(flag<&ast::Parameter::isOptional>(lit<"\\">)) +
             option(assign<&ast::Parameter::mode>(ikw<"VAR"> | ikw<"PERS"> |
                                                  ikw<"INOUT">)) +
             ((assign<&ast::Parameter::type>(ref<IDENTIFIER>) +
               assign<&ast::Parameter::name>(ref<IDENTIFIER>)) |
              assign<&ast::Parameter::name>(ref<IDENTIFIER>)) +
             option(assign<&ast::Parameter::dimensions>(ref<Dimensions>))> {};
  static constexpr auto parameters = option(
      flag<&ast::Routine::parenthesized>(lit<"(">) +
      many(append<&ast::Routine::parameters>(ref<Parameter>), lit<",">) +
      lit<")">);
  static constexpr auto body =
      many(append<&ast::Routine::locals>(ref<Declaration>)) +
      many(append<&ast::Routine::statements>(ref<Statement>));
  struct Procedure
      : rule<create<ast::Procedure>(
            ikw<"PROC"> + assign<&ast::Routine::name>(ref<IDENTIFIER>) +
            parameters + body + ikw<"ENDPROC">)> {};
  struct Function
      : rule<create<ast::Function>(
            ikw<"FUNC"> + assign<&ast::Function::returnType>(ref<IDENTIFIER>) +
            assign<&ast::Routine::name>(ref<IDENTIFIER>) + parameters + body +
            ikw<"ENDFUNC">)> {};
  struct RecordField
      : rule<assign<&ast::RecordField::type>(ref<IDENTIFIER>) +
             assign<&ast::RecordField::name>(ref<IDENTIFIER>) + lit<";">> {};
  struct Record
      : rule<ikw<"RECORD"> + assign<&ast::Record::name>(ref<IDENTIFIER>) +
             many(append<&ast::Record::fields>(ref<RecordField>)) +
             ikw<"ENDRECORD">> {};
  struct Module
      : rule<ikw<"MODULE"> + assign<&ast::Module::name>(ref<IDENTIFIER>) +
             option(flag<&ast::Module::parenthesized>(lit<"(">) +
                    many(append<&ast::Module::options>(ref<IDENTIFIER>),
                         lit<",">) +
                    lit<")">) +
             many(append<&ast::Module::records>(ref<Record>) |
                  append<&ast::Module::declarations>(ref<Declaration>) |
                  append<&ast::Module::procedures>(ref<Procedure>) |
                  append<&ast::Module::functions>(ref<Function>)) +
             ikw<"ENDMODULE">> {};
  using entry = Module;
  using ignored = WS;
  using hidden = LINE_COMMENT;
};
} // namespace abb
