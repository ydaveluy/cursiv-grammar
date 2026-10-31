// The python language: its grammar, covering Python 3.14's statement and
// expression surface -- suites by indentation (`indent`/`dedent`) or, where
// Python itself allows it, on the same line as the header.
#pragma once

#include <type_traits>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "python/core/ast.hpp"

namespace python {

struct Python : cursiv::GrammarSpec {
  // TRIVIA
  // ======

  static constexpr auto comment_body = lit<"#"> + many(cr<R"(^\r\n)">);
  struct COMMENT : terminal<comment_body> {};

  struct WS : terminal<some(cr<R"( \t\f)"> | (lit<"\\"> + eol))> {};

  // A line with nothing but whitespace and/or a comment does not end a
  // logical line in Python: folded into NEWLINE so the grammar never sees it.
  static constexpr auto blank_line =
      many(cr<R"( \t\f)">) + option(comment_body) + eol;
  struct NEWLINE : terminal<eol + many(blank_line)> {};
  struct LeadingTrivia : terminal<some(blank_line)> {};
  // `indent` and `dedent` are the parse's own: inside these brackets they
  // answer nothing.
  static constexpr auto indent_open = lit<"("> | lit<"["> | lit<"{">;
  static constexpr auto indent_close = lit<")"> | lit<"]"> | lit<"}">;
  using indent_newline = NEWLINE;

  // The indentation runtime suppresses NEWLINE inside indent_open/close.
  // Outside brackets a physical newline must remain a statement boundary.

  // FORWARD DECLARATIONS
  // ====================
  // Expressions and statements reach each other mutually (parens restart
  // from the top of precedence; blocks hold statements that hold
  // expressions), so every rule name is declared before any body uses it.

  template <bool Raw> struct ReplacementFieldRule;
  using ReplacementField = ReplacementFieldRule<false>;
  struct Expression;
  struct Lambda;
  struct Disjunction;
  struct Conjunction;
  struct Inversion;
  struct Comparison;
  struct BitwiseOr;
  struct Factor;
  struct Power;
  struct AwaitPrimary;
  struct Primary;
  struct Atom;
  struct StarredExpr;
  struct StarExpression;
  struct StarNamedExpression;
  struct StarExpressions;
  struct StarNamedExpressions;
  struct NamedExpression;
  struct YieldExpr;
  struct AnnotatedRhs;
  struct Tuple;
  struct Group;
  struct List;
  struct ListComp;
  struct Set;
  struct SetComp;
  struct Dict;
  struct DictComp;
  struct GenExp;
  struct CompClause;
  struct KeywordArg;
  struct SliceItem;
  struct Slices;
  struct ParamItem;
  struct Params;
  struct LambdaParam;
  struct LambdaParams;
  struct TypeParamItem;
  struct Strings;
  struct SimpleStmt;
  struct CompoundStmt;
  struct Assignment;
  struct AugAssign;
  struct AnnAssign;
  struct TypeAlias;
  struct ReturnStmt;
  struct ImportStmt;
  struct DottedName;
  struct RaiseStmt;
  struct PassStmt;
  struct DelStmt;
  struct AssertStmt;
  struct BreakStmt;
  struct ContinueStmt;
  struct GlobalStmt;
  struct NonlocalStmt;
  struct ExprStmt;
  struct ElifChain;
  struct IfStmt;
  struct WhileStmt;
  struct ForStmt;
  struct WithItem;
  struct WithStmt;
  struct ExprList;
  struct ExceptHandler;
  struct TryStmt;
  struct FunctionDef;
  struct ClassDef;
  struct Pattern;
  struct ClosedPattern;
  struct OrPattern;
  struct PatternsTop;
  struct MaybeStarPattern;
  struct StarPattern;
  struct CapturePattern;
  struct WildcardPattern;
  struct ValuePattern;
  struct GroupPattern;
  struct SequencePatternBracket;
  struct SequencePatternParen;
  struct MappingPattern;
  struct ClassPattern;
  struct KeyValuePattern;
  struct KeywordPattern;
  struct AttrChain;
  struct NameOrAttr;
  struct LiteralExprBare;
  struct LiteralPattern;
  struct ComplexNumberPattern;
  struct SignedNumber;
  struct SignedRealNumber;
  struct CaseBlock;
  struct MatchStmt;

  // A rule with no directive of its own, `some(BodyPtr, sep)`'d into the
  // caller's own body: a compound statement's same-line suite is a
  // semicolon-separated run of simple statements, ended by NEWLINE -- the
  // one alternative `block` offers besides an indented suite.
  template <auto BodyPtr>
  static constexpr auto sameLineBlock = some(append<BodyPtr>(ref<SimpleStmt>),
                                             lit<";">) +
                                        option(lit<";">) +
                                        (ref<NEWLINE> | eof);

  template <auto BodyPtr>
  static constexpr auto indentedBlock =
      ref<NEWLINE> + indent +
      some(append<BodyPtr>(ref<CompoundStmt>)  //
           | (some(append<BodyPtr>(ref<SimpleStmt>), lit<";">) +
              option(lit<";">) + (ref<NEWLINE> | eof))) +
      dedent;

  template <auto BodyPtr>
  static constexpr auto suite = sameLineBlock<BodyPtr> | indentedBlock<BodyPtr>;

  template <auto OrElsePtr>
  static constexpr auto elseBlock =
      kw<"else"> + lit<":"> + suite<OrElsePtr>;

  template <auto TypeParamsPtr>
  static constexpr auto typeParamsGroup =
      lit<"["> + some(append<TypeParamsPtr>(ref<TypeParamItem>), lit<",">) +
      option(lit<",">) + lit<"]">;

  // NAMES AND KEYWORDS
  // ==================

  static constexpr auto hard_keyword =
      kw<"False"> | kw<"None"> | kw<"True"> | kw<"and"> | kw<"as"> |
      kw<"assert"> | kw<"async"> | kw<"await"> | kw<"break"> | kw<"class"> |
      kw<"continue"> | kw<"def"> | kw<"del"> | kw<"elif"> | kw<"else"> |
      kw<"except"> | kw<"finally"> | kw<"for"> | kw<"from"> | kw<"global"> |
      kw<"if"> | kw<"import"> | kw<"in"> | kw<"is"> | kw<"lambda"> |
      kw<"nonlocal"> | kw<"not"> | kw<"or"> | kw<"pass"> | kw<"raise"> |
      kw<"return"> | kw<"try"> | kw<"while"> | kw<"with"> | kw<"yield">;
  struct ID : terminal<!hard_keyword +
      (uc::XID_Start | cr<"_">) + many(uc::XID_Continue)> {};

  // Soft keywords also read here, each its own terminal: the lexer commits
  // to one token identity for a word wherever its keyword spelling is
  // declared at all, not only in the state that uses it as a keyword, so
  // `Ident` must accept that same token too, everywhere an identifier is
  // legal -- `return type(x)` needs it exactly as much as `type X = int`
  // does. Once both the keyword's own rule and Ident can shift it, the
  // ambiguity at statement start (`type = 5` vs `type X = int`) is a
  // parser-level fork, which GLR resolves by which one completes.
  struct TypeKwTerm : terminal<kw<"type">> {};
  struct MatchKwTerm : terminal<kw<"match">> {};
  struct CaseKwTerm : terminal<kw<"case">> {};
  struct WildcardKwTerm : terminal<kw<"_">> {};
  struct Ident : rule<ref<ID> | ref<TypeKwTerm> | ref<MatchKwTerm> |
                      ref<CaseKwTerm> | ref<WildcardKwTerm>> {};

  // A capture target in a pattern: any name except the wildcard, which the
  // grammar always reads as WildcardPattern instead.
  struct NameNotWildcard : rule<!ref<WildcardKwTerm> +
      (ref<ID> | ref<TypeKwTerm> | ref<MatchKwTerm> | ref<CaseKwTerm>)> {};

  struct Name : rule<assign<&ast::Name::id>(ref<Ident>)> {};

  // NUMBERS
  // =======
  // Spelling kept verbatim (see ast.hpp): base prefix, digit separators and
  // the imaginary suffix are for a later pass to interpret.

  static constexpr auto digitpart = d + many(option(lit<"_">) + d);
  static constexpr auto numExponent = cr<"eE"> + option(cr<"+-">) + digitpart;
  static constexpr auto pointfloat =
      (option(digitpart) + lit<"."> + digitpart) | (digitpart + lit<".">);
  static constexpr auto floatnumber =
      (pointfloat + option(numExponent)) | (digitpart + numExponent);
  static constexpr auto decinteger =
      (cr<"1-9"> + many(option(lit<"_">) + d)) |
      (some(lit<"0">) + many(option(lit<"_">) + lit<"0">));
  static constexpr auto bininteger =
      lit<"0"> + cr<"bB"> + some(option(lit<"_">) + cr<"01">);
  static constexpr auto octinteger =
      lit<"0"> + cr<"oO"> + some(option(lit<"_">) + cr<"0-7">);
  static constexpr auto hexinteger =
      lit<"0"> + cr<"xX"> + some(option(lit<"_">) + cr<"0-9a-fA-F">);

  struct NUM_INT : terminal<hexinteger | bininteger | octinteger | decinteger> {
  };
  struct NUM_FLOAT : terminal<floatnumber> {};
  struct NUM_IMAG : terminal<(floatnumber | digitpart) + cr<"jJ">> {};

  struct IntLit : rule<assign<&ast::IntLit::text>(ref<NUM_INT>)> {};
  struct FloatLit : rule<assign<&ast::FloatLit::text>(ref<NUM_FLOAT>)> {};
  struct ImagLit : rule<assign<&ast::ImagLit::text>(ref<NUM_IMAG>)> {};

  // STRINGS
  // =======

  // Plain (not `ikw<>`) case folding: `ikw<>` refuses when a word character
  // follows, which is exactly the second letter of a two-letter prefix --
  // `ikw<"r"> + ikw<"b">` would refuse "rb" itself, seeing 'b' after 'r'.
  static constexpr auto short_prefix = cr<"rR"> | cr<"uU"> | cr<"bB"> |
                                       (cr<"bB"> + cr<"rR">) |
                                       (cr<"rR"> + cr<"bB">);
  static constexpr auto plain_escape = lit<"\\"> + (eol | any);
  struct STR_SHORT
      : terminal<option(short_prefix) +
                 ((lit<"'"> + many(plain_escape | cr<R"(^\\\r\n')">) + lit<"'">) |
                  (lit<"\""> + many(plain_escape | cr<R"(^\\\r\n")">) +
                   lit<"\"">))> {};
  struct STR_LONG
      : terminal<option(short_prefix) +
                 ((lit<"'''"> +
                   many(plain_escape | (!lit<"'''"> + cr<R"(^\\)">)) +
                   lit<"'''">) |
                  (lit<"\"\"\""> +
                   many(plain_escape | (!lit<"\"\"\""> + cr<R"(^\\)">)) +
                   lit<"\"\"\"">))> {};

  struct Str : rule<assign<&ast::Str::text>(ref<STR_SHORT> | ref<STR_LONG>)> {
  };

  // F-STRINGS AND T-STRINGS
  // =======================
  static constexpr auto fPrefix = cr<"fF">;
  static constexpr auto tPrefix = cr<"tT">;
  static constexpr auto rawFPrefix =
      (cr<"fF"> + cr<"rR">) | (cr<"rR"> + cr<"fF">);
  static constexpr auto rawTPrefix =
      (cr<"tT"> + cr<"rR">) | (cr<"rR"> + cr<"tT">);
  static constexpr auto i_escape =
      lit<"\\"> + (eol | (!cr<"{}"> + any) | &cr<"{}">);
  static constexpr auto named_escape = lit<"\\N{"> + many(cr<"^}">) + lit<"}">;

  struct FStartSQ1 : terminal<fPrefix + lit<"'">> {};
  struct FStartDQ1 : terminal<fPrefix + lit<"\"">> {};
  struct FStartSQ3 : terminal<fPrefix + lit<"'''">> {};
  struct FStartDQ3 : terminal<fPrefix + lit<"\"\"\"">> {};
  struct TStartSQ1 : terminal<tPrefix + lit<"'">> {};
  struct TStartDQ1 : terminal<tPrefix + lit<"\"">> {};
  struct TStartSQ3 : terminal<tPrefix + lit<"'''">> {};
  struct TStartDQ3 : terminal<tPrefix + lit<"\"\"\"">> {};
  struct RawFStartSQ1 : terminal<rawFPrefix + lit<"'">> {};
  struct RawFStartDQ1 : terminal<rawFPrefix + lit<"\"">> {};
  struct RawFStartSQ3 : terminal<rawFPrefix + lit<"'''">> {};
  struct RawFStartDQ3 : terminal<rawFPrefix + lit<"\"\"\"">> {};
  struct RawTStartSQ1 : terminal<rawTPrefix + lit<"'">> {};
  struct RawTStartDQ1 : terminal<rawTPrefix + lit<"\"">> {};
  struct RawTStartSQ3 : terminal<rawTPrefix + lit<"'''">> {};
  struct RawTStartDQ3 : terminal<rawTPrefix + lit<"\"\"\"">> {};

  static constexpr auto rawSQ1 =
      lit<"{{"> | lit<"}}"> | i_escape | cr<R"(^\\{}'\r\n)">;
  struct RawMidSQ1 : terminal<some(rawSQ1)> {};
  struct MidSQ1 : terminal<some(named_escape | rawSQ1)> {};
  static constexpr auto rawDQ1 =
      lit<"{{"> | lit<"}}"> | i_escape | cr<R"(^\\{}"\r\n)">;
  struct RawMidDQ1 : terminal<some(rawDQ1)> {};
  struct MidDQ1 : terminal<some(named_escape | rawDQ1)> {};
  static constexpr auto rawSQ3 =
      lit<"{{"> | lit<"}}"> | i_escape | (!lit<"'''"> + cr<R"(^\\{})">);
  struct RawMidSQ3 : terminal<some(rawSQ3)> {};
  struct MidSQ3 : terminal<some(named_escape | rawSQ3)> {};
  static constexpr auto rawDQ3 =
      lit<"{{"> | lit<"}}"> | i_escape | (!lit<"\"\"\""> + cr<R"(^\\{})">);
  struct RawMidDQ3 : terminal<some(rawDQ3)> {};
  struct MidDQ3 : terminal<some(named_escape | rawDQ3)> {};
  struct CONVERSION : terminal<cr<"sra">> {};

  template <bool Raw>
  static constexpr auto replPart =
      ref<ReplacementFieldRule<Raw>> |
      ref<std::conditional_t<Raw, RawMidSQ1, MidSQ1>> |
      ref<std::conditional_t<Raw, RawMidDQ1, MidDQ1>> |
      ref<std::conditional_t<Raw, RawMidSQ3, MidSQ3>> |
      ref<std::conditional_t<Raw, RawMidDQ3, MidDQ3>>;

  template <bool Raw>
  struct ReplacementFieldRule
      : rule<create<ast::ReplField>(
            lit<"{"> + assign<&ast::ReplField::value>(ref<AnnotatedRhs>) +
            option(flag<&ast::ReplField::debug>(lit<"=">)) +
            option(tight(lit<"!"> +
                assign<&ast::ReplField::conversion>(ref<CONVERSION>))) +
            option(lit<":"> +
                   many(append<&ast::ReplField::formatSpec>(replPart<Raw>))) +
            lit<"}">)> {};

  template <auto Start, auto RawStart, class Mid, class RawMid, auto End>
  static constexpr auto interpolatedString =
      ((Start +
        many(append<&ast::IStr::parts>(ref<ReplacementField> | ref<Mid>))) |
       (flag<&ast::IStr::isRaw>(RawStart) +
        many(append<&ast::IStr::parts>(ref<ReplacementFieldRule<true>> |
                                       ref<RawMid>)))) +
      End;

  struct FStringSQ1 : rule<interpolatedString<ref<FStartSQ1>, ref<RawFStartSQ1>,
                                              MidSQ1, RawMidSQ1, lit<"'">>> {};
  struct FStringDQ1 : rule<interpolatedString<ref<FStartDQ1>, ref<RawFStartDQ1>,
                                              MidDQ1, RawMidDQ1, lit<"\"">>> {};
  struct FStringSQ3 : rule<interpolatedString<ref<FStartSQ3>, ref<RawFStartSQ3>,
                                              MidSQ3, RawMidSQ3, lit<"'''">>> {
  };
  struct FStringDQ3
      : rule<interpolatedString<ref<FStartDQ3>, ref<RawFStartDQ3>, MidDQ3,
                                RawMidDQ3, lit<"\"\"\"">>> {};
  struct TStringSQ1 : rule<interpolatedString<
                          flag<&ast::IStr::isTemplate>(ref<TStartSQ1>),
                          flag<&ast::IStr::isTemplate>(ref<RawTStartSQ1>),
                          MidSQ1, RawMidSQ1, lit<"'">>> {};
  struct TStringDQ1 : rule<interpolatedString<
                          flag<&ast::IStr::isTemplate>(ref<TStartDQ1>),
                          flag<&ast::IStr::isTemplate>(ref<RawTStartDQ1>),
                          MidDQ1, RawMidDQ1, lit<"\"">>> {};
  struct TStringSQ3 : rule<interpolatedString<
                          flag<&ast::IStr::isTemplate>(ref<TStartSQ3>),
                          flag<&ast::IStr::isTemplate>(ref<RawTStartSQ3>),
                          MidSQ3, RawMidSQ3, lit<"'''">>> {};
  struct TStringDQ3 : rule<interpolatedString<
                          flag<&ast::IStr::isTemplate>(ref<TStartDQ3>),
                          flag<&ast::IStr::isTemplate>(ref<RawTStartDQ3>),
                          MidDQ3, RawMidDQ3, lit<"\"\"\"">>> {};

  static constexpr auto AnyFString =
      ref<FStringSQ1> | ref<FStringDQ1> | ref<FStringSQ3> | ref<FStringDQ3>;
  static constexpr auto AnyTString =
      ref<TStringSQ1> | ref<TStringDQ1> | ref<TStringSQ3> | ref<TStringDQ3>;

  // Adjacent literals concatenate into one node, one piece even when there is
  // only one: `"a" "b"` and `f"x"` are both a Strings of their own pieces.
  struct Strings
      : rule<some(append<&ast::Strings::pieces>(ref<Str> | AnyFString)) |
             some(append<&ast::Strings::pieces>(AnyTString))> {};

  // EXPRESSIONS: ATOMS AND POSTFIX CHAIN
  // =====================================

  struct Atom : rule<
      ref<Name>                                                        //
      | create<ast::BoolLit>(flag<&ast::BoolLit::value>(kw<"True">))  //
      | create<ast::BoolLit>(kw<"False">)                                 //
      | create<ast::NoneLit>(kw<"None">)                                 //
      | ref<Strings>                                                    //
      | ref<FloatLit>                                                   //
      | ref<ImagLit>                                                    //
      | ref<IntLit>                                                     //
      | ref<Tuple> | ref<GenExp> | ref<Group>                        //
      | ref<List> | ref<ListComp>                                   //
      | ref<Dict> | ref<Set> | ref<DictComp> | ref<SetComp>     //
      | create<ast::EllipsisLit>(lit<"...">)> {};

  struct Primary : rule<
      ref<Atom> +
      many(
          nest<ast::Attribute, &ast::Attribute::value>(
              lit<"."> + assign<&ast::Attribute::attr>(ref<Ident>))  //
          | nest<ast::CallExpr, &ast::CallExpr::func>(
                append<&ast::CallExpr::args>(ref<GenExp>))  //
          | nest<ast::CallExpr, &ast::CallExpr::func>(
                lit<"("> +
                option(some(append<&ast::CallExpr::keywords>(ref<KeywordArg>) |
                                append<&ast::CallExpr::args>(ref<StarredExpr>) |
                                append<&ast::CallExpr::args>(
                                    ref<NamedExpression>),
                            lit<",">) +
                       option(lit<",">)) +
                lit<")">)  //
          | nest<ast::Subscript, &ast::Subscript::value>(
                lit<"["> + assign<&ast::Subscript::slice>(ref<Slices>) + lit<"]">))> {};

  struct AwaitPrimary : rule<
      create<ast::AwaitExpr>(kw<"await"> +
                             assign<&ast::AwaitExpr::value>(ref<Primary>))  //
      | ref<Primary>> {};

  struct Power : rule<
      create<ast::BinOp>(assign<&ast::BinOp::left>(ref<AwaitPrimary>) +
                         assign<&ast::BinOp::op>(lit<"**">) +
                         assign<&ast::BinOp::right>(ref<Factor>))  //
      | ref<AwaitPrimary>> {};

  struct Factor : rule<
      create<ast::UnaryOp>(assign<&ast::UnaryOp::op>(lit<"+"> | lit<"-"> |
                                                      lit<"~">) +
                           assign<&ast::UnaryOp::operand>(ref<Factor>))  //
      | ref<Power>> {};

  // PRECEDENCE TABLE A: term .. bitwise_or
  // =======================================

  static constexpr auto TermOp =
      lit<"*"> | lit<"//"> | lit<"/"> | lit<"%"> | lit<"@">;
  static constexpr auto SumOp = lit<"+"> | lit<"-">;
  static constexpr auto ShiftOp = lit<"<<"> | lit<">>">;
  static constexpr auto BitAndOp = lit<"&">;
  static constexpr auto BitXorOp = lit<"^">;
  static constexpr auto BitOrOp = lit<"|">;

  struct BitwiseOr : infix<Factor, ast::BinOp,
                           levels(left(TermOp), left(SumOp), left(ShiftOp),
                                  left(BitAndOp), left(BitXorOp),
                                  left(BitOrOp))> {};

  // COMPARISON AND NOT
  // ===================
  // Chained (`a < b <= c`) is one node: a left-associative tree would change
  // its meaning, so this is hand-written rather than another infix level.

  struct NotInOp : rule<kw<"not"> + kw<"in">> {};
  struct IsNotOp : rule<kw<"is"> + kw<"not">> {};
  static constexpr auto CompareOp = lit<"=="> | lit<"!="> | lit<"<="> |
                                    lit<"<"> | lit<">="> | lit<">"> |
                                    ref<NotInOp> | kw<"in"> | ref<IsNotOp> |
                                    kw<"is">;

  struct Comparison : rule<
      create<ast::CompareExpr>(
          assign<&ast::CompareExpr::left>(ref<BitwiseOr>) +
          some(append<&ast::CompareExpr::ops>(CompareOp) +
               append<&ast::CompareExpr::comparators>(ref<BitwiseOr>)))  //
      | ref<BitwiseOr>> {};

  struct Inversion : rule<
      create<ast::UnaryOp>(assign<&ast::UnaryOp::op>(kw<"not">) +
                           assign<&ast::UnaryOp::operand>(ref<Inversion>))  //
      | ref<Comparison>> {};

  // PRECEDENCE TABLE B: and, or
  // ============================

  static constexpr auto AndOp = kw<"and">;
  static constexpr auto OrOp = kw<"or">;

  struct Disjunction : infix<Inversion, ast::BinOp,
                             levels(left(AndOp), left(OrOp))> {};

  // TOP-LEVEL EXPRESSION
  // ====================

  struct Lambda : rule<create<ast::Lambda>(
      kw<"lambda"> + assign<&ast::Lambda::params>(ref<LambdaParams>) +
      lit<":"> + assign<&ast::Lambda::body>(ref<Expression>))> {};

  struct Expression : rule<
      create<ast::CondExpr>(
          assign<&ast::CondExpr::body>(ref<Disjunction>) + kw<"if"> +
          assign<&ast::CondExpr::test>(ref<Disjunction>) + kw<"else"> +
          assign<&ast::CondExpr::orelse>(ref<Expression>))  //
      | ref<Lambda>  //
      | ref<Disjunction>> {};

  struct NamedExprNode : rule<create<ast::NamedExpr>(
      assign<&ast::NamedExpr::target>(ref<Name>) + lit<":="> +
      assign<&ast::NamedExpr::value>(ref<Expression>))> {};

  struct NamedExpression : rule<ref<NamedExprNode> | ref<Expression>> {};

  struct StarredExpr : rule<create<ast::StarredExpr>(
      lit<"*"> + assign<&ast::StarredExpr::value>(ref<Expression>))> {};

  struct StarExpression : rule<ref<StarredExpr> | ref<Expression>> {};
  struct StarNamedExpression : rule<ref<StarredExpr> | ref<NamedExpression>> {};

  // A comma list that adopts a single element bare, and wraps a TupleExpr
  // around it the moment a comma is written -- `x = 1` is an int, `x = 1,`
  // is a 1-tuple.
  struct StarExpressions : rule<
      create<ast::TupleExpr>(
          append<&ast::TupleExpr::elts>(ref<StarExpression>) + lit<","> +
          option(some(append<&ast::TupleExpr::elts>(ref<StarExpression>),
                      lit<",">) + option(lit<",">)))  //
      | ref<StarExpression>> {};

  struct StarNamedExpressions : rule<
      create<ast::TupleExpr>(
          append<&ast::TupleExpr::elts>(ref<StarNamedExpression>) + lit<","> +
          option(some(append<&ast::TupleExpr::elts>(ref<StarNamedExpression>),
                      lit<",">) + option(lit<",">)))
      | ref<StarNamedExpression>> {};

  // The target of a `for`: operands below the comparisons, so the `in` that
  // follows it is the clause's own and never a comparison's.
  struct StarTarget : rule<
      create<ast::StarredExpr>(lit<"*"> +
                               assign<&ast::StarredExpr::value>(ref<BitwiseOr>))  //
      | ref<BitwiseOr>> {};

  struct TargetList : rule<
      create<ast::TupleExpr>(
          append<&ast::TupleExpr::elts>(ref<StarTarget>) + lit<","> +
          option(some(append<&ast::TupleExpr::elts>(ref<StarTarget>), lit<",">) +
                 option(lit<",">)))  //
      | ref<StarTarget>> {};

  struct YieldExpr : rule<
      (kw<"yield"> + flag<&ast::YieldExpr::isFrom>(kw<"from">) +
       assign<&ast::YieldExpr::value>(ref<Expression>))  //
      | (kw<"yield"> +
         option(assign<&ast::YieldExpr::value>(ref<StarExpressions>)))> {};

  struct AnnotatedRhs : rule<ref<YieldExpr> | ref<StarExpressions>> {};

  struct ExprStmt : rule<assign<&ast::ExprStmt::value>(
      ref<YieldExpr> | ref<StarExpressions>)> {};

  // CONTAINERS AND COMPREHENSIONS
  // ==============================

  struct CompClause : rule<
      option(flag<&ast::Comprehension::isAsync>(kw<"async">)) +
      kw<"for"> + assign<&ast::Comprehension::target>(ref<TargetList>) +
      kw<"in"> + assign<&ast::Comprehension::iter>(ref<Disjunction>) +
      many(kw<"if"> + append<&ast::Comprehension::ifs>(ref<Disjunction>))> {};

  struct GenExp : rule<
      lit<"("> +
      assign<&ast::GeneratorExp::elt>(ref<NamedExprNode> | ref<Expression>) +
      some(append<&ast::GeneratorExp::generators>(ref<CompClause>)) +
      lit<")">> {};

  struct Tuple : rule<
      lit<"("> +
      option(append<&ast::TupleExpr::elts>(ref<StarNamedExpression>) +
             lit<","> +
             option(some(append<&ast::TupleExpr::elts>(ref<StarNamedExpression>),
                         lit<",">) + option(lit<",">))) +
      lit<")">> {};

  // A parenthesized expression: adopts what it wraps, same as examples/c's
  // PrimaryExpression -- no node of its own.
  struct Group : rule<
      lit<"("> + (ref<YieldExpr> | ref<NamedExpression>) +
      lit<")">> {};

  struct List : rule<
      lit<"["> +
      option(some(append<&ast::ListExpr::elts>(ref<StarNamedExpression>), lit<",">) +
             option(lit<",">)) + lit<"]">> {};

  struct ListComp : rule<
      lit<"["> + assign<&ast::ListComp::elt>(ref<NamedExpression>) +
      some(append<&ast::ListComp::generators>(ref<CompClause>)) +
      lit<"]">> {};

  struct Set : rule<
      lit<"{"> +
      some(append<&ast::SetExpr::elts>(ref<StarNamedExpression>), lit<",">) +
      option(lit<",">) + lit<"}">> {};

  struct SetComp : rule<
      lit<"{"> + assign<&ast::SetComp::elt>(ref<NamedExpression>) +
      some(append<&ast::SetComp::generators>(ref<CompClause>)) +
      lit<"}">> {};

  struct DictItem : rule<
      create<ast::DictItem>(lit<"**"> +
                            assign<&ast::DictItem::value>(ref<BitwiseOr>))  //
      | create<ast::DictItem>(
            assign<&ast::DictItem::key>(ref<Expression>) + lit<":"> +
            assign<&ast::DictItem::value>(ref<Expression>))> {};

  struct Dict : rule<
      lit<"{"> +
      option(some(append<&ast::DictExpr::items>(ref<DictItem>),
                  lit<",">) +
             option(lit<",">)) + lit<"}">> {};

  struct DictComp : rule<
      lit<"{"> + assign<&ast::DictComp::key>(ref<Expression>) +
      lit<":"> + assign<&ast::DictComp::value>(ref<Expression>) +
      some(append<&ast::DictComp::generators>(ref<CompClause>)) +
      lit<"}">> {};

  // SLICES
  // ======

  struct SliceItem : rule<
      create<ast::SliceExpr>(
          option(assign<&ast::SliceExpr::lower>(ref<Expression>)) + lit<":"> +
          option(assign<&ast::SliceExpr::upper>(ref<Expression>)) +
          option(lit<":"> +
                 option(assign<&ast::SliceExpr::step>(ref<Expression>))))  //
      | ref<StarredExpr>  //
      | ref<NamedExpression>> {};

  struct Slices : rule<
      create<ast::TupleExpr>(
          append<&ast::TupleExpr::elts>(ref<SliceItem>) + lit<","> +
          option(some(append<&ast::TupleExpr::elts>(ref<SliceItem>), lit<",">) +
                 option(lit<",">)))  //
      | ref<SliceItem>> {};

  // CALL ARGUMENTS
  // ==============

  struct KeywordArg : rule<
      create<ast::Keyword>(assign<&ast::Keyword::arg>(ref<Ident>) + lit<"="> +
                           assign<&ast::Keyword::value>(ref<Expression>))  //
      | create<ast::Keyword>(lit<"**"> +
                             assign<&ast::Keyword::value>(ref<Expression>))> {
  };

  // Parameter sections keep positional-only, ordinary and keyword-only
  // arguments separate, including the two variadic parameters.

  struct ParamItem : rule<
      assign<&ast::Param::name>(ref<Ident>) +
      option(lit<":"> + assign<&ast::Param::annotation>(ref<StarredExpr> | ref<Expression>)) +
      option(lit<"="> + assign<&ast::Param::defaultValue>(ref<Expression>))> {
  };

  // A lambda's parameters carry no annotation: its `:` is always the body's,
  // so `lambda a: lambda a: ...` is read one way.
  struct LambdaParam : rule<
      assign<&ast::Param::name>(ref<Ident>) +
      option(lit<"="> + assign<&ast::Param::defaultValue>(ref<Expression>))> {
  };

  // Separators belong to nonempty sections. A bare * requires at least one
  // keyword-only parameter; a final comma is legal after any complete list.
  template <class Item>
  static constexpr auto Kwargs = lit<"**"> + assign<&ast::Params::kwarg>(ref<Item>);

  template <class Item>
  static constexpr auto StarParams =
      (lit<"*"> + assign<&ast::Params::vararg>(ref<Item>) +
       many(lit<","> + append<&ast::Params::kwonly>(ref<Item>)) +
       option(lit<","> + Kwargs<Item>)) |
      (lit<"*"> + lit<","> +
       some(append<&ast::Params::kwonly>(ref<Item>), lit<",">) +
       option(lit<","> + Kwargs<Item>)) | Kwargs<Item>;

  template <class Item>
  static constexpr auto RegularParams =
      (some(append<&ast::Params::args>(ref<Item>), lit<",">) +
       option(lit<","> + StarParams<Item>)) | StarParams<Item>;

  template <class Item>
  static constexpr auto ParamsOf = option(
      ((some(append<&ast::Params::posonly>(ref<Item>), lit<",">) +
        lit<","> + lit<"/"> + option(lit<","> + RegularParams<Item>)) |
       RegularParams<Item>) + option(lit<",">));

  struct Params : rule<create<ast::Params>(ParamsOf<ParamItem>)> {};
  struct LambdaParams : rule<create<ast::Params>(ParamsOf<LambdaParam>)> {};

  // TYPE PARAMETERS (PEP 695)
  // ==========================

  struct TypeParamItem : rule<
      create<ast::TypeParam>(
          flag<&ast::TypeParam::isParamSpec>(lit<"**">) +
          assign<&ast::TypeParam::name>(ref<Ident>) +
          option(lit<"="> +
                 assign<&ast::TypeParam::defaultValue>(ref<Expression>)))  //
      | create<ast::TypeParam>(
            flag<&ast::TypeParam::isVarTuple>(lit<"*">) +
            assign<&ast::TypeParam::name>(ref<Ident>) +
            option(lit<"="> +
                   assign<&ast::TypeParam::defaultValue>(ref<StarExpression>)))  //
      | create<ast::TypeParam>(
            assign<&ast::TypeParam::name>(ref<Ident>) +
            option(lit<":"> + assign<&ast::TypeParam::bound>(ref<Expression>)) +
            option(lit<"="> +
                   assign<&ast::TypeParam::defaultValue>(ref<Expression>)))> {
  };

  // SIMPLE STATEMENTS
  // =================

  struct AnnAssign : rule<create<ast::AnnAssignStmt>(
      assign<&ast::AnnAssignStmt::target>(ref<Expression>) + lit<":"> +
      assign<&ast::AnnAssignStmt::annotation>(ref<Expression>) +
      option(lit<"="> +
             assign<&ast::AnnAssignStmt::value>(ref<AnnotatedRhs>)))> {};

  static constexpr auto AugOp = lit<"+="> | lit<"-="> | lit<"*="> |
                                lit<"@="> | lit<"/="> | lit<"%="> |
                                lit<"&="> | lit<"|="> | lit<"^="> |
                                lit<"<<="> | lit<">>="> | lit<"**="> |
                                lit<"//=">;

  struct AugAssign : rule<create<ast::AugAssignStmt>(
      assign<&ast::AugAssignStmt::target>(ref<Expression>) +
      assign<&ast::AugAssignStmt::op>(AugOp) +
      assign<&ast::AugAssignStmt::value>(ref<AnnotatedRhs>))> {};

  struct Assignment : rule<create<ast::AssignStmt>(
      some(append<&ast::AssignStmt::targets>(ref<StarExpressions>) +
           lit<"=">) +
      assign<&ast::AssignStmt::value>(ref<AnnotatedRhs>))> {};

  // `type` is a soft keyword, kept as `kw<>`, not guarded: a guard can
  // only test one class right after the match, not skip spaces to look
  // further ahead, so `type = 5` vs `type X = ...` cannot be told apart
  // lexically.
  struct TypeAlias : rule<create<ast::TypeAliasStmt>(
      ref<TypeKwTerm> + assign<&ast::TypeAliasStmt::name>(ref<Ident>) +
      option(typeParamsGroup<&ast::TypeAliasStmt::typeParams>) + lit<"="> +
      assign<&ast::TypeAliasStmt::value>(ref<Expression>))> {};

  struct ReturnStmt : rule<create<ast::ReturnStmt>(
      kw<"return"> +
      option(assign<&ast::ReturnStmt::value>(ref<StarExpressions>)))> {};

  struct DottedName : rule<some(ref<Ident>, lit<".">)> {};

  struct ImportAliasFromDotted : rule<
      assign<&ast::ImportAlias::name>(ref<DottedName>) +
      option(kw<"as"> + assign<&ast::ImportAlias::asName>(ref<Ident>))> {};

  struct ImportAliasPlain : rule<
      assign<&ast::ImportAlias::name>(ref<Ident>) +
      option(kw<"as"> + assign<&ast::ImportAlias::asName>(ref<Ident>))> {};

  struct ImportName : rule<create<ast::ImportStmt>(
      kw<"import"> +
      some(append<&ast::ImportStmt::names>(ref<ImportAliasFromDotted>),
           lit<",">))> {};

  struct Dots : rule<some(lit<"..."> | lit<".">)> {};

  struct ImportFrom : rule<create<ast::ImportFromStmt>(
      kw<"from"> +
      ((option(assign<&ast::ImportFromStmt::dots>(ref<Dots>)) +
        assign<&ast::ImportFromStmt::module>(ref<DottedName>))  //
       | assign<&ast::ImportFromStmt::dots>(ref<Dots>)) +
      kw<"import"> +
      (flag<&ast::ImportFromStmt::isStar>(lit<"*">)  //
       | (lit<"("> +
          some(append<&ast::ImportFromStmt::names>(ref<ImportAliasPlain>),
               lit<",">) +
          option(lit<",">) + lit<")">)  //
       | some(append<&ast::ImportFromStmt::names>(ref<ImportAliasPlain>),
              lit<",">)))> {};

  struct ImportStmt : rule<ref<ImportName> | ref<ImportFrom>> {};

  struct RaiseStmt : rule<create<ast::RaiseStmt>(
      kw<"raise"> +
      option(assign<&ast::RaiseStmt::exc>(ref<Expression>) +
             option(kw<"from"> +
                    assign<&ast::RaiseStmt::cause>(ref<Expression>))))> {};

  struct PassStmt : rule<create<ast::PassStmt>(kw<"pass">)> {};
  struct BreakStmt : rule<create<ast::BreakStmt>(kw<"break">)> {};
  struct ContinueStmt : rule<create<ast::ContinueStmt>(kw<"continue">)> {};

  struct DelStmt : rule<create<ast::DelStmt>(
      kw<"del"> +
      some(append<&ast::DelStmt::targets>(ref<StarExpression>), lit<",">) +
      option(lit<",">))> {};

  struct AssertStmt : rule<create<ast::AssertStmt>(
      kw<"assert"> + assign<&ast::AssertStmt::test>(ref<Expression>) +
      option(lit<","> + assign<&ast::AssertStmt::msg>(ref<Expression>)))> {};

  struct GlobalStmt : rule<create<ast::GlobalStmt>(
      kw<"global"> +
      some(append<&ast::GlobalStmt::names>(ref<Ident>), lit<",">))> {};

  struct NonlocalStmt : rule<create<ast::NonlocalStmt>(
      kw<"nonlocal"> +
      some(append<&ast::NonlocalStmt::names>(ref<Ident>), lit<",">))> {};

  struct SimpleStmt : rule<
      ref<Assignment> | ref<AnnAssign> | ref<AugAssign> |
      ref<TypeAlias> | ref<ExprStmt> | ref<ReturnStmt> |
      ref<ImportStmt> | ref<RaiseStmt> | ref<PassStmt> |
      ref<DelStmt> | ref<AssertStmt> | ref<BreakStmt> |
      ref<ContinueStmt> | ref<GlobalStmt> | ref<NonlocalStmt>> {};

  // COMPOUND STATEMENTS
  // ===================

  static constexpr auto DecoratorItem = lit<"@"> + ref<NamedExpression> + ref<NEWLINE>;

  struct ElifChain : rule<
      kw<"elif"> + assign<&ast::IfStmt::test>(ref<NamedExpression>) +
      lit<":"> + suite<&ast::IfStmt::body> +
      option(append<&ast::IfStmt::orelse>(ref<ElifChain>) |
             elseBlock<&ast::IfStmt::orelse>)> {};

  struct IfStmt : rule<
      kw<"if"> + assign<&ast::IfStmt::test>(ref<NamedExpression>) +
      lit<":"> + suite<&ast::IfStmt::body> +
      option(append<&ast::IfStmt::orelse>(ref<ElifChain>) |
             elseBlock<&ast::IfStmt::orelse>)> {};

  struct WhileStmt : rule<
      kw<"while"> + assign<&ast::WhileStmt::test>(ref<NamedExpression>) +
      lit<":"> + suite<&ast::WhileStmt::body> +
      option(elseBlock<&ast::WhileStmt::orelse>)> {};

  struct ForStmt : rule<
      option(flag<&ast::ForStmt::isAsync>(kw<"async">)) + kw<"for"> +
      assign<&ast::ForStmt::target>(ref<TargetList>) + kw<"in"> +
      assign<&ast::ForStmt::iter>(ref<StarExpressions>) + lit<":"> +
      suite<&ast::ForStmt::body> +
      option(elseBlock<&ast::ForStmt::orelse>)> {};

  struct WithItem : rule<
      assign<&ast::WithItem::contextExpr>(ref<Expression>) +
      option(kw<"as"> + assign<&ast::WithItem::optionalVars>(ref<Expression>))> {
  };

  struct WithStmt : rule<
      option(flag<&ast::WithStmt::isAsync>(kw<"async">)) + kw<"with"> +
      ((lit<"("> +
        some(append<&ast::WithStmt::items>(ref<WithItem>), lit<",">) +
        option(lit<",">) + lit<")">)  //
       | some(append<&ast::WithStmt::items>(ref<WithItem>), lit<",">)) +
      lit<":"> + suite<&ast::WithStmt::body>> {};

  // `except A, B:` and `except* A, B:` -- an unparenthesized exception list
  // (PEP 758) -- reuse the same "adopt bare, wrap in TupleExpr once a comma
  // appears" shape as every other comma list here.
  struct ExprList : rule<
      create<ast::TupleExpr>(
          append<&ast::TupleExpr::elts>(ref<Expression>) + lit<","> +
          option(some(append<&ast::TupleExpr::elts>(ref<Expression>), lit<",">) +
                 option(lit<",">)))> {};

  struct ExceptHandler : rule<
      kw<"except"> + option(flag<&ast::ExceptHandler::isStar>(lit<"*">)) +
      option((assign<&ast::ExceptHandler::type>(ref<Expression>) +
              option(kw<"as"> + assign<&ast::ExceptHandler::name>(ref<Ident>))) |
             assign<&ast::ExceptHandler::type>(ref<ExprList>)) +
      lit<":"> + suite<&ast::ExceptHandler::body>> {};

  struct TryStmt : rule<
      kw<"try"> + lit<":"> + suite<&ast::TryStmt::body> +
      ((some(append<&ast::TryStmt::handlers>(ref<ExceptHandler>)) +
        option(elseBlock<&ast::TryStmt::orelse>) +
        option(kw<"finally"> + lit<":"> +
               suite<&ast::TryStmt::finalBody>))  //
       | (kw<"finally"> + lit<":"> +
          suite<&ast::TryStmt::finalBody>))> {};

  struct FunctionDef : rule<
      many(append<&ast::FunctionDef::decorators>(DecoratorItem)) +
      option(flag<&ast::FunctionDef::isAsync>(kw<"async">)) +
      kw<"def"> + assign<&ast::FunctionDef::name>(ref<Ident>) +
      option(typeParamsGroup<&ast::FunctionDef::typeParams>) +
      lit<"("> +
      assign<&ast::FunctionDef::params>(ref<Params>) +
      lit<")"> +
      option(lit<"->"> + assign<&ast::FunctionDef::returns>(ref<Expression>)) +
      lit<":"> + suite<&ast::FunctionDef::body>> {};

  struct ClassDef : rule<
      many(append<&ast::ClassDef::decorators>(DecoratorItem)) + kw<"class"> +
      assign<&ast::ClassDef::name>(ref<Ident>) +
      option(typeParamsGroup<&ast::ClassDef::typeParams>) +
      option(
          lit<"("> +
          option(some(append<&ast::ClassDef::keywords>(ref<KeywordArg>) |
                          append<&ast::ClassDef::bases>(ref<StarredExpr> | ref<Expression>),
                      lit<",">) +
                 option(lit<",">)) +
          lit<")">) +
      lit<":"> + suite<&ast::ClassDef::body>> {};

  // MATCH STATEMENT AND PATTERNS
  // =============================

  // `attr`: a dotted chain with at least one `.` -- what tells value_pattern
  // (`Color.RED`) apart from a bare capture_pattern (`Color`).
  struct AttrChain : rule<
      ref<Name> +
      some(nest<ast::Attribute, &ast::Attribute::value>(
          lit<"."> + assign<&ast::Attribute::attr>(ref<Ident>)))> {};

  // `name_or_attr`: the same chain, zero dots allowed -- a class pattern's
  // own name.
  struct NameOrAttr : rule<
      ref<Name> +
      many(nest<ast::Attribute, &ast::Attribute::value>(
          lit<"."> + assign<&ast::Attribute::attr>(ref<Ident>)))> {};

  struct SignedRealNumber : rule<
      create<ast::UnaryOp>(assign<&ast::UnaryOp::op>(lit<"-">) +
                           assign<&ast::UnaryOp::operand>(
                               ref<FloatLit> | ref<IntLit>))  //
      | ref<FloatLit> | ref<IntLit>> {};

  struct SignedNumber : rule<
      create<ast::UnaryOp>(assign<&ast::UnaryOp::op>(lit<"-">) +
                           assign<&ast::UnaryOp::operand>(
                               ref<FloatLit> | ref<ImagLit> | ref<IntLit>))  //
      | ref<FloatLit> | ref<ImagLit> | ref<IntLit>> {};

  struct ComplexNumberPattern : rule<create<ast::BinOp>(
      assign<&ast::BinOp::left>(ref<SignedRealNumber>) +
      assign<&ast::BinOp::op>(lit<"+"> | lit<"-">) +
      assign<&ast::BinOp::right>(ref<ImagLit>))> {};

  // `literal_expr`: the bare value, reused both wrapped (literal_pattern)
  // and bare (a mapping pattern's key).
  struct LiteralExprBare : rule<
      ref<ComplexNumberPattern> | ref<SignedNumber> | ref<Strings>  //
      | create<ast::NoneLit>(kw<"None">)                            //
      | create<ast::BoolLit>(flag<&ast::BoolLit::value>(kw<"True">))  //
      | create<ast::BoolLit>(kw<"False">)> {};

  struct LiteralPattern : rule<assign<&ast::MatchValue::value>(ref<LiteralExprBare>)> {};

  struct ValuePattern : rule<assign<&ast::MatchValue::value>(ref<AttrChain>)> {};

  struct CapturePattern : rule<assign<&ast::MatchCapture::name>(ref<NameNotWildcard>)> {};

  struct WildcardPattern
      : rule<create<ast::MatchCapture>(ref<WildcardKwTerm>)> {};

  struct GroupPattern : rule<lit<"("> + ref<Pattern> + lit<")">> {};

  // `*name` inside a sequence pattern; `name` may be the wildcard (`*_`).
  struct StarPattern : rule<create<ast::MatchStar>(
      lit<"*"> + (ref<WildcardKwTerm> |
                 assign<&ast::MatchStar::name>(ref<NameNotWildcard>)))> {};

  struct MaybeStarPattern : rule<ref<StarPattern> | ref<Pattern>> {};

  struct SequencePatternBracket : rule<create<ast::MatchSequence>(
      lit<"["> +
      option(some(append<&ast::MatchSequence::items>(ref<MaybeStarPattern>),
                  lit<",">) +
             option(lit<",">)) +
      lit<"]">)> {};

  // `'(' open_sequence_pattern? ')'`: unlike group_pattern, the single-item
  // form needs its own trailing comma to tell `(pattern)` (a group) apart
  // from `(pattern,)` (a one-element sequence).
  struct SequencePatternParen : rule<create<ast::MatchSequence>(
      lit<"("> +
      option(append<&ast::MatchSequence::items>(ref<MaybeStarPattern>) +
             lit<","> +
             option(some(append<&ast::MatchSequence::items>(
                             ref<MaybeStarPattern>),
                         lit<",">) +
                    option(lit<",">))) +
      lit<")">)> {};

  struct KeyValuePattern : rule<create<ast::MatchMappingItem>(
      assign<&ast::MatchMappingItem::key>(ref<LiteralExprBare> |
                                          ref<AttrChain>) +
      lit<":"> + assign<&ast::MatchMappingItem::value>(ref<Pattern>))> {};

  struct MappingPattern : rule<create<ast::MatchMapping>(
      lit<"{"> +
      option((some(append<&ast::MatchMapping::items>(ref<KeyValuePattern>),
                   lit<",">) +
              option(lit<","> + option(lit<"**"> +
                     assign<&ast::MatchMapping::rest>(ref<NameNotWildcard>) +
                     option(lit<",">))))  //
             | (lit<"**"> +
                assign<&ast::MatchMapping::rest>(ref<NameNotWildcard>) +
                option(lit<",">))) +
      lit<"}">)> {};

  struct KeywordPattern : rule<create<ast::MatchKeywordPattern>(
      assign<&ast::MatchKeywordPattern::name>(ref<Ident>) + lit<"="> +
      assign<&ast::MatchKeywordPattern::value>(ref<Pattern>))> {};

  struct ClassPattern : rule<create<ast::MatchClass>(
      assign<&ast::MatchClass::cls>(ref<NameOrAttr>) + lit<"("> +
      option((some(append<&ast::MatchClass::positional>(ref<Pattern>),
                   lit<",">) +
              option(lit<","> +
                     some(append<&ast::MatchClass::keywords>(
                              ref<KeywordPattern>),
                          lit<",">)) + option(lit<",">))  //
             | (some(append<&ast::MatchClass::keywords>(ref<KeywordPattern>),
                     lit<",">) + option(lit<",">))) +
      lit<")">)> {};

  struct ClosedPattern : rule<
      ref<LiteralPattern> | ref<ValuePattern> | ref<ClassPattern> |
      ref<CapturePattern> | ref<WildcardPattern> | ref<GroupPattern> |
      ref<SequencePatternBracket> | ref<SequencePatternParen> |
      ref<MappingPattern>> {};

  struct OrPattern : rule<
      create<ast::MatchOr>(
          append<&ast::MatchOr::patterns>(ref<ClosedPattern>) + lit<"|"> +
          some(append<&ast::MatchOr::patterns>(ref<ClosedPattern>),
               lit<"|">))  //
      | ref<ClosedPattern>> {};

  // `as_pattern` is folded directly into `Pattern` rather than kept as its
  // own referenced rule: `GroupPattern` recurses back into `Pattern`
  // through `ClosedPattern`/`OrPattern`, and a rule reached only by bare
  // `ref<>`s all the way round a cycle has nothing of its own to close the
  // cycle on -- deduction needs a `create<>` directly in `Pattern`'s own
  // body, not one level away behind a reference.
  struct Pattern : rule<
      create<ast::MatchAs>(assign<&ast::MatchAs::pattern>(ref<OrPattern>) +
                           kw<"as"> +
                           assign<&ast::MatchAs::name>(ref<NameNotWildcard>))  //
      | ref<OrPattern>> {};

  // `patterns`: a case_block's own top level also allows a bare,
  // unparenthesized sequence (`case a, b:`).
  struct PatternsTop : rule<
      create<ast::MatchSequence>(
          append<&ast::MatchSequence::items>(ref<MaybeStarPattern>) +
          lit<","> +
          option(some(append<&ast::MatchSequence::items>(
                          ref<MaybeStarPattern>),
                      lit<",">) +
                 option(lit<",">)))  //
      | ref<Pattern>> {};

  struct CaseBlock : rule<create<ast::MatchCase>(
      ref<CaseKwTerm> + assign<&ast::MatchCase::pattern>(ref<PatternsTop>) +
      option(kw<"if"> + assign<&ast::MatchCase::guard>(ref<NamedExpression>)) +
      lit<":"> + suite<&ast::MatchCase::body>)> {};

  // `match` always opens an indented block of cases: no same-line form.
  struct MatchStmt : rule<create<ast::MatchStmt>(
      ref<MatchKwTerm> + assign<&ast::MatchStmt::subject>(ref<StarNamedExpressions>) +
      lit<":"> + ref<NEWLINE> + indent +
      some(append<&ast::MatchStmt::cases>(ref<CaseBlock>)) + dedent)> {};

  struct CompoundStmt : rule<
      ref<IfStmt> | ref<WhileStmt> | ref<ForStmt> |
      ref<WithStmt> | ref<TryStmt> | ref<FunctionDef> |
      ref<ClassDef> | ref<MatchStmt>> {};

  // MODULE
  // ======

  struct Module : rule<create<ast::Module>(
      option(ref<LeadingTrivia>) +
      many(append<&ast::Module::body>(ref<CompoundStmt>) |
           sameLineBlock<&ast::Module::body>))> {};

  using entry = Module;
  using ignored = WS;
  static constexpr auto word_boundary = uc::XID_Continue;
  using hidden = list<COMMENT>;
};

}  // namespace python
