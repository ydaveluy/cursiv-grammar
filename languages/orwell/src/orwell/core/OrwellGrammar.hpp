// The orwell language: its grammar.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "orwell/core/ast.hpp"

namespace orwell {

inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource*) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Orwell : cursiv::GrammarSpec {
  // The operator alphabet, spelled once and shared by OP, the ID tail and
  // the maximal-munch guards below.
  static constexpr auto Sym = cr<R"(+*=<>~&\\/^:#!.;|?@`$%{}-)">;
  static constexpr auto IdTail = cr<R"(a-zA-Z0-9'_+*=<>~&\\/^:#!.;|?@`$%{}-)">;
  static constexpr auto word_boundary = IdTail;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  // ID absorbs operator symbols past its first letter: an identifier
  // written flush against an operator (`x+`, no space) reads as one ID
  // token, not ID then OP.
  struct ID : terminal<cr<"a-zA-Z"> + many(IdTail)> {};

  struct INTEGER : terminal<some(d), std::int64_t> {};
  // The exponent sign is an em dash, not a hyphen: `1.0e—3`, never
  // `1.0e-3`.
  struct FLOAT : terminal<some(d) + lit<"."> + some(d) +
                               option(lit<"e"> + option(lit<"—">) + some(d)),
                           double> {};
  struct STRING
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};

  // One byte, so `assoc DIGIT` reads a lone precedence digit; cursiv reads
  // it here because only DIGIT is legal in this position.
  struct DIGIT : terminal<d, std::int8_t> {};

  struct OP : terminal<some(Sym) | (lit<"$"> + cr<"a-zA-Z"> + many(IdTail))> {
  };

  static constexpr auto Eq = lit<"==", !Sym>;
  static constexpr auto ConEq = lit<":==", !Sym>;
  static constexpr auto DColon = lit<"::", !Sym>;
  static constexpr auto Assign = lit<"=", !Sym>;
  static constexpr auto Semi = lit<";", !Sym>;
  static constexpr auto Bar = lit<"|", !Sym>;
  static constexpr auto LArrow = lit<"<-", !Sym>;
  static constexpr auto DotDot = lit<"..", !Sym>;

  // The reference's punctuation is never an operator: its literal tokens
  // win a tie against OP. Read as one, the `=` of an equation lets the next
  // equation run on inside a pattern or a term, to the end of input.
  struct OpRule : rule<!(Eq | ConEq | DColon | Assign | Semi | Bar | LArrow |
                         DotDot) +
                       ref<OP>> {};

  // A bare `%` cannot open a prefix PATTERN: `%else` also precedes a full
  // `pat = rhs`, so nothing downstream ever tells the two apart by
  // lookahead -- `%else f n = n` parses to completion either way. Every
  // other prefix operator, including a longer one starting with `%`, is
  // unaffected. Its own rule, not a bare expression: a guard spliced ahead
  // of `ref<OP>` in an `assign<>` produces no value on its own.
  struct PatPrefixOpRule : rule<!lit<"%", !Sym> + ref<OpRule>> {};

  struct TermRule;
  struct Term1Rule;
  struct Term2Rule;
  struct PrimaryRule;
  struct PrimaryNameRule;
  struct SectionRule;
  struct ListRule;

  struct PatRule;
  struct Pat1Rule;
  struct Pat2Rule;
  struct PatPrimaryRule;
  struct PatPrimaryNameRule;
  struct PatSectionRule;

  struct TypeRule;
  struct TyTerm1Rule;
  struct TyTerm2Rule;
  struct TyPrimaryRule;
  struct TyPrimaryNameRule;
  struct TySectionRule;
  struct TyLhsRule;
  struct TyLhs1Rule;
  struct TyLhsPrimaryRule;
  struct TyLhsSectionRule;

  struct DefRule;
  struct DeclRule;
  struct LastDeclRule;

  // ---- terms -------------------------------------------------------------

  struct ListFormRule
      : rule<create<ast::ListTerm>(
            lit<"["> +
            option(some(append<&ast::ListTerm::items>(ref<TermRule>),
                        lit<",">)) +
            lit<"]">)> {};

  struct UptoRule
      : rule<create<ast::ListRange>(
            lit<"["> + assign<&ast::ListRange::from>(ref<TermRule>) +
            option(lit<","> + assign<&ast::ListRange::step>(ref<TermRule>)) +
            DotDot + option(assign<&ast::ListRange::to>(ref<TermRule>)) +
            lit<"]">)> {};

  struct GuardQualifierRule
      : rule<create<ast::GuardQualifier>(
            assign<&ast::GuardQualifier::condition>(ref<TermRule>))> {};
  struct GeneratorQualifierRule
      : rule<create<ast::GeneratorQualifier>(
            assign<&ast::GeneratorQualifier::pat>(ref<PatRule>) + LArrow +
            assign<&ast::GeneratorQualifier::source>(ref<TermRule>))> {};
  struct QualifierRule
      : rule<ref<GeneratorQualifierRule> | ref<GuardQualifierRule>> {};

  struct CompRule
      : rule<create<ast::ListComp>(
            lit<"["> + assign<&ast::ListComp::head>(ref<TermRule>) + Bar +
            option(some(append<&ast::ListComp::qualifiers>(ref<QualifierRule>),
                        Semi)) +
            lit<"]">)> {};

  struct ListRule : rule<ref<ListFormRule> | ref<UptoRule> | ref<CompRule>> {
  };

  struct FloatLitRule
      : rule<create<ast::FloatLit>(
            assign<&ast::FloatLit::value>(ref<FLOAT>))> {};
  struct IntLitRule
      : rule<create<ast::IntLit>(assign<&ast::IntLit::value>(ref<INTEGER>))> {
  };
  struct StringLitRule
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::value>(ref<STRING>))> {};

  struct FLiteralRule
      : rule<ref<FloatLitRule> | ref<IntLitRule> | ref<StringLitRule>> {};

  struct VarRefRule
      : rule<create<ast::VarRef>(assign<&ast::VarRef::name>(ref<ID>))> {};

  struct SectionRule
      : rule<create<ast::Section>(
                 assign<&ast::Section::op>(ref<OpRule>) +
                 option(assign<&ast::Section::rightOperand>(ref<Term1Rule>)))  //
             | create<ast::Section>(
                   assign<&ast::Section::leftOperand>(ref<Term1Rule>) +
                   assign<&ast::Section::op>(ref<OpRule>))> {};

  struct PrimaryNameRule
      : rule<ref<VarRefRule> |
             (lit<"("> + (ref<TermRule> | ref<SectionRule>) + lit<")">)> {};

  struct PrimaryRule
      : rule<ref<PrimaryNameRule> | ref<FLiteralRule> |
             (lit<"("> + create<ast::TupleTerm>(
                              append<&ast::TupleTerm::items>(ref<TermRule>) +
                              lit<","> +
                              some(append<&ast::TupleTerm::items>(ref<TermRule>),
                                   lit<",">)) +
              lit<")">) |
             ref<ListRule>> {};

  struct Term2Rule
      : rule<create<ast::Apply>(
                 assign<&ast::Apply::fn>(ref<PrimaryRule>) +
                 some(append<&ast::Apply::args>(ref<PrimaryRule>)))  //
             | ref<PrimaryRule>> {};

  struct Term1Rule
      : rule<create<ast::PrefixTerm>(
                 assign<&ast::PrefixTerm::op>(ref<OpRule>) +
                 assign<&ast::PrefixTerm::operand>(ref<Term1Rule>))  //
             | ref<Term2Rule>> {};

  struct TermRule
      : rule<create<ast::BinaryTerm>(
                 assign<&ast::BinaryTerm::left>(ref<Term1Rule>) +
                 assign<&ast::BinaryTerm::op>(ref<OpRule>) +
                 assign<&ast::BinaryTerm::right>(ref<TermRule>))  //
             | ref<Term1Rule>> {};

  // ---- patterns -----------------------------------------------------------

  struct PatIntRule
      : rule<create<ast::PatInt>(assign<&ast::PatInt::value>(ref<INTEGER>))> {
  };
  struct PatStringRule
      : rule<create<ast::PatString>(
            assign<&ast::PatString::value>(ref<STRING>))> {};
  struct PatLiteralRule : rule<ref<PatIntRule> | ref<PatStringRule>> {};

  struct PatVarRule
      : rule<create<ast::PatVar>(assign<&ast::PatVar::name>(ref<ID>))> {};

  struct PatSectionRule
      : rule<create<ast::PatSection>(
                 assign<&ast::PatSection::op>(ref<OpRule>) +
                 option(assign<&ast::PatSection::rightOperand>(ref<Pat1Rule>)))  //
             | create<ast::PatSection>(
                   assign<&ast::PatSection::leftOperand>(ref<Pat1Rule>) +
                   assign<&ast::PatSection::op>(ref<OpRule>))> {};

  struct PatPrimaryNameRule
      : rule<ref<PatVarRule> |
             (lit<"("> + (ref<PatRule> | ref<PatSectionRule>) + lit<")">)> {};

  struct PatPrimaryRule
      : rule<ref<PatPrimaryNameRule> | ref<PatLiteralRule> |
             (lit<"("> +
              create<ast::PatTuple>(
                  append<&ast::PatTuple::items>(ref<PatRule>) + lit<","> +
                  some(append<&ast::PatTuple::items>(ref<PatRule>), lit<",">)) +
              lit<")">) |
             (lit<"["> +
              create<ast::PatList>(
                  option(some(append<&ast::PatList::items>(ref<PatRule>),
                              lit<",">))) +
              lit<"]">)> {};

  struct Pat2Rule
      : rule<create<ast::PatApply>(
                 assign<&ast::PatApply::ctor>(ref<PatPrimaryRule>) +
                 some(append<&ast::PatApply::args>(ref<PatPrimaryRule>)))  //
             | ref<PatPrimaryRule>> {};

  struct Pat1Rule
      : rule<create<ast::PatPrefix>(
                 assign<&ast::PatPrefix::op>(ref<PatPrefixOpRule>) +
                 assign<&ast::PatPrefix::operand>(ref<Pat1Rule>))  //
             | ref<Pat2Rule>> {};

  struct PatRule
      : rule<create<ast::PatBinary>(
                 assign<&ast::PatBinary::left>(ref<Pat1Rule>) +
                 assign<&ast::PatBinary::op>(ref<OpRule>) +
                 assign<&ast::PatBinary::right>(ref<PatRule>))  //
             | ref<Pat1Rule>> {};

  // ---- types ---------------------------------------------------------------

  struct TypeNameRule
      : rule<create<ast::TypeName>(assign<&ast::TypeName::name>(ref<ID>))> {
  };

  struct TypeListRule
      : rule<lit<"["> +
             create<ast::TypeList>(
                 assign<&ast::TypeList::element>(ref<TypeRule>)) +
             lit<"]">> {};

  struct TySectionRule
      : rule<create<ast::TypeSection>(
                 assign<&ast::TypeSection::op>(ref<OpRule>) +
                 option(assign<&ast::TypeSection::rightOperand>(
                     ref<TyTerm1Rule>)))  //
             | create<ast::TypeSection>(
                   assign<&ast::TypeSection::leftOperand>(ref<TyTerm1Rule>) +
                   assign<&ast::TypeSection::op>(ref<OpRule>))> {};

  struct TyPrimaryNameRule
      : rule<ref<TypeNameRule> |
             (lit<"("> + (ref<TypeRule> | ref<TySectionRule>) + lit<")">)> {};

  struct TyPrimaryRule
      : rule<ref<TyPrimaryNameRule> |
             (lit<"("> +
              create<ast::TypeTuple>(
                  append<&ast::TypeTuple::items>(ref<TypeRule>) + lit<","> +
                  some(append<&ast::TypeTuple::items>(ref<TypeRule>),
                       lit<",">)) +
              lit<")">) |
             ref<TypeListRule>> {};

  struct TyTerm2Rule
      : rule<create<ast::TypeApply>(
                 assign<&ast::TypeApply::head>(ref<TyPrimaryRule>) +
                 some(append<&ast::TypeApply::args>(ref<TyPrimaryRule>)))  //
             | ref<TyPrimaryRule>> {};

  struct TyTerm1Rule
      : rule<create<ast::TypePrefix>(
                 assign<&ast::TypePrefix::op>(ref<OpRule>) +
                 assign<&ast::TypePrefix::operand>(ref<TyTerm1Rule>))  //
             | ref<TyTerm2Rule>> {};

  struct TypeRule
      : rule<create<ast::TypeBinary>(
                 assign<&ast::TypeBinary::left>(ref<TyTerm1Rule>) +
                 assign<&ast::TypeBinary::op>(ref<OpRule>) +
                 assign<&ast::TypeBinary::right>(ref<TypeRule>))  //
             | ref<TyTerm1Rule>> {};

  // `tylhs` reuses the type node family: `tyvar infix tyvar` and
  // `prefix tyvar` are a TypeBinary/TypePrefix of two TypeNames, and
  // `tylhs1` is a TyLhs1.
  struct TyLhsSectionRule
      : rule<create<ast::TypeSection>(assign<&ast::TypeSection::op>(ref<OpRule>))  //
             | create<ast::TypeSection>(
                   assign<&ast::TypeSection::op>(ref<OpRule>) +
                   assign<&ast::TypeSection::rightOperand>(ref<TypeNameRule>))  //
             | create<ast::TypeSection>(
                   assign<&ast::TypeSection::leftOperand>(ref<TypeNameRule>) +
                   assign<&ast::TypeSection::op>(ref<OpRule>))> {};

  struct TyLhsPrimaryRule
      : rule<create<ast::TyLhsPrimary>(
            assign<&ast::TyLhsPrimary::tyname>(ref<ID>) + lit<"("> +
            assign<&ast::TyLhsPrimary::inner>(ref<TyLhsRule> |
                                               ref<TyLhsSectionRule>) +
            lit<")">)> {};

  struct TyLhs1Rule
      : rule<create<ast::TyLhs1>(
            assign<&ast::TyLhs1::primary>(ref<TyLhsPrimaryRule>) +
            many(append<&ast::TyLhs1::tyvars>(ref<ID>)))> {};

  struct TyLhsRule
      : rule<create<ast::TypeBinary>(
                 assign<&ast::TypeBinary::left>(ref<TypeNameRule>) +
                 assign<&ast::TypeBinary::op>(ref<OpRule>) +
                 assign<&ast::TypeBinary::right>(ref<TypeNameRule>))  //
             | create<ast::TypePrefix>(
                   assign<&ast::TypePrefix::op>(ref<OpRule>) +
                   assign<&ast::TypePrefix::operand>(ref<TypeNameRule>))  //
             | ref<TyLhs1Rule>> {};

  // ---- declarations ---------------------------------------------------------

  struct SynDeclRule
      : rule<create<ast::SynDecl>(assign<&ast::SynDecl::lhs>(ref<TyLhsRule>) +
                                  Eq +
                                  assign<&ast::SynDecl::rhs>(ref<TypeRule>))> {
  };

  struct ConApplyRule
      : rule<create<ast::ConApply>(
            assign<&ast::ConApply::con>(ref<ID>) +
            many(append<&ast::ConApply::args>(ref<TyPrimaryRule>)))> {};
  struct ConBinaryRule
      : rule<create<ast::ConBinary>(
            assign<&ast::ConBinary::left>(ref<TyPrimaryRule>) +
            assign<&ast::ConBinary::op>(ref<OpRule>) +
            assign<&ast::ConBinary::right>(ref<TyPrimaryRule>))> {};
  struct ConPrefixRule
      : rule<create<ast::ConPrefix>(
            assign<&ast::ConPrefix::op>(ref<OpRule>) +
            assign<&ast::ConPrefix::operand>(ref<TyPrimaryRule>))> {};
  struct ConstructRule
      : rule<ref<ConApplyRule> | ref<ConBinaryRule> | ref<ConPrefixRule>> {};

  struct ConDeclRule
      : rule<create<ast::ConDecl>(
            assign<&ast::ConDecl::lhs>(ref<TyLhsRule>) + ConEq +
            some(append<&ast::ConDecl::constructs>(ref<ConstructRule>),
                 Bar))> {};

  struct NameRule
      : rule<create<ast::Name>(
            assign<&ast::Name::var>(ref<ID>) + lit<"("> +
            ((assign<&ast::Name::param>(ref<ID>) +
              assign<&ast::Name::op>(ref<OpRule>))  //
             | assign<&ast::Name::op>(ref<OpRule>)) +
            lit<")">)> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            append<&ast::TypeDecl::names>(ref<NameRule>) +
            many(lit<","> + append<&ast::TypeDecl::names>(ref<NameRule>)) +
            DColon + assign<&ast::TypeDecl::type>(ref<TypeRule>))> {};

  struct AssocRule
      : rule<kw<"%left"> | kw<"%right"> | kw<"%non"> | kw<"%leftcon"> |
             kw<"%rightcon"> | kw<"%noncon">> {};

  static constexpr auto OpKind =
      (assign<&ast::OpDecl::kind>(ref<AssocRule>) +
       assign<&ast::OpDecl::precedence>(ref<DIGIT>))  //
      | assign<&ast::OpDecl::kind>(kw<"%prefix">)      //
      | assign<&ast::OpDecl::kind>(kw<"%prefixcon">);

  struct OpDeclRule
      : rule<create<ast::OpDecl>(
            OpKind + some(append<&ast::OpDecl::ops>(ref<OpRule>)))> {};

  // ---- equations -------------------------------------------------------------

  struct IfPartRule
      : rule<create<ast::GuardedResult>(
            assign<&ast::GuardedResult::result>(ref<TermRule>) + lit<","> +
            kw<"if"> +
            assign<&ast::GuardedResult::condition>(ref<TermRule>))> {};

  struct ConditionalRule
      : rule<create<ast::Rhs>(
            append<&ast::Rhs::guards>(ref<IfPartRule>) +
            many(Assign + append<&ast::Rhs::guards>(ref<IfPartRule>)) +
            option(Assign + assign<&ast::Rhs::otherwise>(ref<TermRule>) +
                   lit<","> + kw<"otherwise">))> {};

  // A `where` block has no end marker: the reference's greedy `def_+` gives
  // it every declaration that follows, so a block runs to the end of the one
  // that encloses it, and a declaration that opens one is the last of its
  // own. Read any other way, each enclosing block is a reading of its own to
  // the end of input.
  template <auto Into>
  static constexpr auto Decls = many(append<Into>(ref<DeclRule>)) +
                                append<Into>(ref<LastDeclRule>);

  static constexpr auto RhsBody =
      create<ast::Rhs>(assign<&ast::Rhs::simple>(ref<TermRule>)) |
      ref<ConditionalRule>;

  struct RhsRule : rule<RhsBody> {};
  // The block writes into the node the choice above already created (see
  // the "helper that targets the enclosing node" pitfall).
  struct OpenRhsRule
      : rule<RhsBody + kw<"where"> + Decls<&ast::Rhs::whereDecls>> {};

  template <class Rhs>
  struct ClauseRule
      : rule<create<ast::DefClause>(assign<&ast::DefClause::pat>(ref<PatRule>) +
                                    Assign +
                                    assign<&ast::DefClause::rhs>(ref<Rhs>))> {};

  // `%else` is mandatory here: an omitted marker makes a continuation
  // clause indistinguishable from a fresh top-level Def by any amount of
  // lookahead (both read `pat = rhs`).
  template <class Rhs>
  struct DefClauseRule
      : rule<flag<&ast::DefClause::hasElse>(kw<"%else">) +
             create<ast::DefClause>(assign<&ast::DefClause::pat>(ref<PatRule>) +
                                     Assign +
                                     assign<&ast::DefClause::rhs>(ref<Rhs>))> {
  };

  static constexpr auto Clauses =
      append<&ast::Def::clauses>(ref<ClauseRule<RhsRule>>) +
      many(append<&ast::Def::clauses>(ref<DefClauseRule<RhsRule>>));

  struct DefRule : rule<create<ast::Def>(Clauses)> {};
  // Its last clause opens a block.
  struct OpenDefRule
      : rule<create<ast::Def>(
            append<&ast::Def::clauses>(ref<ClauseRule<OpenRhsRule>>) |
            (Clauses +
             append<&ast::Def::clauses>(ref<DefClauseRule<OpenRhsRule>>)))> {};

  struct DeclRule
      : rule<ref<SynDeclRule> | ref<ConDeclRule> | ref<TypeDeclRule> |
             ref<OpDeclRule> | ref<DefRule>> {};
  struct LastDeclRule : rule<ref<DeclRule> | ref<OpenDefRule>> {};

  struct ProgramRule
      : rule<create<ast::Program>(Decls<&ast::Program::decls>)> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace orwell
