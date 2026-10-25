// The javascript language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "javascript/core/ast.hpp"

namespace javascript {

// Drops the delimiters a template-text terminal reads along with its text --
// "`" or "}" before it, "`" or "${" after it -- so the text alone reaches the
// AST.
template <std::size_t Tail>
inline constexpr auto drop_ends =
    [](std::string_view t) { return t.substr(1, t.size() - 1 - Tail); };

struct JavaScript : cursiv::GrammarSpec {
  // A LineTerminator in the gap before the token in hand, comments included:
  // the two separators past ASCII count as ECMAScript counts them.
  static constexpr auto LineBreak =
      gap::has(eol | lit<"\xE2\x80\xA8"> | lit<"\xE2\x80\xA9">);
  // -- Trivia ----
  static constexpr auto LineTerminator = cr<R"(\r\n\u2028\u2029)">;
  struct WS : terminal<some(cr<R"( \t\v\f\r\n\uFEFF\u2028\u2029)"> | uc::Zs)> {
  };
  struct SL_COMMENT : terminal<lit<"//"> + many(~LineTerminator)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct HTML_COMMENT
      : terminal<(lit<"<!--"> | lit<"-->">)+many(~LineTerminator)> {};
  struct CDATA_COMMENT : terminal<through(lit<"<![CDATA[">, lit<"]]>">)> {};
  struct HASHBANG : terminal<lit<"#!"> + many(~LineTerminator)> {};

  static constexpr auto IdEscape =
      lit<"\\u"> + (repeat<4>(cr<"0-9a-fA-F">) |
                    (lit<"{"> + some(cr<"0-9a-fA-F">) + lit<"}">));
  static constexpr auto IdStart =
      uc::L | uc::Nl | cr<R"(_$\u1885\u1886\u2118\u212E\u309B\u309C)">;
  static constexpr auto IdContinue =
      IdStart | uc::Mn | uc::Mc | uc::Nd | uc::Pc |
      cr<R"(\u200C\u200D\u00B7\u0387\u1369-\u1371\u19DA\u30FB\uFF65)">;
  static constexpr auto IdChars =
      (IdStart | IdEscape) + many(IdContinue | IdEscape);
  struct RawId : terminal<IdChars> {};

  static constexpr auto StrictReserved =
      kw<"break"> | kw<"do"> | kw<"instanceof"> | kw<"typeof"> | kw<"case"> |
      kw<"else"> | kw<"new"> | kw<"var"> | kw<"catch"> | kw<"finally"> |
      kw<"return"> | kw<"void"> | kw<"continue"> | kw<"for"> | kw<"switch"> |
      kw<"while"> | kw<"debugger"> | kw<"function"> | kw<"this"> | kw<"with"> |
      kw<"default"> | kw<"if"> | kw<"throw"> | kw<"delete"> | kw<"in"> |
      kw<"try"> | kw<"class"> | kw<"enum"> | kw<"extends"> | kw<"super"> |
      kw<"const"> | kw<"export"> | kw<"import"> | kw<"await"> | kw<"null"> |
      kw<"true"> | kw<"false">;
  // Each of these lexes as a plain identifier outside its own construct
  // (`static`/`let`/`implements`/... only in non-strict code, gated on a
  // runtime flag cursiv has no equivalent of): soft keywords, not a gap --
  // simply never excluded from `RawId` below, so the same shift reads them
  // there too and the parse decides which reading stands. Over-accepts in
  // strict code (semantic-predicate).
  static constexpr auto SoftReserved =
      kw<"async"> | kw<"as"> | kw<"from"> | kw<"yield"> | kw<"of"> |
      kw<"static"> | kw<"let"> | kw<"implements"> | kw<"private"> |
      kw<"public"> | kw<"interface"> | kw<"package"> | kw<"protected">;
  static constexpr auto AllKeywords = StrictReserved | SoftReserved;

  // `get`/`set` are never reserved: they lex as a plain identifier, and only
  // what follows tells the accessor introducer from a key apart. Their own
  // terminal, offered both as a key and as the introducer below, same idiom.
  struct GetKwTerm : terminal<kw<"get">> {};
  struct SetKwTerm : terminal<kw<"set">> {};
  // Same idiom: a class member named "static" (`static() {}`) is one
  // reading of the very word that also opens a static block or modifier.
  struct StaticKwTerm : terminal<kw<"static">> {};

  struct PlainId : terminal<IdChars> {};
  struct Ident : rule<assign<&ast::Ident::name>(ref<PlainId>)> {};
  // Any word, keywords included: `.foo` and `.#foo` both take one -- `assign`
  // reads one part, so the optional `#` is folded into the terminal itself.
  struct PrivateOrName : terminal<option(lit<"#">) + IdChars> {};
  // `AllKeywords` stands beside `PrivateOrName` and not only inside it:
  // offered as the sole candidate, a general terminal loses a keyword-shaped
  // word to the specific keyword token at an ambiguous state; offered next
  // to it, either is admitted (see `ModuleExportName`).
  struct MemberName
      : rule<assign<&ast::Ident::name>(ref<PrivateOrName> | AllKeywords)> {};

  // -- Literals ---- A digit separator stands only between two digits: never
  // leading (a dot or a radix prefix must be followed by an actual digit, not
  // "_"), never doubled, never trailing. `X + many(option("_") + X)` is that
  // shape for one digit class X; unguarded, "._" reads as a two-character
  // NUMBER and out-munches the "." of a member access right after an
  // identifier.
  static constexpr auto DecRest = many(option(lit<"_">) + cr<"0-9">);
  static constexpr auto HexRest = many(option(lit<"_">) + cr<"0-9a-fA-F">);
  static constexpr auto OctRest = many(option(lit<"_">) + cr<"0-7">);
  static constexpr auto BinRest = many(option(lit<"_">) + cr<"01">);
  static constexpr auto DecInt = lit<"0"> | (cr<"1-9"> + DecRest);
  static constexpr auto Exponent =
      cr<"eE"> + option(cr<"+-">) + cr<"0-9"> + DecRest;

  struct NUMBER
      : terminal<(DecInt + lit<"."> + option(cr<"0-9"> + DecRest) +
                  option(Exponent)) |
                 (lit<"."> + cr<"0-9"> + DecRest + option(Exponent)) |
                 (DecInt + option(Exponent)) |
                 (lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> + HexRest) |
                 (lit<"0"> + cr<"oO"> + cr<"0-7"> + OctRest) |
                 (lit<"0"> + cr<"bB"> + cr<"01"> + BinRest) |
                 // Legacy octal: accepted unconditionally.
                 (lit<"0"> + some(cr<"0-7">))> {};

  struct BIGINT
      : terminal<((lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> + HexRest) |
                  (lit<"0"> + cr<"oO"> + cr<"0-7"> + OctRest) |
                  (lit<"0"> + cr<"bB"> + cr<"01"> + BinRest) | DecInt) +
                 lit<"n">> {};

  // Escapes are not decoded, only kept from hiding the closing quote.
  static constexpr auto EscapeSeq = lit<"\\"> + (eol | any);
  struct STRING
      : terminal<(lit<"\""> + many(EscapeSeq | cr<R"(^"\\\r\n)">) + lit<"\"">) |
                 (lit<"'"> + many(EscapeSeq | cr<R"(^'\\\r\n)">) + lit<"'">)> {
  };

  static constexpr auto RegexBackslash = lit<"\\"> + cr<R"(^\r\n)">;
  static constexpr auto RegexClass =
      lit<"["> + many(RegexBackslash | cr<R"(^\r\n\\])">) + lit<"]">;
  struct REGEX
      : terminal<lit<"/"> +
                 (RegexBackslash | RegexClass | cr<R"(^*\r\n\\/[)">)+many(
                     RegexBackslash | RegexClass | cr<R"(^\r\n\\/[)">) +
                 lit<"/"> + many(cr<"a-zA-Z">)> {};

  // Template text, fused to the delimiters around it, as the specification's
  // lexical grammar reads it: a terminal may not match empty, and a skip()
  // region cannot open on a token reached from a shared parser state such as
  // "just finished an Expression". Fused to what ends it too, the text after
  // an interpolation is never as short as a bare "}", which a state merged
  // with a block's end would read instead. `$` alone stays text, `$` before `{`
  // does not (single-byte guard).
  static constexpr auto TplRun =
      many((lit<"\\"> + any) | cr<R"(^`$\\)"> | lit<"$", !cr<"{">>);
  struct TplNoSub
      : terminal<lit<"`"> + TplRun + lit<"`">, text_conv<drop_ends<1>>> {};
  struct TplHead
      : terminal<lit<"`"> + TplRun + lit<"${">, text_conv<drop_ends<2>>> {};
  struct TplMiddle
      : terminal<lit<"}"> + TplRun + lit<"${">, text_conv<drop_ends<2>>> {};
  struct TplTail
      : terminal<lit<"}"> + TplRun + lit<"`">, text_conv<drop_ends<1>>> {};

  struct Expression;
  struct AssignExpr;
  struct ConditionalExpr;
  struct BinaryExpr;
  struct LogicalOrExpr;
  struct CoalesceExpr;
  struct BitwiseOrExpr;
  struct ExponentExpr;
  struct UnaryExpr;
  struct UpdateExpr;
  struct PostfixExpr;
  struct PrimaryExpr;
  struct NewExpr_;
  struct FunctionExpr_;
  struct ArrowFunctionExpr_;
  struct ClassExpr_;
  struct ArrayLiteral;
  struct ObjectLiteral;
  struct Statement;
  struct BlockStmt_;
  struct FunctionBody;
  struct Assignable;

  // -- Templates (tag stays null for the untagged use in PrimaryExpr) ----
  // The text is all in the terminals: an interpolation's expression gets the
  // grammar's ordinary trivia.
  static constexpr auto TemplateBody =
      append<&ast::TemplateLit::quasis>(ref<TplNoSub>) |
      (append<&ast::TemplateLit::quasis>(ref<TplHead>) +
       append<&ast::TemplateLit::expressions>(ref<Expression>) +
       many(append<&ast::TemplateLit::quasis>(ref<TplMiddle>) +
            append<&ast::TemplateLit::expressions>(ref<Expression>)) +
       append<&ast::TemplateLit::quasis>(ref<TplTail>));
  struct TemplateLiteral : rule<create<ast::TemplateLit>(TemplateBody)> {};

  // -- Primary expressions ----
  struct ThisExpr_ : rule<create<ast::ThisExpr>(kw<"this">)> {};
  struct SuperExpr_ : rule<create<ast::SuperExpr>(kw<"super">)> {};
  // `new.<word>` reads any identifier after the dot, so `target` is not
  // special-cased -- `Ident` already spells every word real code writes
  // there.
  struct NewTargetExpr_
      : rule<create<ast::NewTargetExpr>(
            kw<"new"> + lit<"."> +
            assign<&ast::NewTargetExpr::property>(ref<Ident>))> {};
  struct ImportMetaExpr_
      : rule<create<ast::ImportMetaExpr>(
            kw<"import"> + lit<"."> +
            assign<&ast::ImportMetaExpr::property>(ref<Ident>))> {};
  struct NullLit_ : rule<create<ast::NullLit>(kw<"null">)> {};
  struct BOOL_TOK : terminal<kw<"true"> | kw<"false">, bool> {};
  struct BoolLit_ : rule<assign<&ast::BoolLit::value>(ref<BOOL_TOK>)> {};
  struct NumberLit_ : rule<assign<&ast::NumberLit::text>(ref<NUMBER>)> {};
  struct BigIntLit_ : rule<assign<&ast::BigIntLit::text>(ref<BIGINT>)> {};
  struct StringLiteral : rule<assign<&ast::StringLit::text>(ref<STRING>)> {};
  struct RegexLit_ : rule<assign<&ast::RegexLit::text>(ref<REGEX>)> {};

  struct Literal : rule<ref<NullLit_> | ref<BoolLit_> | ref<StringLiteral> |
                        ref<TemplateLiteral> | ref<RegexLit_> |
                        ref<BigIntLit_> | ref<NumberLit_>> {};

  struct ParenExpr : rule<lit<"("> + ref<Expression> + lit<")">> {};

  struct YieldExpr_
      : rule<create<ast::YieldExpr>(
            kw<"yield"> +
            option(!LineBreak +
                   option(flag<&ast::YieldExpr::delegate>(lit<"*">)) +
                   assign<&ast::YieldExpr::argument>(ref<AssignExpr>)))> {};

  struct ImportCallExpr_
      : rule<create<ast::ImportCallExpr>(
            kw<"import"> + lit<"("> +
            assign<&ast::ImportCallExpr::source>(ref<AssignExpr>) +
            option(lit<","> + option(assign<&ast::ImportCallExpr::options>(
                                         ref<AssignExpr>) +
                                     option(lit<",">))) +
            lit<")">)> {};

  struct PrimaryExpr
      : rule<ref<ThisExpr_> | ref<SuperExpr_> | ref<NewTargetExpr_> |
             ref<Literal> | ref<ArrayLiteral> | ref<ObjectLiteral> |
             ref<ParenExpr> | ref<FunctionExpr_> | ref<ClassExpr_> |
             ref<ImportCallExpr_> | ref<ImportMetaExpr_> | ref<Ident>> {};

  // -- Member / call / new chain ----
  static constexpr auto Spread = lit<"...">;
  struct SpreadArg
      : rule<create<ast::Spread>(
            Spread + assign<&ast::Spread::argument>(ref<AssignExpr>))> {};
  static constexpr auto OneArg = ref<SpreadArg> | ref<AssignExpr>;

  static constexpr auto MemberTail =
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          option(flag<&ast::MemberExpr::optional>(lit<"?.">)) +
          flag<&ast::MemberExpr::computed>(lit<"[">) +
          assign<&ast::MemberExpr::property>(ref<Expression>) + lit<"]">) |
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          (flag<&ast::MemberExpr::optional>(lit<"?.">) |
           lit<".">)+assign<&ast::MemberExpr::property>(ref<MemberName>));

  static constexpr auto CallArguments =
      lit<"("> +
      option(append<&ast::CallExpr::args>(OneArg) +
             many(lit<","> + append<&ast::CallExpr::args>(OneArg)) +
             option(lit<",">)) +
      lit<")">;
  static constexpr auto NewArguments =
      lit<"("> +
      option(append<&ast::NewExpr::args>(OneArg) +
             many(lit<","> + append<&ast::NewExpr::args>(OneArg)) +
             option(lit<",">)) +
      lit<")">;

  struct MemberOnlyExpr
      : rule<(ref<PrimaryExpr> | ref<NewExpr_>)+many(MemberTail)> {};

  // A `(` after `new X` is X's arguments, never a call on the bare `new X`.
  struct NewExpr_
      : rule<create<ast::NewExpr>(
            kw<"new"> + assign<&ast::NewExpr::callee>(ref<MemberOnlyExpr>) +
            (NewArguments | !lit<"(">))> {};

  static constexpr auto PostfixTail =
      MemberTail |
      nest<ast::CallExpr, &ast::CallExpr::callee>(
          option(flag<&ast::CallExpr::optional>(lit<"?.">)) +
          CallArguments) |
      nest<ast::TemplateLit, &ast::TemplateLit::tag>(TemplateBody) |
      // No line break before a postfix operator: `a \n ++b` is two statements.
      nest<ast::PostfixUpdateExpr, &ast::PostfixUpdateExpr::operand>(
          !LineBreak +
          assign<&ast::PostfixUpdateExpr::op>(lit<"++"> | lit<"--">));

  struct PostfixExpr
      : rule<(ref<PrimaryExpr> | ref<NewExpr_>)+many(PostfixTail)> {};

  // UpdateExpression: a postfix chain, or prefix `++`/`--` of a full unary
  // expression -- the only unary forms `**` accepts on its left (`++x ** 2`
  // parses, `-x ** 2`/`typeof x ** 2`/... are a SyntaxError below).
  struct UpdateExpr
      : rule<create<ast::PrefixUpdateExpr>(
                 assign<&ast::PrefixUpdateExpr::op>(lit<"++"> | lit<"--">) +
                 assign<&ast::PrefixUpdateExpr::operand>(ref<UnaryExpr>)) //
             | ref<PostfixExpr>> {};

  // -- Unary chain ----
  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(kw<"delete"> | kw<"void"> |
                                             kw<"typeof">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"+", !cr<"+=">> |
                                               lit<"-", !cr<"-=">> | lit<"~"> |
                                               lit<"!", !cr<"=">>) +
                   assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) //
             | create<ast::AwaitExpr>(
                   kw<"await"> +
                   assign<&ast::AwaitExpr::operand>(ref<UnaryExpr>)) //
             | ref<UpdateExpr>> {};

  // ExponentiationExpression: right-associative, and its left operand is
  // `UpdateExpression`, not a full unary -- `-a ** b` is a SyntaxError in
  // real JS (parenthesize either side to disambiguate the sign).
  struct ExponentExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<UpdateExpr>) +
                 assign<&ast::BinaryExpr::op>(lit<"**", !cr<"=">>) +
                 assign<&ast::BinaryExpr::right>(ref<ExponentExpr>)) //
             | ref<UnaryExpr>> {};

  // -- Binary ladder (one table, tightest to loosest, through bitwise-OR) --
  // Every operator that is a textual prefix of another one defined in a
  // DIFFERENT rule (AssignExpr's `=`, `**=`, ...) carries a guard: maximal
  // munch is automatic within one infix<> table, never across two.
  struct BitwiseOrExpr
      : infix<ExponentExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"*", !cr<"*=">> | lit<"/", !cr<"=">> |
                    lit<"%", !cr<"=">>) >>
               left(lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>) >>
               left(lit<"<<", !cr<"=">> | lit<">>>", !cr<"=">> |
                    lit<">>", !cr<"=>">>) >>
               left(lit<"<="> | lit<">="> | lit<"<", !cr<"=<">> |
                    lit<">", !cr<"=>">> | kw<"instanceof"> | kw<"in">) >>
               left(lit<"==="> | lit<"!=="> | lit<"==", !cr<"=">> |
                    lit<"!=", !cr<"=">>) >>
               left(lit<"&", !cr<"&=">>) >> //
               left(lit<"^", !cr<"=">>) >>  //
               left(lit<"|", !cr<"|=">>))> {};

  // ShortCircuitExpression: `??` never mixes with `&&`/`||` without
  // parentheses in real ECMAScript (a SyntaxError either way round) -- each
  // is its own table over BitwiseOrExpr rather than a further rung of one
  // shared ladder.
  struct LogicalOrExpr
      : infix<BitwiseOrExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"&&", !cr<"=">>) >> left(lit<"||", !cr<"=">>))> {};

  // At least one `??`: a bare operand is LogicalOrExpr's alone, and an
  // operand two rules read is a cell the parser must decide, once per operand.
  struct CoalesceExpr
      : rule<ref<BitwiseOrExpr> +
             some(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(lit<"??", !cr<"=">>) +
                 assign<&ast::BinaryExpr::right>(ref<BitwiseOrExpr>)))> {};

  struct BinaryExpr : rule<ref<LogicalOrExpr> | ref<CoalesceExpr>> {};

  struct ConditionalExpr
      : rule<create<ast::ConditionalExpr>(
                 assign<&ast::ConditionalExpr::test>(ref<BinaryExpr>) +
                 lit<"?", !cr<".?">> +
                 assign<&ast::ConditionalExpr::consequent>(ref<AssignExpr>) +
                 lit<":"> +
                 assign<&ast::ConditionalExpr::alternate>(ref<AssignExpr>)) //
             | ref<BinaryExpr>
             // An arrow or a yield is an assignment expression, as the
             // specification ranks them: no operator or tail follows one.
             | ref<ArrowFunctionExpr_> | ref<YieldExpr_>> {};

  struct AssignExpr
      : infix<ConditionalExpr, &ast::AssignExpr::target, &ast::AssignExpr::op,
              &ast::AssignExpr::value,
              right(lit<"=", !cr<"=>">> | lit<"*="> | lit<"/="> | lit<"%="> |
                    lit<"+="> | lit<"-="> | lit<"<<="> | lit<">>>="> |
                    lit<">>="> | lit<"&="> | lit<"^="> | lit<"|="> |
                    lit<"**="> | lit<"?" "?="> | lit<"&&="> | lit<"||=">)> {};

  struct Expression
      : infix<AssignExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(lit<",">)> {};

  // -- Functions ----
  struct ParamItem
      : rule<create<ast::Param>(
                 flag<&ast::Param::rest>(Spread) +
                 assign<&ast::Param::pattern>(ref<AssignExpr>)) //
             | create<ast::Param>(
                   assign<&ast::Param::pattern>(ref<Assignable>) +
                   option(lit<"="> + assign<&ast::Param::defaultValue>(
                                         ref<AssignExpr>)))> {};

  // Parameterised on the target member: FunctionExpr, FunctionDecl and
  // (through MethodBody/ClassMethodBody) an inner FunctionExpr each own
  // their own `params` list. `ParamTailFor` starts right after the opening
  // "(" so a caller that already consumed it (to also flag on it) can
  // continue with the same list.
  template <auto Ptr>
  static constexpr auto ParamTailFor =
      option(append<Ptr>(ref<ParamItem>) +
             many(lit<","> + append<Ptr>(ref<ParamItem>)) + option(lit<",">)) +
      lit<")">;
  template <auto Ptr>
  static constexpr auto ParamListFor = lit<"("> + ParamTailFor<Ptr>;
  static constexpr auto ParamList = ParamListFor<&ast::FunctionExpr::params>;

  struct FunctionExpr_
      : rule<create<ast::FunctionExpr>(
            option(flag<&ast::FunctionExpr::isAsync>(kw<"async">)) +
            kw<"function"> +
            option(flag<&ast::FunctionExpr::isGenerator>(lit<"*">)) +
            option(assign<&ast::FunctionExpr::name>(ref<RawId>)) + ParamList +
            assign<&ast::FunctionExpr::body>(ref<FunctionBody>))> {};

  struct ArrowSingleParam
      : rule<create<ast::Param>(assign<&ast::Param::pattern>(ref<Ident>))> {};

  static constexpr auto ArrowParams =
      (lit<"("> +
       option(append<&ast::ArrowFunctionExpr::params>(ref<ParamItem>) +
              many(lit<","> + append<&ast::ArrowFunctionExpr::params>(
                                  ref<ParamItem>)) +
              option(lit<",">)) +
       lit<")">) //
      | append<&ast::ArrowFunctionExpr::params>(ref<ArrowSingleParam>);

  struct ArrowFunctionExpr_
      : rule<create<ast::ArrowFunctionExpr>(
            option(flag<&ast::ArrowFunctionExpr::isAsync>(kw<"async">)) +
            ArrowParams + !LineBreak + lit<"=>"> +
            (assign<&ast::ArrowFunctionExpr::block>(
                 ref<FunctionBody>) //
                                    // ConciseBody: [lookahead != {] -- a brace
                                    // after `=>` opens a block, never an object
                                    // literal.
             | (!lit<"{"> +
                assign<&ast::ArrowFunctionExpr::exprBody>(ref<AssignExpr>))))> {
  };

  // -- Array / object literals ----
  struct ArrayLiteral
      : rule<create<ast::ArrayLit>(
            lit<"["> +
            many(append<&ast::ArrayLit::elements>(create<ast::ArraySlot>(
                option(assign<&ast::ArraySlot::value>(OneArg)) + lit<",">))) +
            option(append<&ast::ArrayLit::elements>(create<ast::ArraySlot>(
                assign<&ast::ArraySlot::value>(OneArg)))) +
            lit<"]">)> {};

  // `get`/`set` are keys too, through their own terminal (declared above,
  // shared with the accessor introducer below): one token, and what follows
  // decides between the two readings. Every keyword is a key as well:
  // `RawId` loses to the keyword token wherever a state reads both.
  struct PropKeyIdent
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(
            ref<RawId> | AllKeywords | ref<GetKwTerm> | ref<SetKwTerm>))> {};
  static constexpr auto PropKey = ref<PropKeyIdent> | ref<StringLiteral> |
                                  ref<NumberLit_> | ref<BigIntLit_>;
  static constexpr auto PropName =
      (flag<&ast::Prop::computed>(lit<"[">) +
       assign<&ast::Prop::key>(ref<AssignExpr>) + lit<"]">) //
      | assign<&ast::Prop::key>(PropKey);

  struct MethodBody
      : rule<create<ast::FunctionExpr>(
            ParamList + assign<&ast::FunctionExpr::body>(ref<FunctionBody>))> {
  };
  // The method alternative also needs `isMethod` on the ENCLOSING `Prop`,
  // and a directive reads one part: the "(" is consumed here to set it,
  // so the params it opens pick up right after it.
  struct MethodBodyTail
      : rule<create<ast::FunctionExpr>(
            ParamTailFor<&ast::FunctionExpr::params> +
            assign<&ast::FunctionExpr::body>(ref<FunctionBody>))> {};

  struct PropertyRule
      : rule<create<ast::Prop>(
            (PropName + lit<":"> +
             assign<&ast::Prop::value>(ref<AssignExpr>)) //
            | (option(flag<&ast::Prop::isAsync>(kw<"async">)) +
               option(flag<&ast::Prop::isGenerator>(lit<"*">)) + PropName +
               flag<&ast::Prop::isMethod>(lit<"(">) +
               assign<&ast::Prop::value>(ref<MethodBodyTail>)) //
            | (flag<&ast::Prop::isGetter>(ref<GetKwTerm>) + PropName +
               assign<&ast::Prop::value>(ref<MethodBody>)) //
            | (flag<&ast::Prop::isSetter>(ref<SetKwTerm>) + PropName +
               assign<&ast::Prop::value>(ref<MethodBody>)) //
            // `...x` (spread) and the permissive bare-expression shorthand
            // this grammar also allows here (see ast::Prop::key).
            | assign<&ast::Prop::value>(OneArg))> {};

  struct ObjectLiteral
      : rule<create<ast::ObjectLit>(
            lit<"{"> +
            option(append<&ast::ObjectLit::properties>(ref<PropertyRule>) +
                   many(lit<","> + append<&ast::ObjectLit::properties>(
                                       ref<PropertyRule>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  // -- Classes ----
  // `get`, `set`, `static` and `async` name a member only before `(`, `=`, `;`
  // or `}`; before a word they are the modifier, and the name reading is dead
  // there.
  struct ClassMemberKeyIdent
      : rule<create<ast::Ident>(
                 assign<&ast::Ident::name>(ref<PrivateOrName>)) //
             | (create<ast::Ident>(assign<&ast::Ident::name>(
                    ref<GetKwTerm> | ref<SetKwTerm> | ref<StaticKwTerm> |
                    kw<"async">)) +
                &(lit<"("> | lit<"="> | lit<";"> | lit<"}">))> {};
  static constexpr auto ClassMemberKey =
      (flag<&ast::ClassMember::computed>(lit<"[">) +
       assign<&ast::ClassMember::key>(ref<AssignExpr>) + lit<"]">) //
      | assign<&ast::ClassMember::key>(ref<ClassMemberKeyIdent> |
                                       ref<StringLiteral> | ref<NumberLit_> |
                                       ref<BigIntLit_>);

  struct ClassMethodBody
      : rule<create<ast::FunctionExpr>(
            ParamList + assign<&ast::FunctionExpr::body>(ref<FunctionBody>))> {
  };
  struct ClassMethodBodyTail
      : rule<create<ast::FunctionExpr>(
            ParamTailFor<&ast::FunctionExpr::params> +
            assign<&ast::FunctionExpr::body>(ref<FunctionBody>))> {};

  struct ClassMemberRule
      : rule<
            // A static initialisation block: only this alternative can
            // continue with "{" right after (bare) "static", so no lookahead
            // is needed to keep it apart from the modifier below.
            create<ast::ClassMember>(
                ref<StaticKwTerm> +
                assign<&ast::ClassMember::staticBlock>(ref<BlockStmt_>)) //
            // A method, getter or setter, optionally static.
            |
            create<ast::ClassMember>(
                option(flag<&ast::ClassMember::isStatic>(
                    ref<StaticKwTerm> + !lit<"{">)) +
                ((option(flag<&ast::ClassMember::isAsync>(kw<"async">)) +
                  option(flag<&ast::ClassMember::isGenerator>(lit<"*">)) +
                  ClassMemberKey +
                  flag<&ast::ClassMember::isMethod>(lit<"(">) +
                  assign<&ast::ClassMember::value>(ref<ClassMethodBodyTail>)) //
                 | (flag<&ast::ClassMember::isGetter>(ref<GetKwTerm>) +
                    ClassMemberKey +
                    assign<&ast::ClassMember::value>(ref<ClassMethodBody>)) //
                 | (flag<&ast::ClassMember::isSetter>(ref<SetKwTerm>) +
                    ClassMemberKey +
                    assign<&ast::ClassMember::value>(ref<ClassMethodBody>)))) //
            // A field, optionally static.
            | create<ast::ClassMember>(
                  option(flag<&ast::ClassMember::isStatic>(
                      ref<StaticKwTerm> + !lit<"{">)) +
                  ClassMemberKey +
                  option(lit<"="> +
                         assign<&ast::ClassMember::value>(ref<AssignExpr>)))> {
  };

  static constexpr auto ClassBody =
      lit<"{"> +
      many(append<&ast::ClassExpr::members>(ref<ClassMemberRule>) | lit<";">) +
      lit<"}">;

  struct ClassExpr_
      : rule<create<ast::ClassExpr>(
            kw<"class"> + option(assign<&ast::ClassExpr::name>(ref<RawId>)) +
            option(kw<"extends"> +
                   assign<&ast::ClassExpr::superClass>(ref<UnaryExpr>)) +
            ClassBody)> {};

  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<RawId>) +
            option(kw<"extends"> +
                   assign<&ast::ClassDecl::superClass>(ref<UnaryExpr>)) +
            lit<"{"> +
            many(append<&ast::ClassDecl::members>(ref<ClassMemberRule>) |
                 lit<";">) +
            lit<"}">)> {};

  struct FunctionDeclaration
      : rule<create<ast::FunctionDecl>(
            option(flag<&ast::FunctionDecl::isAsync>(kw<"async">)) +
            kw<"function"> +
            option(flag<&ast::FunctionDecl::isGenerator>(lit<"*">)) +
            assign<&ast::FunctionDecl::name>(ref<RawId>) +
            ParamListFor<&ast::FunctionDecl::params> +
            assign<&ast::FunctionDecl::body>(ref<FunctionBody>))> {};

  // -- Assignable (var/let/const target, parameter, for-in/of target) ----
  struct Assignable
      : rule<ref<Ident> | ref<ArrayLiteral> | ref<ObjectLiteral>> {};

  // -- Statements ---- ASI as the specification reads it: a semicolon is
  // inserted where the next token is refused behind a line break, before `}`
  // or at the end; the restricted productions (return, break, continue, throw,
  // yield, postfix update, arrow) refuse a line break with `!LineBreak`.
  static constexpr auto Eos = implied(lit<";">, LineBreak | &lit<"}"> | eof);

  struct Declarator_ : rule<create<ast::Declarator>(
                           assign<&ast::Declarator::id>(ref<Assignable>) +
                           option(lit<"="> + assign<&ast::Declarator::init>(
                                                 ref<AssignExpr>)))> {};

  static constexpr auto VarKind = kw<"var"> | kw<"let"> | kw<"const">;

  struct VarDeclList
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::kind>(VarKind) +
            append<&ast::VarDecl::declarations>(ref<Declarator_>) +
            many(lit<","> +
                 append<&ast::VarDecl::declarations>(ref<Declarator_>)))> {};

  struct VariableStatement : rule<ref<VarDeclList> + Eos> {};

  // `async function` also needs no guard here: read as a bare `async`
  // expression statement, it leaves "function" where `Eos` cannot stand, so
  // that reading fails and only FunctionDeclaration's survives.
  struct ExpressionStatement
      : rule<create<ast::ExprStmt>(
            !lit<"{"> + !kw<"class"> + !kw<"function"> +
            assign<&ast::ExprStmt::expression>(ref<Expression>) + Eos)> {};

  struct EmptyStatement : rule<create<ast::EmptyStmt>(lit<";">)> {};

  struct BlockStmt_
      : rule<create<ast::BlockStmt>(
            lit<"{"> + many(append<&ast::BlockStmt::body>(ref<Statement>)) +
            lit<"}">)> {};

  struct FunctionBody
      : rule<create<ast::BlockStmt>(
            lit<"{"> + many(append<&ast::BlockStmt::body>(ref<Statement>)) +
            lit<"}">)> {};

  struct IfStatement
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> + assign<&ast::IfStmt::test>(ref<Expression>) +
            lit<")"> + assign<&ast::IfStmt::consequent>(ref<Statement>) +
            option(commit(kw<"else">, assign<&ast::IfStmt::alternate>(ref<Statement>))))> {};

  struct DoWhileStatement
      : rule<create<ast::DoWhileStmt>(
            kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoWhileStmt::test>(ref<Expression>) + lit<")"> +
            Eos)> {};

  struct WhileStatement
      : rule<create<ast::WhileStmt>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileStmt::test>(ref<Expression>) + lit<")"> +
            assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct ForClassicStatement
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            option(assign<&ast::ForStmt::initDecl>(ref<VarDeclList>) |
                   assign<&ast::ForStmt::initExpr>(ref<Expression>)) +
            lit<";"> + option(assign<&ast::ForStmt::test>(ref<Expression>)) +
            lit<";"> + option(assign<&ast::ForStmt::update>(ref<Expression>)) +
            lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct ForBindingDeclarator
      : rule<create<ast::Declarator>(
            assign<&ast::Declarator::id>(ref<Assignable>))> {};
  struct ForBindingDecl
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::kind>(VarKind) +
            append<&ast::VarDecl::declarations>(ref<ForBindingDeclarator>))> {};

  struct ForInStatement
      : rule<create<ast::ForInStmt>(
            kw<"for"> + lit<"("> +
            (assign<&ast::ForInStmt::leftDecl>(ref<ForBindingDecl>) //
             | assign<&ast::ForInStmt::leftExpr>(ref<UnaryExpr>)) +
            kw<"in"> + assign<&ast::ForInStmt::right>(ref<Expression>) +
            lit<")"> + assign<&ast::ForInStmt::body>(ref<Statement>))> {};

  struct ForOfStatement
      : rule<create<ast::ForOfStmt>(
            kw<"for"> +
            option(flag<&ast::ForOfStmt::isAwait>(kw<"await">)) +
            lit<"("> +
            (assign<&ast::ForOfStmt::leftDecl>(ref<ForBindingDecl>) //
             | assign<&ast::ForOfStmt::leftExpr>(ref<UnaryExpr>)) +
            kw<"of"> + assign<&ast::ForOfStmt::right>(ref<AssignExpr>) +
            lit<")"> + assign<&ast::ForOfStmt::body>(ref<Statement>))> {};

  struct IterationStatement
      : rule<ref<DoWhileStatement> | ref<WhileStatement> | ref<ForOfStatement> |
             ref<ForInStatement> | ref<ForClassicStatement>> {};

  struct ContinueStatement
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(!LineBreak +
                   assign<&ast::ContinueStmt::label>(ref<PlainId>)) +
            Eos)> {};

  struct BreakStatement
      : rule<create<ast::BreakStmt>(
            kw<"break"> +
            option(!LineBreak + assign<&ast::BreakStmt::label>(ref<PlainId>)) +
            Eos)> {};

  struct ReturnStatement
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(!LineBreak +
                   assign<&ast::ReturnStmt::argument>(ref<Expression>)) +
            Eos)> {};

  struct YieldStatement
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::expression>(ref<YieldExpr_>) + Eos)> {};

  struct ThrowStatement
      : rule<create<ast::ThrowStmt>(
            kw<"throw"> + !LineBreak +
            assign<&ast::ThrowStmt::argument>(ref<Expression>) + Eos)> {};

  struct WithStatement
      : rule<create<ast::WithStmt>(
            kw<"with"> + lit<"("> +
            assign<&ast::WithStmt::object>(ref<Expression>) + lit<")"> +
            assign<&ast::WithStmt::body>(ref<Statement>))> {};

  struct LabelledStatement
      : rule<create<ast::LabeledStmt>(
            assign<&ast::LabeledStmt::label>(ref<PlainId>) + lit<":"> +
            assign<&ast::LabeledStmt::body>(ref<Statement>))> {};

  struct CaseClause
      : rule<create<ast::SwitchCase>(
            kw<"case"> + assign<&ast::SwitchCase::test>(ref<Expression>) +
            lit<":"> +
            many(append<&ast::SwitchCase::consequent>(ref<Statement>)))> {};
  struct DefaultClause
      : rule<create<ast::SwitchCase>(
            kw<"default"> + lit<":"> +
            many(append<&ast::SwitchCase::consequent>(ref<Statement>)))> {};

  struct SwitchStatement
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> + lit<"("> +
            assign<&ast::SwitchStmt::discriminant>(ref<Expression>) + lit<")"> +
            lit<"{"> + many(append<&ast::SwitchStmt::cases>(ref<CaseClause>)) +
            option(append<&ast::SwitchStmt::cases>(ref<DefaultClause>) +
                   many(append<&ast::SwitchStmt::cases>(ref<CaseClause>))) +
            lit<"}">)> {};

  struct DebuggerStatement
      : rule<create<ast::DebuggerStmt>(kw<"debugger"> + Eos)> {};

  struct CatchClause_
      : rule<create<ast::CatchClause>(
            kw<"catch"> +
            option(lit<"("> +
                   assign<&ast::CatchClause::param>(ref<Assignable>) +
                   lit<")">) +
            assign<&ast::CatchClause::body>(ref<BlockStmt_>))> {};

  struct TryStatement
      : rule<create<ast::TryStmt>(
            kw<"try"> + assign<&ast::TryStmt::block>(ref<BlockStmt_>) +
            ((assign<&ast::TryStmt::handler>(ref<CatchClause_>) +
              option(kw<"finally"> +
                     assign<&ast::TryStmt::finalizer>(ref<BlockStmt_>))) //
             | (kw<"finally"> +
                assign<&ast::TryStmt::finalizer>(ref<BlockStmt_>))))> {};

  // -- Modules ----
  struct ModuleExportName : rule<ref<RawId> | AllKeywords | ref<STRING>> {};

  struct ImportSpecifierRule
      : rule<create<ast::ImportSpecifier>(
            assign<&ast::ImportSpecifier::imported>(ref<ModuleExportName>) +
            option(kw<"as"> + assign<&ast::ImportSpecifier::local>(
                                  ref<RawId> | AllKeywords)))> {};

  static constexpr auto ImportNamedList =
      lit<"{"> +
      option(append<&ast::ImportStmt::specifiers>(ref<ImportSpecifierRule>) +
             many(lit<","> + append<&ast::ImportStmt::specifiers>(
                                 ref<ImportSpecifierRule>)) +
             option(lit<",">)) +
      lit<"}">;

  // The bare default import ("import a from...") folds into this same rule
  // (a plain identifier, no "as") rather than needing one of its own.
  struct ImportNamespaceRule
      : rule<create<ast::ImportSpecifier>(
            (flag<&ast::ImportSpecifier::isNamespace>(lit<"*">) +
             option(kw<"as"> +
                    assign<&ast::ImportSpecifier::local>(ref<RawId>))) //
            | assign<&ast::ImportSpecifier::local>(ref<RawId>))> {};

  // `imported` stays empty: that (with `isNamespace` false) is what marks a
  // default specifier, the same way ImportNamespaceRule leaves it empty too.
  struct ImportDefaultRule
      : rule<create<ast::ImportSpecifier>(
            assign<&ast::ImportSpecifier::local>(ref<RawId>))> {};

  struct ImportAttributeRule
      : rule<assign<&ast::ImportAttribute::key>(ref<ModuleExportName>) +
             lit<":"> + assign<&ast::ImportAttribute::value>(ref<STRING>)> {};

  template <auto Ptr>
  static constexpr auto ImportAttributes =
      option(kw<"with"> + lit<"{"> +
             many(append<Ptr>(ref<ImportAttributeRule>), lit<",">) +
             option(lit<",">) + lit<"}">);

  struct ImportStatement
      : rule<create<ast::ImportStmt>(
            kw<"import"> +
            ((option(
                  append<&ast::ImportStmt::specifiers>(ref<ImportDefaultRule>) +
                  lit<",">) +
              (append<&ast::ImportStmt::specifiers>(ref<ImportNamespaceRule>) //
               | ImportNamedList) +
              kw<"from"> + assign<&ast::ImportStmt::source>(ref<STRING>) +
              ImportAttributes<&ast::ImportStmt::attributes> + Eos) //
             | (assign<&ast::ImportStmt::source>(ref<STRING>) +
                ImportAttributes<&ast::ImportStmt::attributes> + Eos)))> {};

  struct ExportSpecifierRule
      : rule<create<ast::ExportSpecifier>(
            assign<&ast::ExportSpecifier::local>(ref<ModuleExportName>) +
            option(kw<"as"> + assign<&ast::ExportSpecifier::exported>(
                                  ref<ModuleExportName>)))> {};

  static constexpr auto ExportNamedList =
      lit<"{"> +
      option(append<&ast::ExportStmt::specifiers>(ref<ExportSpecifierRule>) +
             many(lit<","> + append<&ast::ExportStmt::specifiers>(
                                 ref<ExportSpecifierRule>)) +
             option(lit<",">)) +
      lit<"}">;

  struct ExportStarSpecifier
      : rule<create<ast::ExportSpecifier>(
            assign<&ast::ExportSpecifier::exported>(ref<ModuleExportName>))> {};

  struct ExportDeclaration_
      : rule<ref<VariableStatement> | ref<ClassDeclaration> |
             ref<FunctionDeclaration>> {};

  struct ExportStatement
      : rule<create<ast::ExportStmt>(
            kw<"export"> +
            ((flag<&ast::ExportStmt::isDefault>(kw<"default">) +
              (assign<&ast::ExportStmt::declaration>(ref<ExportDeclaration_>) //
               | (assign<&ast::ExportStmt::defaultExpr>(ref<AssignExpr>) +
                  Eos))) //
             | (flag<&ast::ExportStmt::isNamespace>(lit<"*">) +
                option(kw<"as"> + append<&ast::ExportStmt::specifiers>(
                                      ref<ExportStarSpecifier>)) +
                kw<"from"> + assign<&ast::ExportStmt::source>(ref<STRING>) +
                ImportAttributes<&ast::ExportStmt::attributes> + Eos) //
             | (ExportNamedList +
                option(kw<"from"> +
                       assign<&ast::ExportStmt::source>(ref<STRING>) +
                       ImportAttributes<&ast::ExportStmt::attributes>) +
                Eos) //
             |
             assign<&ast::ExportStmt::declaration>(ref<ExportDeclaration_>)))> {
  };

  struct Statement
      : rule<ref<BlockStmt_> | ref<VariableStatement> | ref<ImportStatement> |
             ref<ExportStatement> | ref<EmptyStatement> |
             ref<ClassDeclaration> | ref<FunctionDeclaration> |
             ref<IfStatement> | ref<IterationStatement> |
             ref<ContinueStatement> | ref<BreakStatement> |
             ref<ReturnStatement> | ref<YieldStatement> | ref<WithStatement> |
             ref<SwitchStatement> | ref<ThrowStatement> | ref<TryStatement> |
             ref<DebuggerStatement> | ref<LabelledStatement> |
             ref<ExpressionStatement>> {};

  struct Program_ : rule<create<ast::Program>(
                        option(assign<&ast::Program::hashbang>(ref<HASHBANG>)) +
                        many(append<&ast::Program::body>(ref<Statement>)))> {};

  using entry = Program_;
  using ignore = WS;
  static constexpr auto word_boundary = IdContinue | cr<R"(\\)">;
  using hidden = list<SL_COMMENT, ML_COMMENT, HTML_COMMENT, CDATA_COMMENT>;
};

} // namespace javascript
