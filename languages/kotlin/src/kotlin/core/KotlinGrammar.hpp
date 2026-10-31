// The kotlin language: its grammar. Newlines: a bare newline is trivia
// (`NL_TRIVIA`, in `ignored`); `NEWLINE` is the same byte and is the token a
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

inline constexpr auto strip_return_at = [](std::string_view t) {
  return t.substr(7);
};
inline constexpr auto strip_continue_at = [](std::string_view t) {
  return t.substr(9);
};
inline constexpr auto strip_break_at = [](std::string_view t) {
  return t.substr(6);
};

inline constexpr auto elvis_text = [] { return std::string_view("?:"); };

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
  struct ElvisOp : rule<as<elvis_text>(NLs + lit<"?:">)> {};

  // NUMBERS
  // =======

  static constexpr auto Letter = uc::L | uc::Nl;
  // Simplified from the source's underscore-position tracking (an
  // underscore is accepted anywhere after the first digit, not only between
  // two digits) and its leading-zero exclusion: both are over-acceptance.
  // The first character must be a real digit -- `1._foo()` (a member access
  // starting with `_`) would otherwise lex as `1.` `_` `foo()`, a malformed
  // float literal `1._` swallowing the `_` a real identifier needed.
  static constexpr auto Digits = d + many(d | lit<"_">);
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
      : terminal<lit<"'"> + (UnicodeEscape | SimpleEscape | cr<R"(^'\\\r\n)">) +
                 lit<"'">> {};

  // NAMES
  // =====

  static constexpr auto IdentifierBody =
      ((Letter | lit<"_">) + many(Letter | lit<"_"> | uc::Nd)) |
      (lit<"`"> + some(cr<R"(^`\r\n)">) + lit<"`">);
  static constexpr auto HardKeywords = kw<"as"> | kw<"break"> | kw<"class"> | kw<"continue"> | kw<"do"> | kw<"else"> | kw<"false"> | kw<"for"> | kw<"fun"> | kw<"if"> | kw<"in"> | kw<"interface"> | kw<"is"> | kw<"null"> | kw<"object"> | kw<"package"> | kw<"return"> | kw<"super"> | kw<"this"> | kw<"throw"> | kw<"true"> | kw<"try"> | kw<"typealias"> | kw<"typeof"> | kw<"val"> | kw<"var"> | kw<"when"> | kw<"while">;
  struct Identifier : terminal<!HardKeywords + IdentifierBody> {};
  struct LabelReference : terminal<lit<"@"> + !HardKeywords + IdentifierBody> {};
  struct LabelDefinition : terminal<!HardKeywords + IdentifierBody + lit<"@">> {};
  struct FieldIdentifier : terminal<lit<"$"> + IdentifierBody> {};

  struct ReturnAtLabel
      : terminal<as<strip_return_at>(lit<"return@"> + !HardKeywords + IdentifierBody)> {};
  struct ContinueAtLabel
      : terminal<as<strip_continue_at>(lit<"continue@"> + !HardKeywords + IdentifierBody)> {};
  struct BreakAtLabel
      : terminal<as<strip_break_at>(lit<"break@"> + !HardKeywords + IdentifierBody)> {};

  static constexpr auto word_boundary = Letter | uc::Nd | cr<"_">;
  struct NOT_IS : terminal<lit<"!is", !word_boundary>> {};
  struct NOT_IN : terminal<lit<"!in", !word_boundary>> {};

  // KEYWORDS referenced from more than one place: a modifier or a use-site
  // target needs a stable name so `SimpleIdentifier` (or, for a target, its
  // group form) can offer the same spelling a modifier list reads.
  struct ALL_SITE : terminal<as<strip_first>(kw<"@all">)> {};
  struct FILE_SITE : terminal<as<strip_first>(kw<"@file">)> {};
  struct FIELD_SITE : terminal<as<strip_first>(kw<"@field">)> {};
  struct PROPERTY_SITE : terminal<as<strip_first>(kw<"@property">)> {};
  struct GET_SITE : terminal<as<strip_first>(kw<"@get">)> {};
  struct SET_SITE : terminal<as<strip_first>(kw<"@set">)> {};
  struct RECEIVER_SITE : terminal<as<strip_first>(kw<"@receiver">)> {};
  struct PARAM_SITE : terminal<as<strip_first>(kw<"@param">)> {};
  struct SETPARAM_SITE : terminal<as<strip_first>(kw<"@setparam">)> {};
  struct DELEGATE_SITE : terminal<as<strip_first>(kw<"@delegate">)> {};
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

  struct Type;
  struct TypeProjection;
  struct Annotation;
  struct Argument;
  struct Param;
  struct ContextParam;
  struct Expression;
  struct PrefixUnaryExpr;
  struct AtomicExpr;
  struct AtomicExprNoIdent;
  struct CallableReferenceExpr;
  struct FunctionLiteral;
  struct Block;
  struct ClassDeclaration;
  struct FunctionDeclaration;
  struct ObjectDeclaration;
  struct CompanionObject;
  struct PropertyDeclaration;
  struct TypeAlias;
  struct SecondaryConstructor;
  struct InitBlock;
  struct ClassMember;

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
      : rule<as<strip_first>(ref<LabelReference> +
                             many(lit<"."> + ref<SimpleIdentifier>))> {};

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
      (lit<"<"> + some(append<TypeArgsPtr>(ref<TypeProjection>), lit<",">) +
       option(lit<",">) + lit<">"> + option(value_arguments<ArgsPtr>) +
       many(append<LambdasPtr>(ref<FunctionLiteral>))) |
      (value_arguments<ArgsPtr> +
       many(append<LambdasPtr>(ref<FunctionLiteral>)));
  template <auto TypeArgsPtr, auto ArgsPtr, auto LambdasPtr>
  static constexpr auto call_suffix_body =
      call_arguments_body<TypeArgsPtr, ArgsPtr, LambdasPtr> |
      some(append<LambdasPtr>(ref<FunctionLiteral>));

  static constexpr auto AnnotationTypeArguments =
      lit<"<"> +
      some(append<&ast::Annotation::typeArguments>(ref<TypeProjection>),
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

  struct TypeProjection
      : rule<create<ast::TypeProjection>(
            flag<&ast::TypeProjection::isStar>(lit<"*">) |
            (option(assign<&ast::TypeProjection::variance>(ref<IN> |
                                                             ref<OUT>)) +
             assign<&ast::TypeProjection::type>(ref<Type>)))> {};

  struct TypeSegment
      : rule<create<ast::TypeSegment>(
            assign<&ast::TypeSegment::name>(ref<SimpleIdentifier>) +
            option(lit<"<"> +
                   many(append<&ast::TypeSegment::typeArguments>(
                            ref<TypeProjection>),
                        lit<",">) +
                   option(lit<",">) + lit<">">))> {};

  static constexpr auto user_type_body =
      some(append<&ast::UserType::segments>(ref<TypeSegment>), lit<".">);
  struct UserType : rule<create<ast::UserType>(user_type_body)> {};

  struct FunctionTypeParam
      : rule<create<ast::Param>(
            (assign<&ast::Param::name>(ref<SimpleIdentifier>) + lit<":"> +
             assign<&ast::Param::type>(ref<Type>)) |
            assign<&ast::Param::type>(ref<Type>))> {};

  struct FunctionTypeReceiver
      : rule<(ref<UserType> +
              option(nest<ast::NullableType, &ast::NullableType::base>(
                  some(lit<"?">)))) |
             (lit<"("> + ref<Type> + lit<")">)> {};

  static constexpr auto function_type_body =
      option(context_parameters<&ast::FunctionType::contextParameters>) +
      option(assign<&ast::FunctionType::receiver>(ref<FunctionTypeReceiver>) +
             lit<".">) +
      lit<"("> +
      many(append<&ast::FunctionType::parameters>(ref<FunctionTypeParam>),
           lit<",">) +
      option(lit<",">) + lit<")"> + NLs + lit<"->"> + NLs +
      assign<&ast::FunctionType::returnType>(ref<Type>);

  static constexpr auto type_modifier =
      annotation_modifier<&ast::Type::annotations> |
      flag<&ast::Type::isSuspend>(ref<SUSPEND>);
  static constexpr auto nullable_suffix = option(
      nest<ast::NullableType, &ast::NullableType::base>(some(lit<"?">)));

  struct Type
      : rule<((many(type_modifier) +
               (create<ast::FunctionType>(function_type_body) |
                ((create<ast::UserType>(user_type_body) |
                  create<ast::DynamicType>(kw<"dynamic">)) +
                 nullable_suffix))) |
              (lit<"("> + ref<Type> + lit<")"> + nullable_suffix) |
              (some(type_modifier) +
               create<ast::ParenthesizedType>(
                   lit<"("> +
                   assign<&ast::ParenthesizedType::type>(ref<Type>) +
                   lit<")">) +
               nullable_suffix)) +
             many(nest<ast::IntersectionType, &ast::IntersectionType::left>(
                      lit<"&"> +
                      assign<&ast::IntersectionType::right>(ref<Type>)))> {};

  // PARAMETERS / DECLARATORS
  // ========================

  struct Param
      : rule<create<ast::Param>(
            modifiers<&ast::Param::modifiers, &ast::Param::annotations> +
            assign<&ast::Param::name>(ref<SimpleIdentifier>) + lit<":"> +
            assign<&ast::Param::type>(ref<Type>) +
            option(lit<"="> +
                   assign<&ast::Param::defaultValue>(ref<Expression>)))> {};

  struct ContextParam
      : rule<ref<Param> |
             create<ast::Param>(assign<&ast::Param::type>(ref<Type>))> {};

  struct SetterParam
      : rule<create<ast::Param>(
            modifiers<&ast::Param::modifiers, &ast::Param::annotations> +
            assign<&ast::Param::name>(ref<SimpleIdentifier>) +
            option(lit<":"> +
                   assign<&ast::Param::type>(ref<Type>)))> {};

  struct ClassParam
      : rule<create<ast::ClassParam>(
            modifiers<&ast::ClassParam::modifiers,
                      &ast::ClassParam::annotations> +
            option(assign<&ast::ClassParam::valOrVar>(ref<VAL> | ref<VAR>)) +
            assign<&ast::ClassParam::name>(ref<SimpleIdentifier>) +
            lit<":"> + assign<&ast::ClassParam::type>(ref<Type>) +
            option(lit<"="> + assign<&ast::ClassParam::defaultValue>(
                                   ref<Expression>)))> {};

  static constexpr auto variable_declarator_body =
            assign<&ast::VariableDeclarator::name>(ref<SimpleIdentifier>) +
            option(lit<":"> +
                   assign<&ast::VariableDeclarator::type>(ref<Type>));
  struct VariableDeclarator : rule<create<ast::VariableDeclarator>(
      variable_declarator_body)> {};

  enum class DestructuringMode { short_form, full, values };
  template<DestructuringMode Mode> static constexpr auto destructuring_prefix = [] {
    if constexpr (Mode == DestructuringMode::short_form)
      return modifiers<&ast::VariableDeclarator::modifiers,
                       &ast::VariableDeclarator::annotations>;
    else if constexpr (Mode == DestructuringMode::full)
      return assign<&ast::VariableDeclarator::valOrVar>(ref<VAL> | ref<VAR>);
    else return assign<&ast::VariableDeclarator::valOrVar>(ref<VAL>);
  }();
  template<bool Named> static constexpr auto destructuring_entry_body = [] {
    if constexpr (Named) return variable_declarator_body + option(lit<"="> +
        assign<&ast::VariableDeclarator::sourceName>(ref<SimpleIdentifier>));
    else return variable_declarator_body;
  }();
  template<bool Named, DestructuringMode Mode> struct DestructuringEntry
      : rule<create<ast::VariableDeclarator>(destructuring_prefix<Mode> +
             destructuring_entry_body<Named>)> {};
  template<auto Entries, auto Kind, DestructuringMode Mode>
  static constexpr auto destructuring_list =
      (assign<Kind>(as<ast::DestructuringKind::named>(lit<"(">)) +
       some(append<Entries>(ref<DestructuringEntry<true, Mode>>), lit<",">) +
       option(lit<",">) + lit<")">) |
      (assign<Kind>(as<ast::DestructuringKind::positional>(lit<"[">)) +
       some(append<Entries>(ref<DestructuringEntry<false, Mode>>), lit<",">) +
       option(lit<",">) + lit<"]">);

  struct TypeParam
      : rule<create<ast::TypeParam>(
            modifiers<&ast::TypeParam::modifiers,
                      &ast::TypeParam::annotations> +
            (flag<&ast::TypeParam::isStar>(lit<"*">) |
             assign<&ast::TypeParam::name>(ref<SimpleIdentifier>)) +
            option(lit<":"> +
                   assign<&ast::TypeParam::bound>(ref<Type>)))> {};

  template <auto Ptr>
  static constexpr auto type_parameters =
      lit<"<"> + some(append<Ptr>(ref<TypeParam>), lit<",">) + option(lit<",">) +
      lit<">">;

  struct TypeConstraint
      : rule<create<ast::TypeConstraint>(
            many(append<&ast::TypeConstraint::annotations>(ref<Annotation>)) +
            assign<&ast::TypeConstraint::name>(ref<SimpleIdentifier>) +
            lit<":"> +
            assign<&ast::TypeConstraint::bound>(ref<Type>))> {};

  template <auto Ptr>
  static constexpr auto type_constraints =
      ref<WHERE> + some(append<Ptr>(ref<TypeConstraint>), lit<",">);

  struct LambdaParam
      : rule<create<ast::LambdaParam>(
            (append<&ast::LambdaParam::declarators>(
                 ref<VariableDeclarator>) |
             destructuring_list<&ast::LambdaParam::declarators,
                                &ast::LambdaParam::destructuring, DestructuringMode::short_form> |
             destructuring_list<&ast::LambdaParam::declarators,
                                &ast::LambdaParam::destructuring, DestructuringMode::values>) +
            option(lit<":"> +
                   assign<&ast::LambdaParam::type>(ref<Type>)))> {};

  // SUPERTYPES
  // ==========

  struct SuperTypeSpecifier
      : rule<create<ast::SuperTypeDelegate>(
                 many(append<&ast::SuperType::annotations>(ref<Annotation>)) +
                 assign<&ast::SuperTypeDelegate::type>(ref<UserType>) +
                 ref<BY> + assign<&ast::SuperTypeDelegate::delegate>(
                               ref<Expression>)) |
            create<ast::SuperTypeCall>(
                many(append<&ast::SuperType::annotations>(ref<Annotation>)) +
                assign<&ast::SuperTypeCall::type>(ref<UserType>) +
                call_suffix_body<&ast::SuperTypeCall::typeArguments,
                                 &ast::SuperTypeCall::arguments,
                                 &ast::SuperTypeCall::trailingLambdas>) |
            create<ast::SuperTypeEntry>(
                many(append<&ast::SuperType::annotations>(ref<Annotation>)) +
                assign<&ast::SuperTypeEntry::type>(ref<UserType>))> {};

  template <auto Ptr>
  static constexpr auto super_types =
      lit<":"> + some(append<Ptr>(ref<SuperTypeSpecifier>), lit<",">);

  // STRING LITERALS
  // ===============

  struct LineStrExprStart : terminal<lit<"${">> {};
  struct StrRef : terminal<as<strip_first>(lit<"$"> + IdentifierBody)> {};
  struct StrThis : terminal<kw<"$this">> {};
  struct StrDollar : terminal<lit<"$">> {};
  struct StrText : terminal<some(cr<R"(^\\"$\r\n)">)> {};
  struct LineStrEscape : terminal<UnicodeEscape | SimpleEscape> {};
  struct MultiLineStrText : terminal<some(cr<R"(^"$)">)> {};
  // A run of one or two quotes is content; three open the closer. Keep the
  // guarded quote run separate from the ordinary multiline text token.
  struct MultiLineQuotes
      : terminal<lit<"\""> + option(lit<"\"">) + !lit<"\"">> {};

  inline static constexpr auto drop_closing_quotes = [](std::string_view text) {
    return text.substr(0, text.size() - 3);
  };
  struct MultiLineClose : terminal<as<drop_closing_quotes>(
                              repeat<3, cursiv::kUnbounded>(lit<"\"">))> {};

  static constexpr auto StringReferencePart =
      append<&ast::StringLiteral::parts>(
          create<ast::StringPart>(assign<&ast::StringPart::expr>(
              create<ast::ThisExpr>(ref<StrThis>) |
              create<ast::Ident>(assign<&ast::Ident::name>(ref<StrRef>)))));

  static constexpr auto StringExprPart =
      append<&ast::StringLiteral::parts>(create<ast::StringPart>(
          skip(ignore<WS, NL_TRIVIA>, hide<DelimitedComment, LineComment>,
               ref<LineStrExprStart> +
               assign<&ast::StringPart::expr>(ref<Expression>) + lit<"}">)));

  struct StringLiteral
      : rule<create<ast::StringLiteral>(tight(
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
             lit<"\"">)))> {};

  // EXPRESSIONS
  // ===========

  struct IfExpr;
  struct WhenExpr;
  struct TryExpr;
  struct ObjectLiteral;
  struct CollectionLiteral;
  struct ForExpr;
  struct WhileExpr;
  struct DoWhileExpr;

  struct ThisExpr
      : rule<create<ast::ThisExpr>(
            kw<"this"> +
            option(assign<&ast::ThisExpr::label>(ref<LabelReference>)))> {};
  struct SuperExpr
      : rule<create<ast::SuperExpr>(
            kw<"super"> +
            option(lit<"<"> +
                   assign<&ast::SuperExpr::targetType>(ref<Type>) +
                   lit<">">) +
            option(assign<&ast::SuperExpr::label>(ref<LabelReference>)))> {};

  struct ThrowExpr
      : rule<create<ast::ThrowExpr>(
            kw<"throw"> +
            assign<&ast::ThrowExpr::value>(ref<Expression>))> {};
  struct ReturnExpr
      : rule<create<ast::ReturnExpr>(
            (kw<"return"> |
             assign<&ast::ReturnExpr::label>(ref<ReturnAtLabel>)) +
            option(assign<&ast::ReturnExpr::value>(ref<Expression>)))> {};
  struct BreakExpr
      : rule<create<ast::BreakExpr>(
            kw<"break"> |
            assign<&ast::BreakExpr::label>(ref<BreakAtLabel>))> {};
  struct ContinueExpr
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
      : rule<(lit<"("> + ref<Expression> + lit<")">) | ref<StringLiteral> |
             create<ast::Literal>(assign<&ast::Literal::text>(
                 ref<BooleanLiteral> | ref<CharacterLiteral> | ref<HexLiteral> |
                 ref<BinLiteral> | ref<DoubleLiteral> | ref<FloatLiteral> |
                 ref<IntegerLiteral> | ref<NullLiteral>)) |
             ref<FunctionLiteral> | ref<ThisExpr> | ref<SuperExpr> |
             ref<IfExpr> | ref<WhenExpr> | ref<TryExpr> |
             ref<ObjectLiteral> | ref<ThrowExpr> | ref<ReturnExpr> |
             ref<ContinueExpr> | ref<BreakExpr> | ref<ForExpr> |
             ref<WhileExpr> | ref<DoWhileExpr> |
             ref<CollectionLiteral> |
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
                        ref<UserType>) +
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
              ref<FunctionLiteral>)));

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
                       option(lit<",">) + lit<"]">) |
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
  // `Type`, not another expression, so it cannot share `BinaryExpr`'s
  // uniform left/right member pair the way every other level does.
  struct TypeOpChain
      : rule<ref<PrefixUnaryExpr> +
            many(nest<ast::CastExpr, &ast::CastExpr::operand>(
                NLs + assign<&ast::CastExpr::op>(ref<AS> | lit<"as?"> |
                                                  lit<":">) +
                assign<&ast::CastExpr::type>(ref<Type>)))> {};

  struct ElvisChain
      : infix<TypeOpChain, ast::BinaryExpr,
              levels(left(lit<"*"> | lit<"/"> | lit<"%">),
                     left(lit<"+"> | lit<"-">), left(lit<"..<"> | lit<"..">),
                     left(ref<SimpleIdentifier>), left(ref<ElvisOp>))> {};

  struct NamedInfixExpr
      : rule<ref<ElvisChain> +
            option(
                some(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                    assign<&ast::BinaryExpr::op>(ref<IN> | ref<NOT_IN>) +
                    assign<&ast::BinaryExpr::right>(ref<ElvisChain>))) |
                nest<ast::TypeTestExpr, &ast::TypeTestExpr::operand>(
                    (flag<&ast::TypeTestExpr::negated>(ref<NOT_IS>) |
                     ref<IS>) +
                    assign<&ast::TypeTestExpr::type>(ref<Type>)))> {};

  // comparison > equality
  struct ComparisonChain
      : infix<NamedInfixExpr, ast::BinaryExpr,
             levels(left(lit<"<="> | lit<">="> | lit<"<"> | lit<">">),
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
      : infix<DisjChain, ast::BinaryExpr,
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
                assign<&ast::TypeTestExpr::type>(ref<Type>))> {};

  struct WhenEntryRule
      : rule<create<ast::WhenEntry>(
            (some(append<&ast::WhenEntry::conditions>(ref<WhenConditionExpr>), lit<",">) +
             option(kw<"if"> +
                    assign<&ast::WhenEntry::guard>(ref<Expression>)) +
             lit<"->"> + assign<&ast::WhenEntry::body>(ControlBody) +
             option(Semi)) |
            (flag<&ast::WhenEntry::isElse>(kw<"else">) +
             option(kw<"if"> +
                    assign<&ast::WhenEntry::guard>(ref<Expression>)) +
             lit<"->"> + assign<&ast::WhenEntry::body>(ControlBody)))> {};

  struct WhenExpr
      : rule<create<ast::WhenExpr>(
            kw<"when"> +
            option(lit<"("> +
                   (assign<&ast::WhenExpr::subjectDeclaration>(
                        ref<PropertyDeclaration>) |
                    assign<&ast::WhenExpr::subject>(ref<Expression>)) +
                   lit<")">) +
            lit<"{"> +
            many(append<&ast::WhenExpr::entries>(ref<WhenEntryRule>)) +
            lit<"}">)> {};

  struct IfExpr
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<Expression>) +
            lit<")"> + option(assign<&ast::IfExpr::thenBranch>(ControlBody)) +
            option(lit<";">) +
            option(commit(kw<"else">, option(assign<&ast::IfExpr::elseBranch>(ControlBody)))))> {
  };

  struct CatchClause
      : rule<create<ast::CatchClause>(
            ref<CATCH> + lit<"("> +
            many(append<&ast::CatchClause::annotations>(ref<Annotation>)) +
            assign<&ast::CatchClause::name>(ref<SimpleIdentifier>) +
            lit<":"> + assign<&ast::CatchClause::type>(ref<Type>) +
            option(lit<",">) + lit<")"> + assign<&ast::CatchClause::body>(ref<Block>))> {};

  struct TryExpr
      : rule<create<ast::TryExpr>(
            kw<"try"> + assign<&ast::TryExpr::tryBlock>(ref<Block>) +
            ((some(append<&ast::TryExpr::catches>(ref<CatchClause>)) +
              option(ref<FINALLY> + assign<&ast::TryExpr::finallyBlock>(ref<Block>))) |
             (ref<FINALLY> + assign<&ast::TryExpr::finallyBlock>(ref<Block>))))> {};

  struct ObjectLiteral
      : rule<create<ast::ObjectLiteral>(
            kw<"object"> +
            option(super_types<&ast::ObjectLiteral::superTypes>) +
            option(lit<"{"> +
                   many(append<&ast::ObjectLiteral::members>(
                       ref<ClassMember>)) +
                   lit<"}">))> {};

  struct CollectionLiteral
      : rule<create<ast::CollectionLiteral>(
            lit<"["> +
            option(append<&ast::CollectionLiteral::items>(ref<Expression>) +
                   many(lit<","> + append<&ast::CollectionLiteral::items>(
                                        ref<Expression>)) + option(lit<",">)) +
            lit<"]">)> {};

  struct ForExpr
      : rule<create<ast::ForExpr>(
            kw<"for"> + lit<"("> +
            many(annotation_modifier<&ast::ForExpr::annotations>) +
            option(assign<&ast::ForExpr::valOrVar>(ref<VAL> | ref<VAR>)) +
            (append<&ast::ForExpr::declarators>(ref<VariableDeclarator>) |
             destructuring_list<&ast::ForExpr::declarators,
                                &ast::ForExpr::destructuring, DestructuringMode::short_form> |
             destructuring_list<&ast::ForExpr::declarators,
                                &ast::ForExpr::destructuring, DestructuringMode::values>) +
            ref<IN> + assign<&ast::ForExpr::iterable>(ref<Expression>) +
            lit<")"> + option(assign<&ast::ForExpr::body>(ControlBody)))> {};

  struct WhileExpr
      : rule<create<ast::WhileExpr>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileExpr::cond>(ref<Expression>) + lit<")"> +
            option(assign<&ast::WhileExpr::body>(ControlBody)))> {};

  struct DoWhileExpr
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
  static constexpr auto FullDestructuringHead =
      (lit<"("> | lit<"[">) + (ref<VAL> | ref<VAR>);
  static constexpr auto LocalDeclarationBody = ref<ClassDeclaration> | ref<FunctionDeclaration> |
      ref<PropertyDeclaration> | ref<TypeAlias>;
  struct LocalDeclaration : rule<
      create<ast::LabeledDeclaration>(some(append<&ast::LabeledDeclaration::labels>(ref<LabelDefinition>)) +
          assign<&ast::LabeledDeclaration::declaration>(LocalDeclarationBody)) | LocalDeclarationBody> {};

  static constexpr auto StatementItem =
      ref<LocalDeclaration> | ref<ObjectDeclaration> |
      ref<CompanionObject> | ref<SecondaryConstructor> |
      ref<InitBlock> | (!FullDestructuringHead + ref<Expression>);

  template <auto Ptr>
  static constexpr auto statements_into =
      many(Anysemi) +
      option(some(append<Ptr>(StatementItem), some(Anysemi)) + many(Anysemi));

  struct Block
      : rule<create<ast::Block>(lit<"{"> +
                                statements_into<&ast::Block::statements> +
                                lit<"}">)> {};

  struct FunctionLiteral
      : rule<create<ast::FunctionLiteral>(
            many(annotation_modifier<&ast::FunctionLiteral::annotations>) +
            lit<"{"> +
            option((some(append<&ast::FunctionLiteral::parameters>(ref<LambdaParam>), lit<",">) +
                    option(lit<",">) + lit<"->">) | lit<"->">) +
            statements_into<&ast::FunctionLiteral::body> + lit<"}">)> {};

  // DECLARATIONS
  // ============

  struct PrimaryConstructor
      : rule<create<ast::PrimaryConstructor>(
            modifiers<&ast::PrimaryConstructor::modifiers,
                      &ast::PrimaryConstructor::annotations> +
            option(ref<CONSTRUCTOR>) + lit<"("> +
            many(append<&ast::PrimaryConstructor::params>(
                     ref<ClassParam>),
                 lit<",">) +
            option(lit<",">) + lit<")">)> {};

  struct SecondaryConstructor
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

  struct EnumEntry
      : rule<create<ast::EnumEntry>(
            many(append<&ast::EnumEntry::annotations>(ref<Annotation>)) +
            assign<&ast::EnumEntry::name>(ref<SimpleIdentifier>) +
            option(value_arguments<&ast::EnumEntry::arguments>) +
            option(lit<"{"> +
                   many(append<&ast::EnumEntry::body>(ref<ClassMember>)) +
                   lit<"}">) +
            option(lit<",">))> {};

  struct InitBlock
      : rule<create<ast::InitBlock>(
            ref<INIT> + assign<&ast::InitBlock::body>(ref<Block>))> {};

  struct ClassMember
      : rule<(ref<ClassDeclaration> | ref<FunctionDeclaration> |
             ref<ObjectDeclaration> | ref<CompanionObject> |
             ref<PropertyDeclaration> | ref<InitBlock> |
             ref<SecondaryConstructor> | ref<TypeAlias>) +
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
                 ref<PrimaryConstructor>) +
             NLs) +
      option(super_types<&ast::ClassDecl::superTypes> + NLs) +
      option(type_constraints<&ast::ClassDecl::typeConstraints> + NLs);
  static constexpr auto class_members =
      many(append<&ast::ClassDecl::members>(ref<ClassMember>));

  // Entries exist only in an enum class, and its members only past a `;`.
  // The enum arm comes first: it names `enumEntries` before `members`, the
  // order the reflection visits them in.
  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
            class_modifiers +
            ((append<&ast::ClassDecl::modifiers>(ref<ENUM>) +
              class_modifiers + class_head +
              option(lit<"{"> +
                     many(append<&ast::ClassDecl::enumEntries>(
                         ref<EnumEntry>)) +
                     option(lit<";"> + class_members) + lit<"}">)) |
             (class_head +
              option(lit<"{"> + option(lit<";">) + class_members +
                     lit<"}">))))> {};

  struct ObjectDeclaration
      : rule<create<ast::ObjectDecl>(
            modifiers<&ast::ObjectDecl::modifiers,
                      &ast::ObjectDecl::annotations> +
            kw<"object"> + assign<&ast::ObjectDecl::name>(ref<SimpleIdentifier>) +
            NLs +
            option(assign<&ast::ObjectDecl::primaryCtor>(
                       ref<PrimaryConstructor>) +
                   NLs) +
            option(super_types<&ast::ObjectDecl::superTypes> + NLs) +
            option(lit<"{"> +
                   many(append<&ast::ObjectDecl::members>(
                       ref<ClassMember>)) +
                   lit<"}">))> {};

  struct CompanionObject
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
                       ref<ClassMember>)) +
                   lit<"}">))> {};

  // Its own rule so the extension-receiver's trailing `?` -- ordinarily
  // optional at the very end of a `type` -- has a reduction boundary before
  // the mandatory `.` that follows it here.
  struct ReceiverType
      : rule<ref<Type> +
            option(nest<ast::NullableType, &ast::NullableType::base>(
                some(lit<"?">)))> {};

  struct FunctionDeclaration
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
                   assign<&ast::FunctionDecl::returnType>(ref<Type>) +
                   NLs) +
            option(type_constraints<&ast::FunctionDecl::typeConstraints> +
                   NLs) +
            option(assign<&ast::FunctionDecl::body>(
                ref<Block> | (lit<"="> + ref<Expression>))))> {};

  struct Getter
      : rule<create<ast::Getter>(
            modifiers<&ast::Getter::modifiers, &ast::Getter::annotations> +
            ref<GETTER> +
            option(lit<"("> + lit<")"> +
                   option(lit<":"> +
                          assign<&ast::Getter::returnType>(ref<Type>)) +
                   assign<&ast::Getter::body>(
                       ref<Block> | (lit<"="> + ref<Expression>))))> {};

  struct Setter
      : rule<create<ast::Setter>(
            modifiers<&ast::Setter::modifiers, &ast::Setter::annotations> +
            ref<SETTER> +
            option(lit<"("> + assign<&ast::Setter::param>(ref<SetterParam>) +
                   option(lit<",">) + lit<")"> +
                   option(lit<":"> + assign<&ast::Setter::returnType>(ref<Type>)) +
                   assign<&ast::Setter::body>(
                       ref<Block> | (lit<"="> + ref<Expression>))))> {};

  struct ExplicitBackingField : rule<create<ast::ExplicitBackingField>(
      modifiers<&ast::ExplicitBackingField::modifiers, &ast::ExplicitBackingField::annotations> +
      ref<FIELD> + option(lit<":"> + assign<&ast::ExplicitBackingField::type>(ref<Type>)) +
      option(lit<"="> + assign<&ast::ExplicitBackingField::initializer>(ref<Expression>)))> {};

  struct PropertyDeclaration
      : rule<create<ast::PropertyDecl>(
            contextual_modifiers<&ast::PropertyDecl::modifiers,
                                 &ast::PropertyDecl::annotations,
                                 &ast::PropertyDecl::contextParameters> +
            (((flag<&ast::PropertyDecl::isVar>(ref<VAR>) | ref<VAL>)+option(
                type_parameters<&ast::PropertyDecl::typeParameters>) +
            option(assign<&ast::PropertyDecl::receiverType>(ref<ReceiverType>) +
                   lit<".">) +
            (destructuring_list<&ast::PropertyDecl::declarators,
                                &ast::PropertyDecl::destructuring, DestructuringMode::short_form> |
             append<&ast::PropertyDecl::declarators>(
                 ref<VariableDeclarator>))) |
             destructuring_list<&ast::PropertyDecl::declarators,
                                &ast::PropertyDecl::destructuring, DestructuringMode::full>) +
            NLs +
            option(type_constraints<&ast::PropertyDecl::typeConstraints> +
                   NLs) +
            option((flag<&ast::PropertyDecl::isDelegate>(ref<BY>) |
                    lit<"=">)+assign<&ast::PropertyDecl::
                                         initializer>(ref<Expression>) +
                   NLs) +
            unordered(option(assign<&ast::PropertyDecl::getter>(ref<Getter>) + option(Semi)),
                      option(assign<&ast::PropertyDecl::setter>(ref<Setter>) + option(Semi)),
                      option(assign<&ast::PropertyDecl::backingField>(ref<ExplicitBackingField>) + option(Semi))))> {};

  struct TypeAlias
      : rule<create<ast::TypeAliasDecl>(
            modifiers<&ast::TypeAliasDecl::modifiers,
                      &ast::TypeAliasDecl::annotations> +
            kw<"typealias"> +
            assign<&ast::TypeAliasDecl::name>(ref<SimpleIdentifier>) +
            option(type_parameters<&ast::TypeAliasDecl::typeParameters>) +
            lit<"="> +
            assign<&ast::TypeAliasDecl::type>(ref<Type>))> {};

  static constexpr auto TopLevelObject =
      ref<ClassDeclaration> | ref<ObjectDeclaration> |
      ref<FunctionDeclaration> | ref<PropertyDeclaration> |
      ref<TypeAlias>;

  // FILE
  // ====

  struct ImportHeader
      : rule<create<ast::ImportHeader>(
            ref<IMPORT> + assign<&ast::ImportHeader::path>(ref<DottedName>) +
            option((lit<"."> +
                    flag<&ast::ImportHeader::wildcard>(lit<"*">)) |
                   (kw<"as"> + assign<&ast::ImportHeader::alias>(
                                   ref<SimpleIdentifier>))) +
            option(Semi))> {};

  struct KotlinFile
      : rule<create<ast::KotlinFile>(
            many(ref<NEWLINE>) +
            many(ref<FILE_SITE> + lit<":"> +
                 ((lit<"["> +
                      some(append<&ast::KotlinFile::fileAnnotations>(
                          ref<UnescapedAnnotation>)) + lit<"]">) |
                  append<&ast::KotlinFile::fileAnnotations>(
                      ref<UnescapedAnnotation>)) +
                 option(Semi)) +
            option(many(append<&ast::KotlinFile::fileAnnotations>(ref<Annotation>) |
                        AllModifierKw) +
                   kw<"package"> +
                   assign<&ast::KotlinFile::packageName>(ref<DottedName>) +
                   option(Semi)) +
            many(append<&ast::KotlinFile::imports>(ref<ImportHeader>)) +
            many(Anysemi) +
            option(some(append<&ast::KotlinFile::members>(
                            TopLevelObject | (!FullDestructuringHead + ref<Expression>)),
                        some(Anysemi)) +
                   many(Anysemi)))> {};

  using entry = KotlinFile;
  using ignored = list<WS, NL_TRIVIA>;
  using hidden = list<ShebangLine, LineComment, DelimitedComment>;
  static constexpr auto indent_open = lit<"("> | lit<"[">;
  static constexpr auto indent_close = lit<")"> | lit<"]">;
  using indent_newline = NEWLINE;
};

}  // namespace kotlin
