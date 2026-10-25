// The lua language: its grammar.
#pragma once

#include <cstddef>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lua/core/ast.hpp"

namespace lua {

inline constexpr auto strip1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<strip1>;

// A long bracket's content drops one leading newline (Lua strips it so an
// author can open `[[` and start writing on the next line for free). The
// delimiters are as wide as the opener's run of `=` made them.
inline constexpr std::string_view strip_long(std::string_view t) {
  const std::size_t w = t.find('[', 1) + 1;
  std::string_view inner = t.substr(w, t.size() - 2 * w);
  if (!inner.empty() && (inner.front() == '\n' || inner.front() == '\r')) {
    const char first = inner.front();
    inner.remove_prefix(1);
    if (!inner.empty() && (inner.front() == '\n' || inner.front() == '\r') &&
        inner.front() != first)
      inner.remove_prefix(1);
  }
  return inner;
}
using Long = cursiv::text_conv<strip_long>;

struct Lua : cursiv::GrammarSpec {
  static constexpr auto Space = cr<R"( \t\r\n\f\v)">;
  struct WS : terminal<some(Space)> {};

  // Long brackets `[=*[ ... ]=*]`: the closer repeats the opener's run of
  // `=`, at any level. The shape is a value shared with COMMENT below -- a
  // terminal's body may not `ref<>` a sibling terminal, only reuse the same
  // expression.
  struct Level {};
  static constexpr auto Bracket =
      through(lit<"["> + mark<Level>(many(cr<"=">)) + lit<"[">, lit<"]"> + same<Level> + lit<"]">);
  struct LONGSTRING : terminal<Bracket, Long> {};

  // A `[` that cannot open a long bracket -- what follows it is neither `[`
  // nor `=`.
  static constexpr auto safeBracket = lit<"["> + many(lit<"=">) + !cr<"[=">;
  // The short form's first byte only, guarded: unguarded, `many(non-newline)`
  // is unbounded and a same-line `--[[ ... ]] code` would run its match past
  // the closer and swallow `code` too, since the longer overall match (the
  // wrong one) wins. Once this first byte is behind it, no later `[` can
  // still be a long bracket's opener -- one begins right after `--` or not
  // at all -- so the rest of the line is unguarded.
  static constexpr auto commentFirst = cr<R"(^\r\n[)"> | safeBracket;
  struct COMMENT
      : terminal<lit<"--"> +
                 option(Bracket | (commentFirst + many(cr<R"(^\r\n)">)))> {};

  // Consumed only at the very start of the entry rule (below), so this
  // never competes with the `#` (length) operator inside an expression.
  struct SHEBANG : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto hexExponent = cr<"pP"> + option(cr<"+-">) + some(d);
  static constexpr auto hexDigits = some(cr<"0-9a-fA-F">);
  struct NUMBER
      : terminal<
            (lit<"0"> + cr<"xX"> +
             ((hexDigits + option(lit<"."> + many(cr<"0-9a-fA-F">))) |
              (lit<"."> + hexDigits)) +
             option(hexExponent)) |
            (((some(d) + option(lit<"."> + many(d))) |
              (lit<"."> + some(d))) +
             option(exponent))> {};

  // `\z` skips the run of whitespace that follows it, newlines included --
  // the one escape whose effect reaches past its own two characters.
  static constexpr auto escape =
      lit<"\\"> +
      ((lit<"z"> + many(Space)) | (lit<"\r"> + option(lit<"\n">)) |
       (lit<"\n"> + option(lit<"\r">)) | any);
  struct NORMALSTRING
      : terminal<lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 Unquoted> {};
  struct CHARSTRING
      : terminal<lit<"'"> + many(escape | cr<R"(^'\\\r\n)">) + lit<"'">,
                 Unquoted> {};

  // Operators, each its own token carrying the constant it denotes rather
  // than its text -- the ladder below reads `op`, never a spelling.
  struct OpAdd : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::BinOp::sub>> {};
  struct OpMul : terminal<lit<"*">, value<ast::BinOp::mul>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::BinOp::div>> {};
  struct OpIDiv : terminal<lit<"//">, value<ast::BinOp::idiv>> {};
  struct OpMod : terminal<lit<"%">, value<ast::BinOp::mod>> {};
  struct OpPow : terminal<lit<"^">, value<ast::BinOp::pow>> {};
  struct OpConcat : terminal<lit<"..">, value<ast::BinOp::concat>> {};
  struct OpEq : terminal<lit<"==">, value<ast::BinOp::eq>> {};
  struct OpNe : terminal<lit<"~=">, value<ast::BinOp::ne>> {};
  struct OpLe : terminal<lit<"<=">, value<ast::BinOp::le>> {};
  struct OpGe : terminal<lit<">=">, value<ast::BinOp::ge>> {};
  struct OpLt : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct OpGt : terminal<lit<">">, value<ast::BinOp::gt>> {};
  struct OpAndTok : terminal<kw<"and">, value<ast::BinOp::logical_and>> {};
  struct OpOrTok : terminal<kw<"or">, value<ast::BinOp::logical_or>> {};
  struct OpBAnd : terminal<lit<"&">, value<ast::BinOp::band>> {};
  struct OpBOr : terminal<lit<"|">, value<ast::BinOp::bor>> {};
  struct OpBXor : terminal<lit<"~">, value<ast::BinOp::bxor>> {};
  struct OpShl : terminal<lit<"<<">, value<ast::BinOp::shl>> {};
  struct OpShr : terminal<lit<">>">, value<ast::BinOp::shr>> {};

  struct OpNeg : terminal<lit<"-">, value<ast::UnOp::neg>> {};
  struct OpNot : terminal<kw<"not">, value<ast::UnOp::lnot>> {};
  struct OpLen : terminal<lit<"#">, value<ast::UnOp::len>> {};
  struct OpBNot : terminal<lit<"~">, value<ast::UnOp::bnot>> {};

  struct StringExprRule
      : rule<create<ast::StringExpr>(assign<&ast::StringExpr::value>(
            ref<NORMALSTRING> | ref<CHARSTRING> | ref<LONGSTRING>))> {};

  struct Expression;
  struct Block;
  struct StatRule;
  struct FuncBodyRule;
  struct TableExprRule;

  // `NAME | '(' exp ')'` followed by any run of suffixes: the shape
  // (index/field/call/method-call) settles the node once a suffix is fully
  // seen, so no lookahead has to guess it in advance.
  struct PrefixExpr;

  static constexpr auto ParenArgs =
      lit<"("> +
      option(some(append<&ast::Call::args>(ref<Expression>), lit<",">)) +
      lit<")">;
  static constexpr auto ArgsBody =
      ParenArgs                                       //
      | append<&ast::Call::args>(ref<TableExprRule>)  //
      | append<&ast::Call::args>(ref<StringExprRule>);

  // A `(` after a prefix expression calls it, as Lua 5.2 reads `f\n(g)`: the
  // run of calls on parentheses is a possessive of its own, its one opening
  // token committing it, between the suffixes that open on any other.
  static constexpr auto ParenCalls =
      many(nest<ast::CallExpr, &ast::CallExpr::callee>(ParenArgs));

  struct PrefixExpr
      : rule<(create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<ID>))  //
              | create<ast::ParenExpr>(
                    lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                    lit<")">)) +
             ParenCalls +
             many((nest<ast::IndexExpr, &ast::IndexExpr::object>(
                       lit<"["> +
                       assign<&ast::IndexExpr::index>(ref<Expression>) +
                       lit<"]">)  //
                   | nest<ast::FieldExpr, &ast::FieldExpr::object>(
                         lit<"."> + assign<&ast::FieldExpr::field>(ref<ID>))  //
                   | nest<ast::CallExpr, &ast::CallExpr::callee>(
                         append<&ast::Call::args>(ref<TableExprRule>) |
                         append<&ast::Call::args>(ref<StringExprRule>))  //
                   | nest<ast::MethodCallExpr, &ast::MethodCallExpr::object>(
                         lit<":"> +
                         assign<&ast::MethodCallExpr::method>(ref<ID>) +
                         ArgsBody)) +
                  ParenCalls)> {};

  struct FieldRule
      : rule<create<ast::TableField>(
            (lit<"["> + assign<&ast::TableField::key>(ref<Expression>) +
             lit<"]"> + lit<"="> +
             assign<&ast::TableField::value>(ref<Expression>))  //
            | (assign<&ast::TableField::name>(ref<ID>) + lit<"="> +
               assign<&ast::TableField::value>(ref<Expression>))  //
            | assign<&ast::TableField::value>(ref<Expression>))> {};

  struct TableExprRule
      : rule<create<ast::TableExpr>(
            lit<"{"> +
            option(some(append<&ast::TableExpr::fields>(ref<FieldRule>),
                        (lit<","> | lit<";">)) +
                   option(lit<","> | lit<";">)) +
            lit<"}">)> {};

  struct FuncBodyRule
      : rule<create<ast::FuncBody>(
            lit<"("> +
            option((some(append<&ast::FuncBody::params>(ref<ID>), lit<",">) +
                    option(lit<","> +
                           flag<&ast::FuncBody::vararg>(lit<"...">))) |
                   flag<&ast::FuncBody::vararg>(lit<"...">)) +
            lit<")"> + assign<&ast::FuncBody::body>(ref<Block>) + kw<"end">)> {
  };

  struct Atom
      : rule<create<ast::NilExpr>(kw<"nil">)  //
             | create<ast::BoolExpr>(
                   flag<&ast::BoolExpr::value>(kw<"true">) | kw<"false">)  //
             | create<ast::VarargExpr>(lit<"...">)  //
             | create<ast::NumberExpr>(assign<&ast::NumberExpr::text>(ref<NUMBER>))  //
             | ref<StringExprRule>  //
             | create<ast::FunctionExpr>(
                   kw<"function"> +
                   assign<&ast::FunctionExpr::body>(ref<FuncBodyRule>))  //
             | ref<TableExprRule>  //
             | ref<PrefixExpr>> {};

  // `^` binds tighter than a unary operator, but a unary operator's operand
  // may itself hold a `^` chain (`-2^2` is `-(2^2)`, `2^-2` is `2^(-2)`):
  // the three alternatives below are the whole of that, not a table entry.
  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OpNeg> | ref<OpNot> |
                                             ref<OpLen> | ref<OpBNot>) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | create<ast::BinaryExpr>(
                   assign<&ast::BinaryExpr::left>(ref<Atom>) +
                   assign<&ast::BinaryExpr::op>(ref<OpPow>) +
                   assign<&ast::BinaryExpr::right>(ref<Unary>))  //
             | ref<Atom>> {};

  struct Expression
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<OpMul> | ref<OpDiv> | ref<OpIDiv> | ref<OpMod>) >>  //
               left(ref<OpAdd> | ref<OpSub>) >>                            //
               right(ref<OpConcat>) >>                                     //
               left(ref<OpShl> | ref<OpShr>) >>                            //
               left(ref<OpBAnd>) >>                                        //
               left(ref<OpBXor>) >>                                        //
               left(ref<OpBOr>) >>                                         //
               left(ref<OpLt> | ref<OpGt> | ref<OpLe> | ref<OpGe> |
                    ref<OpNe> | ref<OpEq>) >>                              //
               left(ref<OpAndTok>) >>                                      //
               left(ref<OpOrTok>))> {};

  struct AttribNameRule
      : rule<create<ast::LocalVar>(
            assign<&ast::LocalVar::name>(ref<ID>) +
            option(lit<"<"> + assign<&ast::LocalVar::attrib>(ref<ID>) +
                   lit<">">))> {};

  struct ExprStat
      : rule<create<ast::AssignStat>(
                 append<&ast::AssignStat::targets>(ref<PrefixExpr>) +
                 many(lit<","> +
                      append<&ast::AssignStat::targets>(ref<PrefixExpr>)) +
                 lit<"="> +
                 some(append<&ast::AssignStat::values>(ref<Expression>),
                      lit<",">))  //
             | create<ast::CallStat>(
                   assign<&ast::CallStat::call>(ref<PrefixExpr>))> {};

  struct IfClauseRule
      : rule<create<ast::IfClause>(
            assign<&ast::IfClause::cond>(ref<Expression>) + kw<"then"> +
            assign<&ast::IfClause::body>(ref<Block>))> {};

  struct IfStatRule
      : rule<create<ast::IfStat>(
            kw<"if"> + append<&ast::IfStat::clauses>(ref<IfClauseRule>) +
            many(kw<"elseif"> +
                 append<&ast::IfStat::clauses>(ref<IfClauseRule>)) +
            option(kw<"else"> + assign<&ast::IfStat::elseBody>(ref<Block>)) +
            kw<"end">)> {};

  struct NumericForRule
      : rule<create<ast::NumericForStat>(
            kw<"for"> + assign<&ast::NumericForStat::name>(ref<ID>) +
            lit<"="> + assign<&ast::NumericForStat::start>(ref<Expression>) +
            lit<","> + assign<&ast::NumericForStat::stop>(ref<Expression>) +
            option(lit<","> +
                   assign<&ast::NumericForStat::step>(ref<Expression>)) +
            kw<"do"> + assign<&ast::NumericForStat::body>(ref<Block>) +
            kw<"end">)> {};

  struct GenericForRule
      : rule<create<ast::GenericForStat>(
            kw<"for"> + append<&ast::GenericForStat::names>(ref<ID>) +
            many(lit<","> + append<&ast::GenericForStat::names>(ref<ID>)) +
            kw<"in"> +
            some(append<&ast::GenericForStat::exprs>(ref<Expression>),
                 lit<",">) +
            kw<"do"> + assign<&ast::GenericForStat::body>(ref<Block>) +
            kw<"end">)> {};

  struct FunctionStatRule
      : rule<create<ast::FunctionStat>(
            kw<"function"> + assign<&ast::FunctionStat::name>(ref<ID>) +
            many(lit<"."> + append<&ast::FunctionStat::path>(ref<ID>)) +
            option(lit<":"> + assign<&ast::FunctionStat::method>(ref<ID>)) +
            assign<&ast::FunctionStat::body>(ref<FuncBodyRule>))> {};

  struct LocalFunctionStatRule
      : rule<create<ast::LocalFunctionStat>(
            kw<"local"> + kw<"function"> +
            assign<&ast::LocalFunctionStat::name>(ref<ID>) +
            assign<&ast::LocalFunctionStat::body>(ref<FuncBodyRule>))> {};

  struct LocalStatRule
      : rule<create<ast::LocalStat>(
            kw<"local"> +
            some(append<&ast::LocalStat::vars>(ref<AttribNameRule>),
                 lit<",">) +
            option(lit<"="> + some(append<&ast::LocalStat::values>(
                                       ref<Expression>),
                                   lit<",">)))> {};

  struct StatRule
      : rule<ref<ExprStat>  //
             | create<ast::LabelStat>(
                   lit<"::"> + assign<&ast::LabelStat::name>(ref<ID>) +
                   lit<"::">)  //
             | create<ast::BreakStat>(kw<"break">)  //
             | create<ast::GotoStat>(
                   kw<"goto"> + assign<&ast::GotoStat::label>(ref<ID>))  //
             | create<ast::DoStat>(kw<"do"> +
                                   assign<&ast::DoStat::body>(ref<Block>) +
                                   kw<"end">)  //
             | create<ast::WhileStat>(
                   kw<"while"> + assign<&ast::WhileStat::cond>(ref<Expression>) +
                   kw<"do"> + assign<&ast::WhileStat::body>(ref<Block>) +
                   kw<"end">)  //
             | create<ast::RepeatStat>(
                   kw<"repeat"> + assign<&ast::RepeatStat::body>(ref<Block>) +
                   kw<"until"> + assign<&ast::RepeatStat::cond>(ref<Expression>))  //
             | ref<IfStatRule>       //
             | ref<NumericForRule>   //
             | ref<GenericForRule>   //
             | ref<FunctionStatRule>       //
             | ref<LocalFunctionStatRule>  //
             | ref<LocalStatRule>> {};

  struct ReturnStatRule
      : rule<create<ast::ReturnStat>(
            kw<"return"> +
            option(some(append<&ast::ReturnStat::values>(ref<Expression>),
                        lit<",">)) +
            option(lit<";">))> {};

  struct Block
      : rule<many(lit<";"> | append<&ast::Block::statements>(ref<StatRule>)) +
             option(append<&ast::Block::statements>(ref<ReturnStatRule>))> {};

  struct ChunkRule : rule<option(ref<SHEBANG>) + ref<Block>> {};

  using entry = ChunkRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace lua
