#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rego/core/StringValue.hpp"
#include "rego/core/ast.hpp"

namespace rego {

inline constexpr auto null_value = [](std::string_view) {
  return std::monostate{};
};
using NullValue = cursiv::text_conv<null_value>;

struct Rego : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};
  struct NAME : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_"> | uc::Nd)> {};
  struct NUMBER : terminal<((some(d) + option(lit<"."> + some(d))) |
                            (lit<"."> + some(d))) +
                           option(cr<"eE"> + option(cr<"+-">) + some(d))> {};
  static constexpr auto Escape =
      lit<"\\"> +
      (cr<R"(\\/bfnrt")"> | (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)));
  struct STRING : terminal<(lit<"\""> + many(Escape | cr<R"(^"\\\x00-\x1f)">) +
                            lit<"\"">) |
                               (lit<"`"> + many(cr<"^`">) + lit<"`">),
                           UnescapedString> {};
  struct BOOL : terminal<kw<"true"> | kw<"false">, bool> {};
  struct NULLK : terminal<kw<"null">, NullValue> {};
  struct EQOPER : terminal<lit<":="> | lit<"=">> {};
  struct RELOP : terminal<lit<"=="> | lit<"!="> | lit<"<="> | lit<">="> |
                          lit<">"> | lit<"<">> {};
  struct FACTOROP : terminal<lit<"*"> | lit<"/"> | lit<"%">> {};
  struct ARITHOP : terminal<lit<"+"> | lit<"-">> {};

  struct Expression;
  struct Relation;
  struct Atom;
  struct Primary;
  struct RefRule;
  struct ArrayComprRule;
  struct SetComprRule;
  struct ObjectComprRule;
  struct ObjectRule;
  struct ObjectItemRule;
  struct ArrayRule;
  struct SetRule;
  struct QueryRule;
  struct QueryItemRule;
  struct LiteralExprRule;
  struct RuleBodyRule;
  struct RuleRule;
  struct FullExpression;
  static constexpr auto Name =
      ref<NAME> | kw<"if"> | kw<"contains"> | kw<"in"> | kw<"every">;

  struct NumberRule : rule<assign<&ast::NumberLiteral::text>(ref<NUMBER>)> {};
  static constexpr auto Scalar =
      ref<NumberRule> | ref<STRING> | ref<BOOL> | ref<NULLK>;
  struct ScalarRule
      : rule<option(assign<&ast::ScalarTerm::sign>(ref<ARITHOP>)) +
             assign<&ast::ScalarTerm::value>(Scalar)> {};
  struct Relation
      : infix<Atom, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<FACTOROP>) >> left(ref<ARITHOP>) >> left(lit<"&">) >>
               left(lit<"|">) >> left(ref<RELOP>))> {};
  struct Expression
      : infix<Relation, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(kw<"in">)> {};

  template <auto Member>
  static constexpr auto part =
      (lit<"."> + append<Member>(Name)) |
      (lit<"["> + append<Member>(ref<Expression>) + lit<"]">);
  struct NameRule : rule<assign<&ast::Ref::name>(Name)> {};
  struct RefRule
      : rule<assign<&ast::Ref::name>(Name) + many(part<&ast::Ref::parts>)> {};
  static constexpr auto callSuffix =
      lit<"("> +
      option(append<&ast::Call::args>(ref<Expression>) +
             many(lit<","> + append<&ast::Call::args>(ref<Expression>)) +
             option(lit<",">)) +
      lit<")">;
  struct Atom : rule<ref<Primary> +
                     many(nest<ast::Call, &ast::Call::target>(callSuffix) |
                          nest<ast::AccessExpr, &ast::AccessExpr::target>(
                              part<&ast::AccessExpr::parts>))> {};
  struct Primary
      : rule<(lit<"("> + ref<FullExpression> + lit<")">) | ref<ArrayComprRule> |
             ref<ObjectComprRule> | ref<SetComprRule> | ref<ObjectRule> |
             ref<ArrayRule> | ref<SetRule> | ref<ScalarRule> | ref<NameRule>> {
  };

  struct ArrayComprRule
      : rule<lit<"["> + assign<&ast::ArrayCompr::item>(ref<Atom>) + lit<"|"> +
             assign<&ast::ArrayCompr::body>(ref<QueryRule>) + lit<"]">> {};
  struct SetComprRule
      : rule<lit<"{"> + assign<&ast::SetCompr::item>(ref<Atom>) + lit<"|"> +
             assign<&ast::SetCompr::body>(ref<QueryRule>) + lit<"}">> {};
  struct ObjectComprRule
      : rule<lit<"{"> + assign<&ast::ObjectCompr::key>(ref<Atom>) + lit<":"> +
             assign<&ast::ObjectCompr::value>(ref<Atom>) + lit<"|"> +
             assign<&ast::ObjectCompr::body>(ref<QueryRule>) + lit<"}">> {};
  struct ObjectItemRule
      : rule<assign<&ast::ObjectItem::key>(ref<Expression>) + lit<":"> +
             assign<&ast::ObjectItem::value>(ref<Expression>)> {};
  struct ObjectRule
      : rule<lit<"{"> +
             option(append<&ast::ObjectLit::items>(ref<ObjectItemRule>) +
                    many(lit<","> +
                         append<&ast::ObjectLit::items>(ref<ObjectItemRule>)) +
                    option(lit<",">)) +
             lit<"}">> {};
  struct ArrayRule
      : rule<lit<"["> + option(lit<",">) +
             option(append<&ast::ArrayLit::items>(ref<Expression>) +
                    many(lit<","> +
                         append<&ast::ArrayLit::items>(ref<Expression>)) +
                    option(lit<",">)) +
             lit<"]">> {};
  struct SetRule
      : rule<(lit<"set("> + create<ast::SetLit> + lit<")">) |
             (lit<"{"> + append<&ast::SetLit::items>(ref<Expression>) +
              many(lit<","> + append<&ast::SetLit::items>(ref<Expression>)) +
              option(lit<",">) + lit<"}">)> {};

  struct FullExpression
      : rule<ref<Expression> +
             option(nest<ast::MembershipExpr, &ast::MembershipExpr::key>(
                 lit<","> + assign<&ast::MembershipExpr::value>(ref<Relation>) +
                 kw<"in"> +
                 assign<&ast::MembershipExpr::collection>(ref<Relation>)))> {};
  struct EqTermRule : rule<assign<&ast::EqTerm::op>(ref<EQOPER>) +
                           assign<&ast::EqTerm::value>(ref<Expression>)> {};
  struct LiteralExprRule
      : rule<assign<&ast::LiteralExpr::first>(ref<FullExpression>) +
             many(append<&ast::LiteralExpr::rest>(ref<EqTermRule>))> {};
  struct WithModRule
      : rule<kw<"with"> + assign<&ast::WithMod::target>(ref<Expression>) +
             kw<"as"> + assign<&ast::WithMod::value>(ref<Expression>)> {};
  static constexpr auto LiteralBody =
      assign<&ast::Literal::expr>(ref<LiteralExprRule>);
  static constexpr auto SomeBody =
      kw<"some"> + append<&ast::SomeDecl::symbols>(ref<Relation>) +
      many(lit<","> + append<&ast::SomeDecl::symbols>(ref<Relation>)) +
      option(kw<"in"> + assign<&ast::SomeDecl::collection>(ref<Expression>));
  static constexpr auto EveryBody =
      kw<"every"> + append<&ast::Every::symbols>(ref<Relation>) +
      many(lit<","> + append<&ast::Every::symbols>(ref<Relation>)) +
      kw<"in"> + assign<&ast::Every::collection>(ref<Expression>) +
      lit<"{"> + assign<&ast::Every::body>(ref<QueryRule>) + lit<"}">;
  struct QueryItemRule
      : rule<option(flag<&ast::QueryItem::negated>(kw<"not">)) +
             (create<ast::SomeDecl>(SomeBody) | create<ast::Every>(EveryBody) |
              create<ast::Literal>(LiteralBody)) +
             many(append<&ast::QueryItem::withs>(ref<WithModRule>))> {};
  struct QueryRule
      : rule<create<ast::Query> +
             option(append<&ast::Query::literals>(ref<QueryItemRule>) +
                    many((lit<";"> | gap::line) +
                         append<&ast::Query::literals>(ref<QueryItemRule>)) +
                    option(lit<";">))> {};
  struct TopQueryRule : rule<append<&ast::Query::literals>(ref<QueryItemRule>) +
                             many(lit<";"> + append<&ast::Query::literals>(
                                                 ref<QueryItemRule>))> {};

  static constexpr auto bracedBody =
      lit<"{"> + assign<&ast::RuleBody::body>(ref<QueryRule>) + lit<"}">;
  static constexpr auto ifBody =
      flag<&ast::RuleBody::hasIf>(kw<"if">) +
      (bracedBody | assign<&ast::RuleBody::body>(create<ast::Query>(
                        append<&ast::Query::literals>(ref<QueryItemRule>))));
  struct RuleBodyRule
      : rule<ifBody | bracedBody |
             (flag<&ast::RuleBody::isElse>(kw<"else">) +
              option(assign<&ast::RuleBody::op>(ref<EQOPER>) +
                     assign<&ast::RuleBody::elseValue>(ref<Expression>)) +
              option(ifBody | bracedBody))> {};
  static constexpr auto ruleHead =
      assign<&ast::Rule::name>(Name) + many(part<&ast::Rule::parts>) +
      option(
          lit<"("> +
          option(append<&ast::Rule::params>(ref<Expression>) +
                 many(lit<","> + append<&ast::Rule::params>(ref<Expression>)) +
                 option(lit<",">)) +
          lit<")">) +
      option((flag<&ast::Rule::contains>(kw<"contains">) +
              assign<&ast::Rule::key>(ref<Expression>)) |
             (assign<&ast::Rule::op>(ref<EQOPER>) +
              assign<&ast::Rule::value>(ref<Expression>)));
  struct RuleRule
      : rule<ruleHead + many(append<&ast::Rule::bodies>(ref<RuleBodyRule>))> {};
  struct DefaultRuleRule
      : rule<create<ast::DefaultRule>(kw<"default"> + ruleHead)> {};
  struct PackageDeclRule
      : rule<kw<"package"> + assign<&ast::PackageDecl::path>(ref<RefRule>)> {};
  struct ImportDeclRule
      : rule<kw<"import"> + assign<&ast::ImportDecl::path>(ref<RefRule>) +
             option(kw<"as"> + assign<&ast::ImportDecl::alias>(ref<RefRule>))> {
  };
  struct QueryStmtRule
      : rule<create<ast::QueryStmt>(
            (lit<"{"> + assign<&ast::QueryStmt::body>(ref<QueryRule>) +
             lit<"}">) |
            assign<&ast::QueryStmt::body>(ref<TopQueryRule>))> {};
  struct StmtRule
      : rule<ref<PackageDeclRule> | ref<ImportDeclRule> | ref<DefaultRuleRule> |
             ref<RuleRule> | ref<QueryStmtRule>> {};
  struct DocumentRule
      : rule<create<ast::Document> +
             option(append<&ast::Document::statements>(ref<StmtRule>) +
                    many((gap::line | !cr<"{[(+-">)+append<
                         &ast::Document::statements>(ref<StmtRule>)))> {};
  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace rego
