// The orwell language: its grammar.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "orwell/core/ast.hpp"

namespace orwell {

// Orwell6.00 pp.30–31: decimal (not octal) escapes, and any other
// backslash character denotes itself.
inline constexpr auto unquote = [](std::string_view text,
                                   std::pmr::memory_resource* mem) {
  const auto body = text.substr(1, text.size() - 2);
  if (body.find('\\') == body.npos) return body;
  const auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') { room[n++] = body[i]; continue; }
    const char c = body[++i];
    if (c >= '0' && c <= '9') {
      unsigned value = static_cast<unsigned>(c - '0');
      for (unsigned k = 1; k < 3 && i + 1 < body.size() &&
                           body[i + 1] >= '0' && body[i + 1] <= '9'; ++k)
        value = value * 10 + static_cast<unsigned>(body[++i] - '0');
      room[n++] = static_cast<char>(value);
    } else {
      switch (c) {
        case 'n': room[n++] = '\n'; break;
        case 't': room[n++] = '\t'; break;
        case 'f': room[n++] = '\f'; break;
        case 'r': room[n++] = '\r'; break;
        case 'b': room[n++] = '\b'; break;
        default: room[n++] = c; break;
      }
    }
  }
  return std::string_view(room.data(), n);
};

struct Orwell : cursiv::GrammarSpec {
  // Operator alphabet and native identifier tail are distinct.
  static constexpr auto Sym = cr<R"(+*=<>~&\\/^:#!.;|?@`$%{}-)">;
  static constexpr auto IdTail = cr<"a-zA-Z0-9'_">;
  static constexpr auto word_boundary = IdTail;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto Reserved = kw<"if"> | kw<"otherwise"> | kw<"where">;
  struct ID : terminal<!Reserved + cr<"a-zA-Z"> + many(IdTail)> {};

  struct INTEGER : terminal<as<std::int64_t>(some(d))> {};
  // The manual typesets a minus; source uses the ordinary ASCII hyphen.
  struct FLOAT
      : terminal<as<double>(some(d) + lit<"."> + some(d) +
                            option(lit<"e"> + option(lit<"-">) + some(d)))> {};
  static constexpr auto Escape = lit<"\\"> +
      (repeat<1, 3>(d) | cr<"^0-9">);
  struct STRING
      : terminal<as<unquote>(lit<"\""> + many(Escape | cr<R"(^"\\)">) + lit<"\"">)> {};
  struct CHARACTER
      : terminal<as<unquote>(lit<"'"> + (Escape | cr<R"(^'\\)">) + lit<"'">)> {};

  // One byte, so `assoc DIGIT` reads a lone precedence digit; cursiv reads
  // it here because only DIGIT is legal in this position.
  struct DIGIT : terminal<as<std::int8_t>(d)> {};

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
  struct Op : rule<!(Eq | ConEq | DColon | Assign | Semi | Bar | LArrow |
                         DotDot) +
                       ref<OP>> {};

  // A bare `%` cannot open a prefix PATTERN: `%else` also precedes a full
  // `pat = rhs`, so nothing downstream ever tells the two apart by
  // lookahead -- `%else f n = n` parses to completion either way. Every
  // other prefix operator, including a longer one starting with `%`, is
  // unaffected. Its own rule, not a bare expression: a guard spliced ahead
  // of `ref<OP>` in an `assign<>` produces no value on its own.
  // Equality is an expression operator as well as an equation delimiter.
  struct TermOp : rule<Assign | ref<Op>> {};
  struct PatPrefixOp : rule<!lit<"%", !Sym> + ref<Op>> {};

  struct Term;
  struct Term1;
  struct Term2;
  struct Primary;
  struct PrimaryName;
  struct Section;
  struct List;

  struct Pat;
  struct Pat1;
  struct Pat2;
  struct PatPrimary;
  struct PatPrimaryName;
  struct PatSection;

  struct Type;
  struct TyTerm1;
  struct TyTerm2;
  struct TyPrimary;
  struct TyPrimaryName;
  struct TySection;
  struct TyLhs;
  struct TyLhs1Rule;
  struct TyLhsPrimary;
  struct TyLhsSection;

  struct DefRule;
  struct Decl;
  struct LastDecl;

  // ---- terms -------------------------------------------------------------

  struct ListForm
      : rule<create<ast::ListTerm>(
            lit<"["> +
            many(append<&ast::ListTerm::items>(ref<Term>),
                        lit<",">) +
            lit<"]">)> {};

  struct Upto
      : rule<create<ast::ListRange>(
            lit<"["> + assign<&ast::ListRange::from>(ref<Term>) +
            option(lit<","> + assign<&ast::ListRange::step>(ref<Term>)) +
            DotDot + option(assign<&ast::ListRange::to>(ref<Term>)) +
            lit<"]">)> {};

  struct GuardQualifier
      : rule<assign<&ast::GuardQualifier::condition>(ref<Term>)> {};
  struct GeneratorQualifier
      : rule<create<ast::GeneratorQualifier>(
            assign<&ast::GeneratorQualifier::pat>(ref<Pat>) + LArrow +
            assign<&ast::GeneratorQualifier::source>(ref<Term>))> {};
  struct Qualifier
      : rule<ref<GeneratorQualifier> | ref<GuardQualifier>> {};

  struct Comp
      : rule<create<ast::ListComp>(
            lit<"["> + assign<&ast::ListComp::head>(ref<Term>) + Bar +
            many(append<&ast::ListComp::qualifiers>(ref<Qualifier>),
                        Semi) +
            lit<"]">)> {};

  struct List : rule<ref<ListForm> | ref<Upto> | ref<Comp>> {
  };

  struct FloatLit
      : rule<assign<&ast::FloatLit::value>(ref<FLOAT>)> {};
  struct IntLit
      : rule<assign<&ast::IntLit::value>(ref<INTEGER>)> {
  };
  struct StringLit
      : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};

  struct CharacterLit
      : rule<assign<&ast::CharacterLit::value>(ref<CHARACTER>)> {};
  struct FLiteral
      : rule<ref<FloatLit> | ref<IntLit> | ref<StringLit> | ref<CharacterLit>> {};

  struct VarRef
      : rule<assign<&ast::VarRef::name>(ref<ID>)> {};

  struct Section
      : rule<create<ast::Section>(
                 assign<&ast::Section::op>(ref<Op>) +
                 option(assign<&ast::Section::rightOperand>(ref<Term1>)))  //
             | create<ast::Section>(
                   assign<&ast::Section::leftOperand>(ref<Term1>) +
                   assign<&ast::Section::op>(ref<Op>))> {};

  struct PrimaryName
      : rule<ref<VarRef> |
             (lit<"("> + (ref<Term> | ref<Section>) + lit<")">)> {};

  struct Primary
      : rule<ref<PrimaryName> | ref<FLiteral> |
             (lit<"("> + create<ast::TupleTerm>(
                              append<&ast::TupleTerm::items>(ref<Term>) +
                              lit<","> +
                              some(append<&ast::TupleTerm::items>(ref<Term>),
                                   lit<",">)) +
              lit<")">) |
             ref<List>> {};

  struct Term2
      : rule<create<ast::Apply>(
                 assign<&ast::Apply::fn>(ref<Primary>) +
                 some(!gap::line + append<&ast::Apply::args>(ref<Primary>)))  //
             | ref<Primary>> {};

  struct Term1
      : rule<create<ast::PrefixTerm>(
                 assign<&ast::PrefixTerm::op>(ref<Op>) +
                 assign<&ast::PrefixTerm::operand>(ref<Term1>))  //
             | ref<Term2>> {};

  struct Term
      : rule<create<ast::BinaryTerm>(
                 assign<&ast::BinaryTerm::left>(ref<Term1>) +
                 !gap::line + assign<&ast::BinaryTerm::op>(ref<TermOp>) +
                 assign<&ast::BinaryTerm::right>(ref<Term>))  //
             | ref<Term1>> {};

  // ---- patterns -----------------------------------------------------------

  struct PatInt
      : rule<assign<&ast::PatInt::value>(ref<INTEGER>)> {
  };
  struct PatString
      : rule<assign<&ast::PatString::value>(ref<STRING>)> {};
  struct PatCharacter
      : rule<assign<&ast::PatCharacter::value>(ref<CHARACTER>)> {};
  struct PatLiteral : rule<ref<PatInt> | ref<PatString> | ref<PatCharacter>> {};

  struct PatVar
      : rule<assign<&ast::PatVar::name>(ref<ID>)> {};

  struct PatSection
      : rule<create<ast::PatSection>(
                 assign<&ast::PatSection::op>(ref<Op>) +
                 option(assign<&ast::PatSection::rightOperand>(ref<Pat1>)))  //
             | create<ast::PatSection>(
                   assign<&ast::PatSection::leftOperand>(ref<Pat1>) +
                   assign<&ast::PatSection::op>(ref<Op>))> {};

  struct PatPrimaryName
      : rule<ref<PatVar> |
             (lit<"("> + (ref<Pat> | ref<PatSection>) + lit<")">)> {};

  struct PatPrimary
      : rule<ref<PatPrimaryName> | ref<PatLiteral> |
             (lit<"("> +
              create<ast::PatTuple>(
                  append<&ast::PatTuple::items>(ref<Pat>) + lit<","> +
                  some(append<&ast::PatTuple::items>(ref<Pat>), lit<",">)) +
              lit<")">) |
             (lit<"["> +
              create<ast::PatList>(
                  many(append<&ast::PatList::items>(ref<Pat>),
                              lit<",">)) +
              lit<"]">)> {};

  struct Pat2
      : rule<create<ast::PatApply>(
                 assign<&ast::PatApply::ctor>(ref<PatPrimary>) +
                 some(!gap::line + append<&ast::PatApply::args>(ref<PatPrimary>)))  //
             | ref<PatPrimary>> {};

  struct Pat1
      : rule<create<ast::PatPrefix>(
                 assign<&ast::PatPrefix::op>(ref<PatPrefixOp>) +
                 assign<&ast::PatPrefix::operand>(ref<Pat1>))  //
             | ref<Pat2>> {};

  struct Pat
      : rule<create<ast::PatBinary>(
                 assign<&ast::PatBinary::left>(ref<Pat1>) +
                 assign<&ast::PatBinary::op>(ref<Op>) +
                 assign<&ast::PatBinary::right>(ref<Pat>))  //
             | ref<Pat1>> {};

  // ---- types ---------------------------------------------------------------

  struct TypeName
      : rule<assign<&ast::TypeName::name>(ref<ID>)> {
  };

  struct TypeList
      : rule<lit<"["> +
             create<ast::TypeList>(
                 assign<&ast::TypeList::element>(ref<Type>)) +
             lit<"]">> {};

  struct TySection
      : rule<create<ast::TypeSection>(
                 assign<&ast::TypeSection::op>(ref<Op>) +
                 option(assign<&ast::TypeSection::rightOperand>(
                     ref<TyTerm1>)))  //
             | create<ast::TypeSection>(
                   assign<&ast::TypeSection::leftOperand>(ref<TyTerm1>) +
                   assign<&ast::TypeSection::op>(ref<Op>))> {};

  struct TyPrimaryName
      : rule<ref<TypeName> |
             (lit<"("> + (ref<Type> | ref<TySection>) + lit<")">)> {};

  struct TyPrimary
      : rule<ref<TyPrimaryName> |
             (lit<"("> +
              create<ast::TypeTuple>(
                  append<&ast::TypeTuple::items>(ref<Type>) + lit<","> +
                  some(append<&ast::TypeTuple::items>(ref<Type>),
                       lit<",">)) +
              lit<")">) |
             ref<TypeList>> {};

  struct TyTerm2
      : rule<create<ast::TypeApply>(
                 assign<&ast::TypeApply::head>(ref<TyPrimary>) +
                 some(!gap::line + append<&ast::TypeApply::args>(ref<TyPrimary>)))  //
             | ref<TyPrimary>> {};

  struct TyTerm1
      : rule<create<ast::TypePrefix>(
                 assign<&ast::TypePrefix::op>(ref<Op>) +
                 assign<&ast::TypePrefix::operand>(ref<TyTerm1>))  //
             | ref<TyTerm2>> {};

  struct Type
      : rule<create<ast::TypeBinary>(
                 assign<&ast::TypeBinary::left>(ref<TyTerm1>) +
                 assign<&ast::TypeBinary::op>(ref<Op>) +
                 assign<&ast::TypeBinary::right>(ref<Type>))  //
             | ref<TyTerm1>> {};

  // `tylhs` reuses the type node family: `tyvar infix tyvar` and
  // `prefix tyvar` are a TypeBinary/TypePrefix of two TypeNames, and
  // `tylhs1` is a TyLhs1.
  struct TyLhsSection
      : rule<create<ast::TypeSection>(assign<&ast::TypeSection::op>(ref<Op>))  //
             | create<ast::TypeSection>(
                   assign<&ast::TypeSection::op>(ref<Op>) +
                   assign<&ast::TypeSection::rightOperand>(ref<TypeName>))  //
             | create<ast::TypeSection>(
                   assign<&ast::TypeSection::leftOperand>(ref<TypeName>) +
                   assign<&ast::TypeSection::op>(ref<Op>))> {};

  struct TyLhsPrimary
      : rule<create<ast::TyLhsPrimary>(
            assign<&ast::TyLhsPrimary::tyname>(ref<ID>) |
            (lit<"("> + assign<&ast::TyLhsPrimary::inner>(
                            ref<TyLhs> | ref<TyLhsSection>) + lit<")">))> {};

  struct TyLhs1Rule
      : rule<create<ast::TyLhs1>(
            assign<&ast::TyLhs1::primary>(ref<TyLhsPrimary>) +
            many(!gap::line + append<&ast::TyLhs1::tyvars>(ref<ID>)))> {};

  struct TyLhs
      : rule<create<ast::TypeBinary>(
                 assign<&ast::TypeBinary::left>(ref<TypeName>) +
                 assign<&ast::TypeBinary::op>(ref<Op>) +
                 assign<&ast::TypeBinary::right>(ref<TypeName>))  //
             | create<ast::TypePrefix>(
                   assign<&ast::TypePrefix::op>(ref<Op>) +
                   assign<&ast::TypePrefix::operand>(ref<TypeName>))  //
             | ref<TyLhs1Rule>> {};

  // ---- declarations ---------------------------------------------------------

  struct SynDecl
      : rule<create<ast::SynDecl>(assign<&ast::SynDecl::lhs>(ref<TyLhs>) +
                                  Eq +
                                  assign<&ast::SynDecl::rhs>(ref<Type>))> {
  };

  struct ConApply
      : rule<create<ast::ConApply>(
            assign<&ast::ConApply::con>(ref<ID>) +
            many(!gap::line + append<&ast::ConApply::args>(ref<TyPrimary>)))> {};
  struct ConBinary
      : rule<create<ast::ConBinary>(
            assign<&ast::ConBinary::left>(ref<TyPrimary>) +
            assign<&ast::ConBinary::op>(ref<Op>) +
            assign<&ast::ConBinary::right>(ref<TyPrimary>))> {};
  struct ConPrefix
      : rule<create<ast::ConPrefix>(
            assign<&ast::ConPrefix::op>(ref<Op>) +
            assign<&ast::ConPrefix::operand>(ref<TyPrimary>))> {};
  struct Construct
      : rule<ref<ConApply> | ref<ConBinary> | ref<ConPrefix>> {};

  struct ConDecl
      : rule<create<ast::ConDecl>(
            assign<&ast::ConDecl::lhs>(ref<TyLhs>) + ConEq +
            some(append<&ast::ConDecl::constructs>(ref<Construct>),
                 Bar))> {};

  struct Name
      : rule<create<ast::Name>(
            assign<&ast::Name::var>(ref<ID>) |
            (lit<"("> + (assign<&ast::Name::param>(ref<ID>) |
                          assign<&ast::Name::op>(ref<TermOp>)) + lit<")">))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            append<&ast::TypeDecl::names>(ref<Name>) +
            many(lit<","> + append<&ast::TypeDecl::names>(ref<Name>)) +
            DColon + assign<&ast::TypeDecl::type>(ref<Type>))> {};

  struct Assoc
      : rule<kw<"%left"> | kw<"%right"> | kw<"%non"> | kw<"%leftcon"> |
             kw<"%rightcon"> | kw<"%noncon">> {};

  static constexpr auto OpKind =
      (assign<&ast::OpDecl::kind>(ref<Assoc>) +
       assign<&ast::OpDecl::precedence>(ref<DIGIT>))  //
      | assign<&ast::OpDecl::kind>(kw<"%prefix">)      //
      | assign<&ast::OpDecl::kind>(kw<"%prefixcon">);

  struct OpDecl
      : rule<create<ast::OpDecl>(
            OpKind + some(append<&ast::OpDecl::ops>(ref<Op>)))> {};

  // ---- equations -------------------------------------------------------------

  struct IfPart
      : rule<create<ast::GuardedResult>(
            assign<&ast::GuardedResult::result>(ref<Term>) + lit<","> +
            kw<"if"> +
            assign<&ast::GuardedResult::condition>(ref<Term>))> {};

  struct Conditional
      : rule<create<ast::Rhs>(
            append<&ast::Rhs::guards>(ref<IfPart>) +
            many(Assign + append<&ast::Rhs::guards>(ref<IfPart>)) +
            option(Assign + assign<&ast::Rhs::otherwise>(ref<Term>) +
                   lit<","> + kw<"otherwise">))> {};

  // A `where` block has no end marker: the reference's greedy `def_+` gives
  // it every declaration that follows, so a block runs to the end of the one
  // that encloses it, and a declaration that opens one is the last of its
  // own. Read any other way, each enclosing block is a reading of its own to
  // the end of input.
  template <auto Into>
  static constexpr auto Decls = many(append<Into>(ref<Decl>)) +
                                append<Into>(ref<LastDecl>);

  static constexpr auto RhsBody =
      create<ast::Rhs>(assign<&ast::Rhs::simple>(ref<Term>)) |
      ref<Conditional>;

  struct RhsRule : rule<RhsBody> {};
  // The block writes into the node the choice above already created (see
  // the "helper that targets the enclosing node" pitfall).
  struct OpenRhs
      : rule<RhsBody + kw<"where"> + Decls<&ast::Rhs::whereDecls>> {};

  template <class Rhs>
  struct Clause
      : rule<create<ast::DefClause>(assign<&ast::DefClause::pat>(ref<Pat>) +
                                    Assign +
                                    assign<&ast::DefClause::rhs>(ref<Rhs>))> {};

  // `%else` is mandatory here: an omitted marker makes a continuation
  // clause indistinguishable from a fresh top-level Def by any amount of
  // lookahead (both read `pat = rhs`).
  template <class Rhs>
  struct DefClause
      : rule<flag<&ast::DefClause::hasElse>(kw<"%else">) +
             create<ast::DefClause>(assign<&ast::DefClause::pat>(ref<Pat>) +
                                     Assign +
                                     assign<&ast::DefClause::rhs>(ref<Rhs>))> {
  };

  static constexpr auto Clauses =
      append<&ast::Def::clauses>(ref<Clause<RhsRule>>) +
      many(append<&ast::Def::clauses>(ref<DefClause<RhsRule>>));

  struct DefRule : rule<create<ast::Def>(Clauses)> {};
  // Its last clause opens a block.
  struct OpenDef
      : rule<create<ast::Def>(
            append<&ast::Def::clauses>(ref<Clause<OpenRhs>>) |
            (Clauses +
             append<&ast::Def::clauses>(ref<DefClause<OpenRhs>>)))> {};

  struct Decl
      : rule<ref<SynDecl> | ref<ConDecl> | ref<TypeDecl> |
             ref<OpDecl> | ref<DefRule>> {};
  struct LastDecl : rule<ref<Decl> | ref<OpenDef>> {};

  struct Program
      : rule<create<ast::Program>(Decls<&ast::Program::decls>)> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace orwell
