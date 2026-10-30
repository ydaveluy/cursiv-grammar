// A syntax subset of Hazelight Unreal AngelScript.
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
  struct IdentifierToken : terminal<IDBODY> {};
  struct ID;

  // A generic's `>` never takes the first byte of `>=`: `a<b>=c` compares.
  static constexpr auto Lt = lit<"<">;
  static constexpr auto Gt = lit<">", !cr<"=">>;
  static constexpr auto AssignLit = lit<"=">;

  // -- Literals ----
  static constexpr auto HEXDIGIT = cr<"0-9a-fA-F">;
  struct INT_LIT : terminal<
      (lit<"0"> + cr<"xX"> + some(HEXDIGIT)) |
      (lit<"0"> + cr<"bB"> + some(cr<"01">)) |
      (lit<"0"> + cr<"oO"> + some(cr<"0-7">)) |
      (lit<"0"> + cr<"dD"> + some(d)) | some(d)> {};
  static constexpr auto DIGITSEQ = some(d);
  static constexpr auto FRACT =
      (option(DIGITSEQ) + lit<"."> + DIGITSEQ) | (DIGITSEQ + lit<".">);
  static constexpr auto EXP = cr<"eE"> + option(cr<"+-">) + DIGITSEQ;
  static constexpr auto FLOATBODY = (FRACT + option(EXP)) | (DIGITSEQ + EXP);
  struct FLOAT_LIT : terminal<FLOATBODY + option(cr<"fF">)> {};

  static constexpr auto ESCAPE =
      lit<"\\"> + ((lit<"u"> + repeat<4>(HEXDIGIT)) |
                   (lit<"U"> + repeat<8>(HEXDIGIT)) |
                   cr<R"('"?\\\\abfnrtv)">                             //
                   | (lit<"\r"> + option(lit<"\n">))                 //
                   | lit<"\n">                                       //
                   | (cr<"0-7"> + option(cr<"0-7">) + option(cr<"0-7">))  //
                   | (lit<"x"> + some(HEXDIGIT)));

  static constexpr auto CHARBODY = lit<"'"> +
                                    some(ESCAPE | (!cr<R"('\\\r\n)"> + any)) +
                                    lit<"'">;
  struct CHAR_LIT : terminal<CHARBODY> {};

  static constexpr auto STRINGBODY =
      through(lit<"\"\"\"">, lit<"\"\"\"">) |
      (option(lit<"n"> | lit<"f">) + lit<"\""> +
       many(ESCAPE | (!cr<R"("\\\r\n)"> + any)) + lit<"\"">);
  struct STRING_LIT : terminal<STRINGBODY> {};

  static constexpr auto BuiltinTypeWord =
      kw<"int8"> | kw<"int16"> | kw<"int32"> | kw<"int64"> | kw<"int"> |
      kw<"uint8"> | kw<"uint16"> | kw<"uint32"> | kw<"uint64"> | kw<"uint"> |
      kw<"float32"> | kw<"float64"> | kw<"float"> | kw<"double"> | kw<"bool"> | kw<"void"> | kw<"auto">;

  static constexpr auto Reserved = BuiltinTypeWord |
      kw<"and"> | kw<"break"> | kw<"case"> | kw<"cast"> | kw<"catch"> |
      kw<"class"> | kw<"const"> | kw<"continue"> | kw<"default"> | kw<"do"> |
      kw<"else"> | kw<"enum"> | kw<"false"> | kw<"for"> | kw<"foreach"> |
      kw<"funcdef"> | kw<"if"> | kw<"import"> | kw<"in"> | kw<"inout"> |
      kw<"interface"> | kw<"is"> | kw<"mixin"> | kw<"namespace"> | kw<"not"> |
      kw<"null"> | kw<"or"> | kw<"out"> | kw<"private"> | kw<"protected"> |
      kw<"return"> | kw<"switch"> | kw<"true"> | kw<"try"> | kw<"typedef"> |
      kw<"using"> | kw<"while"> | kw<"xor"> | kw<"nullptr"> | kw<"delegate"> | kw<"event">;
  struct ID : rule<as<std::string_view>(!Reserved + ref<IdentifierToken>)> {};

  static constexpr auto AccessWord = kw<"private"> | kw<"protected"> | kw<"public">;
  static constexpr auto PostSpecWord = kw<"override"> | kw<"final"> | kw<"property">;

  static constexpr auto XorTok = lit<"^^"> | lit<"^">;
  // A two-byte operator built from two guarded single tokens (so a nested
  // generic's `>>` still closes as two `>`). Its bytes must be adjacent.
  struct ShiftOp : rule<(Lt + gap::none + Lt) | (Gt + gap::none + Gt)> {};
  static constexpr auto AssignOp = AssignLit | lit<"*="> | lit<"/="> |
                                    lit<"%="> | lit<"+="> | lit<"-="> |
                                    lit<">>="> | lit<"<<="> | lit<"&="> |
                                    lit<"^="> | lit<"|=">;

  // Operator overloads use ordinary opAdd/opAssign methods in AngelScript.
  static constexpr auto UnqualifiedIdText = ref<ID> | (lit<"~"> + ref<ID>);

  // A nested-name-specifier's text, e.g. `NS::Sub::`. A leading `decltype`
  // scope is not accepted.
  static constexpr auto NestedNameSpecifierText =
      option(ref<ID>) + lit<"::"> + many(ref<ID> + lit<"::">);

  // `assign<>`/`append<>` read text off a SINGLE part (a ref, an
  // alternation of single parts) -- a multi-part sequence needs a rule of
  // its own before it can be named as one, hence these two wrappers.
  struct QualifiedName
      : rule<option(NestedNameSpecifierText) + ref<ID>> {};

  struct IdText
      : rule<option(NestedNameSpecifierText) + UnqualifiedIdText> {};

  // -- Forward declarations: the grammar is mutually recursive top to bottom.
  struct Declaration;
  struct BlockDeclaration;
  struct Specifiers;
  struct SimpleTypeSpecifier;
  struct ClassSpecifier;
  struct EnumSpecifier;
  struct ElaboratedTypeSpec;
  struct AsGeneric;
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

  struct Annotation
      : rule<create<ast::Annotation>(
            assign<&ast::Annotation::name>(ref<ID>) +
            option(AssignLit +
                   assign<&ast::Annotation::value>(ref<Expression>)))> {};

  static constexpr auto TypeAnnotationBody = lit<"("> +
      option(some(append<&ast::UType::annotations>(ref<Annotation>), lit<",">)) + lit<")">;
  struct UType : rule<assign<&ast::UType::keyword>(kw<"UCLASS"> | kw<"USTRUCT">) + TypeAnnotationBody> {};
  struct UEnum : rule<assign<&ast::UType::keyword>(kw<"UENUM">) + TypeAnnotationBody> {};

  struct UProperty
      : rule<create<ast::UPropertyAnno>(
            kw<"UPROPERTY"> + lit<"("> +
            option(some(append<&ast::UPropertyAnno::annotations>(
                            ref<Annotation>),
                        lit<",">)) +
            lit<")">)> {};

  struct UFunction
      : rule<create<ast::UFunctionAnno>(
            kw<"UFUNCTION"> + lit<"("> +
            option(some(append<&ast::UFunctionAnno::annotations>(
                            ref<Annotation>),
                        lit<",">)) +
            lit<")">)> {};

  // -- Types ----

  struct BuiltinTypeSpec
      : rule<create<ast::BuiltinTypeSpec>(
            assign<&ast::BuiltinTypeSpec::keyword>(BuiltinTypeWord))> {};

  struct NamedTypeSpec
      : rule<create<ast::NamedTypeSpec>(
            assign<&ast::NamedTypeSpec::name>(
                ref<QualifiedName>))> {};

  struct AsGeneric
      : rule<create<ast::GenericTypeSpec>(
            assign<&ast::GenericTypeSpec::name>(ref<ID>) + Lt +
            some(append<&ast::GenericTypeSpec::args>(ref<Specifiers>),
                 lit<",">) +
            Gt)> {};

  struct SimpleTypeSpecifier
      : rule<ref<AsGeneric>          //
             | ref<BuiltinTypeSpec>  //
             | ref<NamedTypeSpec>> {};

  struct ElaboratedTypeSpec
      : rule<create<ast::ElaboratedTypeSpec>(
            assign<&ast::ElaboratedTypeSpec::keyword>(kw<"class"> |
                                                       kw<"struct">) +
            assign<&ast::ElaboratedTypeSpec::name>(
                ref<QualifiedName>))  //
             | create<ast::ElaboratedTypeSpec>(
                   assign<&ast::ElaboratedTypeSpec::keyword>(kw<"enum">) +
                   assign<&ast::ElaboratedTypeSpec::name>(
                       ref<QualifiedName>))> {};

  struct BaseSpecifier : rule<assign<&ast::BaseSpecifier::name>(ref<QualifiedName>)> {};

  struct ClassSpecifier
      : rule<create<ast::ClassDef>(
            option(assign<&ast::ClassDef::utype>(ref<UType>)) +
            assign<&ast::ClassDef::keyword>(kw<"class"> | kw<"struct">) +
            assign<&ast::ClassDef::name>(ref<QualifiedName>) +
            option(flag<&ast::ClassDef::isFinal>(kw<"final">)) +
            option(lit<":"> +
                   some(append<&ast::ClassDef::bases>(ref<BaseSpecifier>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::ClassDef::members>(ref<MemberDeclaration>)) +
            lit<"}">)> {};

  struct Enumerator
      : rule<create<ast::Enumerator>(
            assign<&ast::Enumerator::name>(ref<ID>) +
            option(AssignLit + assign<&ast::Enumerator::value>(
                                    ref<ConditionalExpression>)))> {};

  struct EnumSpecifier
      : rule<create<ast::EnumDef>(
            option(assign<&ast::EnumDef::utype>(ref<UEnum>)) + kw<"enum"> +
            assign<&ast::EnumDef::name>(ref<QualifiedName>) +
            option(lit<":"> + assign<&ast::EnumDef::base>(ref<Specifiers>)) +
            lit<"{"> +
            option(some(append<&ast::EnumDef::enumerators>(ref<Enumerator>),
                        lit<",">) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct OpaqueEnumDecl
      : rule<create<ast::EnumForwardDecl>(
            kw<"enum"> + assign<&ast::EnumForwardDecl::name>(ref<ID>) +
            option(lit<":"> +
                   assign<&ast::EnumForwardDecl::base>(ref<Specifiers>)) +
            lit<";">)> {};

  static constexpr auto Qualifier = append<&ast::Specifiers::parts>(
      create<ast::QualifierSpec>(assign<&ast::QualifierSpec::keyword>(
          kw<"const"> | lit<"&"> | kw<"in"> | kw<"out"> | kw<"inout">)));
  struct Specifiers : rule<many(Qualifier) +
      append<&ast::Specifiers::parts>(ref<SimpleTypeSpecifier>) +
      many(Qualifier)> {};
  struct DefinitionSpecifiers : rule<append<&ast::Specifiers::parts>(ref<ClassSpecifier> | ref<EnumSpecifier>)> {};
  struct TypeDefinition : rule<assign<&ast::VarDecl::specifiers>(ref<DefinitionSpecifiers>) + option(lit<";">)> {};

  // -- Initializers ----

  struct BracedInitList : rule<create<ast::ListInit>(lit<"{"> +
      option(some(append<&ast::ListInit::items>(ref<InitializerClause>), lit<",">) + option(lit<",">)) + lit<"}">)> {};

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
             option(flag<&ast::Params::isConst>(kw<"const">))> {};

  // Only the mandatory final parameter list is kept: no function-pointer-
  // style or array-before-params prefix before it.
  struct Declarator
      : rule<assign<&ast::Declarator::name>(ref<IdText>) +
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
             | (assign<&ast::MemberDeclarator::name>(ref<ID>) +
                option(assign<&ast::MemberDeclarator::init>(ref<BraceOrEqualInitializer>)))> {};

  // -- Expressions ----

  struct LiteralItem
      : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(ref<INT_LIT>))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::text>(ref<FLOAT_LIT>))  //
             | create<ast::CharLit>(
                   assign<&ast::CharLit::text>(ref<CHAR_LIT>))  //
             | create<ast::BoolLit>(kw<"false">)                    //
             | create<ast::BoolLit>(
                   flag<&ast::BoolLit::value>(kw<"true">))  //
             | create<ast::NullLit>(kw<"nullptr">)> {};

  struct LiteralSeq
      : rule<some(append<&ast::LiteralSeq::items>(create<ast::StringLit>(assign<&ast::StringLit::text>(ref<STRING_LIT>)))) |
             append<&ast::LiteralSeq::items>(ref<LiteralItem>)> {};

  struct IdExpr
      : rule<create<ast::IdExpr>(assign<&ast::IdExpr::name>(ref<IdText>))> {};

  struct AssertExpr
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
  struct ConstructExpr
      : rule<create<ast::ConstructExpr>(
            assign<&ast::ConstructExpr::type>(ref<BuiltinTypeSpec> |
                                              ref<AsGeneric>) +
            ((lit<"("> +
              option(some(append<&ast::ConstructExpr::args>(ref<InitializerClause>),
                          lit<",">)) +
              lit<")">)  //
             | assign<&ast::ConstructExpr::list>(ref<BracedInitList>)))> {};

  struct CastExpr
      : rule<create<ast::CastExpr>(
            kw<"Cast"> + Lt + assign<&ast::CastExpr::type>(ref<Specifiers>) +
            Gt + lit<"("> +
            assign<&ast::CastExpr::operand>(ref<Expression>) + lit<")">)> {};

  struct PrimaryExpression
      : rule<ref<LiteralSeq>       //
             | ref<AssertExpr>     //
             | ref<CastExpr>       //
             | ref<ConstructExpr>  //
             | create<ast::ThisExpr>(kw<"this">)               //
             | create<ast::ParenExpr>(
                   lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                   lit<")">)   //
             | ref<IdExpr>> {};

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
                        assign<&ast::MemberExpr::field>(ref<IdText>))  //
                  | nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
                        assign<&ast::PostfixOpExpr::op>(lit<"++"> |
                                                         lit<"--">)))> {};

  // Object references use ordinary values; C++ pointer prefixes are absent.
  struct UnaryExpression
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"+"> |
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
  struct MulAddLevel : infix<UnaryExpression, ast::BinaryExpr,
                             levels(left(OpMul), left(OpAdd))> {};

  struct ShiftLadder : infix<MulAddLevel, ast::BinaryExpr, left(ref<ShiftOp>)> {};

  struct BinaryLadder
      : infix<ShiftLadder, ast::BinaryExpr,
              levels(left(lit<"&">),   //
                     left(BitXor),     //
                     left(lit<"|">),   //
                     left(OpRel),      //
                     left(OpEq),       //
                     left(lit<"&&">),  //
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

  struct Expression : rule<ref<AssignmentExpression>> {};
  struct ForStep : infix<AssignmentExpression, ast::BinaryExpr, left(lit<",">)> {};

  struct Condition : rule<ref<Expression>> {};

  // -- Statements ----

  struct ExprStmtRule
      : rule<option(assign<&ast::ExprStmt::value>(ref<Expression>)) +
             lit<";">> {};

  struct CompoundStatement
      : rule<lit<"{"> +
             option(some(append<&ast::CompoundStmt::items>(ref<Statement>))) +
             lit<"}">> {};

  struct IfStmt
      : rule<kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<Condition>) + lit<")"> +
             assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
             option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct SwitchStmt
      : rule<kw<"switch"> + lit<"("> +
             assign<&ast::SwitchStmt::value>(ref<Condition>) + lit<")"> +
             assign<&ast::SwitchStmt::body>(ref<Statement>)> {};

  struct WhileStmt
      : rule<kw<"while"> + lit<"("> +
             assign<&ast::WhileStmt::cond>(ref<Condition>) + lit<")"> +
             assign<&ast::WhileStmt::body>(ref<Statement>)> {};

  struct DoStmt
      : rule<kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) +
             kw<"while"> + lit<"("> +
             assign<&ast::DoStmt::cond>(ref<Expression>) + lit<")"> +
             lit<";">> {};

  // A statement-scoped declaration always requires a decl-specifier-seq: a
  // bare `assignmentExpression;` reading with no specifiers at all is
  // dropped here in favor of ExprStmt, which parses the same tokens and
  // would otherwise fork the engine on every expression statement in a
  // document.
  struct DeclStmt : rule<assign<&ast::VarDecl::specifiers>(ref<Specifiers>) +
      some(append<&ast::VarDecl::declarators>(ref<InitDeclarator>), lit<",">) + lit<";">> {};

  struct ForRangeDecl
      : rule<create<ast::ForRangeDecl>(
            assign<&ast::ForRangeDecl::specifiers>(ref<Specifiers>) +
            assign<&ast::ForRangeDecl::name>(ref<ID>))> {};

  struct ForStmt
      : rule<kw<"for"> + lit<"("> +
             ((assign<&ast::ForStmt::init>(ref<ExprStmtRule> |
                                            ref<DeclStmt>) +
               option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
               lit<";"> +
               option(assign<&ast::ForStmt::step>(ref<ForStep>)))  //
              | (assign<&ast::ForStmt::rangeDecl>(ref<ForRangeDecl>) +
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
> {};

  struct LabeledStatement
      : rule<create<ast::CaseLabel>(
                   kw<"case"> +
                   assign<&ast::CaseLabel::value>(ref<ConditionalExpression>) +
                   lit<":"> +
                   option(assign<&ast::CaseLabel::body>(ref<Statement>)))  //
             | create<ast::DefaultLabel>(
                   kw<"default"> + lit<":"> +
                   option(assign<&ast::DefaultLabel::body>(ref<Statement>)))> {};

  struct Statement
      : rule<ref<LabeledStatement>  //
             | ref<DeclStmt>    //
             | ref<ExprStmtRule>    //
             | ref<CompoundStatement>  //
             | ref<IfStmt>      //
             | ref<SwitchStmt>  //
             | ref<WhileStmt>   //
             | ref<DoStmt>      //
             | ref<ForStmt>     //
             | ref<JumpStatement>> {};

  // -- Declarations ----

  struct EmptyDecl : rule<create<ast::EmptyDecl>(lit<";">)> {};

  struct ModuleImport
      : rule<create<ast::ModuleImport>(
                 kw<"import"> + some(append<&ast::ModuleImport::path>(ref<ID>), lit<".">) +
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

  struct NamespaceDef
      : rule<create<ast::NamespaceDef>(
            kw<"namespace"> +
            assign<&ast::NamespaceDef::name>(ref<QualifiedName>) + lit<"{"> +
            option(some(append<&ast::NamespaceDef::items>(ref<Declaration>))) +
            lit<"}">)> {};

  struct SimpleDeclaration : rule<assign<&ast::VarDecl::specifiers>(ref<Specifiers>) +
      some(append<&ast::VarDecl::declarators>(ref<InitDeclarator>), lit<",">) + lit<";">> {};
  struct ForwardSpecifiers : rule<append<&ast::Specifiers::parts>(ref<ElaboratedTypeSpec>)> {};
  struct ForwardClass : rule<assign<&ast::VarDecl::specifiers>(ref<ForwardSpecifiers>) + lit<";">> {};

  // `enum Name;` is AngelScript's forward declaration, which has no
  // elaborated type specifier: it must win over `SimpleDeclaration`.
  struct BlockDeclaration
      : rule<ref<OpaqueEnumDecl> | ref<ForwardClass> | ref<SimpleDeclaration>> {};

  struct FunctionDef
      : rule<create<ast::FunctionDef>(
            option(assign<&ast::FunctionDef::ufunction>(ref<UFunction>)) +
            option(assign<&ast::FunctionDef::access>(AccessWord)) +
            option(flag<&ast::FunctionDef::isMixin>(kw<"mixin">)) +
            option(assign<&ast::FunctionDef::specifiers>(ref<Specifiers>)) +
            assign<&ast::FunctionDef::declarator>(ref<Declarator>) +
            option(some(append<&ast::FunctionDef::postSpecifiers>(
                PostSpecWord))) +
            (assign<&ast::FunctionDef::body>(ref<CompoundStatement>)  //
             | lit<";">))> {};

  struct PropertyDef : rule<create<ast::PropertyDef>(
      option(assign<&ast::PropertyDef::uproperty>(ref<UProperty>)) +
      option(assign<&ast::PropertyDef::access>(AccessWord)) +
      option(flag<&ast::PropertyDef::isDefault>(kw<"default">)) +
      option(assign<&ast::PropertyDef::specifiers>(ref<Specifiers>)) +
      (some(append<&ast::PropertyDef::declarators>(ref<MemberDeclarator>), lit<",">) |
       assign<&ast::PropertyDef::expr>(ref<AssignmentExpression>)) + lit<";">)> {};

  struct MemberDeclaration
      : rule<ref<PropertyDef> | ref<FunctionDef> |
             ref<EmptyDecl>> {};

  struct DelegateDecl : rule<create<ast::DelegateDecl>((flag<&ast::DelegateDecl::isEvent>(kw<"event">) | kw<"delegate">) +
      assign<&ast::DelegateDecl::result>(ref<Specifiers>) +
      assign<&ast::DelegateDecl::declarator>(ref<Declarator>) + lit<";">)> {};

  struct Declaration
      : rule<ref<DelegateDecl> | ref<ModuleImport> | ref<TypeDefinition> | ref<BlockDeclaration> | ref<FunctionDef> |
             ref<NamespaceDef> | ref<EmptyDecl>> {};

  struct Script
      : rule<option(some(append<&ast::Script::items>(ref<Declaration>)))> {};

  using entry = Script;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT, PP_LINE>;
};

}  // namespace unrealangelscript
