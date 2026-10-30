// The bison language: its grammar -- the language Bison/Yacc `.y` files are
// written in (declarations, grammar rules, embedded target-language code).
#pragma once

#include <memory_resource>
#include <string_view>

#include "bison/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace bison {

inline constexpr auto stripEnds1 = [](std::string_view t,
                                      std::pmr::memory_resource*) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto stripPct = [](std::string_view t,
                                    std::pmr::memory_resource*) {
  return t.substr(2, t.size() - 4);
};

struct Bison : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f\v,)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};

  static constexpr auto IdStart = cr<".a-zA-Z_$"> | uc::XID_Start;
  static constexpr auto IdCont = cr<"a-zA-Z0-9_$.-"> | uc::XID_Continue;
  static constexpr auto IdBody = IdStart + many(IdCont);
  struct ID : terminal<IdBody> {};
  static constexpr auto EscSeq = lit<"\\"> + any;
  struct CHAR : terminal<lit<"'"> + many(EscSeq | cr<R"(^'\\)">) + lit<"'">> {};
  struct STRING
      : terminal<lit<"\""> + many(EscSeq | cr<R"(^"\\)">) + lit<"\"">> {};
  struct TSTRING : terminal<lit<"_("> + ref<STRING> + lit<")">> {};
  struct INT : terminal<(lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">)) |
                        some(cr<"0-9">)> {};

  struct TAG;
  struct TAG
      : terminal<lit<"<"> + many(lit<"->"> | cr<"^<>"> | ref<TAG>) + lit<">">> {
  };

  template <auto Member>
  static constexpr auto NamedRef =
      option(lit<"["> + assign<Member>(ref<ID>) + lit<"]">);
  static constexpr auto ProductionHead =
      ref<ID> + option(lit<"["> + ref<ID> + lit<"]">) + lit<":">;

  struct CodeString
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                 lit<"\"">> {};
  struct CodeChar
      : terminal<lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\)">) +
                 lit<"'">> {};
  struct SplicedLine : terminal<lit<"\\"> + many(cr<R"( \t\f\v)">) +
                                option(lit<"\r">) + lit<"\n">> {};
  static constexpr auto Splice = many(ref<SplicedLine>);
  static constexpr auto CommentClose = lit<"*"> + Splice + lit<"/">;
  struct CodeBlockComment : terminal<lit<"/"> + Splice + lit<"*"> +
                                     many(!CommentClose + any) + CommentClose> {
  };
  struct CodeLineComment
      : terminal<lit<"/"> + Splice + lit<"/"> +
                 many(ref<SplicedLine> | (!ref<SplicedLine> + cr<R"(^\n)">)) +
                 &(lit<"\n"> | eof)> {};
  struct CodeText : terminal<some(cr<R"(^{}"'/%<)">)> {};
  static constexpr auto OpaqueCode =
      ref<CodeString> | ref<CodeChar> | ref<CodeLineComment> |
      ref<CodeBlockComment> | ref<CodeText> | (lit<"/"> + !(Splice + cr<"*/">));
  static constexpr auto DigraphOpen = lit<"<"> + Splice + lit<"%">;
  static constexpr auto DigraphClose = lit<"%"> + Splice + lit<">">;
  static constexpr auto CodePunctuation = (lit<"<"> + Splice + lit<"<">) |
                                          (lit<"<"> + !(Splice + cr<"%<">)) |
                                          (lit<"%"> + !(Splice + lit<">">));
  struct CodeRegion;
  struct CodeRegion
      : terminal<(lit<"{"> | DigraphOpen) +
                 many(OpaqueCode | CodePunctuation | ref<CodeRegion>) +
                 (lit<"}"> | DigraphClose)> {};
  struct RawBracedCode : terminal<lit<"{"> +
                                  many(OpaqueCode | CodePunctuation |
                                       ref<CodeRegion> | DigraphClose) +
                                  lit<"}">> {};
  struct BRACED_CODE : terminal<as<stripEnds1>(ref<RawBracedCode>)> {};
  struct PROLOGUE
      : terminal<as<stripPct>(lit<"%{"> +
                              many(OpaqueCode | lit<"{"> | lit<"}"> | lit<"<"> |
                                   (lit<"%"> + !lit<"}">)) +
                              lit<"%}">)> {};
  struct BRACED_PREDICATE
      : terminal<as<[](std::string_view t, std::pmr::memory_resource*) {
        const auto start = t.find('{');
        return t.substr(start + 1, t.size() - start - 2);
      }>(lit<"%?"> + many(cr<R"( \t\r\n\f\v)">) + ref<RawBracedCode>)> {};

  struct OBS_ERROR_VERBOSE
      : terminal<lit<"%error"> + cr<"-_"> + lit<"verbose">> {};
  struct OBS_OUTPUT_KW : terminal<lit<"%output"> + option(many(s) + lit<"=">)> {
  };
  struct NAME_PREFIX_KW : terminal<lit<"%name"> + cr<"-_"> + lit<"prefix"> +
                                   option(many(s) + lit<"=">) + many(s)> {};
  struct PURE_PARSER_KW : terminal<lit<"%pure"> + cr<"-_"> + lit<"parser">> {};
  struct TOKEN_TABLE_KW : terminal<lit<"%token"> + cr<"-_"> + lit<"table">> {};

  struct FILE_PREFIX_KW
      : terminal<lit<"%file-prefix"> + option(many(s) + lit<"=">)> {};
  struct EXPECT_RR_KW : terminal<lit<"%expect"> + cr<"-_"> + lit<"rr">> {};
  struct DEFAULT_PREC_KW : terminal<lit<"%default"> + cr<"-_"> + lit<"prec">> {
  };
  struct NO_DEFAULT_PREC_KW : terminal<lit<"%no"> + cr<"-_"> + lit<"default"> +
                                       cr<"-_"> + lit<"prec">> {};
  struct NO_LINES_KW : terminal<lit<"%no"> + cr<"-_"> + lit<"lines">> {};
  struct FIXED_OUTPUT_KW : terminal<lit<"%fixed"> + cr<"-_"> + lit<"output"> +
                                    cr<"-_"> + lit<"files">> {};
  struct SYNCLINE : terminal<lit<"#line "> + some(cr<"0-9">) +
                             option(lit<" "> + ref<STRING>) +
                             option(lit<"\r">) + lit<"\n">> {};
  struct EPILOGUE : terminal<some(any)> {};
  struct EpilogueText : rule<option(ref<EPILOGUE>)> {};

  struct IdText : rule<ref<ID> | ref<CHAR>> {};
  struct SymbolText : rule<ref<IdText> | ref<STRING>> {};
  struct RubyRefType
      : rule<ref<SymbolText> + lit<"("> + ref<SymbolText> + lit<")">> {};
  struct DefineValue : rule<ref<ID> | ref<STRING> | ref<RawBracedCode>> {};
  struct PrecDeclaratorKw : rule<kw<"%left"> | kw<"%right"> | kw<"%nonassoc"> |
                                 kw<"%binary"> | kw<"%precedence">> {};

  struct ActionBlock : rule<create<ast::Action> +
                                assign<&ast::Action::code>(ref<BRACED_CODE>)> {
  };

  struct SymbolElem : rule<!ProductionHead + create<ast::SymbolRef> +
                               assign<&ast::SymbolRef::name>(ref<SymbolText>) +
                               NamedRef<&ast::SymbolRef::namedRef>> {};

  struct ActionElem
      : rule<create<ast::Action> + option(assign<&ast::Action::tag>(ref<TAG>)) +
             assign<&ast::Action::code>(ref<BRACED_CODE>) +
             NamedRef<&ast::Action::namedRef>> {};

  struct PredicateElem
      : rule<create<ast::Predicate> +
             assign<&ast::Predicate::code>(ref<BRACED_PREDICATE>)> {};

  struct PrecMisc
      : rule<create<ast::RhsMisc> + assign<&ast::RhsMisc::kind>(kw<"%prec">) +
             assign<&ast::RhsMisc::text>(ref<SymbolText>)> {};
  struct DPrecMisc
      : rule<create<ast::RhsMisc> + assign<&ast::RhsMisc::kind>(kw<"%dprec">) +
             assign<&ast::RhsMisc::text>(ref<INT>)> {};
  struct MergeMisc
      : rule<create<ast::RhsMisc> + assign<&ast::RhsMisc::kind>(kw<"%merge">) +
             assign<&ast::RhsMisc::text>(ref<TAG>)> {};
  struct ExpectMisc
      : rule<create<ast::RhsMisc> + assign<&ast::RhsMisc::kind>(kw<"%expect">) +
             assign<&ast::RhsMisc::text>(ref<INT>)> {};
  struct ExpectRRMisc
      : rule<create<ast::RhsMisc> +
             assign<&ast::RhsMisc::kind>(ref<EXPECT_RR_KW>) +
             assign<&ast::RhsMisc::text>(ref<INT>)> {};
  struct EmptyMisc
      : rule<create<ast::RhsMisc> + assign<&ast::RhsMisc::kind>(kw<"%empty">)> {
  };

  struct Alternative
      : rule<create<ast::Alternative> +
             many(append<&ast::Alternative::elements>(
                 ref<SymbolElem> | ref<ActionElem> |
                 ref<PredicateElem> | ref<PrecMisc> |
                 ref<DPrecMisc> | ref<MergeMisc> | ref<ExpectMisc> |
                 ref<ExpectRRMisc> | ref<EmptyMisc>))> {};

  struct Production
      : rule<create<ast::Production> +
             assign<&ast::Production::name>(ref<IdText>) +
             NamedRef<&ast::Production::namedRef> + lit<":"> +
             append<&ast::Production::alternatives>(ref<Alternative>) +
             many((lit<"|"> + append<&ast::Production::alternatives>(
                                  ref<Alternative>)) |
                  lit<";">)> {};

  struct TokenDecl
      : rule<create<ast::TokenSymbol> +
             assign<&ast::TokenSymbol::name>(ref<SymbolText>) +
             option(assign<&ast::TokenSymbol::number>(ref<INT>) |
                    assign<&ast::TokenSymbol::refType>(ref<RubyRefType>)) +
             option(assign<&ast::TokenSymbol::alias>(ref<STRING> |
                                                     ref<TSTRING>))> {};

  struct TokenDeclForPrec
      : rule<create<ast::TokenSymbol> +
             ((assign<&ast::TokenSymbol::name>(ref<IdText>) +
               option(assign<&ast::TokenSymbol::number>(ref<INT>))) |
              assign<&ast::TokenSymbol::name>(ref<STRING>))> {};

  struct TypeSymbol
      : rule<create<ast::TokenSymbol> +
             assign<&ast::TokenSymbol::name>(ref<SymbolText>)> {};

  struct OptionalTag : rule<option(ref<TAG>)> {};
  template <class Symbol, class Tag = OptionalTag>
  struct Group
      : rule<create<ast::TokenGroup> + assign<&ast::TokenGroup::tag>(ref<Tag>) +
             some(append<&ast::TokenGroup::symbols>(ref<Symbol>))> {};
  template <class Symbol>
  static constexpr auto Groups =
      append<&ast::SymbolDecl::groups>(ref<Group<Symbol>>) +
      many(append<&ast::SymbolDecl::groups>(ref<Group<Symbol, TAG>>));
  struct SymbolDecl
      : rule<create<ast::SymbolDecl> +
             ((assign<&ast::SymbolDecl::kind>(kw<"%nterm"> | kw<"%token"> |
                                              kw<"%term">) +
               Groups<TokenDecl>) |
              (assign<&ast::SymbolDecl::kind>(kw<"%type">) +
               Groups<TypeSymbol>) |
              (assign<&ast::SymbolDecl::kind>(ref<PrecDeclaratorKw>) +
               Groups<TokenDeclForPrec>))> {};

  struct CodeProps
      : rule<create<ast::CodePropsDecl> +
             assign<&ast::CodePropsDecl::kind>(kw<"%destructor"> |
                                               kw<"%printer">) +
             assign<&ast::CodePropsDecl::action>(ref<ActionBlock>) +
             some(append<&ast::CodePropsDecl::items>(ref<SymbolText>) |
                  append<&ast::CodePropsDecl::items>(ref<TAG>))> {};

  struct GrammarDeclDirective
      : rule<(assign<&ast::Directive::kind>(kw<"%start">) +
              some(append<&ast::Directive::args>(ref<SymbolText>))) |
             (assign<&ast::Directive::kind>(ref<DEFAULT_PREC_KW>)) |
             (assign<&ast::Directive::kind>(ref<NO_DEFAULT_PREC_KW>)) |
             (assign<&ast::Directive::kind>(kw<"%code">) +
              option(append<&ast::Directive::args>(ref<ID>)) +
              append<&ast::Directive::actions>(ref<ActionBlock>)) |
             (assign<&ast::Directive::kind>(kw<"%union">) +
              option(append<&ast::Directive::args>(ref<ID>)) +
              append<&ast::Directive::actions>(ref<ActionBlock>))> {};

  struct PrologueOnlyDirective
      : rule<(assign<&ast::Directive::kind>(kw<"%debug">)) |
             (assign<&ast::Directive::kind>(kw<"%locations">)) |
             (assign<&ast::Directive::kind>(kw<"%define">) +
              append<&ast::Directive::args>(ref<ID>) +
              option(append<&ast::Directive::args>(ref<DefineValue>))) |
             (assign<&ast::Directive::kind>(kw<"%defines"> | kw<"%header">) +
              option(append<&ast::Directive::args>(ref<STRING>))) |
             (assign<&ast::Directive::kind>(ref<OBS_ERROR_VERBOSE>)) |
             (assign<&ast::Directive::kind>(kw<"%expect">) +
              append<&ast::Directive::args>(ref<INT>)) |
             (assign<&ast::Directive::kind>(ref<EXPECT_RR_KW>) +
              append<&ast::Directive::args>(ref<INT>)) |
             (assign<&ast::Directive::kind>(ref<FILE_PREFIX_KW>) +
              append<&ast::Directive::args>(ref<STRING>)) |
             (assign<&ast::Directive::kind>(kw<"%glr-parser">)) |
             (assign<&ast::Directive::kind>(kw<"%initial-action">) +
              append<&ast::Directive::actions>(ref<ActionBlock>)) |
             (assign<&ast::Directive::kind>(kw<"%language">) +
              append<&ast::Directive::args>(ref<STRING>)) |
             (assign<&ast::Directive::kind>(ref<NAME_PREFIX_KW>) +
              append<&ast::Directive::args>(ref<STRING>)) |
             (assign<&ast::Directive::kind>(ref<NO_LINES_KW>)) |
             (assign<&ast::Directive::kind>(kw<"%nondeterministic-parser">)) |
             (assign<&ast::Directive::kind>(ref<OBS_OUTPUT_KW>) +
              append<&ast::Directive::args>(ref<STRING>)) |
             (assign<&ast::Directive::kind>(kw<"%param">) +
              some(append<&ast::Directive::actions>(ref<ActionBlock>))) |
             (assign<&ast::Directive::kind>(ref<PURE_PARSER_KW>)) |
             (assign<&ast::Directive::kind>(kw<"%parse-param">) +
              some(append<&ast::Directive::actions>(ref<ActionBlock>))) |
             (assign<&ast::Directive::kind>(kw<"%lex-param">) +
              some(append<&ast::Directive::actions>(ref<ActionBlock>))) |
             (assign<&ast::Directive::kind>(kw<"%require">) +
              append<&ast::Directive::args>(ref<STRING>)) |
             (assign<&ast::Directive::kind>(kw<"%skeleton">) +
              append<&ast::Directive::args>(ref<STRING>)) |
             (assign<&ast::Directive::kind>(ref<TOKEN_TABLE_KW>)) |
             (assign<&ast::Directive::kind>(ref<FIXED_OUTPUT_KW>)) |
             (assign<&ast::Directive::kind>(kw<"%verbose">)) |
             (assign<&ast::Directive::kind>(kw<"%yacc">)) |
             (assign<&ast::Directive::kind>(lit<";">)) |
             (append<&ast::Directive::args>(ref<PROLOGUE>))> {};

  struct File_
      : rule<create<ast::File> +
             many(append<&ast::File::prologue>(
                 ref<SymbolDecl> | ref<CodeProps> |
                 ref<GrammarDeclDirective> | ref<PrologueOnlyDirective>)) +
             lit<"%%"> +
             some(append<&ast::File::grammarSection>(ref<Production>) |
                  (append<&ast::File::grammarSection>(ref<SymbolDecl>) +
                   lit<";">) |
                  (append<&ast::File::grammarSection>(ref<CodeProps>) +
                   lit<";">) |
                  (append<&ast::File::grammarSection>(
                       ref<GrammarDeclDirective>) +
                   lit<";">)) +
             option(lit<"%%"> +
                    tight(assign<&ast::File::epilogue>(ref<EpilogueText>)))> {};

  using entry = File_;
  using ignored = WS;
  using hidden = list<BLOCK_COMMENT, LINE_COMMENT, SYNCLINE>;
};

}  // namespace bison