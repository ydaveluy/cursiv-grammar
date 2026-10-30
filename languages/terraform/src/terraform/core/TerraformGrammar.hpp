#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "terraform/core/StringValue.hpp"
#include "terraform/core/ast.hpp"
namespace terraform {
inline constexpr auto trim_open = [](std::string_view text) {
  return text.size() > 2 && text[2] == '~';
};
inline constexpr auto decimal_index = [](std::string_view text)
    -> cursiv::ConversionResult<std::string_view> {
  for (const char c : text) if (c < '0' || c > '9')
    return cursiv::ConversionResult<std::string_view>::err("A legacy dot index contains decimal digits only");
  return cursiv::ConversionResult<std::string_view>::ok(text);
};
struct HeredocTag {};
struct Terraform : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<(lit<"#"> | lit<"//">)+many(cr<R"(^\n)">)> {
  };
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  static constexpr auto IdStart =
      uc::L | uc::Nl | cr<"_"> | cr<R"(\u1885\u1886\u2118\u212e\u309b\u309c)">;
  static constexpr auto IdContinue = IdStart | uc::Mn | uc::Mc | uc::Nd |
                                     uc::Pc |
                                     cr<R"(-\u00b7\u0387\u1369-\u1371\u19da)">;
  struct ID : terminal<IdStart + many(IdContinue)> {};
  struct TRUE : terminal<kw<"true">> {};
  struct FALSE : terminal<kw<"false">> {};
  struct NULL_KW : terminal<kw<"null">> {};
  struct FOR : terminal<kw<"for">> {};
  struct IN : terminal<kw<"in">> {};
  struct IF : terminal<kw<"if">> {};
  static constexpr auto Name = ref<ID> | ref<TRUE> | ref<FALSE> | ref<NULL_KW> |
                               ref<FOR> | ref<IN> | ref<IF>;
  struct NUMBER : terminal<some(d) + option(lit<"."> + some(d)) +
                           option(cr<"eE"> + option(cr<"+-">) + some(d))> {};
  static constexpr auto Escaped = lit<"\\"> + any;
  struct INTERP_OPEN
      : terminal<as<trim_open>(lit<"${"> + option(lit<"~">) + many(s))> {};
  struct DIRECTIVE_OPEN
      : terminal<as<trim_open>(lit<"%{"> + option(lit<"~">) + many(s))> {};
  struct STRING_TEXT
      : terminal<some(Escaped | (lit<"$${"> | lit<"%%{">) |
                      (lit<"$", !lit<"{">>) | (lit<"%", !lit<"{">>) |
                      cr<R"(^"\\$%\r\n)">)> {};
  struct HEREDOC
      : terminal<(lit<"<<"> + option(lit<"-">) +
                  &IdStart + mark<HeredocTag>(some(IdContinue)) +
                  option(lit<"\r">) +
      lit<"\n">)+many(!(many(cr<R"( \t)">) + same<HeredocTag> + many(cr<R"( \t)">) +
                                    &((lit<"\r\n"> | lit<"\n">) | eof)) +
                                  many(cr<R"(^\r\n)">) + option(lit<"\r">) +
                                  lit<"\n">) +
                 many(cr<R"( \t)">) + same<HeredocTag> + many(cr<R"( \t)">) +
                 &((lit<"\r\n"> | lit<"\n">) | eof)> {};
  struct AND : terminal<as<ast::Operator::logical_and>(lit<"&&">)> {};

  static constexpr auto OpMul = as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">) | as<ast::Operator::modulo>(lit<"%">);
  static constexpr auto OpAdd = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">);
  static constexpr auto OpCompare =
      as<ast::Operator::greater_equal>(lit<">=">) | as<ast::Operator::less_equal>(lit<"<=">) | as<ast::Operator::greater>(lit<">">) | as<ast::Operator::less>(lit<"<">);
  static constexpr auto OpEq = as<ast::Operator::equal>(lit<"==">) | as<ast::Operator::not_equal>(lit<"!=">);
  static constexpr auto OpAnd = ref<AND>;
  static constexpr auto OpOr = as<ast::Operator::logical_or>(lit<"||">);

  struct Expression;
  struct FullEach;
  struct LegacyEach;
  struct BodyItem;
  struct String;
  struct UnaryExpr;
  static constexpr auto ForHead =
      ref<FOR> + some(append<&ast::ForLoop::variables>(Name), lit<",">) +
      ref<IN> + assign<&ast::ForLoop::collection>(ref<Expression>) + lit<":">;
  static constexpr auto ForTail =
      option(flag<&ast::ForLoop::grouping>(lit<"...">)) +
      option(ref<IF> + assign<&ast::ForLoop::condition>(ref<Expression>));
  struct TupleFor : rule<create<ast::ForLoop>(
                            lit<"["> + ForHead +
                            assign<&ast::ForLoop::result>(ref<Expression>) +
                            ForTail + lit<"]">)> {};
  struct ObjectFor
      : rule<create<ast::ForLoop>(
            flag<&ast::ForLoop::object>(lit<"{">) + ForHead +
            assign<&ast::ForLoop::key>(ref<Expression>) + lit<"=>"> +
            assign<&ast::ForLoop::result>(ref<Expression>) + ForTail +
            lit<"}">)> {};
  struct Number : rule<create<ast::NumberLit>(
                          assign<&ast::NumberLit::text>(ref<NUMBER>))> {};
  struct FuncCall
      : rule<create<ast::FuncCall>(
            some(append<&ast::FuncCall::name>(Name), lit<"::">) + lit<"("> +
            option(
                some(append<&ast::FuncCall::args>(ref<Expression>), lit<",">) +
                (flag<&ast::FuncCall::expandFinal>(lit<"...">) |
                 option(lit<",">))) +
            lit<")">)> {};
  struct List
      : rule<create<ast::List>(
            lit<"["> + !ref<FOR> +
            option(some(append<&ast::List::items>(ref<Expression>), lit<",">) +
                   option(lit<",">)) +
            lit<"]">)> {};
  struct MapEntry : rule<create<ast::MapEntry>(
                            assign<&ast::MapEntry::key>(ref<Expression>) +
                            (lit<"="> | lit<":">)+assign<&ast::MapEntry::value>(
                                ref<Expression>))> {};
  struct Map : rule<create<ast::MapObj>(
                       lit<"{"> + !ref<FOR> +
                       many(append<&ast::MapObj::entries>(ref<MapEntry>) +
                            option(lit<",">)) +
                       lit<"}">)> {};
  struct Interpolation
      : rule<create<ast::Interpolation>(
            assign<&ast::Interpolation::trimLeft>(ref<INTERP_OPEN>) +
            skip(ignore<WS>, hide<LINE_COMMENT, BLOCK_COMMENT>,

                 assign<&ast::Interpolation::value>(ref<Expression>) +
                     option(
                         flag<&ast::Interpolation::trimRight>(lit<"~">)) +
                     lit<"}">))> {};
  struct Directive
      : rule<create<ast::TemplateDirective>(
            assign<&ast::TemplateDirective::trimLeft>(ref<DIRECTIVE_OPEN>) +
            skip(ignore<WS>, hide<LINE_COMMENT, BLOCK_COMMENT>,

                 ((assign<&ast::TemplateDirective::name>(ref<FOR>) +
                   some(append<&ast::TemplateDirective::variables>(Name),
                        lit<",">) +
                   ref<IN> +
                   assign<&ast::TemplateDirective::value>(ref<Expression>)) |
                  (assign<&ast::TemplateDirective::name>(ref<IF>) +
                   assign<&ast::TemplateDirective::value>(ref<Expression>)) |
                  assign<&ast::TemplateDirective::name>(ref<ID>)) +
                     option(flag<&ast::TemplateDirective::trimRight>(
                         lit<"~">)) +
                     lit<"}">))> {};
  struct String
      : rule<create<ast::StringLit>(
            lit<"\""> +
            tight(many(append<&ast::StringLit::parts>(create<ast::TemplateText>(
                          assign<&ast::TemplateText::raw>(ref<STRING_TEXT>))) |
                      append<&ast::StringLit::parts>(ref<Interpolation> |
                                                     ref<Directive>)) +
                     lit<"\"">))> {};
  struct Primary
      : rule<create<ast::ParenthesizedExpr>(
                 lit<"("> +
                 assign<&ast::ParenthesizedExpr::value>(ref<Expression>) +
                 lit<")">) |
             ref<TupleFor> | ref<ObjectFor> | ref<List> |
             ref<Map> | create<ast::NullLit>(ref<NULL_KW>) |
             create<ast::BoolLit>(flag<&ast::BoolLit::value>(ref<TRUE>) |
                                  ref<FALSE>) |
             ref<Number> | ref<String> |
             create<ast::HeredocLit>(
                 assign<&ast::HeredocLit::text>(ref<HEREDOC>)) |
             ref<FuncCall> |
             create<ast::Identifier>(assign<&ast::Identifier::name>(
                 ref<ID> | ref<FOR> | ref<IN> | ref<IF>))> {};
  struct LEGACY_INDEX : rule<as<decimal_index>(ref<NUMBER>)> {};
  struct LegacyIndexNumber : rule<create<ast::NumberLit>(
      assign<&ast::NumberLit::text>(ref<LEGACY_INDEX>))> {};
  static constexpr auto MemberStep =
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          lit<"."> + assign<&ast::MemberExpr::name>(Name)) |
      nest<ast::IndexExpr, &ast::IndexExpr::object>(
          lit<"."> + assign<&ast::IndexExpr::index>(ref<LegacyIndexNumber>));
  static constexpr auto PostfixStep =
      MemberStep |
      nest<ast::IndexExpr, &ast::IndexExpr::object>(
          lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) +
          lit<"]">) |
      nest<ast::SplatExpr, &ast::SplatExpr::source>(
          lit<"["> + lit<"*"> + lit<"]"> +
          assign<&ast::SplatExpr::each>(ref<FullEach>)) |
      nest<ast::SplatExpr, &ast::SplatExpr::source>(
          flag<&ast::SplatExpr::legacy>(lit<"."> + lit<"*">) +
          assign<&ast::SplatExpr::each>(ref<LegacyEach>));
  struct AnonSymbol : rule<create<ast::AnonSymbolExpr>> {};
  struct FullEach : rule<ref<AnonSymbol> + many(PostfixStep)> {};
  struct LegacyEach : rule<ref<AnonSymbol> + many(MemberStep)> {};
  struct UnaryExpr
      : rule<(create<ast::UnaryExpr>(
                  assign<&ast::UnaryExpr::op>(
                      as<ast::UnaryOp::logical_not>(lit<"!">) | as<ast::UnaryOp::negate>(lit<"-">) | as<ast::UnaryOp::positive>(lit<"+">)) +
                  assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) |
              ref<Primary>)+many(PostfixStep)> {};
  struct BinaryExpr : infix<UnaryExpr, ast::BinaryExpr,
                            levels(left(OpMul), left(OpAdd), left(OpCompare),
                                   left(OpEq), left(OpAnd), left(OpOr))> {};
  struct Expression
      : rule<ref<BinaryExpr> +
             option(nest<ast::TernaryExpr, &ast::TernaryExpr::cond>(
                 lit<"?"> +
                 assign<&ast::TernaryExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::TernaryExpr::elseValue>(ref<Expression>)))> {};
  struct Argument : rule<create<ast::Argument>(
                            assign<&ast::Argument::name>(Name) + lit<"="> +
                            assign<&ast::Argument::value>(ref<Expression>))> {};
  struct Block
      : rule<create<ast::Block>(
            assign<&ast::Block::blockType>(Name) +
            many(append<&ast::Block::labels>(
                ref<String> | create<ast::Identifier>(
                                      assign<&ast::Identifier::name>(Name)))) +
            lit<"{"> + many(append<&ast::Block::body>(ref<BodyItem>)) +
            lit<"}">)> {};
  struct BodyItem : rule<ref<Argument> | ref<Block>> {};
  struct File : rule<create<ast::File>(
                        many(append<&ast::File::items>(ref<BodyItem>)))> {};
  using entry = File;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};
}  // namespace terraform
