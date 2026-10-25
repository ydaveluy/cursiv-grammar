// The csharp language: its grammar.
#pragma once

#include <string_view>

#include "csharp/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace csharp {

inline constexpr auto strip_at = [](std::string_view t) {
  return (!t.empty() && t.front() == '@') ? t.substr(1) : t;
};
inline constexpr auto drop1 = [](std::string_view t) { return t.substr(1); };
inline constexpr auto drop2 = [](std::string_view t) { return t.substr(2); };
inline constexpr auto drop3 = [](std::string_view t) { return t.substr(3); };

struct CSharp : cursiv::GrammarSpec {
  // -- Forward declarations (the expression/statement/type/pattern/member
  // families are mutually recursive) ----
  struct TypeNode;
  struct FunctionPointerParam_;
  struct Pattern_;
  struct Expression_;
  struct NonAssignmentExpr_;
  struct ConditionalExpr_;
  struct NullCoalesceExpr_;
  struct EqualityAndAbove_;
  struct IsAsExpr_;
  struct RelationalAndBelow_;
  struct AdditiveLevel_;
  struct CastExpr_;
  struct UnaryExpr_;
  struct PostfixChain_;
  struct Statement_;
  struct Block_;
  struct Member_;
  struct QueryBodyClause_;
  struct SelectOrGroupClause_;
  struct ArrayInitializerExpr_;
  struct ObjectInitializerExpr_;
  struct LocalVarDeclStmt_;

  // -- Trivia ----
  static constexpr auto LineEnd = cr<"\r\n\u0085\u2028\u2029">;
  struct WS
      : terminal<some(cr<" \t\v\f\r\n\u001a\u0085\u2028\u2029\ufeff"> | uc::Zs)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(~LineEnd)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  // Conditional-compilation directives are not evaluated: every directive
  // line is trivia, so both sides of `#if`/`#else` are read.
  struct DIRECTIVE : terminal<lit<"#"> + many(~LineEnd)> {};

  // -- Identifiers & keywords ---- A `\uXXXX`/`\UXXXXXXXX` escape reads as
  // more identifier characters, kept raw and never decoded, like every other
  // escape in this port.
  static constexpr auto IdEsc = (lit<"\\u"> + repeat<4>(cr<"0-9a-fA-F">)) |
                                 (lit<"\\U"> + repeat<8>(cr<"0-9a-fA-F">));
  // Identifier_Start/Part_Character (spec 6.4.3): letters (L, Nl), then also
  // digits (Nd), connectors (Pc) and marks (Mn, Mc, Cf).
  static constexpr auto word_boundary = uc::L | uc::Nl | uc::Mn | uc::Mc |
                                        uc::Nd | uc::Pc | uc::Cf | cr<R"(_\\)">;
  struct ID
      : terminal<(option(lit<"@">) + (uc::L | uc::Nl | cr<"_"> | IdEsc) +
                 many(uc::L | uc::Nl | uc::Mn | uc::Mc | uc::Nd | uc::Pc |
                      uc::Cf | cr<"_"> | IdEsc)),
                 text_conv<strip_at>> {};
  struct Underscore : terminal<kw<"_">> {};

  // A data rule (its own named rule, not spliced inline): `assign<>` needs a
  // single identifiable value, and only a rule's own body falls back to the
  // text it matched.
  struct RefKind_ : rule<kw<"ref"> + option(kw<"readonly">)> {};

  static constexpr auto ReservedWord =
      kw<"abstract"> | kw<"as"> | kw<"base"> | kw<"bool"> | kw<"break"> |
      kw<"byte"> | kw<"case"> | kw<"catch"> | kw<"char"> | kw<"checked"> |
      kw<"class"> | kw<"const"> | kw<"continue"> | kw<"decimal"> |
      kw<"default"> | kw<"delegate"> | kw<"do"> | kw<"double"> | kw<"else"> |
      kw<"enum"> | kw<"event"> | kw<"explicit"> | kw<"extern"> | kw<"false"> |
      kw<"finally"> | kw<"fixed"> | kw<"float"> | kw<"for"> | kw<"foreach"> |
      kw<"goto"> | kw<"if"> | kw<"implicit"> | kw<"in"> | kw<"int"> |
      kw<"interface"> | kw<"internal"> | kw<"is"> | kw<"lock"> | kw<"long"> |
      kw<"namespace"> | kw<"new"> | kw<"null"> | kw<"object"> |
      kw<"operator"> | kw<"out"> | kw<"override"> | kw<"params"> |
      kw<"private"> | kw<"protected"> | kw<"public"> | kw<"readonly"> |
      kw<"ref"> | kw<"return"> | kw<"sbyte"> | kw<"sealed"> | kw<"short"> |
      kw<"sizeof"> | kw<"stackalloc"> | kw<"static"> | kw<"string"> |
      kw<"struct"> | kw<"switch"> | kw<"this"> | kw<"throw"> | kw<"true"> |
      kw<"try"> | kw<"typeof"> | kw<"uint"> | kw<"ulong"> | kw<"unchecked"> |
      kw<"unsafe"> | kw<"ushort"> | kw<"using"> | kw<"virtual"> |
      kw<"void"> | kw<"volatile"> | kw<"while"> | kw<"__arglist"> |
      kw<"__makeref"> | kw<"__reftype"> | kw<"__refvalue">;

  // A contextual keyword is spelled with `kw<>` at its own syntax position,
  // and admitted here too: where a state reads both, the keyword takes the
  // spelling, so the name side must name it to stay a name.
  static constexpr auto ContextualWord =
      kw<"_"> | kw<"add"> | kw<"alias"> | kw<"ascending"> | kw<"async"> |
      kw<"await"> | kw<"by"> | kw<"descending"> | kw<"dynamic"> | kw<"equals"> |
      kw<"from"> | kw<"get"> | kw<"group"> | kw<"into"> | kw<"join"> |
      kw<"let"> | kw<"nameof"> | kw<"notnull"> | kw<"on"> | kw<"orderby"> |
      kw<"partial"> | kw<"remove"> | kw<"select"> | kw<"set"> |
      kw<"unmanaged"> | kw<"var"> | kw<"when"> | kw<"where"> | kw<"yield"> |
      kw<"record"> | kw<"init"> | kw<"required"> | kw<"file"> | kw<"global"> |
      kw<"with"> | kw<"and"> | kw<"or"> | kw<"not"> | kw<"scoped"> |
      kw<"managed">;

  struct Ident : rule<ref<ID> | ContextualWord> {};

  static constexpr auto PredefinedTypeKw =
      kw<"bool"> | kw<"byte"> | kw<"char"> | kw<"decimal"> | kw<"double"> |
      kw<"float"> | kw<"int"> | kw<"long"> | kw<"object"> | kw<"sbyte"> |
      kw<"short"> | kw<"string"> | kw<"uint"> | kw<"ulong"> | kw<"ushort">;

  // Every declaration kind's modifiers unioned into one set: a checker, not
  // the parser, knows which modifier belongs on which kind.
  static constexpr auto ModifierKw =
      kw<"new"> | kw<"public"> | kw<"protected"> | kw<"internal"> |
      kw<"private"> | kw<"abstract"> | kw<"sealed"> | kw<"static"> |
      kw<"unsafe"> | kw<"readonly"> | kw<"volatile"> | kw<"virtual"> |
      kw<"override"> | kw<"extern"> | kw<"partial"> | kw<"async"> |
      kw<"required"> | kw<"file">;

  // A generic's `>` never takes the first byte of `>=`: `a<b>=c` compares.
  static constexpr auto Lt = lit<"<">;
  static constexpr auto Gt = lit<">", !cr<"=">>;

  // -- Literals ----
  static constexpr auto SimpleEsc = lit<"\\"> + cr<R"('"\\0abfnrtv)">;
  static constexpr auto HexEsc = lit<"\\x"> + cr<"0-9a-fA-F"> +
                                  option(cr<"0-9a-fA-F">) +
                                  option(cr<"0-9a-fA-F">) +
                                  option(cr<"0-9a-fA-F">);
  static constexpr auto UniEsc = (lit<"\\u"> + repeat<4>(cr<"0-9a-fA-F">)) |
                                  (lit<"\\U"> + repeat<8>(cr<"0-9a-fA-F">));
  static constexpr auto AnyEsc = UniEsc | HexEsc | SimpleEsc;

  static constexpr auto DecRun = many(cr<"0-9_">);
  static constexpr auto Exponent =
      cr<"eE"> + option(cr<"+-">) + cr<"0-9"> + DecRun;
  static constexpr auto RealSuffix = cr<"FfDdMm">;
  static constexpr auto IntSuffix = lit<"UL"> | lit<"Ul"> | lit<"uL"> |
                                     lit<"ul"> | lit<"LU"> | lit<"Lu"> |
                                     lit<"lU"> | lit<"lu"> | lit<"U"> |
                                     lit<"u"> | lit<"L"> | lit<"l">;

  struct NUMBER
      : terminal<(cr<"0-9"> + DecRun + lit<"."> + cr<"0-9"> + DecRun +
                  option(Exponent) + option(RealSuffix)) |
                 (lit<"."> + cr<"0-9"> + DecRun + option(Exponent) +
                  option(RealSuffix)) |
                 (lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F_">) +
                  option(IntSuffix)) |
                 (lit<"0"> + cr<"bB"> + some(cr<"01_">) + option(IntSuffix)) |
                 (cr<"0-9"> + DecRun + Exponent + option(RealSuffix)) |
                 (cr<"0-9"> + DecRun + RealSuffix) |
                 (cr<"0-9"> + DecRun + option(IntSuffix))> {};

  struct CHAR : terminal<lit<"'"> +
                         (AnyEsc | cr<"^'\\\r\n\u0085\u2028\u2029">)+lit<"'">> {
  };
  struct STRING
      : terminal<lit<"\""> + many(AnyEsc | cr<"^\"\\\r\n\u0085\u2028\u2029">) +
                 lit<"\""> + option(lit<"u8">)> {};
  struct RawDelimiter {};
  struct RAW_STRING
      : terminal<through(mark<RawDelimiter>(repeat<3, cursiv::kUnbounded>(
                      lit<"\"">)), same<RawDelimiter>)+option(lit<"u8">)> {};
  struct VERBATIM_STRING
      : terminal<lit<"@\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">> {};

  // -- Interpolated strings: fused delimiters, like a template literal -- the
  // hole's expression is ordinary grammar, not lexer state.
  static constexpr auto RegularRun =
      many(lit<"{{"> | lit<"}}"> | AnyEsc | ~(LineEnd | cr<R"("\\{})">));
  static constexpr auto VerbatimRun =
      many(lit<"{{"> | lit<"}}"> | lit<"\"\""> | cr<R"(^"{})">);

  struct IStrOpen : terminal<lit<"$\""> + RegularRun, text_conv<drop2>> {};
  struct IStrClose : terminal<lit<"}"> + RegularRun, text_conv<drop1>> {};
  struct IStrFormat : terminal<lit<":"> + RegularRun, text_conv<drop1>> {};
  struct IVStrOpen
      : terminal<(lit<"$@\""> | lit<"@$\"">) + VerbatimRun,
                 text_conv<drop3>> {};
  struct IVStrClose : terminal<lit<"}"> + VerbatimRun, text_conv<drop1>> {};
  struct IVStrFormat : terminal<lit<":"> + VerbatimRun, text_conv<drop1>> {};

  struct RegularHole_
      : rule<create<ast::InterpolationHole>(
            assign<&ast::InterpolationHole::value>(ref<Expression_>) +
            option(lit<","> + assign<&ast::InterpolationHole::alignment>(
                                  ref<Expression_>)) +
            option(assign<&ast::InterpolationHole::format>(
                ref<IStrFormat>)))> {};

  struct VerbatimHole_
      : rule<create<ast::InterpolationHole>(
            assign<&ast::InterpolationHole::value>(ref<Expression_>) +
            option(lit<","> + assign<&ast::InterpolationHole::alignment>(
                                  ref<Expression_>)) +
            option(assign<&ast::InterpolationHole::format>(
                ref<IVStrFormat>)))> {};

  struct InterpolatedRegularStringExpr
      : rule<create<ast::InterpolatedStringExpr>(
            append<&ast::InterpolatedStringExpr::texts>(ref<IStrOpen>) +
            many(lit<"{"> +
                 append<&ast::InterpolatedStringExpr::holes>(
                     ref<RegularHole_>) +
                 append<&ast::InterpolatedStringExpr::texts>(
                     ref<IStrClose>)) +
            lit<"\"">)> {};

  struct InterpolatedVerbatimStringExpr
      : rule<create<ast::InterpolatedStringExpr>(
            flag<&ast::InterpolatedStringExpr::verbatim>(
                append<&ast::InterpolatedStringExpr::texts>(ref<IVStrOpen>)) +
            many(lit<"{"> +
                 append<&ast::InterpolatedStringExpr::holes>(
                     ref<VerbatimHole_>) +
                 append<&ast::InterpolatedStringExpr::texts>(
                     ref<IVStrClose>)) +
            lit<"\"">)> {};

  struct InterpolatedStringExpr_
      : rule<ref<InterpolatedRegularStringExpr> |
             ref<InterpolatedVerbatimStringExpr>> {};

  // -- Types ----
  struct RankSpecifier_ : rule<lit<"["> + many(lit<",">) + lit<"]">> {};

  // One generic argument, named so a generic type's own argument list does
  // not reference its own rule directly.
  struct TypeArg_
      : rule<create<ast::TypeArg>(
            assign<&ast::TypeArg::typeRef>(ref<TypeNode>))> {};

  struct TypeSegment_
      : rule<assign<&ast::TypeSegment::name>(ref<Ident> | PredefinedTypeKw |
                                             kw<"dynamic"> | kw<"var"> |
                                             kw<"void">) +
      option(Lt +
                    many(append<&ast::TypeSegment::arguments>(ref<TypeArg_>),
                  lit<",">) +
             Gt)> {};

  static constexpr auto DottedPath =
      some(append<&ast::Type::segments>(ref<TypeSegment_>),
                       lit<".">);

  struct TupleTypeElement_
      : rule<create<ast::TupleTypeElement>(
            assign<&ast::TupleTypeElement::typeRef>(ref<TypeNode>) +
            option(assign<&ast::TupleTypeElement::name>(ref<Ident>)))> {};

  static constexpr auto TupleTypeBody =
      lit<"("> +
      append<&ast::Type::tupleElements>(ref<TupleTypeElement_>) +
      some(lit<","> +
           append<&ast::Type::tupleElements>(ref<TupleTypeElement_>)) +
      lit<")">;

  struct TypeNode
      : rule<create<ast::Type>(
            option(assign<&ast::Type::alias>(ref<Ident>) + lit<"::">) +
            (DottedPath | TupleTypeBody |
             (flag<&ast::Type::functionPointer>(kw<"delegate">) +
              lit<"*"> +
              option(assign<&ast::Type::callingConvention>(kw<"managed"> |
                                                           kw<"unmanaged">) +
                     option(lit<"["> +
                            some(append<&ast::Type::callingConventions>(
                                     ref<Ident>),
                                 lit<",">) +
                            lit<"]">)) +
              Lt +
              some(append<&ast::Type::functionParameters>(
                       ref<FunctionPointerParam_>),
                   lit<",">) +
              Gt)) +
            many(append<&ast::Type::suffixes>(lit<"?"> | lit<"*"> |
                                              ref<RankSpecifier_>)))> {};

  // -- Shared pieces: arguments, parameters, attributes, type params ----
  struct Argument_
      : rule<create<ast::Argument>(
            option(assign<&ast::Argument::name>(ref<Ident>) + lit<":">) +
            ((assign<&ast::Argument::mode>(kw<"in"> | kw<"ref"> |
                                            kw<"out">) +
              assign<&ast::Argument::value>(ref<Expression_>)) |
             assign<&ast::Argument::value>(ref<Expression_>)))> {};

  static constexpr auto AttributeBody =
      assign<&ast::Attribute::name>(ref<TypeNode>) +
      option(lit<"("> +
             option(some(append<&ast::Attribute::args>(ref<Argument_>),
                         lit<",">)) +
             lit<")">);

  struct Attribute_ : rule<create<ast::Attribute>(AttributeBody)> {};

  // Only a global section names these targets, and it must name one.
  static constexpr auto GlobalTarget = kw<"assembly"> | kw<"module"> |
                                       kw<"@assembly"> | kw<"@module">;

  struct FirstAttribute_
      : rule<create<ast::Attribute>(
            option(!GlobalTarget +
                   assign<&ast::Attribute::target>(ref<Ident> |
                                                     ReservedWord) +
                   lit<":">) +
            AttributeBody)> {};

  struct GlobalAttribute_
      : rule<create<ast::Attribute>(
            &GlobalTarget + assign<&ast::Attribute::target>(ref<Ident>) +
            lit<":"> + AttributeBody)> {};

  template <auto AttributesPtr>
  static constexpr auto attributes =
      many(lit<"["> + append<AttributesPtr>(ref<FirstAttribute_>) +
           many(lit<","> + append<AttributesPtr>(ref<Attribute_>)) +
           option(lit<",">) + lit<"]">);

  static constexpr auto ParamModifierKw = kw<"ref"> | kw<"out"> | kw<"in"> |
                                          kw<"this"> | kw<"params"> |
                                          kw<"readonly"> | kw<"scoped">;

  struct Param_
      : rule<create<ast::Param>(
            flag<&ast::Param::isArglist>(kw<"__arglist">)) |
             create<ast::Param>(
                 many(lit<"["> +
                      append<&ast::Param::attributes>(ref<FirstAttribute_>) +
                      many(lit<","> +
                           append<&ast::Param::attributes>(ref<Attribute_>)) +
                      option(lit<",">) + lit<"]">) +
                 many(append<&ast::Param::modifiers>(ParamModifierKw)) +
                 assign<&ast::Param::typeRef>(ref<TypeNode>) +
                 assign<&ast::Param::name>(ref<Ident>) +
                 option(lit<"="> +
                        assign<&ast::Param::defaultValue>(
                            ref<Expression_>)))> {};

  struct FunctionPointerParam_
      : rule<create<ast::Param>(
            many(append<&ast::Param::modifiers>(ParamModifierKw)) +
            assign<&ast::Param::typeRef>(ref<TypeNode>))> {};

  struct TypeParam_
      : rule<create<ast::TypeParam>(
            attributes<&ast::TypeParam::attributes> +
            option(assign<&ast::TypeParam::variance>(kw<"in"> | kw<"out">)) +
            assign<&ast::TypeParam::name>(ref<Ident>))> {};

  struct ClassConstraint_ : rule<kw<"class"> + option(lit<"?">)> {};

  static constexpr auto ConstraintItem =
      append<&ast::TypeParamConstraintClause::keywordConstraints>(
          ref<ClassConstraint_> | kw<"struct"> | kw<"notnull"> |
          kw<"unmanaged">) |
      flag<&ast::TypeParamConstraintClause::hasNew>(kw<"new"> +
                                                          lit<"("> +
                                                          lit<")">) |
      append<&ast::TypeParamConstraintClause::typeConstraints>(
          ref<TypeNode>);

  struct TypeParamConstraintClause_
      : rule<create<ast::TypeParamConstraintClause>(
            kw<"where"> +
            assign<&ast::TypeParamConstraintClause::name>(ref<Ident>) +
            lit<":"> + some(ConstraintItem, lit<",">))> {};

  struct Declarator_
      : rule<create<ast::Declarator>(
            assign<&ast::Declarator::name>(ref<Ident>) +
            option(lit<"="> +
                   option(flag<&ast::Declarator::isRef>(kw<"ref">)) +
                   assign<&ast::Declarator::value>(
                       ref<Expression_> | ref<ArrayInitializerExpr_>)))> {};

  // -- `{ }`-initializers ----
  struct ArrayInitializerExpr_
      : rule<create<ast::ArrayInitializerExpr>(
            lit<"{"> +
            option(some(append<&ast::ArrayInitializerExpr::items>(
                            ref<Expression_> | ref<ArrayInitializerExpr_>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct ObjectInitMember_
      : rule<create<ast::ObjectInitMember>(
            (assign<&ast::ObjectInitMember::name>(ref<Ident>) |
             (lit<"["> +
              some(append<&ast::ObjectInitMember::indexArgs>(
                       ref<Argument_>),
                   lit<",">) +
              lit<"]">)) +
            lit<"="> +
            assign<&ast::ObjectInitMember::value>(
                ref<Expression_> | ref<ArrayInitializerExpr_> |
                ref<ObjectInitializerExpr_>))> {};

  struct ObjectInitializerExpr_
      : rule<create<ast::ObjectInitializerExpr>(
            lit<"{"> +
            option(some(append<&ast::ObjectInitializerExpr::members>(
                            ref<ObjectInitMember_>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}">)> {};

  static constexpr auto ObjectOrCollectionInit =
      ref<ObjectInitializerExpr_> | ref<ArrayInitializerExpr_>;

  // -- Patterns ----
  struct DiscardPattern_
      : rule<create<ast::DiscardPattern>(ref<Underscore>)> {};

  struct SimpleDesignation_
      : rule<create<ast::Designation>(
            flag<&ast::Designation::discard>(ref<Underscore>)) |
             create<ast::Designation>(
                 assign<&ast::Designation::name>(ref<Ident>))> {};

  struct Designation_
      : rule<ref<SimpleDesignation_> |
             create<ast::TupleDesignation>(
                 lit<"("> +
                 option(some(append<&ast::TupleDesignation::items>(
                                 ref<Designation_>),
                             lit<",">)) +
                 lit<")">)> {};

  struct VarPattern_
      : rule<create<ast::VarPattern>(
            kw<"var"> +
            assign<&ast::VarPattern::designation>(ref<Designation_>))> {};

  struct DeclarationPattern_
      : rule<create<ast::DeclarationPattern>(
            assign<&ast::DeclarationPattern::typeRef>(ref<TypeNode>) +
            assign<&ast::DeclarationPattern::designation>(
                ref<SimpleDesignation_>))> {};

  struct ConstantPattern_
      : rule<create<ast::ConstantPattern>(
            assign<&ast::ConstantPattern::value>(ref<RelationalAndBelow_>))> {};

  struct Subpattern_
      : rule<create<ast::Subpattern>(
            option((assign<&ast::Subpattern::name>(ref<Ident>) |
                    assign<&ast::Subpattern::expression>(
                        ref<RelationalAndBelow_>)) + lit<":">) +
            assign<&ast::Subpattern::value>(ref<Pattern_>))> {};

  struct PositionalPattern_
      : rule<create<ast::PositionalPattern>(
            option(assign<&ast::PositionalPattern::typeRef>(
                ref<TypeNode>)) +
            lit<"("> +
            option(some(append<&ast::PositionalPattern::items>(
                            ref<Subpattern_>),
                        lit<",">)) +
            lit<")"> +
            option(lit<"{"> +
                   option(some(append<&ast::PositionalPattern::propertyItems>(
                                   ref<Subpattern_>),
                               lit<",">) +
                          option(lit<",">)) +
                   lit<"}">) +
            option(assign<&ast::PositionalPattern::designation>(
                ref<SimpleDesignation_>)))> {};

  struct PropertyPattern_
      : rule<create<ast::PropertyPattern>(
            option(assign<&ast::PropertyPattern::typeRef>(ref<TypeNode>)) +
            lit<"{"> +
            option(some(append<&ast::PropertyPattern::items>(
                            ref<Subpattern_>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}"> +
            option(assign<&ast::PropertyPattern::designation>(
                ref<SimpleDesignation_>)))> {};

  struct SlicePattern_
      : rule<create<ast::SlicePattern>(
            lit<".."> +
            option(assign<&ast::SlicePattern::pattern>(ref<Pattern_>)))> {};

  struct ListPattern_
      : rule<create<ast::ListPattern>(
            lit<"["> +
            option(some(append<&ast::ListPattern::items>(ref<Pattern_>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"]"> +
            option(assign<&ast::ListPattern::designation>(
                ref<SimpleDesignation_>)))> {};

  struct PrimaryPattern_
      : rule<ref<PositionalPattern_> | ref<PropertyPattern_> |
             ref<ListPattern_> | ref<SlicePattern_> |
             create<ast::RelationalPattern>(
                 assign<&ast::RelationalPattern::op>(lit<"<="> | lit<">="> |
                                                     Lt | Gt) +
                 assign<&ast::RelationalPattern::value>(
                     ref<RelationalAndBelow_>)) |
             (lit<"("> + ref<Pattern_> + lit<")">) | ref<VarPattern_> |
             ref<DeclarationPattern_> | ref<DiscardPattern_> |
             create<ast::TypePattern>(
                 assign<&ast::TypePattern::typeRef>(ref<TypeNode>)) |
             ref<ConstantPattern_>> {};

  struct NegatedPattern_
      : rule<create<ast::UnaryPattern>(
                 assign<&ast::UnaryPattern::op>(kw<"not">) +
                 assign<&ast::UnaryPattern::operand>(ref<NegatedPattern_>)) |
             ref<PrimaryPattern_>> {};

  struct Pattern_ : infix<NegatedPattern_, &ast::BinaryPattern::left,
                          &ast::BinaryPattern::op, &ast::BinaryPattern::right,
                          (left(kw<"and">) >> left(kw<"or">))> {};

  // -- Expressions: primary chain ----
  struct SimpleNameExpr_
      : rule<create<ast::NameExpr>(
            option(assign<&ast::NameExpr::alias>(ref<Ident>) + lit<"::">) +
            assign<&ast::NameExpr::name>(ref<Ident> | PredefinedTypeKw |
                                          kw<"dynamic">) +
            option(Lt +
                   some(append<&ast::NameExpr::typeArguments>(ref<TypeNode>),
                        lit<",">) +
                   Gt))> {};

  struct TupleExprElement_
      : rule<create<ast::TupleExprElement>(
            option(assign<&ast::TupleExprElement::name>(ref<Ident>) +
                   lit<":">) +
            assign<&ast::TupleExprElement::value>(ref<Expression_>))> {};

  struct TupleExpr_
      : rule<create<ast::TupleExpr>(
            lit<"("> +
            append<&ast::TupleExpr::elements>(ref<TupleExprElement_>) +
            some(lit<","> +
                 append<&ast::TupleExpr::elements>(ref<TupleExprElement_>)) +
            lit<")">)> {};

  struct CollectionElement_
      : rule<create<ast::CollectionElement>(
            option(flag<&ast::CollectionElement::spread>(lit<"..">)) +
            assign<&ast::CollectionElement::value>(ref<Expression_>))> {};

  struct CollectionExpr_
      : rule<create<ast::CollectionExpr>(
            lit<"["> +
            option(some(append<&ast::CollectionExpr::elements>(
                            ref<CollectionElement_>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"]">)> {};

  static constexpr auto ObjectCreationArguments =
      lit<"("> +
      option(some(append<&ast::ObjectCreationExpr::args>(ref<Argument_>),
                  lit<",">)) +
      lit<")">;

  struct ObjectCreationExpr_
      : rule<create<ast::ObjectCreationExpr>(
            kw<"new"> +
            ((option(assign<&ast::ObjectCreationExpr::typeRef>(ref<TypeNode>)) +
              ObjectCreationArguments +
              option(assign<&ast::ObjectCreationExpr::init>(
                  ObjectOrCollectionInit))) |
             (assign<&ast::ObjectCreationExpr::typeRef>(ref<TypeNode>) +
              assign<&ast::ObjectCreationExpr::init>(
                  ObjectOrCollectionInit))))> {};

  struct MemberDeclarator_
      : rule<create<ast::MemberDeclarator>(
            assign<&ast::MemberDeclarator::name>(ref<Ident>) + lit<"="> +
            assign<&ast::MemberDeclarator::value>(ref<Expression_>)) |
             create<ast::MemberDeclarator>(
                 assign<&ast::MemberDeclarator::value>(ref<Expression_>))> {};

  struct AnonymousObjectCreationExpr_
      : rule<create<ast::AnonymousObjectCreationExpr>(
            kw<"new"> + lit<"{"> +
            option(some(append<&ast::AnonymousObjectCreationExpr::members>(
                            ref<MemberDeclarator_>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct ArrayCreationExpr_
      : rule<create<ast::ArrayCreationExpr>(
            kw<"new"> +
            assign<&ast::ArrayCreationExpr::elementType>(ref<TypeNode>) +
            option(lit<"["> +
                   some(append<&ast::ArrayCreationExpr::sizes>(
                            ref<Expression_>),
                        lit<",">) +
                   lit<"]"> +
                   many(append<&ast::ArrayCreationExpr::extraRanks>(
                       ref<RankSpecifier_>))) +
            option(assign<&ast::ArrayCreationExpr::init>(
                ref<ArrayInitializerExpr_>))) |
             create<ast::ArrayCreationExpr>(
                 kw<"new"> +
                 append<&ast::ArrayCreationExpr::extraRanks>(
                     ref<RankSpecifier_>) +
                 assign<&ast::ArrayCreationExpr::init>(
                     ref<ArrayInitializerExpr_>))> {};

  // A data rule (own body, no directives): the dotted name of a `typeof`
  // unbound type or a `nameof` named entity, generics reduced to their
  // comma count (`List<,,>`'s arity, no element types).
  struct DottedNameTail_
      : rule<ref<Ident> + option(Lt + many(lit<",">) + Gt) +
             many(lit<"."> + ref<Ident> +
                  option(Lt + many(lit<",">) + Gt))> {};

  struct TypeofExpr_
      : rule<create<ast::TypeofExpr>(
            kw<"typeof"> + lit<"("> +
            (flag<&ast::TypeofExpr::isVoid>(kw<"void">) |
             assign<&ast::TypeofExpr::typeRef>(ref<TypeNode>) |
             assign<&ast::TypeofExpr::unboundText>(
                 ref<DottedNameTail_>)) +
            lit<")">)> {};

  struct SizeofExpr_
      : rule<create<ast::SizeofExpr>(
            kw<"sizeof"> + lit<"("> +
            assign<&ast::SizeofExpr::typeRef>(ref<TypeNode>) + lit<")">)> {};

  struct CheckedExpr_
      : rule<create<ast::CheckedExpr>(
            flag<&ast::CheckedExpr::isChecked>(kw<"checked">) +
            lit<"("> + assign<&ast::CheckedExpr::value>(ref<Expression_>) +
            lit<")">) |
             create<ast::CheckedExpr>(
                 kw<"unchecked"> + lit<"("> +
                 assign<&ast::CheckedExpr::value>(ref<Expression_>) +
                 lit<")">)> {};

  struct DefaultExpr_
      : rule<create<ast::DefaultExpr>(
            kw<"default"> +
            option(lit<"("> +
                   assign<&ast::DefaultExpr::typeRef>(ref<TypeNode>) +
                   lit<")">))> {};

  struct NameofExpr_
      : rule<create<ast::NameofExpr>(
            kw<"nameof"> + lit<"("> +
            assign<&ast::NameofExpr::text>(
                ref<DottedNameTail_> | kw<"this"> | kw<"base"> |
                PredefinedTypeKw) +
            lit<")">)> {};

  struct StackallocExpr_
      : rule<create<ast::StackallocExpr>(
            kw<"stackalloc"> +
            option(assign<&ast::StackallocExpr::elementType>(
                ref<TypeNode>)) +
            lit<"["> +
            option(assign<&ast::StackallocExpr::size>(ref<Expression_>)) +
            lit<"]"> +
            option(assign<&ast::StackallocExpr::init>(
                ref<ArrayInitializerExpr_>)))> {};

  struct MsExtensionExpr_
      : rule<create<ast::MsExtensionExpr>(
            assign<&ast::MsExtensionExpr::kind>(kw<"__makeref">) + lit<"("> +
            assign<&ast::MsExtensionExpr::arg1>(ref<Expression_>) +
            lit<")">) |
             create<ast::MsExtensionExpr>(
                 assign<&ast::MsExtensionExpr::kind>(kw<"__reftype">) +
                 lit<"("> +
                 assign<&ast::MsExtensionExpr::arg1>(ref<Expression_>) +
                 lit<")">) |
             create<ast::MsExtensionExpr>(
                 assign<&ast::MsExtensionExpr::kind>(kw<"__refvalue">) +
                 lit<"("> +
                 assign<&ast::MsExtensionExpr::arg1>(ref<Expression_>) +
                 lit<","> +
                 assign<&ast::MsExtensionExpr::typeRef>(ref<TypeNode>) +
                 lit<")">)> {};

  static constexpr auto ImplicitLambdaParam =
      create<ast::Param>(assign<&ast::Param::name>(ref<Ident>));
  static constexpr auto ExplicitLambdaParam = ref<Param_>;

  static constexpr auto LambdaSignature =
      (lit<"("> +
       option(some(append<&ast::LambdaExpr::params>(
                       ExplicitLambdaParam | ImplicitLambdaParam),
                   lit<",">)) +
       lit<")">) |
      append<&ast::LambdaExpr::params>(ImplicitLambdaParam);

  struct LambdaExpr_
      : rule<create<ast::LambdaExpr>(
            attributes<&ast::LambdaExpr::attributes> +
            many(flag<&ast::LambdaExpr::isAsync>(kw<"async">) |
                 flag<&ast::LambdaExpr::isStatic>(kw<"static">)) +
            option(option(assign<&ast::LambdaExpr::refKind>(ref<RefKind_>)) +
                   assign<&ast::LambdaExpr::returnType>(ref<TypeNode>)) +
            LambdaSignature + lit<"=>"> +
            (assign<&ast::LambdaExpr::blockBody>(ref<Block_>) |
             (flag<&ast::LambdaExpr::refBody>(kw<"ref">) +
              assign<&ast::LambdaExpr::refBodyValue>(ref<Expression_>)) |
             assign<&ast::LambdaExpr::exprBody>(ref<Expression_>)))> {};

  struct AnonymousMethodExpr_
      : rule<create<ast::AnonymousMethodExpr>(
            option(flag<&ast::AnonymousMethodExpr::isAsync>(
                kw<"async">)) +
            kw<"delegate"> +
            option(flag<&ast::AnonymousMethodExpr::hasParamList>(
                       lit<"(">) +
                   option(some(append<&ast::AnonymousMethodExpr::params>(
                                   ExplicitLambdaParam),
                               lit<",">)) +
                   lit<")">) +
            assign<&ast::AnonymousMethodExpr::body>(ref<Block_>))> {};

  // -- Query expressions ----
  struct FromClause_
      : rule<create<ast::QueryClause>(
            assign<&ast::QueryClause::kind>(kw<"from">) +
            option(assign<&ast::QueryClause::typeRef>(ref<TypeNode>)) +
            assign<&ast::QueryClause::name>(ref<Ident>) + kw<"in"> +
            assign<&ast::QueryClause::expr1>(ref<Expression_>))> {};

  struct LetClause_
      : rule<create<ast::QueryClause>(
            assign<&ast::QueryClause::kind>(kw<"let">) +
            assign<&ast::QueryClause::name>(ref<Ident>) + lit<"="> +
            assign<&ast::QueryClause::expr1>(ref<Expression_>))> {};

  struct WhereClause_
      : rule<create<ast::QueryClause>(
            assign<&ast::QueryClause::kind>(kw<"where">) +
            assign<&ast::QueryClause::expr1>(ref<Expression_>))> {};

  struct JoinClause_
      : rule<create<ast::QueryClause>(
            assign<&ast::QueryClause::kind>(kw<"join">) +
            option(assign<&ast::QueryClause::typeRef>(ref<TypeNode>)) +
            assign<&ast::QueryClause::name>(ref<Ident>) + kw<"in"> +
            assign<&ast::QueryClause::expr1>(ref<Expression_>) + kw<"on"> +
            assign<&ast::QueryClause::expr2>(ref<Expression_>) +
            kw<"equals"> +
            assign<&ast::QueryClause::expr3>(ref<Expression_>) +
            option(kw<"into"> +
                   assign<&ast::QueryClause::intoName>(ref<Ident>)))> {};

  struct Ordering_
      : rule<create<ast::Ordering>(
            assign<&ast::Ordering::key>(ref<Expression_>) +
            option(assign<&ast::Ordering::direction>(
                kw<"ascending"> | kw<"descending">)))> {};

  struct OrderbyClause_
      : rule<create<ast::QueryClause>(
            assign<&ast::QueryClause::kind>(kw<"orderby">) +
            some(append<&ast::QueryClause::orderings>(ref<Ordering_>),
                 lit<",">))> {};

  struct QueryBodyClause_
      : rule<ref<FromClause_> | ref<LetClause_> | ref<WhereClause_> |
             ref<JoinClause_> | ref<OrderbyClause_>> {};

  struct SelectOrGroupClause_
      : rule<create<ast::QueryClause>(
            assign<&ast::QueryClause::kind>(kw<"select">) +
            assign<&ast::QueryClause::expr1>(ref<Expression_>)) |
             create<ast::QueryClause>(
                 assign<&ast::QueryClause::kind>(kw<"group">) +
                 assign<&ast::QueryClause::expr1>(ref<Expression_>) +
                 kw<"by"> +
                 assign<&ast::QueryClause::expr2>(ref<Expression_>))> {};

  // A continuation (`into`) opens a new segment in the same flat list
  // instead of nesting: `QueryClause` would otherwise point at itself.
  struct QueryContinuationMarker_
      : rule<create<ast::QueryClause>(
            assign<&ast::QueryClause::kind>(kw<"into">) +
            assign<&ast::QueryClause::name>(ref<Ident>))> {};

  struct QueryExpr_
      : rule<create<ast::QueryExpr>(
            append<&ast::QueryExpr::clauses>(ref<FromClause_>) +
            many(append<&ast::QueryExpr::clauses>(ref<QueryBodyClause_>)) +
            append<&ast::QueryExpr::clauses>(ref<SelectOrGroupClause_>) +
            many(append<&ast::QueryExpr::clauses>(
                     ref<QueryContinuationMarker_>) +
                 many(append<&ast::QueryExpr::clauses>(
                     ref<QueryBodyClause_>)) +
                 append<&ast::QueryExpr::clauses>(
                     ref<SelectOrGroupClause_>)))> {};

  // -- Primary head and the iterative postfix chain ----
  struct PrimaryHead_
      : rule<assign<&ast::LiteralExpr::text>(
                 ref<NUMBER> | ref<CHAR> | ref<STRING> | ref<VERBATIM_STRING> |
                 ref<RAW_STRING> | kw<"true"> | kw<"false"> | kw<"null">) |
             ref<InterpolatedStringExpr_> | ref<TupleExpr_> |
             ref<CollectionExpr_> | (lit<"("> + ref<Expression_> + lit<")">) |
             create<ast::ThisExpr>(kw<"this">) |
             create<ast::BaseExpr>(kw<"base">) | ref<ObjectCreationExpr_> |
             ref<AnonymousObjectCreationExpr_> | ref<ArrayCreationExpr_> |
             ref<TypeofExpr_> | ref<SizeofExpr_> | ref<CheckedExpr_> |
             ref<DefaultExpr_> | ref<NameofExpr_> | ref<StackallocExpr_> |
             ref<MsExtensionExpr_> | ref<AnonymousMethodExpr_> |
             ref<SimpleNameExpr_>> {};

  struct PostfixChain_
      : rule<ref<PrimaryHead_> +
             many(
                 nest<ast::MemberExpr, &ast::MemberExpr::target>(
                     lit<"."> + assign<&ast::MemberExpr::member>(ref<Ident>) +
                     option(Lt +
                            some(append<&ast::MemberExpr::typeArguments>(
                                     ref<TypeNode>),
                                 lit<",">) +
                            Gt)) |
                 nest<ast::MemberExpr, &ast::MemberExpr::target>(
                     flag<&ast::MemberExpr::nullConditional>(lit<"?">) +
                     lit<"."> + assign<&ast::MemberExpr::member>(ref<Ident>) +
                     option(Lt +
                            some(append<&ast::MemberExpr::typeArguments>(
                                     ref<TypeNode>),
                                 lit<",">) +
                            Gt)) |
                 nest<ast::MemberExpr, &ast::MemberExpr::target>(
                     flag<&ast::MemberExpr::pointerArrow>(lit<"->">) +
                     assign<&ast::MemberExpr::member>(ref<Ident>)) |
                 nest<ast::InvokeExpr, &ast::InvokeExpr::callee>(
                     lit<"("> +
                     option(some(append<&ast::InvokeExpr::args>(ref<Argument_>),
                                 lit<",">)) +
                     lit<")">) |
                 nest<ast::IndexExpr, &ast::IndexExpr::target>(
                     lit<"["> +
                     some(append<&ast::IndexExpr::args>(ref<Argument_>),
                          lit<",">) +
                     lit<"]">) |
                 nest<ast::IndexExpr, &ast::IndexExpr::target>(
                     flag<&ast::IndexExpr::nullConditional>(lit<"?">) +
                     lit<"["> +
                     some(append<&ast::IndexExpr::args>(ref<Argument_>),
                          lit<",">) +
                     lit<"]">) |
                 nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                     assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--"> |
                                                   lit<"!">)))> {};

  // -- Unary / cast / await ----
  static constexpr auto UnaryOp =
      lit<"+"> | lit<"-"> | lit<"!"> | lit<"~"> | lit<"^">;

  struct UnaryExpr_
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--"> | UnaryOp |
                                              lit<"*"> | lit<"&"> |
                                              kw<"await">) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpr_>)) |
             ref<CastExpr_> | ref<PostfixChain_>> {};

  struct CastExpr_
      : rule<create<ast::CastExpr>(
            lit<"("> + assign<&ast::CastExpr::typeRef>(ref<TypeNode>) +
            lit<")"> +
            assign<&ast::CastExpr::operand>(ref<UnaryExpr_>))> {};

  // -- Range / switch expression ----
  struct RangeExpr_
      : rule<create<ast::RangeExpr>(
            assign<&ast::RangeExpr::from>(ref<UnaryExpr_>) + lit<".."> +
            option(assign<&ast::RangeExpr::to>(ref<UnaryExpr_>))) |
             create<ast::RangeExpr>(
                 lit<".."> +
                 option(assign<&ast::RangeExpr::to>(ref<UnaryExpr_>))) |
             ref<UnaryExpr_>> {};

  struct SwitchArm_
      : rule<create<ast::SwitchArm>(
            assign<&ast::SwitchArm::pattern>(ref<Pattern_>) +
            option(kw<"when"> +
                   assign<&ast::SwitchArm::guard>(ref<NullCoalesceExpr_>)) +
            lit<"=>"> +
            assign<&ast::SwitchArm::result>(ref<Expression_>))> {};

  struct SwitchExpr_
      : rule<ref<RangeExpr_> +
             many(nest<ast::SwitchExpr, &ast::SwitchExpr::value>(
                      kw<"switch"> + lit<"{"> +
                      option(
                          some(append<&ast::SwitchExpr::arms>(ref<SwitchArm_>),
                               lit<",">) +
                          option(lit<",">)) +
                      lit<"}">) |
                  nest<ast::WithExpr, &ast::WithExpr::target>(
                      kw<"with"> + assign<&ast::WithExpr::init>(
                                       ref<ObjectInitializerExpr_>)))> {};

  // -- Binary operator ladder ----
  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;

  struct AdditiveLevel_
      : infix<SwitchExpr_, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(OpMul) >> left(OpAdd))> {};

  // `<<`/`>>` open on the same token as the relational operators one level
  // up: kept out of the infix table below and hand-rolled, like `>>`
  // splitting into two adjacent `>` for a nested generic close.
  struct ShiftOp_ : rule<(Lt + Lt) | (Gt + Gt + Gt) | (Gt + Gt)> {};
  struct ShiftExpr_
      : rule<ref<AdditiveLevel_> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<ShiftOp_>) +
                 assign<&ast::BinaryExpr::right>(ref<AdditiveLevel_>)))> {};

  static constexpr auto OpRel = lit<"<="> | Gt | lit<">="> | Lt;

  struct RelationalAndBelow_
      : infix<ShiftExpr_, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(OpRel)> {};

  // `is`/`as` read a type or pattern, not another expression, so they sit
  // outside the infix table; the whole thing is itself a relational
  // expression, so it self-recurses to chain with `<`/`<=`/`>`/`>=`. A name
  // that may be a type or a constant reads as the type, as the spec chooses.
  struct IsAsExpr_
      : rule<create<ast::IsExpr>(
            assign<&ast::IsExpr::value>(ref<IsAsExpr_>) + kw<"is"> +
            (assign<&ast::IsExpr::typeRef>(ref<TypeNode>) |
             assign<&ast::IsExpr::pattern>(ref<Pattern_>))) |
             create<ast::AsExpr>(
                 assign<&ast::AsExpr::value>(ref<IsAsExpr_>) + kw<"as"> +
                 assign<&ast::AsExpr::typeRef>(ref<TypeNode>)) |
             ref<RelationalAndBelow_>> {};

  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;

  struct EqualityAndAbove_
      : infix<IsAsExpr_, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpEq) >> left(OpBitAnd) >> left(OpBitXor) >>
               left(OpBitOr) >> left(OpLogAnd) >> left(OpLogOr))> {};

  struct NullCoalesceExpr_
      : rule<create<ast::BinaryExpr>(
            assign<&ast::BinaryExpr::left>(ref<EqualityAndAbove_>) +
            assign<&ast::BinaryExpr::op>(lit<"??">) +
            assign<&ast::BinaryExpr::right>(ref<NullCoalesceExpr_>)) |
             create<ast::ThrowExpr>(
                 kw<"throw"> +
                 assign<&ast::ThrowExpr::value>(ref<NullCoalesceExpr_>)) |
             ref<EqualityAndAbove_>> {};

  struct ConditionalExpr_
      : rule<create<ast::ConditionalExpr>(
            assign<&ast::ConditionalExpr::cond>(ref<NullCoalesceExpr_>) +
            lit<"?"> +
            ((assign<&ast::ConditionalExpr::thenValue>(ref<Expression_>) +
              lit<":"> +
              assign<&ast::ConditionalExpr::elseValue>(ref<Expression_>)) |
             (flag<&ast::ConditionalExpr::refThen>(kw<"ref">) +
              assign<&ast::ConditionalExpr::thenValue>(ref<Expression_>) +
              lit<":"> +
              flag<&ast::ConditionalExpr::refElse>(kw<"ref">) +
              assign<&ast::ConditionalExpr::elseValue>(
                  ref<Expression_>)))) |
             ref<NullCoalesceExpr_>> {};

  struct DeclarationExpr_
      : rule<create<ast::DeclarationExpr>(
            assign<&ast::DeclarationExpr::typeRef>(ref<TypeNode>) +
            assign<&ast::DeclarationExpr::name>(ref<Ident>))> {};

  struct NonAssignmentExpr_
      : rule<ref<DeclarationExpr_> | ref<QueryExpr_> | ref<LambdaExpr_> |
             ref<ConditionalExpr_>> {};

  static constexpr auto CompoundAssignOp =
      lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> | lit<"%="> |
      lit<"&="> | lit<"|="> | lit<"^="> | lit<"<<="> | lit<"?" "?=">;
  struct RightShiftAssignOp_ : rule<(Gt + Gt + lit<">=">) | (Gt + lit<">=">)> {
  };

  struct Expression_
      : rule<create<ast::AssignExpr>(
            assign<&ast::AssignExpr::target>(ref<NonAssignmentExpr_>) +
            ((assign<&ast::AssignExpr::op>(lit<"=">) +
              option(flag<&ast::AssignExpr::refValue>(kw<"ref">)) +
              assign<&ast::AssignExpr::value>(ref<Expression_>)) |
             (assign<&ast::AssignExpr::op>(CompoundAssignOp) +
              assign<&ast::AssignExpr::value>(ref<Expression_>)) |
             (assign<&ast::AssignExpr::op>(ref<RightShiftAssignOp_>) +
              assign<&ast::AssignExpr::value>(ref<Expression_>)))) |
             ref<NonAssignmentExpr_>> {};

  // -- Statements ----
  static constexpr auto VarOrType =
      (flag<&ast::LocalVarDeclStmt::isVar>(kw<"var">)) |
      assign<&ast::LocalVarDeclStmt::typeRef>(ref<TypeNode>);

  // Shared with `UsingStmt_`'s resource clause, which is not `;`-terminated.
  static constexpr auto LocalVarDeclBody =
      option(assign<&ast::LocalVarDeclStmt::refKind>(
          ref<RefKind_>)) +
      VarOrType +
      some(append<&ast::LocalVarDeclStmt::declarators>(ref<Declarator_>),
           lit<",">);

  struct LocalVarDeclStmt_
      : rule<create<ast::LocalVarDeclStmt>(LocalVarDeclBody + lit<";">)> {};

  // Named so `ForStmt_`/`UsingStmt_` reference it instead of nesting the
  // same create inline under their own.
  struct NonRefLocalVarDecl_
      : rule<create<ast::LocalVarDeclStmt>(LocalVarDeclBody)> {};

  struct LocalConstDeclStmt_
      : rule<create<ast::LocalConstDeclStmt>(
            kw<"const"> +
            assign<&ast::LocalConstDeclStmt::typeRef>(ref<TypeNode>) +
            some(append<&ast::LocalConstDeclStmt::declarators>(
                     ref<Declarator_>),
                 lit<",">) +
            lit<";">)> {};

  static constexpr auto LocalFunctionModifierKw =
      kw<"static"> | kw<"unsafe"> | kw<"async">;

  struct LocalFunctionDeclStmt_
      : rule<create<ast::LocalFunctionDeclStmt>(
            many(append<&ast::LocalFunctionDeclStmt::modifiers>(
                LocalFunctionModifierKw)) +
            option(assign<&ast::LocalFunctionDeclStmt::refKind>(
                ref<RefKind_>)) +
            (flag<&ast::LocalFunctionDeclStmt::isVoid>(kw<"void">) |
             assign<&ast::LocalFunctionDeclStmt::returnType>(
                 ref<TypeNode>)) +
            assign<&ast::LocalFunctionDeclStmt::name>(ref<Ident>) +
            option(Lt +
                   some(append<&ast::LocalFunctionDeclStmt::typeParams>(
                            ref<TypeParam_>),
                        lit<",">) +
                   Gt) +
            lit<"("> +
            option(some(append<&ast::LocalFunctionDeclStmt::params>(
                            ref<Param_>),
                        lit<",">)) +
            lit<")"> +
            many(append<&ast::LocalFunctionDeclStmt::constraints>(
                ref<TypeParamConstraintClause_>)) +
            (assign<&ast::LocalFunctionDeclStmt::body>(ref<Block_>) |
             (lit<"=>"> +
              option(flag<&ast::LocalFunctionDeclStmt::isRefBody>(
                  kw<"ref">)) +
              assign<&ast::LocalFunctionDeclStmt::exprBody>(
                  ref<Expression_>) +
              lit<";">)))> {};

  struct UsingDeclStmt_
      : rule<create<ast::UsingDeclStmt>(
            option(flag<&ast::UsingDeclStmt::isAwait>(kw<"await">)) +
            kw<"using"> +
            ((flag<&ast::UsingDeclStmt::isVar>(kw<"var">)) |
             assign<&ast::UsingDeclStmt::typeRef>(ref<TypeNode>)) +
            some(append<&ast::UsingDeclStmt::declarators>(ref<Declarator_>),
                 lit<",">) +
            lit<";"> +
            many(append<&ast::UsingDeclStmt::statements>(
                ref<Statement_>)))> {};

  struct DeclarationStmt_
      : rule<ref<LocalVarDeclStmt_> | ref<LocalConstDeclStmt_> |
             ref<LocalFunctionDeclStmt_> | ref<UsingDeclStmt_>> {};

  struct LabeledStmt_
      : rule<create<ast::LabeledStmt>(
            assign<&ast::LabeledStmt::label>(ref<Ident>) + lit<":"> +
            assign<&ast::LabeledStmt::body>(ref<Statement_>))> {};

  struct EmptyStmt_ : rule<create<ast::EmptyStmt>(lit<";">)> {};

  struct ExprStmt_
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(ref<Expression_>) + lit<";">)> {};

  struct IfStmt_
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> +
            assign<&ast::IfStmt::cond>(ref<Expression_>) + lit<")"> +
            assign<&ast::IfStmt::thenBranch>(ref<Statement_>) +
            option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement_>))))> {};

  struct SwitchLabel_
      : rule<create<ast::SwitchLabel>(
            kw<"case"> + assign<&ast::SwitchLabel::pattern>(ref<Pattern_>) +
            option(kw<"when"> +
                   assign<&ast::SwitchLabel::guard>(
                       ref<NullCoalesceExpr_>)) +
            lit<":">) |
             create<ast::SwitchLabel>(
                 flag<&ast::SwitchLabel::isDefault>(kw<"default">) +
                 lit<":">)> {};

  struct SwitchSection_
      : rule<create<ast::SwitchSection>(
            some(append<&ast::SwitchSection::labels>(ref<SwitchLabel_>)) +
            some(append<&ast::SwitchSection::statements>(
                ref<Statement_>)))> {};

  struct SwitchStmt_
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> +
            ((lit<"("> + assign<&ast::SwitchStmt::value>(ref<Expression_>) +
              lit<")">) |
             assign<&ast::SwitchStmt::value>(ref<TupleExpr_>)) +
            lit<"{"> +
            many(append<&ast::SwitchStmt::sections>(ref<SwitchSection_>)) +
            lit<"}">)> {};

  struct WhileStmt_
      : rule<create<ast::WhileStmt>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileStmt::cond>(ref<Expression_>) + lit<")"> +
            assign<&ast::WhileStmt::body>(ref<Statement_>))> {};

  struct DoStmt_
      : rule<create<ast::DoStmt>(
            kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement_>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoStmt::cond>(ref<Expression_>) + lit<")"> +
            lit<";">)> {};

  struct ForStmt_
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            option(assign<&ast::ForStmt::initDecl>(
                       ref<NonRefLocalVarDecl_>) |
                   some(append<&ast::ForStmt::initExprs>(ref<Expression_>),
                        lit<",">)) +
            lit<";"> +
            option(assign<&ast::ForStmt::cond>(ref<Expression_>)) +
            lit<";"> +
            option(some(append<&ast::ForStmt::iterators>(ref<Expression_>),
                        lit<",">)) +
            lit<")"> +
            assign<&ast::ForStmt::body>(ref<Statement_>))> {};

  struct ForeachStmt_
      : rule<create<ast::ForeachStmt>(
            option(flag<&ast::ForeachStmt::isAwait>(kw<"await">)) +
            kw<"foreach"> + lit<"("> +
            option(assign<&ast::ForeachStmt::refKind>(
                ref<RefKind_>)) +
            ((flag<&ast::ForeachStmt::isVar>(kw<"var">)) |
             assign<&ast::ForeachStmt::typeRef>(ref<TypeNode>)) +
            (assign<&ast::ForeachStmt::name>(ref<Ident>) |
             assign<&ast::ForeachStmt::deconstruct>(ref<Designation_>)) +
            kw<"in"> +
            assign<&ast::ForeachStmt::collection>(ref<Expression_>) +
            lit<")"> +
            assign<&ast::ForeachStmt::body>(ref<Statement_>))> {};

  struct BreakStmt_
      : rule<create<ast::BreakStmt>(kw<"break"> + lit<";">)> {};
  struct ContinueStmt_
      : rule<create<ast::ContinueStmt>(kw<"continue"> + lit<";">)> {};

  struct GotoStmt_
      : rule<create<ast::GotoStmt>(
            kw<"goto"> + assign<&ast::GotoStmt::label>(ref<Ident>) +
            lit<";">) |
             create<ast::GotoStmt>(
                 kw<"goto"> + kw<"case"> +
                 assign<&ast::GotoStmt::caseValue>(ref<Expression_>) +
                 lit<";">) |
             create<ast::GotoStmt>(
                 kw<"goto"> +
                 flag<&ast::GotoStmt::isDefault>(kw<"default">) +
                 lit<";">)> {};

  struct ReturnStmt_
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(flag<&ast::ReturnStmt::isRef>(kw<"ref">)) +
            option(assign<&ast::ReturnStmt::value>(ref<Expression_>)) +
            lit<";">)> {};

  struct ThrowStmt_
      : rule<create<ast::ThrowStmt>(
            kw<"throw"> +
            option(assign<&ast::ThrowStmt::value>(ref<Expression_>)) +
            lit<";">)> {};

  struct CatchClause_
      : rule<create<ast::CatchClause>(
            kw<"catch"> +
            option(lit<"("> +
                   assign<&ast::CatchClause::exceptionType>(ref<TypeNode>) +
                   option(assign<&ast::CatchClause::name>(ref<Ident>)) +
                   lit<")">) +
            option(kw<"when"> + lit<"("> +
                   assign<&ast::CatchClause::filter>(ref<Expression_>) +
                   lit<")">) +
            assign<&ast::CatchClause::body>(ref<Block_>))> {};

  struct TryStmt_
      : rule<create<ast::TryStmt>(
            kw<"try"> + assign<&ast::TryStmt::block>(ref<Block_>) +
            some(append<&ast::TryStmt::catches>(ref<CatchClause_>)) +
            option(kw<"finally"> +
                   assign<&ast::TryStmt::finallyBlock>(ref<Block_>))) |
             create<ast::TryStmt>(
                 kw<"try"> + assign<&ast::TryStmt::block>(ref<Block_>) +
                 kw<"finally"> +
                 assign<&ast::TryStmt::finallyBlock>(ref<Block_>))> {};

  struct CheckedStmt_
      : rule<create<ast::CheckedStmt>(
            flag<&ast::CheckedStmt::isChecked>(kw<"checked">) +
            assign<&ast::CheckedStmt::body>(ref<Block_>)) |
             create<ast::CheckedStmt>(
                 kw<"unchecked"> +
                 assign<&ast::CheckedStmt::body>(ref<Block_>))> {};

  struct LockStmt_
      : rule<create<ast::LockStmt>(
            kw<"lock"> + lit<"("> +
            assign<&ast::LockStmt::value>(ref<Expression_>) + lit<")"> +
            assign<&ast::LockStmt::body>(ref<Statement_>))> {};

  struct UsingStmt_
      : rule<create<ast::UsingStmt>(
            option(flag<&ast::UsingStmt::isAwait>(kw<"await">)) +
            kw<"using"> + lit<"("> +
            (assign<&ast::UsingStmt::resourceDecl>(
                 ref<NonRefLocalVarDecl_>) |
             assign<&ast::UsingStmt::resourceExpr>(ref<Expression_>)) +
            lit<")"> +
            assign<&ast::UsingStmt::body>(ref<Statement_>))> {};

  struct YieldStmt_
      : rule<create<ast::YieldStmt>(
            kw<"yield"> + kw<"return"> +
            assign<&ast::YieldStmt::value>(ref<Expression_>) + lit<";">) |
             create<ast::YieldStmt>(
                 kw<"yield"> +
                 flag<&ast::YieldStmt::isBreak>(kw<"break">) +
                 lit<";">)> {};

  struct UnsafeStmt_
      : rule<create<ast::UnsafeStmt>(
            kw<"unsafe"> + assign<&ast::UnsafeStmt::body>(ref<Block_>))> {};

  struct FixedStmt_
      : rule<create<ast::FixedStmt>(
            kw<"fixed"> + lit<"("> +
            assign<&ast::FixedStmt::typeRef>(ref<TypeNode>) +
            some(append<&ast::FixedStmt::declarators>(ref<Declarator_>),
                 lit<",">) +
            lit<")"> +
            assign<&ast::FixedStmt::body>(ref<Statement_>))> {};

  struct EmbeddedStatement_
      : rule<ref<Block_> | ref<EmptyStmt_> | ref<IfStmt_> |
             ref<SwitchStmt_> | ref<WhileStmt_> | ref<DoStmt_> |
             ref<ForStmt_> | ref<ForeachStmt_> | ref<BreakStmt_> |
             ref<ContinueStmt_> | ref<GotoStmt_> | ref<ReturnStmt_> |
             ref<ThrowStmt_> | ref<TryStmt_> | ref<CheckedStmt_> |
             ref<LockStmt_> | ref<UsingStmt_> | ref<YieldStmt_> |
             ref<UnsafeStmt_> | ref<FixedStmt_> | ref<ExprStmt_>> {};

  struct Statement_
      : rule<ref<LabeledStmt_> | ref<DeclarationStmt_> |
             ref<EmbeddedStatement_>> {};

  struct Block_
      : rule<create<ast::Block>(
            lit<"{"> + many(append<&ast::Block::statements>(ref<Statement_>)) +
            lit<"}">)> {};

  // -- Members ----
  struct FieldDecl_
      : rule<create<ast::FieldDecl>(
            attributes<&ast::FieldDecl::attributes> +
            many(append<&ast::FieldDecl::modifiers>(ModifierKw)) +
            assign<&ast::FieldDecl::typeRef>(ref<TypeNode>) +
            some(append<&ast::FieldDecl::declarators>(ref<Declarator_>),
                 lit<",">) +
            lit<";">)> {};

  struct ConstDecl_
      : rule<create<ast::ConstDecl>(
            attributes<&ast::ConstDecl::attributes> +
            many(append<&ast::ConstDecl::modifiers>(ModifierKw)) + kw<"const"> +
            assign<&ast::ConstDecl::typeRef>(ref<TypeNode>) +
            some(append<&ast::ConstDecl::declarators>(ref<Declarator_>),
                 lit<",">) +
            lit<";">)> {};

  struct Accessor_
      : rule<create<ast::Accessor>(
            attributes<&ast::Accessor::attributes> +
            many(append<&ast::Accessor::modifiers>(ModifierKw)) +
            assign<&ast::Accessor::kind>(kw<"get"> | kw<"set"> | kw<"init"> |
                                         kw<"add"> | kw<"remove">) +
            (assign<&ast::Accessor::body>(ref<Block_>) |
             (lit<"=>"> + option(flag<&ast::Accessor::isRef>(kw<"ref">)) +
              assign<&ast::Accessor::exprBody>(ref<Expression_>) + lit<";">) |
             lit<";">))> {};

  struct PropertyDecl_
      : rule<create<ast::PropertyDecl>(
            attributes<&ast::PropertyDecl::attributes> +
            many(append<&ast::PropertyDecl::modifiers>(ModifierKw)) +
            option(assign<&ast::PropertyDecl::refKind>(ref<RefKind_>)) +
            assign<&ast::PropertyDecl::typeRef>(ref<TypeNode>) +
            option(
                assign<&ast::PropertyDecl::explicitInterface>(ref<TypeNode>) +
                lit<".">) +
            assign<&ast::PropertyDecl::name>(ref<Ident>) +
            ((lit<"{"> +
              some(append<&ast::PropertyDecl::accessors>(ref<Accessor_>)) +
              lit<"}"> +
              option(lit<"="> +
                     assign<&ast::PropertyDecl::initializer>(ref<Expression_>) +
                     lit<";">)) |
             (lit<"=>"> + option(kw<"ref">) +
              assign<&ast::PropertyDecl::exprBody>(ref<Expression_>) +
              lit<";">)))> {};

  struct IndexerDecl_
      : rule<create<ast::IndexerDecl>(
            attributes<&ast::IndexerDecl::attributes> +
            many(append<&ast::IndexerDecl::modifiers>(ModifierKw)) +
            option(assign<&ast::IndexerDecl::refKind>(ref<RefKind_>)) +
            assign<&ast::IndexerDecl::typeRef>(ref<TypeNode>) +
            option(assign<&ast::IndexerDecl::explicitInterface>(ref<TypeNode>) +
                   lit<".">) +
            kw<"this"> + lit<"["> +
            some(append<&ast::IndexerDecl::params>(ref<Param_>), lit<",">) +
            lit<"]"> +
            ((lit<"{"> +
              some(append<&ast::IndexerDecl::accessors>(ref<Accessor_>)) +
              lit<"}">) |
             (lit<"=>"> + option(kw<"ref">) +
              assign<&ast::IndexerDecl::exprBody>(ref<Expression_>) +
              lit<";">)))> {};

  struct EventDecl_
      : rule<create<ast::EventDecl>(
            attributes<&ast::EventDecl::attributes> +
            many(append<&ast::EventDecl::modifiers>(ModifierKw)) + kw<"event"> +
            assign<&ast::EventDecl::typeRef>(ref<TypeNode>) +
            ((some(append<&ast::EventDecl::declarators>(ref<Declarator_>),
                   lit<",">) +
              lit<";">) |
             (option(assign<&ast::EventDecl::explicitInterface>(ref<TypeNode>) +
                     lit<".">) +
              assign<&ast::EventDecl::name>(ref<Ident>) + lit<"{"> +
              some(append<&ast::EventDecl::accessors>(ref<Accessor_>)) +
              lit<"}">)))> {};

  struct MethodDecl_
      : rule<create<ast::MethodDecl>(
            attributes<&ast::MethodDecl::attributes> +
            many(append<&ast::MethodDecl::modifiers>(ModifierKw)) +
            option(assign<&ast::MethodDecl::refKind>(ref<RefKind_>)) +
            (flag<&ast::MethodDecl::isVoid>(kw<"void">) |
             assign<&ast::MethodDecl::returnType>(ref<TypeNode>)) +
            option(assign<&ast::MethodDecl::explicitInterface>(ref<TypeNode>) +
                   lit<".">) +
            assign<&ast::MethodDecl::name>(ref<Ident>) +
            option(Lt +
                   some(append<&ast::MethodDecl::typeParams>(ref<TypeParam_>),
                        lit<",">) +
                   Gt) +
            lit<"("> +
            option(
                some(append<&ast::MethodDecl::params>(ref<Param_>), lit<",">)) +
            lit<")"> +
            many(append<&ast::MethodDecl::constraints>(
                ref<TypeParamConstraintClause_>)) +
            (assign<&ast::MethodDecl::body>(ref<Block_>) |
             (lit<"=>"> +
              option(flag<&ast::MethodDecl::isRefBody>(kw<"ref">)) +
              assign<&ast::MethodDecl::exprBody>(ref<Expression_>) + lit<";">) |
             lit<";">))> {};

  static constexpr auto OverloadableUnaryOp = lit<"+"> | lit<"-"> |
                                               lit<"!"> | lit<"~"> |
                                               lit<"++"> | lit<"--"> |
                                               kw<"true"> | kw<"false">;
  struct OverloadableRightShift_ : rule<(Gt + Gt + Gt) | (Gt + Gt)> {};
  static constexpr auto OverloadableBinaryOp =
      lit<"+"> | lit<"-"> | lit<"*"> | lit<"/"> | lit<"%"> | lit<"&"> |
      lit<"|"> | lit<"^"> | lit<"<<"> | ref<OverloadableRightShift_> |
      lit<"=="> | lit<"!="> | Gt | Lt | lit<">="> | lit<"<=">;

  struct OperatorDecl_
      : rule<create<ast::OperatorDecl>(
            attributes<&ast::OperatorDecl::attributes> +
            many(append<&ast::OperatorDecl::modifiers>(ModifierKw)) +
            ((assign<&ast::OperatorDecl::returnType>(ref<TypeNode>) +
              kw<"operator"> +
              option(flag<&ast::OperatorDecl::isChecked>(kw<"checked">)) +
              assign<&ast::OperatorDecl::op>(OverloadableBinaryOp |
                  OverloadableUnaryOp | CompoundAssignOp |
                  ref<RightShiftAssignOp_>)) |
             (assign<&ast::OperatorDecl::opKind>(kw<"implicit"> |
                                                 kw<"explicit">) +
              kw<"operator"> +
              option(flag<&ast::OperatorDecl::isChecked>(kw<"checked">)) +
              assign<&ast::OperatorDecl::returnType>(ref<TypeNode>))) +
            lit<"("> +
            option(some(append<&ast::OperatorDecl::params>(ref<Param_>),
                        lit<",">)) +
            lit<")"> +
            (assign<&ast::OperatorDecl::body>(ref<Block_>) |
             (lit<"=>"> +
              assign<&ast::OperatorDecl::exprBody>(ref<Expression_>) +
              lit<";">) |
             lit<";">))> {};

  struct ConstructorDecl_
      : rule<create<ast::ConstructorDecl>(
            attributes<&ast::ConstructorDecl::attributes> +
            many(append<&ast::ConstructorDecl::modifiers>(ModifierKw)) +
            assign<&ast::ConstructorDecl::name>(ref<Ident>) + lit<"("> +
            option(some(append<&ast::ConstructorDecl::params>(ref<Param_>),
                        lit<",">)) +
            lit<")"> +
            option(lit<":"> +
                   assign<&ast::ConstructorDecl::initializerKind>(kw<"base"> |
                                                                  kw<"this">) +
                   lit<"("> +
                   option(some(append<&ast::ConstructorDecl::initializerArgs>(
                                   ref<Argument_>),
                               lit<",">)) +
                   lit<")">) +
            (assign<&ast::ConstructorDecl::body>(ref<Block_>) |
             (lit<"=>"> +
              assign<&ast::ConstructorDecl::exprBody>(ref<Expression_>) +
              lit<";">) |
             lit<";">))> {};

  struct StaticConstructorDecl_
      : rule<create<ast::StaticConstructorDecl>(
            attributes<&ast::StaticConstructorDecl::attributes> +
            some(append<&ast::StaticConstructorDecl::modifiers>(
                kw<"static"> | kw<"extern"> | kw<"unsafe">)) +
            assign<&ast::StaticConstructorDecl::name>(ref<Ident>) + lit<"("> +
            lit<")"> +
            (assign<&ast::StaticConstructorDecl::body>(ref<Block_>) |
             (lit<"=>"> +
              assign<&ast::StaticConstructorDecl::exprBody>(ref<Expression_>) +
              lit<";">) |
             lit<";">))> {};

  struct FinalizerDecl_
      : rule<create<ast::FinalizerDecl>(
            attributes<&ast::FinalizerDecl::attributes> +
            many(append<&ast::FinalizerDecl::modifiers>(kw<"extern"> |
                                                        kw<"unsafe">)) +
            lit<"~"> + assign<&ast::FinalizerDecl::name>(ref<Ident>) +
            lit<"("> + lit<")"> +
            (assign<&ast::FinalizerDecl::body>(ref<Block_>) |
             (lit<"=>"> +
              assign<&ast::FinalizerDecl::exprBody>(ref<Expression_>) +
              lit<";">) |
             lit<";">))> {};

  struct FixedBufferDeclarator_
      : rule<create<ast::Declarator>(
            assign<&ast::Declarator::name>(ref<Ident>) + lit<"["> +
            assign<&ast::Declarator::value>(ref<Expression_>) +
            lit<"]">)> {};

  struct FixedSizeBufferDecl_
      : rule<create<ast::FixedSizeBufferDecl>(
            attributes<&ast::FixedSizeBufferDecl::attributes> +
            many(append<&ast::FixedSizeBufferDecl::modifiers>(ModifierKw)) +
            kw<"fixed"> +
            assign<&ast::FixedSizeBufferDecl::elementType>(ref<TypeNode>) +
            some(append<&ast::FixedSizeBufferDecl::declarators>(
                     ref<FixedBufferDeclarator_>),
                 lit<",">) +
            lit<";">)> {};

  struct ClassDecl_
      : rule<create<ast::ClassDecl>(
            attributes<&ast::ClassDecl::attributes> +
            many(append<&ast::ClassDecl::modifiers>(ModifierKw)) +
            (kw<"class"> | (flag<&ast::ClassDecl::isRecord>(kw<"record">) +
                            option(kw<"class">))) +
            assign<&ast::ClassDecl::name>(ref<Ident>) +
            option(Lt +
                   some(append<&ast::ClassDecl::typeParams>(ref<TypeParam_>),
                        lit<",">) +
                   Gt) +
            option(
                lit<"("> +
                option(some(append<&ast::ClassDecl::primaryParams>(ref<Param_>),
                            lit<",">)) +
                lit<")">) +
            option(
                lit<":"> +
                some(append<&ast::ClassDecl::bases>(ref<TypeNode>) +
                         option(lit<"("> +
                                option(some(append<&ast::ClassDecl::baseArgs>(
                                                ref<Argument_>),
                                            lit<",">)) +
                                lit<")">),
                     lit<",">)) +
            many(append<&ast::ClassDecl::constraints>(
                ref<TypeParamConstraintClause_>)) +
            ((lit<"{"> + many(append<&ast::ClassDecl::members>(ref<Member_>)) +
              lit<"}"> + option(lit<";">)) |
             lit<";">))> {};

  struct StructDecl_
      : rule<create<ast::StructDecl>(
            attributes<&ast::StructDecl::attributes> +
            many(append<&ast::StructDecl::modifiers>(ModifierKw)) +
            option(append<&ast::StructDecl::modifiers>(kw<"ref">)) +
            (kw<"struct"> |
             (flag<&ast::StructDecl::isRecord>(kw<"record">) +
              kw<"struct">)) +
            assign<&ast::StructDecl::name>(ref<Ident>) +
            option(Lt +
                   some(append<&ast::StructDecl::typeParams>(ref<TypeParam_>),
                        lit<",">) +
                   Gt) +
            option(lit<"("> +
                   option(some(
                       append<&ast::StructDecl::primaryParams>(ref<Param_>),
                       lit<",">)) +
                   lit<")">) +
            option(
                lit<":"> +
                some(append<&ast::StructDecl::bases>(ref<TypeNode>) +
                         option(lit<"("> +
                                option(some(append<&ast::StructDecl::baseArgs>(
                                                ref<Argument_>),
                                            lit<",">)) +
                                lit<")">),
                     lit<",">)) +
            many(append<&ast::StructDecl::constraints>(
                ref<TypeParamConstraintClause_>)) +
            ((lit<"{"> + many(append<&ast::StructDecl::members>(ref<Member_>)) +
              lit<"}"> + option(lit<";">)) |
             lit<";">))> {};

  struct InterfaceDecl_
      : rule<create<ast::InterfaceDecl>(
            attributes<&ast::InterfaceDecl::attributes> +
            many(append<&ast::InterfaceDecl::modifiers>(ModifierKw)) +
            kw<"interface"> + assign<&ast::InterfaceDecl::name>(ref<Ident>) +
            option(
                Lt +
                some(append<&ast::InterfaceDecl::typeParams>(ref<TypeParam_>),
                     lit<",">) +
                Gt) +
            option(lit<":"> +
                   some(append<&ast::InterfaceDecl::bases>(ref<TypeNode>),
                        lit<",">)) +
            many(append<&ast::InterfaceDecl::constraints>(
                ref<TypeParamConstraintClause_>)) +
            lit<"{"> +
            many(append<&ast::InterfaceDecl::members>(ref<Member_>)) +
            lit<"}"> + option(lit<";">))> {};

  struct EnumMember_ : rule<create<ast::EnumMember>(
                           attributes<&ast::EnumMember::attributes> +
                           assign<&ast::EnumMember::name>(ref<Ident>) +
                           option(lit<"="> + assign<&ast::EnumMember::value>(
                                                 ref<Expression_>)))> {};

  struct EnumDecl_
      : rule<create<ast::EnumDecl>(
            attributes<&ast::EnumDecl::attributes> +
            many(append<&ast::EnumDecl::modifiers>(ModifierKw)) + kw<"enum"> +
            assign<&ast::EnumDecl::name>(ref<Ident>) +
            option(lit<":"> + assign<&ast::EnumDecl::baseType>(ref<TypeNode>)) +
            lit<"{"> +
            option(some(append<&ast::EnumDecl::members>(ref<EnumMember_>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}"> + option(lit<";">))> {};

  struct DelegateDecl_
      : rule<create<ast::DelegateDecl>(
            attributes<&ast::DelegateDecl::attributes> +
            many(append<&ast::DelegateDecl::modifiers>(ModifierKw)) +
            kw<"delegate"> +
            option(assign<&ast::DelegateDecl::refKind>(ref<RefKind_>)) +
            (flag<&ast::DelegateDecl::isVoid>(kw<"void">) |
             assign<&ast::DelegateDecl::returnType>(ref<TypeNode>)) +
            assign<&ast::DelegateDecl::name>(ref<Ident>) +
            option(Lt +
                   some(append<&ast::DelegateDecl::typeParams>(ref<TypeParam_>),
                        lit<",">) +
                   Gt) +
            lit<"("> +
            option(some(append<&ast::DelegateDecl::params>(ref<Param_>),
                        lit<",">)) +
            lit<")"> +
            many(append<&ast::DelegateDecl::constraints>(
                ref<TypeParamConstraintClause_>)) +
            lit<";">)> {};

  struct ExternAlias_
      : rule<create<ast::ExternAlias>(
            kw<"extern"> + kw<"alias"> +
            assign<&ast::ExternAlias::name>(ref<Ident>) + lit<";">)> {};

  struct UsingDirective_
      : rule<create<ast::UsingDirective>(
            option(flag<&ast::UsingDirective::isGlobal>(kw<"global">)) +
            kw<"using"> +
            option(assign<&ast::UsingDirective::kind>(kw<"static">)) +
            option(flag<&ast::UsingDirective::isUnsafe>(kw<"unsafe">)) +
            option(assign<&ast::UsingDirective::alias>(ref<Ident>) + lit<"=">) +
            assign<&ast::UsingDirective::target>(ref<TypeNode>) + lit<";">)> {};

  struct NamespaceDecl_
      : rule<create<ast::NamespaceDecl>(
            kw<"namespace"> + append<&ast::NamespaceDecl::path>(ref<Ident>) +
            many(lit<"."> + append<&ast::NamespaceDecl::path>(ref<Ident>)) +
            ((lit<"{"> +
              many(append<&ast::NamespaceDecl::externAliases>(
                  ref<ExternAlias_>)) +
              many(append<&ast::NamespaceDecl::usings>(ref<UsingDirective_>)) +
              many(append<&ast::NamespaceDecl::members>(ref<Member_>)) +
              lit<"}"> + option(lit<";">)) |
             (flag<&ast::NamespaceDecl::fileScoped>(lit<";">) +
              many(append<&ast::NamespaceDecl::externAliases>(
                  ref<ExternAlias_>)) +
              many(append<&ast::NamespaceDecl::usings>(ref<UsingDirective_>)) +
              many(append<&ast::NamespaceDecl::members>(ref<Member_>)))))> {};

  struct Member_
      : rule<ref<NamespaceDecl_> | ref<ClassDecl_> | ref<StructDecl_> |
             ref<InterfaceDecl_> | ref<EnumDecl_> | ref<DelegateDecl_> |
             ref<ConstDecl_> | ref<FixedSizeBufferDecl_> |
             ref<EventDecl_> | ref<IndexerDecl_> | ref<OperatorDecl_> |
             ref<StaticConstructorDecl_> | ref<FinalizerDecl_> |
             ref<ConstructorDecl_> | ref<PropertyDecl_> |
             ref<MethodDecl_> | ref<FieldDecl_>> {};

  // -- Compilation unit ----
  struct CompilationUnit_
      : rule<create<ast::CompilationUnit>(
            many(append<&ast::CompilationUnit::externAliases>(
                ref<ExternAlias_>)) +
            many(append<&ast::CompilationUnit::usings>(ref<UsingDirective_>)) +
            many(
                lit<"["> +
                append<&ast::CompilationUnit::globalAttributes>(
                    ref<GlobalAttribute_>) +
                many(lit<","> + append<&ast::CompilationUnit::globalAttributes>(
                                    ref<Attribute_>)) +
                option(lit<",">) + lit<"]">) +
            many(append<&ast::CompilationUnit::members>(ref<Member_>) |
                 append<&ast::CompilationUnit::statements>(ref<Statement_>)))> {
  };

  using entry = CompilationUnit_;
  using ignore = list<WS, DIRECTIVE>;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace csharp
