// The spass language: its grammar. A SPASS problem file opens with
// begin_problem(name)., a description, then the symbol/declaration/
// formula/clause/proof lists and settings blocks, closed by end_problem.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "spass/core/ast.hpp"

namespace spass {

inline constexpr auto strip_braces = [](std::string_view t) {
  return t.substr(2, t.size() - 4);
};
using FreeTextConv = cursiv::text_conv<strip_braces>;

inline constexpr auto log_state_of = [](std::string_view t) {
  if (t == "satisfiable") return ast::LogState::satisfiable;
  if (t == "unsatisfiable") return ast::LogState::unsatisfiable;
  return ast::LogState::unknown;
};
using LogStateConv = cursiv::text_conv<log_state_of>;

inline constexpr auto origin_of = [](std::string_view t) {
  return t == "axioms" ? ast::Origin::axioms : ast::Origin::conjectures;
};
using OriginConv = cursiv::text_conv<origin_of>;

inline constexpr auto is_dnf_type = [](std::string_view t) {
  return t == "dnf";
};
using DnfTypeConv = cursiv::text_conv<is_dnf_type>;

struct Spass : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  // Letter (Letter | Digit | '-')*; keywords below reserve their own
  // spelling globally, everywhere, not only where a state expects one.
  struct Identifier : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9-">)> {};
  struct NUMBER : terminal<some(d), std::int64_t> {};
  struct MOne : terminal<lit<"-1">, value<std::int64_t(-1)>> {};

  // '{*' everything up to and including the closing '*}', the delimiters
  // stripped by the converter.
  struct FREE_TEXT : terminal<through(lit<"{*">, lit<"*}">), FreeTextConv> {};

  struct LOG_STATE
      : terminal<kw<"satisfiable"> | kw<"unsatisfiable"> | kw<"unknown">,
                 LogStateConv> {};
  struct ORIGIN_TYPE
      : terminal<kw<"axioms"> | kw<"conjectures">, OriginConv> {};
  struct CLAUSE_TYPE : terminal<kw<"cnf"> | kw<"dnf">, DnfTypeConv> {};

  static constexpr auto word_boundary = cr<"a-zA-Z0-9-">;

  struct TermRule;

  static constexpr auto symbolWord =
      kw<"equal"> | kw<"true"> | kw<"false"> | kw<"or"> | kw<"and"> |
      kw<"not"> | kw<"implies"> | kw<"implied"> | kw<"equiv"> |
      ref<Identifier>;

  struct TermListRule
      : rule<lit<"["> +
             some(append<&ast::TermList::items>(ref<TermRule>), lit<",">) +
             lit<"]">> {};

  // A term already reads a quantifier headed by `forall`/`exists`/a bare
  // identifier, so a declaration's `forall(vars, term).` shape is just a
  // bare term -- no separate alternative needed.
  struct TermRule
      : rule<(assign<&ast::QuantTerm::quantifier>(
                  kw<"forall"> | kw<"exists"> | ref<Identifier>) +
              lit<"("> + assign<&ast::QuantTerm::vars>(ref<TermListRule>) +
              lit<","> + assign<&ast::QuantTerm::body>(ref<TermRule>) +
              lit<")">) |
             (assign<&ast::AppTerm::symbol>(symbolWord) +
              option(lit<"("> +
                     some(append<&ast::AppTerm::args>(ref<TermRule>),
                          lit<",">) +
                     lit<")">))> {};

  static constexpr auto labelExpr = ref<Identifier> | ref<NUMBER>;

  struct SymSpecRule
      : rule<(lit<"("> + assign<&ast::SymSpec::name>(ref<Identifier>) +
              lit<","> + assign<&ast::SymSpec::arity>(ref<MOne> |
                                                       ref<NUMBER>) +
              lit<")">) |
             assign<&ast::SymSpec::name>(ref<Identifier>)> {};

  struct SymbolListRule
      : rule<kw<"list_of_symbols"> + lit<"."> +
             option(kw<"functions"> + lit<"["> +
                    some(append<&ast::SymbolList::functions>(
                             ref<SymSpecRule>),
                         lit<",">) +
                    lit<"]"> + lit<".">) +
             option(kw<"predicates"> + lit<"["> +
                    some(append<&ast::SymbolList::predicates>(
                             ref<SymSpecRule>),
                         lit<",">) +
                    lit<"]"> + lit<".">) +
             option(kw<"sorts"> + lit<"["> +
                    some(append<&ast::SymbolList::sorts>(ref<Identifier>),
                         lit<",">) +
                    lit<"]"> + lit<".">) +
             kw<"end_of_list"> + lit<".">> {};

  struct SubsortDeclRule
      : rule<kw<"subsort"> + lit<"("> +
             assign<&ast::SubsortDecl::first>(ref<Identifier>) + lit<","> +
             assign<&ast::SubsortDecl::second>(ref<Identifier>) + lit<")"> +
             lit<".">> {};

  struct TermDeclRule
      : rule<assign<&ast::TermDecl::term>(ref<TermRule>) + lit<".">> {};

  struct PredDeclRule
      : rule<kw<"predicate"> + lit<"("> +
             assign<&ast::PredDecl::pred>(ref<Identifier>) +
             some(lit<","> + append<&ast::PredDecl::sorts>(ref<Identifier>)) +
             lit<")"> + lit<".">> {};

  struct GenDeclRule
      : rule<kw<"sort"> + assign<&ast::GenDecl::sort>(ref<Identifier>) +
             option(flag<&ast::GenDecl::freely>(kw<"freely">)) +
             kw<"generated_by"> + lit<"["> +
             some(append<&ast::GenDecl::functions>(ref<Identifier>),
                  lit<",">) +
             lit<"]"> + lit<".">> {};

  struct DeclarationListRule
      : rule<kw<"list_of_declarations"> + lit<"."> +
             many(append<&ast::DeclarationList::declarations>(
                 ref<SubsortDeclRule> | ref<TermDeclRule> |
                 ref<PredDeclRule> | ref<GenDeclRule>)) +
             kw<"end_of_list"> + lit<".">> {};

  struct FormulaRule
      : rule<kw<"formula"> + lit<"("> +
             option(assign<&ast::Formula::term>(ref<TermRule>)) +
             option(lit<","> + assign<&ast::Formula::label>(labelExpr)) +
             lit<")"> + lit<".">> {};

  struct FormulaListRule
      : rule<kw<"list_of_formulae"> + lit<"("> +
             assign<&ast::FormulaList::origin>(ref<ORIGIN_TYPE>) + lit<")"> +
             lit<"."> + many(append<&ast::FormulaList::formulas>(
                                 ref<FormulaRule>)) +
             kw<"end_of_list"> + lit<".">> {};

  struct ClauseRule
      : rule<(kw<"forall"> + lit<"("> +
              assign<&ast::Clause::vars>(ref<TermListRule>) + lit<","> +
              kw<"or"> + lit<"("> +
              some(append<&ast::Clause::literals>(ref<TermRule>), lit<",">) +
              lit<")"> + lit<")">) |
             (kw<"or"> + lit<"("> +
              some(append<&ast::Clause::literals>(ref<TermRule>), lit<",">) +
              lit<")">) |
             (kw<"exists"> + lit<"("> +
              assign<&ast::Clause::vars>(ref<TermListRule>) + lit<","> +
              flag<&ast::Clause::isDnf>(kw<"and">) + lit<"("> +
              some(append<&ast::Clause::literals>(ref<TermRule>), lit<",">) +
              lit<")"> + lit<")">) |
             (flag<&ast::Clause::isDnf>(kw<"and">) + lit<"("> +
              some(append<&ast::Clause::literals>(ref<TermRule>), lit<",">) +
              lit<")">)> {};

  struct ClauseEntryRule
      : rule<kw<"clause"> + lit<"("> +
             option(assign<&ast::ClauseEntry::clause>(ref<ClauseRule>)) +
             option(lit<","> + assign<&ast::ClauseEntry::label>(labelExpr)) +
             lit<")"> + lit<".">> {};

  struct ClauseListRule
      : rule<kw<"list_of_clauses"> + lit<"("> +
             assign<&ast::ClauseList::origin>(ref<ORIGIN_TYPE>) + lit<","> +
             assign<&ast::ClauseList::isDnf>(ref<CLAUSE_TYPE>) + lit<")"> +
             lit<"."> + many(append<&ast::ClauseList::clauses>(
                                 ref<ClauseEntryRule>)) +
             kw<"end_of_list"> + lit<".">> {};

  static constexpr auto userRuleApplWord =
      kw<"GeR"> | kw<"SpL"> | kw<"SpR"> | kw<"EqF"> | kw<"Rew"> |
      kw<"Obv"> | kw<"EmS"> | kw<"SoR"> | kw<"EqR"> | kw<"MPm"> |
      kw<"SPm"> | kw<"OPm"> | kw<"SHy"> | kw<"OHy"> | kw<"URR"> |
      kw<"Fac"> | kw<"Spt"> | kw<"Inp"> | kw<"Con"> | kw<"RRE"> |
      kw<"SSi"> | kw<"ClR"> | kw<"UnC"> | kw<"Ter">;

  // A result spelling a clause is the clause: the term that reads the same
  // text dies where the clause's literals open. A lookahead reads one token
  // and its guard, so `or (` spaced keeps both readings to the election.
  static constexpr auto orCall = lit<"or", !cr<"^(">>;
  static constexpr auto andCall = lit<"and", !cr<"^(">>;

  struct ResultQuantRule
      : rule<(assign<&ast::QuantTerm::quantifier>(kw<"forall">) + lit<"("> +
              assign<&ast::QuantTerm::vars>(ref<TermListRule>) + lit<","> +
              !orCall + assign<&ast::QuantTerm::body>(ref<TermRule>) +
              lit<")">) |
             (assign<&ast::QuantTerm::quantifier>(kw<"exists">) + lit<"("> +
              assign<&ast::QuantTerm::vars>(ref<TermListRule>) + lit<","> +
              !andCall + assign<&ast::QuantTerm::body>(ref<TermRule>) +
              lit<")">)> {};

  static constexpr auto referenceExpr = ref<TermRule> | ref<NUMBER>;
  // The guard is kept: without it a quantifier or clause reads as a term.
  static constexpr auto resultExpr =
      ref<ResultQuantRule> |
      (!(kw<"forall"> | kw<"exists"> | orCall | andCall) + ref<TermRule>) |
      ref<ClauseRule>;
  static constexpr auto ruleApplExpr = ref<TermRule> | userRuleApplWord;
  static constexpr auto parentExpr = ref<TermRule> | ref<NUMBER>;
  static constexpr auto keyExpr = ref<TermRule> | kw<"splitlevel">;
  static constexpr auto valueExpr = ref<TermRule> | ref<NUMBER>;

  struct ParentListRule
      : rule<lit<"["> +
             some(append<&ast::ParentList::items>(parentExpr), lit<",">) +
             lit<"]">> {};

  struct AssocEntryRule
      : rule<assign<&ast::AssocEntry::key>(keyExpr) + lit<":"> +
             assign<&ast::AssocEntry::value>(valueExpr)> {};

  struct AssocListRule
      : rule<lit<"["> +
             some(append<&ast::AssocList::entries>(ref<AssocEntryRule>),
                  lit<",">) +
             lit<"]">> {};

  struct StepRule
      : rule<kw<"step"> + lit<"("> +
             assign<&ast::Step::reference>(referenceExpr) + lit<","> +
             assign<&ast::Step::result>(resultExpr) + lit<","> +
             assign<&ast::Step::ruleAppl>(ruleApplExpr) + lit<","> +
             assign<&ast::Step::parents>(ref<ParentListRule>) +
             option(lit<","> +
                    assign<&ast::Step::assoc>(ref<AssocListRule>)) +
             lit<")"> + lit<".">> {};

  struct ProofListRule
      : rule<kw<"list_of_proof"> +
             option(lit<"("> +
                    assign<&ast::ProofList::proofType>(
                        ref<Identifier> | kw<"SPASS">) +
                    option(lit<","> +
                           assign<&ast::ProofList::assoc>(
                               ref<AssocListRule>)) +
                    lit<")">) +
             lit<"."> +
             many(append<&ast::ProofList::steps>(ref<StepRule>)) +
             kw<"end_of_list"> + lit<".">> {};

  struct SettingEntryRule
      : rule<kw<"hypothesis"> + lit<"["> +
             some(append<&ast::SettingEntry::labels>(labelExpr), lit<",">) +
             lit<"]"> + lit<".">> {};

  struct GeneralSettingsRule
      : rule<kw<"list_of_general_settings"> +
             some(append<&ast::GeneralSettings::entries>(
                 ref<SettingEntryRule>)) +
             kw<"end_of_list"> + lit<".">> {};

  static constexpr auto settingLabelWord =
      kw<"KIV"> | kw<"LEM"> | kw<"OTTER"> | kw<"PROTEIN"> |
      kw<"SATURATE"> | kw<"3TAP"> | kw<"SETHEO"> | kw<"SPASS">;

  struct NamedSettingsRule
      : rule<kw<"list_of_settings"> + lit<"("> +
             assign<&ast::NamedSettings::label>(settingLabelWord) +
             lit<")"> + lit<"."> +
             option(assign<&ast::NamedSettings::text>(ref<FREE_TEXT>)) +
             kw<"end_of_list"> + lit<".">> {};

  struct DescriptionRule
      : rule<kw<"list_of_descriptions"> + lit<"."> + kw<"name"> + lit<"("> +
             option(assign<&ast::Description::name>(ref<FREE_TEXT>)) +
             lit<")"> + lit<"."> + kw<"author"> + lit<"("> +
             option(assign<&ast::Description::author>(ref<FREE_TEXT>)) +
             lit<")"> + lit<"."> +
             option(kw<"version"> + lit<"("> +
                    option(assign<&ast::Description::version>(
                        ref<FREE_TEXT>)) +
                    lit<")"> + lit<".">) +
             option(kw<"logic"> + lit<"("> +
                    option(assign<&ast::Description::logic>(ref<FREE_TEXT>)) +
                    lit<")"> + lit<".">) +
             kw<"status"> + lit<"("> +
             assign<&ast::Description::status>(ref<LOG_STATE>) + lit<")"> +
             lit<"."> + kw<"description"> + lit<"("> +
             option(assign<&ast::Description::text>(ref<FREE_TEXT>)) +
             lit<")"> + lit<"."> +
             option(kw<"date"> + lit<"("> +
                    option(assign<&ast::Description::date>(ref<FREE_TEXT>)) +
                    lit<")"> + lit<".">) +
             kw<"end_of_list"> + lit<".">> {};

  struct LogicalPartRule
      : rule<option(assign<&ast::LogicalPart::symbols>(ref<SymbolListRule>)) +
             option(assign<&ast::LogicalPart::declarations>(
                 ref<DeclarationListRule>)) +
             many(append<&ast::LogicalPart::formulaLists>(
                 ref<FormulaListRule>)) +
             many(append<&ast::LogicalPart::clauseLists>(
                 ref<ClauseListRule>)) +
             many(append<&ast::LogicalPart::proofLists>(
                 ref<ProofListRule>))> {};

  struct ProblemRule
      : rule<kw<"begin_problem"> + lit<"("> +
             assign<&ast::Problem::name>(ref<Identifier>) + lit<")"> +
             lit<"."> +
             assign<&ast::Problem::description>(ref<DescriptionRule>) +
             assign<&ast::Problem::logicalPart>(ref<LogicalPartRule>) +
             many(append<&ast::Problem::settings>(
                 ref<GeneralSettingsRule> | ref<NamedSettingsRule>)) +
             kw<"end_problem"> + lit<".">> {};

  using entry = ProblemRule;
  using ignore = WS;
};

}  // namespace spass
