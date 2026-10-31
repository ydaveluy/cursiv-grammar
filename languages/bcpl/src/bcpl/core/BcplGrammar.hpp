// The bcpl language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "bcpl/core/ast.hpp"

namespace bcpl {

inline constexpr auto section_tag = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text == "{" || text == "}") return Result::ok({});
  if (text.size() < 2 || text[0] != '$' || (text[1] != '(' && text[1] != ')'))
    return Result::err("Incomplete BCPL section bracket.");
  return Result::ok(text.substr(2));
};

struct Bcpl : cursiv::GrammarSpec {
  // Identifiers absorb '.' and digits (`a.b`, `x1`), so a keyword must not
  // be followed by any of those bytes either, or "IFx" would still lex as
  // the keyword `IF` plus a name.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9._">;

  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<ML_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};

  struct IdentTok : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9._">)> {};

  static constexpr auto DecimalDigits = d + many(d | lit<"_">);
  template <auto Digit>
  static constexpr auto RadixDigits = many(lit<"_">) +
      some(Digit + many(lit<"_">));
  struct Digits : terminal<DecimalDigits> {};
  struct OctalNumber
      : terminal<lit<"#"> + option(cr<"oO">) + RadixDigits<cr<"0-7">>> {};
  struct HexNumber
      : terminal<(lit<"#X"> | lit<"#x">) + RadixDigits<cr<"0-9A-Fa-f">>> {};
  struct BinaryNumber
      : terminal<(lit<"#B"> | lit<"#b">) + RadixDigits<cr<"01">>> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + DecimalDigits;
  static constexpr auto FloatBody =
      (DecimalDigits + lit<".", !lit<".">> + many(d | lit<"_">) +
       option(Exponent)) |
      (lit<"."> + DecimalDigits + option(Exponent)) |
      (DecimalDigits + Exponent);
  struct FloatNumber : terminal<FloatBody> {};

  // Keep the complete source spelling; interpretation depends on the
  // compiler's character encoding and word length.
  static constexpr auto Hex = cr<"0-9A-Fa-f">;
  static constexpr auto EncodingSwitch = lit<"*#"> + cr<"gGuU">;
  static constexpr auto CharEscape =
      lit<"*"> + ((cr<"xX"> + repeat<2>(Hex)) |
                  repeat<3>(cr<"0-7">) |
                  (lit<"##"> + repeat<1, 8>(Hex)) |
                  (lit<"#"> + repeat<1, 4>(Hex)) |
                  cr<R"(nNcCpPsSbBtTeE"'*)">);
  struct CharConst
      : terminal<lit<"'"> + many(EncodingSwitch) +
                 (CharEscape | cr<R"(^'*\r\n)">) + lit<"'">> {};
  static constexpr auto StringContinuation =
      lit<"*"> + some(s | ref<SL_COMMENT>) + lit<"*">;
  struct StringConst
      : terminal<lit<"\""> +
                 many(StringContinuation | (lit<"*"> + any) |
                      cr<R"(^"*)">) + lit<"\"">> {};

  struct SignedNumber
      : terminal<option(cr<"+-">) +
                 (FloatBody | ref<OctalNumber> | ref<HexNumber> |
                  ref<BinaryNumber> | DecimalDigits)> {};

  // Builds a node: most name sites hold `Ident*`. Where a field is a bare
  // `string_view` instead, the raw `IdentTok` is used directly.
  struct Identifier
      : rule<option(flag<&ast::Ident::floating>(kw<"FLT">)) +
             assign<&ast::Ident::name>(ref<IdentTok>)> {};

  static constexpr auto MultOp = lit<"*"> | lit<"/"> | kw<"MOD"> | kw<"REM"> |
                                 lit<"#*"> | lit<"#/"> | kw<"#MOD"> | kw<"#REM">;
  static constexpr auto AddOp = lit<"+"> | lit<"-"> | lit<"#+"> | lit<"#-">;
  static constexpr auto Relop = lit<"="> | kw<"EQ"> | lit<"~="> | kw<"NE"> |
                                lit<"<="> | lit<"<"> | lit<">="> | lit<">"> |
                                lit<"#="> | lit<"#~="> | lit<"#<="> |
                                lit<"#<"> | lit<"#>="> | lit<"#>">;
  static constexpr auto ShiftOp = lit<"<<"> | lit<">>">;
  static constexpr auto EqvOp = kw<"EQV"> | kw<"XOR"> | kw<"NEQV">;
  static constexpr auto Fcond = lit<"->"> | lit<"#->">;
  static constexpr auto RangeOp = lit<".."> | lit<"#..">;
  static constexpr auto Jcom = kw<"NEXT"> | kw<"EXIT"> | kw<"BREAK"> |
                               kw<"LOOP"> | kw<"ENDCASE">;
  static constexpr auto AssOp =
      lit<":="> | lit<"*:="> | lit<"/:="> | kw<"MOD:="> | lit<"+:="> |
      lit<"~:="> | lit<"#:="> | lit<"#*:="> | lit<"#/:="> | kw<"#MOD:="> |
      lit<"-:="> | lit<"#+:="> | lit<"#-:="> | lit<"#~:="> |
      lit<"<<:="> | lit<">>:="> | lit<"&:="> |
      lit<"|:="> | kw<"EQV:="> | kw<"NEQV:="> | kw<"XOR:=">;
  static constexpr auto TagChar = cr<"a-zA-Z0-9_"> | lit<".", !lit<".">>;
  struct OPEN_SECTION : terminal<as<section_tag>(
      lit<"{"> | (lit<"$("> + many(TagChar)))> {};
  struct CLOSE_SECTION : terminal<as<section_tag>(
      lit<"}"> | (lit<"$)"> + many(TagChar)))> {};
  static constexpr auto OB = ref<OPEN_SECTION>;
  static constexpr auto CB = ref<CLOSE_SECTION>;
  static constexpr auto Bpat =
      ref<SignedNumber> | kw<"TRUE"> | kw<"FALSE"> | lit<"?"> | ref<CharConst>;
  // One or more semicolons, or nothing where a newline sits in the gap:
  // BCPL's only other statement separator.
  static constexpr auto Semi = some(lit<";">) | gap::line;

  struct ConstExpr;
  struct Expression;
  struct Command;
  struct Declaration;
  struct PatternList;
  struct PostfixExpr;
  struct MultExpr;
  struct ShiftExpr;
  struct CompoundStmt;

  // -- Constant expressions (8.8.4): no postfix, no relational/logical
  // levels, no `nonl` gating -- a smaller, ungated grammar of its own. --

  struct ConstPrimary
      : rule<create<ast::CharLit>(
                 assign<&ast::CharLit::text>(ref<CharConst>))  //
             | create<ast::NumberLit>(
                   assign<&ast::NumberLit::text>(
                       ref<OctalNumber> | ref<HexNumber> | ref<Digits> |
                       ref<BinaryNumber> | ref<FloatNumber>))  //
             | ref<Identifier>  //
             | create<ast::BoolLit>(
                   flag<&ast::BoolLit::value>(kw<"TRUE">))  //
             | create<ast::BoolLit>(kw<"FALSE">)  //
             | create<ast::UndefLit>(lit<"?">)  //
             | create<ast::GroupExpr>(
                   lit<"("> + assign<&ast::GroupExpr::inner>(ref<ConstExpr>) +
                   lit<")">)> {};

  struct ConstUnary
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(AddOp) +
                 assign<&ast::PrefixExpr::operand>(ref<ConstPrimary>))  //
             | ref<ConstPrimary>> {};

  struct ConstExpr
      : infix<ConstUnary, ast::BinaryExpr,
              levels(left(MultOp),    //
                     left(AddOp),     //
                     left(ShiftOp),   //
                     left(lit<"&">),  //
                     left(lit<"|">))> {};

  // -- Expressions (8.8.3): postfix, then a manual precedence climb so
  // every step can be gated by `nonl` -- no operator continues onto a new
  // line; the operand of one that does may still start on the next. --

  struct Fname
      : rule<create<ast::FlnamePattern>(
            kw<"FLT"> + assign<&ast::FlnamePattern::name>(ref<IdentTok>))> {
  };

  struct Sp
      : rule<create<ast::RelPattern>(
                 assign<&ast::RelPattern::op>(Relop) +
                 (assign<&ast::RelPattern::value>(Bpat)  //
                  | (lit<"("> +
                     assign<&ast::RelPattern::expr>(ref<Expression>) +
                     lit<")">)))  //
             | create<ast::JcomPattern>(assign<&ast::JcomPattern::op>(Jcom))  //
             | create<ast::GroupPattern>(
                   lit<"("> +
                   assign<&ast::GroupPattern::body>(ref<PatternList>) +
                   lit<")">)  //
             | create<ast::GroupPattern>(
                   flag<&ast::GroupPattern::bracket>(lit<"[">) +
                   assign<&ast::GroupPattern::body>(ref<PatternList>) +
                   lit<"]">)  //
             | create<ast::ValuePattern>(
                   assign<&ast::ValuePattern::value>(Bpat) +
                   option(assign<&ast::ValuePattern::rangeOp>(RangeOp) +
                          assign<&ast::ValuePattern::rangeValue>(Bpat)))  //
             | ref<Fname>> {};

  struct PatternTermFirst
      : rule<assign<&ast::PatternTerm::term>(ref<Sp>)> {};
  struct PatternTermJoined
      : rule<create<ast::PatternTerm>(
            option(assign<&ast::PatternTerm::join>(lit<","> | lit<"|">)) +
            assign<&ast::PatternTerm::term>(ref<Sp>))> {};
  struct PatternList
      : rule<append<&ast::PatternList::items>(ref<PatternTermFirst>) +
            many(append<&ast::PatternList::items>(ref<PatternTermJoined>))> {
  };

  struct MatchClause
      : rule<create<ast::MatchClause>(
            lit<":"> +
            option(assign<&ast::MatchClause::pattern>(ref<PatternList>)) +
            (assign<&ast::MatchClause::result>(lit<"->"> + ref<Expression>)  //
             | assign<&ast::MatchClause::action>(kw<"BE"> + ref<Command>)))> {
  };

  // Spliced into the enclosing `MatchExpr` below, not its own rule: its
  // writes target `MatchExpr`, not a class of its own.
  static constexpr auto Mlist =
      some(append<&ast::MatchExpr::clauses>(ref<MatchClause>)) +
      option(lit<".">);

  struct Primary
      : rule<ref<Identifier>  //
             | create<ast::CharLit>(
                   assign<&ast::CharLit::text>(ref<CharConst>))  //
             | create<ast::NumberLit>(
                   assign<&ast::NumberLit::text>(
                       ref<OctalNumber> | ref<HexNumber> | ref<Digits> |
                       ref<BinaryNumber> | ref<FloatNumber>))  //
             | create<ast::BoolLit>(
                   flag<&ast::BoolLit::value>(kw<"TRUE">))  //
             | create<ast::BoolLit>(kw<"FALSE">)  //
             | create<ast::UndefLit>(lit<"?">)  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::text>(ref<StringConst>))  //
             | create<ast::SelectExpr>(
                   kw<"SLCT"> + assign<&ast::SelectExpr::a>(ref<Expression>) +
                   option(lit<":"> +
                          assign<&ast::SelectExpr::b>(ref<Expression>) +
                          option(lit<":"> +
                                 assign<&ast::SelectExpr::c>(
                                     ref<Expression>))))  //
             | create<ast::KeywordExpr>(
                   assign<&ast::KeywordExpr::text>(
                       kw<"NEXT"> | kw<"EXIT"> | kw<"BREAK"> | kw<"LOOP"> |
                       kw<"ENDCASE">))  //
             | create<ast::GroupExpr>(
                   lit<"("> +
                   assign<&ast::GroupExpr::inner>(ref<Expression>) +
                   lit<")">)  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(
                       kw<"FLOAT"> | kw<"FIX"> | lit<"!"> | lit<"@">) +
                   assign<&ast::PrefixExpr::operand>(ref<PostfixExpr>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(
                       lit<"+"> | lit<"-"> | kw<"ABS"> | lit<"#+"> |
                       lit<"#-"> | kw<"#ABS">) +
                   assign<&ast::PrefixExpr::operand>(ref<MultExpr>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(kw<"NOT"> | lit<"~">) +
                   assign<&ast::PrefixExpr::operand>(ref<ShiftExpr>))  //
             | create<ast::TableExpr>(
                   kw<"TABLE"> +
                   some(append<&ast::TableExpr::values>(ref<Expression>),
                        lit<",">))  //
             | create<ast::VecAllocExpr>(
                   lit<"#("> +
                   some(append<&ast::VecAllocExpr::values>(ref<Expression>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::MatchExpr>(
                   assign<&ast::MatchExpr::kind>(kw<"MATCH"> | kw<"EVERY">) +
                   lit<"("> +
                   some(append<&ast::MatchExpr::scrutinees>(ref<Expression>),
                        lit<",">) +
                   lit<")"> + Mlist)  //
             | create<ast::ValofExpr>(
                   kw<"VALOF"> +
                   assign<&ast::ValofExpr::body>(ref<Command>))> {};

  // Call, index, and the OF/::/!/% suffixes all share BCPL's tightest tier.
  struct PostfixExpr
      : rule<ref<Primary> +
            many(!gap::line +
                 (nest<ast::CallExpr, &ast::CallExpr::callee>(
                      option(flag<&ast::CallExpr::method>(lit<"#">)) +
                      lit<"("> +
                      many(append<&ast::CallExpr::args>(ref<Expression>),
                           lit<",">) +
                      lit<")">)  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::array>(
                        lit<"["> +
                        assign<&ast::IndexExpr::index>(ref<Expression>) +
                        lit<"]">)  //
                  | nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                        assign<&ast::BinaryExpr::op>(
                            kw<"OF"> | lit<"::"> | lit<"!"> | lit<"%">) +
                        assign<&ast::BinaryExpr::right>(ref<PostfixExpr>))))> {
  };

  struct MultExpr
      : rule<ref<PostfixExpr> +
            many(!gap::line +
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(MultOp) +
                     assign<&ast::BinaryExpr::right>(ref<PostfixExpr>)))> {};

  struct AddExpr
      : rule<ref<MultExpr> +
            many(!gap::line +
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(AddOp) +
                     assign<&ast::BinaryExpr::right>(ref<MultExpr>)))> {};

  struct RelExpr
      : rule<ref<AddExpr> +
            many(!gap::line +
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(Relop) +
                     assign<&ast::BinaryExpr::right>(ref<AddExpr>)))> {};

  struct ShiftExpr
      : rule<ref<RelExpr> +
            many(!gap::line +
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(ShiftOp) +
                     assign<&ast::BinaryExpr::right>(ref<RelExpr>)))> {};

  struct AndExpr
      : rule<ref<ShiftExpr> +
            many(!gap::line +
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(lit<"&">) +
                     assign<&ast::BinaryExpr::right>(ref<ShiftExpr>)))> {};

  struct OrExpr
      : rule<ref<AndExpr> +
            many(!gap::line +
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(lit<"|">) +
                     assign<&ast::BinaryExpr::right>(ref<AndExpr>)))> {};

  struct EqvExpr
      : rule<ref<OrExpr> +
            many(!gap::line +
                 nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                     assign<&ast::BinaryExpr::op>(EqvOp) +
                     assign<&ast::BinaryExpr::right>(ref<OrExpr>)))> {};

  struct Expression
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<EqvExpr>) + !gap::line +
                 assign<&ast::CondExpr::op>(Fcond) +
                 assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<","> +
                 assign<&ast::CondExpr::elseValue>(ref<Expression>))  //
             | ref<EqvExpr>> {};

  // -- Declarations (8.8.6) --

  struct ManifestItem
      : rule<create<ast::ManifestItem>(
            assign<&ast::ManifestItem::name>(ref<Identifier>) +
            option((lit<"="> | kw<"EQ">) +
                   assign<&ast::ManifestItem::value>(ref<ConstExpr>)))> {};

  struct ManifestDecl
      : rule<kw<"MANIFEST"> + assign<&ast::ManifestDecl::openTag>(OB) +
            option(append<&ast::ManifestDecl::items>(ref<ManifestItem>) +
                   many(Semi + append<&ast::ManifestDecl::items>(ref<ManifestItem>)) +
                   option(lit<";">)) +
            assign<&ast::ManifestDecl::closeTag>(CB)> {};

  struct StaticDecl
      : rule<kw<"STATIC"> + assign<&ast::StaticDecl::openTag>(OB) +
            option(append<&ast::StaticDecl::items>(ref<ManifestItem>) +
                   many(Semi + append<&ast::StaticDecl::items>(ref<ManifestItem>)) +
                   option(lit<";">)) +
            assign<&ast::StaticDecl::closeTag>(CB)> {};

  struct GlobalItem
      : rule<create<ast::GlobalItem>(
            assign<&ast::GlobalItem::name>(ref<Identifier>) +
            option(lit<":"> +
                   assign<&ast::GlobalItem::value>(ref<ConstExpr>)))> {};

  struct GlobalDecl
      : rule<kw<"GLOBAL"> + assign<&ast::GlobalDecl::openTag>(OB) +
            option(append<&ast::GlobalDecl::items>(ref<GlobalItem>) +
                   many(Semi + append<&ast::GlobalDecl::items>(ref<GlobalItem>)) +
                   option(lit<";">)) +
            assign<&ast::GlobalDecl::closeTag>(CB)> {};

  struct SimpleDef
      : rule<append<&ast::SimpleDef::names>(ref<Identifier>) +
            many(lit<","> + append<&ast::SimpleDef::names>(ref<Identifier>)) +
            (lit<"="> | kw<"EQ">) +
            append<&ast::SimpleDef::values>(ref<Expression>) +
            many(lit<","> +
                 append<&ast::SimpleDef::values>(ref<Expression>))> {};

  struct VectorDef
      : rule<assign<&ast::VectorDef::name>(ref<Identifier>) +
            (lit<"="> | kw<"EQ">) + kw<"VEC"> +
            assign<&ast::VectorDef::size>(ref<ConstExpr>)> {};

  struct FunctionDef
      : rule<assign<&ast::FunctionDef::name>(ref<Identifier>) + lit<"("> +
            option(append<&ast::FunctionDef::params>(ref<Identifier>) +
                   many(lit<","> + append<&ast::FunctionDef::params>(
                                       ref<Identifier>))) +
            lit<")"> + (lit<"="> | kw<"EQ">) +
            assign<&ast::FunctionDef::body>(ref<Expression>)> {};

  struct RoutineDef
      : rule<assign<&ast::RoutineDef::name>(ref<Identifier>) + lit<"("> +
            option(append<&ast::RoutineDef::params>(ref<Identifier>) +
                   many(lit<","> + append<&ast::RoutineDef::params>(
                                       ref<Identifier>))) +
            lit<")"> + kw<"BE"> +
            assign<&ast::RoutineDef::body>(ref<Command>)> {};

  struct Definition
      : rule<ref<SimpleDef> | ref<VectorDef> | ref<FunctionDef> |
            ref<RoutineDef>> {};

  struct LetDecl
      : rule<kw<"LET"> +
            append<&ast::LetDecl::definitions>(ref<Definition>) +
            many(kw<"AND"> +
                 append<&ast::LetDecl::definitions>(ref<Definition>))> {};

  struct Declaration
      : rule<ref<LetDecl> | ref<ManifestDecl> | ref<StaticDecl> |
            ref<GlobalDecl>> {};

  // -- Commands (8.8.8-8.8.10) --

  // Spliced into the enclosing `AssignStmt` below: its writes target
  // `AssignStmt`, not a class of its own.
  static constexpr auto LhsList =
      append<&ast::AssignStmt::targets>(ref<Expression>) +
      many(lit<","> + append<&ast::AssignStmt::targets>(ref<Expression>));

  struct AssignStmtRule
      : rule<create<ast::AssignStmt>(
            LhsList + assign<&ast::AssignStmt::op>(AssOp) +
            append<&ast::AssignStmt::values>(ref<Expression>) +
            many(lit<","> +
                 append<&ast::AssignStmt::values>(ref<Expression>)))> {};

  struct SimpleStmt
      : rule<create<ast::SimpleStmt>(assign<&ast::SimpleStmt::kw>(
            kw<"BREAK"> | kw<"LOOP"> | kw<"ENDCASE"> | kw<"RETURN"> |
            kw<"FINISH">))> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
            kw<"GOTO"> + assign<&ast::GotoStmt::target>(ref<Expression>))> {};

  // A call used as a statement is one call layer over a bare primary, not
  // the general postfix chain: `f(x)(y)` is not a statement, only an
  // expression.
  struct RoutineCmd
      : rule<create<ast::CallStmt>(
            assign<&ast::CallStmt::callee>(ref<Primary>) +
            option(flag<&ast::CallStmt::method>(lit<"#">)) + lit<"("> +
            many(append<&ast::CallStmt::args>(ref<Expression>), lit<",">) +
            lit<")">)> {};

  struct ResultisStmt
      : rule<create<ast::ResultisStmt>(
            kw<"RESULTIS"> +
            assign<&ast::ResultisStmt::value>(ref<Expression>))> {};

  struct CompoundStmt
      : rule<create<ast::CompoundStmt>(
            assign<&ast::CompoundStmt::openTag>(OB) +
            option(append<&ast::CompoundStmt::commands>(ref<Command>) +
                   many(Semi +
                        append<&ast::CompoundStmt::commands>(ref<Command>))) +
            assign<&ast::CompoundStmt::closeTag>(CB))> {};

  struct SwitchonStmt
      : rule<create<ast::SwitchonStmt>(
            kw<"SWITCHON"> +
            assign<&ast::SwitchonStmt::value>(ref<Expression>) +
            kw<"INTO"> +
            assign<&ast::SwitchonStmt::body>(ref<CompoundStmt>))> {};

  struct Block
      : rule<create<ast::Block>(
            assign<&ast::Block::openTag>(OB) +
            append<&ast::Block::declarations>(ref<Declaration>) +
            many(Semi + append<&ast::Block::declarations>(ref<Declaration>)) +
            option(Semi +
                   option(append<&ast::Block::commands>(ref<Command>) +
                          many(Semi + append<&ast::Block::commands>(ref<Command>)))) +
            assign<&ast::Block::closeTag>(CB))> {};

  // Every form `REPEAT`/`REPEATUNTIL`/`REPEATWHILE` may suffix, one rule.
  struct BaseStmt
      : rule<(ref<AssignStmtRule> | ref<SimpleStmt> | ref<GotoStmt> |
             ref<RoutineCmd> | ref<ResultisStmt> |
             ref<SwitchonStmt> | ref<CompoundStmt> | ref<Block>) +
            many(nest<ast::RepeatStmt, &ast::RepeatStmt::body>(
                assign<&ast::RepeatStmt::kind>(kw<"REPEAT">)  //
                | (assign<&ast::RepeatStmt::kind>(kw<"REPEATUNTIL">) +
                   assign<&ast::RepeatStmt::cond>(ref<Expression>))  //
                | (assign<&ast::RepeatStmt::kind>(kw<"REPEATWHILE">) +
                   assign<&ast::RepeatStmt::cond>(ref<Expression>))))> {};

  struct UntilStmt
      : rule<create<ast::UntilStmt>(
            kw<"UNTIL"> + assign<&ast::UntilStmt::cond>(ref<Expression>) +
            kw<"DO"> + assign<&ast::UntilStmt::body>(ref<Command>))> {};

  struct WhileStmt
      : rule<create<ast::WhileStmt>(
            kw<"WHILE"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
            kw<"DO"> + assign<&ast::WhileStmt::body>(ref<Command>))> {};

  struct ForStmt
      : rule<create<ast::ForStmt>(
            kw<"FOR"> + assign<&ast::ForStmt::var>(ref<Identifier>) +
            (lit<"="> | kw<"EQ">) +
            assign<&ast::ForStmt::from>(ref<Expression>) + kw<"TO"> +
            assign<&ast::ForStmt::to>(ref<Expression>) +
            option(kw<"BY"> + assign<&ast::ForStmt::by>(ref<ConstExpr>)) +
            option(kw<"DO">) + assign<&ast::ForStmt::body>(ref<Command>))> {};

  struct TestStmt
      : rule<create<ast::TestStmt>(
            kw<"TEST"> + assign<&ast::TestStmt::cond>(ref<Expression>) +
            option(kw<"THEN"> | kw<"DO">) +
            assign<&ast::TestStmt::thenBranch>(ref<Command>) +
            (kw<"ELSE"> | kw<"OR">) +
            assign<&ast::TestStmt::elseBranch>(ref<Command>))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"IF"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            option(kw<"THEN"> | kw<"DO">) +
            assign<&ast::IfStmt::thenBranch>(ref<Command>))> {};

  struct UnlessStmt
      : rule<create<ast::UnlessStmt>(
            kw<"UNLESS"> + assign<&ast::UnlessStmt::cond>(ref<Expression>) +
            option(kw<"THEN"> | kw<"DO">) +
            assign<&ast::UnlessStmt::thenBranch>(ref<Command>))> {};

  struct UnlabelledCommand
      : rule<ref<BaseStmt> | ref<UntilStmt> | ref<WhileStmt> |
            ref<ForStmt> | ref<UnlessStmt> | ref<TestStmt> |
            ref<IfStmt>> {};

  struct LabelPrefix
      : rule<create<ast::LabelPrefix>(
            assign<&ast::LabelPrefix::name>(ref<IdentTok>) + lit<":">)> {};

  struct CasePrefix
      : rule<create<ast::CasePrefix>(
            kw<"CASE"> + assign<&ast::CasePrefix::value>(ref<ConstExpr>) +
            lit<":">)> {};

  struct DefaultPrefix
      : rule<create<ast::DefaultPrefix>(kw<"DEFAULT"> + lit<":">)> {};

  struct Prefix
      : rule<ref<LabelPrefix> | ref<CasePrefix> |
            ref<DefaultPrefix>> {};

  struct Command
      : rule<create<ast::Command>(
                 many(append<&ast::Command::prefixes>(ref<Prefix>)) +
                 assign<&ast::Command::body>(ref<UnlabelledCommand>))  //
             | create<ast::Command>(
                   some(append<&ast::Command::prefixes>(ref<Prefix>)))> {
  };

  struct Directive
      : rule<create<ast::Directive>(
            assign<&ast::Directive::kw>(kw<"GET"> | kw<"SECTION">) +
            assign<&ast::Directive::text>(ref<StringConst>))> {};

  struct Program
      : rule<many(append<&ast::Program::directives>(ref<Directive>)) +
            append<&ast::Program::declarations>(ref<Declaration>) +
            many(Semi +
                 append<&ast::Program::declarations>(ref<Declaration>))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = list<ML_COMMENT, SL_COMMENT>;
};

}  // namespace bcpl
