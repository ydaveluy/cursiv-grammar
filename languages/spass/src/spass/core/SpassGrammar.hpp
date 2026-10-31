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

inline constexpr auto log_state_of = [](std::string_view t) {
  if (t == "satisfiable") return ast::LogState::satisfiable;
  if (t == "unsatisfiable") return ast::LogState::unsatisfiable;
  return ast::LogState::unknown;
};

inline constexpr auto origin_of = [](std::string_view t) {
  return t == "axioms" ? ast::Origin::axioms : ast::Origin::conjectures;
};

// Native dfglexer.c reserves these tokens globally. Keep the name token
// permissive and report a reserved spelling as a conversion diagnostic.
inline constexpr auto identifier_value = [](std::string_view text)
    -> cursiv::ConversionResult<std::string_view> {
  constexpr std::string_view reserved[] = {
      "MIN", "AED", "App", "CRW", "Con", "Def", "EmS",
      "EqF", "EqR", "Fac", "Inp", "MPm", "MRR", "OHy",
      "OPm", "Obv", "Res", "Rew", "SHy", "SPASS", "SPm",
      "SSi", "SoR", "SpL", "SpR", "Spt", "Ter", "Top",
      "URR", "UnC", "and", "author", "axioms", "begin_problem", "by",
      "clause", "cnf", "concept_formula", "conjectures", "date", "description", "dnf",
      "eml", "end_of_list", "end_problem", "equal", "equiv", "exists", "extrafuns",
      "false", "freely", "forall", "formula", "functions", "generated", "hypothesis",
      "implied", "implies", "include", "list_of_clauses", "list_of_declarations", "list_of_descriptions", "list_of_formulae",
      "list_of_general_settings", "list_of_includes", "list_of_proof", "list_of_settings", "list_of_special_formulae", "list_of_symbols", "logic",
      "name", "nand", "nor", "not", "operators", "or", "predicate",
      "predicates", "prop_formula", "quantifiers", "rel_formula", "role_formula", "satisfiable", "set_flag",
      "set_precedence", "set_selection", "set_ClauseFormulaRelation", "set_DomPred", "sort", "sorts", "splitlevel",
      "status", "step", "subsort", "translpairs", "true", "unknown", "unsatisfiable",
      "version", "xor",
  };
  for (const auto word : reserved)
    if (word == text)
      return cursiv::ConversionResult<std::string_view>::err(
          "A DFG keyword cannot be used as an identifier.");
  return cursiv::ConversionResult<std::string_view>::ok(text);
};

struct Spass : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\n)">)> {};

  // SPASS accepts digit-leading names and numeric symbol names as well.
  struct Identifier : terminal<as<identifier_value>(
      some(w) | (lit<"-"> + some(d)))> {};
  struct NUMBER : terminal<as<std::int64_t>(option(lit<"-">) + some(d))> {};

  // '{*' everything up to and including the closing '*}', the delimiters
  // stripped by the converter.
  struct FREE_TEXT : terminal<as<strip_braces>(through(lit<"{*">, lit<"*}">))> {
  };

  struct LOG_STATE
      : terminal<as<log_state_of>(kw<"satisfiable"> | kw<"unsatisfiable"> |
                                  kw<"unknown">)> {};
  struct ORIGIN_TYPE
      : terminal<as<origin_of>(kw<"axioms"> | kw<"conjectures">)> {};

  static constexpr auto word_boundary = w;

  struct Term;

  struct TermList
      : rule<lit<"["> +
             some(append<&ast::TermList::items>(ref<Term>), lit<",">) +
             lit<"]">> {};

  static constexpr auto oneArgument = lit<"("> +
      append<&ast::AppTerm::args>(ref<Term>) + lit<")">;
  static constexpr auto twoArguments = lit<"("> +
      append<&ast::AppTerm::args>(ref<Term>) + lit<","> +
      append<&ast::AppTerm::args>(ref<Term>) + lit<")">;
  static constexpr auto arguments = lit<"("> +
      some(append<&ast::AppTerm::args>(ref<Term>), lit<",">) + lit<")">;

  // Shared argument fragments express the reserved connectives' arities.
  struct Term : rule<
      (assign<&ast::QuantTerm::quantifier>(kw<"forall"> | kw<"exists">) +
       lit<"("> + assign<&ast::QuantTerm::vars>(ref<TermList>) + lit<","> +
       assign<&ast::QuantTerm::body>(ref<Term>) + lit<")">) |
      (assign<&ast::AppTerm::symbol>(kw<"not">) + oneArgument) |
      (assign<&ast::AppTerm::symbol>(kw<"equal"> | kw<"implies"> |
          kw<"implied"> | kw<"equiv"> | kw<"nor"> | kw<"nand"> | kw<"xor">) +
       twoArguments) |
      (assign<&ast::AppTerm::symbol>(kw<"and"> | kw<"or">) + arguments) |
      assign<&ast::AppTerm::symbol>(kw<"true"> | kw<"false">) |
      (assign<&ast::AppTerm::symbol>(ref<Identifier>) + option(arguments))> {};

  static constexpr auto labelExpr = ref<Identifier> | ref<NUMBER>;

  struct SymSpec
      : rule<(lit<"("> + assign<&ast::SymSpec::name>(ref<Identifier>) +
              lit<","> + assign<&ast::SymSpec::arity>(as<std::int64_t(-1)>(lit<"-1">) |
                                                       ref<NUMBER>) +
              lit<")">) |
             assign<&ast::SymSpec::name>(ref<Identifier>)> {};

  struct SymbolList
      : rule<kw<"list_of_symbols"> + lit<"."> +
             option(kw<"functions"> + lit<"["> +
                    some(append<&ast::SymbolList::functions>(
                             ref<SymSpec>),
                         lit<",">) +
                    lit<"]"> + lit<".">) +
             option(kw<"predicates"> + lit<"["> +
                    some(append<&ast::SymbolList::predicates>(
                             ref<SymSpec>),
                         lit<",">) +
                    lit<"]"> + lit<".">) +
             option(kw<"sorts"> + lit<"["> +
                    some(append<&ast::SymbolList::sorts>(ref<Identifier>),
                         lit<",">) +
                    lit<"]"> + lit<".">) +
             kw<"end_of_list"> + lit<".">> {};

  struct SubsortDecl
      : rule<kw<"subsort"> + lit<"("> +
             assign<&ast::SubsortDecl::first>(ref<Identifier>) + lit<","> +
             assign<&ast::SubsortDecl::second>(ref<Identifier>) + lit<")"> +
             lit<".">> {};

  struct TermDecl
      : rule<assign<&ast::TermDecl::term>(ref<Term>) + lit<".">> {};

  struct PredDecl
      : rule<kw<"predicate"> + lit<"("> +
             assign<&ast::PredDecl::pred>(ref<Identifier>) +
             some(lit<","> + append<&ast::PredDecl::sorts>(ref<Identifier>)) +
             lit<")"> + lit<".">> {};

  struct GenDecl
      : rule<kw<"sort"> + assign<&ast::GenDecl::sort>(ref<Identifier>) +
             option(flag<&ast::GenDecl::freely>(kw<"freely">)) +
             kw<"generated_by"> + lit<"["> +
             some(append<&ast::GenDecl::functions>(ref<Identifier>),
                  lit<",">) +
             lit<"]"> + lit<".">> {};

  struct DeclarationList
      : rule<kw<"list_of_declarations"> + lit<"."> +
             many(append<&ast::DeclarationList::declarations>(
                 ref<SubsortDecl> | ref<TermDecl> |
                 ref<PredDecl> | ref<GenDecl>)) +
             kw<"end_of_list"> + lit<".">> {};

  struct Formula
      : rule<kw<"formula"> + lit<"("> +
             assign<&ast::Formula::term>(ref<Term>) +
             option(lit<","> + assign<&ast::Formula::label>(labelExpr)) +
             lit<")"> + lit<".">> {};

  struct FormulaList
      : rule<kw<"list_of_formulae"> + lit<"("> +
             assign<&ast::FormulaList::origin>(ref<ORIGIN_TYPE>) + lit<")"> +
             lit<"."> + many(append<&ast::FormulaList::formulas>(
                                 ref<Formula>)) +
             kw<"end_of_list"> + lit<".">> {};

  // In native plain clauses function terms, atoms and negated constraint
  // literals have distinct syntax, despite sharing the AppTerm AST family.
  struct FunctionTerm
      : rule<assign<&ast::AppTerm::symbol>(ref<Identifier>) +
             option(lit<"("> + some(append<&ast::AppTerm::args>(
                                        ref<FunctionTerm>), lit<",">) + lit<")">)> {};
  struct Atom
      : rule<(assign<&ast::AppTerm::symbol>(kw<"equal">) + lit<"("> +
              append<&ast::AppTerm::args>(ref<FunctionTerm>) + lit<","> +
              append<&ast::AppTerm::args>(ref<FunctionTerm>) + lit<")">) |
             assign<&ast::AppTerm::symbol>(kw<"true"> | kw<"false">) |
             (assign<&ast::AppTerm::symbol>(ref<Identifier>) +
              option(lit<"("> + some(append<&ast::AppTerm::args>(
                                         ref<FunctionTerm>), lit<",">) + lit<")">))> {};
  struct ConstraintLiteral
      : rule<(assign<&ast::AppTerm::symbol>(kw<"not">) + lit<"("> +
              append<&ast::AppTerm::args>(ref<Atom>) + lit<")">) | ref<Atom>> {};
  struct PlainClause
      : rule<many(append<&ast::Clause::constraints>(ref<ConstraintLiteral>)) +
             flag<&ast::Clause::plain>(lit<"||">) +
             many(append<&ast::Clause::antecedent>(ref<Atom>)) +
             lit<"->"> + many(append<&ast::Clause::succedent>(ref<Atom>))> {};

  struct CnfClause
      : rule<(kw<"forall"> + lit<"("> +
              assign<&ast::Clause::vars>(ref<TermList>) + lit<","> +
              kw<"or"> + lit<"("> +
              some(append<&ast::Clause::literals>(ref<Term>), lit<",">) +
              lit<")"> + lit<")">) |
             (kw<"or"> + lit<"("> +
              some(append<&ast::Clause::literals>(ref<Term>), lit<",">) +
              lit<")">) | ref<PlainClause>> {};

  struct DnfClause
      : rule<(kw<"exists"> + lit<"("> +
              assign<&ast::Clause::vars>(ref<TermList>) + lit<","> +
              flag<&ast::Clause::isDnf>(kw<"and">) + lit<"("> +
              some(append<&ast::Clause::literals>(ref<Term>), lit<",">) +
              lit<")"> + lit<")">) |
             (flag<&ast::Clause::isDnf>(kw<"and">) + lit<"("> +
              some(append<&ast::Clause::literals>(ref<Term>), lit<",">) +
              lit<")">)> {};

  struct Clause : rule<ref<CnfClause> | ref<DnfClause>> {};

  template <class Body>
  struct ClauseEntryFor
      : rule<kw<"clause"> + lit<"("> +
             assign<&ast::ClauseEntry::clause>(ref<Body>) +
             option(lit<","> + assign<&ast::ClauseEntry::label>(labelExpr)) +
             lit<")"> + lit<".">> {};

  struct ClauseList
      : rule<kw<"list_of_clauses"> + lit<"("> +
             assign<&ast::ClauseList::origin>(ref<ORIGIN_TYPE>) + lit<","> +
             ((kw<"cnf"> + lit<")"> + lit<"."> +
               many(append<&ast::ClauseList::clauses>(ref<ClauseEntryFor<CnfClause>>))) |
              (flag<&ast::ClauseList::isDnf>(kw<"dnf">) + lit<")"> + lit<"."> +
               many(append<&ast::ClauseList::clauses>(ref<ClauseEntryFor<DnfClause>>)))) +
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

  struct ResultQuant
      : rule<(assign<&ast::QuantTerm::quantifier>(kw<"forall">) + lit<"("> +
              assign<&ast::QuantTerm::vars>(ref<TermList>) + lit<","> +
              !orCall + assign<&ast::QuantTerm::body>(ref<Term>) +
              lit<")">) |
             (assign<&ast::QuantTerm::quantifier>(kw<"exists">) + lit<"("> +
              assign<&ast::QuantTerm::vars>(ref<TermList>) + lit<","> +
              !andCall + assign<&ast::QuantTerm::body>(ref<Term>) +
              lit<")">)> {};

  static constexpr auto referenceExpr =
      ref<NUMBER> | (!ref<NUMBER> + ref<Term>);
  // The guard is kept: without it a quantifier or clause reads as a term.
  static constexpr auto resultExpr =
      ref<ResultQuant> |
      (!(kw<"forall"> | kw<"exists"> | orCall | andCall) + ref<Term>) |
      ref<Clause>;
  static constexpr auto ruleApplExpr = ref<Term> | userRuleApplWord;
  static constexpr auto parentExpr = referenceExpr;
  static constexpr auto keyExpr = ref<Term> | kw<"splitlevel">;
  static constexpr auto valueExpr = referenceExpr;

  struct ParentList
      : rule<lit<"["> +
             some(append<&ast::ParentList::items>(parentExpr), lit<",">) +
             lit<"]">> {};

  struct AssocEntry
      : rule<assign<&ast::AssocEntry::key>(keyExpr) + lit<":"> +
             assign<&ast::AssocEntry::value>(valueExpr)> {};

  struct AssocList
      : rule<lit<"["> +
             some(append<&ast::AssocList::entries>(ref<AssocEntry>),
                  lit<",">) +
             lit<"]">> {};

  struct Step
      : rule<kw<"step"> + lit<"("> +
             assign<&ast::Step::reference>(referenceExpr) + lit<","> +
             assign<&ast::Step::result>(resultExpr) + lit<","> +
             assign<&ast::Step::ruleAppl>(ruleApplExpr) + lit<","> +
             assign<&ast::Step::parents>(ref<ParentList>) +
             option(lit<","> +
                    assign<&ast::Step::assoc>(ref<AssocList>)) +
             lit<")"> + lit<".">> {};

  struct ProofList
      : rule<kw<"list_of_proof"> +
             option(lit<"("> +
                    assign<&ast::ProofList::proofType>(
                        ref<Identifier> | kw<"SPASS">) +
                    option(lit<","> +
                           assign<&ast::ProofList::assoc>(
                               ref<AssocList>)) +
                    lit<")">) +
             lit<"."> +
             many(append<&ast::ProofList::steps>(ref<Step>)) +
             kw<"end_of_list"> + lit<".">> {};

  struct SettingEntry
      : rule<kw<"hypothesis"> + lit<"["> +
             some(append<&ast::SettingEntry::labels>(labelExpr), lit<",">) +
             lit<"]"> + lit<".">> {};

  struct GeneralSettings
      : rule<kw<"list_of_general_settings"> +
             some(append<&ast::GeneralSettings::entries>(
                 ref<SettingEntry>)) +
             kw<"end_of_list"> + lit<".">> {};

  static constexpr auto settingLabelWord =
      kw<"KIV"> | kw<"LEM"> | kw<"OTTER"> | kw<"PROTEIN"> |
      kw<"SATURATE"> | kw<"3TAP"> | kw<"SETHEO"> | kw<"SPASS">;

  struct NamedSettings
      : rule<kw<"list_of_settings"> + lit<"("> +
             assign<&ast::NamedSettings::label>(settingLabelWord) +
             lit<")"> + lit<"."> +
             option(assign<&ast::NamedSettings::text>(ref<FREE_TEXT>)) +
             kw<"end_of_list"> + lit<".">> {};

  struct Description
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

  struct LogicalPart
      : rule<option(assign<&ast::LogicalPart::symbols>(ref<SymbolList>)) +
             option(assign<&ast::LogicalPart::declarations>(
                 ref<DeclarationList>)) +
             many(append<&ast::LogicalPart::formulaLists>(
                 ref<FormulaList>)) +
             many(append<&ast::LogicalPart::clauseLists>(
                 ref<ClauseList>)) +
             many(append<&ast::LogicalPart::proofLists>(
                 ref<ProofList>))> {};

  struct FILENAME : terminal<as<[](std::string_view text) {
      return text.substr(1, text.size() - 2);
    }>(lit<"'"> + many(cr<"^'">) + lit<"'">)> {};
  struct Include
      : rule<kw<"include"> + lit<"("> +
             assign<&ast::Include::filename>(ref<FILENAME>) +
             option(lit<","> + flag<&ast::Include::hasSelection>(lit<"[">) +
                    some(append<&ast::Include::selection>(ref<Identifier>), lit<",">) +
                    lit<"]">) + lit<")"> + lit<".">> {};
  static constexpr auto include_list =
      kw<"list_of_includes"> + lit<"."> +
      many(append<&ast::Problem::includes>(ref<Include>)) +
      kw<"end_of_list"> + lit<".">;

  struct Problem
      : rule<kw<"begin_problem"> + lit<"("> +
             assign<&ast::Problem::name>(ref<Identifier>) + lit<")"> +
             lit<"."> +
             assign<&ast::Problem::description>(ref<Description>) +
             assign<&ast::Problem::logicalPart>(ref<LogicalPart>) +
             option(include_list) +
             many(append<&ast::Problem::settings>(
                 ref<GeneralSettings> | ref<NamedSettings>)) +
             kw<"end_problem"> + lit<".">> {};

  using entry = Problem;
  using hidden = COMMENT;
  using ignored = WS;
};

}  // namespace spass
