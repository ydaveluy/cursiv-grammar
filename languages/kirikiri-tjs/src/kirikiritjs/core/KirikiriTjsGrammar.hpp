// The kirikiritjs language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "kirikiritjs/core/ast.hpp"

namespace kirikiritjs {

struct KirikiriTjs : cursiv::GrammarSpec {
  // -- Trivia ---- `\xEF\xBB\xBF` is a leading UTF-8 byte-order mark: three
  // corpus files open with one, an encoding artifact rather than a token.
  struct WS : terminal<some(cr<R"( \t\v\f\r\n)"> | lit<"\xC2\xA0"> |
                            lit<"\xE2\x80\xA8"> | lit<"\xE2\x80\xA9"> |
                            lit<"\xEF\xBB\xBF">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // -- Identifiers ---- `$`/`_` and a Unicode letter open one; a `\uXXXX`
  // escape reads as more identifier bytes, kept raw like every other escape.
  static constexpr auto IdEscape = lit<"\\u"> + repeat<4>(cr<"0-9a-fA-F">);
  static constexpr auto IdStart = uc::L | cr<"$_"> | IdEscape;
  static constexpr auto IdChar =
      uc::L | uc::Mn | uc::Mc | uc::Nd | uc::Pc | cr<R"($‌‍)">;
  static constexpr auto IdPart = IdChar | IdEscape;
  struct RawId : terminal<IdStart + many(IdPart)> {};

  // A dotted member name takes `ReservedWordSet` too, where a state also
  // reads one as a keyword -- the TJS2 additions are not in it.
  static constexpr auto KeywordSet =
      kw<"break"> | kw<"do"> | kw<"instanceof"> | kw<"typeof"> | kw<"case"> |
      kw<"else"> | kw<"new"> | kw<"var"> | kw<"catch"> | kw<"finally"> |
      kw<"return"> | kw<"void"> | kw<"continue"> | kw<"for"> | kw<"switch"> |
      kw<"while"> | kw<"debugger"> | kw<"function"> | kw<"this"> | kw<"with"> |
      kw<"default"> | kw<"if"> | kw<"throw"> | kw<"delete"> | kw<"in"> |
      kw<"try"> | kw<"class"> | kw<"enum"> | kw<"extends"> | kw<"super"> |
      kw<"const"> | kw<"export"> | kw<"import"> | kw<"static"> |
      kw<"incontextof">;
  static constexpr auto ReservedWordSet =
      KeywordSet | kw<"null"> | kw<"true"> | kw<"false">;

  struct PlainNameTok : terminal<IdStart + many(IdPart)> {};
  struct Identifier
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(ref<PlainNameTok>))> {
  };
  struct IdentifierName : rule<create<ast::Ident>(assign<&ast::Ident::name>(
                              ref<RawId> | ReservedWordSet))> {};

  // -- Numbers ---- Hex/octal/binary literals may also carry a fractional
  // part and an exponent (`p`/`P` alongside `e`/`E`).
  static constexpr auto DecimalIntegerLiteral =
      lit<"0"> | (cr<"1-9"> + many(cr<"0-9">));
  static constexpr auto ExponentPart =
      cr<"eEpP"> + option(cr<"+-">) + some(cr<"0-9">);
  struct NUMBER
      : terminal<
            (DecimalIntegerLiteral + lit<"."> + many(cr<"0-9">) +
             option(ExponentPart)) |
            (lit<".", !lit<".">> + some(cr<"0-9">) + option(ExponentPart)) |
            (DecimalIntegerLiteral + option(ExponentPart)) |
            (lit<"0"> + cr<"xX"> + some(cr<"0-9a-fA-F">) +
             option(lit<"."> + some(cr<"0-9a-fA-F">)) + option(ExponentPart)) |
            (lit<"0"> + cr<"oO"> + some(cr<"0-7">) +
             option(lit<"."> + some(cr<"0-7">)) + option(ExponentPart)) |
            (lit<"0"> + cr<"bB"> + some(cr<"01">) +
             option(lit<"."> + some(cr<"01">)) + option(ExponentPart)) |
            // Legacy octal (`'0' OctalDigit+`, no marker letter).
            (lit<"0"> + some(cr<"0-7">) + option(lit<"."> + some(cr<"0-7">)) +
             option(ExponentPart))> {};

  // -- Strings ---- Escapes are kept raw, only enough to not hide the closer.
  static constexpr auto EscapeSeq = lit<"\\"> + any;
  struct STRING
      : terminal<(lit<"\""> + many(EscapeSeq | cr<R"(^"\\\r\n)">) + lit<"\"">) |
                 (lit<"'"> + many(EscapeSeq | cr<R"(^'\\\r\n)">) + lit<"'">)> {
  };
  // `@"..."`/`@'...'`: only its own quote is escapable, unlike STRING above.
  struct TEMPLATE_STRING
      : terminal<(lit<"@\""> + many(lit<"\\\""> | cr<R"(^")">) + lit<"\"">) |
                 (lit<"@'"> + many(lit<"\\'"> | cr<"^'">) + lit<"'">)> {};
  // `<% ... %>`, kept whole with its delimiters.
  struct OCTET : terminal<through(lit<"<%">, lit<"%>">)> {};

  static constexpr auto RegexBackslash = lit<"\\"> + cr<R"(^\r\n)">;
  static constexpr auto RegexClass =
      lit<"["> + many(RegexBackslash | cr<R"(^\r\n\\])">) + lit<"]">;
  struct REGEX
      : terminal<lit<"/"> +
                 (RegexBackslash | RegexClass | cr<R"(^*\r\n\\/[)">)+many(
                     RegexBackslash | RegexClass | cr<R"(^\r\n\\/[)">) +
                 lit<"/"> + many(cr<"a-zA-Z">)> {};

  struct NullLitRule : rule<create<ast::NullLit>(kw<"null">)> {};
  struct VoidLitRule : rule<create<ast::VoidLit>(kw<"void">)> {};
  struct BOOL_TOK : terminal<kw<"true"> | kw<"false">, bool> {};
  struct BoolLitRule : rule<assign<&ast::BoolLit::value>(ref<BOOL_TOK>)> {};
  struct NumberLitRule : rule<assign<&ast::NumberLit::text>(ref<NUMBER>)> {};
  struct OctetLitRule : rule<assign<&ast::OctetLit::text>(ref<OCTET>)> {};
  struct RegexLitRule : rule<assign<&ast::RegexLit::text>(ref<REGEX>)> {};
  struct StringLitRule
      : rule<create<ast::StringLit>(some(append<&ast::StringLit::parts>(
            ref<STRING> | ref<TEMPLATE_STRING>)))> {};
  struct Literal : rule<ref<NullLitRule> | ref<VoidLitRule> | ref<BoolLitRule> |
                        ref<StringLitRule> | ref<OctetLitRule> |
                        ref<RegexLitRule> | ref<NumberLitRule>> {};

  // -- Forward declarations: the expression ladder and its statement peers
  // reach each other mutually. ----
  struct Expression;
  struct Statement;
  struct Block;

  template <auto Ptr>
  static constexpr auto ExpressionsInto =
      append<Ptr>(ref<Expression>) +
      many(lit<","> + append<Ptr>(ref<Expression>));

  // `varModifier?`, folded to just the keyword's own text either spelling.
  template <auto Ptr>
  static constexpr auto VarModifierInto =
      option(assign<Ptr>(kw<"var"> | kw<"const">) |
             (lit<"("> + assign<Ptr>(kw<"var"> | kw<"const">) + lit<")">));

  struct ParenExprRule
      : rule<create<ast::ParenExpr>(
            lit<"("> + ExpressionsInto<&ast::ParenExpr::expressions> +
            lit<")">)> {};

  struct ThisExprRule : rule<create<ast::ThisExpr>(kw<"this">)> {};
  struct SuperExprRule : rule<create<ast::SuperExpr>(kw<"super">)> {};
  // `.name` with no receiver: reads the enclosing `with`'s implicit object.
  struct WithDotExprRule : rule<create<ast::MemberExpr>(
                               lit<"."> + assign<&ast::MemberExpr::property>(
                                              ref<IdentifierName>))> {};

  struct ParamRule
      : rule<create<ast::Param>(
                 assign<&ast::Param::name>(ref<Identifier>) +
                 option(lit<"="> +
                        assign<&ast::Param::defaultValue>(ref<Expression>))) //
             | create<ast::Param>(
                   option(assign<&ast::Param::name>(ref<Identifier>)) +
                   flag<&ast::Param::rest>(lit<"*">))> {};
  template <auto Ptr>
  static constexpr auto ParamListInto =
      append<Ptr>(ref<ParamRule>) +
      many(lit<","> + append<Ptr>(ref<ParamRule>));

  struct AnonFunctionExprRule
      : rule<create<ast::FunctionExpr>(
            kw<"function"> +
            option(lit<"("> +
                   option(ParamListInto<&ast::FunctionExpr::params>) +
                   lit<")">) +
            assign<&ast::FunctionExpr::body>(ref<Block>))> {};

  static constexpr auto ArraySep = lit<","> | lit<"=>">;
  static constexpr auto ArrayValue = append<&ast::ArrayLit::elements>(
      create<ast::ArraySlot>(assign<&ast::ArraySlot::value>(ref<Expression>)));
  static constexpr auto ArrayOptionalValue =
      append<&ast::ArrayLit::elements>(create<ast::ArraySlot>(
          option(assign<&ast::ArraySlot::value>(ref<Expression>))));
  struct ArrayLiteralRule
      : rule<create<ast::ArrayLit>(
            VarModifierInto<&ast::ArrayLit::varModifier> + lit<"["> +
            option((ArrayValue + many(ArraySep + ArrayOptionalValue)) |
                   (append<&ast::ArrayLit::elements>(create<ast::ArraySlot>) +
                    some(ArraySep + ArrayOptionalValue))) +
            lit<"]">)> {};

  // Any of `:`, `=`, `=>` or `,` separates a property's key from its value --
  // the language resolves the key at runtime, so the grammar does not judge.
  struct ObjectPropertyRule
      : rule<create<ast::Prop>(
            assign<&ast::Prop::key>(ref<Expression>) +
            (lit<":"> | lit<"="> | lit<"=>"> |
             lit<",">)+assign<&ast::Prop::value>(ref<Expression>))> {};
  struct ObjectLiteralRule
      : rule<create<ast::ObjectLit>(
            VarModifierInto<&ast::ObjectLit::varModifier> + lit<"%["> +
            option(
                append<&ast::ObjectLit::properties>(ref<ObjectPropertyRule>) +
                many(lit<","> + append<&ast::ObjectLit::properties>(
                                    ref<ObjectPropertyRule>))) +
            option(lit<",">) + lit<"]">)> {};

  struct PrimaryExpr
      : rule<ref<ThisExprRule> | ref<SuperExprRule> | ref<WithDotExprRule> |
             ref<Literal> | ref<ArrayLiteralRule> | ref<ObjectLiteralRule> |
             ref<ParenExprRule> | ref<AnonFunctionExprRule> | ref<Identifier>> {
  };

  // -- Member / call / new / cast (one mutually recursive tier: `new` binds
  // no looser than a member or call chain, and its own result re-enters the
  // same chain) ----
  struct ArgumentRule
      : rule<create<ast::Arg>(assign<&ast::Arg::value>(ref<Expression>) +
                              option(flag<&ast::Arg::star>(lit<"*">))) //
             | create<ast::Arg>(flag<&ast::Arg::star>(lit<"*">))> {};
  template <class T>
  static constexpr auto ArgumentsInto =
      lit<"("> +
      (flag<&T::spreadArgs>(lit<"...">) |
       option((append<&T::args>(ref<ArgumentRule>) +
               many(lit<","> +
                    append<&T::args>(ref<ArgumentRule> | create<ast::Arg>))) |
              (append<&T::args>(create<ast::Arg>) +
               some(lit<","> +
                    append<&T::args>(ref<ArgumentRule> | create<ast::Arg>))))) +
      lit<")">;

  struct MemberCallExpr;
  struct NewExprRule
      : rule<create<ast::NewExpr>(
            kw<"new"> + assign<&ast::NewExpr::callee>(ref<MemberCallExpr>) +
            option(ArgumentsInto<ast::NewExpr>))> {};
  // TJS2's own documentation lists `int`/`real`/`string` casts beside calls
  // and postfix operators, its tightest step -- so `(int)
  // y + 1` is `((int) y) + 1`, and the operand is the same tight chain
  // `new` takes.
  static constexpr auto TypeConverterText =
      kw<"int"> | kw<"real"> | kw<"string">;
  struct TypeConvertExprRule
      : rule<create<ast::TypeConvertExpr>(
                 lit<"("> +
                 assign<&ast::TypeConvertExpr::targetType>(TypeConverterText) +
                 lit<")"> +
                 assign<&ast::TypeConvertExpr::operand>(ref<MemberCallExpr>)) //
             |
             create<ast::TypeConvertExpr>(
                 assign<&ast::TypeConvertExpr::targetType>(TypeConverterText) +
                 assign<&ast::TypeConvertExpr::operand>(ref<MemberCallExpr>))> {
  };
  struct MemberCallExpr
      : rule<(
            ref<PrimaryExpr> | ref<NewExprRule> |
            ref<TypeConvertExprRule>)+many(nest<ast::MemberExpr,
                                                &ast::MemberExpr::object>(
                                               flag<
                                                   &ast::MemberExpr::computed>(
                                                   lit<"[">) +
                                               ExpressionsInto<
                                                   &ast::MemberExpr::indices> +
                                               lit<"]">) //
                                           |
                                           nest<ast::MemberExpr,
                                                &ast::MemberExpr::object>(
                                               lit<"."> +
                                               assign<
                                                   &ast::MemberExpr::property>(
                                                   ref<IdentifierName>)) //
                                           |
                                           nest<ast::CallExpr,
                                                &ast::CallExpr::callee>(
                                               ArgumentsInto<ast::CallExpr>) //
                                           |
                                           nest<ast::PostfixUpdateExpr,
                                                &ast::PostfixUpdateExpr::
                                                    operand>(
                                               assign<
                                                   &ast::PostfixUpdateExpr::op>(
                                                   lit<"++"> | lit<"--">)) //
                                           | nest<ast::IsValidExpr,
                                                  &ast::IsValidExpr::operand>(
                                                 kw<"isvalid">) //
                                           | nest<ast::EvalExpr,
                                                  &ast::EvalExpr::operand>(
                                                 lit<"!", !cr<"=">>))> {};
  // Every prefix operator (`#` -- not a token TJS2 defines at all, see
  // ast::CharCodeExpr -- `& * delete isvalid typeof invalidate
  // ++ -- + - ~ !`) is one mutually recursive tier rather than the strict
  // rule-for-rule chain, matching how the sibling javascript/typescript
  // ports treat their own equal-standing prefix operators.
  struct UnaryChainExpr
      : rule<create<ast::CharCodeExpr>(
                 lit<"#"> +
                 assign<&ast::CharCodeExpr::operand>(ref<UnaryChainExpr>)) //
             | create<ast::ReferenceExpr>(
                   lit<"&", !cr<"&=">> +
                   assign<&ast::ReferenceExpr::operand>(ref<UnaryChainExpr>)) //
             | create<ast::PointerExpr>(
                   lit<"*", !lit<"=">> +
                   assign<&ast::PointerExpr::operand>(ref<UnaryChainExpr>)) //
             | create<ast::DeleteExpr>(
                   kw<"delete"> +
                   assign<&ast::DeleteExpr::operand>(ref<UnaryChainExpr>)) //
             | create<ast::IsValidExpr>(
                   kw<"isvalid"> +
                   assign<&ast::IsValidExpr::operand>(ref<UnaryChainExpr>)) //
             | create<ast::TypeofExpr>(
                   kw<"typeof"> +
                   assign<&ast::TypeofExpr::operand>(ref<UnaryChainExpr>)) //
             | create<ast::InvalidateExpr>(
                   kw<"invalidate"> + assign<&ast::InvalidateExpr::operand>(
                                          ref<UnaryChainExpr>)) //
             | create<ast::PrefixUpdateExpr>(
                   assign<&ast::PrefixUpdateExpr::op>(lit<"++"> | lit<"--">) +
                   assign<&ast::PrefixUpdateExpr::operand>(
                       ref<UnaryChainExpr>)) //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(lit<"+", !cr<"+=">> |
                                               lit<"-", !cr<"-=">> | lit<"~"> |
                                               lit<"!", !cr<"=">>) +
                   assign<&ast::UnaryExpr::operand>(ref<UnaryChainExpr>)) //
             | ref<MemberCallExpr>> {};

  // The binary ladder: eleven levels. `<->` and `incontextof` (both
  // TJS-specific) sit at the tightest step, and `instanceof`/`in` join the
  // relational step -- the same grouping the sibling javascript port gives
  // `instanceof`/`in`.
  struct BinaryExpr
      : infix<UnaryChainExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"<->"> | kw<"incontextof">) >>
               left(lit<"*", !lit<"=">> | lit<"/", !lit<"=">> |
                    lit<"%", !lit<"=">> | lit<"\\", !lit<"=">>) >>
               left(lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>) >>
               left(lit<">>>", !lit<"=">> | lit<"<<", !lit<"=">> |
                    lit<">>", !cr<"=>">>) >>
               left(lit<"<="> | lit<">="> | lit<"<", !cr<"=<">> |
                    lit<">", !cr<"=>">> | kw<"instanceof"> | kw<"in">) >>
               left(lit<"==="> | lit<"!=="> | lit<"==", !lit<"=">> |
                    lit<"!=", !lit<"=">>) >>
               left(lit<"&", !cr<"&=">>) >>  //
               left(lit<"^", !lit<"=">>) >>  //
               left(lit<"|", !cr<"|=">>) >>  //
               left(lit<"&&", !lit<"=">>) >> //
               left(lit<"||", !lit<"=">>))> {};

  struct TernaryExpr
      : rule<create<ast::TernaryExpr>(
                 assign<&ast::TernaryExpr::test>(ref<BinaryExpr>) +
                 lit<"?", !cr<".?">> +
                 assign<&ast::TernaryExpr::consequent>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::TernaryExpr::alternate>(ref<Expression>)) //
             | ref<BinaryExpr>> {};

  // Right-associative: TJS2's own documentation has `a = b = c = 0` assign
  // to c, then b, then a.
  struct AssignExpr
      : infix<TernaryExpr, &ast::AssignExpr::target, &ast::AssignExpr::op,
              &ast::AssignExpr::value,
              right(lit<"=", !cr<"=>">> | lit<"*="> | lit<"/="> | lit<"%="> |
                    lit<"+="> | lit<"-="> | lit<"<<="> | lit<">>>="> |
                    lit<">>="> | lit<"&="> | lit<"^="> | lit<"|="> |
                    lit<"\\="> | lit<"&&="> | lit<"||=">)> {};

  // Loosest: `stmt-as-expr if cond, cond2` -- the whole rest of the
  // expression is the body an `if` modifies.
  struct Expression
      : rule<create<ast::IfExpr>(assign<&ast::IfExpr::body>(ref<AssignExpr>) +
                                 kw<"if"> +
                                 ExpressionsInto<&ast::IfExpr::tests>) //
             | ref<AssignExpr>> {};

  // -- Statements ---- No line-break-sensitive ASI: a statement needs `;`
  // unless it is the document's very last token. `implied(lit<";">, eof)`,
  // not a bare `lit<";"> | eof` choice: the latter puts eof's own follow --
  // everything a fresh statement can open on -- into the lookahead of every
  // state that ends an operand, and `/` loses to a `REGEX` reading of
  // whatever runs to the next later `/` (`a / b / c` relexed `/ b /` whole).
  // `implied` carries no such automaton (see docs/reference/grammar.md).
  static constexpr auto Eos = implied(lit<";">, eof);

  struct Block
      : rule<create<ast::Block>(
            lit<"{"> + many(append<&ast::Block::body>(ref<Statement>)) +
            lit<"}">)> {};

  struct DeclaratorRule : rule<create<ast::Declarator>(
                              assign<&ast::Declarator::id>(ref<Identifier>) +
                              option(lit<"="> + assign<&ast::Declarator::init>(
                                                    ref<Expression>)))> {};
  struct VarDeclRule
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::kind>(kw<"var"> | kw<"const">) +
            append<&ast::VarDecl::declarations>(ref<DeclaratorRule>) +
            many(lit<","> +
                 append<&ast::VarDecl::declarations>(ref<DeclaratorRule>)))> {};
  struct VariableStatement : rule<ref<VarDeclRule> + Eos> {};

  struct EmptyStatement : rule<create<ast::EmptyStmt>(lit<";">)> {};
  struct ExpressionStatement
      : rule<create<ast::ExprStmt>(
            ExpressionsInto<&ast::ExprStmt::expressions> + Eos)> {};

  struct IfStatement
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> + ExpressionsInto<&ast::IfStmt::test> +
            lit<")"> + assign<&ast::IfStmt::consequent>(ref<Statement>) +
            option(commit(kw<"else">, assign<&ast::IfStmt::alternate>(ref<Statement>))))> {};

  struct DoWhileStatement
      : rule<create<ast::DoWhileStmt>(
            kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
            kw<"while"> + lit<"("> + ExpressionsInto<&ast::DoWhileStmt::test> +
            lit<")"> + Eos)> {};
  struct WhileStatement
      : rule<create<ast::WhileStmt>(
            kw<"while"> + lit<"("> + ExpressionsInto<&ast::WhileStmt::test> +
            lit<")"> + assign<&ast::WhileStmt::body>(ref<Statement>))> {};
  struct ForClassicStatement
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            option(ExpressionsInto<&ast::ForStmt::initExprs>) + lit<";"> +
            option(ExpressionsInto<&ast::ForStmt::test>) + lit<";"> +
            option(ExpressionsInto<&ast::ForStmt::update>) + lit<")"> +
            assign<&ast::ForStmt::body>(ref<Statement>))> {};
  struct ForVarStatement
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            assign<&ast::ForStmt::initDecl>(ref<VarDeclRule>) + lit<";"> +
            option(ExpressionsInto<&ast::ForStmt::test>) + lit<";"> +
            option(ExpressionsInto<&ast::ForStmt::update>) + lit<")"> +
            assign<&ast::ForStmt::body>(ref<Statement>))> {};
  struct IterationStatement
      : rule<ref<DoWhileStatement> | ref<WhileStatement> |
             ref<ForClassicStatement> | ref<ForVarStatement>> {};

  struct ContinueStatement
      : rule<create<ast::ContinueStmt>(kw<"continue"> + Eos)> {};
  struct BreakStatement : rule<create<ast::BreakStmt>(kw<"break"> + Eos)> {};
  struct ReturnStatement
      : rule<create<ast::ReturnStmt>(
            kw<"return"> + option(ExpressionsInto<&ast::ReturnStmt::argument>) +
            Eos)> {};
  struct WithStatement
      : rule<create<ast::WithStmt>(
            kw<"with"> + lit<"("> + ExpressionsInto<&ast::WithStmt::object> +
            lit<")"> + assign<&ast::WithStmt::body>(ref<Statement>))> {};

  struct CaseClause
      : rule<create<ast::SwitchCase>(
            kw<"case"> + ExpressionsInto<&ast::SwitchCase::test> + lit<":"> +
            many(append<&ast::SwitchCase::consequent>(ref<Statement>)))> {};
  struct DefaultClause
      : rule<create<ast::SwitchCase>(
            kw<"default"> + lit<":"> +
            many(append<&ast::SwitchCase::consequent>(ref<Statement>)))> {};
  struct SwitchStatement
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> + lit<"("> +
            ExpressionsInto<&ast::SwitchStmt::discriminant> + lit<")"> +
            lit<"{"> +
            many(append<&ast::SwitchStmt::cases>(ref<CaseClause>) //
                 | append<&ast::SwitchStmt::cases>(ref<DefaultClause>)) +
            lit<"}">)> {};

  struct ThrowStatement
      : rule<create<ast::ThrowStmt>(
            kw<"throw"> + ExpressionsInto<&ast::ThrowStmt::argument> + Eos)> {};

  struct CatchClauseRule
      : rule<create<ast::CatchClause>(
            kw<"catch"> +
            option(lit<"("> +
                   option(assign<&ast::CatchClause::param>(ref<Identifier>)) +
                   lit<")">) +
            assign<&ast::CatchClause::body>(ref<Block>))> {};
  struct TryStatement
      : rule<create<ast::TryStmt>(
            kw<"try"> + assign<&ast::TryStmt::block>(ref<Block>) +
            assign<&ast::TryStmt::handler>(ref<CatchClauseRule>))> {};

  struct DebuggerStatement
      : rule<create<ast::DebuggerStmt>(kw<"debugger"> + Eos)> {};

  struct FunctionDeclRule
      : rule<create<ast::FunctionDecl>(
            kw<"function"> +
            assign<&ast::FunctionDecl::name>(ref<PlainNameTok>) +
            option(lit<"("> +
                   option(ParamListInto<&ast::FunctionDecl::params>) +
                   lit<")">) +
            assign<&ast::FunctionDecl::body>(ref<Block>))> {};

  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<PlainNameTok>) +
            option(kw<"extends"> +
                   append<&ast::ClassDecl::superClasses>(ref<Expression>) +
                   many(lit<","> + append<&ast::ClassDecl::superClasses>(
                                       ref<Expression>))) +
            assign<&ast::ClassDecl::body>(ref<Block>))> {};

  struct AccessorRule
      : rule<create<ast::Accessor>(
                 flag<&ast::Accessor::isGetter>(kw<"getter">) +
                 option(lit<"("> + lit<")">) +
                 assign<&ast::Accessor::body>(ref<Block>)) //
             | create<ast::Accessor>(
                   flag<&ast::Accessor::isSetter>(kw<"setter">) +
                   lit<"("> + assign<&ast::Accessor::param>(ref<ParamRule>) +
                   lit<")"> + assign<&ast::Accessor::body>(ref<Block>))> {};
  struct PropertyDeclaration
      : rule<create<ast::PropertyDecl>(
            kw<"property"> +
            assign<&ast::PropertyDecl::name>(ref<PlainNameTok>) + lit<"{"> +
            many(append<&ast::PropertyDecl::accessors>(ref<AccessorRule>)) +
            lit<"}">)> {};

  struct Statement
      : rule<ref<Block> | ref<VariableStatement> | ref<EmptyStatement> |
             ref<ClassDeclaration> | ref<ExpressionStatement> |
             ref<IfStatement> | ref<IterationStatement> |
             ref<ContinueStatement> | ref<BreakStatement> |
             ref<ReturnStatement> | ref<WithStatement> | ref<SwitchStatement> |
             ref<ThrowStatement> | ref<TryStatement> | ref<DebuggerStatement> |
             ref<FunctionDeclRule> | ref<PropertyDeclaration>> {};

  struct ProgramRule : rule<create<ast::Program>(
                           many(append<&ast::Program::body>(ref<Statement>)))> {
  };

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
  static constexpr auto word_boundary = IdChar | cr<R"(\\)">;
};

} // namespace kirikiritjs
