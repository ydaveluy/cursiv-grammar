// The Algol 60 language: its grammar. LR with the default engine; the token
// graph is per-state, so a keyword's spelling never competes with a name
// outside the states that read it.
#pragma once

#include <charconv>
#include <cmath>
#include <cstddef>
#include <string_view>

#include "algol60/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace algol60 {

// A raised-ten exponent signed at its own mark ("2.5+10+6") cannot be
// mistaken for a longer mantissa, since a mantissa has no sign; unsigned
// ("2.5106") reads one decimal, the mark undistinguished from more digits --
// the report's own ambiguity once "10" loses its raised typography.
inline constexpr auto unsigned_number = [](std::string_view t) {
  for (std::size_t i = 0; i + 2 < t.size(); ++i) {
    if (t[i] != '1' || t[i + 1] != '0') continue;
    const bool unicodeMinus = t.substr(i + 2).starts_with("–");
    if (t[i + 2] != '+' && t[i + 2] != '-' && !unicodeMinus) continue;
    const std::size_t signSize = unicodeMinus ? 3 : 1;
    double mantissa = 0, exp_digits = 0;
    const auto [p1, e1] = std::from_chars(t.data(), t.data() + i, mantissa);
    const auto [p2, e2] = std::from_chars(t.data() + i + 2 + signSize,
                                          t.data() + t.size(), exp_digits);
    if (e1 != std::errc{} || p1 != t.data() + i) continue;
    if (e2 != std::errc{} || p2 != t.data() + t.size()) continue;
    const double sign = (t[i + 2] == '-' || unicodeMinus) ? -1.0 : 1.0;
    return mantissa * std::pow(10.0, sign * exp_digits);
  }
  double v = 0;
  std::from_chars(t.data(), t.data() + t.size(), v);
  return v;
};
using UnsignedNumberValue = cursiv::text_conv<unsigned_number>;

struct Algol60 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // A `comment` clause reads to the next `;`; the classic close-tag after
  // `end` (`end name;`) is handled at the block rule instead, since its
  // closer must not be swallowed.
  struct COMMENT : terminal<through(ikw<"comment">, lit<";">)> {};

  static constexpr auto Digits = some(cr<"0-9">);
  static constexpr auto ExpSign = lit<"+"> | lit<"-"> | lit<"–">;
  // Unsigned_number: an integer, a fraction, or both, with an optional
  // exponent introduced by the digits `10` (the report's raised "ten").
  struct UnsignedNumber
      : terminal<(Digits + option(lit<"."> + Digits) | lit<"."> + Digits) +
                     option(lit<"10"> + option(ExpSign) + Digits),
                 UnsignedNumberValue> {};

  // (Letter|Digit)+, narrowed to require one letter somewhere: a bare digit
  // run is a number, not a name. A digit-then-letter spelling like `1e` (an
  // identifier this alphabet allows, seen where a document leans on a
  // foreign exponent mark) still matches.
  struct Identifier
      : terminal<(Digits + cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)) |
                     (cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">))> {};
  struct UnsignedInteger : terminal<some(cr<"0-9">)> {};

  // Two non-standard string forms the corpus uses, plus the report's own
  // nesting corner brackets.
  struct DQ_STRING : terminal<through(lit<"\"">, lit<"\"">)> {};
  struct BQ_STRING : terminal<through(lit<"`">, lit<"'">)> {};
  struct CORNER_STRING
      : terminal<lit<"⌜"> +
                 many(ref<CORNER_STRING> | (!lit<"⌜"> + !lit<"⌝"> + any)) +
                 lit<"⌝">> {};

  static constexpr auto StringLit = create<ast::StringLiteral>(
      assign<&ast::StringLiteral::text>(ref<DQ_STRING> | ref<BQ_STRING> |
                                        ref<CORNER_STRING>));

  static constexpr auto TypeTok =
      ikw<"real"> | ikw<"integer"> | ikw<"boolean">;

  struct TrueKw : terminal<ikw<"true">, value<true>> {};
  struct FalseKw : terminal<ikw<"false">, value<false>> {};

  struct ExpOp : terminal<lit<"↑"> | lit<"^">, value<ast::BinOp::power>> {};
  struct MulOp : terminal<lit<"×"> | lit<"*">, value<ast::BinOp::mul>> {};
  struct DivOp : terminal<lit<"/"> | lit<"÷">, value<ast::BinOp::div>> {};
  struct AddOp : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct SubOp
      : terminal<lit<"-"> | lit<"–">, value<ast::BinOp::sub>> {};

  struct LtOp : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct LeOp : terminal<lit<"<="> | lit<"≤">, value<ast::BinOp::le>> {};
  struct EqOp : terminal<lit<"=">, value<ast::BinOp::eq>> {};
  struct NeOp : terminal<lit<"≠"> | lit<"!=">, value<ast::BinOp::ne>> {};
  struct GtOp : terminal<lit<">">, value<ast::BinOp::gt>> {};
  struct GeOp : terminal<lit<">="> | lit<"≥">, value<ast::BinOp::ge>> {};

  struct UPlusOp : terminal<lit<"+">, value<ast::UnOp::plus>> {};
  struct UMinusOp
      : terminal<lit<"-"> | lit<"–">, value<ast::UnOp::minus>> {};

  struct NotOp : terminal<lit<"¬"> | lit<"!">, value<ast::UnOp::lnot>> {};
  struct AndOp : terminal<lit<"⋀"> | lit<"&">, value<ast::BinOp::land>> {};
  struct OrOp : terminal<lit<"⋁"> | lit<"|">, value<ast::BinOp::lor>> {};
  struct ImpliesOp : terminal<lit<"⊃">, value<ast::BinOp::implies>> {};
  struct EquivOp : terminal<lit<"≡">, value<ast::BinOp::equiv>> {};

  // `go`, a gap of any width (blank or newline), then `to` -- the report's
  // own spelling of the keyword. Spelled by hand, not `ikw<> + ikw<>`: the
  // word boundary belongs at the very end only, or `goto` (no gap) would
  // refuse itself between the two halves.
  struct GotoKw
      : terminal<(lit<"g"> | lit<"G">) + (lit<"o"> | lit<"O">) +
                     many(cr<R"( \t\r\n)">) + (lit<"t"> | lit<"T">) +
                     (lit<"o", !w> | lit<"O", !w>)> {};

  // parameter_delimiter: a comma, or a closing/opening parenthesis pair
  // around a documentary name -- splitting one long list across several
  // `(...)` groups.
  static constexpr auto ParamSep =
      lit<","> | (lit<")"> + ref<Identifier> + lit<":"> + lit<"(">);

  struct ArithExpr;
  struct BoolExpr;
  struct DesigExpr;
  struct Statement;
  struct Block;

  struct PrimaryExpr
      : rule<create<ast::NumberLiteral>(
                 assign<&ast::NumberLiteral::value>(ref<UnsignedNumber>))  //
             | create<ast::IndexExpr>(
                   assign<&ast::IndexExpr::name>(ref<Identifier>) + lit<"["> +
                   some(append<&ast::IndexExpr::indices>(ref<ArithExpr>),
                        lit<",">) +
                   lit<"]">)  //
             | create<ast::CallExpr>(
                   assign<&ast::CallExpr::callee>(ref<Identifier>) +
                   lit<"("> +
                   some(append<&ast::CallExpr::args>(
                            StringLit | ref<ArithExpr> | ref<BoolExpr>),
                        ParamSep) +
                   lit<")">)  //
             | create<ast::NameRef>(
                   assign<&ast::NameRef::name>(ref<Identifier>))  //
             | (lit<"("> + ref<ArithExpr> + lit<")">)> {};

  struct Factor : infix<PrimaryExpr, &ast::BinaryExpr::left,
                        &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                        left(ref<ExpOp>)> {};

  // The operand of `+`/`-`: an optional sign wrapped around a Factor. A
  // sign is available at every `*`/`/`/`+`/`-` operand this way, which
  // over-accepts `2*-3` beside the report's own `2*(-3)` -- harmless.
  struct Term
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<UPlusOp> | ref<UMinusOp>) +
                 assign<&ast::UnaryExpr::operand>(ref<Factor>))  //
             | ref<Factor>> {};

  struct SimpleArith
      : infix<Term, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<MulOp> | ref<DivOp>) >> left(ref<AddOp> | ref<SubOp>))> {
  };

  struct ArithExpr
      : rule<create<ast::IfExpr>(
                 ikw<"if"> + assign<&ast::IfExpr::cond>(ref<BoolExpr>) +
                 ikw<"then"> +
                 assign<&ast::IfExpr::then_branch>(ref<SimpleArith>) +
                 ikw<"else"> +
                 assign<&ast::IfExpr::else_branch>(ref<ArithExpr>))  //
             | ref<SimpleArith>> {};

  struct RelationRule
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<SimpleArith>) +
                 assign<&ast::BinaryExpr::op>(ref<LtOp> | ref<LeOp> |
                                              ref<EqOp> | ref<NeOp> |
                                              ref<GtOp> | ref<GeOp>) +
                 assign<&ast::BinaryExpr::right>(ref<SimpleArith>))> {};

  struct BoolPrimaryExpr
      : rule<create<ast::LogicalLiteral>(
                 assign<&ast::LogicalLiteral::value>(ref<TrueKw> |
                                                     ref<FalseKw>))  //
             | ref<RelationRule>  //
             | create<ast::IndexExpr>(
                   assign<&ast::IndexExpr::name>(ref<Identifier>) + lit<"["> +
                   some(append<&ast::IndexExpr::indices>(ref<ArithExpr>),
                        lit<",">) +
                   lit<"]">)  //
             | create<ast::CallExpr>(
                   assign<&ast::CallExpr::callee>(ref<Identifier>) +
                   lit<"("> +
                   some(append<&ast::CallExpr::args>(
                            StringLit | ref<ArithExpr> | ref<BoolExpr>),
                        ParamSep) +
                   lit<")">)  //
             | create<ast::NameRef>(
                   assign<&ast::NameRef::name>(ref<Identifier>))  //
             | (lit<"("> + ref<BoolExpr> + lit<")">)> {};

  struct BoolSecondary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<NotOp>) +
                 assign<&ast::UnaryExpr::operand>(ref<BoolPrimaryExpr>))  //
             | ref<BoolPrimaryExpr>> {};

  struct SimpleBool
      : infix<BoolSecondary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<AndOp>) >> left(ref<OrOp>) >> left(ref<ImpliesOp>) >>
               left(ref<EquivOp>))> {};

  struct BoolExpr
      : rule<create<ast::IfExpr>(
                 ikw<"if"> + assign<&ast::IfExpr::cond>(ref<BoolExpr>) +
                 ikw<"then"> +
                 assign<&ast::IfExpr::then_branch>(ref<SimpleBool>) +
                 ikw<"else"> +
                 assign<&ast::IfExpr::else_branch>(ref<BoolExpr>))  //
             | ref<SimpleBool>> {};

  struct SimpleDesig
      : rule<create<ast::NameRef>(
                 assign<&ast::NameRef::name>(ref<Identifier> |
                                             ref<UnsignedInteger>))  //
             | create<ast::IndexExpr>(
                   assign<&ast::IndexExpr::name>(ref<Identifier>) + lit<"["> +
                   append<&ast::IndexExpr::indices>(ref<ArithExpr>) +
                   lit<"]">)  //
             | (lit<"("> + ref<DesigExpr> + lit<")">)> {};

  struct DesigExpr
      : rule<create<ast::IfExpr>(
                 ikw<"if"> + assign<&ast::IfExpr::cond>(ref<BoolExpr>) +
                 ikw<"then"> +
                 assign<&ast::IfExpr::then_branch>(ref<SimpleDesig>) +
                 ikw<"else"> +
                 assign<&ast::IfExpr::else_branch>(ref<DesigExpr>))  //
             | ref<SimpleDesig>> {};

  // A destination: a variable, possibly subscripted, or a bare name
  // (procedure_identifier and simple_variable share one spelling).
  static constexpr auto Destination =
      create<ast::IndexExpr>(assign<&ast::IndexExpr::name>(ref<Identifier>) +
                             lit<"["> +
                             some(append<&ast::IndexExpr::indices>(
                                      ref<ArithExpr>),
                                  lit<",">) +
                             lit<"]">)  //
      | create<ast::NameRef>(assign<&ast::NameRef::name>(ref<Identifier>));

  struct AssignStatement
      : rule<create<ast::AssignStmt>(
                 some(append<&ast::AssignStmt::targets>(Destination) +
                          lit<":=">) +
                 assign<&ast::AssignStmt::value>(ref<ArithExpr> |
                                                 ref<BoolExpr>))> {};

  struct GotoStatement
      : rule<create<ast::GotoStmt>(
                 ref<GotoKw> +
                 assign<&ast::GotoStmt::target>(ref<DesigExpr>))> {};

  struct CallStatement
      : rule<create<ast::CallStmt>(
                 assign<&ast::CallStmt::callee>(ref<Identifier>) +
                 option(lit<"("> +
                        some(append<&ast::CallStmt::args>(
                                 StringLit | ref<ArithExpr> | ref<BoolExpr>),
                             ParamSep) +
                        lit<")">))> {};

  // Three distinct node types so the heterogeneous `items` list can tell
  // them apart; a shared wrapper rule would blur them to one base.
  struct ForStepUntilItem
      : rule<create<ast::ForStepUntil>(
                 assign<&ast::ForStepUntil::from>(ref<ArithExpr>) +
                 ikw<"step"> +
                 assign<&ast::ForStepUntil::step>(ref<ArithExpr>) +
                 ikw<"until"> +
                 assign<&ast::ForStepUntil::to>(ref<ArithExpr>))> {};
  struct ForWhileItem
      : rule<create<ast::ForWhile>(
                 assign<&ast::ForWhile::value>(ref<ArithExpr>) +
                 ikw<"while"> +
                 assign<&ast::ForWhile::cond>(ref<BoolExpr>))> {};
  struct ForSimpleItem
      : rule<create<ast::ForSimple>(
                 assign<&ast::ForSimple::value>(ref<ArithExpr>))> {};

  struct ForStatement
      : rule<create<ast::ForStmt>(
                 ikw<"for"> + assign<&ast::ForStmt::var>(ref<Identifier>) +
                 lit<":="> +
                 some(append<&ast::ForStmt::items>(
                          ref<ForStepUntilItem> | ref<ForWhileItem> |
                          ref<ForSimpleItem>),
                      lit<",">) +
                 ikw<"do"> + assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct IfStatement
      : rule<create<ast::IfStmt>(
                 ikw<"if"> + assign<&ast::IfStmt::cond>(ref<BoolExpr>) +
                 ikw<"then"> +
                 assign<&ast::IfStmt::then_branch>(ref<Statement>) +
                 option(ikw<"else"> +
                        assign<&ast::IfStmt::else_branch>(ref<Statement>)))> {
  };

  // The `unlabelled_*` core statement kinds, unified: assignment, goto, a
  // dummy (empty) body handled by the optional slot at each call site, a
  // procedure call, if/for, and a nested block.
  struct Statement
      : rule<ref<IfStatement>       //
             | ref<ForStatement>    //
             | ref<AssignStatement> //
             | ref<GotoStatement>   //
             | ref<Block>           //
             | ref<CallStatement>> {};

  // A statement list entry: zero or more labels, then an optional
  // statement -- `out2: end` labels nothing at all.
  struct StatementEntry
      : rule<create<ast::StmtEntry>(
                 some(append<&ast::StmtEntry::labels>(
                          ref<Identifier> | ref<UnsignedInteger>) +
                      lit<":">) +
                 option(assign<&ast::StmtEntry::stmt>(ref<Statement>)))  //
             | create<ast::StmtEntry>(
                   assign<&ast::StmtEntry::stmt>(ref<Statement>))> {};

  static constexpr auto LocalOrOwn =
      option(flag<&ast::TypeDecl::own_>(ikw<"own">));

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
                 LocalOrOwn +
                 assign<&ast::TypeDecl::type_name>(TypeTok) +
                 some(append<&ast::TypeDecl::names>(ref<Identifier>),
                      lit<",">))> {};

  struct BoundPairRule
      : rule<create<ast::BoundPair>(
                 assign<&ast::BoundPair::lower>(ref<ArithExpr>) + lit<":"> +
                 assign<&ast::BoundPair::upper>(ref<ArithExpr>))> {};

  struct ArraySegmentRule
      : rule<create<ast::ArraySegment>(
                 some(append<&ast::ArraySegment::names>(ref<Identifier>),
                      lit<",">) +
                 lit<"["> +
                 some(append<&ast::ArraySegment::bounds>(
                          ref<BoundPairRule>),
                      lit<",">) +
                 lit<"]">)> {};

  struct ArrayDeclRule
      : rule<create<ast::ArrayDecl>(
                 option(flag<&ast::ArrayDecl::own_>(ikw<"own">)) +
                 option(assign<&ast::ArrayDecl::type_name>(TypeTok)) +
                 ikw<"array"> +
                 some(append<&ast::ArrayDecl::segments>(ref<ArraySegmentRule>),
                      lit<",">))> {};

  struct SwitchDeclRule
      : rule<create<ast::SwitchDecl>(
                 ikw<"switch"> +
                 assign<&ast::SwitchDecl::name>(ref<Identifier>) +
                 lit<":="> +
                 some(append<&ast::SwitchDecl::items>(ref<DesigExpr>),
                      lit<",">))> {};

  struct SpecifierRule
      : rule<create<ast::Specifier>(
                 assign<&ast::Specifier::kind>(ikw<"string">)  //
                 | (option(assign<&ast::Specifier::type_name>(TypeTok)) +
                    assign<&ast::Specifier::kind>(ikw<"array">))  //
                 | assign<&ast::Specifier::kind>(ikw<"label">)  //
                 | assign<&ast::Specifier::kind>(ikw<"switch">)  //
                 | (option(assign<&ast::Specifier::type_name>(TypeTok)) +
                    assign<&ast::Specifier::kind>(ikw<"procedure">))  //
                 | assign<&ast::Specifier::kind>(TypeTok)) +
             some(append<&ast::Specifier::names>(ref<Identifier>),
                  lit<",">)> {};

  // Splices into the caller's node (a `rule<>` of its own would open a
  // second head on `ast::ProcDecl`).
  static constexpr auto ProcHeading =
      assign<&ast::ProcDecl::name>(ref<Identifier>) +
      option(lit<"("> +
             some(append<&ast::ProcDecl::params>(ref<Identifier>), ParamSep) +
             lit<")">) +
      lit<";"> +
      option(ikw<"value"> +
             some(append<&ast::ProcDecl::value_params>(ref<Identifier>),
                  lit<",">) +
             lit<";">) +
      many(append<&ast::ProcDecl::specs>(ref<SpecifierRule>) + lit<";">);

  // procedure_body: a statement, or `code` -- unspecified, non-Algol text.
  // No corpus document needs `code`.
  struct ProcDeclRule
      : rule<create<ast::ProcDecl>(
                 option(assign<&ast::ProcDecl::type_name>(TypeTok)) +
                 ikw<"procedure"> + ProcHeading +
                 assign<&ast::ProcDecl::body>(ref<Statement>))> {};

  struct DeclarationRule
      : rule<ref<ProcDeclRule>    //
             | ref<ArrayDeclRule> //
             | ref<SwitchDeclRule>//
             | ref<TypeDeclRule>> {};

  // The optional close-tag after `end` -- `end name;`, echoing the block's
  // own label or procedure name. Stops on its own: `;`/`end`/`else` never
  // lex as a name here, so no lookahead guard is needed.
  static constexpr auto EndCloseTag =
      option(ref<Identifier> | ref<UnsignedInteger>);

  // block / compound_statement, unified: an optional `begin decl;...;`
  // head, then a `;`-separated statement list, closed by `end`.
  struct Block
      : rule<create<ast::Block>(
                 ikw<"begin"> +
                 option(some(append<&ast::Block::decls>(ref<DeclarationRule>),
                             lit<";">) +
                        lit<";">) +
                 some(append<&ast::Block::statements>(ref<StatementEntry>),
                      lit<";">) +
                 // The statement right before `end` may be empty and
                 // unlabelled -- a bare trailing `;`, not itself a list item.
                 option(lit<";">) +
                 ikw<"end"> + EndCloseTag)> {};

  struct ProgramRule
      : rule<many((ref<Identifier> | ref<UnsignedInteger>) + lit<":">) +
             ref<Block>> {};

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace algol60
