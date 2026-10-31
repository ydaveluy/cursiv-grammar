// Haskell syntax; operator fixities are resolved after parsing.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "haskell/core/ast.hpp"

namespace haskell {

inline constexpr auto quasi_name = [](std::string_view t) {
  return t.substr(1);
};

inline constexpr auto quasi_body = [](std::string_view t) {
  return t.substr(1, t.size() - 3);
};

struct Haskell : cursiv::GrammarSpec {
  static constexpr auto symbolChar =
      cr<R"(!#$%&*+./<=>?@\\^|~:-)"> |
      (!cr<R"(\x00-\x7f)"> +
       (uc::Pc | uc::Pd | uc::Po | uc::Sm | uc::Sc | uc::Sk | uc::So));
  // TRIVIA AND LAYOUT
  // =================
  // This port reads explicit NEWLINE separators outside brackets, with
  // indented blocks and inline blocks expressed below. It does not yet model
  // every Haskell layout continuation; that remains a limitation of this port.

  static constexpr auto horizontalWhite =
      cr<R"( \t\v\u0085)"> | uc::Zs | uc::Zl | uc::Zp;
  static constexpr auto newline = eol | lit<"\f">;
  static constexpr auto whitechar = horizontalWhite | newline;
  struct WS : terminal<some(horizontalWhite)> {};
  static constexpr auto comment_body =
      lit<"--"> + many(lit<"-">) + !symbolChar + many(cr<R"(^\r\n\f)">);
  struct NestedComment
      : terminal<lit<"{-"> +
                 many(ref<NestedComment> | (!lit<"{-"> + !lit<"-}"> + any)) +
                 lit<"-}">> {};
  // Only the outer opener excludes pragmas; an inner `{-#` still nests.
  static constexpr auto block_comment = !lit<"{-#"> + ref<NestedComment>;
  // A line with nothing but whitespace and comments does not end a logical
  // line: folded into NEWLINE, as python's port does. The indentation is read
  // where NEWLINE ends, so a comment line at any column decides nothing.
  static constexpr auto blank_line = many(horizontalWhite) +
                                     many(block_comment + many(horizontalWhite)) +
                                     option(comment_body) + newline;
  struct NEWLINE : terminal<newline + many(blank_line)> {};
  struct SL_COMMENT : terminal<comment_body> {};
  struct ML_COMMENT : terminal<block_comment> {};

  static constexpr auto indent_open = lit<"("> | lit<"["> | lit<"{">;
  static constexpr auto indent_close = lit<")"> | lit<"]"> | lit<"}">;
  using indent_newline = NEWLINE;

  // NAMES
  // =====

  static constexpr auto smallChar = cr<"_a-z"> | uc::Ll | uc::Lo;
  static constexpr auto largeChar = cr<"A-Z"> | uc::Lu | uc::Lt;
  static constexpr auto idTail = smallChar | largeChar | uc::Nd | uc::Nl |
                                 uc::No | uc::Lm | uc::Mn | cr<"'">;

  static constexpr auto conidBody = largeChar + many(idTail) + many(lit<"#">);

  // A lone `_` is the wildcard, never a variable.
  struct VARID_TOK
      : terminal<!kw<"_"> + smallChar + many(idTail) + many(lit<"#">)> {};
  struct CONID_TOK : terminal<conidBody> {};
  // A qualifying "A.B." prefix, one contiguous run: no
  // whitespace or comment may sit inside a qualified name.
  struct QUAL_TOK : terminal<some(conidBody + lit<".">)> {};
  // A full (possibly dotted) module name -- "Data.Kind", not just its
  // qualifying prefix.
  struct MODID_TOK : terminal<conidBody + many(lit<"."> + conidBody)> {};

  static constexpr auto NoMoreSym = !symbolChar;
  // A `:` opens a consym, never a varsym (Haskell 2010, 2.4).
  struct VARSYM_TOK : terminal<!lit<":"> + some(symbolChar)> {};
  struct CONSYM_TOK : terminal<lit<":"> + some(symbolChar)> {};
  // Reserved operators end at a complete symbol boundary.
  static constexpr auto ReservedOpExact =
      lit<"->", NoMoreSym> | lit<"<-", NoMoreSym> | lit<"=>", NoMoreSym> |
      lit<"..", NoMoreSym> | lit<"=", NoMoreSym> | lit<"\\", NoMoreSym> |
      lit<"@", NoMoreSym> | lit<"~", NoMoreSym> | lit<"|", NoMoreSym>;
  struct GeneralVarSym : rule<!ReservedOpExact + ref<VARSYM_TOK>> {};
  struct GeneralConSym : rule<!lit<"::", NoMoreSym> + ref<CONSYM_TOK>> {};

  static constexpr auto DCOLON = lit<"::", NoMoreSym>;
  static constexpr auto RARROW = lit<"->", NoMoreSym>;
  static constexpr auto LARROW = lit<"<-", NoMoreSym>;
  static constexpr auto DARROW = lit<"=>", NoMoreSym>;
  static constexpr auto EQSIGN = lit<"=", NoMoreSym>;
  static constexpr auto DOTDOT = lit<"..", NoMoreSym>;
  static constexpr auto PIPE = lit<"|", NoMoreSym>;
  static constexpr auto BACKSLASH = lit<"\\", NoMoreSym>;
  static constexpr auto ATSIGN = lit<"@", NoMoreSym>;
  static constexpr auto TILDE = lit<"~", NoMoreSym>;
  static constexpr auto COLON = lit<":", NoMoreSym>;

  // Contextual keywords can also be identifiers.
  struct AsKw : terminal<kw<"as">> {};
  struct QualifiedKw : terminal<kw<"qualified">> {};
  struct HidingKw : terminal<kw<"hiding">> {};
  struct ExportKw : terminal<kw<"export">> {};
  struct StdcallKw : terminal<kw<"stdcall">> {};
  struct CcallKw : terminal<kw<"ccall">> {};
  struct CapiKw : terminal<kw<"capi">> {};
  struct CppCallKw : terminal<kw<"cplusplus">> {};
  struct JsCallKw : terminal<kw<"javascript">> {};
  struct StockKw : terminal<kw<"stock">> {};
  struct AnyclassKw : terminal<kw<"anyclass">> {};
  struct ViaKw : terminal<kw<"via">> {};
  struct UnsafeKw : terminal<kw<"unsafe">> {};
  struct SafeKw : terminal<kw<"safe">> {};
  struct InterruptibleKw : terminal<kw<"interruptible">> {};
  static constexpr auto SoftKw =
      ref<AsKw> | ref<QualifiedKw> | ref<HidingKw> | ref<ExportKw> |
      ref<StdcallKw> | ref<CcallKw> | ref<CapiKw> | ref<CppCallKw> |
      ref<JsCallKw> | ref<StockKw> | ref<AnyclassKw> | ref<ViaKw> |
      ref<UnsafeKw> | ref<SafeKw> | ref<InterruptibleKw>;

  struct VarIdTerm : rule<ref<VARID_TOK> | SoftKw> {};

  // OPERATORS
  // =========

  struct BacktickVarOp
      : rule<lit<"`"> + option(ref<QUAL_TOK>) + ref<VarIdTerm> + lit<"`">> {};
  struct BacktickConOp
      : rule<lit<"`"> + option(ref<QUAL_TOK>) + ref<CONID_TOK> + lit<"`">> {};
  static constexpr auto QConSymBody = COLON | ref<GeneralConSym>;
  struct SymbolOp
      : rule<option(ref<QUAL_TOK>) + (ref<GeneralVarSym> | QConSymBody)> {};
  static constexpr auto Qop =
      ref<SymbolOp> | ref<BacktickVarOp> | ref<BacktickConOp>;

  // LITERALS
  // ========

  template <auto Digit>
  static constexpr auto digits = Digit + many(many(lit<"_">) + Digit);
  static constexpr auto decimal = digits<d>;
  static constexpr auto hexadecimal = digits<cr<"0-9a-fA-F">>;
  static constexpr auto exponent_ =
      many(lit<"_">) + cr<"eE"> + option(cr<"+-">) + decimal;
  static constexpr auto binExponent =
      many(lit<"_">) + cr<"pP"> + option(cr<"+-">) + decimal;
  struct DECIMAL_TOK : terminal<decimal> {};
  struct OCTAL_TOK
      : terminal<lit<"0"> + cr<"oO"> + many(lit<"_">) + digits<cr<"0-7">>> {};
  struct BINARY_TOK
      : terminal<lit<"0"> + cr<"bB"> + many(lit<"_">) + digits<cr<"01">>> {};
  struct HEX_TOK
      : terminal<lit<"0"> + cr<"xX"> + many(lit<"_">) + hexadecimal> {};
  struct FLOAT_TOK
      : terminal<(decimal + lit<"."> + decimal + option(exponent_)) |
                 (decimal + exponent_) |
                 (lit<"0"> + cr<"xX"> + many(lit<"_">) + hexadecimal +
                  ((lit<"."> + hexadecimal + option(binExponent)) |
                   binExponent))> {};

  static constexpr auto charEscapeBody =
      cr<R"(abfnrtv\\"')"> |
      (lit<"x"> + some(cr<"0-9a-fA-F">)) |
      (lit<"o"> + some(cr<"0-7">)) | some(d) |
      (lit<"^"> + (cr<"A-Z@_"> | lit<"["> | lit<"\\"> | lit<"]"> | lit<"^">)) |
      lit<"NUL"> | lit<"SOH"> | lit<"STX"> | lit<"ETX"> | lit<"EOT"> |
      lit<"ENQ"> | lit<"ACK"> | lit<"BEL"> | lit<"BS"> | lit<"HT"> |
      lit<"LF"> | lit<"VT"> | lit<"FF"> | lit<"CR"> | lit<"SO"> |
      lit<"SI"> | lit<"DLE"> | lit<"DC1"> | lit<"DC2"> | lit<"DC3"> |
      lit<"DC4"> | lit<"NAK"> | lit<"SYN"> | lit<"ETB"> | lit<"CAN"> |
      lit<"EM"> | lit<"SUB"> | lit<"ESC"> | lit<"FS"> | lit<"GS"> |
      lit<"RS"> | lit<"US"> | lit<"SP"> | lit<"DEL">;
  static constexpr auto charEscape = lit<"\\"> + charEscapeBody;
  // Literal whitespace is restricted to an ordinary space. A string gap
  // consumes both backslashes and one or more white characters as one token.
  static constexpr auto literalChar =
      (!horizontalWhite + cr<R"(^\x00-\x1f\x7f-\x9f\\)">) | lit<" ">;
  struct CHAR_TOK
      : terminal<lit<"'"> + (charEscape | (!lit<"'"> + literalChar)) + lit<"'">> {};
  struct STRING_TOK
      : terminal<lit<"\""> +
                 many(charEscape | (lit<"\\"> + lit<"&">) |
                      (lit<"\\"> + some(whitechar) + lit<"\\">) |
                      (!lit<"\""> + literalChar)) + lit<"\"">> {};

  struct IntLit : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(
                          ref<HEX_TOK> | ref<OCTAL_TOK> | ref<BINARY_TOK> |
                          ref<DECIMAL_TOK>))> {};
  struct FloatLit : rule<assign<&ast::FloatLit::text>(ref<FLOAT_TOK>)> {};
  struct CharLit
      : rule<assign<&ast::CharLit::text>(ref<CHAR_TOK>)> {
  };
  struct StringLit : rule<assign<&ast::StringLit::text>(ref<STRING_TOK>)> {
  };

  // A pragma body from right after its keyword through `#-}`, for the
  // pragmas this port accepts but does not model further.
  struct PragmaTail
      : terminal<many(cr<"^#"> | (lit<"#"> + !lit<"-}">)) + lit<"#-}">> {};

  // FORWARD DECLARATIONS
  // ====================

  struct Aexp;
  struct Aexp1;
  struct Fexp;
  struct Exp10;
  struct InfixExp;
  struct Exp;
  struct Pat;
  struct Apat;
  struct Type_;
  struct BType;
  struct AType;
  struct CType;
  struct Decl_;
  struct DoStmt_;
  struct CaseAlt_;
  struct TypeEqn;

  template <auto BodyPtr, auto Item>
  static constexpr auto genericBlock =
      (lit<"{"> + many(lit<";">) +
       many(append<BodyPtr>(Item) + some(lit<";">)) +
       option(append<BodyPtr>(Item)) + lit<"}">) //
      | (some(ref<NEWLINE>) + indent +
         some(append<BodyPtr>(Item) + many(lit<";">) + many(ref<NEWLINE>)) +
         dedent) |
      (append<BodyPtr>(Item) + many(lit<";"> + append<BodyPtr>(Item)));
  template <auto BodyPtr>
  static constexpr auto declBlock = genericBlock<BodyPtr, ref<Decl_>>;
  template <auto BodyPtr>
  static constexpr auto stmtBlock = genericBlock<BodyPtr, ref<DoStmt_>>;
  template <auto BodyPtr>
  static constexpr auto altBlock = genericBlock<BodyPtr, ref<CaseAlt_>>;

  // NAMES AS AST
  // ============

  struct VarExpr : rule<create<ast::Var>(
                       option(assign<&ast::Var::qualifier>(ref<QUAL_TOK>)) +
                       assign<&ast::Var::name>(ref<VarIdTerm>))> {};
  struct ConExpr : rule<create<ast::Con>(
                       option(assign<&ast::Con::qualifier>(ref<QUAL_TOK>)) +
                       assign<&ast::Con::name>(ref<CONID_TOK>))> {};
  // A parenthesised operator (`(+)`, `` (`mod`) ``) used as an ordinary
  // atom: a var or con built from its symbolic/backtick spelling.
  struct BacktickVar : rule<lit<"`"> + ref<VarIdTerm> + lit<"`">> {};
  struct ParenOpName
      : rule<lit<"("> + (ref<GeneralVarSym> | QConSymBody) + lit<")">> {};
  struct ParenVarOp
      : rule<lit<"("> +
             create<ast::Var>(
                 option(assign<&ast::Var::qualifier>(ref<QUAL_TOK>)) +
                 assign<&ast::Var::name>(ref<GeneralVarSym> |
                                         ref<BacktickVar>)) +
             lit<")">> {};
  struct BacktickCon : rule<lit<"`"> + ref<CONID_TOK> + lit<"`">> {};
  struct ParenConOp
      : rule<lit<"("> +
             create<ast::Con>(
                 option(assign<&ast::Con::qualifier>(ref<QUAL_TOK>)) +
                 assign<&ast::Con::name>(QConSymBody | ref<BacktickCon>)) +
             lit<")">> {};

  // TYPES
  // =====
  // Types, patterns and kinds all reuse this same grammar.

  struct TyVarExpr
      : rule<assign<&ast::TyVar::name>(ref<VarIdTerm>)> {};
  struct TyConExpr : rule<create<ast::TyCon>(
                         option(assign<&ast::TyCon::qualifier>(ref<QUAL_TOK>)) +
                         assign<&ast::TyCon::name>(ref<CONID_TOK>))> {};
  struct TyOpExpr
      : rule<create<ast::TyCon>(
            lit<"`"> + option(assign<&ast::TyCon::qualifier>(ref<QUAL_TOK>)) +
            assign<&ast::TyCon::name>(ref<VARID_TOK> | ref<CONID_TOK>) +
            lit<"`">)> {};

  struct TyBndr
      : rule<create<ast::TyBndr>(assign<&ast::TyBndr::name>(ref<VarIdTerm>)) //
             |
             create<ast::TyBndr>(
                 lit<"("> + assign<&ast::TyBndr::name>(ref<VarIdTerm>) +
                 DCOLON + assign<&ast::TyBndr::kind>(ref<Type_>) + lit<")">) //
             | create<ast::TyBndr>(
                   lit<"{"> + assign<&ast::TyBndr::name>(ref<VarIdTerm>) +
                   option(DCOLON + assign<&ast::TyBndr::kind>(ref<Type_>)) +
                   lit<"}">)> {};

  struct BuiltinTypeName
      : rule<(lit<"["> + lit<"]">) |
             (lit<"("> + option(RARROW | some(lit<",">)) + lit<")">)> {};

  struct AType
      : rule<create<ast::TyBang>(assign<&ast::TyBang::marker>(TILDE) +
                                 assign<&ast::TyBang::type>(ref<AType>)) //
             | create<ast::TyBang>(assign<&ast::TyBang::marker>(lit<"!">) +
                                   assign<&ast::TyBang::type>(ref<AType>)) //
             | create<ast::TyWildcard>(lit<"_">)                           //
             | create<ast::TyLit>(assign<&ast::TyLit::text>(
                   ref<HEX_TOK> | ref<OCTAL_TOK> | ref<BINARY_TOK> |
                   ref<DECIMAL_TOK> | ref<STRING_TOK>)) //
             | create<ast::TyList>(lit<"["> +
                                   assign<&ast::TyList::item>(ref<Type_>) +
                                   lit<"]">)                      //
             | create<ast::TyUnboxedTuple>(lit<"(#"> + lit<"#)">) //
             | create<ast::TyUnboxedTuple>(
                   lit<"(#"> + append<&ast::TyUnboxedTuple::items>(ref<Type_>) +
                   some(lit<","> +
                        append<&ast::TyUnboxedTuple::items>(ref<Type_>)) +
                   lit<"#)">) //
             | create<ast::TyPromotedList>(
                   lit<"'"> + lit<"["> +
                   option(append<&ast::TyPromotedList::items>(ref<Type_>) +
                          many(lit<","> + append<&ast::TyPromotedList::items>(
                                              ref<Type_>))) +
                   lit<"]">) //
             | create<ast::TyPromotedTuple>(
                   lit<"'"> + lit<"("> +
                   append<&ast::TyPromotedTuple::items>(ref<Type_>) +
                   some(lit<","> +
                        append<&ast::TyPromotedTuple::items>(ref<Type_>)) +
                   lit<")">)                                            //
             | ref<TyConExpr>                                           //
             | ref<TyVarExpr>                                           //
             | create<ast::TyCon>(assign<&ast::TyCon::name>(lit<"*">))  //
             | create<ast::TyCon>(assign<&ast::TyCon::name>(ref<BuiltinTypeName>)) //
             // An explicit-kind binder in a header's own argument list, e.g.
             // `type family F (a :: k) ...`.
             | create<ast::TyKindSig>(
                   lit<"("> + assign<&ast::TyKindSig::type>(ref<TyVarExpr>) +
                   DCOLON + assign<&ast::TyKindSig::kind>(ref<Type_>) +
                   lit<")">)                      //
             | (lit<"("> + ref<Type_> + lit<")">) //
             | create<ast::TyTuple>(
                   lit<"("> + append<&ast::TyTuple::items>(ref<Type_>) +
                   some(lit<","> + append<&ast::TyTuple::items>(ref<Type_>)) +
                   lit<")">)> {};

  struct TypeAtomOrOp
      : rule<ref<AType> | create<ast::TyVisibleArg>(ATSIGN +
                   assign<&ast::TyVisibleArg::type>(ref<AType>)) | ref<TyOpExpr>> {};

  struct BType
      : rule<create<ast::TyApp>(
                 append<&ast::TyApp::atoms>(ref<TypeAtomOrOp>) +
                 some(append<&ast::TyApp::atoms>(ref<TypeAtomOrOp>))) //
             | ref<TypeAtomOrOp>> {};

  struct Type_
      : rule<create<ast::TyFun>(assign<&ast::TyFun::from>(ref<BType>) + RARROW +
                                assign<&ast::TyFun::to>(ref<Type_>)) //
             | ref<BType>> {};

  struct CType
      : rule<create<ast::TyForall>(
                 kw<"forall"> + append<&ast::TyForall::vars>(ref<TyBndr>) +
                 many(append<&ast::TyForall::vars>(ref<TyBndr>)) +
                 (lit<"."> | RARROW) +
                 assign<&ast::TyForall::body>(ref<CType>)) //
             | create<ast::TyContext>(
                   assign<&ast::TyContext::context>(ref<BType>) + DARROW +
                   assign<&ast::TyContext::body>(ref<CType>)) //
             | ref<Type_>> {};

  struct KindSigType
      : rule<ref<Type_> |
             create<ast::TyKindSig>(
                 assign<&ast::TyKindSig::type>(ref<Type_>) + DCOLON +
                 assign<&ast::TyKindSig::kind>(ref<Type_>))> {};

  // EXPRESSIONS
  // ===========
  // A pattern IS an expression, resolved by a later pass. This port keeps
  // the same shape -- `Pat` and `Apat` below are `Exp`/`Aexp1` under
  // another name.

  struct FieldBind
      : rule<create<ast::FieldBind>(
                 flag<&ast::FieldBind::isWildcard>(DOTDOT)) //
             | create<ast::FieldBind>(
                   option(assign<&ast::FieldBind::qualifier>(ref<QUAL_TOK>)) +
                   assign<&ast::FieldBind::name>(ref<VarIdTerm>) +
                   option(EQSIGN + assign<&ast::FieldBind::value>(ref<Exp>)))> {
  };

  struct CompQual
      : rule<create<ast::CompQual>(
                 kw<"then"> + kw<"group"> + kw<"by"> +
                 assign<&ast::CompQual::byExpr>(ref<Exp>) + kw<"using"> +
                 assign<&ast::CompQual::usingExpr>(ref<Exp>)) //
             | create<ast::CompQual>(
                   kw<"then"> + kw<"group"> + kw<"using"> +
                   assign<&ast::CompQual::usingExpr>(ref<Exp>)) //
             | create<ast::CompQual>(
                   kw<"then"> + assign<&ast::CompQual::transformFn>(ref<Exp>) +
                   kw<"by"> + assign<&ast::CompQual::byExpr>(ref<Exp>)) //
             |
             create<ast::CompQual>(
                 kw<"then"> + assign<&ast::CompQual::transformFn>(ref<Exp>)) //
             | create<ast::CompQual>(kw<"let"> +
                                     declBlock<&ast::CompQual::letDecls>) //
             | create<ast::CompQual>(
                   assign<&ast::CompQual::bindPattern>(ref<Pat>) + LARROW +
                   assign<&ast::CompQual::value>(ref<Exp>)) //
             | create<ast::CompQual>(assign<&ast::CompQual::value>(ref<Exp>))> {
  };

  template <auto GuardsPtr>
  static constexpr auto guardListImpl =
      some(append<GuardsPtr>(ref<CompQual>), lit<",">);

  struct GuardedExpr
      : rule<PIPE + guardListImpl<&ast::GuardedExpr::guards> + RARROW +
             assign<&ast::GuardedExpr::value>(ref<Exp>)> {};

  struct CaseAlt_
      : rule<create<ast::CaseAlt>(
            assign<&ast::CaseAlt::pattern>(ref<Pat>) +
            ((RARROW + assign<&ast::CaseAlt::body>(ref<Exp>)) //
             | some(append<&ast::CaseAlt::guards>(ref<GuardedExpr>))) +
            option(kw<"where"> + declBlock<&ast::CaseAlt::whereDecls>))> {};

  struct DoStmt_
      : rule<create<ast::DoStmt>(kw<"let"> +
                                 declBlock<&ast::DoStmt::letDecls>) //
             | create<ast::DoStmt>(kw<"rec"> +
                                   stmtBlock<&ast::DoStmt::recStmts>) //
             | create<ast::DoStmt>(assign<&ast::DoStmt::bindPattern>(ref<Pat>) +
                                   LARROW +
                                   assign<&ast::DoStmt::value>(ref<Exp>)) //
             | create<ast::DoStmt>(assign<&ast::DoStmt::value>(ref<Exp>))> {};

  struct QuasiName
      : terminal<as<quasi_name>(lit<"["> + option(some(conidBody + lit<".">)) +
                                smallChar + many(idTail) + many(lit<"#">) +
                                &cr<"|">)> {};
  struct QuasiPayload : terminal<as<quasi_body>(through(lit<"|">, lit<"|]">))> {
  };

  static constexpr auto closingNameChar =
      smallChar | largeChar | uc::Nd | uc::Nl | uc::No | uc::Lm | lit<"'">;
  static constexpr auto openingToken = closingNameChar | cr<R"("([)"> |
                                       (lit<"{"> + !lit<"-">) | lit<"⟦"> |
                                       lit<"⦇">;
  struct AsName
      : terminal<!kw<"_"> + smallChar + option(many(idTail) + closingNameChar) +
                 &lit<"@", openingToken>> {};
  struct AsBinder : rule<create<ast::Var> +
                         option(assign<&ast::Var::qualifier>(ref<QUAL_TOK>)) +
                         assign<&ast::Var::name>(ref<AsName>)> {};
  struct Aexp
      : rule<create<ast::AsPat>(assign<&ast::AsPat::binder>(ref<AsBinder>) +
                                ATSIGN +
                                assign<&ast::AsPat::pattern>(ref<Aexp>)) //
             | ref<VarExpr>                                              //
             | ref<ConExpr>                                              //
             | ref<FloatLit>                                         //
             | ref<IntLit>                                           //
             | ref<CharLit>                                          //
             | ref<StringLit>                                        //
             | create<ast::WildcardPat>(lit<"_">)                        //
             | create<ast::LazyPat>(
                   TILDE + assign<&ast::LazyPat::pattern>(ref<Aexp>)) //
             | create<ast::BangPat>(
                   lit<"!"> + assign<&ast::BangPat::pattern>(ref<Aexp>)) //
             | create<ast::Lambda>(
                   BACKSLASH + some(append<&ast::Lambda::params>(ref<Apat>)) +
                   RARROW + assign<&ast::Lambda::body>(ref<Exp>)) //
             | create<ast::LetExpr>(kw<"let"> +
                                    declBlock<&ast::LetExpr::decls> + kw<"in"> +
                                    assign<&ast::LetExpr::body>(ref<Exp>)) //
             | create<ast::LambdaCaseExpr>(
                   BACKSLASH + kw<"case"> +
                   altBlock<&ast::LambdaCaseExpr::alts>) //
             | create<ast::IfExpr>(
                   kw<"if"> + assign<&ast::IfExpr::cond>(ref<Exp>) +
                   many(lit<";">) + kw<"then"> +
                   assign<&ast::IfExpr::thenBranch>(ref<Exp>) + many(lit<";">) +
                   kw<"else"> + assign<&ast::IfExpr::elseBranch>(ref<Exp>)) //
             | create<ast::MultiIfExpr>(
                   kw<"if"> + option(lit<"{">) +
                   some(append<&ast::MultiIfExpr::arms>(ref<GuardedExpr>)) +
                   option(lit<"}">)) //
             | create<ast::CaseExpr>(
                   kw<"case"> + assign<&ast::CaseExpr::scrutinee>(ref<Exp>) +
                   kw<"of"> + altBlock<&ast::CaseExpr::alts>)                 //
             | create<ast::DoExpr>(kw<"do"> + stmtBlock<&ast::DoExpr::stmts>) //
             | create<ast::DoExpr>(flag<&ast::DoExpr::isMdo>(kw<"mdo">) +
                                   stmtBlock<&ast::DoExpr::stmts>) //
             | create<ast::TupleExpr>(lit<"("> + lit<")">)         //
             | (lit<"("> + ref<Exp> + lit<")">)                    //
             | create<ast::TupleExpr>(
                   lit<"("> + append<&ast::TupleExpr::items>(ref<Exp>) +
                   some(lit<","> + append<&ast::TupleExpr::items>(ref<Exp>)) +
                   lit<")">)                                        //
             | create<ast::UnboxedTupleExpr>(lit<"(#"> + lit<"#)">) //
             | create<ast::UnboxedTupleExpr>(
                   lit<"(#"> + append<&ast::UnboxedTupleExpr::items>(ref<Exp>) +
                   some(lit<","> +
                        append<&ast::UnboxedTupleExpr::items>(ref<Exp>)) +
                   lit<"#)">) //
             | create<ast::LeftSection>(
                   lit<"("> +
                   assign<&ast::LeftSection::op>(Qop) +
                   assign<&ast::LeftSection::operand>(ref<InfixExp>) +
                   lit<")">) //
             |
             create<ast::RightSection>(
                 lit<"("> + assign<&ast::RightSection::operand>(ref<InfixExp>) +
                 assign<&ast::RightSection::op>(Qop) +
                 lit<")">)                                //
             | ref<ParenVarOp>                            //
             | ref<ParenConOp>                            //
             | create<ast::ListExpr>(lit<"["> + lit<"]">) //
             |
             create<ast::ListCompExpr>(
                 lit<"["> + assign<&ast::ListCompExpr::body>(ref<Exp>) + PIPE +
                 guardListImpl<&ast::ListCompExpr::quals> + lit<"]">) //
             | create<ast::ArithSeqExpr>(
                   lit<"["> + assign<&ast::ArithSeqExpr::from>(ref<Exp>) +
                   lit<","> + assign<&ast::ArithSeqExpr::thenExpr>(ref<Exp>) +
                   DOTDOT + option(assign<&ast::ArithSeqExpr::to>(ref<Exp>)) +
                   lit<"]">) //
             | create<ast::ArithSeqExpr>(
                   lit<"["> + assign<&ast::ArithSeqExpr::from>(ref<Exp>) +
                   DOTDOT + option(assign<&ast::ArithSeqExpr::to>(ref<Exp>)) +
                   lit<"]">) //
             | create<ast::ListExpr>(
                   lit<"["> + some(append<&ast::ListExpr::items>(ref<Exp>), lit<",">) +
                   lit<"]">) //
             | create<ast::Splice>(flag<&ast::Splice::isTyped>(lit<"$$">) +
                                   assign<&ast::Splice::value>(ref<Aexp>)) //
             | create<ast::Splice>(lit<"$"> +
                                   assign<&ast::Splice::value>(ref<Aexp>)) //
             | create<ast::Quote>(flag<&ast::Quote::isTyped>(lit<"[||">) +
                                  assign<&ast::Quote::exprBody>(ref<Exp>) +
                                  lit<"||]">) //
             | create<ast::Quote>(lit<"[|"> +
                                  assign<&ast::Quote::exprBody>(ref<Exp>) +
                                  lit<"|]">) //
             | create<ast::Quote>(lit<"[t|"> +
                                  assign<&ast::Quote::typeBody>(ref<Type_>) +
                                  lit<"|]">) //
             | create<ast::Quote>(flag<&ast::Quote::isPat>(lit<"[p|">) +
                                  assign<&ast::Quote::exprBody>(ref<InfixExp>) +
                                  lit<"|]">) //
             | create<ast::Quote>(
                   lit<"[d|"> + declBlock<&ast::Quote::declBody> + lit<"|]">) //
             | create<ast::QuasiQuote>(
                   assign<&ast::QuasiQuote::quoter>(ref<QuasiName>) +
                   assign<&ast::QuasiQuote::body>(ref<QuasiPayload>))> {};

  // Record construction/update chains onto any atom.
  struct Aexp1
      : rule<ref<Aexp> +
             many(nest<ast::RecordExpr, &ast::RecordExpr::base>(
                 lit<"{"> +
                 option(append<&ast::RecordExpr::fields>(ref<FieldBind>) +
                        many(lit<","> + append<&ast::RecordExpr::fields>(
                                            ref<FieldBind>))) +
                 lit<"}">))> {};

  struct Fexp : rule<ref<Aexp1> +
                     many(nest<ast::TypeApp, &ast::TypeApp::func>(
                              ATSIGN + assign<&ast::TypeApp::arg>(ref<AType>)) |
                          nest<ast::App, &ast::App::func>(
                              assign<&ast::App::arg>(ref<Aexp1>)))> {};

  struct Exp10
      : rule<create<ast::NegApp>(lit<"-"> +
                                 assign<&ast::NegApp::operand>(ref<Fexp>)) //
             | ref<Fexp>> {};

  // An operator starting a further line continues the chain -- unlike a
  // decl boundary, `Qop` is never mistakable for the start of one, so no
  // bracket is needed here.
  struct InfixExp
      : rule<create<ast::OpChain>(
                 assign<&ast::OpChain::first>(ref<Exp10>) +
                 some(append<&ast::OpChain::ops>(Qop) +
                      append<&ast::OpChain::operands>(ref<Exp10>))) //
             | ref<Exp10>> {};

  struct Exp : rule<create<ast::TypeSig>(
                        assign<&ast::TypeSig::value>(ref<InfixExp>) + DCOLON +
                        assign<&ast::TypeSig::type>(ref<CType>)) //
                    | create<ast::ArrowCmd>(
                          assign<&ast::ArrowCmd::left>(ref<InfixExp>) +
                          assign<&ast::ArrowCmd::op>(lit<"-<<"> | lit<"-<"> |
                                                     lit<">>-"> | lit<">-">) +
                          assign<&ast::ArrowCmd::right>(ref<Exp>)) //
                    | ref<InfixExp>> {};

  struct Pat : rule<ref<Exp>> {};
  struct Apat : rule<ref<Aexp1>> {};

  // DECLARATIONS
  // ============

  struct GuardedRhsOne
      : rule<create<ast::GuardedRhs>(
            PIPE + guardListImpl<&ast::GuardedRhs::guards> + EQSIGN +
            assign<&ast::GuardedRhs::value>(ref<Exp>))> {};
  struct PlainRhs : rule<create<ast::GuardedRhs>(
                        EQSIGN + assign<&ast::GuardedRhs::value>(ref<Exp>))> {};

  struct ValueDecl
      : rule<create<ast::ValueDecl>(
            assign<&ast::ValueDecl::lhs>(ref<InfixExp>) +
            option(DCOLON + assign<&ast::ValueDecl::resultSig>(ref<CType>)) +
            (append<&ast::ValueDecl::rhs>(ref<PlainRhs>) //
             | some(append<&ast::ValueDecl::rhs>(ref<GuardedRhsOne>))) +
            option(kw<"where"> + declBlock<&ast::ValueDecl::whereDecls>))> {};

  struct TypeSigDecl
      : rule<create<ast::TypeSigDecl>(
            some(append<&ast::TypeSigDecl::names>(ref<InfixExp>), lit<",">) +
            DCOLON + assign<&ast::TypeSigDecl::type>(ref<CType>))> {};

  struct FixityDecl
      : rule<create<ast::FixityDecl>(
            assign<&ast::FixityDecl::assoc>(kw<"infixl"> | kw<"infixr"> |
                                            kw<"infix">) +
            option(assign<&ast::FixityDecl::precedence>(ref<DECIMAL_TOK>)) +
            append<&ast::FixityDecl::ops>(Qop) +
            many(lit<","> + append<&ast::FixityDecl::ops>(Qop)))> {};

  struct PatSynDecl
      : rule<create<ast::PatternSynonymDecl>(
            kw<"pattern"> +
            assign<&ast::PatternSynonymDecl::lhs>(ref<InfixExp>) +
            ((flag<&ast::PatternSynonymDecl::isExplicitBidirectional>(
                  EQSIGN) +
              assign<&ast::PatternSynonymDecl::pattern>(ref<Pat>)) //
             | (LARROW + assign<&ast::PatternSynonymDecl::pattern>(ref<Pat>) +
                option(kw<"where"> +
                       declBlock<&ast::PatternSynonymDecl::whereDecls>))))> {};

  template <auto Kw>
  static constexpr auto genericPragmaBody =
      create<ast::PragmaDecl>(lit<"{-#"> + assign<&ast::PragmaDecl::kind>(Kw) +
                              assign<&ast::PragmaDecl::body>(tight(ref<PragmaTail>)));

  // A long extension list commonly wraps -- unlike other decls, the
  // pragma's own braces make a further physical line unambiguous.
  struct LanguagePragma
      : rule<create<ast::LanguagePragmaDecl>(
            lit<"{-#"> + ikw<"LANGUAGE"> +
            append<&ast::LanguagePragmaDecl::extensions>(ref<CONID_TOK>) +
            many(many(ref<NEWLINE>) + lit<","> + many(ref<NEWLINE>) +
                 append<&ast::LanguagePragmaDecl::extensions>(ref<CONID_TOK>)) +
            many(ref<NEWLINE>) + lit<"#-}">)> {};

  struct UnknownPragma
      : rule<genericPragmaBody<ref<VARID_TOK> | ref<CONID_TOK>>> {};

  struct OptionsPragma
      : rule<genericPragmaBody<ikw<"OPTIONS_GHC"> | ikw<"OPTIONS">>> {};
  struct DeprecatedPragma : rule<genericPragmaBody<ikw<"DEPRECATED">>> {};
  struct WarningPragma : rule<genericPragmaBody<ikw<"WARNING">>> {};
  struct RulesPragma : rule<genericPragmaBody<ikw<"RULES">>> {};
  struct AnnPragma : rule<genericPragmaBody<ikw<"ANN">>> {};
  struct InlinePragma
      : rule<genericPragmaBody<ikw<"INLINE"> | ikw<"NOINLINE">>> {};
  struct SpecialisePragma
      : rule<genericPragmaBody<ikw<"SPECIALISE_INLINE"> | ikw<"SPECIALISE">>> {};
  struct SccPragma : rule<genericPragmaBody<ikw<"SCC">>> {};
  struct MinimalPragma : rule<genericPragmaBody<ikw<"MINIMAL">>> {};
  struct CompletePragma : rule<genericPragmaBody<ikw<"COMPLETE">>> {};
  struct CapiCtype : rule<lit<"{-#"> + ikw<"CTYPE"> + ref<STRING_TOK> +
                              option(ref<STRING_TOK>) + lit<"#-}">> {};

  // DATA / NEWTYPE / GADT
  // ======================

  struct FieldDecl
      : rule<create<ast::FieldDecl>(
            some(append<&ast::FieldDecl::names>(ref<VarIdTerm>), lit<",">) +
            DCOLON + assign<&ast::FieldDecl::type>(ref<CType>))> {};

  struct ConstructorDecl
      : rule<create<ast::ConstructorDecl>(
                 append<&ast::ConstructorDecl::gadtNames>(ref<CONID_TOK>) +
                 many(lit<","> + append<&ast::ConstructorDecl::gadtNames>(
                                     ref<CONID_TOK>)) +
                 DCOLON +
                 assign<&ast::ConstructorDecl::gadtType>(ref<CType>)) //
             | create<ast::ConstructorDecl>(
                   option(kw<"forall"> +
                          some(append<&ast::ConstructorDecl::forallVars>(
                              ref<TyBndr>)) +
                          lit<".">) +
                   option(assign<&ast::ConstructorDecl::context>(ref<BType>) +
                          DARROW) +
                   assign<&ast::ConstructorDecl::type>(ref<BType>) +
                   option(lit<"{"> +
                          option(append<&ast::ConstructorDecl::fields>(
                                     ref<FieldDecl>) +
                                 many(lit<","> +
                                      append<&ast::ConstructorDecl::fields>(
                                          ref<FieldDecl>))) +
                          lit<"}">))> {};

  struct DerivingClause
      : rule<create<ast::DerivingClause>(
            kw<"deriving"> +
            option(assign<&ast::DerivingClause::strategy>(
                kw<"stock"> | kw<"anyclass"> | kw<"newtype">)) +
            ((lit<"("> +
              option(some(append<&ast::DerivingClause::types>(ref<CType>), lit<",">)) +
              lit<")">) //
             | append<&ast::DerivingClause::types>(ref<CType>)) +
            option(kw<"via"> +
                   assign<&ast::DerivingClause::viaType>(ref<CType>)))> {};

  struct DataDecl
      : rule<
            create<ast::DataDecl>(
                kw<"data"> + flag<&ast::DataDecl::isFamily>(kw<"family">) +
                assign<&ast::DataDecl::header>(ref<BType>) +
                option(DCOLON + assign<&ast::DataDecl::kindSig>(ref<CType>))) //
            |
            create<ast::DataDecl>(
                (kw<"data"> |
                 flag<&ast::DataDecl::isNewtype>(kw<"newtype">)) +
                option(flag<&ast::DataDecl::isInstance>(kw<"instance">)) +
                option(assign<&ast::DataDecl::ctype>(tight(ref<CapiCtype>))) +
                option(assign<&ast::DataDecl::context>(ref<BType>) + DARROW) +
                assign<&ast::DataDecl::header>(ref<BType>) +
                ((EQSIGN +
                  append<&ast::DataDecl::constructors>(
                      ref<ConstructorDecl>) +
                  many(PIPE + append<&ast::DataDecl::constructors>(
                                  ref<ConstructorDecl>))) //
                 |
                 (option(DCOLON + assign<&ast::DataDecl::kindSig>(ref<CType>)) +
                  option(flag<&ast::DataDecl::isGadt>(kw<"where">) +
                         genericBlock<&ast::DataDecl::constructors,
                                      ref<ConstructorDecl>>))) +
                many(append<&ast::DataDecl::derivings>(
                    ref<DerivingClause>)))> {};

  // TYPE SYNONYMS / FAMILIES
  // ========================

  struct TypeEqn : rule<create<ast::TypeDecl>(
                           assign<&ast::TypeDecl::header>(ref<BType>) + EQSIGN +
                           assign<&ast::TypeDecl::rhs>(ref<CType>))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
                 kw<"type"> +
                 flag<&ast::TypeDecl::isFamily>(kw<"family">) +
                 assign<&ast::TypeDecl::header>(ref<BType>) +
                 option(DCOLON + assign<&ast::TypeDecl::kindSig>(ref<CType>)) +
                 option(kw<"where"> + genericBlock<&ast::TypeDecl::familyEqns,
                                                   ref<TypeEqn>>)) //
             | create<ast::TypeDecl>(
                   kw<"type"> +
                   flag<&ast::TypeDecl::isInstance>(kw<"instance">) +
                   assign<&ast::TypeDecl::header>(ref<BType>) + EQSIGN +
                   assign<&ast::TypeDecl::rhs>(ref<CType>)) //
             | create<ast::TypeDecl>(
                   kw<"type"> + assign<&ast::TypeDecl::header>(ref<BType>) +
                   EQSIGN + assign<&ast::TypeDecl::rhs>(ref<CType>))> {};

  struct StandaloneKindSig
      : rule<create<ast::StandaloneKindSigDecl>(
            kw<"type"> +
            some(append<&ast::StandaloneKindSigDecl::names>(ref<InfixExp>), lit<",">) +
            DCOLON + assign<&ast::StandaloneKindSigDecl::kind>(ref<CType>))> {};

  struct RoleDecl
      : rule<create<ast::RoleDecl>(
            kw<"type"> + kw<"role"> +
            assign<&ast::RoleDecl::header>(ref<TyConExpr>) +
            many(append<&ast::RoleDecl::roles>(ref<VarIdTerm> | lit<"_">)))> {};

  // CLASS / INSTANCE
  // ================

  struct FunDep
      : rule<create<ast::FunDep>(
            many(append<&ast::FunDep::from>(ref<VarIdTerm>)) + RARROW +
            many(append<&ast::FunDep::to>(ref<VarIdTerm>)))> {};

  struct ClassDecl
      : rule<create<ast::ClassDecl>(
            kw<"class"> +
            option(assign<&ast::ClassDecl::context>(ref<BType>) + DARROW) +
            assign<&ast::ClassDecl::header>(ref<BType>) +
            option(PIPE +
                   some(append<&ast::ClassDecl::dependencies>(ref<FunDep>),
                        lit<",">)) +
            option(kw<"where"> + declBlock<&ast::ClassDecl::body>))> {};

  struct InstanceDecl
      : rule<create<ast::InstanceDecl>(
            kw<"instance"> +
            option(lit<"{-#"> +
                   assign<&ast::InstanceDecl::overlapPragma>(
                       kw<"OVERLAPPABLE"> | kw<"OVERLAPPING"> | kw<"OVERLAPS"> |
                       kw<"INCOHERENT">) +
                   lit<"#-}">) +
            option(assign<&ast::InstanceDecl::context>(ref<BType>) + DARROW) +
            assign<&ast::InstanceDecl::head>(ref<CType>) +
            option(kw<"where"> + declBlock<&ast::InstanceDecl::body>))> {};

  struct StandaloneDeriving
      : rule<create<ast::StandaloneDerivingDecl>(
            kw<"deriving"> +
            option(assign<&ast::StandaloneDerivingDecl::strategy>(
                kw<"stock"> | kw<"anyclass"> | kw<"newtype">)) +
            kw<"instance"> +
            option(assign<&ast::StandaloneDerivingDecl::context>(ref<BType>) +
                   DARROW) +
            assign<&ast::StandaloneDerivingDecl::type>(ref<CType>))> {};

  struct DefaultDecl
      : rule<create<ast::DefaultDecl>(
            kw<"default"> + lit<"("> +
            option(
                some(append<&ast::DefaultDecl::types>(ref<CType>), lit<",">)) +
            lit<")">)> {};

  static constexpr auto CallConv = ref<CcallKw> | ref<StdcallKw> |
                                   ref<CppCallKw> | ref<JsCallKw> | ref<CapiKw>;
  static constexpr auto Safety =
      ref<UnsafeKw> | ref<SafeKw> | ref<InterruptibleKw>;

  struct ForeignDecl
      : rule<
            create<ast::ForeignDecl>(
                kw<"foreign"> +
                flag<&ast::ForeignDecl::isImport>(kw<"import">) +
                assign<&ast::ForeignDecl::callconv>(CallConv) +
                option(assign<&ast::ForeignDecl::safety>(Safety)) +
                option(assign<&ast::ForeignDecl::externName>(ref<STRING_TOK>)) +
                assign<&ast::ForeignDecl::varName>(ref<VarIdTerm>) + DCOLON +
                assign<&ast::ForeignDecl::type>(ref<CType>)) //
            |
            create<ast::ForeignDecl>(
                kw<"foreign"> + kw<"export"> +
                assign<&ast::ForeignDecl::callconv>(CallConv) +
                option(assign<&ast::ForeignDecl::externName>(ref<STRING_TOK>)) +
                assign<&ast::ForeignDecl::varName>(ref<VarIdTerm>) + DCOLON +
                assign<&ast::ForeignDecl::type>(ref<CType>))> {};

  // A NESTED declaration: everything a `where`/`let`/class/instance body
  // may hold. Top-level declarations (below) add module-only forms.
  struct Decl_ : rule<ref<TypeSigDecl>        //
                      | ref<FixityDecl>       //
                      | ref<PatSynDecl>       //
                      | ref<LanguagePragma>   //
                      | ref<OptionsPragma>    //
                      | ref<InlinePragma>     //
                      | ref<SpecialisePragma> //
                      | ref<SccPragma>        //
                      | ref<MinimalPragma>    //
                      | ref<CompletePragma>   //
                      | ref<UnknownPragma>
                      | ref<ValueDecl>> {};

  struct TopDecl_ : rule<ref<ClassDecl>            //
                         | ref<StandaloneKindSig>  //
                         | ref<DataDecl>           //
                         | ref<TypeDecl>           //
                         | ref<InstanceDecl>       //
                         | ref<StandaloneDeriving> //
                         | ref<RoleDecl>           //
                         | ref<DefaultDecl>        //
                         | ref<ForeignDecl>        //
                         | ref<DeprecatedPragma>   //
                         | ref<WarningPragma>      //
                         | ref<RulesPragma>        //
                         | ref<AnnPragma>          //
                         | ref<Decl_>> {};

  // IMPORTS / EXPORTS / MODULE
  // ==========================

  struct ImportItem
      : rule<create<ast::ImportItem>(
            assign<&ast::ImportItem::name>(ref<VarIdTerm> | ref<CONID_TOK> |
                                           ref<ParenOpName>) +
            option((lit<"("> + flag<&ast::ImportItem::wildcard>(DOTDOT) +
                    lit<")">) //
                   | (lit<"("> +
                      option(append<&ast::ImportItem::subNames>(
                                 ref<VarIdTerm> | ref<CONID_TOK>) +
                             many(lit<","> +
                                  append<&ast::ImportItem::subNames>(
                                      ref<VarIdTerm> | ref<CONID_TOK>))) +
                      lit<")">)))> {};

  struct ImportDecl
      : rule<create<ast::ImportDecl>(
            kw<"import"> +
            option(flag<&ast::ImportDecl::source>(
                lit<"{-#"> + kw<"SOURCE"> + lit<"#-}">)) +
            option(flag<&ast::ImportDecl::safe>(ref<SafeKw>)) +
            option(flag<&ast::ImportDecl::qualified>(ref<QualifiedKw>)) +
            option(assign<&ast::ImportDecl::packageLiteral>(ref<STRING_TOK>)) +
            assign<&ast::ImportDecl::moduleName>(ref<MODID_TOK>) +
            option(flag<&ast::ImportDecl::qualified>(ref<QualifiedKw>)) +
            option(kw<"as"> +
                   assign<&ast::ImportDecl::asName>(ref<MODID_TOK>)) +
            option(option(flag<&ast::ImportDecl::hiding>(ref<HidingKw>)) +
                   flag<&ast::ImportDecl::hasSpec>(lit<"(">) +
                   option(append<&ast::ImportDecl::items>(ref<ImportItem>) +
                          many(lit<","> + append<&ast::ImportDecl::items>(
                                              ref<ImportItem>))) +
                   option(lit<",">) + lit<")">))> {};

  struct ExportName
      : rule<(option(ref<QUAL_TOK>) + (ref<VarIdTerm> | ref<CONID_TOK>)) |
             (lit<"("> + option(ref<QUAL_TOK>) +
              (ref<GeneralVarSym> | QConSymBody) + lit<")">)> {};

  struct ExportItem
      : rule<create<ast::ExportItem>(
                 flag<&ast::ExportItem::isModule>(kw<"module">) +
                 assign<&ast::ExportItem::name>(ref<MODID_TOK>)) //
             |
             create<ast::ExportItem>(
                 assign<&ast::ExportItem::name>(ref<ExportName>) +
                 option((lit<"("> +
                         flag<&ast::ExportItem::wildcard>(DOTDOT) +
                         lit<")">) //
                        | (lit<"("> +
                           option(append<&ast::ExportItem::subNames>(
                                      ref<VarIdTerm> | ref<CONID_TOK>) +
                                  many(lit<","> +
                                       append<&ast::ExportItem::subNames>(
                                           ref<VarIdTerm> | ref<CONID_TOK>))) +
                           lit<")">)))> {};

  static constexpr auto ModuleExports =
      flag<&ast::Module::hasExports>(lit<"(">) +
      option(
          some(append<&ast::Module::exports>(ref<ExportItem>), lit<",">)) +
      option(lit<",">) + lit<")">;

  // A leading comment or blank run before the first pragma/decl: nothing
  // yet has matched a `NEWLINE` for it to fold into (python's LeadingTrivia).
  struct LeadingTrivia : terminal<some(blank_line)> {};

  // Between two declarations: a `;` or a newline, or none after a layout
  // block, which closed on its own newline -- no byte but a comment between.
  static constexpr auto DeclSep =
      (some(lit<";">) + many(ref<NEWLINE>)) | some(ref<NEWLINE>) | gap::none |
      gap::is(lit<"{-"> + many(any) + lit<"-}"> + many(cr<R"( \t)">));

  static constexpr auto ModuleItem =
      append<&ast::Module::imports>(ref<ImportDecl>) |
      append<&ast::Module::decls>(ref<TopDecl_>);
  static constexpr auto ModuleItems =
      many(lit<";">) + option(ModuleItem + many(DeclSep + ModuleItem) +
                              many(lit<";">) + many(ref<NEWLINE>));

  struct Module
      : rule<create<ast::Module>(
            option(ref<LeadingTrivia>) +
            many(append<&ast::Module::pragmas>(ref<LanguagePragma> |
                                               ref<OptionsPragma> | ref<UnknownPragma>) +
                 many(lit<";">) + many(ref<NEWLINE>)) +
            option(kw<"module"> + assign<&ast::Module::name>(ref<MODID_TOK>) +
                   option(ModuleExports) + kw<"where"> + many(ref<NEWLINE>)) +
            ((lit<"{"> + ModuleItems + lit<"}"> + many(ref<NEWLINE>)) |
             (indent + ModuleItems + dedent) | ModuleItems))> {};

  using entry = Module;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
  static constexpr auto word_boundary = cr<R"(a-zA-Z0-9_'#\u{80}-\u{10FFFF})">;
};

} // namespace haskell
