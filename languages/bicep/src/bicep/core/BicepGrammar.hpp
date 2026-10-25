// The bicep language: its grammar.
#pragma once

#include <string_view>

#include "bicep/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace bicep {

inline constexpr auto trim_1_1 = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
inline constexpr auto trim_1_2 = [](std::string_view t) {
  return t.substr(1, t.size() - 3);
};
inline constexpr auto trim_3_3 = [](std::string_view t) {
  return t.substr(3, t.size() - 6);
};

struct Bicep : cursiv::GrammarSpec {
  // -- Trivia ----
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct SlComment : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct MlComment : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto NLRun = some(cr<R"(\r\n)">);
  struct NL : terminal<NLRun> {};
  // Swallows its own trailing NL, unlike an ordinary comment: the directive
  // line leaves no blank-statement NL behind it either.
  struct DisableNextLine
      : terminal<lit<"#disable-next-line"> + some(cr<R"(^\r\n)">) + NLRun> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(w)> {};

  // Soft keywords remain ordinary names where an identifier is expected.
  static constexpr auto IdentSpelling =
      ref<ID>;
  static constexpr auto IdentExprSpelling =
      ref<ID>;
  static constexpr auto TypeNameSpelling =
      ref<ID>;

  // -- Strings: '...', with '${' / '}' interpolation pieces ----
  static constexpr auto Escape =
      lit<"\\"> + (cr<R"(\\'nrt$)"> | (lit<"u{"> + some(cr<"0-9a-fA-F">) + lit<"}">));
  static constexpr auto StrRun = many(Escape | cr<R"(^\\'\n\r\t$)">);
  struct StrLeft : terminal<lit<"'"> + StrRun + lit<"${">, text_conv<trim_1_2>> {};
  struct StrMiddle : terminal<lit<"}"> + StrRun + lit<"${">, text_conv<trim_1_2>> {};
  struct StrRight : terminal<lit<"}"> + StrRun + lit<"'">, text_conv<trim_1_1>> {};
  struct StrComplete : terminal<lit<"'"> + StrRun + lit<"'">, text_conv<trim_1_1>> {};
  struct MultilineStr
      : terminal<through(lit<"'''">, lit<"'''">), text_conv<trim_3_3>> {};

  struct NUMBER : terminal<some(d) + option(lit<"."> + some(d))> {};

  // -- Forward declarations: the expression/type webs are mutually
  // recursive through member access, calls, arrays and objects. ----
  struct Expression;
  struct MemberExpr_;
  struct PrimaryExpr_;
  struct ObjectExpr_;
  struct ArrayExpr_;
  struct ForExpr_;
  struct IfExpr_;
  struct FunctionCall_;
  struct LambdaExpr_;
  struct ParenExpr_;
  struct InterpStringRule;
  struct Decorator_;
  struct TypeExpression;
  struct SingularTypeExpr;
  struct PrimaryTypeExpr;
  struct ParenTypeExpr;
  struct ObjectType_;
  struct TupleType_;
  struct ObjectTypeProperty_;
  struct ObjectTypeAdditionalProperties_;
  struct TupleItem_;

  // -- Interpolated strings ----
  static constexpr auto InterpBody =
      append<&ast::InterpString::quasis>(ref<StrLeft>) +
      many(append<&ast::InterpString::expressions>(ref<Expression>) +
           append<&ast::InterpString::quasis>(ref<StrMiddle>)) +
      append<&ast::InterpString::expressions>(ref<Expression>) +
      append<&ast::InterpString::quasis>(ref<StrRight>);
  struct InterpStringRule
      : rule<create<ast::InterpString>(InterpBody) |
             create<ast::InterpString>(
                 append<&ast::InterpString::quasis>(ref<StrComplete>))> {};

  // -- Number, bool, null and identifier literals ----
  struct NumberLit_ : rule<assign<&ast::NumberLit::text>(ref<NUMBER>)> {};
  struct BOOL_TOK : terminal<kw<"true"> | kw<"false">, bool> {};
  struct BoolLit_ : rule<assign<&ast::BoolLit::value>(ref<BOOL_TOK>)> {};
  struct NullLit_ : rule<create<ast::NullLit>(kw<"null">)> {};
  struct IdentExpr_
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(IdentExprSpelling))> {
  };

  struct LiteralValue
      : rule<ref<NumberLit_> | ref<BoolLit_> | ref<NullLit_> |
             ref<IdentExpr_>> {};
  // Type position: a plain name is already `TypeReference` below, so the
  // identifier alternative is dropped here too.
  struct LiteralValueNoIdent
      : rule<ref<NumberLit_> | ref<BoolLit_> | ref<NullLit_>> {};

  // -- Primary expressions ----
  struct FunctionCall_
      : rule<create<ast::FunctionCall>(
            assign<&ast::FunctionCall::name>(IdentSpelling) + lit<"("> +
            option(append<&ast::FunctionCall::args>(ref<Expression>) +
                   many(lit<","> +
                        append<&ast::FunctionCall::args>(ref<Expression>))) +
            lit<")">)> {};

  struct ParenExpr_
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
            lit<")">)> {};

  struct LambdaExpr_
      : rule<create<ast::LambdaExpr>(
            ((lit<"("> +
              option(append<&ast::LambdaExpr::params>(ref<Expression>) +
                     many(lit<","> +
                          append<&ast::LambdaExpr::params>(ref<Expression>))) +
              lit<")">) |
             append<&ast::LambdaExpr::params>(ref<IdentExpr_>)) +
            lit<"=>"> + assign<&ast::LambdaExpr::body>(ref<Expression>))> {};

  struct ObjectProperty_
      : rule<create<ast::ObjectProperty>(
            (assign<&ast::ObjectProperty::name>(IdentSpelling) |
             assign<&ast::ObjectProperty::nameExpr>(ref<InterpStringRule>)) +
            lit<":"> + assign<&ast::ObjectProperty::value>(ref<Expression>))> {
  };
  struct ObjectExpr_
      : rule<create<ast::ObjectExpr>(
            lit<"{"> +
            option(some(ref<NL>) +
                   many(append<&ast::ObjectExpr::properties>(
                            ref<ObjectProperty_>) +
                        some(ref<NL>))) +
            lit<"}">)> {};

  struct ArrayExpr_
      : rule<create<ast::ArrayExpr>(
            lit<"["> +
            option(some(ref<NL>) +
                   many(append<&ast::ArrayExpr::items>(ref<Expression>) +
                        some(ref<NL>))) +
            lit<"]">)> {};

  struct IfExpr_
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> +
            assign<&ast::IfExpr::condition>(ref<Expression>) + lit<")"> +
            assign<&ast::IfExpr::body>(ref<ObjectExpr_>))> {};

  static constexpr auto ForVarBlock =
      lit<"("> + assign<&ast::ForExpr::item>(IdentSpelling) + lit<","> +
      assign<&ast::ForExpr::index>(IdentSpelling) + lit<")">;
  struct ForExpr_
      : rule<create<ast::ForExpr>(
            lit<"["> + kw<"for"> +
            (assign<&ast::ForExpr::item>(IdentSpelling) | ForVarBlock) +
            kw<"in"> + assign<&ast::ForExpr::collection>(ref<Expression>) +
            lit<":"> +
            (assign<&ast::ForExpr::body>(ref<Expression>) |
             assign<&ast::ForExpr::guard>(ref<IfExpr_>)) +
            lit<"]">)> {};

  struct PrimaryExpr_
      : rule<ref<FunctionCall_> | ref<LiteralValue> | ref<InterpStringRule> |
             assign<&ast::MultilineString::text>(ref<MultilineStr>) |
             ref<ArrayExpr_> | ref<ForExpr_> | ref<ObjectExpr_> |
             ref<ParenExpr_> | ref<LambdaExpr_>> {};

  // -- Member access, indexing and calls (postfix chain) ----
  static constexpr auto MemberTail =
      nest<ast::IndexExpr, &ast::IndexExpr::object>(
          lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) +
          lit<"]">) |
      nest<ast::MethodCallExpr, &ast::MethodCallExpr::object>(
          lit<"."> + assign<&ast::MethodCallExpr::call>(ref<FunctionCall_>)) |
      nest<ast::PropertyExpr, &ast::PropertyExpr::object>(
          lit<"."> + assign<&ast::PropertyExpr::property>(IdentSpelling)) |
      nest<ast::NamedAccessExpr, &ast::NamedAccessExpr::object>(
          lit<":"> + assign<&ast::NamedAccessExpr::name>(IdentSpelling));

  struct MemberExpr_ : rule<ref<PrimaryExpr_> + many(MemberTail)> {};

  // -- Unary prefix operators ----
  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
            assign<&ast::UnaryExpr::op>(lit<"!"> | lit<"-"> | lit<"+">) +
            assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) |
             ref<MemberExpr_>> {};

  // -- multiplicative > additive > relational > equality > && > || > ?? ----
  struct BinaryExpr
      : infix<UnaryExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"*"> | lit<"/"> | lit<"%">) >>
               left(lit<"+"> | lit<"-">) >>
               left(lit<">="> | lit<"<="> | lit<">"> | lit<"<">) >>
               left(lit<"=="> | lit<"!=">) >>
               left(lit<"&&">) >>
               // The source grammar spells this operator `'OR'` (a
               // transcription defect); ported as the real `||`.
               left(lit<"||">) >>
               left(lit<"??">))> {};

  // -- The top-level expression, with its optional ternary ----
  struct Expression
      : rule<create<ast::CondExpr>(
            assign<&ast::CondExpr::cond>(ref<BinaryExpr>) + lit<"?"> +
            assign<&ast::CondExpr::thenValue>(ref<Expression>) + lit<":"> +
            assign<&ast::CondExpr::elseValue>(ref<Expression>)) |
             ref<BinaryExpr>> {};

  // -- Decorators ----
  // decoratorExpression restricts to `functionCall | memberExpression '.'
  // functionCall`; both shapes already occur inside the general member
  // chain, so the decorator's expression is read as one (a strict
  // superset).
  struct Decorator_
      : rule<create<ast::Decorator>(
            lit<"@"> + assign<&ast::Decorator::expression>(ref<MemberExpr_>) +
            ref<NL>)> {};

  // -- Types ----
  struct AmbientTypeRef
      : rule<create<ast::AmbientType>(assign<&ast::AmbientType::name>(
            // The source grammar lists INT twice and omits BOOL (a
            // transcription defect); ported with `bool` restored.
            kw<"string"> | kw<"int"> | kw<"bool"> | kw<"array"> |
            kw<"object">))> {};
  struct TypeReference
      : rule<create<ast::TypeRef>(
            assign<&ast::TypeRef::name>(TypeNameSpelling))> {};
  struct LiteralTypeExpr
      : rule<create<ast::LiteralType>(
            option(assign<&ast::LiteralType::op>(
                lit<"!"> | lit<"-"> | lit<"+">)) +
            assign<&ast::LiteralType::value>(ref<LiteralValueNoIdent>))> {};
  struct StringTypeExpr
      : rule<assign<&ast::StringType::text>(ref<StrComplete>)> {};
  struct MultilineStringTypeExpr
      : rule<assign<&ast::MultilineStringType::text>(ref<MultilineStr>)> {};

  struct ObjectTypeProperty_
      : rule<create<ast::ObjectTypeProperty>(
            many(append<&ast::ObjectTypeProperty::decorators>(
                ref<Decorator_>)) +
            assign<&ast::ObjectTypeProperty::name>(
                IdentSpelling | ref<StrComplete> | ref<MultilineStr>) +
            lit<":"> +
            assign<&ast::ObjectTypeProperty::type>(ref<TypeExpression>))> {};
  struct ObjectTypeAdditionalProperties_
      : rule<create<ast::ObjectTypeAdditionalProperties>(
            many(append<&ast::ObjectTypeAdditionalProperties::decorators>(
                ref<Decorator_>)) +
            lit<"*:"> +
            assign<&ast::ObjectTypeAdditionalProperties::type>(
                ref<TypeExpression>))> {};
  struct ObjectType_
      : rule<create<ast::ObjectType>(
            lit<"{"> +
            option(
                some(ref<NL>) +
                many((append<&ast::ObjectType::members>(
                          ref<ObjectTypeProperty_>) |
                      append<&ast::ObjectType::members>(
                          ref<ObjectTypeAdditionalProperties_>)) +
                     some(ref<NL>))) +
            lit<"}">)> {};

  struct TupleItem_
      : rule<create<ast::TupleItem>(
            many(append<&ast::TupleItem::decorators>(ref<Decorator_>)) +
            assign<&ast::TupleItem::type>(ref<TypeExpression>) +
            some(ref<NL>))> {};
  struct TupleType_
      : rule<create<ast::TupleType>(
            lit<"["> +
            option(some(ref<NL>) +
                   many(append<&ast::TupleType::items>(ref<TupleItem_>))) +
            lit<"]">)> {};

  struct PrimaryTypeExpr
      : rule<ref<AmbientTypeRef> | ref<TypeReference> | ref<LiteralTypeExpr> |
             ref<StringTypeExpr> | ref<MultilineStringTypeExpr> |
             ref<ObjectType_> | ref<TupleType_>> {};

  struct ParenTypeExpr
      : rule<create<ast::ParenType>(
            lit<"("> + assign<&ast::ParenType::inner>(ref<TypeExpression>) +
            lit<")">)> {};

  struct SingularTypeExpr
      : rule<(ref<PrimaryTypeExpr> | ref<ParenTypeExpr>) +
             many(nest<ast::ArrayType, &ast::ArrayType::element>(
                 lit<"["> + lit<"]">))> {};

  struct TypeExpression
      : rule<create<ast::UnionType>(
            append<&ast::UnionType::options>(ref<SingularTypeExpr>) +
            some(lit<"|"> +
                 append<&ast::UnionType::options>(ref<SingularTypeExpr>))) |
             ref<SingularTypeExpr>> {};

  // -- Top-level statements ----
  struct TargetScopeDecl_
      : rule<create<ast::TargetScopeDecl>(
            kw<"targetScope"> + lit<"="> +
            assign<&ast::TargetScopeDecl::value>(ref<Expression>))> {};

  struct ImportDecl_
      : rule<create<ast::ImportDecl>(
            many(append<&ast::ImportDecl::decorators>(ref<Decorator_>)) +
            kw<"import"> +
            assign<&ast::ImportDecl::specification>(ref<InterpStringRule>) +
            option(kw<"with"> +
                   assign<&ast::ImportDecl::withClause>(ref<ObjectExpr_>)) +
            option(kw<"as"> +
                   assign<&ast::ImportDecl::alias>(IdentSpelling)) +
            ref<NL>)> {};

  struct MetadataDecl_
      : rule<create<ast::MetadataDecl>(
            kw<"metadata"> +
            assign<&ast::MetadataDecl::name>(IdentSpelling) + lit<"="> +
            assign<&ast::MetadataDecl::value>(ref<Expression>) + ref<NL>)> {};

  struct ParameterDecl_
      : rule<create<ast::ParameterDecl>(
            many(append<&ast::ParameterDecl::decorators>(ref<Decorator_>)) +
            kw<"param"> + assign<&ast::ParameterDecl::name>(IdentSpelling) +
            ((assign<&ast::ParameterDecl::type>(ref<TypeExpression>) +
              option(lit<"="> + assign<&ast::ParameterDecl::defaultValue>(
                                     ref<Expression>))) |
             (kw<"resource"> +
              assign<&ast::ParameterDecl::resourceType>(
                  ref<InterpStringRule>) +
              option(lit<"="> + assign<&ast::ParameterDecl::defaultValue>(
                                     ref<Expression>)))) +
            ref<NL>)> {};

  struct TypeDecl_
      : rule<create<ast::TypeDecl>(
            many(append<&ast::TypeDecl::decorators>(ref<Decorator_>)) +
            kw<"type"> + assign<&ast::TypeDecl::name>(IdentSpelling) +
            lit<"="> + assign<&ast::TypeDecl::type>(ref<TypeExpression>) +
            ref<NL>)> {};

  struct VariableDecl_
      : rule<create<ast::VariableDecl>(
            many(append<&ast::VariableDecl::decorators>(ref<Decorator_>)) +
            kw<"var"> + assign<&ast::VariableDecl::name>(IdentSpelling) +
            lit<"="> + assign<&ast::VariableDecl::value>(ref<Expression>) +
            ref<NL>)> {};

  struct ResourceDecl_
      : rule<create<ast::ResourceDecl>(
            many(append<&ast::ResourceDecl::decorators>(ref<Decorator_>)) +
            kw<"resource"> +
            assign<&ast::ResourceDecl::name>(IdentSpelling) +
            assign<&ast::ResourceDecl::type>(ref<InterpStringRule>) +
            option(
                flag<&ast::ResourceDecl::existing>(kw<"existing">)) +
            lit<"="> +
            (assign<&ast::ResourceDecl::ifValue>(ref<IfExpr_>) |
             assign<&ast::ResourceDecl::object>(ref<ObjectExpr_>) |
             assign<&ast::ResourceDecl::forValue>(ref<ForExpr_>)) +
            ref<NL>)> {};

  struct ModuleDecl_
      : rule<create<ast::ModuleDecl>(
            many(append<&ast::ModuleDecl::decorators>(ref<Decorator_>)) +
            kw<"module"> + assign<&ast::ModuleDecl::name>(IdentSpelling) +
            assign<&ast::ModuleDecl::type>(ref<InterpStringRule>) +
            lit<"="> +
            (assign<&ast::ModuleDecl::ifValue>(ref<IfExpr_>) |
             assign<&ast::ModuleDecl::object>(ref<ObjectExpr_>) |
             assign<&ast::ModuleDecl::forValue>(ref<ForExpr_>)) +
            ref<NL>)> {};

  struct OutputDecl_
      : rule<create<ast::OutputDecl>(
            many(append<&ast::OutputDecl::decorators>(ref<Decorator_>)) +
            kw<"output"> + assign<&ast::OutputDecl::name>(IdentSpelling) +
            (assign<&ast::OutputDecl::typeName>(IdentSpelling) |
             (kw<"resource"> +
              assign<&ast::OutputDecl::resourceType>(
                  ref<InterpStringRule>))) +
            lit<"="> + assign<&ast::OutputDecl::value>(ref<Expression>) +
            ref<NL>)> {};

  static constexpr auto StatementAlt =
      ref<TargetScopeDecl_> | ref<ImportDecl_> | ref<MetadataDecl_> |
      ref<ParameterDecl_> | ref<TypeDecl_> | ref<VariableDecl_> |
      ref<ResourceDecl_> | ref<ModuleDecl_> | ref<OutputDecl_>;

  // A blank line is its own (void) statement, so runs of them between
  // declarations need no separate handling.
  struct Program_
      : rule<create<ast::Program>(
            many(append<&ast::Program::body>(StatementAlt) | ref<NL>))> {};

  using entry = Program_;
  using ignore = list<WS, SlComment, MlComment, DisableNextLine>;
};

}  // namespace bicep
