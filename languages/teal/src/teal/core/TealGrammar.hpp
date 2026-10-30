// The teal language: its grammar. Teal is Lua's syntax plus a type
// annotation layer (`typ`, `newtype`) grafted onto the same statements and
// expressions.
#pragma once

#include <cstddef>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "teal/core/ast.hpp"

namespace teal {

// Preserve the unquoted source spelling, while rejecting malformed escapes.
inline constexpr auto strip1 = [](std::string_view quoted)
    -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  const auto text = quoted.substr(1, quoted.size() - 2);
  const auto bad = [] { return Result::err("Invalid Teal string escape."); };
  const auto hex = [](char c) -> int {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 :
           c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
  };
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\') continue;
    if (++i >= text.size()) return bad();
    const char c = text[i];
    if (std::string_view("abfnrtv\\\"'\r\n").find(c) != std::string_view::npos) continue;
    if (c == 'z') {
      while (i + 1 < text.size() && std::string_view(" \t\r\n\f\v").find(text[i + 1]) != std::string_view::npos) ++i;
    } else if (c >= '0' && c <= '9') {
      unsigned value = c - '0';
      for (unsigned n = 1; n < 3 && i + 1 < text.size() && text[i + 1] >= '0' && text[i + 1] <= '9'; ++n)
        value = value * 10 + (text[++i] - '0');
      if (value > 255) return bad();
    } else if (c == 'x') {
      if (i + 2 >= text.size() || hex(text[i + 1]) < 0 || hex(text[i + 2]) < 0) return bad();
      i += 2;
    } else if (c == 'u') {
      if (i + 1 >= text.size() || text[++i] != '{') return bad();
      unsigned value = 0, count = 0;
      while (i + 1 < text.size() && hex(text[i + 1]) >= 0) {
        if (value > (0x7fffffffU >> 4)) return bad();
        value = value * 16 + static_cast<unsigned>(hex(text[++i]));
        ++count;
      }
      if (!count || i + 1 >= text.size() || text[++i] != '}') return bad();
    } else return bad();
  }
  return Result::ok(text);
};

// A long bracket's content drops one leading newline; the delimiters are as
// wide as the opener's run of `=` made them.
inline constexpr std::string_view strip_long(
    std::string_view t, std::pmr::memory_resource* memory) {
  const std::size_t w = t.find('[', 1) + 1;
  std::string_view inner = t.substr(w, t.size() - 2 * w);
  if (!inner.empty() && (inner.front() == '\n' || inner.front() == '\r')) {
    const char first = inner.front();
    inner.remove_prefix(1);
    if (!inner.empty() && (inner.front() == '\n' || inner.front() == '\r') &&
        inner.front() != first)
      inner.remove_prefix(1);
  }
  if (inner.find('\r') == inner.npos)
    return inner;
  const auto room = cursiv::text_room(memory, inner.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < inner.size(); ++i) {
    const char c = inner[i];
    if (c == '\n' || c == '\r') {
      if (i + 1 < inner.size() && (inner[i + 1] == '\n' || inner[i + 1] == '\r') &&
          inner[i + 1] != c)
        ++i;
      room[size++] = '\n';
    } else {
      room[size++] = c;
    }
  }
  return {room.data(), size};
}

struct Teal : cursiv::GrammarSpec {
  static constexpr auto Space = cr<R"( \t\r\n\f\v)">;
  struct WS : terminal<some(Space)> {};

  // Long brackets `[=*[ ... ]=*]`: the closer repeats the opener's run of
  // `=`, at any level. The fragment shares the raw text pattern with COMMENT.
  struct Level {};
  static constexpr auto Bracket =
      through(lit<"["> + mark<Level>(many(cr<"=">)) + lit<"[">, lit<"]"> + same<Level> + lit<"]">);
  struct LONGSTRING : terminal<as<strip_long>(Bracket)> {};

  static constexpr auto safeBracket = lit<"["> + many(lit<"=">) + !cr<"[=">;
  static constexpr auto commentFirst = cr<R"(^\r\n[)"> | safeBracket;
  struct COMMENT
      : terminal<lit<"--"> +
                 option(Bracket | (commentFirst + many(cr<R"(^\r\n)">)))> {};

  struct SHEBANG : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto Reserved =
      kw<"and"> | kw<"break"> | kw<"do"> | kw<"else"> | kw<"elseif"> |
      kw<"end"> | kw<"false"> | kw<"for"> | kw<"function"> | kw<"goto"> |
      kw<"if"> | kw<"in"> | kw<"local"> | kw<"nil"> | kw<"not"> | kw<"or"> |
      kw<"repeat"> | kw<"return"> | kw<"then"> | kw<"true"> |
      kw<"until"> | kw<"while">;
  struct ID : terminal<!Reserved + cr<"a-zA-Z_"> + many(w)> {};
  // Type words are contextual identifiers; the following tokens decide
  // whether a word starts a declaration or denotes an ordinary value.
  template <cursiv::str Word>
  struct ContextWord : rule<&lit<Word> + ref<ID>> {};

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

  // Preserve short-string spelling; the converter validates escape values.
  static constexpr auto escape =
      lit<"\\"> + ((lit<"z"> + many(Space)) |
      (lit<"\r"> + option(lit<"\n">)) | (lit<"\n"> + option(lit<"\r">)) | any);
  struct NORMALSTRING
      : terminal<as<strip1>(lit<"\""> + many(escape | cr<R"(^"\\\r\n)">) +
                            lit<"\"">)> {};
  struct CHARSTRING
      : terminal<as<strip1>(lit<"'"> + many(escape | cr<R"(^'\\\r\n)">) +
                            lit<"'">)> {};
  static constexpr auto StrText =
      ref<NORMALSTRING> | ref<CHARSTRING> | ref<LONGSTRING>;

  struct StringExpr
      : rule<create<ast::StringExpr>(
            assign<&ast::StringExpr::value>(StrText))> {};

  // Operators, each its own token carrying the constant it denotes rather
  // than its text.


  struct Typ;
  struct NewType;
  struct Expression;
  struct Block;
  struct FuncBody;
  struct TableExpr;
  struct PrefixExpr;

  struct TypeArgs
      : rule<create<ast::TypeArgs>(
            lit<"<"> + some(append<&ast::TypeArgs::names>(ref<ID>), lit<",">) +
            lit<">">)> {};

  // `partype`: an optional name ahead of a required type.
  struct Partype
      : rule<create<ast::Param>(
            option(assign<&ast::Param::name>(ref<ID>) + lit<":">) +
            assign<&ast::Param::type>(ref<Typ>))> {};

  // `parname`: a required name, optionally typed.
  struct Parname
      : rule<create<ast::Param>(
            assign<&ast::Param::name>(ref<ID>) +
            option(lit<":"> + assign<&ast::Param::type>(ref<Typ>)))> {};

  struct RetList : rule<create<ast::RetList>(
      (lit<"("> + option(some(append<&ast::RetList::types>(ref<Typ>), lit<",">)) + option(flag<&ast::RetList::vararg>(lit<"...">)) + lit<")">) |
      (some(append<&ast::RetList::types>(ref<Typ>), lit<",">) + option(flag<&ast::RetList::vararg>(lit<"...">))))> {};

  struct FunctionType
      : rule<create<ast::FunctionSigType>(
            option(assign<&ast::FunctionSigType::typeParams>(
                ref<TypeArgs>)) +
            lit<"("> +
            option(some(append<&ast::FunctionSigType::params>(
                            ref<Partype>),
                        lit<",">)) +
            lit<")"> +
            option(lit<":"> +
                   assign<&ast::FunctionSigType::rets>(ref<RetList>)))> {
  };

  struct BaseType
      : rule<create<ast::PrimitiveType>(assign<&ast::PrimitiveType::kind>(
                 kw<"string"> | kw<"boolean"> | kw<"nil"> | kw<"number">))  //
             | create<ast::MapType>(
                   lit<"{"> + assign<&ast::MapType::key>(ref<Typ>) +
                   lit<":"> + assign<&ast::MapType::value>(ref<Typ>) +
                   lit<"}">)  //
             | create<ast::ArrayType>(
                   lit<"{"> + assign<&ast::ArrayType::element>(ref<Typ>) +
                   lit<"}">)  //
             | (kw<"function"> + ref<FunctionType>)  //
             | create<ast::NamedType>(
                   assign<&ast::NamedType::name>(ref<ID>) +
                   option(assign<&ast::NamedType::typeArgs>(
                       ref<TypeArgs>)))> {};

  // `typ`: a union of one or more base types, always this node -- a reader
  // checks `alternatives.size()` rather than a grammar-level special case
  // for the trivial one-element union.
  struct Typ
      : rule<(lit<"("> + ref<Typ> + lit<")">)  //
             | create<ast::UnionType>(
                   append<&ast::UnionType::alternatives>(ref<BaseType>) +
                   many(lit<"|"> + append<&ast::UnionType::alternatives>(
                                       ref<BaseType>)))> {};

  struct RecordBody;
  struct EnumBody;
  struct AliasBody;
  template <auto Name, auto Value>
  static constexpr auto type_binding =
      (ref<ContextWord<"record">> + assign<Name>(ref<ID>) + assign<Value>(ref<RecordBody>)) |
      (ref<ContextWord<"enum">> + assign<Name>(ref<ID>) + assign<Value>(ref<EnumBody>)) |
      (ref<ContextWord<"type">> + assign<Name>(ref<ID>) + assign<Value>(ref<AliasBody>));

  struct NamedNewType
      : rule<type_binding<&ast::NamedNewType::name, &ast::NamedNewType::value> |
             (assign<&ast::NamedNewType::name>(ref<ID>) + lit<"="> +
              assign<&ast::NamedNewType::value>(ref<NewType>))> {};
  struct RecordField
      : rule<create<ast::Param>(assign<&ast::Param::name>(ref<ID>) + lit<":"> +
                                assign<&ast::Param::type>(ref<Typ>))> {};
  static constexpr auto record_body =
      option(assign<&ast::RecordNewType::typeParams>(ref<TypeArgs>)) +
      option(lit<"{"> + assign<&ast::RecordNewType::arrayPart>(ref<Typ>) + lit<"}">) +
      many(append<&ast::RecordNewType::nestedTypes>(ref<NamedNewType>) |
           append<&ast::RecordNewType::fields>(ref<RecordField>)) + kw<"end">;
  static constexpr auto enum_body =
      many(append<&ast::EnumNewType::values>(StrText)) + kw<"end">;
  struct RecordBody : rule<create<ast::RecordNewType> + record_body> {};
  struct EnumBody : rule<create<ast::EnumNewType> + enum_body> {};
  struct AliasBody
      : rule<create<ast::AliasNewType> +
             option(assign<&ast::AliasNewType::typeParams>(ref<TypeArgs>)) + lit<"="> +
             assign<&ast::AliasNewType::type>(ref<Typ>)> {};
  struct NewType
      : rule<create<ast::RecordNewType>(
                 flag<&ast::NewType::legacy>(ref<ContextWord<"record">>) + record_body) |
             create<ast::EnumNewType>(
                 flag<&ast::NewType::legacy>(ref<ContextWord<"enum">>) + enum_body) |
             create<ast::FuncNewType>(flag<&ast::NewType::legacy>(ref<ContextWord<"functiontype">>) +
                 assign<&ast::FuncNewType::signature>(ref<FunctionType>))> {};

  static constexpr auto ArgsBody =
      (lit<"("> +
       option(some(append<&ast::Call::args>(ref<Expression>), lit<",">)) +
       lit<")">)                                     //
      | append<&ast::Call::args>(ref<TableExpr>)  //
      | append<&ast::Call::args>(ref<StringExpr>);

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

  struct Field
      : rule<create<ast::TableField>(
                 lit<"["> + assign<&ast::TableField::key>(ref<Expression>) +
                 lit<"]"> + lit<"="> +
                 assign<&ast::TableField::value>(ref<Expression>))  //
             | create<ast::TableField>(
                   assign<&ast::TableField::name>(ref<ID>) +
                   option(lit<":"> +
                          assign<&ast::TableField::fieldType>(ref<Typ>)) +
                   lit<"="> +
                   assign<&ast::TableField::value>(ref<Expression>))  //
             | create<ast::TableField>(
                   assign<&ast::TableField::name>(ref<ID>) + lit<"="> +
                   assign<&ast::TableField::newType>(ref<NewType>))  //
             | create<ast::TableField>(
                   assign<&ast::TableField::value>(ref<Expression>))> {};

  struct TableExpr
      : rule<create<ast::TableExpr>(
            lit<"{"> +
            option(some(append<&ast::TableExpr::fields>(ref<Field>),
                        (lit<","> | lit<";">)) +
                   option(lit<","> | lit<";">)) +
            lit<"}">)> {};

  static constexpr auto Vararg =
      flag<&ast::FuncBody::vararg>(lit<"...">) +
      option(lit<":"> + assign<&ast::FuncBody::varargType>(ref<Typ>));

  struct FuncBody
      : rule<create<ast::FuncBody>(
            option(assign<&ast::FuncBody::typeParams>(ref<TypeArgs>)) +
            lit<"("> +
            option((some(append<&ast::FuncBody::params>(ref<Parname>),
                         lit<",">) +
                    option(lit<","> + Vararg)) |
                   Vararg) +
            lit<")"> +
            option(lit<":"> + assign<&ast::FuncBody::rets>(ref<RetList>)) +
            assign<&ast::FuncBody::body>(ref<Block>) + kw<"end">)> {};

  struct AtomPrimary
      : rule<create<ast::NilExpr>(kw<"nil">)  //
             | create<ast::BoolExpr>(
                   flag<&ast::BoolExpr::value>(kw<"true">) | kw<"false">)  //
             | create<ast::VarargExpr>(lit<"...">)  //
             | create<ast::NumberExpr>(assign<&ast::NumberExpr::text>(ref<NUMBER>))  //
             | ref<StringExpr>  //
             | create<ast::FunctionExpr>(
                   kw<"function"> +
                   assign<&ast::FunctionExpr::body>(ref<FuncBody>))  //
             | ref<TableExpr>  //
             | create<ast::IsExpr>(assign<&ast::IsExpr::name>(ref<ID>) +
                                   ref<ContextWord<"is">> +
                                   assign<&ast::IsExpr::type>(ref<Typ>))  //
             | ref<PrefixExpr>> {};

  // `as` binds tighter than every other operator, including `^`.
  struct AtomAs
      : rule<ref<AtomPrimary> +
             many(nest<ast::AsExpr, &ast::AsExpr::expr>(
                 ref<ContextWord<"as">> + assign<&ast::AsExpr::type>(ref<Typ>)))> {};

  // `^` binds tighter than a unary operator, but a unary operand may itself
  // hold a `^` chain (`-2^2` is `-(2^2)`); the alternatives below are the
  // whole of that, not a table entry.
  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::UnOp::lnot>(kw<"not">) | as<ast::UnOp::len>(lit<"#">) |
                                             as<ast::UnOp::neg>(lit<"-">) | as<ast::UnOp::bnot>(lit<"~">)) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | create<ast::BinaryExpr>(
                   assign<&ast::BinaryExpr::left>(ref<AtomAs>) +
                   assign<&ast::BinaryExpr::op>(as<ast::BinOp::pow>(lit<"^">)) +
                   assign<&ast::BinaryExpr::right>(ref<Unary>))  //
             | ref<AtomAs>> {};

  // The bitwise level sits between comparison and concatenation, as the Lua
  // manual places it (Teal follows Lua's precedence).
  struct Expression
      : infix<Unary, ast::BinaryExpr,
              levels(left(as<ast::BinOp::mul>(lit<"*">) | as<ast::BinOp::div>(lit<"/">) | as<ast::BinOp::idiv>(lit<"//">) |
                          as<ast::BinOp::mod>(lit<"%">)),               //
                     left(as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::sub>(lit<"-">)),  //
                     right(as<ast::BinOp::concat>(lit<"..">)),           //
                     left(as<ast::BinOp::shl>(lit<"<<">) | as<ast::BinOp::shr>(lit<">>">)),  //
                     left(as<ast::BinOp::band>(lit<"&">)),              //
                     left(as<ast::BinOp::bxor>(lit<"~">)),              //
                     left(as<ast::BinOp::bor>(lit<"|">)),               //
                     left(as<ast::BinOp::lt>(lit<"<">) | as<ast::BinOp::gt>(lit<">">) | as<ast::BinOp::le>(lit<"<=">) | as<ast::BinOp::ge>(lit<">=">) |
                          as<ast::BinOp::ne>(lit<"~=">) | as<ast::BinOp::eq>(lit<"==">)),    //
                     left(as<ast::BinOp::logical_and>(kw<"and">)),            //
                     left(as<ast::BinOp::logical_or>(kw<"or">)))> {};

  struct AttribName
      : rule<create<ast::AttribName>(
            assign<&ast::AttribName::name>(ref<ID>) +
            option(lit<"<"> + assign<&ast::AttribName::attrib>(ref<ID>) +
                   lit<">">))> {};

  struct ExprStat
      : rule<create<ast::AssignStat>(
                 some(append<&ast::AssignStat::targets>(ref<PrefixExpr>), lit<",">) +
                 lit<"="> +
                 some(append<&ast::AssignStat::values>(ref<Expression>),
                      lit<",">))  //
             | create<ast::CallStat>(
                   assign<&ast::CallStat::call>(ref<PrefixExpr>))> {};

  struct IfClause
      : rule<create<ast::IfClause>(
            assign<&ast::IfClause::cond>(ref<Expression>) + kw<"then"> +
            assign<&ast::IfClause::body>(ref<Block>))> {};

  struct IfStat
      : rule<create<ast::IfStat>(
            kw<"if"> + append<&ast::IfStat::clauses>(ref<IfClause>) +
            many(kw<"elseif"> +
                 append<&ast::IfStat::clauses>(ref<IfClause>)) +
            option(kw<"else"> + assign<&ast::IfStat::elseBody>(ref<Block>)) +
            kw<"end">)> {};

  struct NumericFor
      : rule<create<ast::NumericForStat>(
            kw<"for"> + assign<&ast::NumericForStat::name>(ref<ID>) +
            lit<"="> + assign<&ast::NumericForStat::start>(ref<Expression>) +
            lit<","> + assign<&ast::NumericForStat::stop>(ref<Expression>) +
            option(lit<","> +
                   assign<&ast::NumericForStat::step>(ref<Expression>)) +
            kw<"do"> + assign<&ast::NumericForStat::body>(ref<Block>) +
            kw<"end">)> {};

  struct GenericFor
      : rule<create<ast::GenericForStat>(
            kw<"for"> + some(append<&ast::GenericForStat::names>(ref<ID>), lit<",">) +
            kw<"in"> +
            some(append<&ast::GenericForStat::exprs>(ref<Expression>),
                 lit<",">) +
            kw<"do"> + assign<&ast::GenericForStat::body>(ref<Block>) +
            kw<"end">)> {};

  struct FuncStat
      : rule<create<ast::FuncStat>(
            kw<"function"> + assign<&ast::FuncStat::name>(ref<ID>) +
            many(lit<"."> + append<&ast::FuncStat::path>(ref<ID>)) +
            option(lit<":"> + assign<&ast::FuncStat::method>(ref<ID>)) +
            assign<&ast::FuncStat::body>(ref<FuncBody>))> {};

  struct LocalFuncStat
      : rule<create<ast::LocalFuncStat>(
            kw<"local"> + kw<"function"> +
            assign<&ast::LocalFuncStat::name>(ref<ID>) +
            assign<&ast::LocalFuncStat::body>(ref<FuncBody>))> {};

  struct GlobalFuncStat
      : rule<create<ast::GlobalFuncStat>(
            ref<ContextWord<"global">> + kw<"function"> +
            assign<&ast::GlobalFuncStat::name>(ref<ID>) +
            assign<&ast::GlobalFuncStat::body>(ref<FuncBody>))> {};

  struct LocalNewTypeStat
      : rule<create<ast::LocalNewTypeStat>(
            kw<"local"> +
            (type_binding<&ast::LocalNewTypeStat::name, &ast::LocalNewTypeStat::value> |
             (assign<&ast::LocalNewTypeStat::name>(ref<ID>) + lit<"="> +
              assign<&ast::LocalNewTypeStat::value>(ref<NewType>))))> {};

  struct LocalAttrAssignStat
      : rule<create<ast::LocalAttrAssignStat>(
            kw<"local"> +
            some(append<&ast::LocalAttrAssignStat::vars>(ref<AttribName>),
                 lit<",">) +
            option(
                lit<":"> +
                some(append<&ast::LocalAttrAssignStat::types>(ref<Typ>), lit<",">)) +
            option(
                lit<"="> +
                some(append<&ast::LocalAttrAssignStat::values>(ref<Expression>),
                     lit<",">)))> {};

  struct GlobalAssignStat
      : rule<create<ast::GlobalAssignStat>(
            ref<ContextWord<"global">> +
            (type_binding<&ast::GlobalAssignStat::name, &ast::GlobalAssignStat::value> |
             (assign<&ast::GlobalAssignStat::name>(ref<ID>) + lit<"="> +
              assign<&ast::GlobalAssignStat::value>(ref<NewType>))))> {};

  struct GlobalAttrStat
      : rule<create<ast::GlobalAttrStat>(
            ref<ContextWord<"global">> +
            some(append<&ast::GlobalAttrStat::vars>(ref<AttribName>),
                 lit<",">) +
            lit<":"> + some(append<&ast::GlobalAttrStat::types>(ref<Typ>), lit<",">))> {};

  struct GlobalAttrAssignStat
      : rule<create<ast::GlobalAttrAssignStat>(
            ref<ContextWord<"global">> +
            some(append<&ast::GlobalAttrAssignStat::vars>(ref<AttribName>),
                 lit<",">) +
            option(lit<":"> +
                   some(append<&ast::GlobalAttrAssignStat::types>(ref<Typ>), lit<",">)) +
            lit<"="> +
            some(append<&ast::GlobalAttrAssignStat::values>(ref<Expression>),
                 lit<",">))> {};

  struct Stat
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
             | ref<IfStat>              //
             | ref<NumericFor>          //
             | ref<GenericFor>          //
             | ref<FuncStat>            //
             | ref<LocalFuncStat>       //
             | ref<LocalNewTypeStat>    //
             | ref<LocalAttrAssignStat>  //
             | ref<GlobalFuncStat>      //
             | ref<GlobalAssignStat>    //
             | ref<GlobalAttrStat>      //
             | ref<GlobalAttrAssignStat>> {};

  struct ReturnStat
      : rule<create<ast::ReturnStat>(
            kw<"return"> +
            option(some(append<&ast::ReturnStat::values>(ref<Expression>),
                        lit<",">)) +
            option(lit<";">))> {};

  // A lone `;` is an empty statement, dropped rather than given its own
  // node, like real Lua's own C parser does.
  struct Block
      : rule<many(lit<";"> | append<&ast::Block::statements>(ref<Stat>)) +
             option(append<&ast::Block::statements>(ref<ReturnStat>))> {};

  struct Chunk : rule<option(ref<SHEBANG>) + ref<Block>> {};

  using entry = Chunk;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace teal
