#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "smtlibv2/core/ast.hpp"

namespace smtlibv2 {

inline constexpr auto undouble_dquote = [](std::string_view t,
                                           std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('"') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '"') ++i;
  }
  return std::string_view(room.data(), n);
};
using Unescaped = cursiv::text_conv<undouble_dquote>;

inline constexpr auto strip_bars = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unbarred = cursiv::text_conv<strip_bars>;

struct Smtlibv2 : cursiv::GrammarSpec {
  // The Sym alphabet plus digits: what a symbol may continue with, and what a
  // keyword must not run into.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9+=/*%?!$_~&^<>@.-">;
  static constexpr auto sym_first = cr<"a-zA-Z+=/*%?!$_~&^<>@.-">;

  struct WS : terminal<some(cr<R"(\t\n\r )">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\n)">)> {};

  static constexpr auto digit = cr<"0-9">;
  static constexpr auto numeral_body = lit<"0"> | (cr<"1-9"> + many(digit));

  struct NUMERAL : terminal<numeral_body> {};
  struct DECIMAL : terminal<numeral_body + lit<"."> + some(digit)> {};
  struct BINARY : terminal<lit<"#b"> + some(cr<"01">)> {};
  struct HEXADECIMAL : terminal<lit<"#x"> + some(cr<"0-9a-fA-F">)> {};

  struct STRING
      : terminal<lit<"\""> + many(lit<"\"\""> | cr<R"(\t\n\r !#-~\u{80}-\u{10FFFF})">) +
                     lit<"\"">,
                 Unescaped> {};
  struct QUOTED_SYMBOL
      : terminal<lit<"|"> + many(cr<R"(\t\n\r -[]-{}-~\u{80}-\u{10FFFF})">) + lit<"|">,
                 Unbarred> {};
  struct UNDEF_SYMBOL : terminal<sym_first + many(word_boundary)> {};

  static constexpr auto Symbol = ref<UNDEF_SYMBOL> | ref<QUOTED_SYMBOL>;

  // Any ':'-keyword, predefined or user-chosen: command names and the general
  // reserved words are spelled at their own sites instead, since a symbol
  // never admits them.
  struct GenericKeyword : terminal<lit<":"> + sym_first + many(word_boundary)> {
  };

  struct SpecConstantRule;
  struct SExprListRule;
  struct IdentifierRule;
  struct SortRule;
  struct QualIdentifierRule;
  struct AttributeRule;
  struct TermRule;
  struct VarBindingRule;
  struct SortedVarRule;
  struct QuantTermRule;
  struct PatternRule;
  struct MatchCaseRule;
  struct MatchTermRule;
  struct AnnotTermRule;
  struct ConstTermRule;
  struct IdentTermRule;
  struct AppTermRule;
  struct LetTermRule;

  struct SpecConstantRule
      : rule<create<ast::SpecConstant>(
            assign<&ast::SpecConstant::text>(ref<NUMERAL> | ref<DECIMAL> |
                                             ref<HEXADECIMAL> | ref<BINARY>) |
            flag<&ast::SpecConstant::isString>(
                assign<&ast::SpecConstant::text>(ref<STRING>)))> {};

  struct SExprListRule : rule<create<ast::SExprList>(
                             lit<"("> +
                             many(append<&ast::SExprList::items>(
                                 ref<SpecConstantRule> | Symbol |
                                 ref<GenericKeyword> | ref<SExprListRule>)) +
                             lit<")">)> {};

  static constexpr auto s_expr_val =
      ref<SpecConstantRule> | Symbol | ref<GenericKeyword> | ref<SExprListRule>;
  // No bare keyword alternative here, unlike s_expr_val above.
  static constexpr auto attr_value =
      ref<SpecConstantRule> | Symbol | ref<SExprListRule>;

  static constexpr auto index_val = ref<NUMERAL> | Symbol;

  struct IdentifierRule
      : rule<create<ast::Identifier>(
            assign<&ast::Identifier::symbol>(Symbol) |
            (lit<"("> + kw<"_"> + assign<&ast::Identifier::symbol>(Symbol) +
             some(append<&ast::Identifier::indices>(index_val)) + lit<")">))> {
  };

  struct SortRule
      : rule<create<ast::Sort>(assign<&ast::Sort::id>(ref<IdentifierRule>)) |
             create<ast::Sort>(
                 lit<"("> + assign<&ast::Sort::id>(ref<IdentifierRule>) +
                 some(append<&ast::Sort::args>(ref<SortRule>)) + lit<")">)> {};

  struct QualIdentifierRule
      : rule<create<ast::QualIdentifier>(
                 assign<&ast::QualIdentifier::id>(ref<IdentifierRule>)) |
             create<ast::QualIdentifier>(
                 lit<"("> + kw<"as"> +
                 assign<&ast::QualIdentifier::id>(ref<IdentifierRule>) +
                 assign<&ast::QualIdentifier::ascribedSort>(ref<SortRule>) +
                 lit<")">)> {};

  struct AttributeRule
      : rule<create<ast::Attribute>(
            assign<&ast::Attribute::keyword>(ref<GenericKeyword>) +
            option(assign<&ast::Attribute::value>(attr_value)))> {};

  struct ConstTermRule
      : rule<create<ast::ConstTerm>(
            assign<&ast::ConstTerm::value>(ref<SpecConstantRule>))> {};

  struct IdentTermRule
      : rule<create<ast::IdentTerm>(
            assign<&ast::IdentTerm::id>(ref<QualIdentifierRule>))> {};

  struct AppTermRule
      : rule<create<ast::AppTerm>(
            lit<"("> + assign<&ast::AppTerm::head>(ref<QualIdentifierRule>) +
            many(append<&ast::AppTerm::args>(ref<TermRule>)) + lit<")">)> {};

  struct VarBindingRule
      : rule<create<ast::VarBinding>(
            lit<"("> + assign<&ast::VarBinding::name>(Symbol) +
            assign<&ast::VarBinding::value>(ref<TermRule>) + lit<")">)> {};

  struct LetTermRule
      : rule<create<ast::LetTerm>(
            lit<"("> + kw<"let"> + lit<"("> +
            many(append<&ast::LetTerm::bindings>(ref<VarBindingRule>)) +
            lit<")"> + assign<&ast::LetTerm::body>(ref<TermRule>) + lit<")">)> {
  };

  struct SortedVarRule
      : rule<create<ast::SortedVar>(
            lit<"("> + assign<&ast::SortedVar::name>(Symbol) +
            assign<&ast::SortedVar::sort>(ref<SortRule>) + lit<")">)> {};

  struct QuantTermRule
      : rule<create<ast::QuantTerm>(
            lit<"("> +
            (flag<&ast::QuantTerm::exists>(kw<"exists">) |
             kw<"forall">)+lit<"("> +
            many(append<&ast::QuantTerm::vars>(ref<SortedVarRule>)) + lit<")"> +
            assign<&ast::QuantTerm::body>(ref<TermRule>) + lit<")">)> {};

  struct PatternRule
      : rule<create<ast::Pattern>(assign<&ast::Pattern::head>(Symbol)) |
             create<ast::Pattern>(
                 lit<"("> + assign<&ast::Pattern::head>(Symbol) +
                 some(append<&ast::Pattern::args>(Symbol)) + lit<")">)> {};

  struct MatchCaseRule
      : rule<create<ast::MatchCase>(
            lit<"("> + assign<&ast::MatchCase::pattern>(ref<PatternRule>) +
            assign<&ast::MatchCase::body>(ref<TermRule>) + lit<")">)> {};

  struct MatchTermRule
      : rule<create<ast::MatchTerm>(
            lit<"("> + kw<"match"> +
            assign<&ast::MatchTerm::scrutinee>(ref<TermRule>) + lit<"("> +
            many(append<&ast::MatchTerm::cases>(ref<MatchCaseRule>)) +
            lit<")"> + lit<")">)> {};

  struct AnnotTermRule
      : rule<create<ast::AnnotTerm>(
            lit<"("> + kw<"!"> + assign<&ast::AnnotTerm::term>(ref<TermRule>) +
            many(append<&ast::AnnotTerm::attrs>(ref<AttributeRule>)) +
            lit<")">)> {};

  struct TermRule
      : rule<ref<ConstTermRule> | ref<LetTermRule> | ref<QuantTermRule> |
             ref<MatchTermRule> | ref<AnnotTermRule> | ref<AppTermRule> |
             ref<IdentTermRule>> {};

  // Theory declarations.

  struct SortSymbolDeclRule
      : rule<create<ast::SortSymbolDecl>(
            lit<"("> + assign<&ast::SortSymbolDecl::id>(ref<IdentifierRule>) +
            assign<&ast::SortSymbolDecl::arity>(ref<NUMERAL>) +
            many(append<&ast::SortSymbolDecl::attrs>(ref<AttributeRule>)) +
            lit<")">)> {};

  struct IdentFunDeclRule
      : rule<create<ast::FunSymbolDecl>(
            lit<"("> + assign<&ast::FunSymbolDecl::head>(ref<IdentifierRule>) +
            some(append<&ast::FunSymbolDecl::sorts>(ref<SortRule>)) +
            many(append<&ast::FunSymbolDecl::attrs>(ref<AttributeRule>)) +
            lit<")">)> {};

  struct FunSymbolDeclRule
      : rule<create<ast::FunSymbolDecl>(
                 lit<"("> +
                 assign<&ast::FunSymbolDecl::head>(ref<SpecConstantRule>) +
                 append<&ast::FunSymbolDecl::sorts>(ref<SortRule>) +
                 many(append<&ast::FunSymbolDecl::attrs>(ref<AttributeRule>)) +
                 lit<")">) |
             create<ast::FunSymbolDecl>(
                 lit<"("> +
                 assign<&ast::FunSymbolDecl::head>(
                     kw<"NUMERAL"> | kw<"DECIMAL"> | kw<"string">) +
                 append<&ast::FunSymbolDecl::sorts>(ref<SortRule>) +
                 many(append<&ast::FunSymbolDecl::attrs>(ref<AttributeRule>)) +
                 lit<")">) |
             ref<IdentFunDeclRule>> {};

  struct ParFunSymbolDeclRule
      : rule<create<ast::ParFunSymbolDecl>(
                 assign<&ast::ParFunSymbolDecl::decl>(ref<FunSymbolDeclRule>)) |
             create<ast::ParFunSymbolDecl>(
                 lit<"("> + kw<"par"> + lit<"("> +
                 some(append<&ast::ParFunSymbolDecl::params>(Symbol)) +
                 lit<")"> +
                 assign<&ast::ParFunSymbolDecl::decl>(ref<IdentFunDeclRule>) +
                 lit<")">)> {};

  struct TheoryAttributeRule
      : rule<create<ast::SortsAttr>(
                 kw<":sorts"> + lit<"("> +
                 some(append<&ast::SortsAttr::decls>(ref<SortSymbolDeclRule>)) +
                 lit<")">) |
             create<ast::FunsAttr>(kw<":funs"> + lit<"("> +
                                   some(append<&ast::FunsAttr::decls>(
                                       ref<ParFunSymbolDeclRule>)) +
                                   lit<")">) |
             create<ast::TheoryTextAttr>(
                 assign<&ast::TheoryTextAttr::keyword>(
                     kw<":sorts-description"> | kw<":funs-description"> |
                     kw<":definition"> | kw<":values"> | kw<":notes">) +
                 assign<&ast::TheoryTextAttr::text>(ref<STRING>)) |
             create<ast::TheoryGenericAttr>(
                 assign<&ast::TheoryGenericAttr::attr>(ref<AttributeRule>))> {};

  struct TheoryDeclRule
      : rule<create<ast::TheoryDecl>(
            lit<"("> + kw<"theory"> + assign<&ast::TheoryDecl::name>(Symbol) +
            some(append<&ast::TheoryDecl::attrs>(ref<TheoryAttributeRule>)) +
            lit<")">)> {};

  struct LogicAttributeRule
      : rule<create<ast::TheoriesAttr>(
                 kw<":theories"> + lit<"("> +
                 some(append<&ast::TheoriesAttr::theories>(Symbol)) +
                 lit<")">) |
             create<ast::LogicTextAttr>(
                 assign<&ast::LogicTextAttr::keyword>(
                     kw<":language"> | kw<":extensions"> | kw<":values"> |
                     kw<":notes">) +
                 assign<&ast::LogicTextAttr::text>(ref<STRING>)) |
             create<ast::LogicGenericAttr>(
                 assign<&ast::LogicGenericAttr::attr>(ref<AttributeRule>))> {};

  struct LogicRule
      : rule<create<ast::Logic>(
            lit<"("> + kw<"logic"> + assign<&ast::Logic::name>(Symbol) +
            some(append<&ast::Logic::attrs>(ref<LogicAttributeRule>)) +
            lit<")">)> {};

  // Scripts.

  struct SortDecRule
      : rule<create<ast::SortDec>(
            lit<"("> + assign<&ast::SortDec::name>(Symbol) +
            assign<&ast::SortDec::arity>(ref<NUMERAL>) + lit<")">)> {};

  struct SelectorDecRule
      : rule<create<ast::SelectorDec>(
            lit<"("> + assign<&ast::SelectorDec::name>(Symbol) +
            assign<&ast::SelectorDec::sort>(ref<SortRule>) + lit<")">)> {};

  struct ConstructorDecRule
      : rule<create<ast::ConstructorDec>(
            lit<"("> + assign<&ast::ConstructorDec::name>(Symbol) +
            many(
                append<&ast::ConstructorDec::selectors>(ref<SelectorDecRule>)) +
            lit<")">)> {};

  struct DatatypeDecRule
      : rule<create<ast::DatatypeDec>(
                 lit<"("> +
                 many(append<&ast::DatatypeDec::constructors>(
                     ref<ConstructorDecRule>)) +
                 lit<")">) |
             create<ast::DatatypeDec>(
                 lit<"("> + kw<"par"> + lit<"("> +
                 some(append<&ast::DatatypeDec::params>(Symbol)) + lit<")"> +
                 lit<"("> +
                 many(append<&ast::DatatypeDec::constructors>(
                     ref<ConstructorDecRule>)) +
                 lit<")"> + lit<")">)> {};

  struct FunctionDecRule
      : rule<create<ast::FunctionDec>(
            lit<"("> + assign<&ast::FunctionDec::name>(Symbol) + lit<"("> +
            many(append<&ast::FunctionDec::params>(ref<SortedVarRule>)) +
            lit<")"> + assign<&ast::FunctionDec::result>(ref<SortRule>) +
            lit<")">)> {};

  struct FunctionDefRule
      : rule<create<ast::FunctionDef>(
            assign<&ast::FunctionDef::name>(Symbol) + lit<"("> +
            many(append<&ast::FunctionDef::params>(ref<SortedVarRule>)) +
            lit<")"> + assign<&ast::FunctionDef::result>(ref<SortRule>) +
            assign<&ast::FunctionDef::body>(ref<TermRule>))> {};

  struct PropLiteralRule
      : rule<create<ast::PropLiteral>(
                 assign<&ast::PropLiteral::symbol>(Symbol)) |
             create<ast::PropLiteral>(
                 lit<"("> + flag<&ast::PropLiteral::negated>(kw<"not">) +
                 assign<&ast::PropLiteral::symbol>(Symbol) + lit<")">)> {};

  // Commands.

  struct SimpleCommandRule
      : rule<create<ast::SimpleCommand>(assign<&ast::SimpleCommand::name>(
            kw<"check-sat"> | kw<"exit"> | kw<"get-assertions"> |
            kw<"get-assignment"> | kw<"get-model"> | kw<"get-proof"> |
            kw<"get-unsat-assumptions"> | kw<"get-unsat-core"> |
            kw<"reset-assertions"> | kw<"reset">))> {};

  struct AssertCommandRule
      : rule<create<ast::AssertCommand>(
            kw<"assert"> + assign<&ast::AssertCommand::term>(ref<TermRule>))> {
  };

  struct CheckSatAssumingCommandRule
      : rule<create<ast::CheckSatAssumingCommand>(
            kw<"check-sat-assuming"> + lit<"("> +
            many(append<&ast::CheckSatAssumingCommand::assumptions>(
                ref<PropLiteralRule>)) +
            lit<")">)> {};

  struct DeclareConstCommandRule
      : rule<create<ast::DeclareConstCommand>(
            kw<"declare-const"> +
            assign<&ast::DeclareConstCommand::name>(Symbol) +
            assign<&ast::DeclareConstCommand::sort>(ref<SortRule>))> {};

  struct DeclareDatatypeCommandRule
      : rule<create<ast::DeclareDatatypeCommand>(
            kw<"declare-datatype"> +
            assign<&ast::DeclareDatatypeCommand::name>(Symbol) +
            assign<&ast::DeclareDatatypeCommand::decl>(ref<DatatypeDecRule>))> {
  };

  struct DeclareDatatypesCommandRule
      : rule<create<ast::DeclareDatatypesCommand>(
            kw<"declare-datatypes"> + lit<"("> +
            many(append<&ast::DeclareDatatypesCommand::sorts>(
                ref<SortDecRule>)) +
            lit<")"> + lit<"("> +
            many(append<&ast::DeclareDatatypesCommand::decls>(
                ref<DatatypeDecRule>)) +
            lit<")">)> {};

  struct DeclareFunCommandRule
      : rule<create<ast::DeclareFunCommand>(
            kw<"declare-fun"> + assign<&ast::DeclareFunCommand::name>(Symbol) +
            lit<"("> +
            many(append<&ast::DeclareFunCommand::params>(ref<SortRule>)) +
            lit<")"> +
            assign<&ast::DeclareFunCommand::result>(ref<SortRule>))> {};

  struct DeclareSortCommandRule
      : rule<create<ast::DeclareSortCommand>(
            kw<"declare-sort"> +
            assign<&ast::DeclareSortCommand::name>(Symbol) +
            assign<&ast::DeclareSortCommand::arity>(ref<NUMERAL>))> {};

  struct DefineFunCommandRule
      : rule<create<ast::DefineFunCommand>(
            kw<"define-fun"> +
            assign<&ast::DefineFunCommand::def>(ref<FunctionDefRule>))> {};

  struct DefineFunRecCommandRule
      : rule<create<ast::DefineFunRecCommand>(
            kw<"define-fun-rec"> +
            assign<&ast::DefineFunRecCommand::def>(ref<FunctionDefRule>))> {};

  struct DefineFunsRecCommandRule
      : rule<create<ast::DefineFunsRecCommand>(
            kw<"define-funs-rec"> + lit<"("> +
            many(append<&ast::DefineFunsRecCommand::decls>(
                ref<FunctionDecRule>)) +
            lit<")"> + lit<"("> +
            many(append<&ast::DefineFunsRecCommand::bodies>(ref<TermRule>)) +
            lit<")">)> {};

  struct DefineSortCommandRule
      : rule<create<ast::DefineSortCommand>(
            kw<"define-sort"> + assign<&ast::DefineSortCommand::name>(Symbol) +
            lit<"("> + many(append<&ast::DefineSortCommand::params>(Symbol)) +
            lit<")"> + assign<&ast::DefineSortCommand::body>(ref<SortRule>))> {
  };

  struct EchoCommandRule
      : rule<create<ast::EchoCommand>(
            kw<"echo"> + assign<&ast::EchoCommand::text>(ref<STRING>))> {};

  struct GetInfoCommandRule
      : rule<create<ast::GetInfoCommand>(
            kw<"get-info"> +
            assign<&ast::GetInfoCommand::flag>(ref<GenericKeyword>))> {};

  struct GetOptionCommandRule
      : rule<create<ast::GetOptionCommand>(
            kw<"get-option"> +
            assign<&ast::GetOptionCommand::keyword>(ref<GenericKeyword>))> {};

  struct GetValueCommandRule
      : rule<create<ast::GetValueCommand>(
            kw<"get-value"> + lit<"("> +
            some(append<&ast::GetValueCommand::terms>(ref<TermRule>)) +
            lit<")">)> {};

  struct PopCommandRule
      : rule<create<ast::PopCommand>(
            kw<"pop"> +
            option(assign<&ast::PopCommand::count>(ref<NUMERAL>)))> {};

  struct PushCommandRule
      : rule<create<ast::PushCommand>(
            kw<"push"> +
            option(assign<&ast::PushCommand::count>(ref<NUMERAL>)))> {};

  struct SetInfoCommandRule
      : rule<create<ast::SetInfoCommand>(
            kw<"set-info"> +
            assign<&ast::SetInfoCommand::attr>(ref<AttributeRule>))> {};

  struct SetLogicCommandRule
      : rule<create<ast::SetLogicCommand>(
            kw<"set-logic"> + assign<&ast::SetLogicCommand::name>(Symbol))> {};

  struct SetOptionCommandRule
      : rule<create<ast::SetOptionCommand>(
            kw<"set-option"> +
            assign<&ast::SetOptionCommand::option>(ref<AttributeRule>))> {};

  static constexpr auto command_body =
      ref<AssertCommandRule> | ref<SimpleCommandRule> |
      ref<CheckSatAssumingCommandRule> | ref<DeclareConstCommandRule> |
      ref<DeclareDatatypeCommandRule> | ref<DeclareDatatypesCommandRule> |
      ref<DeclareFunCommandRule> | ref<DeclareSortCommandRule> |
      ref<DefineFunCommandRule> | ref<DefineFunRecCommandRule> |
      ref<DefineFunsRecCommandRule> | ref<DefineSortCommandRule> |
      ref<EchoCommandRule> | ref<GetInfoCommandRule> |
      ref<GetOptionCommandRule> | ref<GetValueCommandRule> |
      ref<PopCommandRule> | ref<PushCommandRule> | ref<SetInfoCommandRule> |
      ref<SetLogicCommandRule> | ref<SetOptionCommandRule>;

  struct CommandRule : rule<lit<"("> + command_body + lit<")">> {};

  struct ScriptRule : rule<create<ast::Script>(many(
                          append<&ast::Script::commands>(ref<CommandRule>)))> {
  };

  struct LogicDocRule : rule<create<ast::LogicDoc>(
                            assign<&ast::LogicDoc::logic>(ref<LogicRule>))> {};

  struct TheoryDeclDocRule
      : rule<create<ast::TheoryDeclDoc>(
            assign<&ast::TheoryDeclDoc::decl>(ref<TheoryDeclRule>))> {};

  struct CheckSatResponseRule : rule<assign<&ast::CheckSatResponse::result>(
                                    kw<"sat"> | kw<"unsat"> | kw<"unknown">)> {
  };
  struct EchoResponseRule
      : rule<assign<&ast::EchoResponse::text>(ref<STRING>)> {};
  struct SExprResponseRule
      : rule<assign<&ast::SExprResponse::value>(s_expr_val)> {};
  struct GeneralResponseRule
      : rule<(create<ast::SuccessResponse> + kw<"success">) |
             (create<ast::UnsupportedResponse> + kw<"unsupported">) |
             create<ast::ErrorResponse>(
                 lit<"("> + kw<"error"> +
                 assign<&ast::ErrorResponse::message>(ref<STRING>) + lit<")">) |
             ref<CheckSatResponseRule> | ref<EchoResponseRule> |
             ref<SExprResponseRule>> {};
  struct ResponseDocRule : rule<some(append<&ast::ResponseDoc::responses>(
                               ref<GeneralResponseRule>))> {};
  struct StartRule : rule<ref<ScriptRule> | ref<LogicDocRule> |
                          ref<TheoryDeclDocRule> | ref<ResponseDocRule>> {};

  using entry = StartRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace smtlibv2
