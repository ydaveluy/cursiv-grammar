// The teal language: its grammar. Teal is Lua's syntax plus a type
// annotation layer (`typ`, `newtype`) grafted onto the same statements and
// expressions.
#pragma once

#include <cstddef>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "teal/core/ast.hpp"

namespace teal {

inline constexpr auto strip1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<strip1>;

// A long bracket's content drops one leading newline; the delimiters are as
// wide as the opener's run of `=` made them.
inline constexpr std::string_view strip_long(std::string_view t) {
  const std::size_t w = t.find('[', 1) + 1;
  std::string_view inner = t.substr(w, t.size() - 2 * w);
  if (!inner.empty() && inner.front() == '\n') {
    inner.remove_prefix(1);
  } else if (inner.size() >= 2 && inner[0] == '\r' && inner[1] == '\n') {
    inner.remove_prefix(2);
  }
  return inner;
}
using Long = cursiv::text_conv<strip_long>;

struct Teal : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};

  // Long brackets `[=*[ ... ]=*]`: the closer repeats the opener's run of
  // `=`, at any level. Shared shape with COMMENT below -- a terminal's body
  // may not `ref<>` a sibling terminal, only reuse the same expression.
  struct Level {};
  static constexpr auto Bracket =
      through(lit<"["> + mark<Level>(many(cr<"=">)) + lit<"[">, lit<"]"> + same<Level> + lit<"]">);
  struct LONGSTRING : terminal<Bracket, Long> {};

  static constexpr auto safeBracket = lit<"[", !cr<"[=">>;
  static constexpr auto commentFirst = cr<R"(^\r\n[)"> | safeBracket;
  struct COMMENT
      : terminal<lit<"--"> +
                 option(Bracket | (commentFirst + many(cr<R"(^\r\n)">)))> {};

  struct SHEBANG : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  // Kept: dropping it changes no tree but costs +26 % instructions.
  static constexpr auto Reserved =
      kw<"and"> | kw<"as"> | kw<"boolean"> | kw<"break"> | kw<"do"> |
      kw<"else"> | kw<"elseif"> | kw<"end"> | kw<"enum"> | kw<"false"> |
      kw<"for"> | kw<"function"> | kw<"functiontype"> | kw<"global"> |
      kw<"goto"> | kw<"if"> | kw<"in"> | kw<"is"> | kw<"local"> |
      kw<"nil"> | kw<"not"> | kw<"number"> | kw<"or"> | kw<"record"> |
      kw<"repeat"> | kw<"return"> | kw<"string"> | kw<"then"> | kw<"true"> |
      kw<"until"> | kw<"while">;
  // Reserved everywhere: real Teal never lets `local end = 1` parse.
  struct ID : terminal<!Reserved + cr<"a-zA-Z_"> + many(w)> {};

  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto hexExponent = cr<"pP"> + option(cr<"+-">) + some(d);
  static constexpr auto hexDigits = some(cr<"0-9a-fA-F">);
  struct NUMBER
      : terminal<
            (lit<"0"> + cr<"xX"> + hexDigits + lit<"."> +
             many(cr<"0-9a-fA-F">) + option(hexExponent))  //
            | (lit<"0"> + cr<"xX"> + lit<"."> + hexDigits + option(hexExponent))  //
            | (lit<"0"> + cr<"xX"> + hexDigits + hexExponent)  //
            | (lit<"0"> + cr<"xX"> + hexDigits)                //
            | (some(d) + lit<"."> + many(d) + option(exponent))  //
            | (lit<"."> + some(d) + option(exponent))            //
            | (some(d) + exponent)                                //
            | some(d)> {};

  // `\z` skips the run of whitespace that follows it, newlines included.
  // Every other escape is accepted generically rather than enumerated
  // (decimal, hex and unicode forms all fit `\` + one byte read on): a
  // superset of what real Teal accepts, never a refusal of it.
  static constexpr auto escape =
      (lit<"\\z"> + many(cr<R"( \t\r\n\f)">)) | (lit<"\\"> + any);
  struct NORMALSTRING
      : terminal<lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 Unquoted> {};
  struct CHARSTRING
      : terminal<lit<"'"> + many(escape | cr<R"(^'\\\r\n)">) + lit<"'">,
                 Unquoted> {};
  static constexpr auto StrText =
      ref<NORMALSTRING> | ref<CHARSTRING> | ref<LONGSTRING>;

  struct StringExprRule
      : rule<create<ast::StringExpr>(
            assign<&ast::StringExpr::value>(StrText))> {};

  // Operators, each its own token carrying the constant it denotes rather
  // than its text.
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

  struct TypRule;
  struct NewTypeRule;
  struct Expression;
  struct Block;
  struct FuncBodyRule;
  struct TableExprRule;
  struct PrefixExpr;

  struct TypeArgsRule
      : rule<create<ast::TypeArgs>(
            lit<"<"> + some(append<&ast::TypeArgs::names>(ref<ID>), lit<",">) +
            lit<">">)> {};

  // `partype`: an optional name ahead of a required type.
  struct PartypeRule
      : rule<create<ast::Param>(
            option(assign<&ast::Param::name>(ref<ID>) + lit<":">) +
            assign<&ast::Param::type>(ref<TypRule>))> {};

  // `parname`: a required name, optionally typed.
  struct ParnameRule
      : rule<create<ast::Param>(
            assign<&ast::Param::name>(ref<ID>) +
            option(lit<":"> + assign<&ast::Param::type>(ref<TypRule>)))> {};

  struct RetListRule : rule<create<ast::RetList>(
      (lit<"("> + option(some(append<&ast::RetList::types>(ref<TypRule>), lit<",">)) + option(flag<&ast::RetList::vararg>(lit<"...">)) + lit<")">) |
      (some(append<&ast::RetList::types>(ref<TypRule>), lit<",">) + option(flag<&ast::RetList::vararg>(lit<"...">))))> {};

  struct FunctionTypeRule
      : rule<create<ast::FunctionSigType>(
            option(assign<&ast::FunctionSigType::typeParams>(
                ref<TypeArgsRule>)) +
            lit<"("> +
            option(some(append<&ast::FunctionSigType::params>(
                            ref<PartypeRule>),
                        lit<",">)) +
            lit<")"> +
            option(lit<":"> +
                   assign<&ast::FunctionSigType::rets>(ref<RetListRule>)))> {
  };

  struct BaseTypeRule
      : rule<create<ast::PrimitiveType>(assign<&ast::PrimitiveType::kind>(
                 kw<"string"> | kw<"boolean"> | kw<"nil"> | kw<"number">))  //
             | create<ast::MapType>(
                   lit<"{"> + assign<&ast::MapType::key>(ref<TypRule>) +
                   lit<":"> + assign<&ast::MapType::value>(ref<TypRule>) +
                   lit<"}">)  //
             | create<ast::ArrayType>(
                   lit<"{"> + assign<&ast::ArrayType::element>(ref<TypRule>) +
                   lit<"}">)  //
             | (kw<"function"> + ref<FunctionTypeRule>)  //
             | create<ast::NamedType>(
                   assign<&ast::NamedType::name>(ref<ID>) +
                   option(assign<&ast::NamedType::typeArgs>(
                       ref<TypeArgsRule>)))> {};

  // `typ`: a union of one or more base types, always this node -- a reader
  // checks `alternatives.size()` rather than a grammar-level special case
  // for the trivial one-element union.
  struct TypRule
      : rule<(lit<"("> + ref<TypRule> + lit<")">)  //
             | create<ast::UnionType>(
                   append<&ast::UnionType::alternatives>(ref<BaseTypeRule>) +
                   many(lit<"|"> + append<&ast::UnionType::alternatives>(
                                       ref<BaseTypeRule>)))> {};

  struct NamedNewTypeRule
      : rule<create<ast::NamedNewType>(
            assign<&ast::NamedNewType::name>(ref<ID>) + lit<"="> +
            assign<&ast::NamedNewType::value>(ref<NewTypeRule>))> {};

  struct RecordFieldRule
      : rule<create<ast::Param>(assign<&ast::Param::name>(ref<ID>) +
                                lit<":"> +
                                assign<&ast::Param::type>(ref<TypRule>))> {};

  struct NewTypeRule
      : rule<create<ast::RecordNewType>(
                 kw<"record"> +
                 option(assign<&ast::RecordNewType::typeParams>(
                     ref<TypeArgsRule>)) +
                 option(lit<"{"> +
                        assign<&ast::RecordNewType::arrayPart>(ref<TypRule>) +
                        lit<"}">) +
                 many(append<&ast::RecordNewType::nestedTypes>(
                     ref<NamedNewTypeRule>)) +
                 many(append<&ast::RecordNewType::fields>(
                     ref<RecordFieldRule>)) +
                 kw<"end">)  //
             | create<ast::EnumNewType>(
                   kw<"enum"> +
                   many(append<&ast::EnumNewType::values>(StrText)) +
                   kw<"end">)  //
             | create<ast::FuncNewType>(
                   kw<"functiontype"> +
                   assign<&ast::FuncNewType::signature>(
                       ref<FunctionTypeRule>))> {};

  static constexpr auto ArgsBody =
      (lit<"("> +
       option(some(append<&ast::Call::args>(ref<Expression>), lit<",">)) +
       lit<")">)                                     //
      | append<&ast::Call::args>(ref<TableExprRule>)  //
      | append<&ast::Call::args>(ref<StringExprRule>);

  // A name or a parenthesized expression, followed by any run of suffixes:
  // the shape settles once a suffix is fully seen, so no lookahead has to
  // guess it upfront.
  struct PrefixExpr
      : rule<(create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<ID>))  //
              | create<ast::ParenExpr>(
                    lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                    lit<")">)) +
             many(nest<ast::IndexExpr, &ast::IndexExpr::object>(
                      lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) +
                      lit<"]">)  //
                  | nest<ast::FieldExpr, &ast::FieldExpr::object>(
                        lit<"."> + assign<&ast::FieldExpr::field>(ref<ID>))  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(ArgsBody)  //
                  | nest<ast::MethodCallExpr, &ast::MethodCallExpr::object>(
                        lit<":"> +
                        assign<&ast::MethodCallExpr::method>(ref<ID>) +
                        ArgsBody))> {};

  struct FieldRule
      : rule<create<ast::TableField>(
                 lit<"["> + assign<&ast::TableField::key>(ref<Expression>) +
                 lit<"]"> + lit<"="> +
                 assign<&ast::TableField::value>(ref<Expression>))  //
             | create<ast::TableField>(
                   assign<&ast::TableField::name>(ref<ID>) +
                   option(lit<":"> +
                          assign<&ast::TableField::fieldType>(ref<TypRule>)) +
                   lit<"="> +
                   assign<&ast::TableField::value>(ref<Expression>))  //
             | create<ast::TableField>(
                   assign<&ast::TableField::name>(ref<ID>) + lit<"="> +
                   assign<&ast::TableField::newType>(ref<NewTypeRule>))  //
             | create<ast::TableField>(
                   assign<&ast::TableField::value>(ref<Expression>))> {};

  struct TableExprRule
      : rule<create<ast::TableExpr>(
            lit<"{"> +
            option(some(append<&ast::TableExpr::fields>(ref<FieldRule>),
                        (lit<","> | lit<";">)) +
                   option(lit<","> | lit<";">)) +
            lit<"}">)> {};

  static constexpr auto Vararg =
      flag<&ast::FuncBody::vararg>(lit<"...">) +
      option(lit<":"> + assign<&ast::FuncBody::varargType>(ref<TypRule>));

  struct FuncBodyRule
      : rule<create<ast::FuncBody>(
            option(assign<&ast::FuncBody::typeParams>(ref<TypeArgsRule>)) +
            lit<"("> +
            option((some(append<&ast::FuncBody::params>(ref<ParnameRule>),
                         lit<",">) +
                    option(lit<","> + Vararg)) |
                   Vararg) +
            lit<")"> +
            option(lit<":"> + assign<&ast::FuncBody::rets>(ref<RetListRule>)) +
            assign<&ast::FuncBody::body>(ref<Block>) + kw<"end">)> {};

  struct AtomPrimary
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
             | create<ast::IsExpr>(assign<&ast::IsExpr::name>(ref<ID>) +
                                   kw<"is"> +
                                   assign<&ast::IsExpr::type>(ref<TypRule>))  //
             | ref<PrefixExpr>> {};

  // `as` binds tighter than every other operator, including `^`.
  struct AtomAsRule
      : rule<ref<AtomPrimary> +
             many(nest<ast::AsExpr, &ast::AsExpr::expr>(
                 kw<"as"> + assign<&ast::AsExpr::type>(ref<TypRule>)))> {};

  // `^` binds tighter than a unary operator, but a unary operand may itself
  // hold a `^` chain (`-2^2` is `-(2^2)`); the alternatives below are the
  // whole of that, not a table entry.
  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<OpNot> | ref<OpLen> |
                                             ref<OpNeg> | ref<OpBNot>) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | create<ast::BinaryExpr>(
                   assign<&ast::BinaryExpr::left>(ref<AtomAsRule>) +
                   assign<&ast::BinaryExpr::op>(ref<OpPow>) +
                   assign<&ast::BinaryExpr::right>(ref<Unary>))  //
             | ref<AtomAsRule>> {};

  // The bitwise level sits between comparison and concatenation, as the Lua
  // manual places it (Teal follows Lua's precedence).
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
      : rule<create<ast::AttribName>(
            assign<&ast::AttribName::name>(ref<ID>) +
            option(lit<"<"> + assign<&ast::AttribName::attrib>(ref<ID>) +
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

  struct FuncStatRule
      : rule<create<ast::FuncStat>(
            kw<"function"> + assign<&ast::FuncStat::name>(ref<ID>) +
            many(lit<"."> + append<&ast::FuncStat::path>(ref<ID>)) +
            option(lit<":"> + assign<&ast::FuncStat::method>(ref<ID>)) +
            assign<&ast::FuncStat::body>(ref<FuncBodyRule>))> {};

  struct LocalFuncStatRule
      : rule<create<ast::LocalFuncStat>(
            kw<"local"> + kw<"function"> +
            assign<&ast::LocalFuncStat::name>(ref<ID>) +
            assign<&ast::LocalFuncStat::body>(ref<FuncBodyRule>))> {};

  struct GlobalFuncStatRule
      : rule<create<ast::GlobalFuncStat>(
            kw<"global"> + kw<"function"> +
            assign<&ast::GlobalFuncStat::name>(ref<ID>) +
            assign<&ast::GlobalFuncStat::body>(ref<FuncBodyRule>))> {};

  struct LocalNewTypeStatRule
      : rule<create<ast::LocalNewTypeStat>(
            kw<"local"> + assign<&ast::LocalNewTypeStat::name>(ref<ID>) +
            lit<"="> +
            assign<&ast::LocalNewTypeStat::value>(ref<NewTypeRule>))> {};

  struct LocalAttrAssignStatRule
      : rule<create<ast::LocalAttrAssignStat>(
            kw<"local"> +
            some(append<&ast::LocalAttrAssignStat::vars>(ref<AttribNameRule>),
                 lit<",">) +
            option(
                lit<":"> +
                some(append<&ast::LocalAttrAssignStat::types>(ref<TypRule>), lit<",">)) +
            option(
                lit<"="> +
                some(append<&ast::LocalAttrAssignStat::values>(ref<Expression>),
                     lit<",">)))> {};

  struct GlobalAssignStatRule
      : rule<create<ast::GlobalAssignStat>(
            kw<"global"> + assign<&ast::GlobalAssignStat::name>(ref<ID>) +
            lit<"="> +
            assign<&ast::GlobalAssignStat::value>(ref<NewTypeRule>))> {};

  struct GlobalAttrStatRule
      : rule<create<ast::GlobalAttrStat>(
            kw<"global"> +
            some(append<&ast::GlobalAttrStat::vars>(ref<AttribNameRule>),
                 lit<",">) +
            lit<":"> + append<&ast::GlobalAttrStat::types>(ref<TypRule>) +
            option(lit<","> +
                   append<&ast::GlobalAttrStat::types>(ref<TypRule>)))> {};

  struct GlobalAttrAssignStatRule
      : rule<create<ast::GlobalAttrAssignStat>(
            kw<"global"> +
            some(append<&ast::GlobalAttrAssignStat::vars>(ref<AttribNameRule>),
                 lit<",">) +
            option(lit<":"> +
                   some(append<&ast::GlobalAttrAssignStat::types>(ref<TypRule>), lit<",">)) +
            lit<"="> +
            some(append<&ast::GlobalAttrAssignStat::values>(ref<Expression>),
                 lit<",">))> {};

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
             | ref<IfStatRule>              //
             | ref<NumericForRule>          //
             | ref<GenericForRule>          //
             | ref<FuncStatRule>            //
             | ref<LocalFuncStatRule>       //
             | ref<LocalNewTypeStatRule>    //
             | ref<LocalAttrAssignStatRule>  //
             | ref<GlobalFuncStatRule>      //
             | ref<GlobalAssignStatRule>    //
             | ref<GlobalAttrStatRule>      //
             | ref<GlobalAttrAssignStatRule>> {};

  struct ReturnStatRule
      : rule<create<ast::ReturnStat>(
            kw<"return"> +
            option(some(append<&ast::ReturnStat::values>(ref<Expression>),
                        lit<",">)) +
            option(lit<";">))> {};

  // A lone `;` is an empty statement, dropped rather than given its own
  // node, like real Lua's own C parser does.
  struct Block
      : rule<many(lit<";"> | append<&ast::Block::statements>(ref<StatRule>)) +
             option(append<&ast::Block::statements>(ref<ReturnStatRule>))> {};

  struct ChunkRule : rule<option(ref<SHEBANG>) + ref<Block>> {};

  using entry = ChunkRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace teal
