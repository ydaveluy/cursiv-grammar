// The unrealangelscript language: its grammar, adapted from a C++14 grammar
// for Unreal's Angelscript dialect. A few inherited rules are unreachable
// from `script` and so carry nothing here.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "unrealangelscript/core/ast.hpp"

namespace unrealangelscript {

struct UnrealAngelscript : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // A preprocessor branch always takes the `#if`/`#ifdef` side and discards
  // `#else` bodily: no macro table, no condition evaluated.
  struct PP_LINE
      : terminal<through(lit<"#else">, lit<"#endif">) |
                 ((lit<"#if"> | lit<"#ifdef"> | lit<"#else"> |
                   lit<"#endif">) +
                  many(cr<R"(^\r\n)">))> {};

  static constexpr auto IDBODY = cr<"a-zA-Z_"> + many(w);
  struct ID : terminal<IDBODY> {};

  // A generic's `>` never takes the first byte of `>=`: `a<b>=c` compares.
  static constexpr auto Lt = lit<"<">;
  static constexpr auto Gt = lit<">", !cr<"=">>;
  static constexpr auto AssignLit = lit<"=">;

  // -- Literals ----
  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;
  static constexpr auto DECRUN = many(option(lit<"'">) + d);
  static constexpr auto INTSUFFIX =
      (cr<"uU"> + option(lit<"ll"> | lit<"LL"> | cr<"lL">)) |
      ((lit<"ll"> | lit<"LL"> | cr<"lL">) + option(cr<"uU">));
  static constexpr auto INTBODY =
      (cr<"1-9"> + DECRUN) |
      ((lit<"0x"> | lit<"0X">) + HEXDIGIT + many(option(lit<"'">) + HEXDIGIT)) |
      ((lit<"0b"> | lit<"0B">) + cr<"01"> + many(option(lit<"'">) + cr<"01">)) |
      (lit<"0"> + many(option(lit<"'">) + cr<"0-7">));
  struct INT_LIT : terminal<INTBODY + option(INTSUFFIX)> {};

  static constexpr auto DIGITSEQ = d + many(option(lit<"'">) + d);
  static constexpr auto FRACT =
      (option(DIGITSEQ) + lit<"."> + DIGITSEQ) | (DIGITSEQ + lit<".">);
  static constexpr auto EXP = cr<"eE"> + option(cr<"+-">) + DIGITSEQ;
  static constexpr auto FLOATBODY = (FRACT + option(EXP)) | (DIGITSEQ + EXP);
  struct FLOAT_LIT : terminal<FLOATBODY + option(cr<"flFL">)> {};

  static constexpr auto ESCAPE =
      lit<"\\"> + (cr<R"('"?\\\\abfnrtv)">                             //
                   | (lit<"\r"> + option(lit<"\n">))                 //
                   | lit<"\n">                                       //
                   | (cr<"0-7"> + option(cr<"0-7">) + option(cr<"0-7">))  //
                   | (lit<"x"> + some(HEXDIGIT)));

  static constexpr auto CHARBODY = option(cr<"uUL">) + lit<"'"> +
                                    some(ESCAPE | (!cr<R"('\\\r\n)"> + any)) +
                                    lit<"'">;
  struct CHAR_LIT : terminal<CHARBODY> {};

  static constexpr auto STRINGBODY =
      through(lit<"\"\"\"">, lit<"\"\"\"">) |
      (option(lit<"n"> | lit<"f">) + lit<"\""> +
       many(ESCAPE | (!cr<R"("\\)"> + any)) + lit<"\"">);
  struct STRING_LIT : terminal<STRINGBODY> {};

  // A user-defined literal's suffix is legal syntax, but AngelScript has no
  // such feature, so it is never used in practice.
  struct USER_LIT
      : terminal<(INTBODY | FLOATBODY | CHARBODY | STRINGBODY) + IDBODY> {};

  // `catch`, `export` and `short` are reserved words with no grammar role:
  // spelled here as ordinary identifiers, not unusable reservations.

  static constexpr auto BuiltinTypeWord =
      kw<"int8"> | kw<"int16"> | kw<"int32"> | kw<"int64"> | kw<"int"> |
      kw<"uint8"> | kw<"uint16"> | kw<"uint32"> | kw<"uint64"> | kw<"uint"> |
      kw<"float"> | kw<"double"> | kw<"bool"> | kw<"void"> | kw<"auto">;

  static constexpr auto AccessWord = kw<"private"> | kw<"protected"> | kw<"public">;
  static constexpr auto PostSpecWord = kw<"override"> | kw<"final"> | kw<"property">;

  static constexpr auto XorTok = lit<"^^"> | lit<"^">;
  // A two-byte operator built from two guarded single tokens (so a nested
  // generic's `>>` still closes as two `>`): an infix operator slot needs a
  // named data rule for a sequence, not a bare composed expr.
  struct ShiftOpRule : rule<(Lt + Lt) | (Gt + Gt)> {};
  static constexpr auto AssignOp = AssignLit | lit<"*="> | lit<"/="> |
                                    lit<"%="> | lit<"+="> | lit<"-="> |
                                    lit<">>="> | lit<"<<="> | lit<"&="> |
                                    lit<"^="> | lit<"|=">;

  // Every overloadable spelling, or none for a bare `operator` -- `/=` is
  // not one of them.
  static constexpr auto TheOperatorText = option(
      lit<"+"> | lit<"-"> | lit<"*"> | lit<"/"> | lit<"%"> | XorTok |
      lit<"&"> | lit<"|"> | lit<"~"> | lit<"!"> | AssignLit | lit<">="> |
      lit<"+="> | lit<"-="> | lit<"*="> | lit<"%="> | lit<"^="> | lit<"&="> |
      lit<"|="> | (Lt + Lt) | (Gt + Gt) | lit<">>="> | lit<"<<="> |
      lit<"=="> | lit<"!="> | lit<"<="> | lit<"&&"> | lit<"||"> | lit<"++"> |
      lit<"--"> | lit<","> | Gt | Lt | (lit<"("> + lit<")">) |
      (lit<"["> + lit<"]">));

  // A name: an identifier, `operator<spelling>`, a literal-operator-id, or a
  // destructor spelling -- captured whole, no separate node per kind.
  static constexpr auto UnqualifiedIdText =
      ref<ID>                                          //
      | (kw<"operator"> + TheOperatorText)              //
      | (kw<"operator"> + (ref<STRING_LIT> + ref<ID>))  //
      | (kw<"operator"> + ref<USER_LIT>)                //
      | (lit<"~"> + ref<ID>);

  // A nested-name-specifier's text, e.g. `NS::Sub::`. A leading `decltype`
  // scope is not accepted.
  static constexpr auto NestedNameSpecifierText =
      option(ref<ID>) + lit<"::"> + many(ref<ID> + lit<"::">);

  // `assign<>`/`append<>` read text off a SINGLE part (a ref, an
  // alternation of single parts) -- a multi-part sequence needs a rule of
  // its own before it can be named as one, hence these two wrappers.
  struct QualifiedNameRule
      : rule<option(NestedNameSpecifierText) + ref<ID>> {};

  struct IdTextRule
      : rule<option(NestedNameSpecifierText) + UnqualifiedIdText> {};

  // -- Forward declarations: the grammar is mutually recursive top to bottom.
  struct Declaration;
  struct BlockDeclaration;
  struct Specifiers;
  struct SimpleTypeSpecifier;
  struct ClassSpecifier;
  struct EnumSpecifier;
  struct ElaboratedTypeSpecRule;
  struct DecltypeSpecRule;
  struct AsGenericRule;
  struct Declarator;
  struct Params;
  struct Param;
  struct InitializerClause;
  struct BracedInitList;
  struct BraceOrEqualInitializer;
  struct Statement;
  struct CompoundStatement;
  struct Expression;
  struct AssignmentExpression;
  struct ConditionalExpression;
  struct BinaryLadder;
  struct UnaryExpression;
  struct PostfixExpression;
  struct PrimaryExpression;
  struct MemberDeclaration;

  // -- Annotations ----

  struct AnnotationRule
      : rule<create<ast::Annotation>(
            assign<&ast::Annotation::name>(ref<ID>) +
            option(AssignLit +
                   assign<&ast::Annotation::value>(ref<Expression>)))> {};

  struct UTypeRule
      : rule<create<ast::UType>(
            assign<&ast::UType::keyword>(kw<"UCLASS"> | kw<"USTRUCT">) +
            lit<"("> +
            option(some(append<&ast::UType::annotations>(ref<AnnotationRule>),
                        lit<",">)) +
            lit<")">)> {};

  struct UPropertyRule
      : rule<create<ast::UPropertyAnno>(
            kw<"UPROPERTY"> + lit<"("> +
            option(some(append<&ast::UPropertyAnno::annotations>(
                            ref<AnnotationRule>),
                        lit<",">)) +
            lit<")">)> {};

  struct UFunctionRule
      : rule<create<ast::UFunctionAnno>(
            kw<"UFUNCTION"> + lit<"("> +
            option(some(append<&ast::UFunctionAnno::annotations>(
                            ref<AnnotationRule>),
                        lit<",">)) +
            lit<")">)> {};

  // -- Types ----

  struct BuiltinTypeSpecRule
      : rule<create<ast::BuiltinTypeSpec>(
            assign<&ast::BuiltinTypeSpec::keyword>(BuiltinTypeWord))> {};

  struct NamedTypeSpecRule
      : rule<create<ast::NamedTypeSpec>(
            assign<&ast::NamedTypeSpec::name>(
                ref<QualifiedNameRule>))> {};

  struct DecltypeSpecRule
      : rule<create<ast::DecltypeSpec>(
            lit<"("> +
            (assign<&ast::DecltypeSpec::expr>(ref<Expression>)  //
             | flag<&ast::DecltypeSpec::isAuto>(kw<"auto">)) +
            lit<")">)> {};

  struct AsGenericRule
      : rule<create<ast::GenericTypeSpec>(
            assign<&ast::GenericTypeSpec::name>(ref<ID>) + Lt +
            some(append<&ast::GenericTypeSpec::args>(ref<Specifiers>),
                 lit<",">) +
            Gt)> {};

  struct SimpleTypeSpecifier
      : rule<ref<AsGenericRule>          //
             | ref<BuiltinTypeSpecRule>  //
             | ref<DecltypeSpecRule>     //
             | ref<NamedTypeSpecRule>> {};

  // The scope alone, with no trailing name at all, is legal here
  // (`struct NS::;`).
  struct OptionalQualifiedNameRule
      : rule<option(NestedNameSpecifierText) + option(ref<ID>)> {};

  struct ElaboratedTypeSpecRule
      : rule<create<ast::ElaboratedTypeSpec>(
            assign<&ast::ElaboratedTypeSpec::keyword>(kw<"class"> |
                                                       kw<"struct">) +
            assign<&ast::ElaboratedTypeSpec::name>(
                ref<OptionalQualifiedNameRule>))  //
             | create<ast::ElaboratedTypeSpec>(
                   assign<&ast::ElaboratedTypeSpec::keyword>(kw<"enum">) +
                   assign<&ast::ElaboratedTypeSpec::name>(
                       ref<QualifiedNameRule>))> {};

  static constexpr auto BaseTypeTail =
      assign<&ast::BaseSpecifier::declType>(ref<DecltypeSpecRule>) |
      assign<&ast::BaseSpecifier::name>(ref<QualifiedNameRule>);

  struct BaseSpecifierRule
      : rule<BaseTypeTail |
             (flag<&ast::BaseSpecifier::isVirtual>(kw<"virtual">) +
              option(assign<&ast::BaseSpecifier::access>(AccessWord)) +
              BaseTypeTail) |
             (assign<&ast::BaseSpecifier::access>(AccessWord) +
              option(flag<&ast::BaseSpecifier::isVirtual>(
                  kw<"virtual">)) +
              BaseTypeTail)> {};

  struct ClassSpecifier
      : rule<create<ast::ClassDef>(
            option(assign<&ast::ClassDef::utype>(ref<UTypeRule>)) +
            assign<&ast::ClassDef::keyword>(kw<"class"> | kw<"struct">) +
            option(assign<&ast::ClassDef::name>(
                       ref<QualifiedNameRule>) +
                   option(flag<&ast::ClassDef::isFinal>(kw<"final">))) +
            option(lit<":"> +
                   some(append<&ast::ClassDef::bases>(ref<BaseSpecifierRule>),
                        lit<",">)) +
            lit<"{"> +
            option(some(
                append<&ast::ClassDef::members>(
                    ref<MemberDeclaration> |
                    create<ast::AccessLabel>(
                        assign<&ast::AccessLabel::access>(AccessWord) +
                        lit<":">)))) +
            lit<"}">)> {};

  struct EnumeratorRule
      : rule<create<ast::Enumerator>(
            assign<&ast::Enumerator::name>(ref<ID>) +
            option(AssignLit + assign<&ast::Enumerator::value>(
                                    ref<ConditionalExpression>)))> {};

  struct EnumSpecifier
      : rule<create<ast::EnumDef>(
            kw<"enum"> +
            option(assign<&ast::EnumDef::name>(
                ref<QualifiedNameRule>)) +
            option(lit<":"> + assign<&ast::EnumDef::base>(ref<Specifiers>)) +
            lit<"{"> +
            option(some(append<&ast::EnumDef::enumerators>(ref<EnumeratorRule>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct OpaqueEnumDeclRule
      : rule<create<ast::EnumForwardDecl>(
            kw<"enum"> + assign<&ast::EnumForwardDecl::name>(ref<ID>) +
            option(lit<":"> +
                   assign<&ast::EnumForwardDecl::base>(ref<Specifiers>)) +
            lit<";">)> {};

  struct Specifiers
      : rule<some(append<&ast::Specifiers::parts>(
            create<ast::QualifierSpec>(assign<&ast::QualifierSpec::keyword>(
                kw<"const"> | lit<"&"> | kw<"out"> | kw<"virtual">))  //
            | ref<SimpleTypeSpecifier>                                //
            | ref<ElaboratedTypeSpecRule>                             //
            | ref<ClassSpecifier>                                     //
            | ref<EnumSpecifier>))> {};

  // -- Initializers ----

  struct BracedInitList
      : rule<create<ast::ListInit>(
            (lit<"{"> | lit<"[">) +
            option(some(append<&ast::ListInit::items>(ref<InitializerClause>),
                        lit<",">) +
                   option(lit<",">)) +
            (lit<"}"> | lit<"]">))> {};

  struct InitializerClause
      : rule<create<ast::ExprInit>(
                 assign<&ast::ExprInit::value>(ref<AssignmentExpression>))  //
             | ref<BracedInitList>> {};

  struct BraceOrEqualInitializer
      : rule<(AssignLit + ref<InitializerClause>) | ref<BracedInitList>> {};

  // -- Declarators ----

  struct Param
      : rule<assign<&ast::Param::specifiers>(ref<Specifiers>) +
             option(assign<&ast::Param::name>(ref<ID>)) +
             option(AssignLit + assign<&ast::Param::defaultValue>(
                                     ref<InitializerClause>))> {};

  struct Params
      : rule<lit<"("> +
             option(some(append<&ast::Params::params>(ref<Param>), lit<",">)) +
             lit<")"> +
             option(flag<&ast::Params::isConst>(kw<"const">)) +
             option(assign<&ast::Params::refQualifier>(lit<"&&"> |
                                                        lit<"&">))> {};

  // Only the mandatory final parameter list is kept: no function-pointer-
  // style or array-before-params prefix before it.
  struct Declarator
      : rule<assign<&ast::Declarator::name>(ref<IdTextRule>) +
             assign<&ast::Declarator::params>(ref<Params>)> {};

  struct InitDeclarator
      : rule<assign<&ast::InitDeclarator::name>(ref<ID>) +
             option(assign<&ast::InitDeclarator::init>(
                        ref<BraceOrEqualInitializer>)  //
                    | (lit<"("> +
                       some(append<&ast::InitDeclarator::callArgs>(
                                ref<InitializerClause>),
                            lit<",">) +
                       lit<")">))> {};

  struct MemberDeclarator
      : rule<(assign<&ast::MemberDeclarator::declarator>(ref<Declarator>) +
              option(some(append<&ast::MemberDeclarator::postSpecifiers>(
                              PostSpecWord))  //
                     | assign<&ast::MemberDeclarator::init>(
                           ref<BraceOrEqualInitializer>)))  //
             | (option(assign<&ast::MemberDeclarator::name>(ref<ID>)) +
                lit<":"> + assign<&ast::MemberDeclarator::bitWidth>(
                               ref<ConditionalExpression>))  //
             | assign<&ast::MemberDeclarator::name>(ref<ID>)> {};

  // -- Expressions ----

  struct LiteralItem
      : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INT_LIT>))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::text>(ref<FLOAT_LIT>))  //
             | create<ast::CharLit>(
                   assign<&ast::CharLit::text>(ref<CHAR_LIT>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::text>(ref<STRING_LIT>))  //
             | create<ast::BoolLit>(kw<"false">)                    //
             | create<ast::BoolLit>(
                   flag<&ast::BoolLit::value>(kw<"true">))  //
             | create<ast::UserLit>(
                   assign<&ast::UserLit::text>(ref<USER_LIT>))  //
             | create<ast::NullLit>(kw<"nullptr">)> {};

  struct LiteralSeqRule
      : rule<some(append<&ast::LiteralSeq::items>(ref<LiteralItem>))> {};

  struct IdExprRule
      : rule<create<ast::IdExpr>(assign<&ast::IdExpr::name>(ref<IdTextRule>))> {};

  struct AssertExprRule
      : rule<create<ast::AssertExpr>(
            assign<&ast::AssertExpr::kind>(kw<"ensure"> | kw<"ensureAlways"> |
                                           kw<"check">) +
            lit<"("> +
            option(some(append<&ast::AssertExpr::args>(ref<InitializerClause>),
                        lit<",">)) +
            lit<")">)> {};

  // The type is restricted to builtin types: a generic or plain-name target
  // is already reachable via CallExpr / a declaration's own initializer,
  // avoiding the classic `a<b>(c)` generics-vs-comparison ambiguity.
  struct ConstructExprRule
      : rule<create<ast::ConstructExpr>(
            assign<&ast::ConstructExpr::type>(ref<BuiltinTypeSpecRule> |
                                              ref<AsGenericRule>) +
            ((lit<"("> +
              option(some(append<&ast::ConstructExpr::args>(ref<InitializerClause>),
                          lit<",">)) +
              lit<")">)  //
             | assign<&ast::ConstructExpr::list>(ref<BracedInitList>)))> {};

  struct CastExprRule
      : rule<create<ast::CastExpr>(
            kw<"Cast"> + Lt + assign<&ast::CastExpr::type>(ref<Specifiers>) +
            Gt + lit<"("> +
            assign<&ast::CastExpr::operand>(ref<Expression>) + lit<")">)> {};

  struct LambdaCaptureRule
      : rule<create<ast::LambdaCapture>(
                 flag<&ast::LambdaCapture::isThis>(kw<"this">))  //
             | create<ast::LambdaCapture>(
                   option(flag<&ast::LambdaCapture::byRef>(lit<"&">)) +
                   assign<&ast::LambdaCapture::name>(ref<ID>) +
                   option(assign<&ast::LambdaCapture::init>(
                       ref<BraceOrEqualInitializer>)))> {};

  struct LambdaExprRule
      : rule<create<ast::LambdaExpr>(
            lit<"["> +
            option((assign<&ast::LambdaExpr::captureDefault>(
                        lit<"&"> | AssignLit) +
                    option(lit<","> +
                           some(append<&ast::LambdaExpr::captures>(
                                    ref<LambdaCaptureRule>),
                                lit<",">)))  //
                   | some(append<&ast::LambdaExpr::captures>(
                              ref<LambdaCaptureRule>),
                          lit<",">)) +
            lit<"]"> +
            option(lit<"("> +
                   option(some(append<&ast::LambdaExpr::params>(ref<Param>),
                               lit<",">)) +
                   lit<")">) +
            assign<&ast::LambdaExpr::body>(ref<CompoundStatement>))> {};

  struct PrimaryExpression
      : rule<ref<LiteralSeqRule>       //
             | ref<AssertExprRule>     //
             | ref<CastExprRule>       //
             | ref<ConstructExprRule>  //
             | create<ast::ThisExpr>(kw<"this">)               //
             | create<ast::ParenExpr>(
                   lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                   lit<")">)   //
             | ref<LambdaExprRule>  //
             | ref<IdExprRule>> {};

  struct PostfixExpression
      : rule<ref<PrimaryExpression> +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> +
                      (assign<&ast::IndexExpr::index>(ref<Expression>)  //
                       | assign<&ast::IndexExpr::indexList>(
                             ref<BracedInitList>)) +
                      lit<"]">)  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        option(some(append<&ast::CallExpr::args>(
                                        ref<InitializerClause>),
                                    lit<",">)) +
                        lit<")">)  //
                  | nest<ast::MemberExpr, &ast::MemberExpr::object>(
                        lit<"."> +
                        assign<&ast::MemberExpr::field>(ref<IdTextRule>))  //
                  | nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
                        assign<&ast::PostfixOpExpr::op>(lit<"++"> |
                                                         lit<"--">)))> {};

  // `|` is a valid unary prefix operator here too, alongside
  // address-of/deref/sign/not.
  struct UnaryExpression
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"|"> | lit<"*"> |
                                                 lit<"&"> | lit<"+"> |
                                                 lit<"~"> | lit<"-"> |
                                                 lit<"!">) +
                   assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | ref<PostfixExpression>> {};

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | Lt | Gt;
  // `^^` is the logical (boolean) xor, at equality's own precedence step --
  // distinct from bitwise `^`, four steps tighter. Both share a lexer state
  // wherever either may follow, so the longer spelling still wins there.
  static constexpr auto OpEq = lit<"=="> | lit<"!="> | lit<"^^">;
  static constexpr auto BitXor = lit<"^">;

  // `<`/`>` open both the shift operator and the plain relational one: one
  // infix table can only give a token one step, so the shift level is its
  // own table, nested as the primary of the level above it (as the csharp
  // port does for the same reason).
  struct MulAddLevel
      : infix<UnaryExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(OpMul) >> left(OpAdd))> {};

  // A precedence table can only give one token one step, even across
  // separate tables -- so the shift level, whose tokens double as the
  // relational level's, is hand-rolled here instead of nested infix<>s.
  struct ShiftLadder
      : rule<ref<MulAddLevel> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<ShiftOpRule>) +
                 assign<&ast::BinaryExpr::right>(ref<MulAddLevel>)))> {};

  struct BinaryLadder
      : infix<ShiftLadder, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"&">) >>   //
               left(BitXor) >>     //
               left(lit<"|">) >>   //
               left(OpRel) >>      //
               left(OpEq) >>       //
               left(lit<"&&">) >>  //
               left(lit<"||">))> {};

  struct ConditionalExpression
      : rule<create<ast::CondExpr>(
                 assign<&ast::CondExpr::cond>(ref<BinaryLadder>) + lit<"?"> +
                 assign<&ast::CondExpr::thenValue>(ref<Expression>) +
                 lit<":"> + assign<&ast::CondExpr::elseValue>(
                                ref<AssignmentExpression>))  //
             | ref<BinaryLadder>> {};

  struct AssignmentExpression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<BinaryLadder>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<InitializerClause>))  //
             | ref<ConditionalExpression>> {};

  struct Expression
      : infix<AssignmentExpression, &ast::BinaryExpr::left,
              &ast::BinaryExpr::op, &ast::BinaryExpr::right, left(lit<",">)> {
  };

  struct ConditionRule
      : rule<ref<Expression>  //
             | create<ast::CondDecl>(
                   assign<&ast::CondDecl::specifiers>(ref<Specifiers>) +
                   assign<&ast::CondDecl::declarator>(ref<Declarator>) +
                   ((AssignLit + assign<&ast::CondDecl::init>(
                                     ref<InitializerClause>))  //
                    | assign<&ast::CondDecl::list>(
                          ref<BracedInitList>)))> {};

  // -- Statements ----

  struct ExprStmtRule
      : rule<option(assign<&ast::ExprStmt::value>(ref<Expression>)) +
             lit<";">> {};

  struct CompoundStatement
      : rule<lit<"{"> +
             option(some(append<&ast::CompoundStmt::items>(ref<Statement>))) +
             lit<"}">> {};

  struct IfStmtRule
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<ConditionRule>) + lit<")"> +
             assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct SwitchStmtRule
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<ConditionRule>) + lit<")"> +
             assign<&ast::SwitchStmt::body>(ref<Statement>)> {};

  struct WhileStmtRule
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<ConditionRule>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct DoStmtRule
      : rule<kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoStmt::cond>(ref<Expression>) + lit<")"> +
             lit<";">> {};

  // A statement-scoped declaration always requires a decl-specifier-seq: a
  // bare `assignmentExpression;` reading with no specifiers at all is
  // dropped here in favor of ExprStmt, which parses the same tokens and
  // would otherwise fork the engine on every expression statement in a
  // document.
  struct DeclStmtRule
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::specifiers>(ref<Specifiers>) +
            option(some(append<&ast::VarDecl::declarators>(ref<InitDeclarator>),
                        lit<",">)) +
            lit<";">)> {};

  struct ForRangeDeclRule
      : rule<create<ast::ForRangeDecl>(
            assign<&ast::ForRangeDecl::specifiers>(ref<Specifiers>) +
            assign<&ast::ForRangeDecl::name>(ref<ID>))> {};

  struct ForStmtRule
      : rule<kw<"for"> + lit<"("> +
             ((assign<&ast::ForStmt::init>(ref<ExprStmtRule> |
                                            ref<DeclStmtRule>) +
               option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
               lit<";"> +
               option(assign<&ast::ForStmt::step>(ref<Expression>)))  //
              | (assign<&ast::ForStmt::rangeDecl>(ref<ForRangeDeclRule>) +
                 lit<":"> + assign<&ast::ForStmt::rangeInit>(
                                ref<InitializerClause>))) +
             lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)> {};

  struct JumpStatement
      : rule<create<ast::BreakStmt>(kw<"break"> + lit<";">)        //
             | create<ast::ContinueStmt>(kw<"continue"> + lit<";">)  //
             | create<ast::ReturnStmt>(
                   kw<"return"> +
                   option(assign<&ast::ReturnStmt::value>(ref<Expression>)  //
                          | assign<&ast::ReturnStmt::list>(
                                ref<BracedInitList>)) +
                   lit<";">)  //
             | create<ast::GotoStmt>(kw<"goto"> +
                                     assign<&ast::GotoStmt::target>(ref<ID>) +
                                     lit<";">)> {};

  struct LabeledStatement
      : rule<create<ast::Label>(assign<&ast::Label::name>(ref<ID>) +
                                lit<":"> +
                                assign<&ast::Label::body>(ref<Statement>))  //
             | create<ast::CaseLabel>(
                   kw<"case"> +
                   assign<&ast::CaseLabel::value>(ref<ConditionalExpression>) +
                   lit<":"> +
                   assign<&ast::CaseLabel::body>(ref<Statement>))  //
             | create<ast::DefaultLabel>(
                   kw<"default"> + lit<":"> +
                   assign<&ast::DefaultLabel::body>(ref<Statement>))> {};

  struct Statement
      : rule<ref<LabeledStatement>  //
             | ref<DeclStmtRule>    //
             | ref<ExprStmtRule>    //
             | ref<CompoundStatement>  //
             | ref<IfStmtRule>      //
             | ref<SwitchStmtRule>  //
             | ref<WhileStmtRule>   //
             | ref<DoStmtRule>      //
             | ref<ForStmtRule>     //
             | ref<JumpStatement>> {};

  // -- Declarations ----

  struct EmptyDeclRule : rule<create<ast::EmptyDecl>(lit<";">)> {};

  struct ModuleImportRule
      : rule<create<ast::ModuleImport>(
                 kw<"import"> + append<&ast::ModuleImport::path>(ref<ID>) +
                 many(lit<"."> + append<&ast::ModuleImport::path>(ref<ID>)) +
                 lit<";">)  //
             | create<ast::ImportFunction>(
                   kw<"import"> +
                   option(assign<&ast::ImportFunction::specifiers>(
                       ref<Specifiers>)) +
                   assign<&ast::ImportFunction::declarator>(ref<Declarator>) +
                   option(some(append<&ast::ImportFunction::postSpecifiers>(
                       PostSpecWord))) +
                   kw<"from"> +
                   assign<&ast::ImportFunction::from>(ref<STRING_LIT>) +
                   lit<";">)> {};

  struct NamespaceDefRule
      : rule<create<ast::NamespaceDef>(
            kw<"namespace"> +
            option(assign<&ast::NamespaceDef::name>(ref<ID>)) + lit<"{"> +
            option(some(append<&ast::NamespaceDef::items>(ref<Declaration>))) +
            lit<"}">)> {};

  struct NamespaceAliasDefRule
      : rule<create<ast::NamespaceAlias>(
            kw<"namespace"> + assign<&ast::NamespaceAlias::name>(ref<ID>) +
            AssignLit +
            assign<&ast::NamespaceAlias::target>(
                ref<QualifiedNameRule>) +
            lit<";">)> {};

  struct AliasDeclRule
      : rule<create<ast::AliasDecl>(
            assign<&ast::AliasDecl::name>(ref<ID>) + AssignLit +
            assign<&ast::AliasDecl::type>(ref<Specifiers>) + lit<";">)> {};

  struct SimpleDeclaration
      : rule<create<ast::VarDecl>(
            option(assign<&ast::VarDecl::specifiers>(ref<Specifiers>)) +
            option(some(append<&ast::VarDecl::declarators>(ref<InitDeclarator>),
                        lit<",">)  //
                   | assign<&ast::VarDecl::expr>(ref<AssignmentExpression>)) +
            lit<";">)> {};

  // `enum Name;` is AngelScript's forward declaration, which has no
  // elaborated type specifier: it must win over `SimpleDeclaration`.
  struct BlockDeclaration
      : rule<ref<OpaqueEnumDeclRule> | ref<SimpleDeclaration> |
             ref<NamespaceAliasDefRule> | ref<AliasDeclRule>> {};

  struct FunctionDef
      : rule<create<ast::FunctionDef>(
            option(assign<&ast::FunctionDef::ufunction>(ref<UFunctionRule>)) +
            option(assign<&ast::FunctionDef::access>(AccessWord)) +
            option(flag<&ast::FunctionDef::isMixin>(kw<"mixin">)) +
            option(assign<&ast::FunctionDef::specifiers>(ref<Specifiers>)) +
            assign<&ast::FunctionDef::declarator>(ref<Declarator>) +
            option(some(append<&ast::FunctionDef::postSpecifiers>(
                PostSpecWord))) +
            (assign<&ast::FunctionDef::body>(ref<CompoundStatement>)  //
             | (flag<&ast::FunctionDef::isDefaulted>(
                    AssignLit + kw<"default">) +
                lit<";">)  //
             | lit<";">))> {};

  struct PropertyDef
      : rule<create<ast::PropertyDef>(
            option(assign<&ast::PropertyDef::uproperty>(ref<UPropertyRule>)) +
            option(assign<&ast::PropertyDef::access>(AccessWord)) +
            option(flag<&ast::PropertyDef::isDefault>(kw<"default">)) +
            option(assign<&ast::PropertyDef::specifiers>(ref<Specifiers>)) +
            option(some(append<&ast::PropertyDef::declarators>(
                            ref<MemberDeclarator>),
                        lit<",">)  //
                   | assign<&ast::PropertyDef::expr>(
                         ref<AssignmentExpression>)) +
            lit<";">)> {};

  struct MemberDeclaration
      : rule<ref<PropertyDef> | ref<FunctionDef> | ref<AliasDeclRule> |
             ref<EmptyDeclRule>> {};

  struct Declaration
      : rule<ref<ModuleImportRule> | ref<BlockDeclaration> | ref<FunctionDef> |
             ref<NamespaceDefRule> | ref<EmptyDeclRule>> {};

  struct ScriptRule
      : rule<option(some(append<&ast::Script::items>(ref<Declaration>)))> {};

  using entry = ScriptRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT, PP_LINE>;
};

}  // namespace unrealangelscript
