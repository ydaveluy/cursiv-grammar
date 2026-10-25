// The tinyosnesc language: its grammar. nesC bodies are lexed per state, so
// a keyword is reserved only where a statement can start; everything else
// (declarations, assignments, call arguments) is a run of balanced tokens --
// nesC bodies have no real C expression grammar below the shell.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tinyosnesc/core/ast.hpp"

namespace tinyosnesc {

struct TinyosNesc : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z_0-9">)> {};
  struct INT : terminal<some(d)> {};
  struct HEX : terminal<lit<"0x"> + some(cr<"0-9a-fA-F">)> {};
  struct FLOAT
      : terminal<(some(d) + lit<"."> + many(d)) | (lit<"."> + some(d))> {};
  struct STRING
      : terminal<lit<"\""> + many(lit<"\"\""> | (!cr<R"("\r\n)"> + any)) +
                 lit<"\"">> {};
  // The fallback: one otherwise-unmatched byte. Excludes whitespace
  // explicitly -- `any` alone would let it swallow trivia that should stay
  // silently skipped wherever `OTHER` is a live candidate. Braces are
  // structure, never an unknown byte.
  struct OTHER : terminal<cr<R"(^ \t\r\n{})">> {};

  // Forward declarations: the soup primitives are mutually recursive
  // (a balanced group's content may hold another group), and the statement
  // rules recurse into blocks of themselves and into `Expression`.
  struct Inner;
  struct ParenGroup;
  struct BracketGroup;
  struct BraceGroup;
  struct Soup;
  struct StmtSoup;
  struct LineSoup;
  struct PrimaryExpressionRule;
  struct UnaryExpressionRule;
  struct Expression;
  struct StatRule;
  struct FuncStatRule;
  struct IfBranchRule;
  struct ElseBranchRule;
  struct IfStatRule;
  struct WhileStatRule;
  struct ForStatRule;
  struct CaseClauseRule;
  struct SwitchStatRule;
  struct AtomicStatRule;
  struct EnumStatRule;
  struct CallStatRule;
  struct DefineStatRule;
  struct PacketDefineRule;
  struct UnknownCharStatRule;
  struct InterfaceNameAsRule;
  struct UsesOrProvidesRule;
  struct InterfaceMemberRule;
  struct InterfaceDeclRule;
  struct AtSoup;
  struct ModuleRule;
  struct ComponentsStatRule;
  struct WiringStatRule;
  struct ConfigItemRule;
  struct ConfigurationRule;
  struct ComponentRule;
  struct IncludeRule;
  struct DocumentRule;

  // Every ordinary token a name/signature run may hold: literals, `ID`,
  // keywords no rule otherwise uses (so they would be unusable anywhere,
  // not even as a name -- `signal`, `generic` and
  // `char`/`double`/`long`/`short` excepted, which are real structural or
  // base-type keywords handled on their own), and the operator punctuation.
  // The statement-leading keywords (`if`, `while`,
  // `for`, `switch`, `atomic`, `enum`, `call`, `signal`, `event`, `task`,
  // `static`, `typedef`, `case`, `default`, `as`, `new`) and the top-level
  // keywords are excluded, so a soup run always stops where one of those
  // may legally start.
  static constexpr auto Tok =
      ref<ID> | ref<INT> | ref<FLOAT> | ref<HEX> | ref<STRING> |    //
      kw<"true"> | kw<"false"> |                                    //
      kw<"void"> | kw<"return"> | kw<"post"> | kw<"error"> |         //
      kw<"abstract"> | kw<"break"> | kw<"continue"> | kw<"do"> |     //
      kw<"else"> | kw<"generic"> | kw<"includes"> | kw<"extends"> |  //
      kw<"final"> | kw<"log"> | kw<"component"> |                    //
      kw<"boolean"> | kw<"char"> | kw<"double"> | kw<"long"> |       //
      kw<"short"> | kw<"defined"> | kw<"define"> | kw<"elif"> |      //
      kw<"endif"> |                                                  //
      lit<"||"> | lit<"&&"> | lit<"=="> | lit<"!="> | lit<">="> |    //
      lit<"<="> | lit<">"> | lit<"<"> | lit<"+"> | lit<"-"> |        //
      lit<"*"> | lit<"/"> | lit<"%"> | lit<"^"> | lit<"!"> |         //
      lit<"="> | lit<"~"> | lit<"?"> | lit<":"> | lit<"++"> |        //
      lit<"--"> | lit<"&"> | lit<"|"> | lit<"->"> | lit<"<-"> |      //
      lit<"::"> | lit<"@"> | lit<".">;

  // A balanced group's content: ordinary tokens, `,`/`;` (meaningful only to
  // whatever encloses the group -- a for-header, an argument list), and a
  // nested group of any of the three kinds.
  struct Inner
      : rule<some(Tok | lit<","> | lit<";"> | ref<ParenGroup> |
                  ref<BracketGroup> | ref<BraceGroup>)> {};
  struct ParenGroup : rule<lit<"("> + option(ref<Inner>) + lit<")">> {};
  struct BracketGroup : rule<lit<"["> + option(ref<Inner>) + lit<"]">> {};
  struct BraceGroup : rule<lit<"{"> + option(ref<Inner>) + lit<"}">> {};

  // A name/signature run at top level: no bare `,` or `{` (those close the
  // run), balanced `(...)`/`[...]` throughout.
  struct Soup : rule<some(Tok | ref<ParenGroup> | ref<BracketGroup>)> {};
  // The same, with a bare `,` allowed -- a plain statement's own top-level
  // separator (`int a, b;`), never available inside `Soup`.
  struct StmtSoup
      : rule<some(Tok | lit<","> | ref<ParenGroup> | ref<BracketGroup>)> {};
  // Bounded to one physical line: `#include`/`#define` are preprocessor
  // lines, never structured by braces the way a statement is.
  struct LineSoup
      : rule<(Tok | ref<ParenGroup> | ref<BracketGroup>) +
             many(!gap::line + (Tok | ref<ParenGroup> | ref<BracketGroup>))> {
  };

  // --- Expressions: the one place with real operator precedence
  // (`if`/`while`/`switch` conditions, enum items). ---
  static constexpr auto AtomTok = ref<STRING> | ref<ID> | ref<INT> |
                                  ref<FLOAT> | ref<HEX> | kw<"true"> |
                                  kw<"false">;
  struct AtomRun : rule<some(AtomTok)> {};
  struct ArrayInitRun
      : rule<lit<"{"> +
             many(AtomTok | lit<","> |
                  (lit<"{"> + many(AtomTok | lit<",">) + lit<"}">)) +
             lit<"}">> {};

  // `(call Foo.bar(...)) != SUCCESS` -- a command call used for its result,
  // needing both grouping parens and a call primary, neither otherwise
  // available.
  // A dedicated, unshared run (not `Soup`): reached from inside an
  // expression, its own continuation is any infix operator -- many of them
  // `Tok` members -- which would otherwise merge into `Soup`'s shared reduce
  // action and re-open the same cross-context ambiguity fixed above.
  struct ExprCallSoup : rule<some(Tok | ref<ParenGroup> | ref<BracketGroup>)> {
  };
  struct CallExprRun : rule<kw<"call"> + ref<ExprCallSoup>> {};

  struct PrimaryExpressionRule
      : rule<create<ast::AtomExpr>(assign<&ast::AtomExpr::text>(ref<AtomRun>))  //
             | create<ast::AtomExpr>(
                   assign<&ast::AtomExpr::text>(ref<ArrayInitRun>))  //
             | create<ast::AtomExpr>(
                   assign<&ast::AtomExpr::text>(ref<CallExprRun>))  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  struct UnaryExpressionRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpressionRule>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"!">) +
                   assign<&ast::UnaryExpr::operand>(
                       ref<UnaryExpressionRule>))  //
             | ref<PrimaryExpressionRule>> {};

  // `&`/`|` (bitwise) are added at the usual C precedence, between equality
  // and the logical levels -- a guard like `if (data & 0x0004)`, ordinary
  // nesC, could not otherwise parse. `^` is bitwise XOR (K&R C, between `&`
  // and `|`), not exponentiation; `=` is assignment, loosest and right-
  // associative, not one level with `+`/`-`.
  struct Expression
      : infix<UnaryExpressionRule, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (left(lit<"*"> | lit<"/"> | lit<"%">) >>                //
               left(lit<"+"> | lit<"-">) >>                          //
               left(lit<"<="> | lit<">="> | lit<"<"> | lit<">">) >>   //
               left(lit<"=="> | lit<"!=">) >>                        //
               left(lit<"&">) >>                                     //
               left(lit<"^">) >>                                     //
               left(lit<"|">) >>                                     //
               left(lit<"&&">) >>                                    //
               left(lit<"||">) >>                                    //
               right(lit<"=">))> {};

  // --- Statements: one keyword-disjoint alternative per shape, so no two
  // ever compete for the same input; not ported rule for rule.
  // ---

  // The terminator is never optional: a signature run's own extent is
  // undecidable (LALR-ambiguous with "a new statement starts here") unless
  // every caller's continuation is disjoint from an ordinary token.
  static constexpr auto FuncBody =
      (lit<"{"> + many(append<&ast::FuncStat::body>(ref<StatRule>)) +
       lit<"}">)  //
      | lit<";">;

  // A tasklet component marks its commands/events `async`/`tasklet_async`,
  // neither with a parser rule of its own (`async` is a lexed keyword no
  // parser rule uses anywhere), so there is no dedicated slot for it:
  // per-state lexing reads it back as a
  // plain word wherever the keyword is not itself reachable (the state right
  // after `implementation {`, where none of `event`/`task`/`static`/
  // `command` is expected yet), and the whole prefixed signature falls
  // through to the keyword-less branch below as one run. A dedicated
  // optional-prefix rule was tried and measured instead: tying it to every
  // one of the four keyword branches multiplied the live GLR fork count with
  // every further `event`/`task`/`static`/`command` in the file (two such
  // functions took a corpus file from clean in under a second to a 20 s
  // timeout) -- the free per-state repair costs nothing extra.
  struct FuncStatRule
      : rule<create<ast::FuncStat>(
                 assign<&ast::FuncStat::kind>(kw<"event">) +
                 assign<&ast::FuncStat::signature>(ref<Soup>) +
                 FuncBody)  //
             | create<ast::FuncStat>(
                   assign<&ast::FuncStat::kind>(kw<"task">) +
                   assign<&ast::FuncStat::signature>(ref<Soup>) +
                   FuncBody)  //
             | create<ast::FuncStat>(
                   assign<&ast::FuncStat::kind>(kw<"static">) +
                   assign<&ast::FuncStat::signature>(ref<Soup>) +
                   FuncBody)  //
             // `command`, the module body's most basic content, forced by
             // every corpus module that provides an interface.
             | create<ast::FuncStat>(
                   assign<&ast::FuncStat::kind>(kw<"command">) +
                   assign<&ast::FuncStat::signature>(ref<Soup>) +
                   FuncBody)  //
             // A soft keyword is also a plain word wherever its own state does
             // not read it: without this guard `else { … }` is one more
             // "function" beside the else branch, and every `if … else {…}`
             // keeps two readings alive.
             | create<ast::FuncStat>(
                   !(kw<"if"> | kw<"else"> | kw<"while"> | kw<"for"> |
                     kw<"switch"> | kw<"atomic"> | kw<"enum"> | kw<"call"> |
                     kw<"signal"> | kw<"case"> | kw<"default"> |
                     kw<"typedef"> | kw<"event"> | kw<"task"> | kw<"static">) +
                   assign<&ast::FuncStat::signature>(ref<StmtSoup>) +
                   FuncBody)> {};

  struct IfBranchRule
      : rule<create<ast::IfBranch>(
                 lit<"("> + assign<&ast::IfBranch::cond>(ref<Expression>) +
                 lit<")"> +
                 ((lit<"{"> +
                   many(append<&ast::IfBranch::body>(ref<StatRule>)) +
                   lit<"}">)  //
                  | append<&ast::IfBranch::body>(ref<StatRule>)))> {};

  struct ElseBranchRule
      : rule<create<ast::IfBranch>(
                 (lit<"{"> + many(append<&ast::IfBranch::body>(ref<StatRule>)) +
                  lit<"}">)  //
                 | append<&ast::IfBranch::body>(ref<StatRule>))> {};

  struct IfStatRule
      : rule<create<ast::IfStat>(
                 kw<"if"> + append<&ast::IfStat::branches>(ref<IfBranchRule>) +
                 many(kw<"else"> + kw<"if"> +
                      append<&ast::IfStat::branches>(ref<IfBranchRule>)) +
                 option(kw<"else"> +
                        append<&ast::IfStat::branches>(ref<ElseBranchRule>)))> {
  };

  struct WhileStatRule
      : rule<create<ast::WhileStat>(
                 kw<"while"> + lit<"("> +
                 assign<&ast::WhileStat::cond>(ref<Expression>) + lit<")"> +
                 ((lit<"{"> +
                   many(append<&ast::WhileStat::body>(ref<StatRule>)) +
                   lit<"}">)  //
                  | append<&ast::WhileStat::body>(ref<StatRule>)))> {};

  struct ForStatRule
      : rule<create<ast::ForStat>(
                 kw<"for"> + lit<"("> +
                 assign<&ast::ForStat::header>(ref<Inner>) + lit<")"> +
                 ((lit<"{"> +
                   many(append<&ast::ForStat::body>(ref<StatRule>)) +
                   lit<"}">)  //
                  | append<&ast::ForStat::body>(ref<StatRule>)))> {};

  struct CaseClauseRule
      : rule<create<ast::CaseClause>(
                 ((kw<"case"> + assign<&ast::CaseClause::label>(ref<Soup>))  //
                  | flag<&ast::CaseClause::isDefault>(kw<"default">)) +
                 lit<":"> +
                 many(append<&ast::CaseClause::body>(ref<StatRule>)))> {};

  struct SwitchStatRule
      : rule<create<ast::SwitchStat>(
                 kw<"switch"> + lit<"("> +
                 assign<&ast::SwitchStat::cond>(ref<Expression>) + lit<")"> +
                 lit<"{"> +
                 many(append<&ast::SwitchStat::cases>(ref<CaseClauseRule>)) +
                 lit<"}">)> {};

  struct AtomicStatRule
      : rule<create<ast::AtomicStat>(
                 kw<"atomic"> +
                 ((lit<"{"> +
                   many(append<&ast::AtomicStat::body>(ref<StatRule>)) +
                   lit<"}">)  //
                  | append<&ast::AtomicStat::body>(ref<StatRule>)))> {};

  struct EnumStatRule
      : rule<create<ast::EnumStat>(
                 kw<"enum"> + lit<"{"> +
                 many(append<&ast::EnumStat::items>(ref<Expression>),
                      lit<",">) +
                 option(lit<",">) + lit<"}"> + lit<";">)> {};

  struct CallStatRule
      : rule<create<ast::CallStat>(
                 (kw<"call"> |
                  flag<&ast::CallStat::isSignal>(kw<"signal">)) +
                 assign<&ast::CallStat::target>(ref<Soup>) + lit<";">)> {};

  // The common, brace-less form (`#define NAME value`, one physical line);
  // the braced shape never appears in the corpus and is not ported.
  struct DefineStatRule
      : rule<create<ast::DefineStat>(
                 lit<"#"> + kw<"define"> +
                 assign<&ast::DefineStat::name>(ref<ID>) +
                 option(!gap::line +
                        assign<&ast::DefineStat::value>(ref<LineSoup>)))> {};

  struct PacketDefineRule
      : rule<create<ast::PacketDefine>(
                 kw<"typedef"> + assign<&ast::PacketDefine::header>(ref<Soup>) +
                 lit<"{"> +
                 many(append<&ast::PacketDefine::body>(ref<StatRule>)) +
                 lit<"}"> +
                 assign<&ast::PacketDefine::trailing>(ref<StatRule>))> {};

  struct UnknownCharStatRule
      : rule<create<ast::UnknownCharStat>(
            !Tok + assign<&ast::UnknownCharStat::text>(ref<OTHER>))> {};

  // The fallback comes last: a byte that also starts a statement is one.
  struct StatRule
      : rule<ref<IfStatRule> | ref<WhileStatRule> | ref<ForStatRule> |
             ref<SwitchStatRule> | ref<AtomicStatRule> | ref<EnumStatRule> |
             ref<CallStatRule> | ref<DefineStatRule> | ref<PacketDefineRule> |
             ref<FuncStatRule> | ref<UnknownCharStatRule>> {};

  // --- The nesC shell: interfaces, modules, configurations, wiring. ---

  // `ICMP_RA[uint8_t code]` -- a parameterized interface's alias, name plus
  // its bracketed parameter list.
  struct AliasRun : rule<ref<ID> + option(ref<BracketGroup>)> {};

  struct InterfaceNameAsRule
      : rule<create<ast::InterfaceRef>(
                 assign<&ast::InterfaceRef::text>(ref<Soup>) +
                 option(kw<"as"> +
                        assign<&ast::InterfaceRef::alias>(ref<AliasRun>)))> {
  };

  // The single (non-braced) form takes exactly one interface -- the corpus
  // never writes two under one bare `interface` keyword, and repeating one
  // with no separator between instances leaves Soup's own extent
  // undecidable wherever it is reached: with a real
  // repetition here, LALR merges that state into the shared `Soup`/`StmtSoup`
  // machinery used everywhere else, and the cost compounds with every further
  // statement (measured: a file with 8 module-body statements went from
  // clean to a 20 s timeout once this loop could repeat).
  struct UsesOrProvidesRule
      : rule<create<ast::UsesProvides>(
                 (kw<"uses"> |
                  flag<&ast::UsesProvides::isProvides>(kw<"provides">)) +
                 ((kw<"interface"> +
                   option(append<&ast::UsesProvides::interfaces>(
                       ref<InterfaceNameAsRule>)) +
                   lit<";">)  //
                  | (lit<"{"> +
                     many(kw<"interface"> +
                          append<&ast::UsesProvides::interfaces>(
                              ref<InterfaceNameAsRule>) +
                          lit<";">) +
                     lit<"}">)))> {};

  struct InterfaceMemberRule
      : rule<create<ast::InterfaceMember>(
                 (flag<&ast::InterfaceMember::isEvent>(kw<"event">)  //
                  | kw<"command">) +
                 assign<&ast::InterfaceMember::signature>(ref<Soup>) +
                 lit<";">)> {};

  struct InterfaceDeclRule
      : rule<create<ast::InterfaceDecl>(
                 kw<"interface"> +
                 assign<&ast::InterfaceDecl::name>(ref<ID>) + lit<"{"> +
                 many(append<&ast::InterfaceDecl::members>(
                     ref<InterfaceMemberRule>)) +
                 lit<"}">)> {};

  // `@safe()`, `@atomic_hwevent()` -- an nesC attribute, kept as text.
  struct AtSoup : rule<lit<"@"> + ref<ID> + option(ref<ParenGroup>)> {};

  struct ParameterInner : rule<some(Tok | kw<"typedef"> | lit<","> | ref<ParenGroup> |
                                    ref<BracketGroup> | ref<BraceGroup>)> {};
  struct ModuleRule
      : rule<create<ast::Module>(
            option(flag<&ast::Module::isGeneric>(kw<"generic">)) + kw<"module"> +
            assign<&ast::Module::name>(ref<ID>) +
            option(assign<&ast::Module::attribute>(ref<AtSoup>)) +
            option(flag<&ast::Module::hasParameters>(lit<"(">) +
                   option(assign<&ast::Module::parameters>(ref<ParameterInner>)) +
                   lit<")">) +
            lit<"{"> + many(append<&ast::Module::signature>(ref<UsesOrProvidesRule>)) +
            lit<"}"> + kw<"implementation"> + lit<"{"> +
            many(append<&ast::Module::body>(ref<StatRule>)) + lit<"}">)> {};

  // `new TimerMilliC() as Timer0` -- the instantiation and its alias kept
  // together as one item's text.
  struct ComponentItem
      : rule<some(Tok | ref<ParenGroup> | ref<BracketGroup>) +
             option(kw<"as"> + ref<ID>)> {};

  struct ComponentsStatRule
      : rule<create<ast::ComponentsStat>(
                 kw<"components"> +
                 some(append<&ast::ComponentsStat::names>(ref<ComponentItem>),
                      lit<",">) +
                 option(lit<",">) + lit<";">)> {};

  struct WiringStatRule
      : rule<create<ast::WiringStat>(
                 assign<&ast::WiringStat::left>(ref<Soup>) +
                 // `=` is a real nesC wiring connector (parameterized
                 // interface equating), missing here otherwise.
                 assign<&ast::WiringStat::arrow>(
                     lit<"->"> | lit<"<-"> | lit<"=">) +
                 assign<&ast::WiringStat::right>(ref<Soup>) + lit<";">)> {};

  struct ConfigItemRule
      : rule<ref<ComponentsStatRule> | ref<WiringStatRule>> {};

  // A configuration's signature holds `uses`/`provides` exactly like a
  // module's, not a bare optional expression -- forced by the corpus
  // (RPLDAORoutingEngineC.nc).
  struct ConfigurationRule
      : rule<create<ast::Configuration>(
            option(flag<&ast::Configuration::isGeneric>(kw<"generic">)) +
            kw<"configuration"> + assign<&ast::Configuration::name>(ref<ID>) +
            option(assign<&ast::Configuration::attribute>(ref<AtSoup>)) +
            option(
                flag<&ast::Configuration::hasParameters>(lit<"(">) +
                option(assign<&ast::Configuration::parameters>(ref<ParameterInner>)) +
                lit<")">) +
            lit<"{"> +
            many(append<&ast::Configuration::signature>(ref<UsesOrProvidesRule>)) +
            lit<"}"> + kw<"implementation"> + lit<"{"> +
            many(append<&ast::Configuration::items>(ref<ConfigItemRule>)) + lit<"}">)> {
  };

  struct ComponentRule
      : rule<ref<ModuleRule> | ref<ConfigurationRule> | ref<InterfaceDeclRule>> {
  };

  struct IncludeRule
      : rule<create<ast::Include>(
                 lit<"#"> + kw<"include"> +
                 assign<&ast::Include::path>(ref<LineSoup>))> {};

  struct DocumentRule
      : rule<create<ast::Document>(
                 many(append<&ast::Document::includes>(ref<IncludeRule>)) +
                 append<&ast::Document::components>(ref<ComponentRule>) +
                 option(many(append<&ast::Document::includes>(
                            ref<IncludeRule>)) +
                        append<&ast::Document::components>(
                            ref<ComponentRule>)))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace tinyosnesc
