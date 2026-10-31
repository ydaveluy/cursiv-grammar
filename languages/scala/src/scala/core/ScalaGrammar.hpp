// The scala language: its grammar (Scala 2.12). Semicolons and newlines are
// trivia, read back from the gap where they decide: between two statements
// and around a postfix operator. The postfix chain is one rule, left-recursive.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "scala/core/ast.hpp"

namespace scala {

struct Scala : cursiv::GrammarSpec {
  // ==== Trivia =============================================================

  struct WS : terminal<some(cr<R"( \t\f\r\n)">)> {};
  struct SEMI : terminal<lit<";">> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<ML_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};

  // ==== Identifiers =========================================================

  static constexpr auto Symbol = !cr<R"(\x00-\x7f)"> + (uc::Sm | uc::So);
  static constexpr auto OpCharacter = cr<R"(!#%&*+:<=>?@^|~\\/-)"> | Symbol;
  struct OperatorChar : terminal<!(lit<"//"> | lit<"/*">)+OpCharacter> {};
  static constexpr auto Opchar = ref<OperatorChar>;
  static constexpr auto Op = some(Opchar);
  static constexpr auto Letter = uc::L | uc::Nl | cr<"$_">;
  static constexpr auto IdContinue =
      Letter | uc::Nd | uc::Mn | uc::Mc | uc::Pc | uc::Cf |
      cr<R"(\u0000-\u0008\u000E-\u0019\u001B\u007F-\u009F)">;
  static constexpr auto DecDigit = cr<"0-9">;

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  // All other Unicode escapes have already been substituted. Retain only
  // the UTF16 units which cannot be represented as a Unicode scalar.
  static constexpr auto SurrogateEscape =
      lit<"\\u"> + many(lit<"u">) + cr<"dD"> + cr<"8-9a-fA-F"> + repeat<2>(HexDigit);
  static constexpr auto CharEscapeSeq =
      lit<"\\"> + (lit<"b"> | lit<"t"> | lit<"n"> | lit<"f"> | lit<"r"> |
                   lit<"\""> | lit<"'"> | lit<"\\">);
  static constexpr auto OctalEscape =
      lit<"\\"> + ((cr<"0-3"> + repeat<0, 2>(cr<"0-7">)) |
                   (cr<"4-7"> + option(cr<"0-7">)));
  static constexpr auto Escape = CharEscapeSeq | SurrogateEscape | OctalEscape;
  static constexpr auto CharNoBackQuote =
      ~cr<R"(`\\)">;
  static constexpr auto Idrest =
      many(IdContinue) + option(lit<"_"> + Op);
  static constexpr auto Plainid = (Letter + Idrest) | Op;
  static constexpr auto VarId =
      !(lit<"_"> + !IdContinue) + Letter + Idrest;
  static constexpr auto word_boundary = IdContinue;
  static constexpr auto ReservedOp =
      (lit<"="> | lit<"=>"> | lit<":"> | lit<"<-"> | lit<"<:"> | lit<"<%"> |
       lit<">:"> | lit<"#"> | lit<"@">)+!Opchar;
  static constexpr auto Arrow = lit<"=>", !Opchar> | lit<"⇒">;
  static constexpr auto LeftArrow = lit<"<-", !Opchar> | lit<"←">;
  static constexpr auto OpName = !(lit<"⇒"> | lit<"←">) + !ReservedOp + Op;
  static constexpr auto ReservedWord =
      kw<"abstract"> | kw<"case"> | kw<"catch"> | kw<"class"> | kw<"def"> |
      kw<"do"> | kw<"else"> | kw<"extends"> | kw<"false"> | kw<"final"> |
      kw<"finally"> | kw<"for"> | kw<"forSome"> | kw<"if"> | kw<"implicit"> |
      kw<"import"> | kw<"lazy"> | kw<"match"> | kw<"new"> | kw<"null"> |
      kw<"object"> | kw<"override"> | kw<"package"> | kw<"private"> | kw<"protected"> |
      kw<"return"> | kw<"sealed"> | kw<"super"> | kw<"this"> | kw<"throw"> |
      kw<"trait"> | kw<"true"> | kw<"try"> | kw<"type"> | kw<"val"> |
      kw<"var"> | kw<"while"> | kw<"with"> | kw<"yield">;
  struct ID
      : terminal<(!ReservedWord + VarId) | (lit<"`"> + some(CharNoBackQuote | Escape) +
                          lit<"`">)> {};
  struct OP_ID : terminal<OpName> {};
  struct VARIABLE_ID
      : terminal<!ReservedWord + &(uc::Ll | cr<"_">) + VarId> {};
  static constexpr auto StarName = lit<"*", !Opchar> | lit<"`*`">;
  static constexpr auto Name = ref<ID> | ref<OP_ID>;

  // ==== Literals =============================================================

  static constexpr auto NonZeroDigit = cr<"1-9">;
  static constexpr auto DecimalNumeral =
      lit<"0", !DecDigit> | (NonZeroDigit + many(DecDigit));
  static constexpr auto HexNumeral = lit<"0"> + cr<"xX"> + some(HexDigit);
  struct INT_LIT
      : terminal<(DecimalNumeral | HexNumeral) + option(cr<"Ll">) + !IdContinue> {};

  static constexpr auto ExponentPart =
      cr<"Ee"> + option(cr<"+-">) + some(DecDigit);
  static constexpr auto FloatTypeSuffix = cr<"FfDd">;
  struct FLOAT_LIT
      : terminal<((some(DecDigit) + lit<"."> + some(DecDigit) +
                  option(ExponentPart) + option(FloatTypeSuffix))  //
                 | (lit<"."> + some(DecDigit) + option(ExponentPart) +
                    option(FloatTypeSuffix))                                  //
                 | (some(DecDigit) + ExponentPart + option(FloatTypeSuffix))  //
                 | (some(DecDigit) + option(ExponentPart) + FloatTypeSuffix)) + !IdContinue> {
  };

  struct CHAR_LIT
      // Scala2.12 still accepts the deprecated apostrophe spelling '''.
      : terminal<lit<"'"> + (Escape | lit<"'"> | ~cr<R"('\\)">) +
                 lit<"'">> {};

  // Translated CR/LF are legal inside these tokens. The validator checks
  // their original spelling to reject physical line ends locally.
  static constexpr auto StringElement = Escape | ~cr<R"("\\)">;
  struct MULTILINE_STRING_LIT
      : terminal<through(lit<"\"\"\"">, lit<"\"\"\"">)+many(lit<"\"">)> {};
  struct STRING_LIT : terminal<lit<"\""> + many(StringElement) + lit<"\"">> {};

  struct SYMBOL_LIT : terminal<lit<"'"> + Plainid> {};

  struct MINUS : terminal<cr<"-">> {};

  struct Literal
      : rule<create<ast::IntLit>(
                 option(flag<&ast::IntLit::negative>(ref<MINUS>)) +
                 assign<&ast::IntLit::text>(ref<INT_LIT>)) //
             | create<ast::FloatLit>(
                   option(flag<&ast::FloatLit::negative>(ref<MINUS>)) +
                   assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)) //
             |
             create<ast::BoolLit>(
                 (flag<&ast::BoolLit::value>(kw<"true">)) | kw<"false">) //
             |
             create<ast::CharLit>(assign<&ast::CharLit::text>(ref<CHAR_LIT>)) //
             |
             create<ast::StringLit>(
                 flag<&ast::StringLit::multiline>(
                     assign<&ast::StringLit::text>(ref<MULTILINE_STRING_LIT>)) |
                 assign<&ast::StringLit::text>(ref<STRING_LIT>)) //
             | create<ast::SymbolLit>(
                   assign<&ast::SymbolLit::text>(ref<SYMBOL_LIT>)) //
             | create<ast::NullLit>(kw<"null">)> {};

  static constexpr auto CloseParen = option(lit<",">) + lit<")">;
  static constexpr auto CloseBracket = option(lit<",">) + lit<"]">;
  static constexpr auto CloseBrace = option(lit<",">) + lit<"}">;

  // ==== Paths ================================================================

  struct QualId : rule<some(append<&ast::QualId::parts>(Name), lit<".">)> {
  };

  // `head ('.' Id)*`, where `head` is `this`, `super[Q].Id`, or `Id` alone /
  // followed by `.this` or `.super[Q].Id`: an iterative rewrite of the same
  // language -- at most one leading segment ever comes before `this`/`super`.
  struct StableId
      : rule<create<ast::StableId>(
            ((flag<&ast::StableId::hasThis>(kw<"this">))  //
             | (flag<&ast::StableId::hasSuper>(kw<"super">) +
                option(lit<"["> +
                       assign<&ast::StableId::superQualifier>(Name |
                                                               kw<"this">) +
                       CloseBracket) +
                lit<"."> + assign<&ast::StableId::root>(Name))  //
             | (assign<&ast::StableId::root>(Name) +
                option(
                    lit<"."> +
                    ((flag<&ast::StableId::hasThis>(kw<"this">))  //
                     | (flag<&ast::StableId::hasSuper>(kw<"super">) +
                        option(
                            lit<"["> +
                            assign<&ast::StableId::superQualifier>(
                                Name | kw<"this">) +
                            CloseBracket) +
                        lit<"."> +
                        assign<&ast::StableId::qualifierPrefix>(
                            Name)))))) +
            many(lit<"."> + append<&ast::StableId::segments>(Name)))> {};

  // Every grammar rule below may be mutually recursive with rules defined
  // later in the file; forward-declaring them all here (as the c/xsmp
  // examples do for their own recursive cores) lets each `ref<...>` resolve
  // regardless of definition order.
  struct Type_;
  struct InfixType;
  struct WithChain;
  struct AnnotType;
  struct CompoundType;
  struct RefineStat;
  struct ParamType;
  struct TypeParam;
  struct Param;
  struct ParamClause;
  struct ClassParam;
  struct ClassParamClause;
  struct Binding;
  struct Annotation;
  struct Expr_;
  struct Expr1;
  struct PostfixExpr;
  struct InfixExpr;
  struct PrefixExpr;
  struct SimpleExprChain;
  struct SimplePostfixChain;
  struct Block_;
  struct BlockContent_;
  struct BlockBody_;
  struct BlockLambda_;
  struct BlockExprAtom_;
  struct Guard_;
  struct Generator_;
  struct CaseClause_;
  struct Pattern_;
  struct Pattern1_;
  struct Pattern2_;
  struct Pattern3_;
  struct SimplePattern_;
  struct Dcl;
  struct ValDcl;
  struct VarDcl;
  struct DefDcl;
  struct TypeDcl;
  struct PatVarDef_;
  struct DefDef_;
  struct SecondaryCtorDef_;
  struct TypeDef_;
  struct TmplDef_;
  struct ClassDef_;
  struct TraitDef_;
  struct ObjectDef_;
  struct SelfType_;
  struct TemplateBody_;
  struct TemplateStat_;
  struct Import_;
  struct ImportExpr_;
  struct ImportSelector_;
  struct ClassTemplateOpt_;
  struct TraitTemplateOpt_;
  struct ClassTemplate_;
  struct TraitTemplate_;
  struct Constr_;
  struct EarlyDefs_;
  struct EarlyDef_;
  struct ConstrExpr_;
  struct ConstrBlock_;
  struct SelfInvocation_;
  struct TopStatSeq_;
  struct TopStat_;
  struct Packaging_;
  struct PackageObject_;
  struct CompilationUnit_;

  // A genuine `;`/newline in the trivia gap -- required between two
  // statements with no other separator, so an expression that could still
  // extend (`a + b`) is never mistaken for one statement ending and a new
  // one starting on the same line.
  static constexpr auto Separator = gap::has(eol | lit<"\f"> | lit<";">);
  // A `;` or a blank line: no operator reaches past it.
  static constexpr auto StatementBreak =
      gap::has(lit<";"> | (eol + many(cr<R"( \t\r)">) + eol));

  // The modifier set here is widened to the general one (see `defPrefix`,
  // defined with the definitions below) since every definition rule parses
  // its own prefix; a block accepting `private`/`override` locally is
  // over-acceptance a later pass would reject, not a parse-shape difference.
  static constexpr auto BlockDefOnce = append<&ast::Block::stats>(
      ref<Import_>                                                //
      | ref<PatVarDef_> | ref<DefDef_> | ref<SecondaryCtorDef_>  //
      | ref<TypeDef_>                                             //
      | ref<ClassDef_> | ref<TraitDef_> | ref<ObjectDef_>);
  static constexpr auto BlockExprOnce = append<&ast::Block::stats>(
      create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expr1>)));
  static constexpr auto BlockStatOnce = BlockDefOnce | BlockExprOnce;

  // A block ends on a function literal whose body is the rest of the block
  // (SLS 6.23 `ResultExpr`).
  static constexpr auto LambdaResult =
      Separator + assign<&ast::Block::result>(ref<BlockLambda_>);

  // Braces are the caller's: `caseClause`'s block has none of its own.
  struct BlockContent_
      : rule<create<ast::Block>(
            (BlockStatOnce + many(Separator + BlockStatOnce) +
             option(LambdaResult)) |
            assign<&ast::Block::result>(ref<BlockLambda_>))> {};

  static constexpr auto BareBlock = ref<BlockContent_>;

  inline static constexpr auto drop_dollar = [](std::string_view text) {
    return text.substr(1);
  };
  inline static constexpr auto drop_quote = [](std::string_view text) {
    return text.substr(0, text.size() - 1);
  };
  inline static constexpr auto drop_closer = [](std::string_view text) {
    return text.substr(0, text.size() - 3);
  };
  struct StringOpen : terminal<as<drop_quote>(ref<ID> + lit<"\"">)> {};
  struct MultiStringOpen : terminal<as<drop_closer>(ref<ID> + lit<"\"\"\"">)> {
  };
  static constexpr auto InterpolationIdentPart =
      uc::L | uc::Nl | uc::Nd | uc::Mn | uc::Mc | uc::Pc | uc::Cf | cr<"_">;
  struct StringRef
      : terminal<as<drop_dollar>(lit<"$"> + (uc::L | uc::Nl | cr<"_">)+many(
                                                InterpolationIdentPart))> {};
  struct StringThis : terminal<lit<"$this", !InterpolationIdentPart>> {};
  struct StringText : terminal<some(lit<"$$"> | cr<R"(^"$)">)> {};
  using MultiStringText = StringText;
  struct StringDollar : terminal<lit<"$">> {};
  struct MultiStringQuotes
      : terminal<lit<"\""> + option(lit<"\"">) + !lit<"\"">> {};
  struct MultiStringClose
      : terminal<as<drop_closer>(repeat<3, cursiv::kUnbounded>(lit<"\"">))> {};

  struct StringShortHole
      : rule<create<ast::InterpolationPart>(
            assign<&ast::InterpolationPart::expression>(create<ast::PathExpr>(
                assign<&ast::PathExpr::path>(create<ast::StableId>(
                    flag<&ast::StableId::hasThis>(ref<StringThis>) |
                    assign<&ast::StableId::root>(ref<StringRef>))))))> {};
  struct PatternShortHole
      : rule<create<ast::InterpolationPart>(
            assign<&ast::InterpolationPart::pattern>(create<ast::StablePattern>(
                assign<&ast::StablePattern::path>(create<ast::StableId>(
                    assign<&ast::StableId::root>(ref<StringRef>))))))> {};
  struct StringLongHole
      : rule<create<ast::InterpolationPart>(
            skip(ignore<WS, SEMI>, hide<SL_COMMENT, ML_COMMENT>,
            lit<"${"> +
            assign<&ast::InterpolationPart::expression>(
                ref<BlockBody_> | create<ast::Block>(&CloseBrace)) +
            CloseBrace))> {};
  struct PatternLongHole
      : rule<create<ast::InterpolationPart>(
            skip(ignore<WS, SEMI>, hide<SL_COMMENT, ML_COMMENT>,
            lit<"${"> +
            assign<&ast::InterpolationPart::pattern>(ref<Pattern_>) +
            CloseBrace))> {};

  template <auto PrefixPtr, auto MultilinePtr, auto PartsPtr, class ShortHole,
            class LongHole>
  static constexpr auto interpolation =
      tight(((flag<MultilinePtr>(assign<PrefixPtr>(ref<MultiStringOpen>)) +
        many(append<PartsPtr>(
            create<ast::InterpolationPart>(
                assign<&ast::InterpolationPart::text>(ref<MultiStringText> |
                                                      ref<MultiStringQuotes> |
                                                      ref<StringDollar>)) |
            ref<ShortHole> | ref<LongHole>)) +
        append<PartsPtr>(create<ast::InterpolationPart>(
            assign<&ast::InterpolationPart::text>(ref<MultiStringClose>)))) |
       (assign<PrefixPtr>(ref<StringOpen>) +
        many(append<PartsPtr>(create<ast::InterpolationPart>(
                                  assign<&ast::InterpolationPart::text>(
                                      ref<StringText> | ref<StringDollar>)) |
                              ref<ShortHole> | ref<LongHole>)) +
        lit<"\"">)));

  struct InterpolatedString
      : rule<create<ast::InterpolatedString>(
            interpolation<&ast::InterpolatedString::prefix,
                          &ast::InterpolatedString::multiline,
                          &ast::InterpolatedString::parts, StringShortHole,
                          StringLongHole>)> {};
  struct InterpolatedPattern
      : rule<create<ast::InterpolatedPattern>(
            interpolation<&ast::InterpolatedPattern::prefix,
                          &ast::InterpolatedPattern::multiline,
                          &ast::InterpolatedPattern::parts, PatternShortHole,
                          PatternLongHole>)> {};

  struct NamedType_
      : rule<create<ast::NamedType>(
            assign<&ast::NamedType::path>(ref<StableId>) +
            option(lit<"."> +
                   flag<&ast::NamedType::isSingleton>(kw<"type">)))> {};

  // `stableId ('.' 'type')?` then the postfix `[args]` / `#Id` chain; `_`
  // with its bounds is a wildcard argument.
  struct SimpleType
      : rule<create<ast::TupleType>(
                 lit<"("> +
                 many(append<&ast::TupleType::items>(ref<Type_>),
                             lit<",">) +
                 CloseParen)  //
             | create<ast::WildcardType>(
                   lit<"_", !Opchar> +
                   option(lit<">:"> +
                          assign<&ast::WildcardType::lowerBound>(ref<Type_>)) +
                   option(lit<"<:"> + assign<&ast::WildcardType::upperBound>(
                                          ref<Type_>)))  //
             | (ref<NamedType_> +
                many(nest<ast::AppliedType, &ast::AppliedType::base>(
                         lit<"["> +
                         some(append<&ast::AppliedType::args>(ref<Type_>),
                              lit<",">) +
                         CloseBracket)  //
                     | nest<ast::TypeProjection, &ast::TypeProjection::base>(
                           lit<"#"> +
                           assign<&ast::TypeProjection::member>(Name))))> {
  };

  struct AnnotType
      : rule<ref<SimpleType> +
             option(nest<ast::AnnotatedType, &ast::AnnotatedType::base>(
                 some(append<&ast::AnnotatedType::annotations>(
                     ref<Annotation>))))> {};

  // `annotType ('with' annotType)*`, a shared shape with `infixType`'s
  // `Id`-operator chain -- both a `BinaryType`, `op` empty for `with`.
  struct WithChain : infix<AnnotType, ast::BinaryType,
                           left(kw<"with">)> {};


  static constexpr auto RefinementTail =
      lit<"{"> + many(append<&ast::RefinedType::members>(ref<RefineStat>)) +
      CloseBrace;

  // `annotType ('with' annotType)* refinement?` or a bare `refinement`.
  struct CompoundType
      : rule<(ref<WithChain> +
              option(nest<ast::RefinedType, &ast::RefinedType::base>(
                  RefinementTail)))  //
             | create<ast::RefinedType>(RefinementTail)> {};

  struct InfixType : infix<CompoundType, ast::BinaryType,
                           left(Name)> {};

  static constexpr auto ExistentialDecls =
      kw<"forSome"> + lit<"{"> +
      some(append<&ast::ExistentialType::decls>(ref<TypeDcl> | ref<ValDcl>)) +
      CloseBrace;

  // `functionArgTypes '=>' type_ | infixType existentialClause?`. The
  // parenthesised `functionArgTypes` form is unified with a plain tuple type
  // (both `'(' T,... ')'`): the two only differ in whether `=>` follows the
  // `)`, which one token of lookahead resolves.
  struct Type_
      : rule<create<ast::FunctionType>(
                 lit<"("> +
                 many(append<&ast::FunctionType::params>(
                                  ref<ParamType>),
                             lit<",">) +
                 CloseParen + Arrow +
                 assign<&ast::FunctionType::result>(ref<Type_>))  //
             | create<ast::FunctionType>(
                   append<&ast::FunctionType::params>(
                       create<ast::ParamType>(
                           assign<&ast::ParamType::type>(ref<InfixType>))) +
                   Arrow +
                   assign<&ast::FunctionType::result>(ref<Type_>))  //
             | (ref<InfixType> +
                option(nest<ast::ExistentialType, &ast::ExistentialType::base>(
                    ExistentialDecls)))> {};

  // `type_ | '=>' type_ | type_ '*'`.
  struct ParamType
      : rule<create<ast::ParamType>(
            (flag<&ast::ParamType::byName>(Arrow) +
             assign<&ast::ParamType::type>(ref<Type_>))  //
            | (assign<&ast::ParamType::type>(ref<Type_>) +
               option(flag<&ast::ParamType::repeated>(
                   StarName))))> {};

  // ==== Type parameters ======================================================

  struct TypeParam
      : rule<create<ast::TypeParam>(
            many(append<&ast::TypeParam::annotations>(ref<Annotation>)) +
            option(assign<&ast::TypeParam::variance>(lit<"+", !Opchar> |
                                                      lit<"-", !Opchar>)) +
            (flag<&ast::TypeParam::isUnderscore>(lit<"_">) |
             assign<&ast::TypeParam::name>(Name)) +
            option(lit<"["> +
                   append<&ast::TypeParam::nestedParams>(ref<TypeParam>) +
                   many(lit<","> + append<&ast::TypeParam::nestedParams>(
                                       ref<TypeParam>)) +
                   CloseBracket) +
            option(lit<">:"> +
                   assign<&ast::TypeParam::lowerBound>(ref<Type_>)) +
            option(lit<"<:"> +
                   assign<&ast::TypeParam::upperBound>(ref<Type_>)) +
            many(lit<"<%"> + append<&ast::TypeParam::viewBounds>(ref<Type_>)) +
            many(lit<":", !Opchar> +
                 append<&ast::TypeParam::contextBounds>(ref<Type_>)))> {};

  template <auto Member>
  static constexpr auto typeParamClauseInto =
      lit<"["> + append<Member>(ref<TypeParam>) +
      many(lit<","> + append<Member>(ref<TypeParam>)) + CloseBracket;

  // ==== Params, bindings =====================================================

  struct Param
      : rule<create<ast::Param>(
            many(append<&ast::Param::annotations>(ref<Annotation>)) +
            assign<&ast::Param::name>(Name) +
            option(lit<":"> + assign<&ast::Param::type>(ref<ParamType>)) +
            option(lit<"="> +
                   assign<&ast::Param::defaultValue>(ref<Expr_>)))> {};

  struct ParamClause
      : rule<create<ast::ParamClause>(
            lit<"("> +
            option(append<&ast::ParamClause::params>(ref<Param>) +
                   many(lit<","> +
                        append<&ast::ParamClause::params>(ref<Param>))) +
            CloseParen)> {};

  template <auto ClausesMember, auto ImplicitMember>
  static constexpr auto paramClausesInto =
      many(append<ClausesMember>(ref<ParamClause>)) +
      option(lit<"("> + kw<"implicit"> + append<ImplicitMember>(ref<Param>) +
             many(lit<","> + append<ImplicitMember>(ref<Param>)) +
             CloseParen);

  struct ClassParam
      : rule<create<ast::ClassParam>(
            many(append<&ast::ClassParam::annotations>(ref<Annotation>)) +
            many(append<&ast::ClassParam::modifiers>(
                     kw<"override"> | kw<"abstract"> | kw<"final"> |
                     kw<"sealed"> | kw<"implicit"> | kw<"lazy">)  //
                 | (append<&ast::ClassParam::modifiers>(kw<"private"> |
                                                         kw<"protected">) +
                    option(lit<"["> +
                           append<&ast::ClassParam::modifiers>(Name |
                                                                kw<"this">) +
                           CloseBracket))) +
            option((flag<&ast::ClassParam::isVal>(kw<"val">)) |
                   (flag<&ast::ClassParam::isVar>(kw<"var">))) +
            assign<&ast::ClassParam::name>(Name) + lit<":"> +
            assign<&ast::ClassParam::type>(ref<ParamType>) +
            option(lit<"="> +
                   assign<&ast::ClassParam::defaultValue>(ref<Expr_>)))> {};

  struct ClassParamClause
      : rule<create<ast::ClassParamClause>(
            lit<"("> +
            option(
                append<&ast::ClassParamClause::params>(ref<ClassParam>) +
                many(lit<","> + append<&ast::ClassParamClause::params>(
                                     ref<ClassParam>))) +
            CloseParen)> {};

  template <auto ClausesMember, auto ImplicitMember>
  static constexpr auto classParamClausesInto =
      many(append<ClausesMember>(ref<ClassParamClause>)) +
      option(lit<"("> + kw<"implicit"> +
             append<ImplicitMember>(ref<ClassParam>) +
             many(lit<","> + append<ImplicitMember>(ref<ClassParam>)) +
             CloseParen);

  struct Binding
      : rule<create<ast::Binding>(
            (flag<&ast::Binding::isUnderscore>(lit<"_">) |
             assign<&ast::Binding::name>(Name)) +
            option(lit<":"> + assign<&ast::Binding::type>(ref<Type_>)))> {};

  static constexpr auto Bindings =
      lit<"("> + option(append<&ast::Lambda::params>(ref<Binding>) +
                        many(lit<","> + append<&ast::Lambda::params>(ref<Binding>))) +
      CloseParen;

  // ==== Annotations ==========================================================

  static constexpr auto ArgListOnce = create<ast::ArgList>(
      lit<"("> +
      option(append<&ast::ArgList::args>(ref<Expr_>) +
             many(lit<","> + append<&ast::ArgList::args>(ref<Expr_>))) +
      CloseParen);

  struct Annotation
      : rule<create<ast::Annotation>(
            lit<"@"> + assign<&ast::Annotation::type>(ref<SimpleType>) +
            many(append<&ast::Annotation::argLists>(ArgListOnce)))> {};

  // ==== Expressions ==========================================================


  static constexpr auto CaseClausesBody =
      some(append<&ast::MatchBlock::cases>(ref<CaseClause_>));

  // `argumentExprs: '(' args ')' | blockExpr` -- folded onto a `CallExpr`
  // chain step. A block argument that is one expression or one function
  // literal is that argument, as scalac reads it.
  // A repeated argument is already an AscribeExpr inside Expr_, as in
  // scalac. A second suffix here would admit two ascriptions.
  template <auto ArgsMember>
  static constexpr auto callArgsInto =
      (lit<"("> +
       option(append<ArgsMember>(ref<Expr_>) +
              many(lit<","> + append<ArgsMember>(ref<Expr_>))) +
       CloseParen)  //
      | (lit<"{"> +
         append<ArgsMember>(create<ast::MatchBlock>(CaseClausesBody)) +
         CloseBrace)  //
      | (lit<"{"> + option(append<ArgsMember>(ref<BlockBody_>)) + CloseBrace);

  // The primary atoms of `simpleExpr`/`simpleExpr1`.
  struct SimpleExprChain
      : rule<create<ast::NewExpr>(
                 kw<"new"> +
                 ((assign<&ast::NewExpr::classTemplate>(ref<ClassTemplate_>)) //
                  |
                  (assign<&ast::NewExpr::templateBody>(ref<TemplateBody_>)))) //
             | ref<BlockExprAtom_>                                            //
             | ref<InterpolatedString> |
             ref<Literal> //
             // A `.` after a path is the path's own next segment.
             | create<ast::PathExpr>(
                   assign<&ast::PathExpr::path>(ref<StableId>) + !lit<".">) //
             | create<ast::UnderscoreExpr>(lit<"_">)                        //
             | create<ast::ParenExpr>(
                   lit<"("> +
                   option(append<&ast::ParenExpr::items>(ref<Expr_>) +
                          many(lit<","> +
                               append<&ast::ParenExpr::items>(ref<Expr_>))) +
                   CloseParen +
                   // A `)` followed by `=>` is a lambda header, never a tuple:
                   // unguarded, the two readings fork on every multi-argument
                   // lambda.
                   !Arrow)> {};

  struct SimplePostfixChain
      : rule<ref<SimpleExprChain> +
             many(nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"."> + assign<&ast::MemberExpr::field>(Name))  //
                  | nest<ast::TypeApplyExpr, &ast::TypeApplyExpr::callee>(
                        lit<"["> +
                        append<&ast::TypeApplyExpr::typeArgs>(ref<Type_>) +
                        many(lit<","> + append<&ast::TypeApplyExpr::typeArgs>(
                                            ref<Type_>)) +
                        CloseBracket)  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        callArgsInto<&ast::CallExpr::args>)) +
             option(nest<ast::EtaExpr, &ast::EtaExpr::operand>(lit<"_">))> {};

  struct PrefixSign : terminal<cr<"+~!-"> + !Opchar> {};
  struct PrefixExpr
      : rule<create<ast::PrefixOpExpr>(
                 assign<&ast::PrefixOpExpr::op>(ref<PrefixSign>) +
                 assign<&ast::PrefixOpExpr::operand>(ref<SimplePostfixChain>))  //
             | ref<SimplePostfixChain>> {};

  // Scala's ten infix-operator classes, tightest first: every other
  // operator character, then */%, +-, :, <>, =!, &, ^, |, and a letter-led
  // name last (loosest). The last character decides associativity, so each
  // symbolic class splits into a left step and a `:`-final right step beside
  // it; the language refuses a chain that mixes the two. No class reads a
  // reserved `=`, `=>`, `:`, `<-`, `<:`, `<%`, `>:`, `#` or `@`.
  static constexpr auto OpcharNoColon = !lit<":"> + Opchar;
  static constexpr auto LeftTail = option(many(Opchar) + OpcharNoColon);
  static constexpr auto RightTail = many(Opchar) + lit<":">;
  struct OpLetter : rule<!Separator + ref<ID> + !StatementBreak> {};
  struct OpPipe : terminal<&OpName + cr<"|"> + LeftTail> {};
  struct OpPipeR : terminal<&OpName + cr<"|"> + RightTail> {};
  struct OpCaret : terminal<&OpName + cr<"^"> + LeftTail> {};
  struct OpCaretR : terminal<&OpName + cr<"^"> + RightTail> {};
  struct OpAmp : terminal<&OpName + cr<"&"> + LeftTail> {};
  struct OpAmpR : terminal<&OpName + cr<"&"> + RightTail> {};
  struct OpEqBang : terminal<&OpName + cr<"=!"> + LeftTail> {};
  struct OpEqBangR : terminal<&OpName + cr<"=!"> + RightTail> {};
  struct OpAngle : terminal<&OpName + cr<"<>"> + LeftTail> {};
  struct OpAngleR : terminal<&OpName + cr<"<>"> + RightTail> {};
  struct OpColon : terminal<&OpName + lit<":"> + LeftTail> {};
  struct OpColonR : terminal<&OpName + lit<":"> + RightTail> {};
  struct OpAddSub : terminal<&OpName + cr<"+-"> + LeftTail> {};
  struct OpAddSubR : terminal<&OpName + cr<"+-"> + RightTail> {};
  struct OpMulDiv : terminal<&OpName + cr<"*%/"> + LeftTail> {};
  struct OpMulDivR : terminal<&OpName + cr<"*%/"> + RightTail> {};
  struct OpOther : terminal<&OpName + (cr<R"(#?@~\\)"> | Symbol) + LeftTail> {};
  struct OpOtherR : terminal<&OpName + (cr<R"(#?@~\\)"> | Symbol) + RightTail> {
  };

  struct InfixExpr
      : infix<PrefixExpr, ast::InfixExpr,
             levels(left(ref<OpOther>), right(ref<OpOtherR>),
                    left(ref<OpMulDiv>), right(ref<OpMulDivR>),
                    left(ref<OpAddSub>), right(ref<OpAddSubR>),
                    left(ref<OpColon>), right(ref<OpColonR>),
                    left(ref<OpAngle>), right(ref<OpAngleR>),
                    left(ref<OpEqBang>), right(ref<OpEqBangR>),
                    left(ref<OpAmp>), right(ref<OpAmpR>),
                    left(ref<OpCaret>), right(ref<OpCaretR>),
                    left(ref<OpPipe>), right(ref<OpPipeR>),
                    left(ref<OpLetter>))> {};

  // A postfix id follows its operand with no separator between them and
  // comes before a token no operand opens on: an operator ending a line takes
  // the next line as its right operand unless a `;` or a blank line comes
  // between.
  static constexpr auto PostfixBoundary =
      StatementBreak | eof | &CloseBrace | &CloseParen | &lit<","> |
      &Arrow | &lit<"=", !Opchar> | &lit<":", !Opchar> |
      &LeftArrow | &lit<"@", !Opchar> |
      &(kw<"val"> | kw<"var"> | kw<"def"> | kw<"type"> | kw<"class"> |
        kw<"object"> | kw<"trait"> | kw<"case"> | kw<"import"> |
        kw<"package"> | kw<"else"> | kw<"match"> | kw<"catch"> |
        kw<"finally"> | kw<"yield"> | kw<"lazy"> | kw<"implicit"> |
        kw<"override"> | kw<"private"> | kw<"protected"> | kw<"final"> |
        kw<"sealed"> | kw<"abstract">);
  // A sign right after an expression is its infix operator: the sign that
  // opens a next statement stands behind a separator.
  static constexpr auto SignBoundary = Separator | !ref<PrefixSign>;
  struct PostfixExpr
      : rule<ref<InfixExpr> +
             option(nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                 !Separator + assign<&ast::PostfixExpr::postfixOp>(Name) +
                 PostfixBoundary)) +
             SignBoundary> {};

  static constexpr auto AscriptionSuffix =
      lit<":", !Opchar> +
      ((!lit<"_"> +
        assign<&ast::AscribeExpr::type>(ref<InfixType>))  //
       | (some(append<&ast::AscribeExpr::annotations>(ref<Annotation>)))  //
       | (flag<&ast::AscribeExpr::varargsSplice>(lit<"_"> + StarName)));

  static constexpr auto EnumeratorsBody =
      some(append<&ast::ForExpr::enumerators>(ref<Generator_>));

  // `postfixExpr (ascription? | 'match' '{' caseClauses '}' | '=' expr)`.
  struct Expr1
      : rule<create<ast::IfExpr>(
                 kw<"if"> + lit<"("> +
                 assign<&ast::IfExpr::cond>(ref<Expr_>) + CloseParen +
                 assign<&ast::IfExpr::thenBranch>(ref<Expr_>) +
                 option(commit(kw<"else">, assign<&ast::IfExpr::elseBranch>(ref<Expr_>))))  //
             | create<ast::WhileExpr>(
                   kw<"while"> + lit<"("> +
                   assign<&ast::WhileExpr::cond>(ref<Expr_>) + CloseParen +
                   assign<&ast::WhileExpr::body>(ref<Expr_>))  //
             | create<ast::TryExpr>(
                   kw<"try"> + assign<&ast::TryExpr::body>(ref<Expr_>) +
                   option(kw<"catch"> +
                          assign<&ast::TryExpr::catchHandler>(ref<Expr_>)) +
                   option(kw<"finally"> +
                          assign<&ast::TryExpr::finallyBlock>(ref<Expr_>)))  //
             | create<ast::DoWhileExpr>(
                   kw<"do"> + assign<&ast::DoWhileExpr::body>(ref<Expr_>) +
                   kw<"while"> + lit<"("> +
                   assign<&ast::DoWhileExpr::cond>(ref<Expr_>) + CloseParen)  //
             | create<ast::ForExpr>(
                   kw<"for"> +
                   ((lit<"("> + EnumeratorsBody + CloseParen) |
                    (lit<"{"> + EnumeratorsBody + CloseBrace)) +
                   option(flag<&ast::ForExpr::yields>(kw<"yield">)) +
                   assign<&ast::ForExpr::body>(ref<Expr_>))  //
             | create<ast::ThrowExpr>(kw<"throw"> +
                                      assign<&ast::ThrowExpr::value>(
                                          ref<Expr_>))  //
             // `return` may end a statement: a line break after it is one.
             | create<ast::ReturnExpr>(
                   kw<"return"> +
                   option(!gap::line +
                          assign<&ast::ReturnExpr::value>(ref<Expr_>)))  //
             | (ref<PostfixExpr> +
                option(nest<ast::AssignExpr, &ast::AssignExpr::target>(
                           lit<"=", !Opchar> +
                           assign<&ast::AssignExpr::value>(ref<Expr_>))  //
                       | nest<ast::MatchExpr, &ast::MatchExpr::value>(
                             kw<"match"> + lit<"{"> +
                             some(append<&ast::MatchExpr::cases>(
                                 ref<CaseClause_>)) +
                             CloseBrace)  //
                       | nest<ast::AscribeExpr, &ast::AscribeExpr::value>(
                             AscriptionSuffix)))> {};

  // `(bindings | implicit? Id | '_') '=>' expr | expr1`.
  struct Expr_
      : rule<create<ast::Lambda>(Bindings + Arrow +
                                 assign<&ast::Lambda::body>(ref<Expr_>))  //
             | create<ast::Lambda>(
                   append<&ast::Lambda::params>(create<ast::Binding>(
                       option(flag<&ast::Binding::isImplicit>(
                           kw<"implicit">)) +
                       assign<&ast::Binding::name>(Name))) +
                   Arrow +
                   assign<&ast::Lambda::body>(ref<Expr_>))  //
             | create<ast::Lambda>(
                   append<&ast::Lambda::params>(create<ast::Binding>(
                       flag<&ast::Binding::isUnderscore>(lit<"_">))) +
                   Arrow +
                   assign<&ast::Lambda::body>(ref<Expr_>))  //
             | ref<Expr1>> {};

  // ==== for-comprehensions, case clauses ====================================

  struct Guard_
      : rule<create<ast::Guard>(kw<"if"> +
                                assign<&ast::Guard::cond>(ref<PostfixExpr>))> {
  };

  struct Generator_
      : rule<create<ast::Generator>(
            assign<&ast::Generator::pattern>(ref<Pattern1_>) +
            LeftArrow + assign<&ast::Generator::source>(ref<Expr_>) +
            many(append<&ast::Generator::refinements>(ref<Guard_>) |
                 append<&ast::Generator::refinements>(create<ast::ValueDef>(
                     assign<&ast::ValueDef::pattern>(ref<Pattern1_>) +
                     lit<"=", !Opchar> +
                     assign<&ast::ValueDef::value>(ref<Expr_>)))))> {};

  struct CaseClause_
      : rule<create<ast::CaseClause>(
            kw<"case"> + assign<&ast::CaseClause::pattern>(ref<Pattern_>) +
            option(kw<"if"> +
                   assign<&ast::CaseClause::guard>(ref<PostfixExpr>)) +
            Arrow +
            assign<&ast::CaseClause::body>(BareBlock))> {};

  // ==== Blocks, block-or-match atom =========================================

  struct Block_ : rule<lit<"{"> + BareBlock + CloseBrace> {};

  // `(bindings | implicit? (Id | _) (':' compoundType)?) '=>' block`.
  struct BlockLambda_
      : rule<create<ast::Lambda>(
            (Bindings  //
             | append<&ast::Lambda::params>(create<ast::Binding>(
                   ((option(flag<&ast::Binding::isImplicit>(
                         kw<"implicit">)) +
                     assign<&ast::Binding::name>(Name))  //
                    | flag<&ast::Binding::isUnderscore>(lit<"_">)) +
                   option(lit<":", !Opchar> +
                          assign<&ast::Binding::type>(ref<CompoundType>))))) +
            Arrow +
            assign<&ast::Lambda::body>(ref<BlockBody_>))> {};

  // The content of braces that may stand for what they hold: one expression
  // or one function literal is that value, more statements are a block.
  struct BlockBody_
      : rule<ref<Expr1> | ref<BlockLambda_> |
             create<ast::Block>(
                 (((BlockDefOnce + many(Separator + BlockStatOnce)) |
                   (BlockExprOnce + some(Separator + BlockStatOnce))) +
                  option(LambdaResult)) |
                 (BlockExprOnce + LambdaResult))> {};

  struct BlockExprAtom_
      : rule<create<ast::MatchBlock>(lit<"{"> + CaseClausesBody + CloseBrace)  //
             | ref<Block_>> {};

  // ==== Patterns =============================================================

  struct SimplePattern_
      : rule<
            ref<InterpolatedPattern> | create<ast::WildcardPattern>(lit<"_">) //
            | create<ast::LiteralPattern>(
                  assign<&ast::LiteralPattern::value>(ref<Literal>)) //
            | create<ast::TuplePattern>(
                  lit<"("> +
                  option(append<&ast::TuplePattern::items>(ref<Pattern_>) +
                         many(lit<","> + append<&ast::TuplePattern::items>(
                                             ref<Pattern_>))) +
                  CloseParen) //
            |
            create<ast::ExtractorPattern>(
                assign<&ast::ExtractorPattern::path>(ref<StableId>) + lit<"("> +
                option(
                    (append<&ast::ExtractorPattern::args>(ref<Pattern_>) +
                     many(lit<","> +
                          append<&ast::ExtractorPattern::args>(ref<Pattern_>)) +
                     option(
                         lit<","> +
                         option(assign<&ast::ExtractorPattern::varargsBindName>(
                                    Name) +
                                lit<"@", !Opchar>) +
                         flag<&ast::ExtractorPattern::varargsSplice>(
                             lit<"_">) +
                         StarName)) //
                    | (option(assign<&ast::ExtractorPattern::varargsBindName>(
                                  Name) +
                              lit<"@", !Opchar>) +
                       flag<&ast::ExtractorPattern::varargsSplice>(
                           lit<"_">) +
                       StarName)) +
                CloseParen) //
            | create<ast::VarPattern>(
                  assign<&ast::VarPattern::name>(ref<VARIABLE_ID>) + !lit<".">)
            | create<ast::StablePattern>(
                  assign<&ast::StablePattern::path>(ref<StableId>))> {};

  struct Pattern3_
      : infix<SimplePattern_, ast::InfixPattern,
              levels(left(ref<OpOther>), right(ref<OpOtherR>),
                     left(ref<OpMulDiv>), right(ref<OpMulDivR>),
                     left(ref<OpAddSub>), right(ref<OpAddSubR>),
                     left(ref<OpColon>), right(ref<OpColonR>),
                     left(ref<OpAngle>), right(ref<OpAngleR>),
                     left(ref<OpEqBang>), right(ref<OpEqBangR>),
                     left(ref<OpAmp>), right(ref<OpAmpR>), left(ref<OpCaret>),
                     right(ref<OpCaretR>), left(ref<OpPipe>),
                     right(ref<OpPipeR>), left(ref<OpLetter>))> {};

  struct Pattern2_
      : rule<(lit<"_"> + lit<"@", !Opchar> + ref<Pattern3_>) |
             create<ast::BindPattern>(
                 assign<&ast::BindPattern::name>(Name) +
                 lit<"@", !Opchar> +
                 assign<&ast::BindPattern::inner>(ref<Pattern3_>))  //
             | ref<Pattern3_>> {};

  struct Pattern1_
      : rule<create<ast::TypedPattern>(
                 ((flag<&ast::TypedPattern::isUnderscore>(lit<"_">))  //
                  | assign<&ast::TypedPattern::name>(Name)) +
                 lit<":", !Opchar> +
                 assign<&ast::TypedPattern::type>(ref<Type_>))  //
             | ref<Pattern2_>> {};

  struct Pattern_
      : rule<ref<Pattern1_> +
             option(nest<ast::AltPattern, &ast::AltPattern::first>(
                 some(lit<"|"> +
                      append<&ast::AltPattern::rest>(ref<Pattern1_>))))> {};

  // ==== Declarations =========================================================

  // The `(annotation NL?)*`/`modifier*` prefix every definition and
  // declaration below parses for itself; `private`/`protected`'s bracket
  // takes an `Id` or `this`, appended as one more `modifiers` entry.
  template <auto AnnMember, auto ModMember>
  static constexpr auto defPrefix =
      many(append<AnnMember>(ref<Annotation>)) +
      many(append<ModMember>(kw<"abstract"> | kw<"final"> | kw<"sealed"> |
                              kw<"implicit"> | kw<"lazy"> | kw<"override">)  //
           | (append<ModMember>(kw<"private"> | kw<"protected">) +
              option(lit<"["> + append<ModMember>(Name | kw<"this">) +
                     CloseBracket)));

  struct ValDcl
      : rule<create<ast::ValDcl>(
            defPrefix<&ast::Dcl::annotations, &ast::Dcl::modifiers> +
            (kw<"val"> | flag<&ast::ValDcl::isVar>(kw<"var">)) +
            append<&ast::ValDcl::names>(Name) +
            many(lit<","> + append<&ast::ValDcl::names>(Name)) +
            lit<":"> + assign<&ast::ValDcl::type>(ref<Type_>))> {};

  struct DefDcl
      : rule<create<ast::DefDcl>(
            defPrefix<&ast::Dcl::annotations, &ast::Dcl::modifiers> +
            kw<"def"> + assign<&ast::DefDcl::name>(Name) +
            option(typeParamClauseInto<&ast::DefDcl::typeParams>) +
            paramClausesInto<&ast::DefDcl::paramClauses,
                              &ast::DefDcl::implicitParams> +
            option(lit<":"> +
                   assign<&ast::DefDcl::returnType>(ref<Type_>)))> {};

  struct TypeDcl
      : rule<create<ast::TypeDcl>(
            defPrefix<&ast::Dcl::annotations, &ast::Dcl::modifiers> +
            kw<"type"> + assign<&ast::TypeDcl::name>(Name) +
            option(typeParamClauseInto<&ast::TypeDcl::typeParams>) +
            option(lit<">:"> +
                   assign<&ast::TypeDcl::lowerBound>(ref<Type_>)) +
            option(lit<"<:"> +
                   assign<&ast::TypeDcl::upperBound>(ref<Type_>)))> {};

  struct Dcl : rule<ref<ValDcl> | ref<DefDcl> | ref<TypeDcl>> {};

  // ==== Definitions ==========================================================

  static constexpr auto PatDefBody =
      append<&ast::PatDef::patterns>(ref<Pattern2_>) +
      many(lit<","> + append<&ast::PatDef::patterns>(ref<Pattern2_>)) +
      option(lit<":"> + assign<&ast::PatDef::type>(ref<Type_>)) +
      lit<"=", !Opchar> + assign<&ast::PatDef::value>(ref<Expr_>);

  struct PatVarDef_
      : rule<create<ast::PatDef>(
            defPrefix<&ast::PatDef::annotations, &ast::PatDef::modifiers> +
            ((kw<"val"> + PatDefBody)  //
             | (flag<&ast::PatDef::isVar>(kw<"var">) +
                (PatDefBody  //
                 | (append<&ast::PatDef::patterns>(create<ast::BindPattern>(
                        assign<&ast::BindPattern::name>(Name))) +
                    many(lit<","> +
                         append<&ast::PatDef::patterns>(
                             create<ast::BindPattern>(
                                 assign<&ast::BindPattern::name>(
                                     Name)))) +
                    lit<":"> + assign<&ast::PatDef::type>(ref<Type_>) +
                    lit<"=", !Opchar> +
                    flag<&ast::PatDef::uninitialized>(
                        lit<"_">))))))> {};

  struct DefDef_
      : rule<create<ast::DefDef>(
            defPrefix<&ast::DefDef::annotations, &ast::DefDef::modifiers> +
            kw<"def"> + assign<&ast::DefDef::name>(Name) +
            option(typeParamClauseInto<&ast::DefDef::typeParams>) +
            paramClausesInto<&ast::DefDef::paramClauses,
                              &ast::DefDef::implicitParams> +
            option(lit<":"> +
                   assign<&ast::DefDef::returnType>(ref<Type_>)) +
            ((lit<"=", !Opchar> +
              option(flag<&ast::DefDef::isMacro>(kw<"macro">)) +
              assign<&ast::DefDef::value>(ref<Expr_>))  //
             | assign<&ast::DefDef::body>(ref<Block_>)))> {};

  // `this(...)`, curried via the same nest chain the general postfix
  // expression uses for calls.
  static constexpr auto SelfInvocation =
      create<ast::PathExpr>(assign<&ast::PathExpr::path>(create<ast::StableId>(
          flag<&ast::StableId::hasThis>(kw<"this">)))) +
      some(nest<ast::CallExpr, &ast::CallExpr::callee>(
          callArgsInto<&ast::CallExpr::args>));

  struct SecondaryCtorDef_
      : rule<create<ast::SecondaryCtorDef>(
            defPrefix<&ast::SecondaryCtorDef::annotations,
                      &ast::SecondaryCtorDef::modifiers> +
            kw<"def"> + kw<"this"> + lit<"("> +
            option(append<&ast::SecondaryCtorDef::firstParams>(ref<Param>) +
                   many(lit<","> +
                        append<&ast::SecondaryCtorDef::firstParams>(
                            ref<Param>))) +
            CloseParen +
            paramClausesInto<&ast::SecondaryCtorDef::moreParamClauses,
                              &ast::SecondaryCtorDef::implicitParams> +
            option(lit<"=", !Opchar>) +
            ((assign<&ast::SecondaryCtorDef::selfInvocation>(
                 SelfInvocation))  //
             | (lit<"{"> +
                assign<&ast::SecondaryCtorDef::selfInvocation>(
                    SelfInvocation) +
                assign<&ast::SecondaryCtorDef::body>(
                    create<ast::Block>(many(Separator + BlockStatOnce))) +
                CloseBrace)))> {};

  struct TypeDef_
      : rule<create<ast::TypeDef>(
            defPrefix<&ast::TypeDef::annotations, &ast::TypeDef::modifiers> +
            kw<"type"> + assign<&ast::TypeDef::name>(Name) +
            option(typeParamClauseInto<&ast::TypeDef::typeParams>) +
            lit<"=", !Opchar> +
            assign<&ast::TypeDef::value>(ref<Type_>))> {};

  // ==== Templates ============================================================

  struct SelfType_
      : rule<create<ast::SelfType>(
            ((assign<&ast::SelfType::name>(Name) +
              option(lit<":"> +
                     assign<&ast::SelfType::type>(ref<Type_>)))  //
             | (flag<&ast::SelfType::isThis>(kw<"this">) + lit<":"> +
                assign<&ast::SelfType::type>(ref<Type_>))) +
            Arrow)> {};

  struct ImportSelector_
      : rule<create<ast::ImportSelector>(
            assign<&ast::ImportSelector::name>(Name) +
            option(Arrow +
                   (assign<&ast::ImportSelector::renameTo>(Name)  //
                    | flag<&ast::ImportSelector::renameToUnderscore>(
                          lit<"_">))))> {};

  struct ImportExpr_
      : rule<create<ast::ImportExpr>(
            assign<&ast::ImportExpr::path>(ref<StableId>) +
            option(lit<"."> +
                   ((flag<&ast::ImportExpr::wildcard>(lit<"_">))  //
                    | assign<&ast::ImportExpr::singleName>(Name)    //
                    | (lit<"{"> +
                       many(append<&ast::ImportExpr::selectors>(
                                ref<ImportSelector_>) +
                            lit<",">) +
                       (append<&ast::ImportExpr::selectors>(
                            ref<ImportSelector_>)  //
                        | flag<&ast::ImportExpr::wildcard>(
                              lit<"_">)) +
                       CloseBrace))))> {};

  struct Import_
      : rule<create<ast::Import>(
            kw<"import"> + append<&ast::Import::clauses>(ref<ImportExpr_>) +
            many(lit<","> +
                 append<&ast::Import::clauses>(ref<ImportExpr_>)))> {};

  template <auto Members>
  static constexpr auto TemplateStatsInto =
      option(append<Members>(ref<TemplateStat_>) +
             many(Separator + append<Members>(ref<TemplateStat_>)));

  struct TemplateBody_
      : rule<create<ast::TemplateBody>(
            lit<"{"> +
            option(assign<&ast::TemplateBody::self>(ref<SelfType_>)) +
            TemplateStatsInto<&ast::TemplateBody::members> + CloseBrace)> {};

  struct TemplateStat_
      : rule<ref<Import_>                    //
             | ref<PatVarDef_>                 //
             | ref<DefDef_>                     //
             | ref<SecondaryCtorDef_>          //
             | ref<TypeDef_>                    //
             | ref<ClassDef_>                   //
             | ref<TraitDef_>                   //
             | ref<ObjectDef_>                  //
             | ref<Dcl>                          //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expr_>))> {};

  struct RefineStat : rule<ref<Dcl> | ref<TypeDef_>> {};

  static constexpr auto EarlyDefsBody =
      lit<"{"> +
      some(append<&ast::ClassTemplate::earlyDefs>(ref<PatVarDef_>)) +
      CloseBrace + kw<"with">;

  struct ClassTemplate_
      : rule<create<ast::ClassTemplate>(
            option(EarlyDefsBody) +
            assign<&ast::ClassTemplate::firstParent>(ref<AnnotType>) +
            many(append<&ast::ClassTemplate::constrArgLists>(ArgListOnce)) +
            many(kw<"with"> +
                 append<&ast::ClassTemplate::withParents>(ref<AnnotType>)) +
            option(flag<&ast::ClassTemplate::hasBody>(lit<"{">) +
                   option(assign<&ast::ClassTemplate::self>(
                       ref<SelfType_>)) +
                   TemplateStatsInto<&ast::ClassTemplate::members> +
                   CloseBrace))> {};

  struct TraitTemplate_
      : rule<create<ast::ClassTemplate>(
            option(EarlyDefsBody) +
            assign<&ast::ClassTemplate::firstParent>(ref<AnnotType>) +
            many(kw<"with"> +
                 append<&ast::ClassTemplate::withParents>(ref<AnnotType>)) +
            option(flag<&ast::ClassTemplate::hasBody>(lit<"{">) +
                   option(assign<&ast::ClassTemplate::self>(
                       ref<SelfType_>)) +
                   TemplateStatsInto<&ast::ClassTemplate::members> +
                   CloseBrace))> {};

  template <auto Member>
  static constexpr auto classTemplateOptInto = option(
      (kw<"extends"> + assign<Member>(ref<ClassTemplate_>))  //
      | (option(kw<"extends">) +
         assign<Member>(create<ast::ClassTemplate>(
             flag<&ast::ClassTemplate::hasBody>(lit<"{">) +
             option(assign<&ast::ClassTemplate::self>(ref<SelfType_>)) +
             TemplateStatsInto<&ast::ClassTemplate::members> + CloseBrace))));

  template <auto Member>
  static constexpr auto traitTemplateOptInto = option(
      (kw<"extends"> + assign<Member>(ref<TraitTemplate_>))  //
      | (option(kw<"extends">) +
         assign<Member>(create<ast::ClassTemplate>(
             flag<&ast::ClassTemplate::hasBody>(lit<"{">) +
             option(assign<&ast::ClassTemplate::self>(ref<SelfType_>)) +
             TemplateStatsInto<&ast::ClassTemplate::members> + CloseBrace))));

  struct ClassDef_
      : rule<create<ast::ClassDef>(
            defPrefix<&ast::ClassDef::annotations,
                      &ast::ClassDef::modifiers> +
            option(flag<&ast::ClassDef::isCase>(kw<"case">)) +
            kw<"class"> + assign<&ast::ClassDef::name>(Name) +
            option(typeParamClauseInto<&ast::ClassDef::typeParams>) +
            many(append<&ast::ClassDef::ctorAnnotations>(ref<Annotation>)) +
            option(append<&ast::ClassDef::ctorModifiers>(kw<"private"> |
                                                          kw<"protected">) +
                   option(lit<"["> +
                          append<&ast::ClassDef::ctorModifiers>(Name |
                                                                 kw<"this">) +
                          CloseBracket)) +
            classParamClausesInto<&ast::ClassDef::paramClauses,
                                   &ast::ClassDef::implicitParams> +
            classTemplateOptInto<&ast::ClassDef::templateOpt>)> {};

  struct TraitDef_
      : rule<create<ast::TraitDef>(
            defPrefix<&ast::TraitDef::annotations,
                      &ast::TraitDef::modifiers> +
            kw<"trait"> + assign<&ast::TraitDef::name>(Name) +
            option(typeParamClauseInto<&ast::TraitDef::typeParams>) +
            traitTemplateOptInto<&ast::TraitDef::templateOpt>)> {};

  struct ObjectDef_
      : rule<create<ast::ObjectDef>(
            option(
                flag<&ast::ObjectDef::isPackageObject>(kw<"package">)) +
            defPrefix<&ast::ObjectDef::annotations,
                      &ast::ObjectDef::modifiers> +
            option(flag<&ast::ObjectDef::isCase>(kw<"case">)) +
            kw<"object"> + assign<&ast::ObjectDef::name>(Name) +
            classTemplateOptInto<&ast::ObjectDef::templateOpt>)> {};

  // ==== Top level ============================================================

  struct TopStat_
      : rule<ref<ClassDef_> | ref<TraitDef_> | ref<ObjectDef_>  //
             | ref<Import_>                                       //
             | ref<Packaging_>> {};

  struct Packaging_
      : rule<create<ast::Packaging>(
            kw<"package"> + assign<&ast::Packaging::name>(ref<QualId>) +
            lit<"{"> + many(append<&ast::Packaging::items>(ref<TopStat_>)) +
            CloseBrace)> {};

  struct CompilationUnit_
      : rule<create<ast::CompilationUnit>(
            many(kw<"package"> +
                 append<&ast::CompilationUnit::packageClauses>(ref<QualId>) +
                 !lit<"{">) +
            many(append<&ast::CompilationUnit::items>(ref<TopStat_>)))> {};

  using entry = CompilationUnit_;
  using ignored = list<WS, SEMI>;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace scala
