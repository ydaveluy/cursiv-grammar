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
  struct SHEBANG : terminal<lit<"#!"> + many(cr<R"(^\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n\x00)">)> {};
  // Native nested comments advance one character at inner delimiters. The
  // recursive body leaves their slash for its caller; the outer token owns it.
  struct CommentBody
      : terminal<many((&lit<"/*"> + lit<"/"> + ref<CommentBody>) |
                      (!lit<"/*"> + !lit<"*/"> + cr<R"(^\x00)">)) +
                 lit<"*"> + &lit<"/">> {};
  struct ML_COMMENT : terminal<lit<"/*"> + ref<CommentBody> + lit<"/">> {};

  // Native TJS2 treats every wide character from U0100 as name text;
  // '$' is the character-from-code operator, and identifier escapes are not JS.
  static constexpr auto IdStart = cr<"A-Za-z_\u0100-\U0010ffff">;
  static constexpr auto IdChar = IdStart | d;
  static constexpr auto IdPart = IdChar;
  struct RawId : terminal<IdStart + many(IdPart)> {};
  static constexpr auto ReservedWordSet = kw<"break"> | kw<"continue"> | kw<"const"> | kw<"catch"> | kw<"class"> | kw<"case"> | kw<"debugger"> | kw<"default"> | kw<"delete"> | kw<"do"> | kw<"extends"> | kw<"export"> | kw<"enum"> | kw<"else"> | kw<"function"> | kw<"finally"> | kw<"false"> | kw<"for"> | kw<"global"> | kw<"getter"> | kw<"goto"> | kw<"incontextof"> | kw<"invalidate"> | kw<"instanceof"> | kw<"isvalid"> | kw<"import"> | kw<"int"> | kw<"in"> | kw<"if"> | kw<"null"> | kw<"new"> | kw<"octet"> | kw<"protected"> | kw<"property"> | kw<"private"> | kw<"public"> | kw<"return"> | kw<"real"> | kw<"synchronized"> | kw<"switch"> | kw<"static"> | kw<"setter"> | kw<"string"> | kw<"super"> | kw<"typeof"> | kw<"throw"> | kw<"this"> | kw<"true"> | kw<"try"> | kw<"void"> | kw<"var"> | kw<"while"> | kw<"NaN"> | kw<"Infinity"> | kw<"with">;
  struct PlainNameTok : terminal<!ReservedWordSet + IdStart + many(IdPart)> {};
  struct Identifier
      : rule<assign<&ast::Ident::name>(ref<PlainNameTok>)> {
  };
  struct IdentifierName : rule<create<ast::Ident>(assign<&ast::Ident::name>(
                              ref<RawId> | ReservedWordSet))> {};

  // -- Numbers ---- Hex/octal/binary literals may also carry a fractional
  // part and an exponent (`p`/`P` alongside `e`/`E`).
  static constexpr auto DecimalIntegerLiteral =
      lit<"0"> | (cr<"1-9"> + many(cr<"0-9">));
  static constexpr auto ExponentPart = cr<"eE"> + many(s) + option(cr<"+-"> + many(s)) + some(d);
  static constexpr auto BinaryExponent = cr<"pP"> + many(s) + option(cr<"+-"> + many(s)) + some(d);
  template <auto Digits> static constexpr auto BasedNumber =
      ((some(Digits) + option(lit<"."> + many(Digits))) | (lit<"."> + some(Digits))) + option(BinaryExponent);
  struct NUMBER : terminal<
      (DecimalIntegerLiteral + lit<"."> + many(d) + option(ExponentPart)) |
      (lit<".", !lit<".">> + some(d) + option(ExponentPart)) |
      (DecimalIntegerLiteral + option(ExponentPart)) |
      (lit<"0"> + cr<"xX"> + BasedNumber<cr<"0-9a-fA-F">>) |
      (lit<"0"> + cr<"bB"> + BasedNumber<cr<"01">>) |
      (lit<"0"> + some(cr<"0-7">) + option(lit<"."> + many(cr<"0-7">)) + option(BinaryExponent))> {};

  // -- Strings ---- Escapes are kept raw, only enough to not hide the closer.
  static constexpr auto EscapeSeq = lit<"\\"> + cr<R"(^\x00)">;
  struct STRING
      : terminal<(lit<"\""> + many(EscapeSeq | cr<R"(^"\\\x00)">) + lit<"\"">) |
                 (lit<"'"> + many(EscapeSeq | cr<R"(^'\\\x00)">) + lit<"'">)> {
  };
  // `@"..."`/`@'...'`: only its own quote is escapable, unlike STRING above.
  struct TEMPLATE_STRING
      : terminal<(lit<"@\""> + many(lit<"\\\""> | cr<R"(^")">) + lit<"\"">) |
                 (lit<"@'"> + many(lit<"\\'"> | cr<"^'">) + lit<"'">)> {};
  // Native octets skip comments before testing their closing delimiter.
  // Source CR/CRLF line boundaries match the native newline-unification stage.
  struct OCTET : terminal<lit<"<%"> + many(
      ref<ML_COMMENT> | (ref<SL_COMMENT> + eol) |
      (!lit<"%>"> + !lit<"/*"> + !lit<"//"> + cr<R"(^\x00)">)) + lit<"%>">> {};

  // The native regex lexer does not give [] a delimiter-quoting role.
  // Raw newlines are allowed; only lowercase letters extend its flag token.
  static constexpr auto RegexBackslash = lit<"\\"> + cr<R"(^\x00)">;
  static constexpr auto RegexChar = RegexBackslash | cr<R"(^/\\\x00)">;
  struct REGEX
      : terminal<lit<"/"> +
                 (RegexBackslash | cr<R"(^*/\\\x00)">) + many(RegexChar) +
                 lit<"/"> + many(cr<"a-z">)> {};

  struct NullLit : rule<create<ast::NullLit>(kw<"null">)> {};
  struct VoidLit : rule<create<ast::VoidLit>(kw<"void">)> {};
  struct BOOL_TOK : terminal<as<bool>(kw<"true"> | kw<"false">)> {};
  struct BoolLit : rule<assign<&ast::BoolLit::value>(ref<BOOL_TOK>)> {};
  struct NumberLit : rule<assign<&ast::NumberLit::text>(ref<NUMBER> | kw<"NaN"> | kw<"Infinity">)> {};
  struct OctetLit : rule<assign<&ast::OctetLit::text>(ref<OCTET>)> {};
  struct RegexLit : rule<assign<&ast::RegexLit::text>(ref<REGEX>)> {};
  struct StringLit
      : rule<create<ast::StringLit>(some(append<&ast::StringLit::parts>(
            ref<STRING> | ref<TEMPLATE_STRING>)))> {};
  struct Literal : rule<ref<NullLit> | ref<VoidLit> | ref<BoolLit> |
                        ref<StringLit> | ref<OctetLit> |
                        ref<RegexLit> | ref<NumberLit>> {};

  // -- Forward declarations: the expression ladder and its statement peers
  // reach each other mutually. ----
  struct Expression;
  struct Statement;
  struct Block;
  struct UnaryChainExpr;
  struct AssignExpr;

  template <auto Ptr>
  static constexpr auto ExpressionsInto =
      some(append<Ptr>(ref<Expression>), lit<",">);

  // `varModifier?`, folded to just the keyword's own text either spelling.
  template <auto Ptr>
  static constexpr auto VarModifierInto =
      option(assign<Ptr>(kw<"var"> | kw<"const">) |
             (lit<"("> + assign<Ptr>(kw<"var"> | kw<"const">) + lit<")">));

  struct ParenExpr
      : rule<create<ast::ParenExpr>(
            lit<"("> + ExpressionsInto<&ast::ParenExpr::expressions> +
            lit<")">)> {};

  struct ThisExpr : rule<create<ast::ThisExpr>(kw<"this">)> {};
  struct GlobalExpr : rule<create<ast::GlobalExpr>(kw<"global">)> {};
  struct SuperExpr : rule<create<ast::SuperExpr>(kw<"super">)> {};
  // `.name` with no receiver: reads the enclosing `with`'s implicit object.
  struct WithDotExpr : rule<create<ast::MemberExpr>(
                               lit<"."> + assign<&ast::MemberExpr::property>(
                                              ref<IdentifierName>))> {};

  static constexpr auto TypeName = ref<PlainNameTok> | kw<"void"> | kw<"int"> | kw<"real"> | kw<"string"> | kw<"octet">;

  struct Param
      : rule<create<ast::Param>(
                 assign<&ast::Param::name>(ref<Identifier>) +
                 option(lit<":"> + assign<&ast::Param::typeName>(TypeName)) +
                 option(lit<"="> +
                        assign<&ast::Param::defaultValue>(ref<AssignExpr>))) //
             | create<ast::Param>(
                   option(assign<&ast::Param::name>(ref<Identifier>) +
                          option(lit<":"> + assign<&ast::Param::typeName>(TypeName))) +
                   flag<&ast::Param::rest>(lit<"*">))> {};
  template <auto Ptr>
  static constexpr auto ParamListInto =
      some(append<Ptr>(ref<Param>), lit<",">);

  struct AnonFunctionExpr
      : rule<create<ast::FunctionExpr>(
            kw<"function"> +
            option(lit<"("> +
                   option(ParamListInto<&ast::FunctionExpr::params>) +
                   lit<")">) +
            option(lit<":"> + assign<&ast::FunctionExpr::returnType>(TypeName)) +
            assign<&ast::FunctionExpr::body>(ref<Block>))> {};

  static constexpr auto ArraySep = lit<","> | lit<"=>">;
  static constexpr auto ArrayValue = append<&ast::ArrayLit::elements>(
      create<ast::ArraySlot>(assign<&ast::ArraySlot::value>(ref<AssignExpr>)));
  static constexpr auto ArrayOptionalValue =
      append<&ast::ArrayLit::elements>(create<ast::ArraySlot>(
          option(assign<&ast::ArraySlot::value>(ref<AssignExpr>))));
  struct ArrayLiteral
      : rule<create<ast::ArrayLit>(
            VarModifierInto<&ast::ArrayLit::varModifier> + lit<"["> +
            option((ArrayValue + many(ArraySep + ArrayOptionalValue)) |
                   (append<&ast::ArrayLit::elements>(create<ast::ArraySlot>) +
                    some(ArraySep + ArrayOptionalValue))) +
            lit<"]">)> {};

  // Any of `:`, `=`, `=>` or `,` separates a property's key from its value --
  // the language resolves the key at runtime, so the grammar does not judge.
  struct ObjectProperty : rule<
      (flag<&ast::Prop::identifierKey>(assign<&ast::Prop::key>(ref<Identifier>) + lit<":">) |
       (assign<&ast::Prop::key>(ref<AssignExpr>) + (lit<"=>"> | lit<",">))) +
      assign<&ast::Prop::value>(ref<AssignExpr>)> {};
  struct ObjectLiteral
      : rule<create<ast::ObjectLit>(
            VarModifierInto<&ast::ObjectLit::varModifier> + lit<"%["> +
            option(
                append<&ast::ObjectLit::properties>(ref<ObjectProperty>) +
                many(lit<","> + append<&ast::ObjectLit::properties>(
                                    ref<ObjectProperty>))) +
            option(lit<",">) + lit<"]">)> {};

  struct PrimaryExpr
      : rule<ref<ThisExpr> | ref<SuperExpr> | ref<GlobalExpr> | ref<WithDotExpr> |
             ref<Literal> | ref<ArrayLiteral> | ref<ObjectLiteral> |
             ref<ParenExpr> | ref<AnonFunctionExpr> | ref<Identifier>> {
  };

  // -- Member / call / new / cast (one mutually recursive tier: `new` binds
  // no looser than a member or call chain, and its own result re-enters the
  // same chain) ----
  struct Argument
      : rule<create<ast::Arg>(assign<&ast::Arg::value>(ref<AssignExpr>) +
                              option(flag<&ast::Arg::star>(lit<"*">))) //
             | create<ast::Arg>(flag<&ast::Arg::star>(lit<"*">))> {};
  template <class T>
  static constexpr auto ArgumentsInto =
      lit<"("> +
      (flag<&T::spreadArgs>(lit<"...">) |
       option((append<&T::args>(ref<Argument>) +
               many(lit<","> +
                    append<&T::args>(ref<Argument> | create<ast::Arg>))) |
              (append<&T::args>(create<ast::Arg>) +
               some(lit<","> +
                    append<&T::args>(ref<Argument> | create<ast::Arg>))))) +
      lit<")">;

  struct MemberCallExpr;
  struct NewExpr
      : rule<create<ast::NewExpr>(
            kw<"new"> + assign<&ast::NewExpr::callee>(ref<MemberCallExpr>) +
            option(ArgumentsInto<ast::NewExpr>))> {};
  // TJS2's own documentation lists `int`/`real`/`string` casts beside calls
  // and postfix operators, its tightest step -- so `(int)
  // y + 1` is `((int) y) + 1`, and the operand is the same tight chain
  // `new` takes.
  static constexpr auto TypeConverterText =
      kw<"int"> | kw<"real"> | kw<"string">;
  struct TypeConvertExpr
      : rule<create<ast::TypeConvertExpr>(
                 lit<"("> +
                 assign<&ast::TypeConvertExpr::targetType>(TypeConverterText) +
                 lit<")"> +
                 assign<&ast::TypeConvertExpr::operand>(ref<UnaryChainExpr>)) //
             |
             create<ast::TypeConvertExpr>(
                 assign<&ast::TypeConvertExpr::targetType>(TypeConverterText) +
                 assign<&ast::TypeConvertExpr::operand>(ref<UnaryChainExpr>))> {
  };
  struct MemberCallExpr
      : rule<(
            ref<PrimaryExpr> | ref<NewExpr>)+many(nest<ast::MemberExpr,
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
                                           | nest<ast::EvalExpr,
                                                  &ast::EvalExpr::operand>(
                                                 lit<"!", !cr<"=">>))> {};
  // Native incontextof is right-associative above unary; instanceof and
  // postfix isvalid are native unary productions, not relational operators.
  struct ContextExpr : infix<MemberCallExpr, ast::BinaryExpr, right(kw<"incontextof">)> {};
  struct InstanceExpr : rule<ref<ContextExpr> + option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
      assign<&ast::BinaryExpr::op>(kw<"instanceof">) +
      assign<&ast::BinaryExpr::right>(ref<UnaryChainExpr>)) |
      nest<ast::IsValidExpr, &ast::IsValidExpr::operand>(kw<"isvalid">))> {};
  struct UnaryChainExpr
      : rule<create<ast::CharFromCodeExpr>(lit<"$"> +
                 assign<&ast::CharFromCodeExpr::operand>(ref<UnaryChainExpr>)) |
             ref<TypeConvertExpr> | create<ast::CharCodeExpr>(
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
             | ref<InstanceExpr>> {};

  // Native arithmetic through logical precedence. Swap belongs with assignment.
  struct BinaryExpr
      : infix<UnaryChainExpr, ast::BinaryExpr,
              levels(left(lit<"*", !lit<"=">> | lit<"/", !lit<"=">> |
                          lit<"%", !lit<"=">> | lit<"\\", !lit<"=">>),
                     left(lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>),
                     left(lit<">>>", !lit<"=">> | lit<"<<", !lit<"=">> |
                          lit<">>", !cr<"=>">>),
                     left(lit<"<="> | lit<">="> | lit<"<", !cr<"=<">> |
                          lit<">", !cr<"=>">> | kw<"in">),
                     left(lit<"==="> | lit<"!=="> | lit<"==", !lit<"=">> |
                          lit<"!=", !lit<"=">>),
                     left(lit<"&", !cr<"&=">>),   //
                     left(lit<"^", !lit<"=">>),   //
                     left(lit<"|", !cr<"|=">>),   //
                     left(lit<"&&", !lit<"=">>),  //
                     left(lit<"||", !lit<"=">>))> {};

  struct TernaryExpr
      : rule<create<ast::TernaryExpr>(
                 assign<&ast::TernaryExpr::test>(ref<BinaryExpr>) +
                 lit<"?", !cr<".?">> +
                 assign<&ast::TernaryExpr::consequent>(ref<TernaryExpr>) +
                 lit<":"> +
                 assign<&ast::TernaryExpr::alternate>(ref<TernaryExpr>)) //
             | ref<BinaryExpr>> {};

  // Right-associative: TJS2's own documentation has `a = b = c = 0` assign
  // to c, then b, then a.
  struct AssignExpr
      : infix<TernaryExpr, ast::AssignExpr,
              right(lit<"<->"> | lit<"=", !cr<"=>">> | lit<"*="> | lit<"/="> | lit<"%="> |
                    lit<"+="> | lit<"-="> | lit<"<<="> | lit<">>>="> |
                    lit<">>="> | lit<"&="> | lit<"^="> | lit<"|="> |
                    lit<"\\="> | lit<"&&="> | lit<"||=">),
              &ast::AssignExpr::target, &ast::AssignExpr::op,
              &ast::AssignExpr::value> {};

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

  struct Declarator : rule<create<ast::Declarator>(
                              assign<&ast::Declarator::id>(ref<Identifier>) +
                              option(lit<":"> + assign<&ast::Declarator::typeName>(TypeName)) +
                              option(lit<"="> + assign<&ast::Declarator::init>(
                                                    ref<AssignExpr>)))> {};
  struct VarDecl
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::kind>(kw<"var"> | kw<"const">) +
            some(append<&ast::VarDecl::declarations>(ref<Declarator>), lit<",">))> {};
  struct VariableStatement : rule<ref<VarDecl> + Eos> {};

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
            assign<&ast::ForStmt::initDecl>(ref<VarDecl>) + lit<";"> +
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
            many(append<&ast::SwitchStmt::leading>(ref<Statement>)) +
            many(append<&ast::SwitchStmt::cases>(ref<CaseClause>) //
                 | append<&ast::SwitchStmt::cases>(ref<DefaultClause>)) +
            lit<"}">)> {};

  struct ThrowStatement
      : rule<create<ast::ThrowStmt>(
            kw<"throw"> + ExpressionsInto<&ast::ThrowStmt::argument> + Eos)> {};

  struct CatchClause
      : rule<create<ast::CatchClause>(
            kw<"catch"> +
            option(lit<"("> +
                   option(assign<&ast::CatchClause::param>(ref<Identifier>)) +
                   lit<")">) +
            assign<&ast::CatchClause::body>(ref<Statement>))> {};
  struct TryStatement
      : rule<create<ast::TryStmt>(
            kw<"try"> + assign<&ast::TryStmt::body>(ref<Statement>) +
            assign<&ast::TryStmt::handler>(ref<CatchClause>))> {};

  struct DebuggerStatement
      : rule<create<ast::DebuggerStmt>(kw<"debugger"> + Eos)> {};

  struct FunctionDecl
      : rule<create<ast::FunctionDecl>(
            kw<"function"> +
            assign<&ast::FunctionDecl::name>(ref<PlainNameTok>) +
            option(lit<"("> +
                   option(ParamListInto<&ast::FunctionDecl::params>) +
                   lit<")">) +
            option(lit<":"> + assign<&ast::FunctionDecl::returnType>(TypeName)) +
            assign<&ast::FunctionDecl::body>(ref<Block>))> {};

  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<PlainNameTok>) +
            option(kw<"extends"> +
                   append<&ast::ClassDecl::superClasses>(ref<AssignExpr>) +
                   many(lit<","> + append<&ast::ClassDecl::superClasses>(
                                       ref<Expression>))) +
            assign<&ast::ClassDecl::body>(ref<Block>))> {};

  struct Accessor
      : rule<create<ast::Accessor>(
                 flag<&ast::Accessor::isGetter>(kw<"getter">) +
                 option(lit<"("> + lit<")">) +
                 option(lit<":"> + assign<&ast::Accessor::returnType>(TypeName)) +
                 assign<&ast::Accessor::body>(ref<Block>)) //
             | create<ast::Accessor>(
                   flag<&ast::Accessor::isSetter>(kw<"setter">) +
                   lit<"("> + assign<&ast::Accessor::param>(ref<Param>) +
                   lit<")"> + assign<&ast::Accessor::body>(ref<Block>))> {};
  struct PropertyDeclaration
      : rule<create<ast::PropertyDecl>(
            kw<"property"> +
            assign<&ast::PropertyDecl::name>(ref<PlainNameTok>) + lit<"{"> +
            many(append<&ast::PropertyDecl::accessors>(ref<Accessor>)) +
            lit<"}">)> {};

  struct Statement
      : rule<ref<Block> | ref<VariableStatement> | ref<EmptyStatement> |
             ref<ClassDeclaration> | ref<ExpressionStatement> |
             ref<IfStatement> | ref<IterationStatement> |
             ref<ContinueStatement> | ref<BreakStatement> |
             ref<ReturnStatement> | ref<WithStatement> | ref<SwitchStatement> |
             ref<ThrowStatement> | ref<TryStatement> | ref<DebuggerStatement> |
             ref<FunctionDecl> | ref<PropertyDeclaration>> {};

  struct Program : rule<create<ast::Program>(
                           option(lit<"\xEF\xBB\xBF">) + option(ref<SHEBANG>) + many(append<&ast::Program::body>(ref<Statement>)))> {
  };

  using entry = Program;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
  static constexpr auto word_boundary = IdChar;
};

} // namespace kirikiritjs
