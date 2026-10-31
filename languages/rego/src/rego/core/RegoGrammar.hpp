#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rego/core/StringValue.hpp"
#include "rego/core/ast.hpp"

namespace rego {

inline constexpr auto null_value = [](std::string_view) {
  return std::monostate{};
};

struct Rego : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};
  static constexpr auto reserved = kw<"package"> | kw<"import"> | kw<"as"> |
      kw<"default"> | kw<"else"> | kw<"not"> | kw<"some"> | kw<"with"> |
      kw<"true"> | kw<"false"> | kw<"null">;
  struct NAME : terminal<!reserved + cr<"a-zA-Z_"> +
                         many(cr<"a-zA-Z0-9_"> | uc::Nd)> {};
  struct NUMBER : terminal<((some(d) + option(lit<"."> + some(d))) |
                            (lit<"."> + some(d))) +
                           option(cr<"eE"> + option(cr<"+-">) + some(d))> {};
  static constexpr auto Escape =
      lit<"\\"> +
      (cr<R"(\\/bfnrt")"> | (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)));
  struct STRING
      : terminal<as<unescape_string>(
            (lit<"\""> + many(Escape | cr<R"(^"\\\x00-\x1f)">) + lit<"\"">) |
            (lit<"`"> + many(cr<"^`">) + lit<"`">))> {};
  struct BOOL : terminal<as<bool>(kw<"true"> | kw<"false">)> {};
  struct NULLK : terminal<as<null_value>(kw<"null">)> {};
  struct EQOPER : terminal<lit<":="> | lit<"=">> {};
  struct RELOP : terminal<lit<"=="> | lit<"!="> | lit<"<="> | lit<">="> |
                          lit<">"> | lit<"<">> {};
  struct FACTOROP : terminal<lit<"*"> | lit<"/"> | lit<"%">> {};
  struct ARITHOP : terminal<lit<"+"> | lit<"-">> {};

  struct Expression;
  struct Relation;
  struct Atom;
  struct Primary;
  struct Ref;
  struct ArrayCompr;
  struct SetCompr;
  struct ObjectCompr;
  struct Object;
  struct ObjectItem;
  struct Array;
  struct Set;
  struct Query;
  struct QueryItem;
  struct LiteralExpr;
  struct RuleBody;
  struct Rule;
  struct FullExpression;
  static constexpr auto Name =
      ref<NAME> | kw<"if"> | kw<"contains"> | kw<"in"> | kw<"every">;
  static constexpr auto PathName = Name | reserved;
  static constexpr auto HeadName =
      Name | (reserved + gap::none + &(lit<"."> | lit<"[">));

  struct Number : rule<assign<&ast::NumberLiteral::text>(ref<NUMBER>)> {};
  static constexpr auto Scalar =
      ref<Number> | ref<STRING> | ref<BOOL> | ref<NULLK>;
  struct ScalarRule
      : rule<option(assign<&ast::ScalarTerm::sign>(ref<ARITHOP>)) +
             assign<&ast::ScalarTerm::value>(Scalar)> {};
  struct Relation
      : infix<Atom, ast::BinaryExpr,
              levels(left(ref<FACTOROP>), left(ref<ARITHOP>), left(lit<"&">),
                     left(lit<"|">), left(ref<RELOP>))> {};
  struct Expression
      : infix<Relation, ast::BinaryExpr, left(kw<"in">)> {};

  template <auto Member>
  static constexpr auto part =
      (lit<"."> + gap::none + append<Member>(PathName)) |
      (lit<"["> + append<Member>(ref<Expression>) + lit<"]">);
  struct NameRule : rule<assign<&ast::Ref::name>(HeadName)> {};
  struct Ref
      : rule<assign<&ast::Ref::name>(Name) +
             many(gap::none + part<&ast::Ref::parts>)> {};
  static constexpr auto callSuffix =
      lit<"("> +
      option(append<&ast::Call::args>(ref<Expression>) +
             many(lit<","> + append<&ast::Call::args>(ref<Expression>)) +
             option(lit<",">)) +
      lit<")">;
  struct Atom : rule<ref<Primary> +
                     many(nest<ast::Call, &ast::Call::target>(
                              gap::none + callSuffix) |
                          nest<ast::AccessExpr, &ast::AccessExpr::target>(
                              gap::none + part<&ast::AccessExpr::parts>))> {};
  struct Primary
      : rule<(lit<"("> + ref<FullExpression> + lit<")">) | ref<ArrayCompr> |
             ref<ObjectCompr> | ref<SetCompr> | ref<Object> |
             ref<Array> | ref<Set> | ref<ScalarRule> | ref<NameRule>> {
  };

  struct ArrayCompr
      : rule<lit<"["> + assign<&ast::ArrayCompr::item>(ref<Atom>) + lit<"|"> +
             assign<&ast::ArrayCompr::body>(ref<Query>) + lit<"]">> {};
  struct SetCompr
      : rule<lit<"{"> + assign<&ast::SetCompr::item>(ref<Atom>) + lit<"|"> +
             assign<&ast::SetCompr::body>(ref<Query>) + lit<"}">> {};
  struct ObjectCompr
      : rule<lit<"{"> + assign<&ast::ObjectCompr::key>(ref<Atom>) + lit<":"> +
             assign<&ast::ObjectCompr::value>(ref<Atom>) + lit<"|"> +
             assign<&ast::ObjectCompr::body>(ref<Query>) + lit<"}">> {};
  struct ObjectItem
      : rule<assign<&ast::ObjectItem::key>(ref<Expression>) + lit<":"> +
             assign<&ast::ObjectItem::value>(ref<Expression>)> {};
  struct Object
      : rule<lit<"{"> +
             option(append<&ast::ObjectLit::items>(ref<ObjectItem>) +
                    many(lit<","> +
                         append<&ast::ObjectLit::items>(ref<ObjectItem>)) +
                    option(lit<",">)) +
             lit<"}">> {};
  struct Array
      : rule<lit<"["> +
             option(option(lit<",">) +
                    append<&ast::ArrayLit::items>(ref<Expression>) +
                    many(lit<","> +
                         append<&ast::ArrayLit::items>(ref<Expression>)) +
                    option(lit<",">)) +
             lit<"]">> {};
  struct Set
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
  struct EqTerm : rule<assign<&ast::EqTerm::op>(ref<EQOPER>) +
                           assign<&ast::EqTerm::value>(ref<Expression>)> {};
  struct LiteralExpr
      : rule<assign<&ast::LiteralExpr::first>(ref<FullExpression>) +
             many(append<&ast::LiteralExpr::rest>(ref<EqTerm>))> {};
  struct WithMod
      : rule<kw<"with"> + assign<&ast::WithMod::target>(ref<Expression>) +
             kw<"as"> + assign<&ast::WithMod::value>(ref<Expression>)> {};
  static constexpr auto LiteralBody =
      assign<&ast::Literal::expr>(ref<LiteralExpr>);
  static constexpr auto SomeBody =
      kw<"some"> + append<&ast::SomeDecl::symbols>(ref<Relation>) +
      many(lit<","> + append<&ast::SomeDecl::symbols>(ref<Relation>)) +
      option(kw<"in"> + assign<&ast::SomeDecl::collection>(ref<Expression>));
  static constexpr auto EveryBody =
      kw<"every"> + append<&ast::Every::symbols>(ref<Relation>) +
      many(lit<","> + append<&ast::Every::symbols>(ref<Relation>)) +
      kw<"in"> + assign<&ast::Every::collection>(ref<Expression>) +
      lit<"{"> + assign<&ast::Every::body>(ref<Query>) + lit<"}">;
  struct QueryItem
      : rule<option(flag<&ast::QueryItem::negated>(kw<"not">)) +
             (create<ast::SomeDecl>(SomeBody) | create<ast::Every>(EveryBody) |
              create<ast::Literal>(LiteralBody)) +
             many(append<&ast::QueryItem::withs>(ref<WithMod>))> {};
  struct Query
      : rule<create<ast::Query> +
             append<&ast::Query::literals>(ref<QueryItem>) +
             many((lit<";"> | gap::line) +
                  append<&ast::Query::literals>(ref<QueryItem>)) +
             option(lit<";">)> {};
  struct TopQuery : rule<append<&ast::Query::literals>(ref<QueryItem>) +
                             many(lit<";"> + append<&ast::Query::literals>(
                                                 ref<QueryItem>))> {};

  static constexpr auto bracedBody =
      lit<"{"> + assign<&ast::RuleBody::body>(ref<Query>) + lit<"}">;
  static constexpr auto ifBody =
      flag<&ast::RuleBody::hasIf>(kw<"if">) +
      (bracedBody | assign<&ast::RuleBody::body>(create<ast::Query>(
                        append<&ast::Query::literals>(ref<QueryItem>))));
  struct RuleBody
      : rule<ifBody | bracedBody |
             (flag<&ast::RuleBody::isElse>(kw<"else">) +
              option(assign<&ast::RuleBody::op>(ref<EQOPER>) +
                     assign<&ast::RuleBody::elseValue>(ref<Expression>)) +
              option(ifBody | bracedBody))> {};
  static constexpr auto headArguments = flag<&ast::Rule::hasParams>(
          lit<"("> +
          option(append<&ast::Rule::params>(ref<Expression>) +
                 many(lit<","> + append<&ast::Rule::params>(ref<Expression>)) +
                 option(lit<",">)) +
          lit<")">);
  static constexpr auto headLoose =
      assign<&ast::Rule::name>(HeadName) +
      option((part<&ast::Rule::parts> +
              many(gap::none + part<&ast::Rule::parts>) +
              option(gap::none + headArguments)) | headArguments);
  static constexpr auto headTight =
      assign<&ast::Rule::name>(HeadName) +
      many(gap::none + part<&ast::Rule::parts>) +
      option(gap::none + headArguments);
  static constexpr auto assignedHead =
      assign<&ast::Rule::op>(ref<EQOPER>) +
      assign<&ast::Rule::value>(ref<Expression>);
  static constexpr auto containsHead =
      flag<&ast::Rule::contains>(kw<"contains">) +
      assign<&ast::Rule::key>(ref<Expression>);
  static constexpr auto ruleHead = headLoose + option(containsHead | assignedHead);
  struct HeadLooseWithBody
      : rule<headLoose +
             ((containsHead + many(append<&ast::Rule::bodies>(ref<RuleBody>))) |
              (option(assignedHead) +
               some(append<&ast::Rule::bodies>(ref<RuleBody>))))> {};
  struct HeadTightFact : rule<headTight + option(assignedHead)> {};
  struct Rule
      : rule<ref<HeadLooseWithBody> | ref<HeadTightFact>> {};
  struct DefaultRule
      : rule<create<ast::DefaultRule>(kw<"default"> + ruleHead)> {};
  struct PackageDecl
      : rule<kw<"package"> + assign<&ast::PackageDecl::path>(ref<Ref>)> {};
  struct ImportDecl
      : rule<kw<"import"> + assign<&ast::ImportDecl::path>(ref<Ref>) +
             option(kw<"as"> + assign<&ast::ImportDecl::alias>(ref<Ref>))> {
  };
  struct QueryStmt
      : rule<create<ast::QueryStmt>(
            (lit<"{"> + assign<&ast::QueryStmt::body>(ref<Query>) +
             lit<"}">) |
            assign<&ast::QueryStmt::body>(ref<TopQuery>))> {};
  struct Stmt
      : rule<ref<PackageDecl> | ref<ImportDecl> | ref<DefaultRule> |
             ref<Rule> | ref<QueryStmt>> {};
  struct Document
      : rule<create<ast::Document> +
             option(append<&ast::Document::statements>(ref<Stmt>) +
                    many((gap::line | !cr<"{[+-">)+append<
                         &ast::Document::statements>(ref<Stmt>)))> {};
  using entry = Document;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace rego
