// The Algol 60 language: its grammar. LR with the default engine; the token
// graph is per-state, so a keyword's spelling never competes with a name
// outside the states that read it.
#pragma once

#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>

#include "algol60/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace algol60 {

// A raised-ten exponent signed at its own mark ("2.5+10+6") cannot be
// mistaken for a longer mantissa, since a mantissa has no sign; unsigned
// ("2.5106") reads one decimal, the mark undistinguished from more digits --
// the report's own ambiguity once "10" loses its raised typography.
inline constexpr auto unsigned_number = [](std::string_view t) {
  // Normalize the report's raised-ten notation before the checked conversion.
  // Converting the mantissa and power separately can overflow even when their
  // product is representable (for example .0110+310 == 1e308).
  std::string normalized;
  for (std::size_t i = 0; i + 2 < t.size(); ++i) {
    if (t[i] != '1' || t[i + 1] != '0') continue;
    const bool unicode_minus = t.substr(i + 2).starts_with("–");
    if (t[i + 2] != '+' && t[i + 2] != '-' && !unicode_minus) continue;
    normalized.assign(t.substr(0, i));
    normalized += 'e';
    normalized += (t[i + 2] == '-' || unicode_minus) ? '-' : '+';
    normalized += t.substr(i + 2 + (unicode_minus ? 3 : 1));
    t = normalized;
    break;
  }
  double value = 0;
  const auto [end, error] = std::from_chars(t.data(), t.data() + t.size(), value);
  using Result = cursiv::ConversionResult<double>;
  if (error != std::errc{} || end != t.data() + t.size())
    return Result::err("Number literal is outside the double range.");
  return Result::ok(value);
};

struct Algol60 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // The first comment structure wins (§2.3). END reads its own comment,
  // including any later `comment` word, before hidden trivia can see it.
  struct COMMENT : terminal<through(ikw<"comment">, lit<";">)> {};

  static constexpr auto Digits = some(cr<"0-9">);
  static constexpr auto ExpSign = lit<"+"> | lit<"-"> | lit<"–">;
  // Unsigned_number: an integer, a fraction, or both, with an optional
  // exponent introduced by the digits `10` (the report's raised "ten").
  struct UnsignedNumber
      : terminal<as<unsigned_number>(
            ((Digits + option(lit<"."> + Digits)) | (lit<"."> + Digits)) +
            option(lit<"10"> + option(ExpSign) + Digits))> {};

  // (Letter|Digit)+, narrowed to require one letter somewhere: a bare digit
  // run is a number, not a name. A digit-then-letter spelling like `1e` (an
  // identifier this alphabet allows, seen where a document leans on a
  // foreign exponent mark) still matches.
  struct Identifier
      : terminal<(Digits + cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">)) |
                     (cr<"a-zA-Z_"> + many(cr<"a-zA-Z0-9_">))> {};
  struct UnsignedInteger : terminal<some(cr<"0-9">)> {};

  struct EndCommentWord
      : terminal<!(ikw<"end"> | ikw<"else">) + some(w)> {};
  struct END
      : terminal<ikw<"end"> +
                 many(ref<EndCommentWord> | cr<R"(^;A-Za-z0-9_)">)> {};

  // Unstropped control words cannot fall back to a bare procedure call.
  struct CallIdentifier
      : terminal<!(ikw<"if"> | ikw<"then"> | ikw<"else"> | ikw<"for"> |
                    ikw<"do"> | ikw<"begin"> | ikw<"end">) +
                 ref<Identifier>> {};

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

  struct ExpOp : terminal<as<ast::BinOp::power>(lit<"↑"> | lit<"^">)> {};
  struct MulOp : terminal<as<ast::BinOp::mul>(lit<"×"> | lit<"*">)> {};
  struct DivOp : terminal<as<ast::BinOp::div>(lit<"/">) |
                          as<ast::BinOp::intdiv>(lit<"÷">)> {};
  struct SubOp : terminal<as<ast::BinOp::sub>(lit<"-"> | lit<"–">)> {};

  struct LeOp : terminal<as<ast::BinOp::le>(lit<"<="> | lit<"≤">)> {};
  struct NeOp : terminal<as<ast::BinOp::ne>(lit<"≠"> | lit<"!=">)> {};
  struct GeOp : terminal<as<ast::BinOp::ge>(lit<">="> | lit<"≥">)> {};

  struct UMinusOp : terminal<as<ast::UnOp::minus>(lit<"-"> | lit<"–">)> {};

  struct NotOp : terminal<as<ast::UnOp::lnot>(lit<"¬"> | lit<"!">)> {};
  struct AndOp : terminal<as<ast::BinOp::land>(lit<"⋀"> | lit<"&">)> {};
  struct OrOp : terminal<as<ast::BinOp::lor>(lit<"⋁"> | lit<"|">)> {};

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

  struct Factor : infix<PrimaryExpr, ast::BinaryExpr,
                        left(ref<ExpOp>)> {};

  // The operand of `+`/`-`: an optional sign wrapped around a Factor. A
  // sign is available at every `*`/`/`/`+`/`-` operand this way, which
  // over-accepts `2*-3` beside the report's own `2*(-3)` -- harmless.
  struct Term
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(as<ast::UnOp::plus>(lit<"+">) | ref<UMinusOp>) +
                 assign<&ast::UnaryExpr::operand>(ref<Factor>))  //
             | ref<Factor>> {};

  struct SimpleArith
      : infix<Term, ast::BinaryExpr,
              levels(left(ref<MulOp> | ref<DivOp>),
                     left(as<ast::BinOp::add>(lit<"+">) | ref<SubOp>))> {};

  struct ArithExpr
      : rule<create<ast::IfExpr>(
                 ikw<"if"> + assign<&ast::IfExpr::cond>(ref<BoolExpr>) +
                 ikw<"then"> +
                 assign<&ast::IfExpr::then_branch>(ref<SimpleArith>) +
                 ikw<"else"> +
                 assign<&ast::IfExpr::else_branch>(ref<ArithExpr>))  //
             | ref<SimpleArith>> {};

  struct Relation
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<SimpleArith>) +
                 assign<&ast::BinaryExpr::op>(as<ast::BinOp::lt>(lit<"<">) | ref<LeOp> |
                                              as<ast::BinOp::eq>(lit<"=">) | ref<NeOp> |
                                              as<ast::BinOp::gt>(lit<">">) | ref<GeOp>) +
                 assign<&ast::BinaryExpr::right>(ref<SimpleArith>))> {};

  struct BoolPrimaryExpr
      : rule<create<ast::LogicalLiteral>(
                 assign<&ast::LogicalLiteral::value>(as<true>(ikw<"true">) |
                                                     as<false>(ikw<"false">)))  //
             | ref<Relation>  //
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
      : infix<BoolSecondary, ast::BinaryExpr,
              levels(left(ref<AndOp>), left(ref<OrOp>), left(as<ast::BinOp::implies>(lit<"⊃">)),
                     left(as<ast::BinOp::equiv>(lit<"≡">)))> {};

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
                 assign<&ast::CallStmt::callee>(ref<CallIdentifier>) +
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
      : rule<assign<&ast::ForSimple::value>(ref<ArithExpr>)> {};

  struct EmptyStatement : rule<create<ast::EmptyStmt>> {};

  struct UnlabelledUnconditional
      : rule<ref<AssignStatement> | ref<GotoStatement> | ref<Block> |
             ref<CallStatement> | ref<EmptyStatement>> {};

  template <class Body>
  struct LabelledStatement
      : rule<some(append<&ast::StmtEntry::labels>(
                      ref<Identifier> | ref<UnsignedInteger>) + lit<":">) +
             assign<&ast::StmtEntry::stmt>(ref<Body>)> {};

  struct UnconditionalStatement
      : rule<ref<LabelledStatement<UnlabelledUnconditional>> |
             ref<UnlabelledUnconditional>> {};

  struct ForCore
      : rule<create<ast::ForStmt>(
                 ikw<"for"> + assign<&ast::ForStmt::var>(ref<Identifier>) +
                 lit<":="> +
                 some(append<&ast::ForStmt::items>(
                          ref<ForStepUntilItem> | ref<ForWhileItem> |
                          ref<ForSimpleItem>),
                      lit<",">) +
                 ikw<"do"> + assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct ForStatement
      : rule<ref<LabelledStatement<ForCore>> | ref<ForCore>> {};

  struct IfStatement
      : rule<create<ast::IfStmt>(
                 ikw<"if"> + assign<&ast::IfStmt::cond>(ref<BoolExpr>) +
                 ikw<"then"> +
                 ((assign<&ast::IfStmt::then_branch>(ref<UnconditionalStatement>) +
                   option(flag<&ast::IfStmt::hasElse>(ikw<"else">) +
                          assign<&ast::IfStmt::else_branch>(ref<Statement>))) |
                  assign<&ast::IfStmt::then_branch>(ref<ForStatement>)))> {
  };

  struct StatementCore
      : rule<ref<IfStatement> | ref<ForCore> |
             ref<UnlabelledUnconditional>> {};

  struct Statement
      : rule<ref<LabelledStatement<StatementCore>> | ref<StatementCore>> {};

  // Even a zero-width dummy has an entry and an EmptyStmt child.
  struct StatementEntry
      : rule<many(append<&ast::StmtEntry::labels>(
                      ref<Identifier> | ref<UnsignedInteger>) + lit<":">) +
             assign<&ast::StmtEntry::stmt>(ref<StatementCore>)> {};

  static constexpr auto LocalOrOwn =
      option(flag<&ast::TypeDecl::own_>(ikw<"own">));

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
                 LocalOrOwn +
                 assign<&ast::TypeDecl::type_name>(TypeTok) +
                 some(append<&ast::TypeDecl::names>(ref<Identifier>),
                      lit<",">))> {};

  struct BoundPair
      : rule<create<ast::BoundPair>(
                 assign<&ast::BoundPair::lower>(ref<ArithExpr>) + lit<":"> +
                 assign<&ast::BoundPair::upper>(ref<ArithExpr>))> {};

  struct ArraySegment
      : rule<create<ast::ArraySegment>(
                 some(append<&ast::ArraySegment::names>(ref<Identifier>),
                      lit<",">) +
                 lit<"["> +
                 some(append<&ast::ArraySegment::bounds>(
                          ref<BoundPair>),
                      lit<",">) +
                 lit<"]">)> {};

  struct ArrayDecl
      : rule<create<ast::ArrayDecl>(
                 option(flag<&ast::ArrayDecl::own_>(ikw<"own">)) +
                 option(assign<&ast::ArrayDecl::type_name>(TypeTok)) +
                 ikw<"array"> +
                 some(append<&ast::ArrayDecl::segments>(ref<ArraySegment>),
                      lit<",">))> {};

  struct SwitchDecl
      : rule<create<ast::SwitchDecl>(
                 ikw<"switch"> +
                 assign<&ast::SwitchDecl::name>(ref<Identifier>) +
                 lit<":="> +
                 some(append<&ast::SwitchDecl::items>(ref<DesigExpr>),
                      lit<",">))> {};

  struct Specifier
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
      many(append<&ast::ProcDecl::specs>(ref<Specifier>) + lit<";">);

  // procedure_body: a statement, or `code` -- unspecified, non-Algol text.
  // No corpus document needs `code`.
  struct ProcDecl
      : rule<create<ast::ProcDecl>(
                 option(assign<&ast::ProcDecl::type_name>(TypeTok)) +
                 ikw<"procedure"> + ProcHeading +
                 assign<&ast::ProcDecl::body>(ref<Statement>))> {};

  struct Declaration
      : rule<ref<ProcDecl>    //
             | ref<ArrayDecl> //
             | ref<SwitchDecl>//
             | ref<TypeDecl>> {};

  // block / compound_statement, unified: an optional `begin decl;...;`
  // head, then a `;`-separated statement list, closed by `end`.
  static constexpr auto BlockBody =
      ikw<"begin"> +
      option(some(append<&ast::Block::decls>(ref<Declaration>), lit<";">) + lit<";">) +
      append<&ast::Block::statements>(ref<StatementEntry>) +
      many(lit<";"> + append<&ast::Block::statements>(ref<StatementEntry>)) +
      ref<END>;

  struct Block : rule<create<ast::Block> + BlockBody> {};

  struct Program
      : rule<create<ast::Block> +
             many(append<&ast::Block::labels>(ref<Identifier> | ref<UnsignedInteger>) +
                  lit<":">) + BlockBody> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace algol60
