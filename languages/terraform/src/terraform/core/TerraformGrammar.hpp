#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "terraform/core/StringValue.hpp"
#include "terraform/core/ast.hpp"
namespace terraform {
inline constexpr auto trim_open = [](std::string_view text) {
  return text.size() > 2 && text[2] == '~';
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
  struct INTERP_OPEN : terminal<lit<"${"> + option(lit<"~">) + many(s),
                                cursiv::text_conv<trim_open>> {};
  struct DIRECTIVE_OPEN : terminal<lit<"%{"> + option(lit<"~">) + many(s),
                                   cursiv::text_conv<trim_open>> {};
  struct STRING_TEXT
      : terminal<some(Escaped | (lit<"$${"> | lit<"%%{">) |
                      (lit<"$", !lit<"{">>) | (lit<"%", !lit<"{">>) |
                      cr<R"(^"\\$%\r\n)">)> {};
  struct HEREDOC
      : terminal<(lit<"<<"> + option(lit<"-">) +
                  mark<HeredocTag>(some(cr<R"(^ \t\r\n)">)) +
                  option(lit<"\r">) +
                  lit<"\n">)+many(!(many(cr<R"( \t)">) + same<HeredocTag> +
                                    &((lit<"\r\n"> | lit<"\n">) | eof)) +
                                  many(cr<R"(^\r\n)">) + option(lit<"\r">) +
                                  lit<"\n">) +
                 many(cr<R"( \t)">) + same<HeredocTag> +
                 &((lit<"\r\n"> | lit<"\n">) | eof)> {};
  struct MULTIPLY : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct MODULO : terminal<lit<"%">, value<ast::Operator::modulo>> {};
  struct ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUBTRACT : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct GREATER_EQUAL
      : terminal<lit<">=">, value<ast::Operator::greater_equal>> {};
  struct GREATER : terminal<lit<">">, value<ast::Operator::greater>> {};
  struct LESS_EQUAL : terminal<lit<"<=">, value<ast::Operator::less_equal>> {};
  struct LESS : terminal<lit<"<">, value<ast::Operator::less>> {};
  struct EQUAL : terminal<lit<"==">, value<ast::Operator::equal>> {};
  struct NOT_EQUAL : terminal<lit<"!=">, value<ast::Operator::not_equal>> {};
  struct AND : terminal<lit<"&&">, value<ast::Operator::logical_and>> {};
  struct OR : terminal<lit<"||">, value<ast::Operator::logical_or>> {};

  static constexpr auto OpMul = ref<MULTIPLY> | ref<DIVIDE> | ref<MODULO>;
  static constexpr auto OpAdd = ref<ADD> | ref<SUBTRACT>;
  static constexpr auto OpCompare =
      ref<GREATER_EQUAL> | ref<LESS_EQUAL> | ref<GREATER> | ref<LESS>;
  static constexpr auto OpEq = ref<EQUAL> | ref<NOT_EQUAL>;
  static constexpr auto OpAnd = ref<AND>;
  static constexpr auto OpOr = ref<OR>;

  struct Expression;
  struct FullEach;
  struct LegacyEach;
  struct BodyItemRule;
  struct StringRule;
  struct UnaryExpr;
  static constexpr auto ForHead =
      ref<FOR> + some(append<&ast::ForLoop::variables>(Name), lit<",">) +
      ref<IN> + assign<&ast::ForLoop::collection>(ref<Expression>) + lit<":">;
  static constexpr auto ForTail =
      option(flag<&ast::ForLoop::grouping>(lit<"...">)) +
      option(ref<IF> + assign<&ast::ForLoop::condition>(ref<Expression>));
  struct TupleForRule : rule<create<ast::ForLoop>(
                            lit<"["> + ForHead +
                            assign<&ast::ForLoop::result>(ref<Expression>) +
                            ForTail + lit<"]">)> {};
  struct ObjectForRule
      : rule<create<ast::ForLoop>(
            flag<&ast::ForLoop::object>(lit<"{">) + ForHead +
            assign<&ast::ForLoop::key>(ref<Expression>) + lit<"=>"> +
            assign<&ast::ForLoop::result>(ref<Expression>) + ForTail +
            lit<"}">)> {};
  struct NumberRule : rule<create<ast::NumberLit>(
                          assign<&ast::NumberLit::text>(ref<NUMBER>))> {};
  struct FuncCallRule
      : rule<create<ast::FuncCall>(
            some(append<&ast::FuncCall::name>(Name), lit<"::">) + lit<"("> +
            option(
                some(append<&ast::FuncCall::args>(ref<Expression>), lit<",">) +
                (option(flag<&ast::FuncCall::expandFinal>(lit<"...">)) +
                 option(lit<",">))) +
            lit<")">)> {};
  struct ListRule
      : rule<create<ast::List>(
            lit<"["> +
            option(some(append<&ast::List::items>(ref<Expression>), lit<",">) +
                   option(lit<",">)) +
            lit<"]">)> {};
  struct MapEntryRule : rule<create<ast::MapEntry>(
                            assign<&ast::MapEntry::key>(ref<Expression>) +
                            (lit<"="> | lit<":">)+assign<&ast::MapEntry::value>(
                                ref<Expression>))> {};
  struct MapRule : rule<create<ast::MapObj>(
                       lit<"{"> +
                       many(append<&ast::MapObj::entries>(ref<MapEntryRule>) +
                            option(lit<",">)) +
                       lit<"}">)> {};
  struct InterpolationRule
      : rule<create<ast::Interpolation>(
            assign<&ast::Interpolation::trimLeft>(ref<INTERP_OPEN>) +
            skip(ignored<WS>, hide<LINE_COMMENT, BLOCK_COMMENT>,

                 assign<&ast::Interpolation::value>(ref<Expression>) +
                     option(
                         flag<&ast::Interpolation::trimRight>(lit<"~">)) +
                     lit<"}">))> {};
  struct DirectiveRule
      : rule<create<ast::TemplateDirective>(
            assign<&ast::TemplateDirective::trimLeft>(ref<DIRECTIVE_OPEN>) +
            skip(ignored<WS>, hide<LINE_COMMENT, BLOCK_COMMENT>,

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
  struct StringRule
      : rule<create<ast::StringLit>(
            lit<"\""> +
            tight(many(append<&ast::StringLit::parts>(create<ast::TemplateText>(
                          assign<&ast::TemplateText::raw>(ref<STRING_TEXT>))) |
                      append<&ast::StringLit::parts>(ref<InterpolationRule> |
                                                     ref<DirectiveRule>)) +
                     lit<"\"">))> {};
  struct Primary
      : rule<create<ast::ParenthesizedExpr>(
                 lit<"("> +
                 assign<&ast::ParenthesizedExpr::value>(ref<Expression>) +
                 lit<")">) |
             ref<TupleForRule> | ref<ObjectForRule> | ref<ListRule> |
             ref<MapRule> | create<ast::NullLit>(ref<NULL_KW>) |
             create<ast::BoolLit>(flag<&ast::BoolLit::value>(ref<TRUE>) |
                                  ref<FALSE>) |
             ref<NumberRule> | ref<StringRule> |
             create<ast::HeredocLit>(
                 assign<&ast::HeredocLit::text>(ref<HEREDOC>)) |
             ref<FuncCallRule> |
             create<ast::Identifier>(assign<&ast::Identifier::name>(
                 ref<ID> | ref<FOR> | ref<IN> | ref<IF>))> {};
  static constexpr auto MemberStep =
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          lit<"."> + assign<&ast::MemberExpr::name>(Name)) |
      nest<ast::IndexExpr, &ast::IndexExpr::object>(
          lit<"."> + assign<&ast::IndexExpr::index>(ref<NumberRule>));
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
  struct FullEach : rule<create<ast::AnonSymbolExpr> + many(PostfixStep)> {};
  struct LegacyEach : rule<create<ast::AnonSymbolExpr> + many(MemberStep)> {};
  struct UNARY_NOT : terminal<lit<"!">, value<ast::UnaryOp::logical_not>> {};
  struct UNARY_MINUS : terminal<lit<"-">, value<ast::UnaryOp::negate>> {};
  struct UNARY_PLUS : terminal<lit<"+">, value<ast::UnaryOp::positive>> {};
  struct UnaryExpr
      : rule<(create<ast::UnaryExpr>(
                  assign<&ast::UnaryExpr::op>(
                      ref<UNARY_NOT> | ref<UNARY_MINUS> | ref<UNARY_PLUS>) +
                  assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) |
              ref<Primary>)+many(PostfixStep)> {};
  struct BinaryExpr : infix<UnaryExpr, &ast::BinaryExpr::left,
                            &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                            (left(OpMul) >> left(OpAdd) >> left(OpCompare) >>
                             left(OpEq) >> left(OpAnd) >> left(OpOr))> {};
  struct Expression
      : rule<ref<BinaryExpr> +
             option(nest<ast::TernaryExpr, &ast::TernaryExpr::cond>(
                 lit<"?"> +
                 assign<&ast::TernaryExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::TernaryExpr::elseValue>(ref<Expression>)))> {};
  struct ArgumentRule : rule<create<ast::Argument>(
                            assign<&ast::Argument::name>(Name) + lit<"="> +
                            assign<&ast::Argument::value>(ref<Expression>))> {};
  struct BlockRule
      : rule<create<ast::Block>(
            assign<&ast::Block::blockType>(Name) +
            many(append<&ast::Block::labels>(
                ref<StringRule> | create<ast::Identifier>(
                                      assign<&ast::Identifier::name>(Name)))) +
            lit<"{"> + many(append<&ast::Block::body>(ref<BodyItemRule>)) +
            lit<"}">)> {};
  struct BodyItemRule : rule<ref<ArgumentRule> | ref<BlockRule>> {};
  struct FileRule : rule<create<ast::File>(
                        many(append<&ast::File::items>(ref<BodyItemRule>)))> {};
  using entry = FileRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};
}  // namespace terraform
