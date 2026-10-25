// The kotlin language: its grammar. Newlines: a bare newline is trivia
// (`NL_TRIVIA`, in `ignore`); `NEWLINE` is the same byte and is the token a
// state reads explicitly -- a statement ends on it (`Semi`/`Anysemi`), and it
// is blind (a join, not a token) inside `(`/`[` via
// `indent_open`/`indent_close`/`indent_newline`, the same mechanism an
// indentation-based grammar uses for implicit line joining. Everywhere else,
// per-state lexing already does the source grammar's job of allowing a line
// break: a raw newline is legal only where a rule reads `NEWLINE` explicitly
// or where nothing yet parsed could reduce to a complete statement, so it
// falls back to trivia.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "kotlin/core/ast.hpp"

namespace kotlin {

inline constexpr auto strip_first = [](std::string_view t) {
  return t.substr(1);
};
using StripFirst = cursiv::text_conv<strip_first>;

inline constexpr auto strip_return_at = [](std::string_view t) {
  return t.substr(7);
};
inline constexpr auto strip_continue_at = [](std::string_view t) {
  return t.substr(9);
};
inline constexpr auto strip_break_at = [](std::string_view t) {
  return t.substr(6);
};
using StripReturnAt = cursiv::text_conv<strip_return_at>;
using StripContinueAt = cursiv::text_conv<strip_continue_at>;
using StripBreakAt = cursiv::text_conv<strip_break_at>;

inline constexpr auto elvis_text = [] { return std::string_view("?:"); };
using ElvisConv = cursiv::text_conv<elvis_text>;

struct Kotlin : cursiv::GrammarSpec {
  // TRIVIA
  // ======

  struct ShebangLine : terminal<lit<"#!"> + many(cr<R"(^\n\r)">)> {};
  struct DelimitedComment
      : terminal<lit<"/*"> +
                 many(ref<DelimitedComment> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};
  struct LineComment : terminal<lit<"//"> + many(cr<R"(^\n\r)">)> {};
  struct WS : terminal<some(cr<R"( \t\f)">)> {};

  struct NEWLINE : terminal<eol> {};
  struct NL_TRIVIA : terminal<eol> {};

  static constexpr auto Anysemi = ref<NEWLINE> | lit<";">;
  static constexpr auto Semi = some(ref<NEWLINE>) | lit<";">;
  // A prefix that is already a complete statement on its own (a class with
  // no body, a property with no initializer, ...) makes the newline right
  // after it a legal statement end, competing with continuing on the next
  // line -- so a construct whose *next* piece is itself optional needs its
  // line crossed explicitly. `NLs` is that crossing.
  static constexpr auto NLs = many(ref<NEWLINE>);

  // An infix operator slot hands over text -- a literal, a terminal or a
  // data rule -- so a leading-newline-tolerant operator needs its own named
  // rule with a fixed converter, rather than `NLs + lit<>` spliced in place.
  // `&&`/`||` stay plain: giving both a leading NLs makes them both open on
  // NEWLINE within the same precedence table, which the table refuses.
  struct ElvisOp : rule<NLs + lit<"?:">, ElvisConv> {};

  // NUMBERS
  // =======

  static constexpr auto Letter = uc::L | uc::Nl;
  // Simplified from the source's underscore-position tracking (an
  // underscore is accepted anywhere after the first digit, not only between
  // two digits) and its leading-zero exclusion: both are over-acceptance.
  // The first character must be a real digit -- `1._foo()` (a member access
  // starting with `_`) would otherwise lex as `1.` `_` `foo()`, a malformed
  // float literal `1._` swallowing the `_` a real identifier needed.
  static constexpr auto Digits = uc::Nd + many(uc::Nd | lit<"_">);
  static constexpr auto ExponentPart = cr<"eE"> + option(cr<"+-">) + Digits;
  // A terminal's body reaches the lexer only through literals, ranges,
  // sequences, choices and repetitions -- never a `ref<>` to another
  // terminal -- so a shared piece is a plain expression, spliced into each.
  static constexpr auto DoubleBody =
      (option(Digits) + lit<"."> + Digits + option(ExponentPart)) |
      (Digits + ExponentPart);
  static constexpr auto HexBody =
      lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F_">);
  static constexpr auto BinBody = lit<"0"> + cr<"bB"> + some(cr<"01_">);

  struct DoubleLiteral : terminal<DoubleBody> {};
  struct FloatLiteral : terminal<(DoubleBody | Digits) + cr<"fF">> {};
  static constexpr auto IntegerSuffix = option(cr<"uU">) + option(cr<"lL">);
  struct IntegerLiteral : terminal<Digits + IntegerSuffix> {};
  struct HexLiteral : terminal<HexBody + IntegerSuffix> {};
  struct BinLiteral : terminal<BinBody + IntegerSuffix> {};
  struct BooleanLiteral : terminal<kw<"true"> | kw<"false">> {};
  struct NullLiteral : terminal<kw<"null">> {};

  static constexpr auto UnicodeEscape =
      lit<"\\u"> + repeat<4>(cr<"0-9a-fA-F">);
  static constexpr auto SimpleEscape = lit<"\\"> + cr<R"(tbrn'"\\$)">;
  struct CharacterLiteral
      : terminal<lit<"'"> + (UnicodeEscape | SimpleEscape | any) +
                 lit<"'">> {};

  // NAMES
  // =====

  static constexpr auto IdentifierBody =
      ((Letter | lit<"_">) + many(Letter | lit<"_"> | uc::Nd)) |
      (lit<"`"> + some(cr<"^`">) + lit<"`">);
  struct Identifier : terminal<IdentifierBody> {};
  struct LabelReference : terminal<lit<"@"> + IdentifierBody> {};
  struct LabelDefinition : terminal<IdentifierBody + lit<"@">> {};
  struct FieldIdentifier : terminal<lit<"$"> + IdentifierBody> {};

  struct ReturnAtLabel
      : terminal<lit<"return@"> + IdentifierBody, StripReturnAt> {};
  struct ContinueAtLabel
      : terminal<lit<"continue@"> + IdentifierBody, StripContinueAt> {};
  struct BreakAtLabel
      : terminal<lit<"break@"> + IdentifierBody, StripBreakAt> {};

  static constexpr auto word_boundary = Letter | uc::Nd | cr<"_">;
  struct NOT_IS : terminal<lit<"!is", !word_boundary>> {};
  struct NOT_IN : terminal<lit<"!in", !word_boundary>> {};

  // KEYWORDS referenced from more than one place: a modifier or a use-site
  // target needs a stable name so `SimpleIdentifier` (or, for a target, its
  // group form) can offer the same spelling a modifier list reads.
  struct ALL_SITE : terminal<kw<"@all">, StripFirst> {};
  struct FILE_SITE : terminal<kw<"@file">, StripFirst> {};
  struct FIELD_SITE : terminal<kw<"@field">, StripFirst> {};
  struct PROPERTY_SITE : terminal<kw<"@property">, StripFirst> {};
  struct GET_SITE : terminal<kw<"@get">, StripFirst> {};
  struct SET_SITE : terminal<kw<"@set">, StripFirst> {};
  struct RECEIVER_SITE : terminal<kw<"@receiver">, StripFirst> {};
  struct PARAM_SITE : terminal<kw<"@param">, StripFirst> {};
  struct SETPARAM_SITE : terminal<kw<"@setparam">, StripFirst> {};
  struct DELEGATE_SITE : terminal<kw<"@delegate">, StripFirst> {};
  static constexpr auto SiteTarget =
      ref<ALL_SITE> | ref<FIELD_SITE> | ref<FILE_SITE> | ref<PROPERTY_SITE> |
      ref<GET_SITE> | ref<SET_SITE> | ref<RECEIVER_SITE> | ref<PARAM_SITE> |
      ref<SETPARAM_SITE> | ref<DELEGATE_SITE>;

  struct VAL : terminal<kw<"val">> {};
  struct VAR : terminal<kw<"var">> {};
  struct IN : terminal<kw<"in">> {};
  struct OUT : terminal<kw<"out">> {};
  struct IS : terminal<kw<"is">> {};
  struct AS : terminal<kw<"as">> {};
  struct FIELD : terminal<kw<"field">> {};
  struct GETTER : terminal<kw<"get">> {};
  struct SETTER : terminal<kw<"set">> {};
  struct BY : terminal<kw<"by">> {};
  struct CONSTRUCTOR : terminal<kw<"constructor">> {};
  struct COMPANION : terminal<kw<"companion">> {};
  struct INIT : terminal<kw<"init">> {};
  struct WHERE : terminal<kw<"where">> {};
  struct CONTEXT : terminal<kw<"context">> {};
  struct IMPORT : terminal<kw<"import">> {};
  struct CATCH : terminal<kw<"catch">> {};
  struct FINALLY : terminal<kw<"finally">> {};
  struct DYNAMIC : terminal<kw<"dynamic">> {};

  struct PUBLIC : terminal<kw<"public">> {};
  struct PRIVATE : terminal<kw<"private">> {};
  struct PROTECTED : terminal<kw<"protected">> {};
  struct INTERNAL : terminal<kw<"internal">> {};
  struct ENUM : terminal<kw<"enum">> {};
  struct SEALED : terminal<kw<"sealed">> {};
  struct ANNOTATION : terminal<kw<"annotation">> {};
  struct DATA : terminal<kw<"data">> {};
  struct INNER : terminal<kw<"inner">> {};
  struct TAILREC : terminal<kw<"tailrec">> {};
  struct OPERATOR : terminal<kw<"operator">> {};
  struct INLINE : terminal<kw<"inline">> {};
  struct INFIX : terminal<kw<"infix">> {};
  struct EXTERNAL : terminal<kw<"external">> {};
  struct SUSPEND : terminal<kw<"suspend">> {};
  struct OVERRIDE : terminal<kw<"override">> {};
  struct ABSTRACT : terminal<kw<"abstract">> {};
  struct FINAL : terminal<kw<"final">> {};
  struct OPEN : terminal<kw<"open">> {};
  struct CONST : terminal<kw<"const">> {};
  struct LATEINIT : terminal<kw<"lateinit">> {};
  struct VARARG : terminal<kw<"vararg">> {};
  struct NOINLINE : terminal<kw<"noinline">> {};
  struct CROSSINLINE : terminal<kw<"crossinline">> {};
  struct REIFIED : terminal<kw<"reified">> {};
  struct VALUE : terminal<kw<"value">> {};
  struct FUN_INTERFACE : rule<kw<"fun"> + &kw<"interface">> {};

  static constexpr auto NonEnumModifierKw =
      ref<VALUE> | ref<FUN_INTERFACE> | ref<SEALED> | ref<ANNOTATION> |
      ref<DATA> | ref<INNER> | ref<OVERRIDE> | ref<LATEINIT> | ref<PUBLIC> |
      ref<PRIVATE> | ref<INTERNAL> | ref<PROTECTED> | ref<IN> | ref<OUT> |
      ref<TAILREC> | ref<OPERATOR> | ref<INFIX> | ref<INLINE> | ref<EXTERNAL> |
      ref<SUSPEND> | ref<CONST> | ref<ABSTRACT> | ref<FINAL> | ref<OPEN> |
      ref<VARARG> | ref<NOINLINE> | ref<CROSSINLINE> | ref<REIFIED>;
  static constexpr auto AllModifierKw = ref<ENUM> | NonEnumModifierKw;

  // FORWARD DECLARATIONS
  // ====================

  struct TypeRule;
  struct TypeProjectionRule;
  struct Annotation;
  struct Argument;
  struct Param;
  struct ContextParam;
  struct Expression;
  struct PrefixUnaryExpr;
  struct AtomicExpr;
  struct AtomicExprNoIdent;
  struct CallableReferenceExpr;
  struct FunctionLiteralRule;
  struct Block;
  struct ClassDeclarationRule;
  struct FunctionDeclarationRule;
  struct ObjectDeclarationRule;
  struct CompanionObjectRule;
  struct PropertyDeclarationRule;
  struct TypeAliasRule;
  struct SecondaryConstructorRule;
  struct InitBlockRule;
  struct ClassMemberRule;

  // NAMES / ANNOTATIONS
  // ===================

  struct SimpleIdentifier
      : rule<ref<Identifier> | ref<ABSTRACT> | ref<ANNOTATION> | ref<BY> |
             ref<CATCH> | ref<CONTEXT> | ref<COMPANION> | ref<CONSTRUCTOR> |
             ref<CROSSINLINE> | ref<DATA> | ref<DYNAMIC> | ref<ENUM> |
             ref<EXTERNAL> | ref<FIELD> | ref<FINAL> | ref<FINALLY> |
             ref<GETTER> | ref<IMPORT> | ref<INFIX> | ref<INIT> | ref<INLINE> |
             ref<INNER> | ref<INTERNAL> | ref<LATEINIT> | ref<NOINLINE> |
             ref<OPEN> | ref<OPERATOR> | ref<OUT> | ref<OVERRIDE> |
             ref<PRIVATE> | ref<PROTECTED> | ref<PUBLIC> | ref<REIFIED> |
             ref<SEALED> | ref<TAILREC> | ref<SETTER> | ref<VARARG> |
             ref<WHERE> | ref<CONST> | ref<SUSPEND> | ref<VALUE>> {};

  struct DottedName
      : rule<ref<SimpleIdentifier> + many(lit<"."> + ref<SimpleIdentifier>)> {
  };
  struct AtDottedName
      : rule<ref<LabelReference> + many(lit<"."> + ref<SimpleIdentifier>),
             StripFirst> {};

  struct Argument
      : rule<create<ast::Argument>(
            option(assign<&ast::Argument::name>(ref<SimpleIdentifier>) +
                   lit<"=">) +
            option(flag<&ast::Argument::spread>(lit<"*">)) +
            assign<&ast::Argument::value>(ref<Expression>))> {};

  // Member function templates returning `auto` cannot be called from a
  // nested class's own base-specifier (the enclosing class is still
  // incomplete there): every such helper is a variable template instead.
  template <auto ArgsPtr>
  static constexpr auto value_arguments =
      lit<"("> + many(append<ArgsPtr>(ref<Argument>), lit<",">) +
      option(lit<",">) + lit<")">;

  // `callSuffix`'s three forms, shared by an ordinary call and a supertype
  // constructor call: explicit type arguments with an optional call and
  // trailing lambdas, a call with trailing lambdas, or trailing lambdas
  // alone.
  template <auto TypeArgsPtr, auto ArgsPtr, auto LambdasPtr>
  static constexpr auto call_arguments_body =
      (lit<"<"> + append<TypeArgsPtr>(ref<TypeProjectionRule>) +
       many(lit<","> + append<TypeArgsPtr>(ref<TypeProjectionRule>)) +
       option(lit<",">) + lit<">"> + option(value_arguments<ArgsPtr>) +
       many(append<LambdasPtr>(ref<FunctionLiteralRule>))) |
      (value_arguments<ArgsPtr> +
       many(append<LambdasPtr>(ref<FunctionLiteralRule>)));
  template <auto TypeArgsPtr, auto ArgsPtr, auto LambdasPtr>
  static constexpr auto call_suffix_body =
      call_arguments_body<TypeArgsPtr, ArgsPtr, LambdasPtr> |
      some(append<LambdasPtr>(ref<FunctionLiteralRule>));

  static constexpr auto AnnotationTypeArguments =
      lit<"<"> +
      some(append<&ast::Annotation::typeArguments>(ref<TypeProjectionRule>),
           lit<",">) +
      option(lit<",">) + lit<">">;

  struct UnescapedAnnotation
      : rule<create<ast::Annotation>(
            assign<&ast::Annotation::name>(ref<DottedName>) +
            option(AnnotationTypeArguments) +
            option(value_arguments<&ast::Annotation::arguments>))> {};

  struct Annotation
      : rule<create<ast::Annotation>(
            ((assign<&ast::Annotation::useSiteTarget>(SiteTarget) + lit<":"> +
              assign<&ast::Annotation::name>(ref<DottedName>)) |
             assign<&ast::Annotation::name>(ref<AtDottedName>)) +
            option(AnnotationTypeArguments) +
            option(value_arguments<&ast::Annotation::arguments>))> {};

  struct AnnotationGroup
      : rule<create<ast::AnnotationGroup>(
            ((assign<&ast::AnnotationGroup::useSiteTarget>(SiteTarget) +
              lit<":">) |
             lit<"@">)+lit<"["> +
            some(append<&ast::AnnotationGroup::annotations>(
                ref<UnescapedAnnotation>)) +
            lit<"]">)> {};

  template <auto AnnsPtr>
  static constexpr auto annotation_modifier =
      append<AnnsPtr>(ref<Annotation> | ref<AnnotationGroup>);
  template <auto ModsPtr, auto AnnsPtr>
  static constexpr auto modifiers =
      many(append<ModsPtr>(AllModifierKw) | annotation_modifier<AnnsPtr>);

  template <auto ParamsPtr>
  static constexpr auto context_parameters =
      ref<CONTEXT> + lit<"("> +
      many(append<ParamsPtr>(ref<ContextParam>), lit<",">) + option(lit<",">) +
      lit<")">;

  template <auto ModsPtr, auto AnnsPtr, auto ParamsPtr>
  static constexpr auto contextual_modifiers =
      many(append<ModsPtr>(AllModifierKw) | annotation_modifier<AnnsPtr> |
           context_parameters<ParamsPtr>);

  // TYPES
  // =====

  struct TypeProjectionRule
      : rule<create<ast::TypeProjection>(
            flag<&ast::TypeProjection::isStar>(lit<"*">) |
            (option(assign<&ast::TypeProjection::variance>(ref<IN> |
                                                             ref<OUT>)) +
             assign<&ast::TypeProjection::type>(ref<TypeRule>)))> {};

  struct TypeSegmentRule
      : rule<create<ast::TypeSegment>(
            assign<&ast::TypeSegment::name>(ref<SimpleIdentifier>) +
            option(lit<"<"> +
                   many(append<&ast::TypeSegment::typeArguments>(
                            ref<TypeProjectionRule>),
                        lit<",">) +
                   option(lit<",">) + lit<">">))> {};

  static constexpr auto user_type_body =
      append<&ast::UserType::segments>(ref<TypeSegmentRule>) +
      many(lit<"."> + append<&ast::UserType::segments>(ref<TypeSegmentRule>));
  struct UserTypeRule : rule<create<ast::UserType>(user_type_body)> {};

  struct FunctionTypeParam
      : rule<create<ast::Param>(
            (assign<&ast::Param::name>(ref<SimpleIdentifier>) + lit<":"> +
             assign<&ast::Param::type>(ref<TypeRule>)) |
            assign<&ast::Param::type>(ref<TypeRule>))> {};

  struct FunctionTypeReceiver
      : rule<(ref<UserTypeRule> +
              option(nest<ast::NullableType, &ast::NullableType::base>(
                  some(lit<"?">)))) |
             (lit<"("> + ref<TypeRule> + lit<")">)> {};

  static constexpr auto function_type_body =
      option(context_parameters<&ast::FunctionType::contextParameters>) +
      option(assign<&ast::FunctionType::receiver>(ref<FunctionTypeReceiver>) +
             lit<".">) +
      lit<"("> +
      many(append<&ast::FunctionType::parameters>(ref<FunctionTypeParam>),
           lit<",">) +
      option(lit<",">) + lit<")"> + NLs + lit<"->"> + NLs +
      assign<&ast::FunctionType::returnType>(ref<TypeRule>);

  static constexpr auto type_modifier =
      annotation_modifier<&ast::Type::annotations> |
      flag<&ast::Type::isSuspend>(ref<SUSPEND>);
  static constexpr auto nullable_suffix = option(
      nest<ast::NullableType, &ast::NullableType::base>(some(lit<"?">)));

  struct TypeRule
      : rule<((many(type_modifier) +
               (create<ast::FunctionType>(function_type_body) |
                ((create<ast::UserType>(user_type_body) |
                  create<ast::DynamicType>(kw<"dynamic">)) +
                 nullable_suffix))) |
              (lit<"("> + ref<TypeRule> + lit<")"> + nullable_suffix) |
              (some(type_modifier) +
               create<ast::ParenthesizedType>(
                   lit<"("> +
                   assign<&ast::ParenthesizedType::type>(ref<TypeRule>) +
                   lit<")">) +
               nullable_suffix)) +
             many(nest<ast::IntersectionType, &ast::IntersectionType::left>(
                      lit<"&"> +
                      assign<&ast::IntersectionType::right>(ref<TypeRule>)) |
                  nest<ast::UnionType, &ast::UnionType::left>(
                      lit<"|"> +
                      assign<&ast::UnionType::right>(ref<TypeRule>)))> {};

  // PARAMETERS / DECLARATORS
  // ========================

  struct Param
      : rule<create<ast::Param>(
            modifiers<&ast::Param::modifiers, &ast::Param::annotations> +
            assign<&ast::Param::name>(ref<SimpleIdentifier>) + lit<":"> +
            assign<&ast::Param::type>(ref<TypeRule>) +
            option(lit<"="> +
                   assign<&ast::Param::defaultValue>(ref<Expression>)))> {};

  struct ContextParam
      : rule<ref<Param> |
             create<ast::Param>(assign<&ast::Param::type>(ref<TypeRule>))> {};

  struct SetterParam
      : rule<create<ast::Param>(
            modifiers<&ast::Param::modifiers, &ast::Param::annotations> +
            assign<&ast::Param::name>(ref<SimpleIdentifier>) +
            option(lit<":"> +
                   assign<&ast::Param::type>(ref<TypeRule>)))> {};

  struct ClassParamRule
      : rule<create<ast::ClassParam>(
            modifiers<&ast::ClassParam::modifiers,
                      &ast::ClassParam::annotations> +
            option(assign<&ast::ClassParam::valOrVar>(ref<VAL> | ref<VAR>)) +
            assign<&ast::ClassParam::name>(ref<SimpleIdentifier>) +
            lit<":"> + assign<&ast::ClassParam::type>(ref<TypeRule>) +
            option(lit<"="> + assign<&ast::ClassParam::defaultValue>(
                                   ref<Expression>)))> {};

  struct VariableDeclaratorRule
      : rule<create<ast::VariableDeclarator>(
            assign<&ast::VariableDeclarator::name>(ref<SimpleIdentifier>) +
            option(lit<":"> +
                   assign<&ast::VariableDeclarator::type>(ref<TypeRule>)))> {
  };

  struct TypeParamRule
      : rule<create<ast::TypeParam>(
            modifiers<&ast::TypeParam::modifiers,
                      &ast::TypeParam::annotations> +
            (flag<&ast::TypeParam::isStar>(lit<"*">) |
             assign<&ast::TypeParam::name>(ref<SimpleIdentifier>)) +
            option(lit<":"> +
                   assign<&ast::TypeParam::bound>(ref<TypeRule>)))> {};

  template <auto Ptr>
  static constexpr auto type_parameters =
      lit<"<"> + append<Ptr>(ref<TypeParamRule>) +
      many(lit<","> + append<Ptr>(ref<TypeParamRule>)) + option(lit<",">) +
      lit<">">;

  struct TypeConstraintRule
      : rule<create<ast::TypeConstraint>(
            many(append<&ast::TypeConstraint::annotations>(ref<Annotation>)) +
            assign<&ast::TypeConstraint::name>(ref<SimpleIdentifier>) +
            lit<":"> +
            assign<&ast::TypeConstraint::bound>(ref<TypeRule>))> {};

  template <auto Ptr>
  static constexpr auto type_constraints =
      ref<WHERE> + append<Ptr>(ref<TypeConstraintRule>) +
      many(lit<","> + append<Ptr>(ref<TypeConstraintRule>));

  struct LambdaParamRule
      : rule<create<ast::LambdaParam>(
            (append<&ast::LambdaParam::declarators>(
                 ref<VariableDeclaratorRule>) |
             (lit<"("> +
              append<&ast::LambdaParam::declarators>(
                  ref<VariableDeclaratorRule>) +
              many(lit<","> + append<&ast::LambdaParam::declarators>(
                                   ref<VariableDeclaratorRule>)) +
              lit<")">)) +
            option(lit<":"> +
                   assign<&ast::LambdaParam::type>(ref<TypeRule>)))> {};

  // SUPERTYPES
  // ==========

  struct SuperTypeSpecifier
      : rule<create<ast::SuperTypeDelegate>(
                 many(append<&ast::SuperType::annotations>(ref<Annotation>)) +
                 assign<&ast::SuperTypeDelegate::type>(ref<UserTypeRule>) +
                 ref<BY> + assign<&ast::SuperTypeDelegate::delegate>(
                               ref<Expression>)) |
            create<ast::SuperTypeCall>(
                many(append<&ast::SuperType::annotations>(ref<Annotation>)) +
                assign<&ast::SuperTypeCall::type>(ref<UserTypeRule>) +
                call_suffix_body<&ast::SuperTypeCall::typeArguments,
                                 &ast::SuperTypeCall::arguments,
                                 &ast::SuperTypeCall::trailingLambdas>) |
            create<ast::SuperTypeEntry>(
                many(append<&ast::SuperType::annotations>(ref<Annotation>)) +
                assign<&ast::SuperTypeEntry::type>(ref<UserTypeRule>))> {};

  template <auto Ptr>
  static constexpr auto super_types =
      lit<":"> + append<Ptr>(ref<SuperTypeSpecifier>) +
      many(lit<","> + append<Ptr>(ref<SuperTypeSpecifier>));

  // STRING LITERALS
  // ===============

  struct LineStrExprStart : terminal<lit<"${">> {};
  struct StrRef : terminal<lit<"$"> + IdentifierBody, StripFirst> {};
  struct StrThis : terminal<kw<"$this">> {};
  struct StrDollar : terminal<lit<"$">> {};
  struct StrText : terminal<some(cr<R"(^\\"$)">)> {};
  struct LineStrEscape : terminal<UnicodeEscape | SimpleEscape> {};
  struct MultiLineStrText : terminal<some(cr<R"(^"$)">)> {};
  // A run of one or two quotes is content; three open the closer -- the
  // guard rejects a third, so it cannot sit inside `MultiLineStrText`'s own
  // repetition (the token graph refuses a guard there) and is instead its
  // own repeatable terminal.
  struct MultiLineQuotes
      : terminal<lit<"\""> + option(lit<"\"">) + !lit<"\"">> {};

  inline static constexpr auto drop_closing_quotes = [](std::string_view text) {
    return text.substr(0, text.size() - 3);
  };
  struct MultiLineClose : terminal<repeat<3, cursiv::kUnbounded>(lit<"\"">),
                                   text_conv<drop_closing_quotes>> {};

  static constexpr auto StringReferencePart =
      append<&ast::StringLiteral::parts>(
          create<ast::StringPart>(assign<&ast::StringPart::expr>(
              create<ast::ThisExpr>(ref<StrThis>) |
              create<ast::Ident>(assign<&ast::Ident::name>(ref<StrRef>)))));

  static constexpr auto StringExprPart =
      append<&ast::StringLiteral::parts>(create<ast::StringPart>(
          ref<LineStrExprStart> +
          assign<&ast::StringPart::expr>(ref<Expression>) + lit<"}">));

  struct StringLiteralRule
      : rule<create<ast::StringLiteral>(
            (flag<&ast::StringLiteral::multiline>(lit<"\"\"\"">) +
             many(append<&ast::StringLiteral::parts>(create<ast::StringPart>(
                      assign<&ast::StringPart::text>(ref<MultiLineStrText> |
                                                     ref<MultiLineQuotes> |
                                                     ref<StrDollar>))) |
                  StringReferencePart | StringExprPart) +
             append<&ast::StringLiteral::parts>(create<ast::StringPart>(
                 assign<&ast::StringPart::text>(ref<MultiLineClose>)))) |
            (lit<"\""> +
             many(
                 append<&ast::StringLiteral::parts>(
                     create<ast::StringPart>(assign<&ast::StringPart::text>(
                         ref<StrText> | ref<LineStrEscape> | ref<StrDollar>))) |
                 StringReferencePart | StringExprPart) +
             lit<"\"">))> {};

  // EXPRESSIONS
  // ===========

  struct FunctionLiteralRule;
  struct IfExprRule;
  struct WhenExprRule;
  struct TryExprRule;
  struct ObjectLiteralRule;
  struct CollectionLiteralRule;
  struct ForExprRule;
  struct WhileExprRule;
  struct DoWhileExprRule;
  struct CallableReferenceExpr;

  struct ThisExprRule
      : rule<create<ast::ThisExpr>(
            kw<"this"> +
            option(assign<&ast::ThisExpr::label>(ref<LabelReference>)))> {};
  struct SuperExprRule
      : rule<create<ast::SuperExpr>(
            kw<"super"> +
            option(lit<"<"> +
                   assign<&ast::SuperExpr::targetType>(ref<TypeRule>) +
                   lit<">">) +
            option(assign<&ast::SuperExpr::label>(ref<LabelReference>)))> {};

  struct ThrowExprRule
      : rule<create<ast::ThrowExpr>(
            kw<"throw"> +
            assign<&ast::ThrowExpr::value>(ref<Expression>))> {};
  struct ReturnExprRule
      : rule<create<ast::ReturnExpr>(
            (kw<"return"> |
             assign<&ast::ReturnExpr::label>(ref<ReturnAtLabel>)) +
            option(assign<&ast::ReturnExpr::value>(ref<Expression>)))> {};
  struct BreakExprRule
      : rule<create<ast::BreakExpr>(
            kw<"break"> |
            assign<&ast::BreakExpr::label>(ref<BreakAtLabel>))> {};
  struct ContinueExprRule
      : rule<create<ast::ContinueExpr>(
            kw<"continue"> |
            assign<&ast::ContinueExpr::label>(ref<ContinueAtLabel>))> {};

  static constexpr auto ControlBody = ref<Block> | ref<Expression>;

  // Every `atomicExpression` alternative except a bare name: shared with
  // `AtomicExpr` below, and with a member-access RHS, whose own leading
  // alternative already reads a bare name as `MemberExpr::member` -- folding
  // it in here too would make a plain `a.b` genuinely ambiguous between the
  // two.
  struct AtomicExprNoIdent
      : rule<(lit<"("> + ref<Expression> + lit<")">) | ref<StringLiteralRule> |
             create<ast::Literal>(assign<&ast::Literal::text>(
                 ref<BooleanLiteral> | ref<CharacterLiteral> | ref<HexLiteral> |
                 ref<BinLiteral> | ref<DoubleLiteral> | ref<FloatLiteral> |
                 ref<IntegerLiteral> | ref<NullLiteral>)) |
             ref<FunctionLiteralRule> | ref<ThisExprRule> | ref<SuperExprRule> |
             ref<IfExprRule> | ref<WhenExprRule> | ref<TryExprRule> |
             ref<ObjectLiteralRule> | ref<ThrowExprRule> | ref<ReturnExprRule> |
             ref<ContinueExprRule> | ref<BreakExprRule> | ref<ForExprRule> |
             ref<WhileExprRule> | ref<DoWhileExprRule> |
             ref<CollectionLiteralRule> |
             create<ast::Ident>(
                 flag<&ast::Ident::hasValPrefix>(ref<VAL>) +
                 assign<&ast::Ident::name>(ref<SimpleIdentifier>))> {};

  struct AtomicExpr
      : rule<ref<AtomicExprNoIdent> |
            create<ast::Ident>(
                assign<&ast::Ident::name>(ref<SimpleIdentifier>))> {};

  struct CallableReferenceExpr
      : rule<create<ast::CallableReferenceExpr>(
            (option(assign<&ast::CallableReferenceExpr::receiverType>(
                        ref<UserTypeRule>) +
                    many(lit<"?">)) +
             (flag<&ast::CallableReferenceExpr::safe>(lit<"?::">) |
              lit<"::">) +
             (flag<&ast::CallableReferenceExpr::isClass>(kw<"class">) |
              assign<&ast::CallableReferenceExpr::name>(ref<DottedName>))) |
            (flag<&ast::CallableReferenceExpr::isThisReceiver>(
                 kw<"this">) +
             lit<"::"> +
             flag<&ast::CallableReferenceExpr::isClass>(
                 kw<"class">)))> {};

  static constexpr auto ArgumentsCallBody =
      call_arguments_body<&ast::CallExpr::typeArguments,
                          &ast::CallExpr::arguments,
                          &ast::CallExpr::trailingLambdas>;
  static constexpr auto LambdasCall =
      nest<ast::CallExpr, &ast::CallExpr::callee>(
          some(append<&ast::CallExpr::trailingLambdas>(
              ref<FunctionLiteralRule>)));

  // Lambdas alone never make a call after another call: it takes them itself.
  struct PostfixUnaryExpr
      : rule<(ref<AtomicExpr> | ref<CallableReferenceExpr>) +
            option(LambdasCall) +
            many(((nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                       assign<&ast::PostfixExpr::op>(
                           lit<"++"> | lit<"--"> | lit<"!!">)) |
                   nest<ast::IndexExpr, &ast::IndexExpr::array>(
                       lit<"["> +
                       append<&ast::IndexExpr::indices>(ref<Expression>) +
                       many(lit<","> + append<&ast::IndexExpr::indices>(
                                            ref<Expression>)) +
                       lit<"]">) |
                   nest<ast::MemberExpr, &ast::MemberExpr::object>(
                       NLs +
                       (flag<&ast::MemberExpr::safe>(lit<"?"> + lit<".">) |
                        lit<".">) +
                       (assign<&ast::MemberExpr::member>(
                            ref<SimpleIdentifier>) |
                        assign<&ast::MemberExpr::memberExpr>(
                            ref<AtomicExprNoIdent>)))) +
                  option(LambdasCall)) |
                 nest<ast::CallExpr, &ast::CallExpr::callee>(
                     ArgumentsCallBody))> {};

  struct PrefixUnaryExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--"> |
                                               lit<"+"> | lit<"-"> |
                                               lit<"!">) +
                 assign<&ast::PrefixExpr::operand>(ref<PrefixUnaryExpr>)) |
            create<ast::AnnotatedExpr>(
                some(append<&ast::AnnotatedExpr::annotations>(
                    ref<Annotation>)) +
                assign<&ast::AnnotatedExpr::operand>(ref<PrefixUnaryExpr>)) |
            create<ast::LabeledExpr>(
                assign<&ast::LabeledExpr::label>(ref<LabelDefinition>) +
                assign<&ast::LabeledExpr::operand>(ref<PrefixUnaryExpr>)) |
            ref<PostfixUnaryExpr>> {};

  // typeRHS(as/as?/:) sits outside the infix table: its right side is a
  // `TypeRule`, not another expression, so it cannot share `BinaryExpr`'s
  // uniform left/right member pair the way every other level does.
  struct TypeOpChain
      : rule<ref<PrefixUnaryExpr> +
            many(nest<ast::CastExpr, &ast::CastExpr::operand>(
                NLs + assign<&ast::CastExpr::op>(ref<AS> | lit<"as?"> |
                                                  lit<":">) +
                assign<&ast::CastExpr::type>(ref<TypeRule>)))> {};

  struct ElvisChain
      : infix<TypeOpChain, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"*"> | lit<"/"> | lit<"%">) >>
               left(lit<"+"> | lit<"-">) >> left(lit<"..<"> | lit<"..">) >>
               left(ref<SimpleIdentifier>) >> left(ref<ElvisOp>))> {};

  struct NamedInfixExpr
      : rule<ref<ElvisChain> +
            option(
                some(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                    assign<&ast::BinaryExpr::op>(ref<IN> | ref<NOT_IN>) +
                    assign<&ast::BinaryExpr::right>(ref<ElvisChain>))) |
                nest<ast::TypeTestExpr, &ast::TypeTestExpr::operand>(
                    (flag<&ast::TypeTestExpr::negated>(ref<NOT_IS>) |
                     ref<IS>) +
                    assign<&ast::TypeTestExpr::type>(ref<TypeRule>)))> {};

  // comparison > equality
  struct ComparisonChain
      : infix<NamedInfixExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
             &ast::BinaryExpr::right,
             (left(lit<"<="> | lit<">="> | lit<"<"> | lit<">">) >>
              left(lit<"!=="> | lit<"!="> | lit<"==="> | lit<"==">))> {};

  // `&&` and `||` both need a leading-NLs tolerance (`NL* CONJ`/`NL* DISJ`
  // in the source); an `infix<>` table refuses two operators that both open
  // on NEWLINE at two levels (`precedenceClash`), so each level is its own
  // hand-written left-recursive rule instead, the same shape as
  // `TypeOpChain`.
  struct ConjChain
      : rule<ref<ComparisonChain> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                NLs + assign<&ast::BinaryExpr::op>(lit<"&&">) +
                assign<&ast::BinaryExpr::right>(ref<ComparisonChain>)))> {};

  struct DisjChain
      : rule<ref<ConjChain> +
            many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                NLs + assign<&ast::BinaryExpr::op>(lit<"||">) +
                assign<&ast::BinaryExpr::right>(ref<ConjChain>)))> {};

  // comparison > equality > && > || > assignment
  struct Expression
      : infix<DisjChain, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
             &ast::BinaryExpr::right,
             right(lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> |
                   lit<"%="> | lit<"=">)> {};

  // WhenEntry conditions reuse the binary/type-test nodes with an implicit
  // (null) left operand.
  struct WhenConditionExpr
      : rule<ref<Expression> |
            create<ast::BinaryExpr>(
                assign<&ast::BinaryExpr::op>(ref<IN> | ref<NOT_IN>) +
                assign<&ast::BinaryExpr::right>(ref<Expression>)) |
            create<ast::TypeTestExpr>(
                (flag<&ast::TypeTestExpr::negated>(ref<NOT_IS>) |
                 ref<IS>) +
                assign<&ast::TypeTestExpr::type>(ref<TypeRule>))> {};

  struct WhenEntryRule
      : rule<create<ast::WhenEntry>(
            (append<&ast::WhenEntry::conditions>(ref<WhenConditionExpr>) +
             many(lit<","> +
                  append<&ast::WhenEntry::conditions>(ref<WhenConditionExpr>)) +
             option(kw<"if"> +
                    assign<&ast::WhenEntry::guard>(ref<Expression>)) +
             lit<"->"> + assign<&ast::WhenEntry::body>(ControlBody) +
             option(Semi)) |
            (flag<&ast::WhenEntry::isElse>(kw<"else">) +
             option(kw<"if"> +
                    assign<&ast::WhenEntry::guard>(ref<Expression>)) +
             lit<"->"> + assign<&ast::WhenEntry::body>(ControlBody)))> {};

  struct WhenExprRule
      : rule<create<ast::WhenExpr>(
            kw<"when"> +
            option(lit<"("> +
                   (assign<&ast::WhenExpr::subjectDeclaration>(
                        ref<PropertyDeclarationRule>) |
                    assign<&ast::WhenExpr::subject>(ref<Expression>)) +
                   lit<")">) +
            lit<"{"> +
            many(append<&ast::WhenExpr::entries>(ref<WhenEntryRule>)) +
            lit<"}">)> {};

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<Expression>) +
            lit<")"> + option(assign<&ast::IfExpr::thenBranch>(ControlBody)) +
            option(lit<";">) +
            option(commit(kw<"else">, option(assign<&ast::IfExpr::elseBranch>(ControlBody)))))> {
  };

  struct CatchClauseRule
      : rule<create<ast::CatchClause>(
            ref<CATCH> + lit<"("> +
            many(append<&ast::CatchClause::annotations>(ref<Annotation>)) +
            assign<&ast::CatchClause::name>(ref<SimpleIdentifier>) +
            lit<":"> + assign<&ast::CatchClause::type>(ref<UserTypeRule>) +
            lit<")"> + assign<&ast::CatchClause::body>(ref<Block>))> {};

  struct TryExprRule
      : rule<create<ast::TryExpr>(
            kw<"try"> + assign<&ast::TryExpr::tryBlock>(ref<Block>) +
            many(append<&ast::TryExpr::catches>(ref<CatchClauseRule>)) +
            option(ref<FINALLY> +
                   assign<&ast::TryExpr::finallyBlock>(ref<Block>)))> {};

  struct ObjectLiteralRule
      : rule<create<ast::ObjectLiteral>(
            kw<"object"> +
            option(super_types<&ast::ObjectLiteral::superTypes>) +
            option(lit<"{"> +
                   many(append<&ast::ObjectLiteral::members>(
                       ref<ClassMemberRule>)) +
                   lit<"}">))> {};

  struct CollectionLiteralRule
      : rule<create<ast::CollectionLiteral>(
            lit<"["> +
            option(append<&ast::CollectionLiteral::items>(ref<Expression>) +
                   many(lit<","> + append<&ast::CollectionLiteral::items>(
                                        ref<Expression>))) +
            lit<"]">)> {};

  struct ForExprRule
      : rule<create<ast::ForExpr>(
            kw<"for"> + lit<"("> +
            many(annotation_modifier<&ast::ForExpr::annotations>) +
            (append<&ast::ForExpr::declarators>(ref<VariableDeclaratorRule>) |
             (lit<"("> +
              append<&ast::ForExpr::declarators>(ref<VariableDeclaratorRule>) +
              many(lit<","> + append<&ast::ForExpr::declarators>(
                                  ref<VariableDeclaratorRule>)) +
              lit<")">)) +
            ref<IN> + assign<&ast::ForExpr::iterable>(ref<Expression>) +
            lit<")"> + option(assign<&ast::ForExpr::body>(ControlBody)))> {};

  struct WhileExprRule
      : rule<create<ast::WhileExpr>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileExpr::cond>(ref<Expression>) + lit<")"> +
            option(assign<&ast::WhileExpr::body>(ControlBody)))> {};

  struct DoWhileExprRule
      : rule<create<ast::DoWhileExpr>(
            kw<"do"> + option(assign<&ast::DoWhileExpr::body>(ControlBody)) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoWhileExpr::cond>(ref<Expression>) + lit<")">)> {};

  // BLOCKS / STATEMENTS
  // ===================

  // `declaration: labelDefinition* (classDeclaration | functionDeclaration |
  // propertyDeclaration | typeAlias);` -- the label prefix is real Kotlin
  // syntax (unused, but not refused) on any of the four, at statement level
  // only (`topLevelObject` has no such prefix).
  struct LocalDeclaration
      : rule<many(ref<LabelDefinition>) +
            (ref<ClassDeclarationRule> | ref<FunctionDeclarationRule> |
             ref<PropertyDeclarationRule> | ref<TypeAliasRule>)> {};

  static constexpr auto StatementItem =
      ref<LocalDeclaration> | ref<ObjectDeclarationRule> |
      ref<CompanionObjectRule> | ref<SecondaryConstructorRule> |
      ref<InitBlockRule> | ref<Expression>;

  template <auto Ptr>
  static constexpr auto statements_into =
      many(Anysemi) +
      option(some(append<Ptr>(StatementItem), some(Anysemi)) + many(Anysemi));

  struct Block
      : rule<create<ast::Block>(lit<"{"> +
                                statements_into<&ast::Block::statements> +
                                lit<"}">)> {};

  struct FunctionLiteralRule
      : rule<create<ast::FunctionLiteral>(
            many(annotation_modifier<&ast::FunctionLiteral::annotations>) +
            lit<"{"> +
            option(many(append<&ast::FunctionLiteral::parameters>(
                            ref<LambdaParamRule>),
                        lit<",">) +
                   lit<"->">) +
            statements_into<&ast::FunctionLiteral::body> + lit<"}">)> {};

  // DECLARATIONS
  // ============

  struct PrimaryConstructorRule
      : rule<create<ast::PrimaryConstructor>(
            modifiers<&ast::PrimaryConstructor::modifiers,
                      &ast::PrimaryConstructor::annotations> +
            option(ref<CONSTRUCTOR>) + lit<"("> +
            many(append<&ast::PrimaryConstructor::params>(
                     ref<ClassParamRule>),
                 lit<",">) +
            option(lit<",">) + lit<")">)> {};

  struct SecondaryConstructorRule
      : rule<create<ast::SecondaryConstructor>(
            modifiers<&ast::SecondaryConstructor::modifiers,
                      &ast::SecondaryConstructor::annotations> +
            ref<CONSTRUCTOR> + lit<"("> +
            many(append<&ast::SecondaryConstructor::params>(ref<Param>),
                 lit<",">) +
            option(lit<",">) + lit<")"> +
            option(lit<":"> +
                   assign<&ast::SecondaryConstructor::delegationKind>(
                       kw<"this"> | kw<"super">) +
                   value_arguments<&ast::SecondaryConstructor::delegationArgs>) +
            option(assign<&ast::SecondaryConstructor::body>(ref<Block>)))> {};

  struct EnumEntryRule
      : rule<create<ast::EnumEntry>(
            many(append<&ast::EnumEntry::annotations>(ref<Annotation>)) +
            assign<&ast::EnumEntry::name>(ref<SimpleIdentifier>) +
            option(value_arguments<&ast::EnumEntry::arguments>) +
            option(lit<"{"> +
                   many(append<&ast::EnumEntry::body>(ref<ClassMemberRule>)) +
                   lit<"}">) +
            option(lit<",">))> {};

  struct InitBlockRule
      : rule<create<ast::InitBlock>(
            ref<INIT> + assign<&ast::InitBlock::body>(ref<Block>))> {};

  struct ClassMemberRule
      : rule<(ref<ClassDeclarationRule> | ref<FunctionDeclarationRule> |
             ref<ObjectDeclarationRule> | ref<CompanionObjectRule> |
             ref<PropertyDeclarationRule> | ref<InitBlockRule> |
             ref<SecondaryConstructorRule> | ref<TypeAliasRule>) +
            many(Anysemi)> {};

  static constexpr auto class_modifiers =
      many(append<&ast::ClassDecl::modifiers>(NonEnumModifierKw) |
           annotation_modifier<&ast::ClassDecl::annotations> |
           context_parameters<&ast::ClassDecl::contextParameters>);
  static constexpr auto class_head =
      (flag<&ast::ClassDecl::isInterface>(kw<"interface">) |
       kw<"class">) +
      assign<&ast::ClassDecl::name>(ref<SimpleIdentifier>) + NLs +
      option(type_parameters<&ast::ClassDecl::typeParameters> + NLs) +
      option(assign<&ast::ClassDecl::primaryCtor>(
                 ref<PrimaryConstructorRule>) +
             NLs) +
      option(super_types<&ast::ClassDecl::superTypes> + NLs) +
      option(type_constraints<&ast::ClassDecl::typeConstraints> + NLs);
  static constexpr auto class_members =
      many(append<&ast::ClassDecl::members>(ref<ClassMemberRule>));

  // Entries exist only in an enum class, and its members only past a `;`.
  // The enum arm comes first: it names `enumEntries` before `members`, the
  // order the reflection visits them in.
  struct ClassDeclarationRule
      : rule<create<ast::ClassDecl>(
            class_modifiers +
            ((append<&ast::ClassDecl::modifiers>(ref<ENUM>) +
              class_modifiers + class_head +
              option(lit<"{"> +
                     many(append<&ast::ClassDecl::enumEntries>(
                         ref<EnumEntryRule>)) +
                     option(lit<";"> + class_members) + lit<"}">)) |
             (class_head +
              option(lit<"{"> + option(lit<";">) + class_members +
                     lit<"}">))))> {};

  struct ObjectDeclarationRule
      : rule<create<ast::ObjectDecl>(
            modifiers<&ast::ObjectDecl::modifiers,
                      &ast::ObjectDecl::annotations> +
            kw<"object"> + assign<&ast::ObjectDecl::name>(ref<SimpleIdentifier>) +
            NLs +
            option(assign<&ast::ObjectDecl::primaryCtor>(
                       ref<PrimaryConstructorRule>) +
                   NLs) +
            option(super_types<&ast::ObjectDecl::superTypes> + NLs) +
            option(lit<"{"> +
                   many(append<&ast::ObjectDecl::members>(
                       ref<ClassMemberRule>)) +
                   lit<"}">))> {};

  struct CompanionObjectRule
      : rule<create<ast::CompanionObjectDecl>(
            modifiers<&ast::CompanionObjectDecl::modifiers,
                      &ast::CompanionObjectDecl::annotations> +
            ref<COMPANION> +
            modifiers<&ast::CompanionObjectDecl::modifiers,
                      &ast::CompanionObjectDecl::annotations> +
            kw<"object"> + NLs +
            option(assign<&ast::CompanionObjectDecl::name>(
                       ref<SimpleIdentifier>) +
                   NLs) +
            option(super_types<&ast::CompanionObjectDecl::superTypes> + NLs) +
            option(lit<"{"> +
                   many(append<&ast::CompanionObjectDecl::members>(
                       ref<ClassMemberRule>)) +
                   lit<"}">))> {};

  // Its own rule so the extension-receiver's trailing `?` -- ordinarily
  // optional at the very end of a `type` -- has a reduction boundary before
  // the mandatory `.` that follows it here.
  struct ReceiverType
      : rule<ref<TypeRule> +
            option(nest<ast::NullableType, &ast::NullableType::base>(
                some(lit<"?">)))> {};

  struct FunctionDeclarationRule
      : rule<create<ast::FunctionDecl>(
            contextual_modifiers<&ast::FunctionDecl::modifiers,
                                 &ast::FunctionDecl::annotations,
                                 &ast::FunctionDecl::contextParameters> +
            kw<"fun"> +
            option(type_parameters<&ast::FunctionDecl::typeParameters>) +
            option(assign<&ast::FunctionDecl::receiverType>(ref<ReceiverType>) +
                   lit<".">) +
            option(assign<&ast::FunctionDecl::name>(ref<SimpleIdentifier>)) +
            option(type_parameters<&ast::FunctionDecl::typeParameters>) +
            lit<"("> +
            many(append<&ast::FunctionDecl::valueParameters>(ref<Param>),
                 lit<",">) +
            option(lit<",">) + lit<")"> + NLs +
            option(lit<":"> +
                   assign<&ast::FunctionDecl::returnType>(ref<TypeRule>) +
                   NLs) +
            option(type_constraints<&ast::FunctionDecl::typeConstraints> +
                   NLs) +
            option(assign<&ast::FunctionDecl::body>(
                ref<Block> | (lit<"="> + ref<Expression>))))> {};

  struct GetterRule
      : rule<create<ast::Getter>(
            modifiers<&ast::Getter::modifiers, &ast::Getter::annotations> +
            ref<GETTER> +
            option(lit<"("> + lit<")"> +
                   option(lit<":"> +
                          assign<&ast::Getter::returnType>(ref<TypeRule>)) +
                   assign<&ast::Getter::body>(
                       ref<Block> | (lit<"="> + ref<Expression>))))> {};

  struct SetterRule
      : rule<create<ast::Setter>(
            modifiers<&ast::Setter::modifiers, &ast::Setter::annotations> +
            ref<SETTER> +
            option(lit<"("> + assign<&ast::Setter::param>(ref<SetterParam>) +
                   lit<")"> +
                   assign<&ast::Setter::body>(
                       ref<Block> | (lit<"="> + ref<Expression>))))> {};

  struct ExplicitBackingFieldRule
      : rule<create<ast::ExplicitBackingField>(
            ref<FIELD> + lit<":"> +
            assign<&ast::ExplicitBackingField::type>(ref<TypeRule>) +
            lit<"="> +
            assign<&ast::ExplicitBackingField::initializer>(
                ref<Expression>))> {};

  struct PropertyDeclarationRule
      : rule<create<ast::PropertyDecl>(
            contextual_modifiers<&ast::PropertyDecl::modifiers,
                                 &ast::PropertyDecl::annotations,
                                 &ast::PropertyDecl::contextParameters> +
            (flag<&ast::PropertyDecl::isVar>(ref<VAR>) | ref<VAL>)+option(
                type_parameters<&ast::PropertyDecl::typeParameters>) +
            option(assign<&ast::PropertyDecl::receiverType>(ref<ReceiverType>) +
                   lit<".">) +
            (lit<"("> +
                 append<&ast::PropertyDecl::declarators>(
                     ref<VariableDeclaratorRule>) +
                 many(lit<","> + append<&ast::PropertyDecl::declarators>(
                                     ref<VariableDeclaratorRule>)) +
                 lit<")"> |
             append<&ast::PropertyDecl::declarators>(
                 ref<VariableDeclaratorRule>)) +
            NLs +
            option(type_constraints<&ast::PropertyDecl::typeConstraints> +
                   NLs) +
            option((flag<&ast::PropertyDecl::isDelegate>(ref<BY>) |
                    lit<"=">)+assign<&ast::PropertyDecl::
                                         initializer>(ref<Expression>) +
                   NLs) +
            option((assign<&ast::PropertyDecl::getter>(ref<GetterRule>) +
                    option(Semi + assign<&ast::PropertyDecl::setter>(
                                      ref<SetterRule>))) |
                   (assign<&ast::PropertyDecl::setter>(ref<SetterRule>) +
                    option(Semi + assign<&ast::PropertyDecl::getter>(
                                      ref<GetterRule>))) |
                   assign<&ast::PropertyDecl::backingField>(
                       ref<ExplicitBackingFieldRule>)))> {};

  struct TypeAliasRule
      : rule<create<ast::TypeAliasDecl>(
            modifiers<&ast::TypeAliasDecl::modifiers,
                      &ast::TypeAliasDecl::annotations> +
            lit<"typealias"> +
            assign<&ast::TypeAliasDecl::name>(ref<SimpleIdentifier>) +
            option(type_parameters<&ast::TypeAliasDecl::typeParameters>) +
            lit<"="> +
            assign<&ast::TypeAliasDecl::type>(ref<TypeRule>))> {};

  static constexpr auto TopLevelObject =
      ref<ClassDeclarationRule> | ref<ObjectDeclarationRule> |
      ref<FunctionDeclarationRule> | ref<PropertyDeclarationRule> |
      ref<TypeAliasRule>;

  // FILE
  // ====

  struct ImportHeaderRule
      : rule<create<ast::ImportHeader>(
            ref<IMPORT> + assign<&ast::ImportHeader::path>(ref<DottedName>) +
            option((lit<"."> +
                    flag<&ast::ImportHeader::wildcard>(lit<"*">)) |
                   (kw<"as"> + assign<&ast::ImportHeader::alias>(
                                   ref<SimpleIdentifier>))) +
            option(Semi))> {};

  struct KotlinFileRule
      : rule<create<ast::KotlinFile>(
            many(ref<NEWLINE>) +
            many(ref<FILE_SITE> + lit<":"> +
                 (lit<"["> +
                      some(append<&ast::KotlinFile::fileAnnotations>(
                          ref<UnescapedAnnotation>)) + lit<"]"> |
                  append<&ast::KotlinFile::fileAnnotations>(
                      ref<UnescapedAnnotation>)) +
                 option(Semi)) +
            option(many(append<&ast::KotlinFile::fileAnnotations>(ref<Annotation>) |
                        AllModifierKw) +
                   lit<"package"> +
                   assign<&ast::KotlinFile::packageName>(ref<DottedName>) +
                   option(Semi)) +
            many(append<&ast::KotlinFile::imports>(ref<ImportHeaderRule>)) +
            many(Anysemi) +
            option(some(append<&ast::KotlinFile::members>(
                            TopLevelObject | ref<Expression>),
                        some(Anysemi)) +
                   many(Anysemi)))> {};

  using entry = KotlinFileRule;
  using ignore = list<WS, NL_TRIVIA>;
  using hidden = list<ShebangLine, LineComment, DelimitedComment>;
  static constexpr auto indent_open = lit<"("> | lit<"[">;
  static constexpr auto indent_close = lit<")"> | lit<"]">;
  using indent_newline = NEWLINE;
};

}  // namespace kotlin
