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
      ikw<"ERROR"> | ikw<"UNDO"> | ikw<"BACKWARD"> | ikw<"FROM"> | ikw<"TO"> |
      ikw<"STEP"> | ikw<"WITH"> | ikw<"GOTO"> | ikw<"BREAK"> | ikw<"CONTINUE"> |
      ikw<"EXIT"> | ikw<"RAISE"> | ikw<"RETRY"> | ikw<"TRYNEXT">;
  static constexpr auto operator_word = ikw<"AND"> | ikw<"OR"> | ikw<"XOR"> |
      ikw<"NOT"> | ikw<"DIV"> | ikw<"MOD"> | ikw<"TRUE"> | ikw<"FALSE">;
  struct CALL_NAME : terminal<!control + !operator_word + ref<IDENTIFIER>> {};
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
  struct FLOATLITERAL
      : terminal<((some(d) + lit<"."> + many(d) + option(exponent)) |
                         (lit<"."> + some(d) + option(exponent)) |
                         (some(d) + exponent)) + !w> {};
  struct INTLITERAL
      : terminal<(some(d) |
                  (lit<"'"> + cr<"hH"> + some(cr<"0-9A-Fa-f">) + lit<"'">) |
                  (lit<"'"> + cr<"bB"> + some(cr<"01">) + lit<"'">)) + !w> {};
  struct Integer : rule<assign<&ast::Integer::text>(ref<INTLITERAL>)> {};
  struct Float : rule<assign<&ast::FloatLiteral::text>(ref<FLOATLITERAL>)> {
  };
  struct Reference : rule<assign<&ast::Reference::name>(ref<CALL_NAME>)> {
  };
  struct Boolean : rule<assign<&ast::BoolLiteral::value>(ref<BOOLLITERAL>)> {};
  struct String : rule<assign<&ast::StringLiteral::value>(ref<STRINGLITERAL>)> {};
  struct Character
      : rule<assign<&ast::CharacterLiteral::value>(ref<CHARLITERAL>)> {};
  struct Expression;
  struct FunctionCall;
  struct Array
      : rule<lit<"["> + some(append<&ast::Array::items>(ref<Expression>), lit<",">) +
             lit<"]">> {};
  struct DataReference
      : rule<ref<Reference> +
             many(nest<&ast::Index::target>(
                      lit<"{"> + some(append<&ast::Index::indexes>(ref<Expression>),
                                      lit<",">) + lit<"}">) |
                  nest<&ast::Component::target>(
                      lit<"."> + assign<&ast::Component::name>(ref<IDENTIFIER>)))> {};
  struct Primary : rule<ref<Boolean> | ref<Character> | ref<String> | ref<Float> |
                        ref<Integer> | ref<FunctionCall> | ref<DataReference> |
                        ref<Array> | (lit<"("> + ref<Expression> + lit<")">)> {};
  struct Term : infix<Primary, ast::Binary,
                      left(lit<"*"> | lit<"/"> | ikw<"DIV"> | ikw<"MOD">)> {};
  // Native 3.1 allows a sign only on the initial term, not on each RHS.
  struct SimpleExpression
      : rule<(create<ast::Unary>(
                  assign<&ast::Unary::op>(lit<"+"> | lit<"-">) +
                  assign<&ast::Unary::operand>(ref<Term>)) | ref<Term>) +
             many(nest<&ast::Binary::left>(
                 assign<&ast::Binary::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::Binary::right>(ref<Term>)))> {};
  struct Relation : infix<SimpleExpression, ast::Binary,
                          nonassoc(lit<"<="> | lit<">="> | lit<"<>"> |
                                   lit<"<"> | lit<">"> | lit<"=">)> {};
  struct LogicalTerm : infix<Relation, ast::Binary, left(ikw<"AND">)> {};
  // NOT applies to the initial logical term; OR and XOR share a left level.
  struct Expression
      : rule<(create<ast::Unary>(
                  assign<&ast::Unary::op>(ikw<"NOT">) +
                  assign<&ast::Unary::operand>(ref<LogicalTerm>)) | ref<LogicalTerm>) +
             many(nest<&ast::Binary::left>(
                 assign<&ast::Binary::op>(ikw<"OR"> | ikw<"XOR">) +
                 assign<&ast::Binary::right>(ref<LogicalTerm>)))> {};
  static constexpr auto expression = ref<Expression>;
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
             option((lit<":="> + assign<&ast::BindingArgument::value>(expression)) |
                    (flag<&ast::BindingArgument::isConditional>(lit<"?">) +
                     assign<&ast::BindingArgument::value>(ref<DataReference>)))> {};
  struct NamedArgument
      : rule<assign<&ast::BindingArgument::name>(ref<IDENTIFIER>) + lit<":="> +
             assign<&ast::BindingArgument::value>(expression)> {};
  static constexpr auto argument =
      ref<OptionalArgument> | ref<NamedArgument> | expression;
  template <auto Member>
  static constexpr auto arguments =
      option(append<Member>(argument)) +
      many((lit<","> + append<Member>(argument)) |
           append<Member>(ref<OptionalArgument>));
  struct FunctionCall
      : rule<assign<&ast::FunctionCall::name>(ref<CALL_NAME>) + lit<"("> +
             arguments<&ast::FunctionCall::arguments> + lit<")">> {};
  struct Call
      : rule<assign<&ast::Call::name>(ref<CALL_NAME>) +
             arguments<&ast::Call::arguments> +
             lit<";">> {};
  struct Statement;
  struct Assignment
      : rule<assign<&ast::Assignment::target>(ref<DataReference>) + lit<":="> +
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
  struct For
      : rule<ikw<"FOR"> + assign<&ast::For::variable>(ref<IDENTIFIER>) +
             ikw<"FROM"> + assign<&ast::For::from>(expression) +
             ikw<"TO"> + assign<&ast::For::to>(expression) +
             option(ikw<"STEP"> + assign<&ast::For::step>(expression)) +
             ikw<"DO"> + many(append<&ast::For::statements>(ref<Statement>)) +
             ikw<"ENDFOR">> {};
  struct TestCase
      : rule<ikw<"CASE"> + some(append<&ast::TestCase::values>(expression), lit<",">) +
             lit<":"> + many(append<&ast::TestCase::statements>(ref<Statement>))> {};
  struct Test
      : rule<ikw<"TEST"> + assign<&ast::Test::value>(expression) +
             many(append<&ast::Test::cases>(ref<TestCase>)) +
             option(flag<&ast::Test::hasDefault>(ikw<"DEFAULT">) + lit<":"> +
                    many(append<&ast::Test::defaultStatements>(ref<Statement>))) +
             ikw<"ENDTEST">> {};
  struct Label : rule<assign<&ast::Label::name>(ref<CALL_NAME>) + lit<":">> {};
  struct Goto : rule<ikw<"GOTO"> + assign<&ast::Goto::name>(ref<IDENTIFIER>) + lit<";">> {};
  struct Break : rule<create<ast::Break>(ikw<"BREAK"> + lit<";">)> {};
  struct Continue : rule<create<ast::Continue>(ikw<"CONTINUE"> + lit<";">)> {};
  struct Exit : rule<create<ast::Exit>(ikw<"EXIT"> + lit<";">)> {};
  struct Retry : rule<create<ast::Retry>(ikw<"RETRY"> + lit<";">)> {};
  struct TryNext : rule<create<ast::TryNext>(ikw<"TRYNEXT"> + lit<";">)> {};
  struct Raise
      : rule<create<ast::Raise>(ikw<"RAISE"> +
             option(assign<&ast::Raise::error>(expression)) + lit<";">)> {};
  struct Connect
      : rule<ikw<"CONNECT"> + assign<&ast::Connect::target>(ref<DataReference>) +
             ikw<"WITH"> + assign<&ast::Connect::trap>(ref<IDENTIFIER>) + lit<";">> {};
  struct Statement : rule<ref<Assignment> | ref<Return> | ref<If> | ref<While> |
                         ref<For> | ref<Test> | ref<Label> | ref<Goto> |
                         ref<Break> | ref<Continue> | ref<Exit> | ref<Raise> |
                         ref<Retry> | ref<TryNext> | ref<Connect> | ref<Call>> {};
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
  struct BackwardHandler
      : rule<create<ast::BackwardHandler>(ikw<"BACKWARD"> +
             many(append<&ast::Handler::statements>(ref<Statement>)))> {};
  struct ErrorHandler
      : rule<create<ast::ErrorHandler>(ikw<"ERROR"> +
             option(lit<"("> + some(append<&ast::ErrorHandler::numbers>(
                 ref<Integer> | ref<Float> | ref<Reference>), lit<",">) + lit<")">) +
             many(append<&ast::Handler::statements>(ref<Statement>)))> {};
  struct UndoHandler
      : rule<create<ast::UndoHandler>(ikw<"UNDO"> +
             many(append<&ast::Handler::statements>(ref<Statement>)))> {};
  static constexpr auto error_and_undo =
      option(assign<&ast::Routine::error>(ref<ErrorHandler>)) +
      option(assign<&ast::Routine::undo>(ref<UndoHandler>));
  struct Procedure
      : rule<create<ast::Procedure>(
            option(flag<&ast::Routine::local>(ikw<"LOCAL">)) +
            ikw<"PROC"> + assign<&ast::Routine::name>(ref<IDENTIFIER>) +
            parameters + body +
            option(assign<&ast::Routine::backward>(ref<BackwardHandler>)) +
            error_and_undo + ikw<"ENDPROC">)> {};
  struct Function
      : rule<create<ast::Function>(
            option(flag<&ast::Routine::local>(ikw<"LOCAL">)) +
            ikw<"FUNC"> + assign<&ast::Function::returnType>(ref<IDENTIFIER>) +
            assign<&ast::Routine::name>(ref<IDENTIFIER>) + parameters + body +
            error_and_undo + ikw<"ENDFUNC">)> {};
  struct Trap
      : rule<create<ast::Trap>(option(flag<&ast::Routine::local>(ikw<"LOCAL">)) +
            ikw<"TRAP"> + assign<&ast::Routine::name>(ref<IDENTIFIER>) + body +
            error_and_undo + ikw<"ENDTRAP">)> {};
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
                  append<&ast::Module::functions>(ref<Function>) |
                  append<&ast::Module::traps>(ref<Trap>)) +
             ikw<"ENDMODULE">> {};
  using entry = Module;
  using ignored = WS;
  using hidden = LINE_COMMENT;
};
} // namespace abb
