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

  static constexpr auto ReservedKeyword =
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
      kw<"gen">;
  static constexpr auto ReservedRaw = lit<"r#"> +
      (kw<"crate"> | kw<"self"> | kw<"Self"> | kw<"super"> | kw<"_">);
  static constexpr auto IdentBody =
      (uc::XID_Start + many(uc::XID_Continue)) |
      (lit<"_"> + some(uc::XID_Continue));
  struct NON_KEYWORD_IDENTIFIER : terminal<!ReservedKeyword + !ReservedRaw + IdentBody> {};
  struct RAW_IDENTIFIER : terminal<!ReservedRaw + lit<"r#"> + IdentBody> {};
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
  struct LT : terminal<as<ast::Operator::lt>(lit<"<", !cr<"=">>)> {};
  struct GT : terminal<as<ast::Operator::gt>(lit<">", !cr<"=">>)> {};
  struct STAR : terminal<as<ast::Operator::multiply>(lit<"*", !cr<"=">>)> {};
  struct MINUS : terminal<as<ast::Operator::subtract>(lit<"-", !cr<"=>">>)> {};
  struct AND : terminal<as<ast::Operator::bit_and>(lit<"&", !cr<"&=">>)> {};
  struct ANDAND : terminal<as<ast::Operator::and_and>(lit<"&&">)> {};
  struct OROR : terminal<as<ast::Operator::or_or>(lit<"||">)> {};
  struct EQ : terminal<as<ast::Operator::assign>(lit<"=", !cr<"=>">>)> {};
  // Shift punctuation is adjacent; generic closing > tokens remain separate
  // grammar reads and can still have whitespace between them.
  struct ShlOp : rule<as<ast::Operator::shl>(ref<LT> + gap::none + ref<LT>)> {};
  struct ShrOp : rule<as<ast::Operator::shr>(ref<GT> + gap::none + ref<GT>)> {};

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

  static constexpr auto AnyKeyword = ReservedKeyword | kw<"union">;
  static constexpr auto AnyPunct =
      ref<LT> | ref<GT> | as<ast::Operator::le>(lit<"<=">) | as<ast::Operator::ge>(lit<">=">) | as<ast::Operator::eq>(lit<"==">) | as<ast::Operator::ne>(lit<"!=">) |
      ref<STAR> | as<ast::Operator::divide>(lit<"/", !cr<"=/*">>) | as<ast::Operator::remainder>(lit<"%", !cr<"=">>) | as<ast::Operator::add>(lit<"+", !cr<"=">>) | ref<MINUS> |
      ref<AND> | as<ast::Operator::bit_xor>(lit<"^", !cr<"=">>) | as<ast::Operator::bit_or>(lit<"|", !cr<"|=">>) | ref<ANDAND> | ref<OROR> | ref<EQ> |
      as<ast::Operator::add_assign>(lit<"+=">) | as<ast::Operator::sub_assign>(lit<"-=">) | as<ast::Operator::mul_assign>(lit<"*=">) | as<ast::Operator::div_assign>(lit<"/=">) |
      as<ast::Operator::rem_assign>(lit<"%=">) | as<ast::Operator::bitxor_assign>(lit<"^=">) | as<ast::Operator::bitand_assign>(lit<"&=">) | as<ast::Operator::bitor_assign>(lit<"|=">) | as<ast::Operator::shl_assign>(lit<"<<=">) |
      as<ast::Operator::shr_assign>(lit<">>=">) | ref<DOT> | ref<DOTDOT> | ref<DOTDOTDOT> | ref<DOTDOTEQ> |
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
  struct MacroTokenRule : rule<as<raw_macro_token>(MacroToken)> {};
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
  struct BlockExpr;
  struct Statement;
  struct IfExpr;
  struct IfLetExpr;
  struct Type;
  struct TypeNoBounds;
  struct Pattern;
  struct PatternNoTopAlt;
  struct LiteralExpr;
  struct Item;

  static constexpr auto Lifetime =
      ref<LIFETIME_OR_LABEL> | lit<"'static"> | lit<"'_">;

  // Paths and generics.
  struct SimplePathSegment
      : rule<assign<&ast::PathSegment::name>(
            Identifier | kw<"super"> | kw<"self"> | kw<"crate"> |
            lit<"$crate">)> {};
  struct SimplePath
      : rule<option(flag<&ast::Path::leadingSep>(ref<PATHSEP>)) +
             append<&ast::Path::segments>(ref<SimplePathSegment>) +
             many(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<SimplePathSegment>))> {};

  static constexpr auto PathIdentSegmentName =
      Identifier | kw<"super"> | kw<"self"> | kw<"Self"> | kw<"crate"> |
      lit<"$crate">;

  struct GenericArg;
  struct GenericArgs
      : rule<create<ast::GenericArgs>(
            lit<"<"> +
            option(append<&ast::GenericArgs::args>(ref<GenericArg>) +
                   many(lit<","> +
                        append<&ast::GenericArgs::args>(ref<GenericArg>)) +
                   option(lit<",">)) +
            ref<GT>)> {};

  struct PathExprSegment
      : rule<assign<&ast::PathSegment::name>(PathIdentSegmentName) +
             option(ref<PATHSEP> +
                    assign<&ast::PathSegment::args>(ref<GenericArgs>))> {
  };
  struct PathInExpression
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
             option(assign<&ast::PathSegment::args>(ref<GenericArgs>) |
                    (lit<"("> +
                     option(append<&ast::PathSegment::fnSugarParams>(
                                ref<Type>) +
                            many(lit<","> +
                                 append<&ast::PathSegment::fnSugarParams>(
                                     ref<Type>)) +
                            option(lit<",">)) +
                     lit<")"> +
                     option(ref<RARROW> +
                            assign<&ast::PathSegment::fnSugarReturn>(
                                ref<Type>))))> {};
  struct TypePath
      : rule<option(flag<&ast::Path::leadingSep>(ref<PATHSEP>)) +
             append<&ast::Path::segments>(ref<TypePathSegment>) +
             many(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<TypePathSegment>))> {};

  struct QualifiedPathInExpression
      : rule<lit<"<"> + assign<&ast::Path::qualifierType>(ref<Type>) +
             option(kw<"as"> +
                    assign<&ast::Path::qualifierTrait>(ref<TypePath>)) +
             ref<GT> +
             some(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<PathExprSegment>))> {};
  struct QualifiedPathInType
      : rule<lit<"<"> + assign<&ast::Path::qualifierType>(ref<Type>) +
             option(kw<"as"> +
                    assign<&ast::Path::qualifierTrait>(ref<TypePath>)) +
             ref<GT> +
             some(ref<PATHSEP> +
                  append<&ast::Path::segments>(ref<TypePathSegment>))> {};

  // rustc starts an unambiguous const argument at a block, literal or minus.
  // A bare path such as N is syntactically a type argument until resolution.
  struct GenericConstExpr
      : rule<ref<BlockExpr> | ref<LiteralExpr> |
             create<ast::UnaryExpr>(assign<&ast::UnaryExpr::op>(lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<LiteralExpr>))> {};
  struct GenericArg
      : rule<create<ast::BindingArg>(
                 assign<&ast::BindingArg::name>(Identifier) + ref<EQ> +
                 assign<&ast::BindingArg::type>(ref<Type>)) |
             create<ast::LifetimeArg>(assign<&ast::LifetimeArg::name>(Lifetime)) |
             create<ast::ConstArg>(assign<&ast::ConstArg::value>(ref<GenericConstExpr>)) |
             create<ast::TypeArg>(assign<&ast::TypeArg::type>(ref<Type>))> {};

  // Attributes and visibility. `literalArg` is text: the `= literal` form
  // reads the literal as bare terminals, not `ref<LiteralExpr>`, which
  // builds a node.
  static constexpr auto AttrLiteral =
      kw<"true"> | kw<"false"> | ref<CHAR_LIT> | ref<STRING_LIT> |
      ref<RAW_STRING_LIT> | ref<BYTE_LIT> | ref<BYTE_STRING_LIT> |
      ref<RAW_BYTE_STRING_LIT> | ref<C_STRING_LIT> | ref<RAW_C_STRING_LIT> |
      ref<INTEGER_LIT> | ref<FLOAT_LIT>;
  struct OuterAttribute
      : rule<create<ast::Attribute>(
            lit<"#"> + lit<"["> +
            assign<&ast::Attribute::path>(ref<SimplePath>) +
            option(
                assign<&ast::Attribute::tokens>(ref<DelimTokenTree>) |
                (ref<EQ> + assign<&ast::Attribute::literalArg>(AttrLiteral))) +
            lit<"]">)> {};
  struct InnerAttribute
      : rule<create<ast::Attribute>(
            lit<"#"> + lit<"!"> + lit<"["> +
            assign<&ast::Attribute::path>(ref<SimplePath>) +
            option(
                assign<&ast::Attribute::tokens>(ref<DelimTokenTree>) |
                (ref<EQ> + assign<&ast::Attribute::literalArg>(AttrLiteral))) +
            lit<"]">)> {};

  struct Visibility
      : rule<kw<"pub"> +
             option(lit<"("> +
                    (assign<&ast::Visibility::kind>(kw<"crate"> |
                                                     kw<"self"> |
                                                     kw<"super">) |
                     (assign<&ast::Visibility::kind>(kw<"in">) +
                      assign<&ast::Visibility::inPath>(
                          ref<SimplePath>))) +
                    lit<")">)> {};

  // Trait bounds sit before generic params/where clauses, which reuse them.
  struct GenericParams;
  struct TraitBound
      : rule<create<ast::TraitBound>(
            (lit<"("> +
             option(flag<&ast::TraitBound::maybe>(lit<"?">)) +
             option(kw<"for"> + assign<&ast::TraitBound::forLifetimes>(
                                     ref<GenericParams>)) +
             assign<&ast::TraitBound::path>(ref<TypePath>) + lit<")">)  //
            | (option(flag<&ast::TraitBound::maybe>(lit<"?">)) +
               option(kw<"for"> + assign<&ast::TraitBound::forLifetimes>(
                                       ref<GenericParams>)) +
               assign<&ast::TraitBound::path>(ref<TypePath>)))> {};
  static constexpr auto TraitBoundOrLifetime = ref<TraitBound> | Lifetime;

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
      option(ref<EQ> + assign<&ast::TypeParam::defaultType>(ref<Type>));
  static constexpr auto ConstParamBody =
      kw<"const"> + assign<&ast::ConstParam::name>(Identifier) + lit<":"> +
      assign<&ast::ConstParam::type>(ref<Type>) +
      option(ref<EQ> + assign<&ast::ConstParam::defaultValue>(ref<Expression>));
  struct GenericParam
      : rule<many(append<&ast::GenericParam::attrs>(ref<OuterAttribute>)) +
             (create<ast::LifetimeParam>(LifetimeParamBody) |
              create<ast::TypeParam>(TypeParamBody) |
              create<ast::ConstParam>(ConstParamBody))> {};
  struct GenericParams
      : rule<create<ast::GenericParams>(
            lit<"<"> +
            option(append<&ast::GenericParams::params>(
                       ref<GenericParam>) +
                   many(lit<","> + append<&ast::GenericParams::params>(
                                       ref<GenericParam>)) +
                   option(lit<",">)) +
            ref<GT>)> {};

  struct LifetimeWhereItem
      : rule<assign<&ast::LifetimeWhereItem::lifetime>(Lifetime) + lit<":"> +
             option(append<&ast::LifetimeWhereItem::bounds>(Lifetime) +
                    many(lit<"+"> + append<&ast::LifetimeWhereItem::bounds>(
                                        Lifetime)))> {};
  struct TypeBoundWhereItem
      : rule<option(kw<"for"> +
                    assign<&ast::TypeBoundWhereItem::forLifetimes>(
                        ref<GenericParams>)) +
             assign<&ast::TypeBoundWhereItem::type>(ref<Type>) +
             lit<":"> +
             option(append<&ast::TypeBoundWhereItem::bounds>(
                        TraitBoundOrLifetime) +
                    many(lit<"+"> +
                         append<&ast::TypeBoundWhereItem::bounds>(
                             TraitBoundOrLifetime)))> {};
  struct WhereClauseItem
      : rule<ref<TypeBoundWhereItem> | ref<LifetimeWhereItem>> {};
  struct WhereClause
      : rule<create<ast::WhereClause>(
            kw<"where"> +
            many(append<&ast::WhereClause::items>(ref<WhereClauseItem>) +
                 lit<",">) +
            option(append<&ast::WhereClause::items>(
                ref<WhereClauseItem>)))> {};

  // Types.
  struct ArrayOrSliceTy
      : rule<create<ast::ArrayTy>(
                 lit<"["> + assign<&ast::ArrayTy::element>(ref<Type>) +
                 lit<";"> + assign<&ast::ArrayTy::size>(ref<Expression>) +
                 lit<"]">)  //
             | create<ast::SliceTy>(
                   lit<"["> + assign<&ast::SliceTy::element>(ref<Type>) +
                   lit<"]">)> {};
  struct BareFnParam
      : rule<create<ast::BareFnParam>(
            many(append<&ast::BareFnParam::attrs>(ref<OuterAttribute>)) +
            option(assign<&ast::BareFnParam::name>(Identifier | ref<UNDERSCORE>) + lit<":">) +
            assign<&ast::BareFnParam::type>(ref<Type>))> {};
  struct BareFnTy
      : rule<create<ast::BareFnTy>(
            option(kw<"for"> + assign<&ast::BareFnTy::forLifetimes>(
                                    ref<GenericParams>)) +
            option(flag<&ast::BareFnTy::isUnsafe>(kw<"unsafe">)) +
            option(flag<&ast::BareFnTy::isExtern>(kw<"extern">) +
                   option(assign<&ast::BareFnTy::abi>(ref<STRING_LIT>))) +
            kw<"fn"> + lit<"("> +
            option(
                append<&ast::BareFnTy::params>(ref<BareFnParam>) +
                many(lit<","> + append<&ast::BareFnTy::params>(ref<BareFnParam>)) +
                option(lit<","> +
                       option(flag<&ast::BareFnTy::variadic>(
                           ref<DOTDOTDOT>)))) +
            lit<")"> +
            option(ref<RARROW> + assign<&ast::BareFnTy::returnType>(
                                      ref<TypeNoBounds>)))> {};
  struct ImplTraitTy
      : rule<create<ast::ImplTraitTy>(
            kw<"impl"> +
            append<&ast::ImplTraitTy::bounds>(TraitBoundOrLifetime) +
            many(lit<"+"> +
                 append<&ast::ImplTraitTy::bounds>(TraitBoundOrLifetime)) +
            option(lit<"+">))> {};
  struct ImplTraitTyOneBound
      : rule<create<ast::ImplTraitTy>(
            kw<"impl"> +
            append<&ast::ImplTraitTy::bounds>(ref<TraitBound>))> {};
  static constexpr auto TraitObjectRest =
      many(lit<"+"> + append<&ast::TraitObjectTy::bounds>(TraitBoundOrLifetime)) +
      option(lit<"+">);
  struct TraitObjectTy
      : rule<create<ast::TraitObjectTy>(
             (flag<&ast::TraitObjectTy::isDyn>(kw<"dyn">) +
              append<&ast::TraitObjectTy::bounds>(TraitBoundOrLifetime) + TraitObjectRest) |
             (append<&ast::TraitObjectTy::bounds>(TraitBoundOrLifetime) + lit<"+"> +
              option(append<&ast::TraitObjectTy::bounds>(TraitBoundOrLifetime) + TraitObjectRest)) |
             (append<&ast::TraitObjectTy::bounds>(
                  &(kw<"for"> | lit<"?">) + ref<TraitBound>) + !lit<"+">))> {};
  struct TraitObjectTyOneBound
      : rule<create<ast::TraitObjectTy>(
             (flag<&ast::TraitObjectTy::isDyn>(kw<"dyn">) +
              append<&ast::TraitObjectTy::bounds>(ref<TraitBound>)) |
             append<&ast::TraitObjectTy::bounds>(
                  &(kw<"for"> | lit<"?">) + ref<TraitBound>))> {};

  struct TypeNoBounds
      : rule<create<ast::ParenTy>(lit<"("> +
                                  assign<&ast::ParenTy::inner>(ref<Type>) +
                                  lit<")">)    //
             | ref<ImplTraitTyOneBound>    //
             | ref<TraitObjectTyOneBound>  //
             | create<ast::PathTy>(
                   assign<&ast::PathTy::path>(ref<QualifiedPathInType>))  //
             | create<ast::PathTy>(
                   assign<&ast::PathTy::path>(ref<TypePath>))  //
             |
             create<ast::TupleTy>(
                 lit<"("> +
                 option(append<&ast::TupleTy::elements>(ref<Type>) +
                        lit<","> +
                        option(append<&ast::TupleTy::elements>(ref<Type>) +
                               many(lit<","> + append<&ast::TupleTy::elements>(
                                                   ref<Type>)) +
                               option(lit<",">))) +
                 lit<")">)                     //
             | create<ast::NeverTy>(lit<"!">)  //
             | create<ast::RawPtrTy>(
                   ref<STAR> +
                   (kw<"const"> | flag<&ast::RawPtrTy::isMut>(kw<"mut">)) +
                   assign<&ast::RawPtrTy::inner>(ref<TypeNoBounds>))  //
             | create<ast::RefTy>(
                   ref<AND> + option(assign<&ast::RefTy::lifetime>(Lifetime)) +
                   option(flag<&ast::RefTy::isMut>(kw<"mut">)) +
                   assign<&ast::RefTy::inner>(ref<TypeNoBounds>))  //
             | ref<ArrayOrSliceTy>                                     //
             | create<ast::InferredTy>(ref<UNDERSCORE>)                //
             | ref<BareFnTy>                                       //
             | create<ast::MacroTy>(
                   assign<&ast::MacroTy::path>(ref<SimplePath>) + lit<"!"> +
                   assign<&ast::MacroTy::tokens>(ref<DelimTokenTree>))> {};

  struct Type
      : rule<ref<ImplTraitTy> | ref<TraitObjectTy> |
             ref<TypeNoBounds>> {};

  // Patterns.
  struct RangePatBound
      : rule<create<ast::RangePatBound>(
                 option(flag<&ast::RangePatBound::negative>(lit<"-">)) +
                 assign<&ast::RangePatBound::text>(
                     ref<CHAR_LIT> | ref<BYTE_LIT> | ref<INTEGER_LIT> |
                     ref<FLOAT_LIT>))  //
             | create<ast::RangePatBound>(
                   assign<&ast::RangePatBound::path>(
                       ref<PathInExpression> |
                       ref<QualifiedPathInExpression>))> {};
  struct LiteralPat
      : rule<create<ast::LiteralPat>(
            option(flag<&ast::LiteralPat::negative>(lit<"-">)) +
            assign<&ast::LiteralPat::text>(
                kw<"true"> | kw<"false"> | ref<CHAR_LIT> | ref<BYTE_LIT> |
                ref<STRING_LIT> | ref<RAW_STRING_LIT> |
                ref<BYTE_STRING_LIT> | ref<RAW_BYTE_STRING_LIT> |
                ref<C_STRING_LIT> | ref<RAW_C_STRING_LIT> |
                ref<INTEGER_LIT> | ref<FLOAT_LIT>))> {};
  struct IdentPat
      : rule<create<ast::IdentPat>(
            option(flag<&ast::IdentPat::isRef>(kw<"ref">)) +
            option(flag<&ast::IdentPat::isMut>(kw<"mut">)) +
            assign<&ast::IdentPat::name>(Identifier) +
            option(ref<ATSIGN> + assign<&ast::IdentPat::subPattern>(
                                      ref<PatternNoTopAlt>)))> {};
  struct StructPatField
      : rule<many(append<&ast::StructPatField::attrs>(ref<OuterAttribute>)) +
             ((assign<&ast::StructPatField::name>(ref<INTEGER_LIT>) +
               lit<":"> +
               assign<&ast::StructPatField::pattern>(ref<Pattern>))  //
              | (assign<&ast::StructPatField::name>(Identifier) + lit<":"> +
                 assign<&ast::StructPatField::pattern>(ref<Pattern>))  //
              | (option(flag<&ast::StructPatField::isRef>(kw<"ref">)) +
                 option(flag<&ast::StructPatField::isMut>(kw<"mut">)) +
                 assign<&ast::StructPatField::name>(Identifier)))> {};
  struct StructPat
      : rule<create<ast::StructPat>(
            assign<&ast::StructPat::path>(ref<PathInExpression>) +
            lit<"{"> +
            option((append<&ast::StructPat::fields>(ref<StructPatField>) +
                    many(lit<","> + append<&ast::StructPat::fields>(
                                         ref<StructPatField>)) +
                    option(lit<","> +
                           option(flag<&ast::StructPat::hasRest>(
                               ref<DOTDOT>))))  //
                   | flag<&ast::StructPat::hasRest>(ref<DOTDOT>)) +
            lit<"}">)> {};
  struct TupleStructPat
      : rule<create<ast::TupleStructPat>(
            assign<&ast::TupleStructPat::path>(ref<PathInExpression>) +
            lit<"("> +
            option(append<&ast::TupleStructPat::elements>(ref<Pattern>) +
                   many(lit<","> + append<&ast::TupleStructPat::elements>(
                                       ref<Pattern>)) +
                   option(lit<",">)) +
            lit<")">)> {};
  struct TuplePat
      : rule<create<ast::TuplePat>(
            lit<"("> +
            option(append<&ast::TuplePat::elements>(ref<Pattern>) +
                   lit<","> +
                   option(append<&ast::TuplePat::elements>(ref<Pattern>) +
                          many(lit<","> + append<&ast::TuplePat::elements>(
                                              ref<Pattern>)) +
                          option(lit<",">))) +
            lit<")">)> {};
  struct SlicePat
      : rule<create<ast::SlicePat>(
            lit<"["> +
            option(append<&ast::SlicePat::elements>(ref<Pattern>) +
                   many(lit<","> + append<&ast::SlicePat::elements>(
                                        ref<Pattern>)) +
                   option(lit<",">)) +
            lit<"]">)> {};

  struct RangePat
      : rule<create<ast::RangePat>(
                 flag<&ast::RangePat::inclusive>(ref<DOTDOTEQ>) +
                 assign<&ast::RangePat::end>(ref<RangePatBound>))  //
             | create<ast::RangePat>(
                   assign<&ast::RangePat::start>(ref<RangePatBound>) +
                   ((flag<&ast::RangePat::inclusive>(ref<DOTDOTEQ>) +
                     assign<&ast::RangePat::end>(ref<RangePatBound>))  //
                    | (flag<&ast::RangePat::obsolete>(ref<DOTDOTDOT>) +
                       assign<&ast::RangePat::end>(ref<RangePatBound>))  //
                    | (ref<DOTDOT> + option(assign<&ast::RangePat::end>(ref<RangePatBound>))))) |
             create<ast::RangePat>(ref<DOTDOT> +
                   assign<&ast::RangePat::end>(ref<RangePatBound>))> {};

  struct PatternWithoutRange
      : rule<
            ref<LiteralPat>                          //
            | create<ast::WildcardPat>(ref<UNDERSCORE>)  //
            | create<ast::RestPat>(ref<DOTDOT>)          //
            | create<ast::RefPat>(
                  flag<&ast::RefPat::doubleRef>(ref<ANDAND>) +
                  option(flag<&ast::RefPat::isMut>(kw<"mut">)) +
                  assign<&ast::RefPat::inner>(ref<PatternWithoutRange>))  //
            | create<ast::RefPat>(
                  ref<AND> + option(flag<&ast::RefPat::isMut>(kw<"mut">)) +
                  assign<&ast::RefPat::inner>(ref<PatternWithoutRange>))  //
            | ref<StructPat>                                              //
            | ref<TupleStructPat>                                         //
            | ref<TuplePat>                                               //
            | create<ast::GroupedPat>(
                  lit<"("> + assign<&ast::GroupedPat::inner>(ref<Pattern>) +
                  lit<")">)      //
            | ref<SlicePat>  //
            | create<ast::MacroPat>(
                  assign<&ast::MacroPat::path>(ref<SimplePath>) + lit<"!"> +
                  assign<&ast::MacroPat::tokens>(ref<DelimTokenTree>))  //
            | ref<IdentPat>
            | create<ast::PathPat>(assign<&ast::PathPat::path>(
                  ref<QualifiedPathInExpression>))  //
            | create<ast::PathPat>(
                  assign<&ast::PathPat::path>(ref<PathInExpression>))> {};

  struct PatternNoTopAlt
      : rule<ref<RangePat> | ref<PatternWithoutRange>> {};

  struct Pattern
      : rule<option(lit<"|">) +
             (create<ast::OrPat>(
                  append<&ast::OrPat::alternatives>(ref<PatternNoTopAlt>) +
                  some(lit<"|"> + append<&ast::OrPat::alternatives>(
                                       ref<PatternNoTopAlt>)))  //
              | ref<PatternNoTopAlt>)> {};

  // Expressions.
  struct LiteralExpr
      : rule<create<ast::LiteralExpr>(assign<&ast::LiteralExpr::text>(
            kw<"true"> | kw<"false"> | ref<CHAR_LIT> | ref<STRING_LIT> |
            ref<RAW_STRING_LIT> | ref<BYTE_LIT> | ref<BYTE_STRING_LIT> |
            ref<RAW_BYTE_STRING_LIT> | ref<INTEGER_LIT> |
            ref<C_STRING_LIT> | ref<RAW_C_STRING_LIT> |
            ref<FLOAT_LIT>))> {};
  struct PathExpr
      : rule<create<ast::PathExpr>(assign<&ast::PathExpr::path>(
                 ref<QualifiedPathInExpression>))  //
             | create<ast::PathExpr>(
                   assign<&ast::PathExpr::path>(ref<PathInExpression>))> {
  };
  struct GroupedExpr
      : rule<create<ast::GroupedExpr>(
            lit<"("> +
            many(append<&ast::GroupedExpr::innerAttrs>(ref<InnerAttribute>)) +
            assign<&ast::GroupedExpr::inner>(ref<Expression>) + lit<")">)> {};
  struct ArrayExpr
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
  struct TupleExpr
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
  struct StructExprField
      : rule<many(append<&ast::StructExprField::attrs>(ref<OuterAttribute>)) +
             ((assign<&ast::StructExprField::name>(ref<INTEGER_LIT>) +
               lit<":"> +
               assign<&ast::StructExprField::value>(ref<Expression>))  //
              | (assign<&ast::StructExprField::name>(Identifier) + lit<":"> +
                 assign<&ast::StructExprField::value>(ref<Expression>))  //
              | assign<&ast::StructExprField::name>(Identifier))> {};
  struct StructExpr
      : rule<create<ast::StructExpr>(
                 assign<&ast::StructExpr::path>(ref<PathInExpression>) +
                 lit<"{"> +
                 many(append<&ast::StructExpr::innerAttrs>(
                     ref<InnerAttribute>)) +
                 option((append<&ast::StructExpr::fields>(
                             ref<StructExprField>) +
                         many(lit<","> +
                              append<&ast::StructExpr::fields>(
                                  ref<StructExprField>)) +
                         option(lit<","> +
                                option(ref<DOTDOT> +
                                       assign<&ast::StructExpr::base>(
                                           ref<Expression>))))  //
                        | (ref<DOTDOT> + assign<&ast::StructExpr::base>(
                                             ref<Expression>))) +
                 lit<"}">)> {};
  struct ClosureParam
      : rule<many(append<&ast::ClosureParam::attrs>(ref<OuterAttribute>)) +
             assign<&ast::ClosureParam::pattern>(ref<PatternNoTopAlt>) +
             option(lit<":"> +
                    assign<&ast::ClosureParam::type>(ref<Type>))> {};
  struct ClosureExpr
      : rule<create<ast::ClosureExpr>(
            option(flag<&ast::ClosureExpr::isAsync>(kw<"async">)) +
            option(flag<&ast::ClosureExpr::isMove>(kw<"move">)) +
            (ref<OROR> |
             (lit<"|"> +
              option(
                  append<&ast::ClosureExpr::params>(ref<ClosureParam>) +
                  many(lit<","> + append<&ast::ClosureExpr::params>(
                                      ref<ClosureParam>)) +
                  option(lit<",">)) +
              lit<"|">)) +
            ((ref<RARROW> +
              assign<&ast::ClosureExpr::returnType>(ref<TypeNoBounds>) +
              assign<&ast::ClosureExpr::body>(ref<BlockExpr>))  //
             | assign<&ast::ClosureExpr::body>(ref<Expression>)))> {};

  struct PrimaryExpr
      : rule<ref<LiteralExpr>  //
             | create<ast::AttrExpr>(
                   some(append<&ast::AttrExpr::attrs>(ref<OuterAttribute>)) +
                   assign<&ast::AttrExpr::value>(ref<Expression>))  //
             | ref<ClosureExpr>                                 //
             | ref<StructExpr>                                  //
             | ref<GroupedExpr>                                 //
             | ref<ArrayExpr>                                   //
             | ref<TupleExpr>                                   //
             | ref<ExpressionWithBlock>                             //
             |
             create<ast::MacroExpr>(
                 assign<&ast::MacroExpr::path>(ref<SimplePath>) + lit<"!"> +
                 assign<&ast::MacroExpr::tokens>(ref<DelimTokenTree>))  //
             |
             create<ast::ContinueExpr>(
                 kw<"continue"> +
                 option(assign<&ast::ContinueExpr::label>(
                     ref<LIFETIME_OR_LABEL>)))  //
             | create<ast::BreakExpr>(
                   kw<"break"> +
                   option(
                       assign<&ast::BreakExpr::label>(ref<LIFETIME_OR_LABEL>)) +
                   option(assign<&ast::BreakExpr::value>(ref<Expression>)))  //
             | create<ast::ReturnExpr>(
                   kw<"return"> +
                   option(assign<&ast::ReturnExpr::value>(ref<Expression>)))  //
             | ref<PathExpr>> {};

  struct PostfixExpr
      : rule<ref<PrimaryExpr> +
             many(
                 nest<ast::MethodCallExpr, &ast::MethodCallExpr::receiver>(
                     ref<DOT> +
                     assign<&ast::MethodCallExpr::method>(Identifier) +
                     option(ref<PATHSEP> +
                            assign<&ast::MethodCallExpr::genericArgs>(
                                ref<GenericArgs>)) +
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
                 option((flag<&ast::BorrowExpr::isRaw>(kw<"raw">) +
                         (flag<&ast::BorrowExpr::isMut>(kw<"mut">) | kw<"const">)) |
                        flag<&ast::BorrowExpr::isMut>(kw<"mut">)) +
                 assign<&ast::BorrowExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::BorrowExpr>(
                   ref<AND> +
                   option((flag<&ast::BorrowExpr::isRaw>(kw<"raw">) +
                           (flag<&ast::BorrowExpr::isMut>(kw<"mut">) | kw<"const">)) |
                          flag<&ast::BorrowExpr::isMut>(kw<"mut">)) +
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
                 assign<&ast::CastExpr::type>(ref<TypeNoBounds>)))> {};

  struct ArithExpr
      : infix<CastExpr, ast::BinaryExpr,
              levels(left(ref<STAR> | as<ast::Operator::divide>(lit<"/", !cr<"=/*">>) | as<ast::Operator::remainder>(lit<"%", !cr<"=">>)),
                     left(as<ast::Operator::add>(lit<"+", !cr<"=">>) | ref<MINUS>))> {};

  struct ShiftExpr
      : rule<ref<ArithExpr> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<ShlOp> | ref<ShrOp>) +
                 assign<&ast::BinaryExpr::right>(ref<ArithExpr>)))> {};

  struct BitwiseExpr
      : infix<ShiftExpr, ast::BinaryExpr,
              levels(left(ref<AND>), left(as<ast::Operator::bit_xor>(lit<"^", !cr<"=">>)), left(as<ast::Operator::bit_or>(lit<"|", !cr<"|=">>)))> {};

  struct ComparisonExpr
      : infix<BitwiseExpr, ast::BinaryExpr,
              left(as<ast::Operator::eq>(lit<"==">) | as<ast::Operator::ne>(lit<"!=">) | ref<LT> | ref<GT> | as<ast::Operator::le>(lit<"<=">) |
                   as<ast::Operator::ge>(lit<">=">))> {};

  struct LogicalOrExpr
      : infix<ComparisonExpr, ast::BinaryExpr,
              levels(left(ref<ANDAND>), left(ref<OROR>))> {};

  struct RangeExpr
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
      : infix<RangeExpr, ast::BinaryExpr,
              right(ref<EQ> | as<ast::Operator::add_assign>(lit<"+=">) | as<ast::Operator::sub_assign>(lit<"-=">) | as<ast::Operator::mul_assign>(lit<"*=">) |
                    as<ast::Operator::div_assign>(lit<"/=">) | as<ast::Operator::rem_assign>(lit<"%=">) | as<ast::Operator::bitand_assign>(lit<"&=">) | as<ast::Operator::bitor_assign>(lit<"|=">) |
                    as<ast::Operator::bitxor_assign>(lit<"^=">) | as<ast::Operator::shl_assign>(lit<"<<=">) | as<ast::Operator::shr_assign>(lit<">>=">))> {};

  struct BlockExpr
      : rule<create<ast::BlockExpr>(
            lit<"{"> +
            many(append<&ast::BlockExpr::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::BlockExpr::statements>(ref<Statement>) |
                 lit<";">) +
            option(assign<&ast::BlockExpr::tailExpr>(ref<Expression>)) +
            lit<"}">)> {};
  struct AsyncBlockExpr
      : rule<create<ast::AsyncBlockExpr>(
            kw<"async">
            + option(flag<&ast::AsyncBlockExpr::isMove>(kw<"move">)) +
            assign<&ast::AsyncBlockExpr::body>(ref<BlockExpr>))> {};
  struct UnsafeBlockExpr
      : rule<create<ast::UnsafeBlockExpr>(
            kw<"unsafe">
            + assign<&ast::UnsafeBlockExpr::body>(ref<BlockExpr>))> {};

  struct InfiniteLoopExpr
      : rule<create<ast::InfiniteLoopExpr>(
            option(assign<&ast::InfiniteLoopExpr::label>(
                       ref<LIFETIME_OR_LABEL>) +
                   lit<":">) +
            kw<"loop"> +
            assign<&ast::InfiniteLoopExpr::body>(ref<BlockExpr>))> {};
  struct WhileLetLoopExpr
      : rule<create<ast::WhileLetLoopExpr>(
            option(assign<&ast::WhileLetLoopExpr::label>(
                       ref<LIFETIME_OR_LABEL>) +
                   lit<":">) +
            kw<"while"> + kw<"let"> +
            assign<&ast::WhileLetLoopExpr::pattern>(ref<Pattern>) +
            ref<EQ> +
            assign<&ast::WhileLetLoopExpr::value>(ref<Expression>) +
            assign<&ast::WhileLetLoopExpr::body>(ref<BlockExpr>))> {};
  struct WhileLoopExpr
      : rule<create<ast::WhileLoopExpr>(
            option(assign<&ast::WhileLoopExpr::label>(
                       ref<LIFETIME_OR_LABEL>) +
                   lit<":">) +
            kw<"while"> + assign<&ast::WhileLoopExpr::cond>(ref<Expression>) +
            assign<&ast::WhileLoopExpr::body>(ref<BlockExpr>))> {};
  struct ForLoopExpr
      : rule<create<ast::ForLoopExpr>(
            option(assign<&ast::ForLoopExpr::label>(ref<LIFETIME_OR_LABEL>) +
                   lit<":">) +
            kw<"for"> + assign<&ast::ForLoopExpr::pattern>(ref<Pattern>) +
            kw<"in"> + assign<&ast::ForLoopExpr::iter>(ref<Expression>) +
            assign<&ast::ForLoopExpr::body>(ref<BlockExpr>))> {};
  struct LoopExprDispatch
      : rule<ref<WhileLetLoopExpr> | ref<WhileLoopExpr> |
             ref<ForLoopExpr> | ref<InfiniteLoopExpr>> {};

  struct IfExpr
      : rule<create<ast::IfExpr>(
            kw<"if"> + assign<&ast::IfExpr::cond>(ref<Expression>) +
            assign<&ast::IfExpr::thenBranch>(ref<BlockExpr>) +
            option(commit(kw<"else">, assign<&ast::IfExpr::elseBranch>(
                         ref<BlockExpr> | ref<IfExpr> |
                         ref<IfLetExpr>))))> {};
  struct IfLetExpr
      : rule<create<ast::IfLetExpr>(
            kw<"if"> + kw<"let"> +
            assign<&ast::IfLetExpr::pattern>(ref<Pattern>) + ref<EQ> +
            assign<&ast::IfLetExpr::value>(ref<Expression>) +
            assign<&ast::IfLetExpr::thenBranch>(ref<BlockExpr>) +
            option(commit(kw<"else">, assign<&ast::IfLetExpr::elseBranch>(
                         ref<BlockExpr> | ref<IfExpr> |
                         ref<IfLetExpr>))))> {};

  static constexpr auto MatchArmPrefix =
      many(append<&ast::MatchArm::attrs>(ref<OuterAttribute>)) +
      assign<&ast::MatchArm::pattern>(ref<Pattern>) +
      option(kw<"if"> + assign<&ast::MatchArm::guard>(ref<Expression>)) + ref<FATARROW>;
  struct MatchArmBody
      : rule<create<ast::MatchArm>(MatchArmPrefix +
             ((assign<&ast::MatchArm::body>(ref<ExpressionWithBlock>) + option(lit<",">)) |
              (assign<&ast::MatchArm::body>(ref<Expression>) + lit<",">)))> {};
  struct LastMatchArm
      : rule<create<ast::MatchArm>(MatchArmPrefix +
             assign<&ast::MatchArm::body>(ref<Expression>))> {};
  struct MatchExpr
      : rule<create<ast::MatchExpr>(
            kw<"match"> + assign<&ast::MatchExpr::subject>(ref<Expression>) + lit<"{"> +
            many(append<&ast::MatchExpr::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::MatchExpr::arms>(ref<MatchArmBody>)) +
            option(append<&ast::MatchExpr::arms>(ref<LastMatchArm>)) + lit<"}">)> {};

  // `#[attr] if cond {} ` at statement level needs the block-shaped
  // reading (no mandatory trailing `;`): the general `AttrExpr` in Primary
  // only reaches it through `Expression`, which does require one.
  struct AttrExpressionWithBlock
      : rule<create<ast::AttrExpr>(
            some(append<&ast::AttrExpr::attrs>(ref<OuterAttribute>)) +
            assign<&ast::AttrExpr::value>(ref<ExpressionWithBlock>))> {};
  struct ExpressionWithBlock
      : rule<ref<BlockExpr> | ref<AsyncBlockExpr> |
             ref<UnsafeBlockExpr> | ref<LoopExprDispatch> |
             ref<IfLetExpr> | ref<IfExpr> | ref<MatchExpr> |
             ref<AttrExpressionWithBlock>> {};

  // Statements.
  struct LetStatement
      : rule<create<ast::LetStmt>(
            many(append<&ast::LetStmt::attrs>(ref<OuterAttribute>)) +
            kw<"let"> +
            assign<&ast::LetStmt::pattern>(ref<PatternNoTopAlt>) +
            option(lit<":"> + assign<&ast::LetStmt::type>(ref<Type>)) +
            option(ref<EQ> + assign<&ast::LetStmt::value>(ref<Expression>) +
                   option(kw<"else"> + assign<&ast::LetStmt::elseBranch>(
                                           ref<BlockExpr>))) +
            lit<";">)> {};
  struct ExpressionStatement
      : rule<create<ast::ExprStmt>(
                 assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">)  //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<ExpressionWithBlock>) +
                   option(lit<";">))> {};

  struct Statement
      : rule<ref<Item> | ref<LetStatement> |
             ref<ExpressionStatement>> {};

  // Items.
  static constexpr auto ItemAttrs =
      many(append<&ast::Item::attrs>(ref<OuterAttribute>));

  struct SelfParam
      : rule<many(append<&ast::FnParam::attrs>(ref<OuterAttribute>)) +
             ((option(flag<&ast::FnParam::selfByRef>(ref<AND>) +
                      option(assign<&ast::FnParam::selfLifetime>(Lifetime))) +
               option(flag<&ast::FnParam::isMut>(kw<"mut">)) +
               flag<&ast::FnParam::isSelf>(kw<"self">))  //
              | (option(flag<&ast::FnParam::isMut>(kw<"mut">)) +
                 flag<&ast::FnParam::isSelf>(kw<"self">) + lit<":"> +
                 assign<&ast::FnParam::selfType>(ref<Type>)))> {};
  struct FunctionParam
      : rule<many(append<&ast::FnParam::attrs>(ref<OuterAttribute>)) +
             ((assign<&ast::FnParam::pattern>(ref<Pattern>) + lit<":"> +
               (assign<&ast::FnParam::type>(ref<Type>) |
                flag<&ast::FnParam::variadic>(ref<DOTDOTDOT>)))  //
              | flag<&ast::FnParam::variadic>(ref<DOTDOTDOT>)  //
              | assign<&ast::FnParam::type>(ref<Type>))> {};
  struct Function
      : rule<create<ast::Function>(
            ItemAttrs + option(assign<&ast::Function::vis>(ref<Visibility>)) +
            option(flag<&ast::Function::isConst>(kw<"const">)) +
            option(flag<&ast::Function::isAsync>(kw<"async">)) +
            option(flag<&ast::Function::isUnsafe>(kw<"unsafe">)) +
            option(flag<&ast::Function::isExtern>(kw<"extern">) +
                   option(assign<&ast::Function::abi>(ref<STRING_LIT>))) +
            kw<"fn"> + assign<&ast::Function::name>(Identifier) +
            option(assign<&ast::Function::generics>(ref<GenericParams>)) +
            lit<"("> +
            option((append<&ast::Function::params>(ref<SelfParam>) +
                    option(lit<",">))  //
                   | (option(append<&ast::Function::params>(
                                 ref<SelfParam>) +
                             lit<",">) +
                      append<&ast::Function::params>(ref<FunctionParam>) +
                      many(lit<","> + append<&ast::Function::params>(
                                          ref<FunctionParam>)) +
                      option(lit<",">))) +
            lit<")"> +
            option(ref<RARROW> +
                   assign<&ast::Function::returnType>(ref<Type>)) +
            option(assign<&ast::Function::whereClause>(ref<WhereClause>)) +
            (assign<&ast::Function::body>(ref<BlockExpr>) |
             lit<";">))> {};

  struct TypeAlias
      : rule<create<ast::TypeAlias>(
            ItemAttrs +
            option(assign<&ast::TypeAlias::vis>(ref<Visibility>)) +
            kw<"type"> + assign<&ast::TypeAlias::name>(Identifier) +
            option(assign<&ast::TypeAlias::generics>(ref<GenericParams>)) +
            option(assign<&ast::TypeAlias::whereClause>(
                ref<WhereClause>)) +
            option(ref<EQ> + assign<&ast::TypeAlias::type>(ref<Type>)) +
            lit<";">)> {};

  struct StructField
      : rule<many(append<&ast::StructField::attrs>(ref<OuterAttribute>)) +
             option(assign<&ast::StructField::vis>(ref<Visibility>)) +
             assign<&ast::StructField::name>(Identifier) + lit<":"> +
             assign<&ast::StructField::type>(ref<Type>)> {};
  struct TupleField
      : rule<many(append<&ast::TupleField::attrs>(ref<OuterAttribute>)) +
             option(assign<&ast::TupleField::vis>(ref<Visibility>)) +
             assign<&ast::TupleField::type>(ref<Type>)> {};
  struct Struct
      : rule<create<ast::Struct>(
                 ItemAttrs +
                 option(assign<&ast::Struct::vis>(ref<Visibility>)) +
                 kw<"struct"> + assign<&ast::Struct::name>(Identifier) +
                 option(assign<&ast::Struct::generics>(
                     ref<GenericParams>)) +
                 option(assign<&ast::Struct::whereClause>(
                     ref<WhereClause>)) +
                 ((lit<"{"> +
                   option(append<&ast::Struct::fields>(ref<StructField>) +
                          many(lit<","> + append<&ast::Struct::fields>(
                                              ref<StructField>)) +
                          option(lit<",">)) +
                   lit<"}">)  //
                  | lit<";">))  //
             | create<ast::Struct>(
                   ItemAttrs +
                   option(assign<&ast::Struct::vis>(ref<Visibility>)) +
                   kw<"struct"> + assign<&ast::Struct::name>(Identifier) +
                   option(assign<&ast::Struct::generics>(
                       ref<GenericParams>)) +
                   flag<&ast::Struct::isTuple>(lit<"(">) +
                   option(append<&ast::Struct::tupleFields>(
                              ref<TupleField>) +
                          many(lit<","> + append<&ast::Struct::tupleFields>(
                                              ref<TupleField>)) +
                          option(lit<",">)) +
                   lit<")"> +
                   option(assign<&ast::Struct::whereClause>(
                       ref<WhereClause>)) +
                   lit<";">)> {};

  struct EnumVariant
      : rule<many(append<&ast::EnumVariant::attrs>(ref<OuterAttribute>)) +
             option(assign<&ast::EnumVariant::vis>(ref<Visibility>)) +
             assign<&ast::EnumVariant::name>(Identifier) +
             option(
                 (flag<&ast::EnumVariant::isTuple>(lit<"(">) +
                  option(append<&ast::EnumVariant::tupleFields>(
                             ref<TupleField>) +
                         many(lit<","> + append<&ast::EnumVariant::tupleFields>(
                                             ref<TupleField>)) +
                         option(lit<",">)) +
                  lit<")">)  //
                 | (flag<&ast::EnumVariant::isStruct>(lit<"{">) +
                    option(append<&ast::EnumVariant::structFields>(
                               ref<StructField>) +
                           many(lit<","> +
                                append<&ast::EnumVariant::structFields>(
                                    ref<StructField>)) +
                           option(lit<",">)) +
                    lit<"}">)  //
                 | (ref<EQ> + assign<&ast::EnumVariant::discriminant>(
                                  ref<Expression>)))> {};
  struct Enum
      : rule<create<ast::Enum>(
            ItemAttrs + option(assign<&ast::Enum::vis>(ref<Visibility>)) +
            kw<"enum"> + assign<&ast::Enum::name>(Identifier) +
            option(assign<&ast::Enum::generics>(ref<GenericParams>)) +
            option(assign<&ast::Enum::whereClause>(ref<WhereClause>)) +
            lit<"{"> +
            option(append<&ast::Enum::variants>(ref<EnumVariant>) +
                   many(lit<","> + append<&ast::Enum::variants>(
                                       ref<EnumVariant>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct Union
      : rule<create<ast::Union>(
            ItemAttrs + option(assign<&ast::Union::vis>(ref<Visibility>)) +
            kw<"union"> + assign<&ast::Union::name>(Identifier) +
            option(assign<&ast::Union::generics>(ref<GenericParams>)) +
            option(assign<&ast::Union::whereClause>(ref<WhereClause>)) +
            lit<"{"> +
            append<&ast::Union::fields>(ref<StructField>) +
            many(lit<","> +
                 append<&ast::Union::fields>(ref<StructField>)) +
            option(lit<",">) + lit<"}">)> {};

  struct ConstItem
      : rule<create<ast::ConstItem>(
            ItemAttrs +
            option(assign<&ast::ConstItem::vis>(ref<Visibility>)) +
            kw<"const"> +
            (assign<&ast::ConstItem::name>(Identifier) | ref<UNDERSCORE>) +
            lit<":"> + assign<&ast::ConstItem::type>(ref<Type>) +
            option(ref<EQ> +
                   assign<&ast::ConstItem::value>(ref<Expression>)) +
            lit<";">)> {};
  struct StaticItem
      : rule<create<ast::StaticItem>(
            ItemAttrs +
            option(assign<&ast::StaticItem::vis>(ref<Visibility>)) +
            kw<"static"> +
            option(flag<&ast::StaticItem::isMut>(kw<"mut">)) +
            assign<&ast::StaticItem::name>(Identifier) + lit<":"> +
            assign<&ast::StaticItem::type>(ref<Type>) +
            option(ref<EQ> +
                   assign<&ast::StaticItem::value>(ref<Expression>)) +
            lit<";">)> {};

  template <auto Member>
  static constexpr auto MacroItemTokens =
      (&lit<"{"> + assign<Member>(ref<DelimTokenTree>) + option(lit<";">)) |
      (!lit<"{"> + assign<Member>(ref<DelimTokenTree>) + lit<";">);
  struct MacroItem
      : rule<create<ast::MacroStmt>(
            ItemAttrs + assign<&ast::MacroStmt::path>(ref<SimplePath>) +
            lit<"!"> + MacroItemTokens<&ast::MacroStmt::tokens>)> {};

  static constexpr auto AssociatedItem = ref<TypeAlias> |
                                         ref<ConstItem> |
                                         ref<Function> | ref<MacroItem>;

  struct TraitRule
      : rule<create<ast::Trait>(
            ItemAttrs + option(assign<&ast::Trait::vis>(ref<Visibility>)) +
            option(flag<&ast::Trait::isUnsafe>(kw<"unsafe">)) +
            kw<"trait"> + assign<&ast::Trait::name>(Identifier) +
            option(assign<&ast::Trait::generics>(ref<GenericParams>)) +
            option(lit<":"> +
                   option(append<&ast::Trait::supertraits>(
                              TraitBoundOrLifetime) +
                          many(lit<"+"> + append<&ast::Trait::supertraits>(
                                              TraitBoundOrLifetime)) +
                          option(lit<"+">))) +
            option(assign<&ast::Trait::whereClause>(ref<WhereClause>)) +
            lit<"{"> +
            many(append<&ast::Trait::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::Trait::items>(AssociatedItem)) + lit<"}">)> {};

  struct Impl
      : rule<create<ast::Impl>(
            ItemAttrs +
            option(flag<&ast::Impl::isUnsafe>(kw<"unsafe">)) +
            kw<"impl"> +
            option(assign<&ast::Impl::generics>(ref<GenericParams>)) +
            ((option(flag<&ast::Impl::isNegative>(lit<"!">)) +
              assign<&ast::Impl::traitRef>(ref<TypePath>) + kw<"for"> +
              assign<&ast::Impl::type>(ref<Type>))  //
             | assign<&ast::Impl::type>(ref<Type>)) +
            option(assign<&ast::Impl::whereClause>(ref<WhereClause>)) +
            lit<"{"> +
            many(append<&ast::Impl::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::Impl::items>(AssociatedItem)) + lit<"}">)> {};

  struct ExternBlock
      : rule<create<ast::ExternBlock>(
            ItemAttrs +
            option(assign<&ast::ExternBlock::vis>(ref<Visibility>)) +
            option(flag<&ast::ExternBlock::isUnsafe>(kw<"unsafe">)) +
            kw<"extern"> +
            option(assign<&ast::ExternBlock::abi>(ref<STRING_LIT>)) +
            lit<"{"> +
            many(append<&ast::ExternBlock::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::ExternBlock::items>(
                ref<StaticItem> | ref<Function> |
                ref<MacroItem>)) +
            lit<"}">)> {};

  struct ModuleItem
      : rule<create<ast::Module>(
            ItemAttrs + option(assign<&ast::Module::vis>(ref<Visibility>)) +
            option(flag<&ast::Module::isUnsafe>(kw<"unsafe">)) +
            kw<"mod"> + assign<&ast::Module::name>(Identifier) +
            (lit<";">  //
             | (flag<&ast::Module::isInline>(lit<"{">) +
                many(append<&ast::Module::innerAttrs>(ref<InnerAttribute>)) +
                many(append<&ast::Module::items>(ref<Item>)) +
                lit<"}">)))> {};

  struct ExternCrate
      : rule<create<ast::ExternCrate>(
            ItemAttrs +
            option(assign<&ast::ExternCrate::vis>(ref<Visibility>)) +
            kw<"extern"> + kw<"crate"> +
            assign<&ast::ExternCrate::name>(Identifier | kw<"self">) +
            option(kw<"as"> + assign<&ast::ExternCrate::asName>(
                                   Identifier | ref<UNDERSCORE>)) +
            lit<";">)> {};

  struct UseTree;
  static constexpr auto UseGroup =
      flag<&ast::UseTree::isGlob>(ref<STAR>) |
      (lit<"{"> + option(some(append<&ast::UseTree::nested>(ref<UseTree>), lit<",">) +
       option(lit<",">)) + lit<"}">);
  struct UseTree
      : rule<create<ast::UseTree>(
             (assign<&ast::UseTree::path>(ref<SimplePath>) +
              ((ref<PATHSEP> + UseGroup) |
               option(kw<"as"> + assign<&ast::UseTree::asName>(Identifier | ref<UNDERSCORE>)))) |
             (option(flag<&ast::UseTree::leadingSep>(ref<PATHSEP>)) + UseGroup))> {};
  struct UseDecl
      : rule<create<ast::UseDecl>(
            ItemAttrs + option(assign<&ast::UseDecl::vis>(ref<Visibility>)) +
            kw<"use"> + assign<&ast::UseDecl::tree>(ref<UseTree>) +
            lit<";">)> {};

  struct MacroRulesDef
      : rule<create<ast::MacroRulesDef>(
            ItemAttrs + kw<"macro_rules"> + lit<"!"> +
            assign<&ast::MacroRulesDef::name>(Identifier) +
            MacroItemTokens<&ast::MacroRulesDef::tokens>)> {};

  struct Item
      : rule<ref<ModuleItem> | ref<ExternCrate> | ref<UseDecl> |
             ref<Function> | ref<TypeAlias> | ref<Struct> |
             ref<Enum> | ref<Union> | ref<ConstItem> |
             ref<StaticItem> | ref<TraitRule> | ref<Impl> |
             ref<ExternBlock> | ref<MacroRulesDef> |
             ref<MacroItem>> {};

  struct Crate
      : rule<create<ast::Crate>(
            option(lit<"\xEF\xBB\xBF">) + option(ref<SHEBANG>) +
            many(append<&ast::Crate::innerAttrs>(ref<InnerAttribute>)) +
            many(append<&ast::Crate::items>(ref<Item>)))> {};

  using entry = Crate;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT, INNER_LINE_DOC,
                       OUTER_LINE_DOC>;
  static constexpr auto word_boundary = uc::XID_Continue;
};

}  // namespace rust
