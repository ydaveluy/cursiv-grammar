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

inline constexpr auto strip_bars = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

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
      : terminal<as<undouble_dquote>(
            lit<"\""> +
            many(lit<"\"\""> | cr<R"(\t\n\r !#-~\u{80}-\u{10FFFF})">) +
            lit<"\"">)> {};
  struct QUOTED_SYMBOL
      : terminal<as<strip_bars>(
            lit<"|"> + many(cr<R"(\t\n\r -[]-{}-~\u{80}-\u{10FFFF})">) +
            lit<"|">)> {};
  struct UNDEF_SYMBOL : terminal<sym_first + many(word_boundary)> {};

  static constexpr auto Symbol = ref<UNDEF_SYMBOL> | ref<QUOTED_SYMBOL>;

  // Any ':'-keyword, predefined or user-chosen: command names and the general
  // reserved words are spelled at their own sites instead, since a symbol
  // never admits them.
  struct GenericKeyword : terminal<lit<":"> + sym_first + many(word_boundary)> {
  };

  struct SpecConstant;
  struct SExprList;
  struct Identifier;
  struct Sort;
  struct QualIdentifier;
  struct Attribute;
  struct Term;
  struct VarBinding;
  struct SortedVar;
  struct QuantTerm;
  struct Pattern;
  struct MatchCase;
  struct MatchTerm;
  struct AnnotTerm;
  struct ConstTerm;
  struct IdentTerm;
  struct AppTerm;
  struct LetTerm;

  struct SpecConstant
      : rule<create<ast::SpecConstant>(
            assign<&ast::SpecConstant::text>(ref<NUMERAL> | ref<DECIMAL> |
                                             ref<HEXADECIMAL> | ref<BINARY>) |
            flag<&ast::SpecConstant::isString>(
                assign<&ast::SpecConstant::text>(ref<STRING>)))> {};

  struct SExprList : rule<create<ast::SExprList>(
                             lit<"("> +
                             many(append<&ast::SExprList::items>(
                                 ref<SpecConstant> | Symbol |
                                 ref<GenericKeyword> | ref<SExprList>)) +
                             lit<")">)> {};

  static constexpr auto s_expr_val =
      ref<SpecConstant> | Symbol | ref<GenericKeyword> | ref<SExprList>;
  // No bare keyword alternative here, unlike s_expr_val above.
  static constexpr auto attr_value =
      ref<SpecConstant> | Symbol | ref<SExprList>;

  static constexpr auto index_val = ref<NUMERAL> | Symbol;

  struct Identifier
      : rule<create<ast::Identifier>(
            assign<&ast::Identifier::symbol>(Symbol) |
            (lit<"("> + kw<"_"> + assign<&ast::Identifier::symbol>(Symbol) +
             some(append<&ast::Identifier::indices>(index_val)) + lit<")">))> {
  };

  struct Sort
      : rule<create<ast::Sort>(assign<&ast::Sort::id>(ref<Identifier>)) |
             create<ast::Sort>(
                 lit<"("> + assign<&ast::Sort::id>(ref<Identifier>) +
                 some(append<&ast::Sort::args>(ref<Sort>)) + lit<")">)> {};

  struct QualIdentifier
      : rule<create<ast::QualIdentifier>(
                 assign<&ast::QualIdentifier::id>(ref<Identifier>)) |
             create<ast::QualIdentifier>(
                 lit<"("> + kw<"as"> +
                 assign<&ast::QualIdentifier::id>(ref<Identifier>) +
                 assign<&ast::QualIdentifier::ascribedSort>(ref<Sort>) +
                 lit<")">)> {};

  struct Attribute
      : rule<create<ast::Attribute>(
            assign<&ast::Attribute::keyword>(ref<GenericKeyword>) +
            option(assign<&ast::Attribute::value>(attr_value)))> {};

  struct ConstTerm
      : rule<assign<&ast::ConstTerm::value>(ref<SpecConstant>)> {};

  struct IdentTerm
      : rule<assign<&ast::IdentTerm::id>(ref<QualIdentifier>)> {};

  struct AppTerm
      : rule<create<ast::AppTerm>(
            lit<"("> + assign<&ast::AppTerm::head>(ref<QualIdentifier>) +
            many(append<&ast::AppTerm::args>(ref<Term>)) + lit<")">)> {};

  struct VarBinding
      : rule<create<ast::VarBinding>(
            lit<"("> + assign<&ast::VarBinding::name>(Symbol) +
            assign<&ast::VarBinding::value>(ref<Term>) + lit<")">)> {};

  struct LetTerm
      : rule<create<ast::LetTerm>(
            lit<"("> + kw<"let"> + lit<"("> +
            many(append<&ast::LetTerm::bindings>(ref<VarBinding>)) +
            lit<")"> + assign<&ast::LetTerm::body>(ref<Term>) + lit<")">)> {
  };

  struct SortedVar
      : rule<create<ast::SortedVar>(
            lit<"("> + assign<&ast::SortedVar::name>(Symbol) +
            assign<&ast::SortedVar::sort>(ref<Sort>) + lit<")">)> {};

  struct QuantTerm
      : rule<create<ast::QuantTerm>(
            lit<"("> +
            (flag<&ast::QuantTerm::exists>(kw<"exists">) |
             kw<"forall">)+lit<"("> +
            many(append<&ast::QuantTerm::vars>(ref<SortedVar>)) + lit<")"> +
            assign<&ast::QuantTerm::body>(ref<Term>) + lit<")">)> {};

  struct Pattern
      : rule<create<ast::Pattern>(assign<&ast::Pattern::head>(Symbol)) |
             create<ast::Pattern>(
                 lit<"("> + assign<&ast::Pattern::head>(Symbol) +
                 some(append<&ast::Pattern::args>(Symbol)) + lit<")">)> {};

  struct MatchCase
      : rule<create<ast::MatchCase>(
            lit<"("> + assign<&ast::MatchCase::pattern>(ref<Pattern>) +
            assign<&ast::MatchCase::body>(ref<Term>) + lit<")">)> {};

  struct MatchTerm
      : rule<create<ast::MatchTerm>(
            lit<"("> + kw<"match"> +
            assign<&ast::MatchTerm::scrutinee>(ref<Term>) + lit<"("> +
            many(append<&ast::MatchTerm::cases>(ref<MatchCase>)) +
            lit<")"> + lit<")">)> {};

  struct AnnotTerm
      : rule<create<ast::AnnotTerm>(
            lit<"("> + kw<"!"> + assign<&ast::AnnotTerm::term>(ref<Term>) +
            many(append<&ast::AnnotTerm::attrs>(ref<Attribute>)) +
            lit<")">)> {};

  struct Term
      : rule<ref<ConstTerm> | ref<LetTerm> | ref<QuantTerm> |
             ref<MatchTerm> | ref<AnnotTerm> | ref<AppTerm> |
             ref<IdentTerm>> {};

  // Theory declarations.

  struct SortSymbolDecl
      : rule<create<ast::SortSymbolDecl>(
            lit<"("> + assign<&ast::SortSymbolDecl::id>(ref<Identifier>) +
            assign<&ast::SortSymbolDecl::arity>(ref<NUMERAL>) +
            many(append<&ast::SortSymbolDecl::attrs>(ref<Attribute>)) +
            lit<")">)> {};

  struct IdentFunDecl
      : rule<create<ast::FunSymbolDecl>(
            lit<"("> + assign<&ast::FunSymbolDecl::head>(ref<Identifier>) +
            some(append<&ast::FunSymbolDecl::sorts>(ref<Sort>)) +
            many(append<&ast::FunSymbolDecl::attrs>(ref<Attribute>)) +
            lit<")">)> {};

  struct FunSymbolDecl
      : rule<create<ast::FunSymbolDecl>(
                 lit<"("> +
                 assign<&ast::FunSymbolDecl::head>(ref<SpecConstant>) +
                 append<&ast::FunSymbolDecl::sorts>(ref<Sort>) +
                 many(append<&ast::FunSymbolDecl::attrs>(ref<Attribute>)) +
                 lit<")">) |
             create<ast::FunSymbolDecl>(
                 lit<"("> +
                 assign<&ast::FunSymbolDecl::head>(
                     kw<"NUMERAL"> | kw<"DECIMAL"> | kw<"string">) +
                 append<&ast::FunSymbolDecl::sorts>(ref<Sort>) +
                 many(append<&ast::FunSymbolDecl::attrs>(ref<Attribute>)) +
                 lit<")">) |
             ref<IdentFunDecl>> {};

  struct ParFunSymbolDecl
      : rule<create<ast::ParFunSymbolDecl>(
                 assign<&ast::ParFunSymbolDecl::decl>(ref<FunSymbolDecl>)) |
             create<ast::ParFunSymbolDecl>(
                 lit<"("> + kw<"par"> + lit<"("> +
                 some(append<&ast::ParFunSymbolDecl::params>(Symbol)) +
                 lit<")"> +
                 assign<&ast::ParFunSymbolDecl::decl>(ref<IdentFunDecl>) +
                 lit<")">)> {};

  struct TheoryAttribute
      : rule<create<ast::SortsAttr>(
                 kw<":sorts"> + lit<"("> +
                 some(append<&ast::SortsAttr::decls>(ref<SortSymbolDecl>)) +
                 lit<")">) |
             create<ast::FunsAttr>(kw<":funs"> + lit<"("> +
                                   some(append<&ast::FunsAttr::decls>(
                                       ref<ParFunSymbolDecl>)) +
                                   lit<")">) |
             create<ast::TheoryTextAttr>(
                 assign<&ast::TheoryTextAttr::keyword>(
                     kw<":sorts-description"> | kw<":funs-description"> |
                     kw<":definition"> | kw<":values"> | kw<":notes">) +
                 assign<&ast::TheoryTextAttr::text>(ref<STRING>)) |
             create<ast::TheoryGenericAttr>(
                 assign<&ast::TheoryGenericAttr::attr>(ref<Attribute>))> {};

  struct TheoryDecl
      : rule<create<ast::TheoryDecl>(
            lit<"("> + kw<"theory"> + assign<&ast::TheoryDecl::name>(Symbol) +
            some(append<&ast::TheoryDecl::attrs>(ref<TheoryAttribute>)) +
            lit<")">)> {};

  struct LogicAttribute
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
                 assign<&ast::LogicGenericAttr::attr>(ref<Attribute>))> {};

  struct Logic
      : rule<create<ast::Logic>(
            lit<"("> + kw<"logic"> + assign<&ast::Logic::name>(Symbol) +
            some(append<&ast::Logic::attrs>(ref<LogicAttribute>)) +
            lit<")">)> {};

  // Scripts.

  struct SortDec
      : rule<create<ast::SortDec>(
            lit<"("> + assign<&ast::SortDec::name>(Symbol) +
            assign<&ast::SortDec::arity>(ref<NUMERAL>) + lit<")">)> {};

  struct SelectorDec
      : rule<create<ast::SelectorDec>(
            lit<"("> + assign<&ast::SelectorDec::name>(Symbol) +
            assign<&ast::SelectorDec::sort>(ref<Sort>) + lit<")">)> {};

  struct ConstructorDec
      : rule<create<ast::ConstructorDec>(
            lit<"("> + assign<&ast::ConstructorDec::name>(Symbol) +
            many(
                append<&ast::ConstructorDec::selectors>(ref<SelectorDec>)) +
            lit<")">)> {};

  struct DatatypeDec
      : rule<create<ast::DatatypeDec>(
                 lit<"("> +
                 many(append<&ast::DatatypeDec::constructors>(
                     ref<ConstructorDec>)) +
                 lit<")">) |
             create<ast::DatatypeDec>(
                 lit<"("> + kw<"par"> + lit<"("> +
                 some(append<&ast::DatatypeDec::params>(Symbol)) + lit<")"> +
                 lit<"("> +
                 many(append<&ast::DatatypeDec::constructors>(
                     ref<ConstructorDec>)) +
                 lit<")"> + lit<")">)> {};

  struct FunctionDec
      : rule<create<ast::FunctionDec>(
            lit<"("> + assign<&ast::FunctionDec::name>(Symbol) + lit<"("> +
            many(append<&ast::FunctionDec::params>(ref<SortedVar>)) +
            lit<")"> + assign<&ast::FunctionDec::result>(ref<Sort>) +
            lit<")">)> {};

  struct FunctionDef
      : rule<create<ast::FunctionDef>(
            assign<&ast::FunctionDef::name>(Symbol) + lit<"("> +
            many(append<&ast::FunctionDef::params>(ref<SortedVar>)) +
            lit<")"> + assign<&ast::FunctionDef::result>(ref<Sort>) +
            assign<&ast::FunctionDef::body>(ref<Term>))> {};

  struct PropLiteral
      : rule<create<ast::PropLiteral>(
                 assign<&ast::PropLiteral::symbol>(Symbol)) |
             create<ast::PropLiteral>(
                 lit<"("> + flag<&ast::PropLiteral::negated>(kw<"not">) +
                 assign<&ast::PropLiteral::symbol>(Symbol) + lit<")">)> {};

  // Commands.

  struct SimpleCommand
      : rule<create<ast::SimpleCommand>(assign<&ast::SimpleCommand::name>(
            kw<"check-sat"> | kw<"exit"> | kw<"get-assertions"> |
            kw<"get-assignment"> | kw<"get-model"> | kw<"get-proof"> |
            kw<"get-unsat-assumptions"> | kw<"get-unsat-core"> |
            kw<"reset-assertions"> | kw<"reset">))> {};

  struct AssertCommand
      : rule<create<ast::AssertCommand>(
            kw<"assert"> + assign<&ast::AssertCommand::term>(ref<Term>))> {
  };

  struct CheckSatAssumingCommand
      : rule<create<ast::CheckSatAssumingCommand>(
            kw<"check-sat-assuming"> + lit<"("> +
            many(append<&ast::CheckSatAssumingCommand::assumptions>(
                ref<PropLiteral>)) +
            lit<")">)> {};

  struct DeclareConstCommand
      : rule<create<ast::DeclareConstCommand>(
            kw<"declare-const"> +
            assign<&ast::DeclareConstCommand::name>(Symbol) +
            assign<&ast::DeclareConstCommand::sort>(ref<Sort>))> {};

  struct DeclareDatatypeCommand
      : rule<create<ast::DeclareDatatypeCommand>(
            kw<"declare-datatype"> +
            assign<&ast::DeclareDatatypeCommand::name>(Symbol) +
            assign<&ast::DeclareDatatypeCommand::decl>(ref<DatatypeDec>))> {
  };

  struct DeclareDatatypesCommand
      : rule<create<ast::DeclareDatatypesCommand>(
            kw<"declare-datatypes"> + lit<"("> +
            many(append<&ast::DeclareDatatypesCommand::sorts>(
                ref<SortDec>)) +
            lit<")"> + lit<"("> +
            many(append<&ast::DeclareDatatypesCommand::decls>(
                ref<DatatypeDec>)) +
            lit<")">)> {};

  struct DeclareFunCommand
      : rule<create<ast::DeclareFunCommand>(
            kw<"declare-fun"> + assign<&ast::DeclareFunCommand::name>(Symbol) +
            lit<"("> +
            many(append<&ast::DeclareFunCommand::params>(ref<Sort>)) +
            lit<")"> +
            assign<&ast::DeclareFunCommand::result>(ref<Sort>))> {};

  struct DeclareSortCommand
      : rule<create<ast::DeclareSortCommand>(
            kw<"declare-sort"> +
            assign<&ast::DeclareSortCommand::name>(Symbol) +
            assign<&ast::DeclareSortCommand::arity>(ref<NUMERAL>))> {};

  struct DefineFunCommand
      : rule<create<ast::DefineFunCommand>(
            kw<"define-fun"> +
            assign<&ast::DefineFunCommand::def>(ref<FunctionDef>))> {};

  struct DefineFunRecCommand
      : rule<create<ast::DefineFunRecCommand>(
            kw<"define-fun-rec"> +
            assign<&ast::DefineFunRecCommand::def>(ref<FunctionDef>))> {};

  struct DefineFunsRecCommand
      : rule<create<ast::DefineFunsRecCommand>(
            kw<"define-funs-rec"> + lit<"("> +
            many(append<&ast::DefineFunsRecCommand::decls>(
                ref<FunctionDec>)) +
            lit<")"> + lit<"("> +
            many(append<&ast::DefineFunsRecCommand::bodies>(ref<Term>)) +
            lit<")">)> {};

  struct DefineSortCommand
      : rule<create<ast::DefineSortCommand>(
            kw<"define-sort"> + assign<&ast::DefineSortCommand::name>(Symbol) +
            lit<"("> + many(append<&ast::DefineSortCommand::params>(Symbol)) +
            lit<")"> + assign<&ast::DefineSortCommand::body>(ref<Sort>))> {
  };

  struct EchoCommand
      : rule<create<ast::EchoCommand>(
            kw<"echo"> + assign<&ast::EchoCommand::text>(ref<STRING>))> {};

  struct GetInfoCommand
      : rule<create<ast::GetInfoCommand>(
            kw<"get-info"> +
            assign<&ast::GetInfoCommand::flag>(ref<GenericKeyword>))> {};

  struct GetOptionCommand
      : rule<create<ast::GetOptionCommand>(
            kw<"get-option"> +
            assign<&ast::GetOptionCommand::keyword>(ref<GenericKeyword>))> {};

  struct GetValueCommand
      : rule<create<ast::GetValueCommand>(
            kw<"get-value"> + lit<"("> +
            some(append<&ast::GetValueCommand::terms>(ref<Term>)) +
            lit<")">)> {};

  struct PopCommand
      : rule<create<ast::PopCommand>(
            kw<"pop"> +
            option(assign<&ast::PopCommand::count>(ref<NUMERAL>)))> {};

  struct PushCommand
      : rule<create<ast::PushCommand>(
            kw<"push"> +
            option(assign<&ast::PushCommand::count>(ref<NUMERAL>)))> {};

  struct SetInfoCommand
      : rule<create<ast::SetInfoCommand>(
            kw<"set-info"> +
            assign<&ast::SetInfoCommand::attr>(ref<Attribute>))> {};

  struct SetLogicCommand
      : rule<create<ast::SetLogicCommand>(
            kw<"set-logic"> + assign<&ast::SetLogicCommand::name>(Symbol))> {};

  struct SetOptionCommand
      : rule<create<ast::SetOptionCommand>(
            kw<"set-option"> +
            assign<&ast::SetOptionCommand::option>(ref<Attribute>))> {};

  static constexpr auto command_body =
      ref<AssertCommand> | ref<SimpleCommand> |
      ref<CheckSatAssumingCommand> | ref<DeclareConstCommand> |
      ref<DeclareDatatypeCommand> | ref<DeclareDatatypesCommand> |
      ref<DeclareFunCommand> | ref<DeclareSortCommand> |
      ref<DefineFunCommand> | ref<DefineFunRecCommand> |
      ref<DefineFunsRecCommand> | ref<DefineSortCommand> |
      ref<EchoCommand> | ref<GetInfoCommand> |
      ref<GetOptionCommand> | ref<GetValueCommand> |
      ref<PopCommand> | ref<PushCommand> | ref<SetInfoCommand> |
      ref<SetLogicCommand> | ref<SetOptionCommand>;

  struct Command : rule<lit<"("> + command_body + lit<")">> {};

  struct Script : rule<create<ast::Script>(many(
                          append<&ast::Script::commands>(ref<Command>)))> {
  };

  struct LogicDoc : rule<assign<&ast::LogicDoc::logic>(ref<Logic>)> {};

  struct TheoryDeclDoc
      : rule<assign<&ast::TheoryDeclDoc::decl>(ref<TheoryDecl>)> {};

  struct CheckSatResponse : rule<assign<&ast::CheckSatResponse::result>(
                                    kw<"sat"> | kw<"unsat"> | kw<"unknown">)> {
  };
  struct EchoResponse
      : rule<assign<&ast::EchoResponse::text>(ref<STRING>)> {};
  struct SExprResponse
      : rule<assign<&ast::SExprResponse::value>(s_expr_val)> {};
  struct GeneralResponse
      : rule<(create<ast::SuccessResponse> + kw<"success">) |
             (create<ast::UnsupportedResponse> + kw<"unsupported">) |
             create<ast::ErrorResponse>(
                 lit<"("> + kw<"error"> +
                 assign<&ast::ErrorResponse::message>(ref<STRING>) + lit<")">) |
             ref<CheckSatResponse> | ref<EchoResponse> |
             ref<SExprResponse>> {};
  struct ResponseDoc : rule<some(append<&ast::ResponseDoc::responses>(
                               ref<GeneralResponse>))> {};
  struct Start : rule<ref<Script> | ref<LogicDoc> |
                          ref<TheoryDeclDoc> | ref<ResponseDoc>> {};

  using entry = Start;
  using ignored = list<WS, COMMENT>;
};

}  // namespace smtlibv2
