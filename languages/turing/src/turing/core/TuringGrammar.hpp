// The turing language: its grammar.
#pragma once

#include <cstdint>
#include <limits>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "turing/core/ast.hpp"

namespace turing {

struct Turing : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct SL_COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};

  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  inline static constexpr auto identifier_value = [](std::string_view text)
      -> cursiv::ConversionResult<std::string_view> {
    using Result = cursiv::ConversionResult<std::string_view>;
    if (text.size() > 50)
      return Result::err("A Turing identifier has at most 50 characters.");
    constexpr std::string_view reserved[] = {
        "all", "and", "array", "assert", "begin", "bind", "body", "boolean",
        "case", "collection", "const", "decreasing", "div", "else", "elsif",
        "end", "enum", "exit", "export", "false", "fcn", "for", "forward",
        "free", "function", "get", "if", "import", "in", "init", "int",
        "invariant", "label", "loop", "mod", "module", "new", "not", "of",
        "opaque", "or", "pervasive", "pointer", "post", "pre", "proc",
        "procedure", "put", "real", "record", "result", "return", "set",
        "skip", "string", "tag", "then", "to", "true", "type", "union",
        "var", "when"};
    for (const auto word : reserved)
      if (text == word) return Result::err("A Turing keyword is reserved.");
    return Result::ok(text);
  };
  struct ID : terminal<as<identifier_value>(cr<"a-zA-Z"> + many(w))> {};

  // Signs are prefix operators and may be separated from their operand.
  struct INT : terminal<as<std::int64_t>(some(d))> {};
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  // The point guard leaves a subrange's two dots intact.
  struct REAL : terminal<as<double>(
      (((some(d) + lit<".", !lit<".">> + many(d)) |
        (lit<"."> + some(d))) + option(exponent)) |
      (some(d) + exponent))> {};

  inline static constexpr auto string_value = [](
      std::string_view quoted, std::pmr::memory_resource* memory)
      -> cursiv::ConversionResult<std::string_view> {
    using Result = cursiv::ConversionResult<std::string_view>;
    if (quoted.size() < 2) return Result::err("Unterminated Turing string.");
    const auto text = quoted.substr(1, quoted.size() - 2);
    if (text.find('\\') == text.npos) return Result::ok(text);
    constexpr std::string_view escapes = "nNtTfFrRbBeEdD\\\"";
    constexpr std::string_view values = "\n\n\t\t\f\f\r\r\b\b\x1b\x1b\x7f\x7f\\\"";
    const auto room = cursiv::text_room(memory, text.size());
    std::size_t count = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
      char value = text[i];
      if (value == '\\') {
        if (++i == text.size()) return Result::err("Incomplete Turing escape.");
        const auto index = escapes.find(text[i]);
        if (index == escapes.npos) return Result::err("Unknown Turing escape.");
        value = values[index];
      }
      room[count++] = value;
    }
    return Result::ok(std::string_view(room.data(), count));
  };
  struct STRING : terminal<as<string_value>(
      lit<"\""> + many((lit<"\\"> + cr<R"(nNtTfFrRbBeEdD\\")">) |
                         cr<R"(^"\\\r\n\x00)">) + lit<"\"">)> {};

  // Each operator token denotes its own constant, so `op` holds meaning
  // rather than spelling. Subtraction and unary minus use ASCII `-`,
  // matching the numeric-literal tokens below.


  struct Expression;
  struct TypeSpec;
  struct Item;

  // One repeated append covers a field access, an index/call, or a
  // substring range.
  struct FieldSelector
      : rule<create<ast::FieldSelector>(
            lit<"."> + assign<&ast::FieldSelector::field>(ref<ID>))> {};

  struct IndexSelector
      : rule<create<ast::IndexSelector>(
            lit<"("> +
            some(append<&ast::IndexSelector::args>(ref<Expression>), lit<",">) +
            lit<")">)> {};

  struct StarPosition
      : rule<create<ast::Position>(
            flag<&ast::Position::fromEnd>(lit<"*">) +
            option(lit<"-"> +
                   assign<&ast::Position::offset>(ref<Expression>)))> {};
  struct PlainPosition
      : rule<assign<&ast::Position::value>(ref<Expression>)> {};
  static constexpr auto Position =
      ref<StarPosition> | ref<PlainPosition>;

  // A range needs `*` or an explicit `..` to be told apart from a plain
  // one-argument index/call -- a lone value is genuinely ambiguous between
  // the two, and this keeps that case an index.
  struct RangeSelector
      : rule<lit<"("> +
            create<ast::RangeSelector>(
                (assign<&ast::RangeSelector::from>(ref<StarPosition>) +
                 option(lit<".."> +
                        assign<&ast::RangeSelector::to>(Position))) |
                (assign<&ast::RangeSelector::from>(ref<PlainPosition>) +
                 lit<".."> + assign<&ast::RangeSelector::to>(Position))) +
            lit<")">> {};

  struct Selector
      : rule<ref<FieldSelector> | ref<IndexSelector> |
            ref<RangeSelector>> {};

  struct Reference
      : rule<create<ast::Reference>(
            assign<&ast::Reference::name>(ref<ID>) +
            many(append<&ast::Reference::selectors>(ref<Selector>)))> {};

  struct IntLit
      : rule<assign<&ast::IntLit::value>(ref<INT>)> {};
  struct RealLit
      : rule<assign<&ast::RealLit::value>(ref<REAL>)> {
  };
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};
  struct BoolLit
      : rule<create<ast::BoolLit>(
            flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};

  struct Primary
      : rule<create<ast::ParenExpr>(
                 lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                 lit<")">) |
            ref<RealLit> | ref<IntLit> | ref<StringLit> |
            ref<BoolLit> |
            ref<Reference>> {};

  static constexpr auto sign =
      as<ast::UnOp::plus>(lit<"+">) | as<ast::UnOp::minus>(lit<"-">);
  // Keep the signed 64-bit minimum representable without accepting an
  // overflowing positive magnitude in INT.
  struct MinimumInt : rule<assign<&ast::IntLit::value>(
      as<std::numeric_limits<std::int64_t>::min()>(
          lit<"-"> + lit<"9223372036854775808", !w>))> {};
  struct SignedPrimary : rule<ref<MinimumInt> |
      (assign<&ast::UnaryExpr::op>(sign) +
       assign<&ast::UnaryExpr::operand>(ref<SignedPrimary>)) | ref<Primary>> {};
  struct Power : rule<ref<Primary> + many(nest<&ast::BinaryExpr::left>(
      assign<&ast::BinaryExpr::op>(as<ast::BinOp::exponent>(lit<"**">)) +
      assign<&ast::BinaryExpr::right>(ref<SignedPrimary>)))> {};
  struct Unary : rule<ref<MinimumInt> |
      (assign<&ast::UnaryExpr::op>(sign) +
       assign<&ast::UnaryExpr::operand>(ref<Unary>)) | ref<Power>> {};
  struct Relational : infix<Unary, ast::BinaryExpr,
      levels(left(as<ast::BinOp::multiply>(lit<"*">) |
                  as<ast::BinOp::divide>(lit<"/">) |
                  as<ast::BinOp::intDivide>(kw<"div">) |
                  as<ast::BinOp::modulo>(kw<"mod">)),
             left(as<ast::BinOp::add>(lit<"+">) |
                  as<ast::BinOp::subtract>(lit<"-">)),
             left(as<ast::BinOp::lessEqual>(lit<"<=">) |
                  as<ast::BinOp::greaterEqual>(lit<">=">) |
                  as<ast::BinOp::notEqual>(lit<"not=">) |
                  as<ast::BinOp::equal>(lit<"=">) |
                  as<ast::BinOp::less>(lit<"<">) |
                  as<ast::BinOp::greater>(lit<">">) |
                  as<ast::BinOp::member>(kw<"in">) |
                  as<ast::BinOp::notMember>(kw<"not"> + kw<"in">)))> {};
  struct Not : rule<
      (assign<&ast::UnaryExpr::op>(as<ast::UnOp::logicalNot>(kw<"not">)) +
       assign<&ast::UnaryExpr::operand>(ref<Not>)) | ref<Relational>> {};
  // The report specifies left association, including exponentiation; `not`
  // binds less tightly than relations and more tightly than `and`.
  struct Expression : infix<Not, ast::BinaryExpr,
      levels(left(as<ast::BinOp::logicalAnd>(kw<"and">)),
             left(as<ast::BinOp::logicalOr>(kw<"or">)),
             left(as<ast::BinOp::implies>(lit<"->">)))> {};

  // An initializer is a plain value, or `init(...)` nesting more of the same
  // for an array or record.
  struct InitList
      : rule<create<ast::InitList>(
            kw<"init"> + lit<"("> +
            some(append<&ast::InitList::items>(ref<Expression> |
                                                ref<InitList>),
                 lit<",">) +
            lit<")">)> {};
  static constexpr auto InitValue = ref<Expression> | ref<InitList>;

  struct StandardType
      : rule<create<ast::StandardType>(
            assign<&ast::StandardType::kind>(as<ast::StandardType::Kind::int_>(kw<"int">) | as<ast::StandardType::Kind::real>(kw<"real">) |
                                              as<ast::StandardType::Kind::boolean>(kw<"boolean">)) |
            (assign<&ast::StandardType::kind>(as<ast::StandardType::Kind::string>(kw<"string">)) +
             option(lit<"("> +
                    assign<&ast::StandardType::length>(ref<Expression>) +
                    lit<")">)))> {};

  struct SubrangeType
      : rule<create<ast::SubrangeType>(
            assign<&ast::SubrangeType::low>(ref<Expression>) + lit<".."> +
            assign<&ast::SubrangeType::high>(ref<Expression>))> {};

  struct EnumType
      : rule<kw<"enum"> + lit<"("> +
             some(append<&ast::EnumType::names>(ref<ID>), lit<",">) + lit<")">> {};

  struct TypeName : rule<ref<ID> + option(lit<"."> + ref<ID>)> {};
  struct NamedType
      : rule<assign<&ast::NamedType::name>(ref<TypeName>)> {};

  static constexpr auto IndexType = ref<SubrangeType> | ref<EnumType> | ref<NamedType>;

  struct SetType
      : rule<kw<"set"> + kw<"of"> + assign<&ast::SetType::base>(IndexType)> {};
  struct PointerType
      : rule<kw<"pointer"> + kw<"to"> + assign<&ast::PointerType::collection>(ref<ID>)> {};

  struct ArrayType
      : rule<create<ast::ArrayType>(
            kw<"array"> +
            some(append<&ast::ArrayType::indices>(IndexType), lit<",">) +
            kw<"of"> +
            assign<&ast::ArrayType::element>(ref<TypeSpec>))> {};

  struct RecordField
      : rule<create<ast::RecordField>(
            some(append<&ast::RecordField::names>(ref<ID>), lit<",">) +
            lit<":"> +
            assign<&ast::RecordField::type>(ref<TypeSpec>))> {};

  struct RecordType
      : rule<create<ast::RecordType>(
            kw<"record"> +
            some(append<&ast::RecordType::fields>(ref<RecordField>)) +
            kw<"end"> + kw<"record">)> {};

  struct TypeSpec
      : rule<ref<StandardType> | ref<SubrangeType> |
            ref<EnumType> | ref<ArrayType> | ref<SetType> | ref<RecordType> |
            ref<PointerType> | ref<NamedType>> {};

  struct StringStarType
      : rule<create<ast::StringStarType> + kw<"string"> + lit<"("> +
            lit<"*"> + lit<")">> {};

  struct SimpleParamType
      : rule<create<ast::SimpleParamType>(
            assign<&ast::SimpleParamType::type>(ref<TypeSpec> |
                                                 ref<StringStarType>))> {
  };

  struct OpenArrayParamType
      : rule<create<ast::OpenArrayParamType>(
            kw<"array"> +
            some(append<&ast::OpenArrayParamType::bounds>(ref<Expression>) +
                     lit<".."> + lit<"*">,
                 lit<",">) +
            kw<"of"> +
            assign<&ast::OpenArrayParamType::element>(
                ref<TypeSpec> | ref<StringStarType>))> {};

  struct ParamType
      : rule<ref<SimpleParamType> | ref<OpenArrayParamType>> {};

  struct Param
      : rule<create<ast::Param>(
            option(flag<&ast::Param::isVar>(kw<"var">)) +
            some(append<&ast::Param::names>(ref<ID>), lit<",">) + lit<":"> +
            assign<&ast::Param::type>(ref<ParamType>))> {};

  // The two forms real Turing declares: a procedure has no return type, a
  // function's header ends in `: typeSpec`.
  struct SubprogramDecl
      : rule<create<ast::SubprogramDecl>(
                 (kw<"procedure"> | kw<"proc">) +
                 assign<&ast::SubprogramDecl::name>(ref<ID>) +
                 option(lit<"("> +
                        some(append<&ast::SubprogramDecl::params>(
                                 ref<Param>),
                             lit<",">) +
                        lit<")">) +
                 many(append<&ast::SubprogramDecl::body>(ref<Item>)) +
                 kw<"end"> +
                 assign<&ast::SubprogramDecl::endName>(ref<ID>)) |
            create<ast::SubprogramDecl>(
                flag<&ast::SubprogramDecl::isFunction>(
                    kw<"function"> | kw<"fcn">) +
                assign<&ast::SubprogramDecl::name>(ref<ID>) +
                option(lit<"("> +
                       some(append<&ast::SubprogramDecl::params>(
                                ref<Param>),
                            lit<",">) +
                       lit<")">) +
                lit<":"> +
                assign<&ast::SubprogramDecl::returnType>(ref<TypeSpec>) +
                many(append<&ast::SubprogramDecl::body>(ref<Item>)) +
                kw<"end"> +
                assign<&ast::SubprogramDecl::endName>(ref<ID>))> {};

  struct ConstDecl
      : rule<create<ast::ConstDecl>(
            kw<"const"> + assign<&ast::ConstDecl::name>(ref<ID>) +
            option(lit<":"> +
                   assign<&ast::ConstDecl::typeSpec>(ref<TypeSpec>)) +
            lit<":="> + assign<&ast::ConstDecl::value>(InitValue))> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
            kw<"var"> +
            some(append<&ast::VarDecl::names>(ref<ID>), lit<",">) +
            ((lit<":="> + assign<&ast::VarDecl::value>(InitValue)) |
             (lit<":"> +
              assign<&ast::VarDecl::typeSpec>(ref<TypeSpec>) +
              option(lit<":="> +
                     assign<&ast::VarDecl::value>(InitValue)))))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            kw<"type"> + assign<&ast::TypeDecl::name>(ref<ID>) + lit<":"> +
            assign<&ast::TypeDecl::typeSpec>(ref<TypeSpec>))> {};

  struct Decl
      : rule<ref<ConstDecl> | ref<VarDecl> | ref<TypeDecl>> {};

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<Reference>) +
            lit<":="> + assign<&ast::AssignStmt::value>(ref<Expression>))> {
  };
  struct CallStmt
      : rule<assign<&ast::CallStmt::call>(ref<Reference>)> {};
  struct AssertStmt
      : rule<create<ast::AssertStmt>(
            kw<"assert"> + assign<&ast::AssertStmt::cond>(ref<Expression>))> {
  };
  struct ResultStmt
      : rule<create<ast::ResultStmt>(
            kw<"result"> +
            assign<&ast::ResultStmt::value>(ref<Expression>))> {};

  struct ElsifClause
      : rule<create<ast::ElsifClause>(
            kw<"elsif"> + assign<&ast::ElsifClause::cond>(ref<Expression>) +
            kw<"then"> +
            many(append<&ast::ElsifClause::body>(ref<Item>)))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"if"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            kw<"then"> +
            many(append<&ast::IfStmt::thenBody>(ref<Item>)) +
            many(append<&ast::IfStmt::elsifs>(ref<ElsifClause>)) +
            option(kw<"else"> +
                   many(append<&ast::IfStmt::elseBody>(ref<Item>))) +
            kw<"end"> + kw<"if">)> {};

  struct LoopStmt
      : rule<create<ast::LoopStmt>(
            kw<"loop"> + many(append<&ast::LoopStmt::body>(ref<Item>)) +
            kw<"end"> + kw<"loop">)> {};

  struct ExitStmt
      : rule<create<ast::ExitStmt>(
            kw<"exit"> +
            option(kw<"when"> +
                   assign<&ast::ExitStmt::cond>(ref<Expression>)))> {};

  // The validator checks that the optional default label is last.
  struct CaseLabel
      : rule<create<ast::CaseLabel>(
            kw<"label"> +
            ((some(append<&ast::CaseLabel::values>(ref<Expression>), lit<",">) +
              lit<":">) |
             lit<":">) +
            many(append<&ast::CaseLabel::body>(ref<Item>)))> {};

  struct CaseStmt
      : rule<create<ast::CaseStmt>(
            kw<"case"> + assign<&ast::CaseStmt::value>(ref<Expression>) +
            kw<"of"> +
            some(append<&ast::CaseStmt::labels>(ref<CaseLabel>)) +
            kw<"end"> + kw<"case">)> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            kw<"for"> +
            option(flag<&ast::ForStmt::decreasing>(kw<"decreasing">)) +
            assign<&ast::ForStmt::varName>(ref<ID>) + lit<":"> +
            assign<&ast::ForStmt::from>(ref<Expression>) + lit<".."> +
            assign<&ast::ForStmt::to>(ref<Expression>) +
            option(kw<"by"> +
                   assign<&ast::ForStmt::step>(ref<Expression>)) +
            many(append<&ast::ForStmt::body>(ref<Item>)) + kw<"end"> +
            kw<"for">)> {};

  struct PutItem
      : rule<create<ast::PutItem>(kw<"skip">) |
            create<ast::PutItem>(
                assign<&ast::PutItem::value>(ref<Expression>) +
                option(lit<":"> +
                       assign<&ast::PutItem::width>(ref<Expression>) +
                       option(lit<":"> +
                              assign<&ast::PutItem::fraction>(
                                  ref<Expression>) +
                              option(lit<":"> +
                                     assign<&ast::PutItem::exponent>(
                                         ref<Expression>)))))> {};

  struct PutStmt
      : rule<create<ast::PutStmt>(
            kw<"put"> +
            option(lit<":"> +
                   assign<&ast::PutStmt::stream>(ref<Expression>) +
                   lit<",">) +
            some(append<&ast::PutStmt::items>(ref<PutItem>), lit<",">) +
            option(flag<&ast::PutStmt::noNewline>(lit<"..">)))> {};

  struct GetItem
      : rule<create<ast::GetItem>(
                 flag<&ast::GetItem::skipToEnd>(kw<"skip">) +
                 assign<&ast::GetItem::target>(ref<Reference>) +
                 lit<":"> + lit<"*">) |
            create<ast::GetItem>(
                assign<&ast::GetItem::target>(ref<Reference>) +
                option(lit<":"> +
                       assign<&ast::GetItem::width>(ref<Expression>)))> {};

  struct GetStmt
      : rule<create<ast::GetStmt>(
            kw<"get"> +
            option(lit<":"> +
                   assign<&ast::GetStmt::stream>(ref<Expression>) +
                   lit<",">) +
            some(append<&ast::GetStmt::items>(ref<GetItem>), lit<",">))> {
  };

  struct OpenStmt
      : rule<create<ast::OpenStmt>(
            kw<"open"> + lit<":"> +
            assign<&ast::OpenStmt::fileNumber>(ref<Expression>) + lit<","> +
            assign<&ast::OpenStmt::path>(ref<STRING>) + lit<","> +
            some(append<&ast::OpenStmt::capabilities>(as<ast::Capability::get_>(kw<"get">) |
                                                        as<ast::Capability::put_>(kw<"put">)),
                 lit<",">))> {};

  struct CloseStmt
      : rule<create<ast::CloseStmt>(
            kw<"close"> + lit<":"> +
            assign<&ast::CloseStmt::fileNumber>(ref<Expression>))> {};

  struct ReturnStmt : rule<create<ast::ReturnStmt> + kw<"return">> {};
  struct BeginStmt : rule<kw<"begin"> + create<ast::BeginStmt>(
      many(append<&ast::BeginStmt::body>(ref<Item>))) + kw<"end">> {};

  struct Stmt
      : rule<ref<AssignStmt> | ref<CallStmt> | ref<AssertStmt> |
            ref<ResultStmt> | ref<ReturnStmt> | ref<BeginStmt> | ref<IfStmt> | ref<LoopStmt> |
            ref<ExitStmt> | ref<CaseStmt> | ref<ForStmt> |
            ref<PutStmt> | ref<GetStmt> | ref<OpenStmt> |
            ref<CloseStmt>> {};

  struct Item : rule<(ref<Decl> | ref<Stmt>) + option(lit<";">)> {};

  struct ProgramItem
      : rule<(ref<Decl> | ref<Stmt> | ref<SubprogramDecl>) + option(lit<";">)> {};

  struct Program
      : rule<create<ast::Program>(
            many(append<&ast::Program::items>(ref<ProgramItem>)))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace turing
