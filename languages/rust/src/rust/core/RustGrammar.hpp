// The rust language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rust/core/ast.hpp"

namespace rust {

struct Rust : cursiv::GrammarSpec {
  // Trivia.
  struct WS : terminal<some(cr<R"( \t\n\r\v\f)"> | lit<"\u0085"> |
                            lit<"\u200e"> | lit<"\u200f"> | lit<"\u2028"> |
                            lit<"\u2029">)> {};
  struct LINE_COMMENT
      : terminal<(lit<"//"> + (cr<R"(^/!\n)"> | lit<"//">) + many(cr<R"(^\n)">)) |
                 lit<"//">> {};
  // Matches `/*!`/`/**` too: `!`/`*` right after the opener are ordinary
  // content to the counter, which only tracks `/*` against `*/`.
  struct BLOCK_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};
  struct INNER_LINE_DOC : terminal<lit<"//!"> + many(cr<R"(^\n)">)> {};
  struct OUTER_LINE_DOC
      : terminal<lit<"///"> + option(cr<R"(^/\n)"> + many(cr<R"(^\n)">))> {};
  static constexpr auto ShebangTrivia = many(
      ref<WS> | ref<LINE_COMMENT> |
      (!(lit<"/*!"> | (lit<"/**"> + !cr<"*/">)) + ref<BLOCK_COMMENT>));
  struct SHEBANG
      : terminal<lit<"#!"> + !(ShebangTrivia + lit<"[">) +
                 many(cr<R"(^\n)">)> {};

  static constexpr auto IdentBody =
      (uc::XID_Start + many(uc::XID_Continue)) |
      (lit<"_"> + some(uc::XID_Continue));
  struct NON_KEYWORD_IDENTIFIER : terminal<IdentBody> {};
  struct RAW_IDENTIFIER : terminal<lit<"r#"> + IdentBody> {};
  // `macro_rules` is a weak keyword: spelled once, read both as itself and
  // as a plain identifier -- what follows decides.
  static constexpr auto Identifier =
      ref<NON_KEYWORD_IDENTIFIER> | ref<RAW_IDENTIFIER> | kw<"macro_rules">;
  struct LIFETIME_OR_LABEL : terminal<lit<"'"> + IdentBody> {};

  // Literals.
  struct LITERAL_SUFFIX
      : terminal<(uc::XID_Start | lit<"_">) + many(uc::XID_Continue)> {};
  static constexpr auto SUFFIX = ref<LITERAL_SUFFIX>;
  static constexpr auto HEX_DIGIT = cr<"0-9a-fA-F">;
  static constexpr auto QUOTE_ESCAPE = lit<"\\"> + cr<R"('")">;
  static constexpr auto ASCII_ESCAPE =
      (lit<"\\x"> + cr<"0-7"> + HEX_DIGIT) | (lit<"\\"> + cr<R"(nrt\\0)">);
  static constexpr auto UNICODE_ESCAPE =
      lit<"\\u{"> + some(HEX_DIGIT | lit<"_">) + lit<"}">;
  static constexpr auto ESC_NEWLINE = lit<"\\"> + option(lit<"\r">) + lit<"\n">;
  struct CHAR_LIT
      : terminal<lit<"'"> +
                 (QUOTE_ESCAPE | ASCII_ESCAPE | UNICODE_ESCAPE |
                  ~cr<R"('\\\n\r\t)">) +
                 lit<"'"> + option(SUFFIX)> {};
  struct STRING_LIT
      : terminal<lit<"\""> +
                 many(QUOTE_ESCAPE | ASCII_ESCAPE | UNICODE_ESCAPE |
                      ESC_NEWLINE | cr<R"(^"\\)">) +
                 lit<"\""> + option(SUFFIX)> {};
  struct RawTag {};
  struct RAW_STRING_LIT
      : terminal<through(lit<"r"> + mark<RawTag>(many(cr<"#">)) + lit<"\"">, lit<"\""> + same<RawTag>) + option(SUFFIX)> {};
  static constexpr auto BYTE_ESCAPE =
      (lit<"\\x"> + HEX_DIGIT + HEX_DIGIT) | (lit<"\\"> + cr<R"(nrt\\0'")">);
  struct BYTE_LIT
      : terminal<lit<"b'"> +
                 (QUOTE_ESCAPE | BYTE_ESCAPE | cr<R"(^'\\)">) +
                 lit<"'"> + option(SUFFIX)> {};
  struct BYTE_STRING_LIT
      : terminal<lit<"b\""> +
                 many(QUOTE_ESCAPE | BYTE_ESCAPE | ESC_NEWLINE | cr<R"(^"\\)">) +
                 lit<"\""> + option(SUFFIX)> {};
  struct RawByteTag {};
  struct RAW_BYTE_STRING_LIT
      : terminal<through(lit<"br"> + mark<RawByteTag>(many(cr<"#">)) +
                   lit<"\"">, lit<"\""> + same<RawByteTag>) + option(SUFFIX)> {};
  struct C_STRING_LIT
      : terminal<lit<"c\""> +
                 many(QUOTE_ESCAPE | BYTE_ESCAPE | UNICODE_ESCAPE | ESC_NEWLINE |
                      cr<R"(^"\\)">) +
                 lit<"\""> + option(SUFFIX)> {};
  struct RAW_C_STRING_LIT
      : terminal<through(lit<"cr"> + mark<RawTag>(many(cr<"#">)) + lit<"\"">, lit<"\""> + same<RawTag>) + option(SUFFIX)> {};

  static constexpr auto DEC_LITERAL = d + many(d | lit<"_">);
  struct INTEGER_LIT
      : terminal<((lit<"0x"> + many(lit<"_">) + HEX_DIGIT +
                   many(HEX_DIGIT | lit<"_">)) |
                  (lit<"0o"> + many(lit<"_">) + cr<"0-7"> + many(cr<"0-7_">)) |
                  (lit<"0b"> + many(lit<"_">) + cr<"01"> + many(cr<"01_">)) |
                  DEC_LITERAL) +
                 option(SUFFIX)> {};
  static constexpr auto FLOAT_EXPONENT =
      cr<"eE"> + option(cr<"+-">) + many(lit<"_">) + DEC_LITERAL;
  // A trailing dot belongs to the float only before neither a name nor a dot.
  struct FLOAT_LIT
      : terminal<DEC_LITERAL +
                 (lit<".", !(cr<"._"> | uc::XID_Start)> |
                  (lit<"."> + DEC_LITERAL + option(FLOAT_EXPONENT) +
                   option(SUFFIX)) |
                  (FLOAT_EXPONENT + option(SUFFIX)) | SUFFIX)> {};

  // Punctuation. Lt/Gt stay single-char, guarded against `=`: nested
  // generics close on two single `>`, so `<<`/`>>` are never their own
  // token -- shift is a hand-rolled pair below (ShlOp/ShrOp).
  struct LT : terminal<lit<"<", !cr<"=">>, value<ast::Operator::lt>> {};
  struct GT : terminal<lit<">", !cr<"=">>, value<ast::Operator::gt>> {};
  struct LE : terminal<lit<"<=">, value<ast::Operator::le>> {};
  struct GE : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct EQEQ : terminal<lit<"==">, value<ast::Operator::eq>> {};
  struct NE : terminal<lit<"!=">, value<ast::Operator::ne>> {};
  struct STAR : terminal<lit<"*", !cr<"=">>, value<ast::Operator::multiply>> {
  };
  struct SLASH
      : terminal<lit<"/", !cr<"=/*">>, value<ast::Operator::divide>> {};
  struct PERCENT
      : terminal<lit<"%", !cr<"=">>, value<ast::Operator::remainder>> {};
  struct PLUS : terminal<lit<"+", !cr<"=">>, value<ast::Operator::add>> {};
  struct MINUS
      : terminal<lit<"-", !cr<"=>">>, value<ast::Operator::subtract>> {};
  struct AND : terminal<lit<"&", !cr<"&=">>, value<ast::Operator::bit_and>> {
  };
  struct CARET : terminal<lit<"^", !cr<"=">>, value<ast::Operator::bit_xor>> {
  };
  struct OR : terminal<lit<"|", !cr<"|=">>, value<ast::Operator::bit_or>> {};
  struct ANDAND : terminal<lit<"&&">, value<ast::Operator::and_and>> {};
  struct OROR : terminal<lit<"||">, value<ast::Operator::or_or>> {};
  struct EQ : terminal<lit<"=", !cr<"=>">>, value<ast::Operator::assign>> {};
  struct PLUSEQ : terminal<lit<"+=">, value<ast::Operator::add_assign>> {};
  struct MINUSEQ : terminal<lit<"-=">, value<ast::Operator::sub_assign>> {};
  struct STAREQ : terminal<lit<"*=">, value<ast::Operator::mul_assign>> {};
  struct SLASHEQ : terminal<lit<"/=">, value<ast::Operator::div_assign>> {};
  struct PERCENTEQ : terminal<lit<"%=">, value<ast::Operator::rem_assign>> {};
  struct CARETEQ : terminal<lit<"^=">, value<ast::Operator::bitxor_assign>> {
  };
  struct ANDEQ : terminal<lit<"&=">, value<ast::Operator::bitand_assign>> {};
  struct OREQ : terminal<lit<"|=">, value<ast::Operator::bitor_assign>> {};
  struct SHLEQ : terminal<lit<"<<=">, value<ast::Operator::shl_assign>> {};
  struct SHREQ : terminal<lit<">>=">, value<ast::Operator::shr_assign>> {};
  // Two adjacent Lt/Gt: grammar context (expression vs. generic argument
  // list) tells shift and closing generics apart, no adjacency check needed.
  struct ShlOp : rule<ref<LT> + ref<LT>, value<ast::Operator::shl>> {};
  struct ShrOp : rule<ref<GT> + ref<GT>, value<ast::Operator::shr>> {};

  struct DOT : terminal<lit<".", !cr<".">>> {};
  struct DOTDOT : terminal<lit<"..", !cr<".=">>> {};
  struct DOTDOTDOT : terminal<lit<"...">> {};
  struct DOTDOTEQ : terminal<lit<"..=">> {};
  struct PATHSEP : terminal<lit<"::">> {};
  struct RARROW : terminal<lit<"->">> {};
  struct FATARROW : terminal<lit<"=>">> {};
  struct QUESTIONMARK : terminal<lit<"?">> {};
  struct ATSIGN : terminal<lit<"@">> {};
  // Unguarded: NON_KEYWORD_IDENTIFIER's `_`-prefixed branch needs a
  // continuation char, so it always outlengths a bare `_` where it applies.
  struct UNDERSCORE : terminal<lit<"_">> {};

  static constexpr auto AnyKeyword =
      kw<"as"> | kw<"break"> | kw<"const"> | kw<"continue"> | kw<"crate"> |
      kw<"else"> | kw<"enum"> | kw<"extern"> | kw<"false"> | kw<"fn"> |
      kw<"for"> | kw<"if"> | kw<"impl"> | kw<"in"> | kw<"let"> | kw<"loop"> |
      kw<"match"> | kw<"mod"> | kw<"move"> | kw<"mut"> | kw<"pub"> |
      kw<"ref"> | kw<"return"> | kw<"self"> | kw<"Self"> | kw<"static"> |
      kw<"struct"> | kw<"super"> | kw<"trait"> | kw<"true"> | kw<"type"> |
      kw<"unsafe"> | kw<"use"> | kw<"where"> | kw<"while"> | kw<"async"> |
      kw<"await"> | kw<"dyn"> | kw<"abstract"> | kw<"become"> | kw<"box"> |
      kw<"do"> | kw<"final"> | kw<"macro"> | kw<"override"> | kw<"priv"> |
      kw<"typeof"> | kw<"unsized"> | kw<"virtual"> | kw<"yield"> | kw<"try"> |
      kw<"union">;
  static constexpr auto AnyPunct =
      ref<LT> | ref<GT> | ref<LE> | ref<GE> | ref<EQEQ> | ref<NE> |
      ref<STAR> | ref<SLASH> | ref<PERCENT> | ref<PLUS> | ref<MINUS> |
      ref<AND> | ref<CARET> | ref<OR> | ref<ANDAND> | ref<OROR> | ref<EQ> |
      ref<PLUSEQ> | ref<MINUSEQ> | ref<STAREQ> | ref<SLASHEQ> |
      ref<PERCENTEQ> | ref<CARETEQ> | ref<ANDEQ> | ref<OREQ> | ref<SHLEQ> |
      ref<SHREQ> | ref<DOT> | ref<DOTDOT> | ref<DOTDOTDOT> | ref<DOTDOTEQ> |
      lit<","> | lit<";"> | lit<":"> | ref<PATHSEP> | ref<RARROW> |
      ref<FATARROW> | ref<QUESTIONMARK> | ref<ATSIGN> | lit<"#"> |
      lit<"!", !cr<"=">> | ref<UNDERSCORE>;
  static constexpr auto MacroToken =
      AnyKeyword | Identifier | lit<"'_"> | lit<"$crate"> |
      ref<LIFETIME_OR_LABEL> | ref<CHAR_LIT> | ref<STRING_LIT> |
      ref<RAW_STRING_LIT> | ref<BYTE_LIT> | ref<BYTE_STRING_LIT> |
      ref<RAW_BYTE_STRING_LIT> | ref<C_STRING_LIT> | ref<RAW_C_STRING_LIT> |
      ref<INTEGER_LIT> | ref<FLOAT_LIT> |
      AnyPunct | lit<"$">;

  // A token tree preserves every leaf and nested delimiter. Macro expansion
  // and the matcher/transcriber split remain later semantic operations.
  static constexpr auto raw_macro_token = [](std::string_view text) {
    return text;
  };
  struct MacroTokenRule : rule<MacroToken, text_conv<raw_macro_token>> {};
  struct TokenTree;
  struct DelimTokenTree
      : rule<create<ast::TokenTree>(
            (assign<&ast::TokenTree::open>(lit<"(">) +
             many(append<&ast::TokenTree::children>(ref<TokenTree>)) +
             lit<")">) |
            (assign<&ast::TokenTree::open>(lit<"[">) +
             many(append<&ast::TokenTree::children>(ref<TokenTree>)) +
             lit<"]">) |
            (assign<&ast::TokenTree::open>(lit<"{">) +
             many(append<&ast::TokenTree::children>(ref<TokenTree>)) +
             lit<"}">))> {};
  struct TokenTree : rule<create<ast::TokenTree>(assign<&ast::TokenTree::token>(
                              ref<MacroTokenRule>)) |
                          ref<DelimTokenTree>> {};

  // Forward declarations: the grammar is heavily mutually recursive.
  struct Expression;
  struct ExpressionWithBlock;
  struct BlockExprRule;
  struct Statement;
  struct IfExprRule;
  struct IfLetExprRule;
  struct TypeRule;
  struct TypeNoBoundsRule;
  struct PatternRule;
  struct PatternNoTopAltRule;
  struct LiteralExprRule;
  struct ItemRule;

  static constexpr auto Lifetime =
      ref<LIFETIME_OR_LABEL> | lit<"'static"> | lit<"'_">;

  // Paths and generics.
  struct SimplePathSegment
      : rule<assign<&ast::PathSegment::name>(
            Identifier | kw<"super"> | kw<"self"> | kw<"crate"> |
            lit<"$crate">)> {};
  struct SimplePathRule
      : rule<option(flag<&ast::Path::leadingSep>(ref<PATHSEP>)) +
             append<&ast::Path::segments>(ref<SimplePathSegment>) +
             many(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<SimplePathSegment>))> {};

  static constexpr auto PathIdentSegmentName =
      Identifier | kw<"super"> | kw<"self"> | kw<"Self"> | kw<"crate"> |
      lit<"$crate">;

  struct GenericArgRule;
  struct GenericArgsRule
      : rule<create<ast::GenericArgs>(
            lit<"<"> +
            option(append<&ast::GenericArgs::args>(ref<GenericArgRule>) +
                   many(lit<","> +
                        append<&ast::GenericArgs::args>(ref<GenericArgRule>)) +
                   option(lit<",">)) +
            ref<GT>)> {};

  struct PathExprSegment
      : rule<assign<&ast::PathSegment::name>(PathIdentSegmentName) +
             option(ref<PATHSEP> +
                    assign<&ast::PathSegment::args>(ref<GenericArgsRule>))> {
  };
  struct PathInExpressionRule
      : rule<option(flag<&ast::Path::leadingSep>(ref<PATHSEP>)) +
             append<&ast::Path::segments>(ref<PathExprSegment>) +
             many(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<PathExprSegment>))> {};

  // A path-fn-sugar tail (`Fn(A, B) -> C`) writes into the SAME PathSegment
  // this rule builds, so it is inlined rather than a rule of its own: a
  // separate rule would reduce to its own node, out of reach of this one.
  struct TypePathSegment
      : rule<assign<&ast::PathSegment::name>(PathIdentSegmentName) +
             option(ref<PATHSEP>) +
             option(assign<&ast::PathSegment::args>(ref<GenericArgsRule>) |
                    (lit<"("> +
                     option(append<&ast::PathSegment::fnSugarParams>(
                                ref<TypeRule>) +
                            many(lit<","> +
                                 append<&ast::PathSegment::fnSugarParams>(
                                     ref<TypeRule>)) +
                            option(lit<",">)) +
                     lit<")"> +
                     option(ref<RARROW> +
                            assign<&ast::PathSegment::fnSugarReturn>(
                                ref<TypeRule>))))> {};
  struct TypePathRule
      : rule<option(flag<&ast::Path::leadingSep>(ref<PATHSEP>)) +
             append<&ast::Path::segments>(ref<TypePathSegment>) +
             many(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<TypePathSegment>))> {};

  struct QualifiedPathInExpressionRule
      : rule<lit<"<"> + assign<&ast::Path::qualifierType>(ref<TypeRule>) +
             option(kw<"as"> +
                    assign<&ast::Path::qualifierTrait>(ref<TypePathRule>)) +
             ref<GT> +
             some(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<PathExprSegment>))> {};
  struct QualifiedPathInTypeRule
      : rule<lit<"<"> + assign<&ast::Path::qualifierType>(ref<TypeRule>) +
             option(kw<"as"> +
                    assign<&ast::Path::qualifierTrait>(ref<TypePathRule>)) +
             ref<GT> +
             some(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<TypePathSegment>))> {};

  struct GenericArgRule
      : rule<create<ast::BindingArg>(
                 assign<&ast::BindingArg::name>(Identifier) + ref<EQ> +
                 assign<&ast::BindingArg::type>(ref<TypeRule>))  //
             | create<ast::LifetimeArg>(
                   assign<&ast::LifetimeArg::name>(Lifetime))  //
             | create<ast::ConstArg>(
                   assign<&ast::ConstArg::value>(ref<Expression>))  //
             | create<ast::TypeArg>(
                   assign<&ast::TypeArg::type>(ref<TypeRule>))> {};

  // Attributes and visibility. `literalArg` is text: the `= literal` form
  // reads the literal as bare terminals, not `ref<LiteralExprRule>`, which
  // builds a node.
  static constexpr auto AttrLiteral =
      kw<"true"> | kw<"false"> | ref<CHAR_LIT> | ref<STRING_LIT> |
      ref<RAW_STRING_LIT> | ref<BYTE_LIT> | ref<BYTE_STRING_LIT> |
      ref<RAW_BYTE_STRING_LIT> | ref<C_STRING_LIT> | ref<RAW_C_STRING_LIT> |
      ref<INTEGER_LIT> | ref<FLOAT_LIT>;
  struct OuterAttribute
      : rule<create<ast::Attribute>(
            lit<"#"> + lit<"["> +
            assign<&ast::Attribute::path>(ref<SimplePathRule>) +
            option(
                assign<&ast::Attribute::tokens>(ref<DelimTokenTree>) |
                (ref<EQ> + assign<&ast::Attribute::literalArg>(AttrLiteral))) +
            lit<"]">)> {};
  struct InnerAttribute
      : rule<create<ast::Attribute>(
            lit<"#"> + lit<"!"> + lit<"["> +
            assign<&ast::Attribute::path>(ref<SimplePathRule>) +
            option(
                assign<&ast::Attribute::tokens>(ref<DelimTokenTree>) |
                (ref<EQ> + assign<&ast::Attribute::literalArg>(AttrLiteral))) +
            lit<"]">)> {};

  struct VisibilityRule
      : rule<kw<"pub"> +
             option(lit<"("> +
                    (assign<&ast::Visibility::kind>(kw<"crate"> |
                                                     kw<"self"> |
                                                     kw<"super">) |
                     (assign<&ast::Visibility::kind>(kw<"in">) +
                      assign<&ast::Visibility::inPath>(
                          ref<SimplePathRule>))) +
                    lit<")">)> {};

  // Trait bounds sit before generic params/where clauses, which reuse them.
  struct GenericParamsRule;
  struct TraitBoundRule
      : rule<create<ast::TraitBound>(
            (lit<"("> +
             option(flag<&ast::TraitBound::maybe>(lit<"?">)) +
             option(kw<"for"> + assign<&ast::TraitBound::forLifetimes>(
                                     ref<GenericParamsRule>)) +
             assign<&ast::TraitBound::path>(ref<TypePathRule>) + lit<")">)  //
            | (option(flag<&ast::TraitBound::maybe>(lit<"?">)) +
               option(kw<"for"> + assign<&ast::TraitBound::forLifetimes>(
                                       ref<GenericParamsRule>)) +
               assign<&ast::TraitBound::path>(ref<TypePathRule>)))> {};
  static constexpr auto TraitBoundOrLifetime = ref<TraitBoundRule> | Lifetime;

  // Generic parameters and where clauses. `genericParam`'s own leading
  // outerAttribute* also covers each leaf's per-alternative one.
  static constexpr auto LifetimeParamBody =
      assign<&ast::LifetimeParam::name>(ref<LIFETIME_OR_LABEL>) +
      option(lit<":"> +
             option(append<&ast::LifetimeParam::bounds>(Lifetime) +
                    many(lit<"+"> +
                         append<&ast::LifetimeParam::bounds>(Lifetime))));
  static constexpr auto TypeParamBody =
      assign<&ast::TypeParam::name>(Identifier) +
      option(lit<":"> +
             option(append<&ast::TypeParam::bounds>(TraitBoundOrLifetime) +
                    many(lit<"+"> + append<&ast::TypeParam::bounds>(
                                        TraitBoundOrLifetime)))) +
      option(ref<EQ> + assign<&ast::TypeParam::defaultType>(ref<TypeRule>));
  static constexpr auto ConstParamBody =
      kw<"const"> + assign<&ast::ConstParam::name>(Identifier) + lit<":"> +
      assign<&ast::ConstParam::type>(ref<TypeRule>) +
      option(ref<EQ> + assign<&ast::ConstParam::defaultValue>(ref<Expression>));
  struct GenericParamRule
      : rule<many(append<&ast::GenericParam::attrs>(ref<OuterAttribute>)) +
             (create<ast::LifetimeParam>(LifetimeParamBody) |
              create<ast::TypeParam>(TypeParamBody) |
              create<ast::ConstParam>(ConstParamBody))> {};
  struct GenericParamsRule
      : rule<create<ast::GenericParams>(
            lit<"<"> +
            option(append<&ast::GenericParams::params>(
                       ref<GenericParamRule>) +
                   many(lit<","> + append<&ast::GenericParams::params>(
                                       ref<GenericParamRule>)) +
                   option(lit<",">)) +
            ref<GT>)> {};

  struct LifetimeWhereItemRule
      : rule<assign<&ast::LifetimeWhereItem::lifetime>(Lifetime) + lit<":"> +
             option(append<&ast::LifetimeWhereItem::bounds>(Lifetime) +
                    many(lit<"+"> + append<&ast::LifetimeWhereItem::bounds>(
                                        Lifetime)))> {};
  struct TypeBoundWhereItemRule
      : rule<option(kw<"for"> +
                    assign<&ast::TypeBoundWhereItem::forLifetimes>(
                        ref<GenericParamsRule>)) +
             assign<&ast::TypeBoundWhereItem::type>(ref<TypeRule>) +
             lit<":"> +
             option(append<&ast::TypeBoundWhereItem::bounds>(
                        TraitBoundOrLifetime) +
                    many(lit<"+"> +
                         append<&ast::TypeBoundWhereItem::bounds>(
                             TraitBoundOrLifetime)))> {};
  struct WhereClauseItemRule
      : rule<ref<TypeBoundWhereItemRule> | ref<LifetimeWhereItemRule>> {};
  struct WhereClauseRule
      : rule<create<ast::WhereClause>(
            kw<"where"> +
            many(append<&ast::WhereClause::items>(ref<WhereClauseItemRule>) +
                 lit<",">) +
            option(append<&ast::WhereClause::items>(
                ref<WhereClauseItemRule>)))> {};

  // Types.
  struct ArrayOrSliceTy
      : rule<create<ast::ArrayTy>(
                 lit<"["> + assign<&ast::ArrayTy::element>(ref<TypeRule>) +
                 lit<";"> + assign<&ast::ArrayTy::size>(ref<Expression>) +
                 lit<"]">)  //
             | create<ast::SliceTy>(
                   lit<"["> + assign<&ast::SliceTy::element>(ref<TypeRule>) +
                   lit<"]">)> {};
  struct BareFnTyRule
      : rule<create<ast::BareFnTy>(
            option(kw<"for"> + assign<&ast::BareFnTy::forLifetimes>(
                                    ref<GenericParamsRule>)) +
            option(flag<&ast::BareFnTy::isUnsafe>(kw<"unsafe">)) +
            option(flag<&ast::BareFnTy::isExtern>(kw<"extern">) +
                   option(assign<&ast::BareFnTy::abi>(ref<STRING_LIT>))) +
            kw<"fn"> + lit<"("> +
            option(
                (option((Identifier | ref<UNDERSCORE>) + lit<":">) +
                 append<&ast::BareFnTy::params>(ref<TypeRule>)) +
                many(lit<","> +
                     option((Identifier | ref<UNDERSCORE>) + lit<":">) +
                     append<&ast::BareFnTy::params>(ref<TypeRule>)) +
                option(lit<","> +
                       option(flag<&ast::BareFnTy::variadic>(
                           ref<DOTDOTDOT>)))) +
            lit<")"> +
            option(ref<RARROW> + assign<&ast::BareFnTy::returnType>(
                                      ref<TypeNoBoundsRule>)))> {};
  struct ImplTraitTyRule
      : rule<create<ast::ImplTraitTy>(
            kw<"impl"> +
            append<&ast::ImplTraitTy::bounds>(TraitBoundOrLifetime) +
            many(lit<"+"> +
                 append<&ast::ImplTraitTy::bounds>(TraitBoundOrLifetime)) +
            option(lit<"+">))> {};
  struct ImplTraitTyOneBoundRule
      : rule<create<ast::ImplTraitTy>(
            kw<"impl"> +
            append<&ast::ImplTraitTy::bounds>(ref<TraitBoundRule>))> {};
  struct TraitObjectTyRule
      : rule<create<ast::TraitObjectTy>(
            option(flag<&ast::TraitObjectTy::isDyn>(kw<"dyn">)) +
            append<&ast::TraitObjectTy::bounds>(TraitBoundOrLifetime) +
            many(lit<"+"> + append<&ast::TraitObjectTy::bounds>(
                                 TraitBoundOrLifetime)) +
            option(lit<"+">))> {};
  struct TraitObjectTyOneBoundRule
      : rule<create<ast::TraitObjectTy>(
            option(flag<&ast::TraitObjectTy::isDyn>(kw<"dyn">)) +
            append<&ast::TraitObjectTy::bounds>(ref<TraitBoundRule>))> {};

  struct TypeNoBoundsRule
      : rule<create<ast::ParenTy>(lit<"("> +
                                  assign<&ast::ParenTy::inner>(ref<TypeRule>) +
                                  lit<")">)    //
             | ref<ImplTraitTyOneBoundRule>    //
             | ref<TraitObjectTyOneBoundRule>  //
             | create<ast::PathTy>(
                   assign<&ast::PathTy::path>(ref<QualifiedPathInTypeRule>))  //
             | create<ast::PathTy>(
                   assign<&ast::PathTy::path>(ref<TypePathRule>))  //
             |
             create<ast::TupleTy>(
                 lit<"("> +
                 option(append<&ast::TupleTy::elements>(ref<TypeRule>) +
                        lit<","> +
                        option(append<&ast::TupleTy::elements>(ref<TypeRule>) +
                               many(lit<","> + append<&ast::TupleTy::elements>(
                                                   ref<TypeRule>)) +
                               option(lit<",">))) +
                 lit<")">)                     //
             | create<ast::NeverTy>(lit<"!">)  //
             | create<ast::RawPtrTy>(
                   ref<STAR> +
                   (kw<"const"> | flag<&ast::RawPtrTy::isMut>(kw<"mut">)) +
                   assign<&ast::RawPtrTy::inner>(ref<TypeNoBoundsRule>))  //
             | create<ast::RefTy>(
                   ref<AND> + option(assign<&ast::RefTy::lifetime>(Lifetime)) +
                   option(flag<&ast::RefTy::isMut>(kw<"mut">)) +
                   assign<&ast::RefTy::inner>(ref<TypeNoBoundsRule>))  //
             | ref<ArrayOrSliceTy>                                     //
             | create<ast::InferredTy>(ref<UNDERSCORE>)                //
             | ref<BareFnTyRule>                                       //
             | create<ast::MacroTy>(
                   assign<&ast::MacroTy::path>(ref<SimplePathRule>) + lit<"!"> +
                   assign<&ast::MacroTy::tokens>(ref<DelimTokenTree>))> {};

  struct TypeRule
      : rule<ref<ImplTraitTyRule> | ref<TraitObjectTyRule> |
             ref<TypeNoBoundsRule>> {};

  // Patterns.
  struct RangePatBoundRule
      : rule<create<ast::RangePatBound>(
                 option(flag<&ast::RangePatBound::negative>(lit<"-">)) +
                 assign<&ast::RangePatBound::text>(
                     ref<CHAR_LIT> | ref<BYTE_LIT> | ref<INTEGER_LIT> |
                     ref<FLOAT_LIT>))  //
             | create<ast::RangePatBound>(
                   assign<&ast::RangePatBound::path>(
                       ref<PathInExpressionRule> |
                       ref<QualifiedPathInExpressionRule>))> {};
  struct LiteralPatRule
      : rule<create<ast::LiteralPat>(
            option(flag<&ast::LiteralPat::negative>(lit<"-">)) +
            assign<&ast::LiteralPat::text>(
                kw<"true"> | kw<"false"> | ref<CHAR_LIT> | ref<BYTE_LIT> |
                ref<STRING_LIT> | ref<RAW_STRING_LIT> |
                ref<BYTE_STRING_LIT> | ref<RAW_BYTE_STRING_LIT> |
                ref<C_STRING_LIT> | ref<RAW_C_STRING_LIT> |
                ref<INTEGER_LIT> | ref<FLOAT_LIT>))> {};
  struct IdentPatRule
      : rule<create<ast::IdentPat>(
            option(flag<&ast::IdentPat::isRef>(kw<"ref">)) +
            option(flag<&ast::IdentPat::isMut>(kw<"mut">)) +
            assign<&ast::IdentPat::name>(Identifier) +
            option(ref<ATSIGN> + assign<&ast::IdentPat::subPattern>(
                                      ref<PatternNoTopAltRule>)))> {};
  struct StructPatFieldRule
      : rule<many(append<&ast::StructPatField::attrs>(ref<OuterAttribute>)) +
             ((assign<&ast::StructPatField::name>(ref<INTEGER_LIT>) +
               lit<":"> +
               assign<&ast::StructPatField::pattern>(ref<PatternRule>))  //
              | (assign<&ast::StructPatField::name>(Identifier) + lit<":"> +
                 assign<&ast::StructPatField::pattern>(ref<PatternRule>))  //
              | (option(flag<&ast::StructPatField::isRef>(kw<"ref">)) +
                 option(flag<&ast::StructPatField::isMut>(kw<"mut">)) +
                 assign<&ast::StructPatField::name>(Identifier)))> {};
  struct StructPatRule
      : rule<create<ast::StructPat>(
            assign<&ast::StructPat::path>(ref<PathInExpressionRule>) +
            lit<"{"> +
            option((append<&ast::StructPat::fields>(ref<StructPatFieldRule>) +
                    many(lit<","> + append<&ast::StructPat::fields>(
                                         ref<StructPatFieldRule>)) +
                    option(lit<","> +
                           option(flag<&ast::StructPat::hasRest>(
                               ref<DOTDOT>))))  //
                   | flag<&ast::StructPat::hasRest>(ref<DOTDOT>)) +
            lit<"}">)> {};
  struct TupleStructPatRule
      : rule<create<ast::TupleStructPat>(
            assign<&ast::TupleStructPat::path>(ref<PathInExpressionRule>) +
            lit<"("> +
            option(append<&ast::TupleStructPat::elements>(ref<PatternRule>) +
                   many(lit<","> + append<&ast::TupleStructPat::elements>(
                                       ref<PatternRule>)) +
                   option(lit<",">)) +
            lit<")">)> {};
  struct TuplePatRule
      : rule<create<ast::TuplePat>(
            lit<"("> +
            option(append<&ast::TuplePat::elements>(ref<PatternRule>) +
                   lit<","> +
                   option(append<&ast::TuplePat::elements>(ref<PatternRule>) +
                          many(lit<","> + append<&ast::TuplePat::elements>(
                                              ref<PatternRule>)) +
                          option(lit<",">))) +
            lit<")">)> {};
  struct SlicePatRule
      : rule<create<ast::SlicePat>(
            lit<"["> +
            option(append<&ast::SlicePat::elements>(ref<PatternRule>) +
                   many(lit<","> + append<&ast::SlicePat::elements>(
                                        ref<PatternRule>)) +
                   option(lit<",">)) +
            lit<"]">)> {};

  struct RangePatRule
      : rule<create<ast::RangePat>(
                 flag<&ast::RangePat::inclusive>(ref<DOTDOTEQ>) +
                 assign<&ast::RangePat::end>(ref<RangePatBoundRule>))  //
             | create<ast::RangePat>(
                   assign<&ast::RangePat::start>(ref<RangePatBoundRule>) +
                   ((flag<&ast::RangePat::inclusive>(ref<DOTDOTEQ>) +
                     assign<&ast::RangePat::end>(ref<RangePatBoundRule>))  //
                    | (flag<&ast::RangePat::obsolete>(ref<DOTDOTDOT>) +
                       assign<&ast::RangePat::end>(ref<RangePatBoundRule>))  //
                    | ref<DOTDOT>))> {};

  struct PatternWithoutRangeRule
      : rule<
            ref<LiteralPatRule>                          //
            | create<ast::WildcardPat>(ref<UNDERSCORE>)  //
            | create<ast::RestPat>(ref<DOTDOT>)          //
            | create<ast::RefPat>(
                  flag<&ast::RefPat::doubleRef>(ref<ANDAND>) +
                  option(flag<&ast::RefPat::isMut>(kw<"mut">)) +
                  assign<&ast::RefPat::inner>(ref<PatternWithoutRangeRule>))  //
            | create<ast::RefPat>(
                  ref<AND> + option(flag<&ast::RefPat::isMut>(kw<"mut">)) +
                  assign<&ast::RefPat::inner>(ref<PatternWithoutRangeRule>))  //
            | ref<StructPatRule>                                              //
            | ref<TupleStructPatRule>                                         //
            | ref<TuplePatRule>                                               //
            | create<ast::GroupedPat>(
                  lit<"("> + assign<&ast::GroupedPat::inner>(ref<PatternRule>) +
                  lit<")">)      //
            | ref<SlicePatRule>  //
            | create<ast::MacroPat>(
                  assign<&ast::MacroPat::path>(ref<SimplePathRule>) + lit<"!"> +
                  assign<&ast::MacroPat::tokens>(ref<DelimTokenTree>))  //
            | create<ast::PathPat>(assign<&ast::PathPat::path>(
                  ref<QualifiedPathInExpressionRule>))  //
            | create<ast::PathPat>(
                  assign<&ast::PathPat::path>(ref<PathInExpressionRule>))  //
            | ref<IdentPatRule>> {};

  struct PatternNoTopAltRule
      : rule<ref<RangePatRule> | ref<PatternWithoutRangeRule>> {};

  struct PatternRule
      : rule<option(lit<"|">) +
             (create<ast::OrPat>(
                  append<&ast::OrPat::alternatives>(ref<PatternNoTopAltRule>) +
                  some(lit<"|"> + append<&ast::OrPat::alternatives>(
                                       ref<PatternNoTopAltRule>)))  //
              | ref<PatternNoTopAltRule>)> {};

  // Expressions.
  struct LiteralExprRule
      : rule<create<ast::LiteralExpr>(assign<&ast::LiteralExpr::text>(
            kw<"true"> | kw<"false"> | ref<CHAR_LIT> | ref<STRING_LIT> |
            ref<RAW_STRING_LIT> | ref<BYTE_LIT> | ref<BYTE_STRING_LIT> |
            ref<RAW_BYTE_STRING_LIT> | ref<INTEGER_LIT> |
            ref<C_STRING_LIT> | ref<RAW_C_STRING_LIT> |
            ref<FLOAT_LIT>))> {};
  struct PathExprRule
      : rule<create<ast::PathExpr>(assign<&ast::PathExpr::path>(
                 ref<QualifiedPathInExpressionRule>))  //
             | create<ast::PathExpr>(
                   assign<&ast::PathExpr::path>(ref<PathInExpressionRule>))> {
  };
  struct GroupedExprRule
      : rule<create<ast::GroupedExpr>(
            lit<"("> +
            many(append<&ast::GroupedExpr::innerAttrs>(ref<InnerAttribute>)) +
            assign<&ast::GroupedExpr::inner>(ref<Expression>) + lit<")">)> {};
  struct ArrayExprRule
      : rule<create<ast::ArrayExpr>(
                 lit<"["> +
                 many(append<&ast::ArrayExpr::innerAttrs>(
                     ref<InnerAttribute>)) +
                 assign<&ast::ArrayExpr::repeatValue>(ref<Expression>) +
                 lit<";"> +
                 assign<&ast::ArrayExpr::repeatCount>(ref<Expression>) +
                 lit<"]">)  //
             | create<ast::ArrayExpr>(
                   lit<"["> +
                   many(append<&ast::ArrayExpr::innerAttrs>(
                       ref<InnerAttribute>)) +
                   option(append<&ast::ArrayExpr::elements>(ref<Expression>) +
                          many(lit<","> + append<&ast::ArrayExpr::elements>(
                                              ref<Expression>)) +
                          option(lit<",">)) +
                   lit<"]">)> {};
  struct TupleExprRule
      : rule<create<ast::TupleExpr>(
            lit<"("> +
            many(append<&ast::TupleExpr::innerAttrs>(ref<InnerAttribute>)) +
            option(append<&ast::TupleExpr::elements>(ref<Expression>) +
                   lit<","> +
                   option(append<&ast::TupleExpr::elements>(ref<Expression>) +
                          many(lit<","> + append<&ast::TupleExpr::elements>(
                                              ref<Expression>)) +
                          option(lit<",">))) +
            lit<")">)> {};
  struct StructExprFieldRule
      : rule<many(append<&ast::StructExprField::attrs>(ref<OuterAttribute>)) +
             ((assign<&ast::StructExprField::name>(ref<INTEGER_LIT>) +
               lit<":"> +
               assign<&ast::StructExprField::value>(ref<Expression>))  //
              | (assign<&ast::StructExprField::name>(Identifier) + lit<":"> +
                 assign<&ast::StructExprField::value>(ref<Expression>))  //
              | assign<&ast::StructExprField::name>(Identifier))> {};
  struct StructExprRule
      : rule<create<ast::StructExpr>(
                 assign<&ast::StructExpr::path>(ref<PathInExpressionRule>) +
                 lit<"{"> +
                 many(append<&ast::StructExpr::innerAttrs>(
                     ref<InnerAttribute>)) +
                 option((append<&ast::StructExpr::fields>(
                             ref<StructExprFieldRule>) +
                         many(lit<","> +
                              append<&ast::StructExpr::fields>(
                                  ref<StructExprFieldRule>)) +
                         option(lit<","> +
                                option(ref<DOTDOT> +
                                       assign<&ast::StructExpr::base>(
                                           ref<Expression>))))  //
                        | (ref<DOTDOT> + assign<&ast::StructExpr::base>(
                                             ref<Expression>))) +
                 lit<"}">)  //
             | create<ast::StructExpr>(
                   assign<&ast::StructExpr::path>(ref<PathInExpressionRule>) +
                   flag<&ast::StructExpr::isTuple>(lit<"(">) +
                   option(append<&ast::StructExpr::tupleArgs>(
                              ref<Expression>) +
                          many(lit<","> + append<&ast::StructExpr::tupleArgs>(
                                              ref<Expression>)) +
                          option(lit<",">)) +
                   lit<")">)> {};
  struct ClosureParamRule
      : rule<many(append<&ast::ClosureParam::attrs>(ref<OuterAttribute>)) +
             assign<&ast::ClosureParam::pattern>(ref<PatternNoTopAltRule>) +
             option(lit<":"> +
                    assign<&ast::ClosureParam::type>(ref<TypeRule>))> {};
  struct ClosureExprRule
      : rule<create<ast::ClosureExpr>(
            option(flag<&ast::ClosureExpr::isAsync>(kw<"async">)) +
            option(flag<&ast::ClosureExpr::isMove>(kw<"move">)) +
            (ref<OROR> |
             (lit<"|"> +
              option(
                  append<&ast::ClosureExpr::params>(ref<ClosureParamRule>) +
                  many(lit<","> + append<&ast::ClosureExpr::params>(
                                      ref<ClosureParamRule>)) +
                  option(lit<",">)) +
              lit<"|">)) +
            ((ref<RARROW> +
              assign<&ast::ClosureExpr::returnType>(ref<TypeNoBoundsRule>) +
              assign<&ast::ClosureExpr::body>(ref<BlockExprRule>))  //
             | assign<&ast::ClosureExpr::body>(ref<Expression>)))> {};

  struct PrimaryExpr
      : rule<ref<LiteralExprRule>  //
             | create<ast::AttrExpr>(
                   some(append<&ast::AttrExpr::attrs>(ref<OuterAttribute>)) +
                   assign<&ast::AttrExpr::value>(ref<Expression>))  //
             | ref<ClosureExprRule>                                 //
             | ref<StructExprRule>                                  //
             | ref<GroupedExprRule>                                 //
             | ref<ArrayExprRule>                                   //
             | ref<TupleExprRule>                                   //
             | ref<ExpressionWithBlock>                             //
             |
             create<ast::MacroExpr>(
                 assign<&ast::MacroExpr::path>(ref<SimplePathRule>) + lit<"!"> +
                 assign<&ast::MacroExpr::tokens>(ref<DelimTokenTree>))  //
             |
             create<ast::ContinueExpr>(
                 kw<"continue"> +
                 option(assign<&ast::ContinueExpr::label>(
                     ref<LIFETIME_OR_LABEL>)) +
                 option(assign<&ast::ContinueExpr::value>(ref<Expression>)))  //
             | create<ast::BreakExpr>(
                   kw<"break"> +
                   option(
                       assign<&ast::BreakExpr::label>(ref<LIFETIME_OR_LABEL>)) +
                   option(assign<&ast::BreakExpr::value>(ref<Expression>)))  //
             | create<ast::ReturnExpr>(
                   kw<"return"> +
                   option(assign<&ast::ReturnExpr::value>(ref<Expression>)))  //
             | ref<PathExprRule>> {};

  struct PostfixExpr
      : rule<ref<PrimaryExpr> +
             many(
                 nest<ast::MethodCallExpr, &ast::MethodCallExpr::receiver>(
                     ref<DOT> +
                     assign<&ast::MethodCallExpr::method>(Identifier) +
                     option(ref<PATHSEP> +
                            assign<&ast::MethodCallExpr::genericArgs>(
                                ref<GenericArgsRule>)) +
                     lit<"("> +
                     option(append<&ast::MethodCallExpr::args>(
                                ref<Expression>) +
                            many(lit<","> + append<&ast::MethodCallExpr::args>(
                                                ref<Expression>)) +
                            option(lit<",">)) +
                     lit<")">)  //
                 | nest<ast::AwaitExpr, &ast::AwaitExpr::receiver>(
                       ref<DOT> + kw<"await">)  //
                 | nest<ast::TupleIndexExpr, &ast::TupleIndexExpr::receiver>(
                       ref<DOT> +
                       assign<&ast::TupleIndexExpr::index>(ref<INTEGER_LIT>))  //
                 | nest<ast::FieldExpr, &ast::FieldExpr::receiver>(
                       ref<DOT> + assign<&ast::FieldExpr::field>(Identifier))  //
                 | nest<ast::CallExpr, &ast::CallExpr::callee>(
                       lit<"("> +
                       option(append<&ast::CallExpr::args>(ref<Expression>) +
                              many(lit<","> + append<&ast::CallExpr::args>(
                                                  ref<Expression>)) +
                              option(lit<",">)) +
                       lit<")">)  //
                 | nest<ast::IndexExpr, &ast::IndexExpr::receiver>(
                       lit<"["> +
                       assign<&ast::IndexExpr::index>(ref<Expression>) +
                       lit<"]">)  //
                 | nest<ast::TryExpr, &ast::TryExpr::receiver>(
                       ref<QUESTIONMARK>))> {};

  struct UnaryExpr
      : rule<create<ast::BorrowExpr>(
                 flag<&ast::BorrowExpr::doubleAmp>(ref<ANDAND>) +
                 option(flag<&ast::BorrowExpr::isMut>(kw<"mut">)) +
                 assign<&ast::BorrowExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::BorrowExpr>(
                   ref<AND> +
                   option(flag<&ast::BorrowExpr::isMut>(kw<"mut">)) +
                   assign<&ast::BorrowExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::DerefExpr>(
                   ref<STAR> +
                   assign<&ast::DerefExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"-", !cr<"=>">> |
                                               lit<"!", !cr<"=">>) +
                   assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | ref<PostfixExpr>> {};

  struct CastExpr
      : rule<ref<UnaryExpr> +
             many(nest<ast::CastExpr, &ast::CastExpr::operand>(
                 kw<"as"> +
                 assign<&ast::CastExpr::type>(ref<TypeNoBoundsRule>)))> {};

  struct ArithExpr
      : infix<CastExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<STAR> | ref<SLASH> | ref<PERCENT>) >>
               left(ref<PLUS> | ref<MINUS>))> {};

  struct ShiftExpr
      : rule<ref<ArithExpr> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<ShlOp> | ref<ShrOp>) +
                 assign<&ast::BinaryExpr::right>(ref<ArithExpr>)))> {};

  struct BitwiseExpr
      : infix<ShiftExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<AND>) >> left(ref<CARET>) >> left(ref<OR>))> {};

  struct ComparisonExpr
      : infix<BitwiseExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(ref<EQEQ> | ref<NE> | ref<LT> | ref<GT> | ref<LE> |
                   ref<GE>)> {};

  struct LogicalOrExpr
      : infix<ComparisonExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<ANDAND>) >> left(ref<OROR>))> {};

  struct RangeExprRule
      : rule<create<ast::RangeExpr>(
                 flag<&ast::RangeExpr::inclusive>(ref<DOTDOTEQ>) +
                 assign<&ast::RangeExpr::end>(ref<LogicalOrExpr>))  //
             | create<ast::RangeExpr>(
                   ref<DOTDOT> +
                   option(assign<&ast::RangeExpr::end>(ref<LogicalOrExpr>)))  //
             | create<ast::RangeExpr>(
                   assign<&ast::RangeExpr::start>(ref<LogicalOrExpr>) +
                   ((flag<&ast::RangeExpr::inclusive>(ref<DOTDOTEQ>) +
                     assign<&ast::RangeExpr::end>(ref<LogicalOrExpr>))  //
                    | (ref<DOTDOT> + option(assign<&ast::RangeExpr::end>(
                                                ref<LogicalOrExpr>)))))  //
             | ref<LogicalOrExpr>> {};

  struct Expression
      : infix<RangeExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              right(ref<EQ> | ref<PLUSEQ> | ref<MINUSEQ> | ref<STAREQ> |
                    ref<SLASHEQ> | ref<PERCENTEQ> | ref<ANDEQ> | ref<OREQ> |
                    ref<CARETEQ> | ref<SHLEQ> | ref<SHREQ>)> {};

  struct BlockExprRule
      : rule<create<ast::BlockExpr>(
            lit<"{"> +
            many(append<&ast::BlockExpr::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::BlockExpr::statements>(ref<Statement>) |
                 lit<";">) +
            option(assign<&ast::BlockExpr::tailExpr>(ref<Expression>)) +
            lit<"}">)> {};
  struct AsyncBlockExprRule
      : rule<create<ast::AsyncBlockExpr>(
            kw<"async">
            + option(flag<&ast::AsyncBlockExpr::isMove>(kw<"move">)) +
            assign<&ast::AsyncBlockExpr::body>(ref<BlockExprRule>))> {};
  struct UnsafeBlockExprRule
      : rule<create<ast::UnsafeBlockExpr>(
            kw<"unsafe">
            + assign<&ast::UnsafeBlockExpr::body>(ref<BlockExprRule>))> {};

  struct InfiniteLoopExprRule
      : rule<create<ast::InfiniteLoopExpr>(
            option(assign<&ast::InfiniteLoopExpr::label>(
                       ref<LIFETIME_OR_LABEL>) +
                   lit<":">) +
            kw<"loop"> +
            assign<&ast::InfiniteLoopExpr::body>(ref<BlockExprRule>))> {};
  struct WhileLetLoopExprRule
      : rule<create<ast::WhileLetLoopExpr>(
            option(assign<&ast::WhileLetLoopExpr::label>(
                       ref<LIFETIME_OR_LABEL>) +
                   lit<":">) +
            kw<"while"> + kw<"let"> +
            assign<&ast::WhileLetLoopExpr::pattern>(ref<PatternRule>) +
            ref<EQ> +
            assign<&ast::WhileLetLoopExpr::value>(ref<Expression>) +
            assign<&ast::WhileLetLoopExpr::body>(ref<BlockExprRule>))> {};
  struct WhileLoopExprRule
      : rule<create<ast::WhileLoopExpr>(
            option(assign<&ast::WhileLoopExpr::label>(
                       ref<LIFETIME_OR_LABEL>) +
                   lit<":">) +
            kw<"while"> + assign<&ast::WhileLoopExpr::cond>(ref<Expression>) +
            assign<&ast::WhileLoopExpr::body>(ref<BlockExprRule>))> {};
  struct ForLoopExprRule
      : rule<create<ast::ForLoopExpr>(
            option(assign<&ast::ForLoopExpr::label>(ref<LIFETIME_OR_LABEL>) +
                   lit<":">) +
            kw<"for"> + assign<&ast::ForLoopExpr::pattern>(ref<PatternRule>) +
            kw<"in"> + assign<&ast::ForLoopExpr::iter>(ref<Expression>) +
            assign<&ast::ForLoopExpr::body>(ref<BlockExprRule>))> {};
  struct LoopExprDispatch
      : rule<ref<WhileLetLoopExprRule> | ref<WhileLoopExprRule> |
             ref<ForLoopExprRule> | ref<InfiniteLoopExprRule>> {};

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            kw<"if"> + assign<&ast::IfExpr::cond>(ref<Expression>) +
            assign<&ast::IfExpr::thenBranch>(ref<BlockExprRule>) +
            option(commit(kw<"else">, assign<&ast::IfExpr::elseBranch>(
                         ref<BlockExprRule> | ref<IfExprRule> |
                         ref<IfLetExprRule>))))> {};
  struct IfLetExprRule
      : rule<create<ast::IfLetExpr>(
            kw<"if"> + kw<"let"> +
            assign<&ast::IfLetExpr::pattern>(ref<PatternRule>) + ref<EQ> +
            assign<&ast::IfLetExpr::value>(ref<Expression>) +
            assign<&ast::IfLetExpr::thenBranch>(ref<BlockExprRule>) +
            option(commit(kw<"else">, assign<&ast::IfLetExpr::elseBranch>(
                         ref<BlockExprRule> | ref<IfExprRule> |
                         ref<IfLetExprRule>))))> {};

  struct MatchArmBodyRule
      : rule<many(append<&ast::MatchArm::attrs>(ref<OuterAttribute>)) +
             assign<&ast::MatchArm::pattern>(ref<PatternRule>) +
             option(kw<"if"> + assign<&ast::MatchArm::guard>(ref<Expression>)) +
             ref<FATARROW> + assign<&ast::MatchArm::body>(ref<Expression>)> {
  };
  struct MatchExprRule
      : rule<create<ast::MatchExpr>(
            kw<"match"> + assign<&ast::MatchExpr::subject>(ref<Expression>) +
            lit<"{"> +
            many(append<&ast::MatchExpr::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::MatchExpr::arms>(ref<MatchArmBodyRule>) +
                 option(lit<",">)) +
            lit<"}">)> {};

  // `#[attr] if cond {} ` at statement level needs the block-shaped
  // reading (no mandatory trailing `;`): the general `AttrExpr` in Primary
  // only reaches it through `Expression`, which does require one.
  struct AttrExpressionWithBlock
      : rule<create<ast::AttrExpr>(
            some(append<&ast::AttrExpr::attrs>(ref<OuterAttribute>)) +
            assign<&ast::AttrExpr::value>(ref<ExpressionWithBlock>))> {};
  struct ExpressionWithBlock
      : rule<ref<BlockExprRule> | ref<AsyncBlockExprRule> |
             ref<UnsafeBlockExprRule> | ref<LoopExprDispatch> |
             ref<IfLetExprRule> | ref<IfExprRule> | ref<MatchExprRule> |
             ref<AttrExpressionWithBlock>> {};

  // Statements.
  struct LetStatementRule
      : rule<create<ast::LetStmt>(
            many(append<&ast::LetStmt::attrs>(ref<OuterAttribute>)) +
            kw<"let"> +
            assign<&ast::LetStmt::pattern>(ref<PatternNoTopAltRule>) +
            option(lit<":"> + assign<&ast::LetStmt::type>(ref<TypeRule>)) +
            option(ref<EQ> + assign<&ast::LetStmt::value>(ref<Expression>) +
                   option(kw<"else"> + assign<&ast::LetStmt::elseBranch>(
                                           ref<BlockExprRule>))) +
            lit<";">)> {};
  struct ExpressionStatementRule
      : rule<create<ast::ExprStmt>(
                 assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">)  //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<ExpressionWithBlock>) +
                   option(lit<";">))> {};

  struct Statement
      : rule<ref<ItemRule> | ref<LetStatementRule> |
             ref<ExpressionStatementRule>> {};

  // Items.
  static constexpr auto ItemAttrs =
      many(append<&ast::Item::attrs>(ref<OuterAttribute>));

  struct SelfParamRule
      : rule<many(append<&ast::FnParam::attrs>(ref<OuterAttribute>)) +
             ((option(flag<&ast::FnParam::selfByRef>(ref<AND>) +
                      option(assign<&ast::FnParam::selfLifetime>(Lifetime))) +
               option(flag<&ast::FnParam::isMut>(kw<"mut">)) +
               flag<&ast::FnParam::isSelf>(kw<"self">))  //
              | (option(flag<&ast::FnParam::isMut>(kw<"mut">)) +
                 flag<&ast::FnParam::isSelf>(kw<"self">) + lit<":"> +
                 assign<&ast::FnParam::selfType>(ref<TypeRule>)))> {};
  struct FunctionParamRule
      : rule<many(append<&ast::FnParam::attrs>(ref<OuterAttribute>)) +
             ((assign<&ast::FnParam::pattern>(ref<PatternRule>) + lit<":"> +
               (assign<&ast::FnParam::type>(ref<TypeRule>) |
                flag<&ast::FnParam::variadic>(ref<DOTDOTDOT>)))  //
              | flag<&ast::FnParam::variadic>(ref<DOTDOTDOT>)  //
              | assign<&ast::FnParam::type>(ref<TypeRule>))> {};
  struct FunctionRule
      : rule<create<ast::Function>(
            ItemAttrs + option(assign<&ast::Function::vis>(ref<VisibilityRule>)) +
            option(flag<&ast::Function::isConst>(kw<"const">)) +
            option(flag<&ast::Function::isAsync>(kw<"async">)) +
            option(flag<&ast::Function::isUnsafe>(kw<"unsafe">)) +
            option(flag<&ast::Function::isExtern>(kw<"extern">) +
                   option(assign<&ast::Function::abi>(ref<STRING_LIT>))) +
            kw<"fn"> + assign<&ast::Function::name>(Identifier) +
            option(assign<&ast::Function::generics>(ref<GenericParamsRule>)) +
            lit<"("> +
            option((append<&ast::Function::params>(ref<SelfParamRule>) +
                    option(lit<",">))  //
                   | (option(append<&ast::Function::params>(
                                 ref<SelfParamRule>) +
                             lit<",">) +
                      append<&ast::Function::params>(ref<FunctionParamRule>) +
                      many(lit<","> + append<&ast::Function::params>(
                                          ref<FunctionParamRule>)) +
                      option(lit<",">))) +
            lit<")"> +
            option(ref<RARROW> +
                   assign<&ast::Function::returnType>(ref<TypeRule>)) +
            option(assign<&ast::Function::whereClause>(ref<WhereClauseRule>)) +
            (assign<&ast::Function::body>(ref<BlockExprRule>) |
             lit<";">))> {};

  struct TypeAliasRule
      : rule<create<ast::TypeAlias>(
            ItemAttrs +
            option(assign<&ast::TypeAlias::vis>(ref<VisibilityRule>)) +
            kw<"type"> + assign<&ast::TypeAlias::name>(Identifier) +
            option(assign<&ast::TypeAlias::generics>(ref<GenericParamsRule>)) +
            option(assign<&ast::TypeAlias::whereClause>(
                ref<WhereClauseRule>)) +
            option(ref<EQ> + assign<&ast::TypeAlias::type>(ref<TypeRule>)) +
            lit<";">)> {};

  struct StructFieldRule
      : rule<many(append<&ast::StructField::attrs>(ref<OuterAttribute>)) +
             option(assign<&ast::StructField::vis>(ref<VisibilityRule>)) +
             assign<&ast::StructField::name>(Identifier) + lit<":"> +
             assign<&ast::StructField::type>(ref<TypeRule>)> {};
  struct TupleFieldRule
      : rule<many(append<&ast::TupleField::attrs>(ref<OuterAttribute>)) +
             option(assign<&ast::TupleField::vis>(ref<VisibilityRule>)) +
             assign<&ast::TupleField::type>(ref<TypeRule>)> {};
  struct StructRule
      : rule<create<ast::Struct>(
                 ItemAttrs +
                 option(assign<&ast::Struct::vis>(ref<VisibilityRule>)) +
                 kw<"struct"> + assign<&ast::Struct::name>(Identifier) +
                 option(assign<&ast::Struct::generics>(
                     ref<GenericParamsRule>)) +
                 option(assign<&ast::Struct::whereClause>(
                     ref<WhereClauseRule>)) +
                 ((lit<"{"> +
                   option(append<&ast::Struct::fields>(ref<StructFieldRule>) +
                          many(lit<","> + append<&ast::Struct::fields>(
                                              ref<StructFieldRule>)) +
                          option(lit<",">)) +
                   lit<"}">)  //
                  | lit<";">))  //
             | create<ast::Struct>(
                   ItemAttrs +
                   option(assign<&ast::Struct::vis>(ref<VisibilityRule>)) +
                   kw<"struct"> + assign<&ast::Struct::name>(Identifier) +
                   option(assign<&ast::Struct::generics>(
                       ref<GenericParamsRule>)) +
                   flag<&ast::Struct::isTuple>(lit<"(">) +
                   option(append<&ast::Struct::tupleFields>(
                              ref<TupleFieldRule>) +
                          many(lit<","> + append<&ast::Struct::tupleFields>(
                                              ref<TupleFieldRule>)) +
                          option(lit<",">)) +
                   lit<")"> +
                   option(assign<&ast::Struct::whereClause>(
                       ref<WhereClauseRule>)) +
                   lit<";">)> {};

  struct EnumVariantRule
      : rule<many(append<&ast::EnumVariant::attrs>(ref<OuterAttribute>)) +
             option(assign<&ast::EnumVariant::vis>(ref<VisibilityRule>)) +
             assign<&ast::EnumVariant::name>(Identifier) +
             option(
                 (flag<&ast::EnumVariant::isTuple>(lit<"(">) +
                  option(append<&ast::EnumVariant::tupleFields>(
                             ref<TupleFieldRule>) +
                         many(lit<","> + append<&ast::EnumVariant::tupleFields>(
                                             ref<TupleFieldRule>)) +
                         option(lit<",">)) +
                  lit<")">)  //
                 | (flag<&ast::EnumVariant::isStruct>(lit<"{">) +
                    option(append<&ast::EnumVariant::structFields>(
                               ref<StructFieldRule>) +
                           many(lit<","> +
                                append<&ast::EnumVariant::structFields>(
                                    ref<StructFieldRule>)) +
                           option(lit<",">)) +
                    lit<"}">)  //
                 | (ref<EQ> + assign<&ast::EnumVariant::discriminant>(
                                  ref<Expression>)))> {};
  struct EnumRule
      : rule<create<ast::Enum>(
            ItemAttrs + option(assign<&ast::Enum::vis>(ref<VisibilityRule>)) +
            kw<"enum"> + assign<&ast::Enum::name>(Identifier) +
            option(assign<&ast::Enum::generics>(ref<GenericParamsRule>)) +
            option(assign<&ast::Enum::whereClause>(ref<WhereClauseRule>)) +
            lit<"{"> +
            option(append<&ast::Enum::variants>(ref<EnumVariantRule>) +
                   many(lit<","> + append<&ast::Enum::variants>(
                                       ref<EnumVariantRule>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct UnionRule
      : rule<create<ast::Union>(
            ItemAttrs + option(assign<&ast::Union::vis>(ref<VisibilityRule>)) +
            kw<"union"> + assign<&ast::Union::name>(Identifier) +
            option(assign<&ast::Union::generics>(ref<GenericParamsRule>)) +
            option(assign<&ast::Union::whereClause>(ref<WhereClauseRule>)) +
            lit<"{"> +
            append<&ast::Union::fields>(ref<StructFieldRule>) +
            many(lit<","> +
                 append<&ast::Union::fields>(ref<StructFieldRule>)) +
            option(lit<",">) + lit<"}">)> {};

  struct ConstItemRule
      : rule<create<ast::ConstItem>(
            ItemAttrs +
            option(assign<&ast::ConstItem::vis>(ref<VisibilityRule>)) +
            kw<"const"> +
            (assign<&ast::ConstItem::name>(Identifier) | ref<UNDERSCORE>) +
            lit<":"> + assign<&ast::ConstItem::type>(ref<TypeRule>) +
            option(ref<EQ> +
                   assign<&ast::ConstItem::value>(ref<Expression>)) +
            lit<";">)> {};
  struct StaticItemRule
      : rule<create<ast::StaticItem>(
            ItemAttrs +
            option(assign<&ast::StaticItem::vis>(ref<VisibilityRule>)) +
            kw<"static"> +
            option(flag<&ast::StaticItem::isMut>(kw<"mut">)) +
            assign<&ast::StaticItem::name>(Identifier) + lit<":"> +
            assign<&ast::StaticItem::type>(ref<TypeRule>) +
            option(ref<EQ> +
                   assign<&ast::StaticItem::value>(ref<Expression>)) +
            lit<";">)> {};

  struct MacroItemRule
      : rule<create<ast::MacroStmt>(
            ItemAttrs + assign<&ast::MacroStmt::path>(ref<SimplePathRule>) +
            lit<"!"> + assign<&ast::MacroStmt::tokens>(ref<DelimTokenTree>) +
            option(lit<";">))> {};

  static constexpr auto AssociatedItem = ref<TypeAliasRule> |
                                         ref<ConstItemRule> |
                                         ref<FunctionRule> | ref<MacroItemRule>;

  struct TraitRule
      : rule<create<ast::Trait>(
            ItemAttrs + option(assign<&ast::Trait::vis>(ref<VisibilityRule>)) +
            option(flag<&ast::Trait::isUnsafe>(kw<"unsafe">)) +
            kw<"trait"> + assign<&ast::Trait::name>(Identifier) +
            option(assign<&ast::Trait::generics>(ref<GenericParamsRule>)) +
            option(lit<":"> +
                   option(append<&ast::Trait::supertraits>(
                              TraitBoundOrLifetime) +
                          many(lit<"+"> + append<&ast::Trait::supertraits>(
                                              TraitBoundOrLifetime)) +
                          option(lit<"+">))) +
            option(assign<&ast::Trait::whereClause>(ref<WhereClauseRule>)) +
            lit<"{"> +
            many(append<&ast::Trait::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::Trait::items>(AssociatedItem)) + lit<"}">)> {};

  struct ImplRule
      : rule<create<ast::Impl>(
            ItemAttrs +
            option(flag<&ast::Impl::isUnsafe>(kw<"unsafe">)) +
            kw<"impl"> +
            option(assign<&ast::Impl::generics>(ref<GenericParamsRule>)) +
            ((option(flag<&ast::Impl::isNegative>(lit<"!">)) +
              assign<&ast::Impl::traitRef>(ref<TypePathRule>) + kw<"for"> +
              assign<&ast::Impl::type>(ref<TypeRule>))  //
             | assign<&ast::Impl::type>(ref<TypeRule>)) +
            option(assign<&ast::Impl::whereClause>(ref<WhereClauseRule>)) +
            lit<"{"> +
            many(append<&ast::Impl::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::Impl::items>(AssociatedItem)) + lit<"}">)> {};

  struct ExternBlockRule
      : rule<create<ast::ExternBlock>(
            ItemAttrs +
            option(assign<&ast::ExternBlock::vis>(ref<VisibilityRule>)) +
            option(flag<&ast::ExternBlock::isUnsafe>(kw<"unsafe">)) +
            kw<"extern"> +
            option(assign<&ast::ExternBlock::abi>(ref<STRING_LIT>)) +
            lit<"{"> +
            many(append<&ast::ExternBlock::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::ExternBlock::items>(
                ref<StaticItemRule> | ref<FunctionRule> |
                ref<MacroItemRule>)) +
            lit<"}">)> {};

  struct ModuleItemRule
      : rule<create<ast::Module>(
            ItemAttrs + option(assign<&ast::Module::vis>(ref<VisibilityRule>)) +
            option(flag<&ast::Module::isUnsafe>(kw<"unsafe">)) +
            kw<"mod"> + assign<&ast::Module::name>(Identifier) +
            (lit<";">  //
             | (flag<&ast::Module::isInline>(lit<"{">) +
                many(append<&ast::Module::innerAttrs>(ref<InnerAttribute>)) +
                many(append<&ast::Module::items>(ref<ItemRule>)) +
                lit<"}">)))> {};

  struct ExternCrateRule
      : rule<create<ast::ExternCrate>(
            ItemAttrs +
            option(assign<&ast::ExternCrate::vis>(ref<VisibilityRule>)) +
            kw<"extern"> + kw<"crate"> +
            assign<&ast::ExternCrate::name>(Identifier | kw<"self">) +
            option(kw<"as"> + assign<&ast::ExternCrate::asName>(
                                   Identifier | ref<UNDERSCORE>)) +
            lit<";">)> {};

  struct UseTreeRule
      : rule<create<ast::UseTree>(
            option(assign<&ast::UseTree::path>(ref<SimplePathRule>)) +
            ((option(ref<PATHSEP>) +
              (flag<&ast::UseTree::isGlob>(ref<STAR>)  //
               | (lit<"{"> +
                  option(append<&ast::UseTree::nested>(ref<UseTreeRule>) +
                         many(lit<","> + append<&ast::UseTree::nested>(
                                             ref<UseTreeRule>)) +
                         option(lit<",">)) +
                  lit<"}">)))  //
             | option(kw<"as"> + assign<&ast::UseTree::asName>(
                                      Identifier | ref<UNDERSCORE>))))> {};
  struct UseDeclRule
      : rule<create<ast::UseDecl>(
            ItemAttrs + option(assign<&ast::UseDecl::vis>(ref<VisibilityRule>)) +
            kw<"use"> + assign<&ast::UseDecl::tree>(ref<UseTreeRule>) +
            lit<";">)> {};

  struct MacroRulesDefRule
      : rule<create<ast::MacroRulesDef>(
            ItemAttrs + kw<"macro_rules"> + lit<"!"> +
            assign<&ast::MacroRulesDef::name>(Identifier) +
            assign<&ast::MacroRulesDef::tokens>(ref<DelimTokenTree>) +
            option(lit<";">))> {};

  struct ItemRule
      : rule<ref<ModuleItemRule> | ref<ExternCrateRule> | ref<UseDeclRule> |
             ref<FunctionRule> | ref<TypeAliasRule> | ref<StructRule> |
             ref<EnumRule> | ref<UnionRule> | ref<ConstItemRule> |
             ref<StaticItemRule> | ref<TraitRule> | ref<ImplRule> |
             ref<ExternBlockRule> | ref<MacroRulesDefRule> |
             ref<MacroItemRule>> {};

  struct CrateRule
      : rule<create<ast::Crate>(
            option(lit<"\xEF\xBB\xBF">) + option(ref<SHEBANG>) +
            many(append<&ast::Crate::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::Crate::items>(ref<ItemRule>)))> {};

  using entry = CrateRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT, INNER_LINE_DOC,
                       OUTER_LINE_DOC>;
  static constexpr auto word_boundary = uc::XID_Continue;
};

}  // namespace rust
