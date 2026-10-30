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
      : terminal<option(lit<"L">) + lit<"\""> + many((lit<"\\"> + (eol | any)) | cr<R"(^"\\\r\n)">) +
                 lit<"\"">> {};
  struct CHAR : terminal<option(lit<"L">) + lit<"'"> +
      some((lit<"\\"> + (eol | any)) | cr<R"(^'\\\r\n)">) + lit<"'">> {};
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
  struct PrimaryExpression;
  struct UnaryExpression;
  struct Expression;
  struct Stat;
  struct FuncStat;
  struct IfBranch;
  struct ElseBranch;
  struct IfStat;
  struct WhileStat;
  struct ForStat;
  struct CaseClause;
  struct SwitchStat;
  struct AtomicStat;
  struct EnumStat;
  struct CallStat;
  struct DefineStat;
  struct PacketDefine;
  struct UnknownCharStat;
  struct InterfaceNameAs;
  struct UsesOrProvides;
  struct InterfaceMember;
  struct InterfaceDecl;
  struct AtSoup;
  struct AttributesRun;
  struct TypeParameter;
  struct Module;
  struct ComponentsStat;
  struct WiringStat;
  struct ConfigItem;
  struct Configuration;
  struct Component;
  struct Include;
  struct Document;

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
      ref<ID> | ref<INT> | ref<FLOAT> | ref<HEX> | ref<STRING> | ref<CHAR> | //
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
  static constexpr auto AtomTok = ref<STRING> | ref<CHAR> | ref<ID> | ref<INT> |
                                  ref<FLOAT> | ref<HEX> | kw<"true"> |
                                  kw<"false">;
  struct AtomRun : rule<some(AtomTok) + many(
      ((lit<"."> | lit<"->">) + ref<ID>) | ref<ParenGroup> | ref<BracketGroup>)> {};
  struct ArrayInitRun
      : rule<lit<"{"> +
             many(AtomTok | lit<","> |
                  (lit<"{"> + many(AtomTok | lit<",">) + lit<"}">)) +
             lit<"}">> {};

  // `(call Foo.bar(...)) != SUCCESS` -- a command call used for its result,
  // needing both grouping parens and a call primary, neither otherwise
  // available.
  // Keep expression calls separate from the raw declaration/signature runs.
  struct ExprCallSoup : rule<some(Tok | ref<ParenGroup> | ref<BracketGroup>)> {
  };
  struct CallExprRun : rule<kw<"call"> + ref<ExprCallSoup>> {};

  struct PrimaryExpression
      : rule<create<ast::AtomExpr>(assign<&ast::AtomExpr::text>(ref<AtomRun>))  //
             | create<ast::AtomExpr>(
                   assign<&ast::AtomExpr::text>(ref<ArrayInitRun>))  //
             | create<ast::AtomExpr>(
                   assign<&ast::AtomExpr::text>(ref<CallExprRun>))  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  struct UnaryExpression
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+"> | lit<"~"> | lit<"&"> | lit<"*">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"!">) +
                   assign<&ast::UnaryExpr::operand>(
                       ref<UnaryExpression>))  //
             | ref<PrimaryExpression>> {};

  // `&`/`|` (bitwise) are added at the usual C precedence, between equality
  // and the logical levels -- a guard like `if (data & 0x0004)`, ordinary
  // nesC, could not otherwise parse. `^` is bitwise XOR (K&R C, between `&`
  // and `|`), not exponentiation; `=` is assignment, loosest and right-
  // associative, not one level with `+`/`-`.
  struct Expression
      : infix<UnaryExpression, ast::BinaryExpr,
              levels(left(lit<"*"> | lit<"/"> | lit<"%">),               //
                     left(lit<"+"> | lit<"-">),                          //
                     left(lit<"<<"> | lit<">>">),                          //
                     left(lit<"<="> | lit<">="> | lit<"<"> | lit<">">),  //
                     left(lit<"=="> | lit<"!=">),                        //
                     left(lit<"&">),                                     //
                     left(lit<"^">),                                     //
                     left(lit<"|">),                                     //
                     left(lit<"&&">),                                    //
                     left(lit<"||">),                                    //
                     right(lit<"=">))> {};

  // --- Statements: one keyword-disjoint alternative per shape, so no two
  // ever compete for the same input; not ported rule for rule.
  // ---

  // The terminator is never optional: a signature run's own extent is
  // undecidable (LALR-ambiguous with "a new statement starts here") unless
  // every caller's continuation is disjoint from an ordinary token.
  static constexpr auto FuncBody =
      (lit<"{"> + many(append<&ast::FuncStat::body>(ref<Stat>)) +
       lit<"}">)  //
      | lit<";">;

  // Other storage modifiers remain in the raw signature. Only the explicit
  // leading event/task/static/command branches set the kind field.
  struct FuncStat
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

  struct IfBranch
      : rule<create<ast::IfBranch>(
                 lit<"("> + assign<&ast::IfBranch::cond>(ref<Expression>) +
                 lit<")"> +
                 ((lit<"{"> +
                   many(append<&ast::IfBranch::body>(ref<Stat>)) +
                   lit<"}">)  //
                  | append<&ast::IfBranch::body>(ref<Stat>)))> {};

  struct ElseBranch
      : rule<create<ast::IfBranch>(
                 (lit<"{"> + many(append<&ast::IfBranch::body>(ref<Stat>)) +
                  lit<"}">)  //
                 | append<&ast::IfBranch::body>(ref<Stat>))> {};

  struct IfStat
      : rule<create<ast::IfStat>(
                 kw<"if"> + append<&ast::IfStat::branches>(ref<IfBranch>) +
                 many(kw<"else"> + kw<"if"> +
                      append<&ast::IfStat::branches>(ref<IfBranch>)) +
                 option(kw<"else"> +
                        append<&ast::IfStat::branches>(ref<ElseBranch>)))> {
  };

  struct WhileStat
      : rule<create<ast::WhileStat>(
                 kw<"while"> + lit<"("> +
                 assign<&ast::WhileStat::cond>(ref<Expression>) + lit<")"> +
                 ((lit<"{"> +
                   many(append<&ast::WhileStat::body>(ref<Stat>)) +
                   lit<"}">)  //
                  | append<&ast::WhileStat::body>(ref<Stat>)))> {};

  struct ForStat
      : rule<create<ast::ForStat>(
                 kw<"for"> + lit<"("> +
                 assign<&ast::ForStat::header>(ref<Inner>) + lit<")"> +
                 ((lit<"{"> +
                   many(append<&ast::ForStat::body>(ref<Stat>)) +
                   lit<"}">)  //
                  | append<&ast::ForStat::body>(ref<Stat>)))> {};

  struct CaseClause
      : rule<create<ast::CaseClause>(
                 ((kw<"case"> + assign<&ast::CaseClause::label>(ref<Soup>))  //
                  | flag<&ast::CaseClause::isDefault>(kw<"default">)) +
                 lit<":"> +
                 many(append<&ast::CaseClause::body>(ref<Stat>)))> {};

  struct SwitchStat
      : rule<create<ast::SwitchStat>(
                 kw<"switch"> + lit<"("> +
                 assign<&ast::SwitchStat::cond>(ref<Expression>) + lit<")"> +
                 lit<"{"> +
                 many(append<&ast::SwitchStat::cases>(ref<CaseClause>)) +
                 lit<"}">)> {};

  struct AtomicStat
      : rule<create<ast::AtomicStat>(
                 kw<"atomic"> +
                 ((lit<"{"> +
                   many(append<&ast::AtomicStat::body>(ref<Stat>)) +
                   lit<"}">)  //
                  | append<&ast::AtomicStat::body>(ref<Stat>)))> {};

  struct EnumStat
      : rule<create<ast::EnumStat>(
                 kw<"enum"> + lit<"{"> +
                 many(append<&ast::EnumStat::items>(ref<Expression>),
                      lit<",">) +
                 option(lit<",">) + lit<"}"> + lit<";">)> {};

  struct CallStat
      : rule<create<ast::CallStat>(
                 (kw<"call"> |
                  flag<&ast::CallStat::isSignal>(kw<"signal">)) +
                 assign<&ast::CallStat::target>(ref<Soup>) + lit<";">)> {};

  // The common, brace-less form (`#define NAME value`, one physical line);
  // the braced shape never appears in the corpus and is not ported.
  struct DefineStat
      : rule<create<ast::DefineStat>(
                 lit<"#"> + kw<"define"> +
                 assign<&ast::DefineStat::name>(ref<ID>) +
                 option(!gap::line +
                        assign<&ast::DefineStat::value>(ref<LineSoup>)))> {};

  struct PacketDefine
      : rule<create<ast::PacketDefine>(
                 kw<"typedef"> + assign<&ast::PacketDefine::header>(ref<Soup>) +
                 lit<"{"> +
                 many(append<&ast::PacketDefine::body>(ref<Stat>)) +
                 lit<"}"> +
                 assign<&ast::PacketDefine::trailing>(ref<Stat>))> {};

  struct UnknownCharStat
      : rule<create<ast::UnknownCharStat>(
            !Tok + assign<&ast::UnknownCharStat::text>(ref<OTHER>))> {};

  // The fallback comes last: a byte that also starts a statement is one.
  struct Stat
      : rule<ref<IfStat> | ref<WhileStat> | ref<ForStat> |
             ref<SwitchStat> | ref<AtomicStat> | ref<EnumStat> |
             ref<CallStat> | ref<DefineStat> | ref<PacketDefine> |
             ref<FuncStat> | ref<UnknownCharStat>> {};

  // --- The nesC shell: interfaces, modules, configurations, wiring. ---

  // `ICMP_RA[uint8_t code]` -- a parameterized interface's alias, name plus
  // its bracketed parameter list.
  struct AliasRun : rule<ref<ID> + option(ref<BracketGroup>)> {};

  struct InterfaceNameAs
      : rule<create<ast::InterfaceRef>(
                 assign<&ast::InterfaceRef::text>(ref<Soup>) +
                 option(kw<"as"> +
                        assign<&ast::InterfaceRef::alias>(ref<AliasRun>)))> {
  };

  // Interface entries and bare command/event declarations may stand alone
  // or share a uses/provides block.
  struct UsesOrProvides
      : rule<create<ast::UsesProvides>(
                 (kw<"uses"> |
                  flag<&ast::UsesProvides::isProvides>(kw<"provides">)) +
                 ((kw<"interface"> +
                   append<&ast::UsesProvides::interfaces>(ref<InterfaceNameAs>) +
                   lit<";">)  //
                  | append<&ast::UsesProvides::members>(ref<InterfaceMember>)
                  | (lit<"{"> +
                     many((kw<"interface"> + append<&ast::UsesProvides::interfaces>(ref<InterfaceNameAs>) + lit<";">) |
                          append<&ast::UsesProvides::members>(ref<InterfaceMember>)) +
                     lit<"}">)))> {};

  struct InterfaceMember
      : rule<create<ast::InterfaceMember>(
                 option(flag<&ast::InterfaceMember::isAsync>(kw<"async">)) +
                 (flag<&ast::InterfaceMember::isEvent>(kw<"event">)  //
                  | kw<"command">) +
                 assign<&ast::InterfaceMember::signature>(ref<Soup>) +
                 lit<";">)> {};

  struct InterfaceDecl
      : rule<create<ast::InterfaceDecl>(
                 kw<"interface"> +
                 assign<&ast::InterfaceDecl::name>(ref<ID>) +
                 option(lit<"<"> + some(append<&ast::InterfaceDecl::typeParameters>(ref<TypeParameter>), lit<",">) + lit<">">) +
                 option(assign<&ast::InterfaceDecl::attributes>(ref<AttributesRun>)) + lit<"{"> +
                 many(append<&ast::InterfaceDecl::members>(
                     ref<InterfaceMember>)) +
                 lit<"}">)> {};

  // `@safe()`, `@atomic_hwevent()` -- an nesC attribute, kept as text.
  struct AtSoup : rule<lit<"@"> + ref<ID> + ref<ParenGroup>> {};
  struct AttributesRun : rule<some(ref<AtSoup>)> {};
  struct TypeParameter : rule<ref<ID> + many(ref<AtSoup>)> {};

  struct ParameterInner : rule<some(Tok | kw<"typedef"> | lit<","> | ref<ParenGroup> |
                                    ref<BracketGroup> | ref<BraceGroup>)> {};
  template<auto Has, auto Parameters>
  static constexpr auto ComponentParameters = flag<Has>(lit<"(">) +
      option(assign<Parameters>(ref<ParameterInner>)) + lit<")">;
  template<auto Has, auto Parameters, auto Attributes>
  static constexpr auto HeaderDetails = option(ComponentParameters<Has, Parameters>) +
      option(assign<Attributes>(ref<AttributesRun>));
  struct Module
      : rule<create<ast::Module>(
            option(flag<&ast::Module::isGeneric>(kw<"generic">)) + kw<"module"> +
            assign<&ast::Module::name>(ref<ID>) +
            HeaderDetails<&ast::Module::hasParameters, &ast::Module::parameters, &ast::Module::attribute> +
            lit<"{"> + many(append<&ast::Module::signature>(ref<UsesOrProvides>)) +
            lit<"}"> + kw<"implementation"> + lit<"{"> +
            many(append<&ast::Module::body>(ref<Stat>)) + lit<"}">)> {};

  // `new TimerMilliC() as Timer0` -- the instantiation and its alias kept
  // together as one item's text.
  struct ComponentItem
      : rule<some(Tok | ref<ParenGroup> | ref<BracketGroup>) +
             option(kw<"as"> + ref<ID>)> {};

  struct ComponentsStat
      : rule<create<ast::ComponentsStat>(
                 kw<"components"> +
                 some(append<&ast::ComponentsStat::names>(ref<ComponentItem>),
                      lit<",">) +
                 option(lit<",">) + lit<";">)> {};

  struct WiringStat
      : rule<create<ast::WiringStat>(
                 assign<&ast::WiringStat::left>(ref<Soup>) +
                 // `=` is a real nesC wiring connector (parameterized
                 // interface equating), missing here otherwise.
                 assign<&ast::WiringStat::arrow>(
                     lit<"->"> | lit<"<-"> | lit<"=">) +
                 assign<&ast::WiringStat::right>(ref<Soup>) + lit<";">)> {};

  struct ConfigItem
      : rule<ref<ComponentsStat> | ref<WiringStat>> {};

  // A configuration's signature holds `uses`/`provides` exactly like a
  // module's, not a bare optional expression -- forced by the corpus
  // (RPLDAORoutingEngineC.nc).
  struct Configuration
      : rule<create<ast::Configuration>(
            option(flag<&ast::Configuration::isGeneric>(kw<"generic">)) +
            kw<"configuration"> + assign<&ast::Configuration::name>(ref<ID>) +
            HeaderDetails<&ast::Configuration::hasParameters, &ast::Configuration::parameters, &ast::Configuration::attribute> +
            lit<"{"> +
            many(append<&ast::Configuration::signature>(ref<UsesOrProvides>)) +
            lit<"}"> + kw<"implementation"> + lit<"{"> +
            many(append<&ast::Configuration::items>(ref<ConfigItem>)) + lit<"}">)> {
  };

  struct Component
      : rule<ref<Module> | ref<Configuration> | ref<InterfaceDecl>> {
  };

  struct Include
      : rule<create<ast::Include>(
                 lit<"#"> + kw<"include"> +
                 assign<&ast::Include::path>(ref<LineSoup>))> {};

  struct Document
      : rule<create<ast::Document>(
                 many(append<&ast::Document::includes>(ref<Include>)) +
                 append<&ast::Document::components>(ref<Component>) +
                 option(many(append<&ast::Document::includes>(
                            ref<Include>)) +
                        append<&ast::Document::components>(
                            ref<Component>)))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace tinyosnesc
