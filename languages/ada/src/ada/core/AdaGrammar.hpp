// The Ada 2012 language: its grammar. A few constructs depend on a name's
// declared kind or a lexer mode, neither of which this parser tracks.
#pragma once

#include "ada/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace ada {

struct Ada : cursiv::GrammarSpec {
  // Kept: dropping it changes no tree but costs +26 % instructions.
  static constexpr auto Reserved =
      ikw<"abort"> | ikw<"abs"> | ikw<"abstract"> | ikw<"accept"> |
      ikw<"access"> | ikw<"aliased"> | ikw<"all"> | ikw<"and"> |
      ikw<"array"> | ikw<"at"> | ikw<"begin"> | ikw<"body"> | ikw<"case"> |
      ikw<"constant"> | ikw<"declare"> | ikw<"delay"> | ikw<"delta"> |
      ikw<"digits"> | ikw<"do"> | ikw<"else"> | ikw<"elsif"> | ikw<"end"> |
      ikw<"entry"> | ikw<"exception"> | ikw<"exit"> | ikw<"for"> |
      ikw<"function"> | ikw<"generic"> | ikw<"goto"> | ikw<"if"> |
      ikw<"in"> | ikw<"interface"> | ikw<"is"> | ikw<"limited"> |
      ikw<"loop"> | ikw<"mod"> | ikw<"new"> | ikw<"not"> | ikw<"null"> |
      ikw<"of"> | ikw<"or"> | ikw<"others"> | ikw<"out"> |
      ikw<"overriding"> | ikw<"package"> | ikw<"pragma"> | ikw<"private"> |
      ikw<"procedure"> | ikw<"protected"> | ikw<"raise"> | ikw<"range"> |
      ikw<"record"> | ikw<"rem"> | ikw<"renames"> | ikw<"requeue"> |
      ikw<"return"> | ikw<"reverse"> | ikw<"select"> | ikw<"separate"> |
      ikw<"some"> | ikw<"subtype"> | ikw<"synchronized"> | ikw<"tagged"> |
      ikw<"task"> | ikw<"terminate"> | ikw<"then"> | ikw<"type"> |
      ikw<"until"> | ikw<"use"> | ikw<"when"> | ikw<"while"> | ikw<"with"> |
      ikw<"xor">;

  struct WS : terminal<some(cr<R"(\t\n\v\f\r )">)> {};
  struct LINE_COMMENT : terminal<lit<"--"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto IdStart = uc::L | uc::Nl;
  static constexpr auto IdContinue =
      IdStart | uc::Mn | uc::Mc | uc::Nd | uc::Pc | uc::Cf;
  static constexpr auto word_boundary = IdContinue;
  struct ID : terminal<!Reserved + IdStart + many(IdContinue)> {};

  // Named rules, referenced through `ref<>`: a raw sequence has no value of
  // its own for `assign<>`/`append<>` to read, only a rule call does.
  static constexpr auto Id = ref<ID>;
  struct ModeTextRule : rule<option(ikw<"in">) + option(ikw<"out">)> {};
  static constexpr auto ModeText = ref<ModeTextRule>;

  // The underscore between digits is optional, not a true separator:
  // `some(x, sep)` requires sep between every pair, which "64" has none of.
  static constexpr auto NUMERAL = cr<"0-9"> + many(option(lit<"_">) + cr<"0-9">);
  static constexpr auto EXTENDED_DIGIT = cr<"0-9A-Fa-f">;
  static constexpr auto BASED_NUMERAL =
      EXTENDED_DIGIT + many(option(lit<"_">) + EXTENDED_DIGIT);
  static constexpr auto EXPONENT = cr<"Ee"> + option(cr<"+-">) + NUMERAL;
  static constexpr auto DECIMAL_LITERAL =
      NUMERAL + option(lit<"."> + NUMERAL) + option(EXPONENT);
  static constexpr auto BASED_LITERAL =
      NUMERAL +
      ((lit<"#"> + BASED_NUMERAL + option(lit<"."> + BASED_NUMERAL) +
        lit<"#">) |
       (lit<":"> + BASED_NUMERAL + option(lit<"."> + BASED_NUMERAL) +
        lit<":">)) +
      option(EXPONENT);
  struct NUMERIC_LITERAL : terminal<BASED_LITERAL | DECIMAL_LITERAL> {};

  struct CHARACTER_LITERAL : terminal<lit<"'"> + cr<R"(^\r\n)"> + lit<"'">> {};
  struct STRING_LITERAL
      : terminal<(lit<"\""> + many(lit<"\"\""> | cr<R"(^"\r\n)">) + lit<"\"">) |
                 (lit<"%"> + many(lit<"%%"> | cr<R"(^%\r\n)">) + lit<"%">)> {};

  struct QualNameRule : rule<some(Id, lit<".">)> {};
  static constexpr auto QualName = ref<QualNameRule>;
  static constexpr auto SelectorText =
      ref<ID> | ref<CHARACTER_LITERAL> | ref<STRING_LITERAL>;

  // -- Forward declarations: this grammar is mutually recursive top to
  // bottom, and `ref<R>` only needs R named, not defined. --------------

  struct Expression;
  struct Simple;
  struct SignedTerm;
  struct Term;
  struct Factor;
  struct Primary;
  struct NameExpr;
  struct Aggregate;
  struct AllocatorRule;
  struct ParenExprRule;
  struct CondExprRule;
  struct QuantifiedExprRule;
  struct SubtypeIndicationRule;
  struct AccessDefRule;
  struct ConstraintRule;
  struct RangeRule;
  struct ArrayTypeDefRule;
  struct DiscreteRange;
  struct DiscreteSubtypeDefRule;
  struct FormalPartRule;
  struct DeclItem;
  struct Stmt;
  struct StmtSeqRule;
  struct SubprogramSpecRule;
  struct TypeDefRule;
  struct AspectClauseRule;
  struct PackageSpecRule;
  struct EntryDeclRule;
  struct EntryBodyRule;
  struct TaskItemRule;
  struct ProtectedOpDeclRule;
  struct ProtectedElemDeclRule;
  struct ProtectedOperationItemRule;
  struct GenericFormalDeclRule;
  struct UseClauseRule;

  // -- Operators: each token or fixed word pair hands `op` a constant. -

  struct OpPow : terminal<lit<"**">, value<ast::Operator::power>> {};
  struct OpMul : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct OpMod : terminal<ikw<"mod">, value<ast::Operator::mod_>> {};
  struct OpRem : terminal<ikw<"rem">, value<ast::Operator::rem_>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct OpConcat : terminal<lit<"&">, value<ast::Operator::concat>> {};
  struct OpNe : terminal<lit<"/=">, value<ast::Operator::ne>> {};
  struct OpLe : terminal<lit<"<=">, value<ast::Operator::le>> {};
  struct OpGe : terminal<lit<">=">, value<ast::Operator::ge>> {};
  struct OpEq : terminal<lit<"=">, value<ast::Operator::eq>> {};
  struct OpLt : terminal<lit<"<">, value<ast::Operator::lt>> {};
  struct OpGt : terminal<lit<">">, value<ast::Operator::gt>> {};
  struct OpAndThen : rule<ikw<"and"> + ikw<"then">, value<ast::Operator::and_then>> {};
  struct OpOrElse : rule<ikw<"or"> + ikw<"else">, value<ast::Operator::or_else>> {};
  struct OpAnd : terminal<ikw<"and">, value<ast::Operator::and_>> {};
  struct OpOr : terminal<ikw<"or">, value<ast::Operator::or_>> {};
  struct OpXor : terminal<ikw<"xor">, value<ast::Operator::xor_>> {};

  static constexpr auto LvlMul =
      ref<OpMul> | ref<OpDiv> | ref<OpMod> | ref<OpRem>;
  static constexpr auto LvlAdd = ref<OpAdd> | ref<OpSub> | ref<OpConcat>;
  static constexpr auto LvlRel = ref<OpNe> | ref<OpLe> | ref<OpGe> |
                                 ref<OpEq> | ref<OpLt> | ref<OpGt>;

  // A document routinely names units and entities outside itself: no
  // `reference<>` here. `with` clauses are handled at Unit level; aspect
  // associations remain attached to their declaration.
  struct AspectNameRule : rule<QualName + option(lit<"'"> + ikw<"class">)> {};
  struct AspectRule
      : rule<assign<&ast::Aspect::name>(ref<AspectNameRule>) +
      option(lit<"=>"> + assign<&ast::Aspect::value>(ref<Expression>))> {
  };
  template <auto Field>
  static constexpr auto AspectSpecOpt =
      option(ikw<"with"> + some(append<Field>(ref<AspectRule>), lit<",">));

  // -- Leaves ------------------------------------------------------------

  struct IdentRule : rule<assign<&ast::Ident::text>(Id)> {};
  struct OpSymbolRule : rule<assign<&ast::Ident::text>(ref<STRING_LITERAL>)> {
  };
  struct SelectorRule : rule<assign<&ast::Ident::text>(SelectorText)> {};
  struct CharLitRule
      : rule<assign<&ast::CharLit::text>(ref<CHARACTER_LITERAL>)> {};
  struct StringLitRule
      : rule<assign<&ast::StringLit::text>(ref<STRING_LITERAL>)> {};
  struct NumericLitRule
      : rule<assign<&ast::NumericLit::text>(ref<NUMERIC_LITERAL>)> {};
  struct NullLitRule : rule<create<ast::NullLit>(ikw<"null">)> {};
  struct OthersRule : rule<create<ast::OthersExpr>(ikw<"others">)> {};
  struct BoxRule : rule<create<ast::BoxExpr>(lit<"<>">)> {};
  struct TerminateStmtRule
      : rule<create<ast::TerminateStmt>(ikw<"terminate"> + lit<";">)> {};

  struct DirectName : rule<ref<IdentRule> | ref<OpSymbolRule>> {};
  static constexpr auto DirectNameText = Id | ref<STRING_LITERAL>;
  // A subtype mark that denotes a class-wide subtype (`T'Class`) is common
  // in parameter/return positions; folded in here rather than left
  // unparsed.
  struct SubtypeMarkTextRule
      : rule<QualName + option(lit<"'"> + ikw<"class">)> {};
  static constexpr auto SubtypeMarkText = ref<SubtypeMarkTextRule>;
  struct SubtypeMarkExprRule : rule<assign<&ast::Ident::text>(SubtypeMarkText)> {
  };

  // -- Name/expression chain --------------------------------------------

  // Indexing, slicing, a call, a generalized indexing or a type
  // conversion: `name '(' ... ')'` in every case, told apart only by
  // whether the prefix names a type; unified here into one Call. Gap
  // `semantic-predicate`, nil price.
  // A slice's bound (`S(I..J)`) is a range, not an expression: `Expression`
  // has no `..`. One argument shape covers calls, indexing and slices.
  struct ArgRule
      : rule<create<ast::Choice>(
            option(append<&ast::Choice::tests>(ref<SelectorRule>) + lit<"=>">) +
            assign<&ast::Choice::value>(ref<BoxRule> | ref<RangeRule> |
                                        ref<CondExprRule> | ref<Expression>))> {};

  struct NameExpr
      : rule<(ref<DirectName> | ref<CharLitRule>) +
             many(
                 nest<ast::AllExpr, &ast::AllExpr::prefix>(lit<"."> +
                                                           ikw<"all">)  //
                 | nest<ast::SelectExpr, &ast::SelectExpr::prefix>(
                       lit<"."> +
                       assign<&ast::SelectExpr::selector>(SelectorText))  //
                 | nest<ast::CallExpr, &ast::CallExpr::callee>(
                       lit<"("> +
                       some(append<&ast::CallExpr::args>(ref<ArgRule>),
                            lit<",">) +
                       lit<")">)  //
                 | nest<ast::QualifiedExpr, &ast::QualifiedExpr::target>(
                       lit<"'"> +
                       assign<&ast::QualifiedExpr::value>(
                           ref<Aggregate> |
                           (lit<"("> + ref<Expression> + lit<")">)))  //
                 | nest<ast::AttrExpr, &ast::AttrExpr::prefix>(
                       lit<"'"> +
                       assign<&ast::AttrExpr::attr>(
                           ref<ID> | ikw<"access"> | ikw<"delta"> |
                           ikw<"digits"> | ikw<"mod"> | ikw<"range">) +
                       option(lit<"("> +
                              assign<&ast::AttrExpr::arg>(ref<CondExprRule> |
                                                    ref<Expression>) +
                              lit<")">)))> {};

  struct SubpoolRule : rule<lit<"("> + ref<NameExpr> + lit<")">> {};

  struct AllocatorTargetRule
      : rule<(ref<SubtypeMarkExprRule> +
              nest<ast::QualifiedExpr, &ast::QualifiedExpr::target>(
                  lit<"'"> +
                  assign<&ast::QualifiedExpr::value>(
                      ref<Aggregate> |
                      (lit<"("> + ref<Expression> + lit<")">))))  //
             | ref<SubtypeIndicationRule>> {};

  struct AllocatorRule
      : rule<create<ast::Allocator>(
            ikw<"new"> +
            option(assign<&ast::Allocator::subpool>(ref<SubpoolRule>)) +
            assign<&ast::Allocator::target>(ref<AllocatorTargetRule>))> {};

  static constexpr auto Quantifier =
      flag<&ast::QuantifiedExpr::forAll>(ikw<"all">) | ikw<"some">;

  struct QuantifiedExprRule
      : rule<create<ast::QuantifiedExpr>(
                 ikw<"for"> + Quantifier +
                 assign<&ast::QuantifiedExpr::var>(create<ast::Param>(
                     append<&ast::Param::names>(Id) + ikw<"in"> +
                     option(ikw<"reverse">) +
                     assign<&ast::Param::type>(
                         ref<DiscreteSubtypeDefRule>))) +
                 lit<"=>"> +
                 assign<&ast::QuantifiedExpr::predicate>(ref<Expression>))  //
             | create<ast::QuantifiedExpr>(
                   ikw<"for"> + Quantifier +
                   assign<&ast::QuantifiedExpr::var>(create<ast::Param>(
                       append<&ast::Param::names>(Id) +
                       option(lit<":"> +
                              assign<&ast::Param::type>(
                                  ref<SubtypeIndicationRule>)) +
                       ikw<"of"> + option(ikw<"reverse">) +
                       assign<&ast::Param::def>(ref<NameExpr>))) +
                   lit<"=>"> +
                   assign<&ast::QuantifiedExpr::predicate>(ref<Expression>))  //
             | create<ast::QuantifiedExpr>(
                   ikw<"for"> + Quantifier +
                   assign<&ast::QuantifiedExpr::var>(create<ast::Param>(
                       append<&ast::Param::names>(Id) + ikw<"in"> +
                       option(ikw<"reverse">) +
                       assign<&ast::Param::def>(ref<NameExpr>))) +
                   lit<"=>"> +
                   assign<&ast::QuantifiedExpr::predicate>(ref<Expression>))> {
  };

  struct ParenExprRule
      : rule<create<ast::GroupedExpr>(
            lit<"("> +
            assign<&ast::GroupedExpr::value>(ref<QuantifiedExprRule> | ref<CondExprRule> | ref<Expression>) +
             lit<")">)> {};

  struct Primary : rule<ref<NumericLitRule>    //
                        | ref<NullLitRule>     //
                        | ref<StringLitRule>   //
                        | ref<Aggregate>       //
                        | ref<NameExpr>        //
                        | ref<AllocatorRule>   //
                        | ref<ParenExprRule>> {};

  struct Factor
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<Primary>) +
                 assign<&ast::BinaryExpr::op>(ref<OpPow>) +
                 assign<&ast::BinaryExpr::right>(ref<Primary>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(ikw<"abs">) +
                   assign<&ast::UnaryExpr::operand>(ref<Primary>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(ikw<"not">) +
                   assign<&ast::UnaryExpr::operand>(ref<Primary>))  //
             | ref<Primary>> {};

  struct Term : infix<Factor, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
                      &ast::BinaryExpr::right, left(LvlMul)> {};

  struct SignedTerm
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Term>))  //
             | ref<Term>> {};

  struct Simple : infix<SignedTerm, &ast::BinaryExpr::left,
                        &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                        left(LvlAdd)> {};

  struct DiscreteRange : rule<ref<RangeRule> | ref<SubtypeIndicationRule>> {};

  // A discrete choice is a discrete range OR a bare choice expression
  // (`when 1 =>`) -- easy to lose sight of since both are Expr-typed and
  // `DiscreteRange` alone still compiles, just under-accepts. A bare name
  // reads as an expression: only its declaration could make it a subtype.
  static constexpr auto DiscreteChoice =
      ref<RangeRule> | ref<Expression> | ref<SubtypeIndicationRule>;

  static constexpr auto MembershipList =
      some(append<&ast::MembershipExpr::choices>(ref<DiscreteRange> |
                                                  ref<Simple>),
           lit<"|">);

  struct RaiseExprRule
      : rule<create<ast::RaiseExpr>(
            ikw<"raise"> + assign<&ast::RaiseExpr::target>(ref<NameExpr>) +
            option(ikw<"with"> +
                   assign<&ast::RaiseExpr::message>(ref<Simple>)))> {};

  struct Relation
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<Simple>) +
                 assign<&ast::BinaryExpr::op>(LvlRel) +
                 assign<&ast::BinaryExpr::right>(ref<Simple>))  //
             | create<ast::MembershipExpr>(
                   assign<&ast::MembershipExpr::operand>(ref<Simple>) +
                   option(flag<&ast::MembershipExpr::negated>(
                       ikw<"not">)) +
                   ikw<"in"> + MembershipList)  //
             | ref<RaiseExprRule>  //
             | ref<Simple>> {};

  // Read a logical chain permissively. Runtime validation checks that
  // unparenthesized links use one operator kind (Ada RM 4.4).
  struct Expression
      : infix<Relation, &ast::BinaryExpr::left,
                            &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                            left(ref<OpAndThen> | ref<OpOrElse> | ref<OpAnd> |
                                 ref<OpOr> | ref<OpXor>)> {};

  struct IfExprElsifRule
      : rule<create<ast::Choice>(
            ikw<"elsif"> + append<&ast::Choice::tests>(ref<Expression>) +
            ikw<"then"> + assign<&ast::Choice::value>(ref<Expression>))> {};

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            ikw<"if"> + assign<&ast::IfExpr::cond>(ref<Expression>) +
            ikw<"then"> + assign<&ast::IfExpr::thenValue>(ref<Expression>) +
            many(append<&ast::IfExpr::elsifs>(ref<IfExprElsifRule>)) +
            option(ikw<"else"> +
                   assign<&ast::IfExpr::elseValue>(ref<Expression>)))> {};

  struct CaseExprAltRule
      : rule<create<ast::Choice>(
            ikw<"when"> +
            some(append<&ast::Choice::tests>(DiscreteChoice |
                                             ref<OthersRule>),
                 lit<"|">) +
            lit<"=>"> + assign<&ast::Choice::value>(ref<Expression>))> {};

  struct CaseExprRule
      : rule<create<ast::CaseExpr>(
            ikw<"case"> + assign<&ast::CaseExpr::selector>(ref<Expression>) +
            ikw<"is"> +
            some(append<&ast::CaseExpr::alts>(ref<CaseExprAltRule>),
                 lit<",">))> {};

  struct CondExprRule : rule<ref<IfExprRule> | ref<CaseExprRule>> {};

  // -- Constraints, subtype indications, access definitions --------------

  struct RangeRule
      : rule<create<ast::RangeExpr>(
                 assign<&ast::RangeExpr::low>(ref<Simple>) + lit<".."> +
                 assign<&ast::RangeExpr::high>(ref<Simple>))  //
             | (ref<NameExpr> + lit<"'"> +
                nest<ast::AttrExpr, &ast::AttrExpr::prefix>(
                    assign<&ast::AttrExpr::attr>(ikw<"range">) +
                    option(lit<"("> +
                           assign<&ast::AttrExpr::arg>(ref<CondExprRule> | ref<Expression>) +
                           lit<")">)))> {};

  struct IndexConstraintRule
      : rule<create<ast::IndexConstraint>(
            lit<"("> +
            some(append<&ast::IndexConstraint::ranges>(ref<DiscreteRange>),
                 lit<",">) +
            lit<")">)> {};

  struct DiscAssocRule
      : rule<create<ast::Choice>(
            option(some(append<&ast::Choice::tests>(ref<SelectorRule>),
                        lit<"|">) +
                   lit<"=>">) +
            assign<&ast::Choice::value>(ref<Expression>))> {};

  struct DiscriminantConstraintRule
      : rule<create<ast::DiscriminantConstraint>(
            lit<"("> +
            some(append<&ast::DiscriminantConstraint::associations>(
                     ref<DiscAssocRule>),
                 lit<",">) +
            lit<")">)> {};

  struct DigitsConstraintRule
      : rule<create<ast::DigitsConstraint>(
            ikw<"digits"> + assign<&ast::DigitsConstraint::digits_>(ref<Simple>) +
            option(ikw<"range"> +
                   assign<&ast::DigitsConstraint::range>(ref<RangeRule>)))> {
  };

  struct DeltaConstraintRule
      : rule<create<ast::DeltaConstraint>(
            ikw<"delta"> + assign<&ast::DeltaConstraint::delta_>(ref<Simple>) +
            option(ikw<"range"> +
                   assign<&ast::DeltaConstraint::range>(ref<RangeRule>)))> {};

  struct RangeConstraintRule : rule<ikw<"range"> + ref<RangeRule>> {};

  // A loop's subtype is discrete (RM 5.5): a range constraint at most
  // (RM 3.2.2), so `F (X)` there is an iterator name, never a subtype.
  struct DiscreteSubtypeDefRule
      : rule<ref<RangeRule> |
             create<ast::SubtypeIndication>(
                 assign<&ast::SubtypeIndication::mark>(SubtypeMarkText) +
                 option(assign<&ast::SubtypeIndication::constraint>(
                     ref<RangeConstraintRule>)))> {};

  struct ConstraintRule
      : rule<ref<RangeConstraintRule> | ref<DigitsConstraintRule> |
             ref<DeltaConstraintRule> | ref<IndexConstraintRule> |
             ref<DiscriminantConstraintRule>> {};

  struct SubtypeIndicationRule
      : rule<create<ast::SubtypeIndication>(
            option(flag<&ast::SubtypeIndication::nullExclusion>(
                ikw<"not"> + ikw<"null">)) +
            assign<&ast::SubtypeIndication::mark>(SubtypeMarkText) +
            option(assign<&ast::SubtypeIndication::constraint>(
                ref<ConstraintRule>)))> {};

  struct AccessDefRule
      : rule<create<ast::AccessDef>(
                 option(flag<&ast::AccessDef::nullExclusion>(
                     ikw<"not"> + ikw<"null">)) +
                 ikw<"access"> +
                 option(flag<&ast::AccessDef::constant_>(ikw<"constant">) |
                        ikw<"all">) +
                 assign<&ast::AccessDef::mark>(SubtypeMarkText) +
                 option(assign<&ast::AccessDef::constraint>(
                     ref<ConstraintRule>)))  //
             | create<ast::AccessDef>(
                   option(flag<&ast::AccessDef::nullExclusion>(
                       ikw<"not"> + ikw<"null">)) +
                   ikw<"access"> +
                   option(flag<&ast::AccessDef::protected_>(
                       ikw<"protected">)) +
                   flag<&ast::AccessDef::isSubprogram>(ikw<"procedure">) +
                   option(assign<&ast::AccessDef::params>(
                       ref<FormalPartRule>)))  //
             | create<ast::AccessDef>(
                   option(flag<&ast::AccessDef::nullExclusion>(
                       ikw<"not"> + ikw<"null">)) +
                   ikw<"access"> +
                   option(flag<&ast::AccessDef::protected_>(
                       ikw<"protected">)) +
                   flag<&ast::AccessDef::isFunction>(ikw<"function">) +
                   option(assign<&ast::AccessDef::params>(
                       ref<FormalPartRule>)) +
                   ikw<"return"> + option(ikw<"not"> + ikw<"null">) +
                   assign<&ast::AccessDef::returnType>(
                       ref<AccessDefRule> | ref<SubtypeMarkExprRule>))> {
  };

  // -- Formal parts / parameters -----------------------------------------

  struct ParamSpecRule
      : rule<create<ast::Param>(
                 some(append<&ast::Param::names>(Id), lit<",">) + lit<":"> +
                 option(flag<&ast::Param::aliased_>(ikw<"aliased">)) +
                 assign<&ast::Param::mode>(ModeText) +
                 option(ikw<"not"> + ikw<"null">) +
                 assign<&ast::Param::type>(ref<SubtypeMarkExprRule>) +
                 option(lit<":="> +
                        assign<&ast::Param::def>(ref<Expression>)))  //
             | create<ast::Param>(
                   some(append<&ast::Param::names>(Id), lit<",">) + lit<":"> +
                   assign<&ast::Param::type>(ref<AccessDefRule>) +
                   option(lit<":="> +
                          assign<&ast::Param::def>(ref<Expression>)))> {};

  struct FormalPartRule
      : rule<lit<"("> +
             some(append<&ast::Params::items>(ref<ParamSpecRule>), lit<";">) +
             lit<")">> {};

  struct DiscSpecRule
      : rule<create<ast::Param>(
                 some(append<&ast::Param::names>(Id), lit<",">) + lit<":"> +
                 option(ikw<"not"> + ikw<"null">) +
                 assign<&ast::Param::type>(ref<SubtypeMarkExprRule>) +
                 option(lit<":="> +
                        assign<&ast::Param::def>(ref<Expression>)))  //
             | create<ast::Param>(
                   some(append<&ast::Param::names>(Id), lit<",">) + lit<":"> +
                   assign<&ast::Param::type>(ref<AccessDefRule>) +
                   option(lit<":="> +
                          assign<&ast::Param::def>(ref<Expression>)))> {};

  struct KnownDiscriminantsRule
      : rule<lit<"("> +
             some(append<&ast::Params::items>(ref<DiscSpecRule>), lit<";">) +
             lit<")">> {};

  static constexpr auto DiscriminantPartOpt =
      assign<&ast::TypeDecl::discriminants>(ref<KnownDiscriminantsRule>) |
      (lit<"("> + lit<"<>"> + lit<")">);

  // -- Aggregates ----------------------------------------------------

  struct AggItemRule
      : rule<create<ast::Choice>(
                 some(append<&ast::Choice::tests>(ref<SelectorRule> |
                                                  ref<OthersRule>),
                      lit<"|">) +
                 lit<"=>"> +
                 assign<&ast::Choice::value>(ref<BoxRule> | ref<Expression>))  //
             | create<ast::Choice>(
                   assign<&ast::Choice::value>(ref<Expression>))> {};

  struct ArrayAggItemRule
      : rule<create<ast::Choice>(
            some(append<&ast::Choice::tests>(DiscreteChoice |
                                             ref<OthersRule>),
                 lit<"|">) +
            lit<"=>"> +
            assign<&ast::Choice::value>(ref<BoxRule> | ref<Expression>))> {};

  struct Aggregate
      : rule<lit<"("> +
             create<ast::Aggregate>(
                 (ikw<"null"> +
                  flag<&ast::Aggregate::isNullRecord>(ikw<"record">))  //
                 | (assign<&ast::Aggregate::ancestorOrTag>(ref<Expression>) +
                    flag<&ast::Aggregate::isExtension>(ikw<"with">) +
                    ((ikw<"null"> + ikw<"record">)  //
                     | some(append<&ast::Aggregate::items>(
                                ref<AggItemRule>),
                            lit<",">)))  //
                 | some(append<&ast::Aggregate::items>(ref<ArrayAggItemRule>),
                        lit<",">)  //
                 // A lone association is named, read above: `(E)` is a
                 // parenthesized expression (RM 4.3.1(7), 4.3.3(3)).
                 | (append<&ast::Aggregate::items>(ref<AggItemRule>) +
                    lit<","> +
                    some(append<&ast::Aggregate::items>(ref<AggItemRule>),
                         lit<",">))) +
             lit<")">> {};

  // -- Statements ------------------------------------------------------

  struct NullStmtRule : rule<create<ast::NullStmt>(ikw<"null">) + lit<";">> {};

  struct AssignStmtRule
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<NameExpr>) + lit<":="> +
            assign<&ast::AssignStmt::value>(ref<Expression>) + lit<";">)> {};

  static constexpr auto CallArgs =
      lit<"("> +
      some(append<&ast::ProcCallStmt::args>(ref<ArgRule>), lit<",">) +
      lit<")">;

  struct ProcCallStmtRule
      : rule<create<ast::ProcCallStmt>(
            assign<&ast::ProcCallStmt::target>(ref<NameExpr>) +
            option(CallArgs) + lit<";">)> {};

  struct ExitStmtRule
      : rule<create<ast::ExitStmt>(
            ikw<"exit"> +
            option(assign<&ast::ExitStmt::target>(ref<NameExpr>)) +
            option(ikw<"when"> +
                   assign<&ast::ExitStmt::cond>(ref<Expression>)) +
            lit<";">)> {};

  struct GotoStmtRule
      : rule<create<ast::GotoStmt>(
            ikw<"goto"> + assign<&ast::GotoStmt::target>(ref<NameExpr>) +
            lit<";">)> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            ikw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
            lit<";">)> {};

  struct ExtReturnObjRule
      : rule<create<ast::ObjectDecl>(
            some(append<&ast::ObjectDecl::names>(Id), lit<",">) + lit<":"> +
            option(flag<&ast::ObjectDecl::aliased_>(ikw<"aliased">)) +
            option(flag<&ast::ObjectDecl::constant_>(ikw<"constant">)) +
            assign<&ast::ObjectDecl::type>(ref<AccessDefRule> |
                                           ref<SubtypeIndicationRule>) +
            option(lit<":="> +
                   assign<&ast::ObjectDecl::init>(ref<Expression>)))> {};

  struct ExtReturnStmtRule
      : rule<create<ast::ExtendedReturnStmt>(
            ikw<"return"> +
            assign<&ast::ExtendedReturnStmt::obj>(ref<ExtReturnObjRule>) +
            option(ikw<"do"> +
                   assign<&ast::ExtendedReturnStmt::body>(ref<StmtSeqRule>) +
                   ikw<"end"> + ikw<"return">) +
            lit<";">)> {};

  struct RequeueStmtRule
      : rule<create<ast::RequeueStmt>(
            ikw<"requeue"> + assign<&ast::RequeueStmt::target>(ref<NameExpr>) +
            option(ikw<"with"> +
                   flag<&ast::RequeueStmt::abort_>(ikw<"abort">)) +
            lit<";">)> {};

  struct DelayStmtRule
      : rule<create<ast::DelayStmt>(
            ikw<"delay"> +
            option(flag<&ast::DelayStmt::until_>(ikw<"until">)) +
            assign<&ast::DelayStmt::value>(ref<Expression>) + lit<";">)> {};

  struct AbortStmtRule
      : rule<create<ast::AbortStmt>(
            ikw<"abort"> +
            some(append<&ast::AbortStmt::targets>(ref<NameExpr>), lit<",">) +
            lit<";">)> {};

  struct RaiseStmtRule
      : rule<create<ast::RaiseStmt>(
            ikw<"raise"> +
            option(assign<&ast::RaiseStmt::target>(ref<NameExpr>) +
                   option(ikw<"with"> +
                          assign<&ast::RaiseStmt::message>(ref<Expression>))) +
            lit<";">)> {};

  struct PragmaArgRule
      : rule<create<ast::Choice>(
            option(append<&ast::Choice::tests>(ref<SelectorRule>) + lit<"=>">) +
            assign<&ast::Choice::value>(ref<Expression>))> {};

  struct PragmaStmtRule
      : rule<create<ast::PragmaStmt>(
            ikw<"pragma"> + assign<&ast::PragmaStmt::name>(Id) +
            option(lit<"("> +
                   some(append<&ast::PragmaStmt::args>(ref<PragmaArgRule>),
                        lit<",">) +
                   lit<")">) +
            lit<";">)> {};

  struct PragmaDeclRule
      : rule<create<ast::PragmaDecl>(
            ikw<"pragma"> + assign<&ast::PragmaDecl::name>(Id) +
            option(lit<"("> +
                   some(append<&ast::PragmaDecl::args>(ref<PragmaArgRule>),
                        lit<",">) +
                   lit<")">) +
            lit<";">)> {};

  struct SimpleStmt
      : rule<ref<NullStmtRule>       //
            | ref<AssignStmtRule>    //
            | ref<ExitStmtRule>      //
            | ref<GotoStmtRule>      //
            | ref<ReturnStmtRule>    //
            | ref<RequeueStmtRule>   //
            | ref<DelayStmtRule>     //
            | ref<AbortStmtRule>     //
            | ref<RaiseStmtRule>     //
            | ref<PragmaStmtRule>    //
            | ref<ProcCallStmtRule>> {};

  struct CondBlockRule
      : rule<create<ast::CondBlock>(
            ikw<"elsif"> + assign<&ast::CondBlock::cond>(ref<Expression>) +
            ikw<"then"> + some(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            ikw<"if"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ikw<"then"> + some(append<&ast::IfStmt::thenBody>(ref<Stmt>)) +
            many(append<&ast::IfStmt::elsifs>(ref<CondBlockRule>)) +
            option(ikw<"else"> +
                   some(append<&ast::IfStmt::elseBody>(ref<Stmt>))) +
            ikw<"end"> + ikw<"if"> + lit<";">)> {};

  struct CaseAltRule
      : rule<create<ast::CaseAlt>(
            ikw<"when"> +
            some(append<&ast::CaseAlt::choices>(DiscreteChoice |
                                                ref<OthersRule>),
                 lit<"|">) +
            lit<"=>"> + some(append<&ast::CaseAlt::body>(ref<Stmt>)))> {};

  struct CaseStmtRule
      : rule<create<ast::CaseStmt>(
            ikw<"case"> + assign<&ast::CaseStmt::selector>(ref<Expression>) +
            ikw<"is"> + some(append<&ast::CaseStmt::alts>(ref<CaseAltRule>)) +
            ikw<"end"> + ikw<"case"> + lit<";">)> {};

  struct ForSpecRule
      : rule<create<ast::ForSpec>(
                 assign<&ast::ForSpec::var>(Id) +
                 option(lit<":"> + assign<&ast::ForSpec::subtypeIndication>(
                                       ref<SubtypeIndicationRule>)) +
                 ikw<"of"> +
                 option(flag<&ast::ForSpec::reverse_>(ikw<"reverse">)) +
                 flag<&ast::ForSpec::isOf>(
                     flag<&ast::ForSpec::isIterator>(
                         assign<&ast::ForSpec::range>(ref<NameExpr>))))  //
             // A bare name reads as a subtype: only its declaration could
             // make it an iterator.
             | create<ast::ForSpec>(
                   assign<&ast::ForSpec::var>(Id) + ikw<"in"> +
                   option(flag<&ast::ForSpec::reverse_>(
                       ikw<"reverse">)) +
                   assign<&ast::ForSpec::range>(
                       ref<DiscreteSubtypeDefRule>))  //
             | create<ast::ForSpec>(
                   assign<&ast::ForSpec::var>(Id) +
                   flag<&ast::ForSpec::isIterator>(ikw<"in">) +
                   option(flag<&ast::ForSpec::reverse_>(
                       ikw<"reverse">)) +
                   assign<&ast::ForSpec::range>(ref<NameExpr>))> {};

  static constexpr auto IterScheme =
      (ikw<"while"> + assign<&ast::LoopStmt::whileCond>(ref<Expression>))  //
      | (ikw<"for"> + assign<&ast::LoopStmt::forSpec>(ref<ForSpecRule>));

  struct LoopStmtRule
      : rule<create<ast::LoopStmt>(
            option(assign<&ast::LoopStmt::label>(Id) + lit<":">) +
            option(IterScheme) + ikw<"loop"> +
            some(append<&ast::LoopStmt::body>(ref<Stmt>)) + ikw<"end"> +
            ikw<"loop"> + option(assign<&ast::LoopStmt::endLabel>(Id)) +
            lit<";">)> {};

  struct BlockStmtRule
      : rule<create<ast::BlockStmt>(
            option(assign<&ast::BlockStmt::label>(Id) + lit<":">) +
            option(ikw<"declare"> +
                   many(append<&ast::BlockStmt::decls>(ref<DeclItem>))) +
            ikw<"begin"> + assign<&ast::BlockStmt::body>(ref<StmtSeqRule>) +
            ikw<"end"> + option(DirectNameText) + lit<";">)> {};

  struct AcceptStmtRule
      : rule<create<ast::AcceptStmt>(
            ikw<"accept"> + assign<&ast::AcceptStmt::name>(Id) +
            option(lit<"("> +
                   assign<&ast::AcceptStmt::index>(ref<Expression>) +
                   lit<")">) +
            option(assign<&ast::AcceptStmt::params>(ref<FormalPartRule>)) +
            option(ikw<"do"> +
                   assign<&ast::AcceptStmt::body>(ref<StmtSeqRule>) +
                   ikw<"end"> + option(DirectNameText)) +
            lit<";">)> {};

  struct SelectAltRule
      : rule<create<ast::CondBlock>(
            option(ikw<"when"> + assign<&ast::CondBlock::cond>(ref<Expression>) +
                   lit<"=>">) +
            append<&ast::CondBlock::body>(ref<AcceptStmtRule> |
                                          ref<DelayStmtRule> |
                                          ref<TerminateStmtRule>) +
            many(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct EntryCallAltRule
      : rule<create<ast::CondBlock>(
            append<&ast::CondBlock::body>(ref<ProcCallStmtRule>) +
            many(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct DelayAltRule
      : rule<create<ast::CondBlock>(
            append<&ast::CondBlock::body>(ref<DelayStmtRule>) +
            many(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct TriggerAltRule
      : rule<create<ast::CondBlock>(
            append<&ast::CondBlock::body>(ref<ProcCallStmtRule> |
                                          ref<DelayStmtRule>) +
            many(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct SelectStmtRule
      : rule<create<ast::SelectStmt>(
                 ikw<"select"> +
                 append<&ast::SelectStmt::alts>(ref<SelectAltRule>) +
                 many(ikw<"or"> +
                      append<&ast::SelectStmt::alts>(ref<SelectAltRule>)) +
                 option(ikw<"else"> +
                        some(append<&ast::SelectStmt::elseBody>(
                            ref<Stmt>))) +
                 ikw<"end"> + ikw<"select"> + lit<";">)  //
             | create<ast::SelectStmt>(
                   ikw<"select"> +
                   append<&ast::SelectStmt::alts>(ref<EntryCallAltRule>) +
                   flag<&ast::SelectStmt::isTimed>(ikw<"or">) +
                   append<&ast::SelectStmt::alts>(ref<DelayAltRule>) +
                   ikw<"end"> + ikw<"select"> + lit<";">)  //
             | create<ast::SelectStmt>(
                   ikw<"select"> +
                   append<&ast::SelectStmt::alts>(ref<EntryCallAltRule>) +
                   flag<&ast::SelectStmt::isConditional>(ikw<"else">) +
                   some(append<&ast::SelectStmt::elseBody>(ref<Stmt>)) +
                   ikw<"end"> + ikw<"select"> + lit<";">)  //
             | create<ast::SelectStmt>(
                   ikw<"select"> +
                   append<&ast::SelectStmt::alts>(ref<TriggerAltRule>) +
                   flag<&ast::SelectStmt::isAsynchronous>(ikw<"then">) +
                   ikw<"abort"> +
                   some(append<&ast::SelectStmt::elseBody>(ref<Stmt>)) +
                   ikw<"end"> + ikw<"select"> + lit<";">)> {};

  struct CompoundStmt
      : rule<ref<IfStmtRule>          //
            | ref<CaseStmtRule>       //
            | ref<LoopStmtRule>       //
            | ref<BlockStmtRule>      //
            | ref<ExtReturnStmtRule>  //
            | ref<AcceptStmtRule>     //
            | ref<SelectStmtRule>> {};

  struct LabeledStmtRule
      : rule<create<ast::LabeledStmt>(
                 some(lit<"<<"> + append<&ast::LabeledStmt::labels>(Id) +
                      lit<">>">) +
                 assign<&ast::LabeledStmt::inner>(ref<SimpleStmt> |
                                                  ref<CompoundStmt>))  //
             | ref<SimpleStmt>  //
             | ref<CompoundStmt>> {};

  struct Stmt : rule<ref<LabeledStmtRule>> {};

  struct ExcHandlerRule
      : rule<create<ast::ExcHandler>(
            ikw<"when"> +
            option(assign<&ast::ExcHandler::name>(Id) + lit<":">) +
            some(append<&ast::ExcHandler::choices>(ref<NameExpr> |
                                                    ref<OthersRule>),
                 lit<"|">) +
            lit<"=>"> + some(append<&ast::ExcHandler::body>(ref<Stmt>)))> {};

  struct StmtSeqRule
      : rule<create<ast::StmtSeq>(
            some(append<&ast::StmtSeq::body>(ref<Stmt>)) +
            many(lit<"<<"> + Id + lit<">>">) +
            option(ikw<"exception"> +
                   some(append<&ast::StmtSeq::handlers>(
                       ref<ExcHandlerRule>))))> {};

  // -- Objects, renamings, exceptions ------------------------------------

  struct ObjectDeclRule
      : rule<create<ast::ObjectDecl>(
            some(append<&ast::ObjectDecl::names>(Id), lit<",">) + lit<":"> +
            option(flag<&ast::ObjectDecl::aliased_>(ikw<"aliased">)) +
            option(flag<&ast::ObjectDecl::constant_>(ikw<"constant">)) +
            assign<&ast::ObjectDecl::type>(ref<AccessDefRule> |
                                           ref<ArrayTypeDefRule> |
                                           ref<SubtypeIndicationRule>) +
            option(lit<":="> +
                   assign<&ast::ObjectDecl::init>(ref<Expression>)) +
            AspectSpecOpt<&ast::ObjectDecl::aspects> + lit<";">)> {};

  struct NumberDeclRule
      : rule<create<ast::ObjectDecl>(
            some(append<&ast::ObjectDecl::names>(Id), lit<",">) + lit<":"> +
            flag<&ast::ObjectDecl::constant_>(ikw<"constant">) +
            lit<":="> + assign<&ast::ObjectDecl::init>(ref<Expression>) +
            lit<";">)> {};

  struct ExceptionDeclRule
      : rule<create<ast::ExceptionDecl>(
            some(append<&ast::ExceptionDecl::names>(Id), lit<",">) +
            lit<":"> + ikw<"exception"> + AspectSpecOpt<&ast::ExceptionDecl::aspects> + lit<";">)> {};

  template <auto Negative, auto Present>
  static constexpr auto OverridingOpt =
      option(option(flag<Negative>(ikw<"not">)) +
             flag<Present>(ikw<"overriding">));

  struct SubprogramSpecRule
      : rule<create<ast::SubprogramSpec>(
                 OverridingOpt<&ast::SubprogramSpec::notOverriding,
                               &ast::SubprogramSpec::overriding_> +
                 ikw<"procedure"> +
                 assign<&ast::SubprogramSpec::name>(QualName) +
                 option(assign<&ast::SubprogramSpec::params>(
                     ref<FormalPartRule>)))  //
             | create<ast::SubprogramSpec>(
                   OverridingOpt<&ast::SubprogramSpec::notOverriding,
                                 &ast::SubprogramSpec::overriding_> +
                   flag<&ast::SubprogramSpec::isFunction>(ikw<"function">) +
                   assign<&ast::SubprogramSpec::name>(
                       QualName | ref<STRING_LITERAL>) +
                   option(assign<&ast::SubprogramSpec::params>(
                       ref<FormalPartRule>)) +
                   ikw<"return"> + option(ikw<"not"> + ikw<"null">) +
                   assign<&ast::SubprogramSpec::returnType>(
                       ref<AccessDefRule> | ref<SubtypeMarkExprRule>))> {
  };

  struct RenameDeclRule
      : rule<create<ast::RenameDecl>(
                 assign<&ast::RenameDecl::name>(Id) + lit<":"> +
                 option(ikw<"not"> + ikw<"null">) +
                 assign<&ast::RenameDecl::type>(
                     ref<AccessDefRule> | ref<SubtypeMarkExprRule>) +
                 ikw<"renames"> +
                 assign<&ast::RenameDecl::target>(ref<NameExpr>) +
                 AspectSpecOpt<&ast::RenameDecl::aspects> + lit<";">)  //
             | create<ast::RenameDecl>(
                   assign<&ast::RenameDecl::name>(Id) + lit<":"> +
                   flag<&ast::RenameDecl::isException>(
                       ikw<"exception">) +
                   ikw<"renames"> +
                   assign<&ast::RenameDecl::target>(ref<NameExpr>) +
                   AspectSpecOpt<&ast::RenameDecl::aspects> + lit<";">)  //
             | create<ast::RenameDecl>(
                   ikw<"package"> +
                   assign<&ast::RenameDecl::name>(QualName) +
                   flag<&ast::RenameDecl::isPackage>(ikw<"renames">) +
                   assign<&ast::RenameDecl::target>(ref<NameExpr>) +
                   AspectSpecOpt<&ast::RenameDecl::aspects> + lit<";">)  //
             | create<ast::RenameDecl>(
                   assign<&ast::RenameDecl::spec>(ref<SubprogramSpecRule>) +
                   ikw<"renames"> +
                   assign<&ast::RenameDecl::target>(ref<NameExpr>) +
                   AspectSpecOpt<&ast::RenameDecl::aspects> + lit<";">)  //
             | create<ast::RenameDecl>(
                   ikw<"generic"> +
                   assign<&ast::RenameDecl::genericKind>(
                       ikw<"package"> | ikw<"procedure"> | ikw<"function">) +
                   assign<&ast::RenameDecl::name>(QualName) +
                   ikw<"renames"> +
                   assign<&ast::RenameDecl::target>(ref<NameExpr>) +
                   AspectSpecOpt<&ast::RenameDecl::aspects> + lit<";">)> {};

  // -- Types ---------------------------------------------------------

  struct EnumLiteralRule
      : rule<assign<&ast::Ident::text>(Id) | ref<CharLitRule>> {};

  struct EnumTypeDefRule
      : rule<create<ast::EnumTypeDef>(
            lit<"("> +
            some(append<&ast::EnumTypeDef::literals>(ref<EnumLiteralRule>),
                 lit<",">) +
            lit<")">)> {};

  struct ModularTypeDefRule
      : rule<create<ast::ModularTypeDef>(
            ikw<"mod"> +
            assign<&ast::ModularTypeDef::modulus>(ref<Expression>))> {};

  struct IntegerTypeDefRule
      : rule<create<ast::IntegerTypeDef>(
            ikw<"range"> +
            assign<&ast::IntegerTypeDef::range>(create<ast::RangeExpr>(
                assign<&ast::RangeExpr::low>(ref<Expression>) + lit<".."> +
                assign<&ast::RangeExpr::high>(ref<Expression>))))> {};

  struct FloatTypeDefRule
      : rule<create<ast::FloatTypeDef>(
            ikw<"digits"> +
            assign<&ast::FloatTypeDef::digits_>(ref<Expression>) +
            option(ikw<"range"> +
                   assign<&ast::FloatTypeDef::range>(create<ast::RangeExpr>(
                       assign<&ast::RangeExpr::low>(ref<Expression>) +
                       lit<".."> +
                       assign<&ast::RangeExpr::high>(ref<Expression>)))))> {};

  struct OrdinaryFixedDefRule
      : rule<create<ast::OrdinaryFixedDef>(
            ikw<"delta"> +
            assign<&ast::OrdinaryFixedDef::delta_>(ref<Expression>) +
            ikw<"range"> +
            assign<&ast::OrdinaryFixedDef::range>(create<ast::RangeExpr>(
                assign<&ast::RangeExpr::low>(ref<Expression>) + lit<".."> +
                assign<&ast::RangeExpr::high>(ref<Expression>))))> {};

  struct DecimalFixedDefRule
      : rule<create<ast::DecimalFixedDef>(
            ikw<"delta"> +
            assign<&ast::DecimalFixedDef::delta_>(ref<Expression>) +
            ikw<"digits"> +
            assign<&ast::DecimalFixedDef::digits_>(ref<Expression>) +
            option(ikw<"range"> +
                   assign<&ast::DecimalFixedDef::range>(
                       create<ast::RangeExpr>(
                           assign<&ast::RangeExpr::low>(ref<Expression>) +
                           lit<".."> +
                           assign<&ast::RangeExpr::high>(
                               ref<Expression>)))))> {};

  struct IndexSubtypeDefRule
      : rule<assign<&ast::Ident::text>(QualName) + ikw<"range"> +
             lit<"<>">> {};

  struct ArrayTypeDefRule
      : rule<create<ast::ArrayTypeDef>(
                 ikw<"array"> + lit<"("> +
                 some(append<&ast::ArrayTypeDef::indexes>(
                          ref<IndexSubtypeDefRule>),
                      lit<",">) +
                 lit<")"> + ikw<"of"> +
                 option(flag<&ast::ArrayTypeDef::aliased_>(
                     ikw<"aliased">)) +
                 assign<&ast::ArrayTypeDef::componentType>(
                     ref<AccessDefRule> | ref<SubtypeIndicationRule>))  //
             | create<ast::ArrayTypeDef>(
                   ikw<"array"> + lit<"("> +
                   some(append<&ast::ArrayTypeDef::indexes>(
                            ref<DiscreteRange>),
                        lit<",">) +
                   lit<")"> +
                   flag<&ast::ArrayTypeDef::constrained>(ikw<"of">) +
                   option(flag<&ast::ArrayTypeDef::aliased_>(
                       ikw<"aliased">)) +
                   assign<&ast::ArrayTypeDef::componentType>(
                       ref<AccessDefRule> | ref<SubtypeIndicationRule>))> {};

  struct VariantPartRule;

  struct ComponentDeclRule
      : rule<create<ast::ComponentDecl>(
            some(append<&ast::ComponentDecl::names>(Id), lit<",">) +
            lit<":"> +
            option(flag<&ast::ComponentDecl::aliased_>(
                ikw<"aliased">)) +
            assign<&ast::ComponentDecl::type>(ref<AccessDefRule> |
                                              ref<SubtypeIndicationRule>) +
            option(lit<":="> +
                   assign<&ast::ComponentDecl::def>(ref<Expression>)) +
            AspectSpecOpt<&ast::ComponentDecl::aspects> + lit<";">)> {};

  struct ComponentItemRule
      : rule<ref<ComponentDeclRule> | ref<AspectClauseRule> |
             ref<PragmaDeclRule>> {};

  struct VariantRule
      : rule<create<ast::Variant>(
            ikw<"when"> +
            some(append<&ast::Variant::choices>(DiscreteChoice |
                                                ref<OthersRule>),
                 lit<"|">) +
            lit<"=>"> +
            (some(append<&ast::Variant::components>(ref<ComponentItemRule>))  //
             | (many(append<&ast::Variant::components>(ref<ComponentItemRule>)) +
                assign<&ast::Variant::nested>(ref<VariantPartRule>))  //
             | (ikw<"null"> + lit<";">)))> {};

  struct VariantPartRule
      : rule<create<ast::VariantPart>(
            ikw<"case"> +
            assign<&ast::VariantPart::discriminant>(Id) + ikw<"is"> +
            some(append<&ast::VariantPart::variants>(ref<VariantRule>)) +
            ikw<"end"> + ikw<"case"> + lit<";">)> {};

  struct RecordTypeDefRule
      : rule<create<ast::RecordTypeDef>(
                 option(option(ikw<"abstract">) +
                        flag<&ast::RecordTypeDef::tagged_>(
                            ikw<"tagged">)) +
                 option(flag<&ast::RecordTypeDef::limited_>(
                     ikw<"limited">)) +
                 ikw<"record"> +
                 (some(append<&ast::RecordTypeDef::components>(
                          ref<ComponentItemRule>))  //
                  | (many(append<&ast::RecordTypeDef::components>(
                              ref<ComponentItemRule>)) +
                     assign<&ast::RecordTypeDef::variant>(
                         ref<VariantPartRule>))  //
                  | (ikw<"null"> + lit<";">)) +
                 ikw<"end"> + ikw<"record">)  //
             | create<ast::RecordTypeDef>(
                   option(option(ikw<"abstract">) +
                          flag<&ast::RecordTypeDef::tagged_>(
                              ikw<"tagged">)) +
                   option(flag<&ast::RecordTypeDef::limited_>(
                       ikw<"limited">)) +
                   ikw<"null"> +
                   flag<&ast::RecordTypeDef::isNullRecord>(
                       ikw<"record">))> {};

  struct InterfaceListRule
      : rule<some(append<&ast::DerivedTypeDef::interfaces>(
                      create<ast::Ident>(assign<&ast::Ident::text>(QualName))),
                  ikw<"and">)> {};

  struct DerivedTypeDefRule
      : rule<create<ast::DerivedTypeDef>(
            option(flag<&ast::DerivedTypeDef::abstract_>(
                ikw<"abstract">)) +
            option(flag<&ast::DerivedTypeDef::limited_>(
                ikw<"limited">)) +
            ikw<"new"> +
            assign<&ast::DerivedTypeDef::base>(ref<SubtypeIndicationRule>) +
            option(
                option(ikw<"and"> +
                       some(append<&ast::DerivedTypeDef::interfaces>(
                                create<ast::Ident>(
                                    assign<&ast::Ident::text>(QualName))),
                            ikw<"and">)) +
                ikw<"with"> +
                assign<&ast::DerivedTypeDef::extension>(
                    ref<RecordTypeDefRule>)))> {};

  struct InterfaceTypeDefRule
      : rule<create<ast::InterfaceTypeDef>(
            option(assign<&ast::InterfaceTypeDef::kind>(
                ikw<"limited"> | ikw<"task"> | ikw<"protected"> |
                ikw<"synchronized">)) +
            ikw<"interface"> +
            option(ikw<"and"> +
                   some(append<&ast::InterfaceTypeDef::interfaces>(
                            create<ast::Ident>(
                                assign<&ast::Ident::text>(QualName))),
                        ikw<"and">)))> {};

  struct PrivateTypeDefRule
      : rule<create<ast::PrivateTypeDef>(
            option(option(ikw<"abstract">) +
                   flag<&ast::PrivateTypeDef::tagged_>(ikw<"tagged">)) +
            option(flag<&ast::PrivateTypeDef::limited_>(
                ikw<"limited">)) +
            ikw<"private">)> {};

  struct PrivateExtensionDefRule
      : rule<create<ast::PrivateExtensionDef>(
            option(flag<&ast::PrivateExtensionDef::abstract_>(
                ikw<"abstract">)) +
            option(assign<&ast::PrivateExtensionDef::kind>(
                ikw<"limited"> | ikw<"synchronized">)) +
            ikw<"new"> +
            assign<&ast::PrivateExtensionDef::base>(
                ref<SubtypeIndicationRule>) +
            option(ikw<"and"> +
                   some(append<&ast::PrivateExtensionDef::interfaces>(
                            create<ast::Ident>(
                                assign<&ast::Ident::text>(QualName))),
                        ikw<"and">)) +
            ikw<"with"> + ikw<"private">)> {};

  struct TypeDefRule
      : rule<ref<EnumTypeDefRule>       //
            | ref<ModularTypeDefRule>   //
            | ref<IntegerTypeDefRule>   //
            | ref<FloatTypeDefRule>     //
            | ref<OrdinaryFixedDefRule> //
            | ref<DecimalFixedDefRule>  //
            | ref<ArrayTypeDefRule>     //
            | ref<RecordTypeDefRule>    //
            | ref<AccessDefRule>        //
            | ref<DerivedTypeDefRule>   //
            | ref<InterfaceTypeDefRule>> {};

  struct FullTypeDeclRule
      : rule<create<ast::TypeDecl>(
                 ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                 option(assign<&ast::TypeDecl::discriminants>(
                     ref<KnownDiscriminantsRule>)) +
                 ikw<"is"> + assign<&ast::TypeDecl::def>(ref<TypeDefRule>) +
                 AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)  //
             | create<ast::TypeDecl>(
                   ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                   option(assign<&ast::TypeDecl::discriminants>(
                       ref<KnownDiscriminantsRule>)) +
                   ikw<"is"> +
                   flag<&ast::TypeDecl::private_>(ikw<"private">))  //
             | create<ast::TypeDecl>(
                   ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                   option(assign<&ast::TypeDecl::discriminants>(
                       ref<KnownDiscriminantsRule>)) +
                   ikw<"is"> +
                   assign<&ast::TypeDecl::def>(ref<PrivateTypeDefRule>) +
                   AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)  //
             | create<ast::TypeDecl>(
                   ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                   option(assign<&ast::TypeDecl::discriminants>(
                       ref<KnownDiscriminantsRule>)) +
                   flag<&ast::TypeDecl::privateExtension>(ikw<"is">) +
                   assign<&ast::TypeDecl::def>(
                       ref<PrivateExtensionDefRule>) +
                   AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)> {};

  struct IncompleteTypeDeclRule
      : rule<create<ast::TypeDecl>(
            ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
            option(DiscriminantPartOpt) +
            option(ikw<"is"> +
                   flag<&ast::TypeDecl::isTagged>(ikw<"tagged">)) +
            flag<&ast::TypeDecl::incomplete>(lit<";">))> {};

  struct TypeDeclRule
      : rule<ref<FullTypeDeclRule> | ref<IncompleteTypeDeclRule>> {};

  struct SubtypeDeclRule
      : rule<create<ast::TypeDecl>(
            ikw<"subtype"> + assign<&ast::TypeDecl::name>(Id) + ikw<"is"> +
            assign<&ast::TypeDecl::def>(ref<SubtypeIndicationRule>) +
            AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)> {};

  // -- Subprograms -----------------------------------------------------

  struct AbstractSubprogramDeclRule
      : rule<create<ast::SubprogramDecl>(
            assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpecRule>) +
            ikw<"is"> +
            flag<&ast::SubprogramDecl::abstract_>(ikw<"abstract">) +
            AspectSpecOpt<&ast::SubprogramDecl::aspects> + lit<";">)> {};

  struct NullProcedureDeclRule
      : rule<create<ast::SubprogramDecl>(
            assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpecRule>) +
            ikw<"is"> +
            flag<&ast::SubprogramDecl::isNullProc>(ikw<"null">) +
            AspectSpecOpt<&ast::SubprogramDecl::aspects> + lit<";">)> {};

  struct ExpressionFuncDeclRule
      : rule<create<ast::SubprogramDecl>(
            assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpecRule>) +
            ikw<"is"> + lit<"("> +
            assign<&ast::SubprogramDecl::exprBody>(ref<Expression>) +
            lit<")"> + AspectSpecOpt<&ast::SubprogramDecl::aspects> + lit<";">)> {};

  struct SubprogramDeclRule
      : rule<create<ast::SubprogramDecl>(
            assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpecRule>) +
            AspectSpecOpt<&ast::SubprogramDecl::aspects> + lit<";">)> {};

  struct SubprogramBodyRule
      : rule<create<ast::SubprogramBody>(
            assign<&ast::SubprogramBody::spec>(ref<SubprogramSpecRule>) +
            AspectSpecOpt<&ast::SubprogramBody::aspects> + ikw<"is"> +
            many(append<&ast::SubprogramBody::decls>(ref<DeclItem>)) +
            ikw<"begin"> +
            assign<&ast::SubprogramBody::body>(ref<StmtSeqRule>) +
            ikw<"end"> +
            option(assign<&ast::SubprogramBody::endName>(
                QualName | ref<STRING_LITERAL>)) +
            lit<";">)> {};

  // -- Packages --------------------------------------------------------

  struct PackageSpecRule
      : rule<create<ast::PackageDecl>(
            ikw<"package"> + assign<&ast::PackageDecl::name>(QualName) +
            AspectSpecOpt<&ast::PackageDecl::aspects> + ikw<"is"> +
            many(append<&ast::PackageDecl::visible>(ref<DeclItem>)) +
            option(ikw<"private"> +
                   many(append<&ast::PackageDecl::private_>(ref<DeclItem>))) +
            ikw<"end"> +
            option(assign<&ast::PackageDecl::endName>(QualName)))> {};

  struct PackageDeclRule : rule<ref<PackageSpecRule> + lit<";">> {};

  struct PackageBodyRule
      : rule<create<ast::PackageBody>(
            ikw<"package"> + ikw<"body"> +
            assign<&ast::PackageBody::name>(QualName) + AspectSpecOpt<&ast::PackageBody::aspects> +
            ikw<"is"> +
            many(append<&ast::PackageBody::decls>(ref<DeclItem>)) +
            option(ikw<"begin"> +
                   assign<&ast::PackageBody::body>(ref<StmtSeqRule>)) +
            ikw<"end"> +
            option(assign<&ast::PackageBody::endName>(QualName)) +
            lit<";">)> {};

  // -- Entries, tasks, protected objects ---------------------------------

  struct EntryDeclRule
      : rule<create<ast::EntryDecl>(
            OverridingOpt<&ast::EntryDecl::notOverriding,
                          &ast::EntryDecl::overriding_> + ikw<"entry"> +
            assign<&ast::EntryDecl::name>(Id) +
            option(lit<"("> +
                   assign<&ast::EntryDecl::familyRange>(ref<DiscreteRange>) +
                   lit<")">) +
            option(assign<&ast::EntryDecl::params>(ref<FormalPartRule>)) +
            AspectSpecOpt<&ast::EntryDecl::aspects> + lit<";">)> {};

  struct EntryIndexSpecRule
      : rule<create<ast::Param>(
            ikw<"for"> + append<&ast::Param::names>(Id) + ikw<"in"> +
            assign<&ast::Param::type>(ref<DiscreteRange>))> {};

  struct EntryBodyRule
      : rule<create<ast::EntryBody>(
            ikw<"entry"> + assign<&ast::EntryBody::name>(Id) +
            option(lit<"("> +
                   assign<&ast::EntryBody::index>(ref<EntryIndexSpecRule>) +
                   lit<")">) +
            option(assign<&ast::EntryBody::params>(ref<FormalPartRule>)) +
            ikw<"when"> + assign<&ast::EntryBody::barrier>(ref<Expression>) +
            ikw<"is"> + many(append<&ast::EntryBody::decls>(ref<DeclItem>)) +
            ikw<"begin"> + assign<&ast::EntryBody::body>(ref<StmtSeqRule>) +
            ikw<"end"> + option(DirectNameText) + lit<";">)> {};

  struct TaskItemRule
      : rule<ref<EntryDeclRule> | ref<AspectClauseRule> |
             ref<PragmaDeclRule>> {};

  struct InterfacesAndRule
      : rule<some(append<&ast::TaskSpec::interfaces>(
                      create<ast::Ident>(assign<&ast::Ident::text>(QualName))),
                  ikw<"and">)> {};

  struct TaskSpecRule
      : rule<create<ast::TaskSpec>(
            ikw<"task"> +
            option(flag<&ast::TaskSpec::isType>(ikw<"type">)) +
            assign<&ast::TaskSpec::name>(Id) +
            option(assign<&ast::TaskSpec::discriminants>(
                ref<KnownDiscriminantsRule>)) +
            AspectSpecOpt<&ast::TaskSpec::aspects> +
            option(flag<&ast::TaskSpec::hasDef>(ikw<"is">) +
                   option(ikw<"new"> +
                          some(append<&ast::TaskSpec::interfaces>(create<ast::Ident>(
                                   assign<&ast::Ident::text>(QualName))),
                               ikw<"and">) +
                          ikw<"with">) +
                   many(append<&ast::TaskSpec::items>(ref<TaskItemRule>)) +
                   option(ikw<"private"> +
                          many(append<&ast::TaskSpec::privateItems>(
                              ref<TaskItemRule>))) +
                   ikw<"end"> + option(DirectNameText)) +
            lit<";">)> {};

  struct TaskBodyRule
      : rule<create<ast::TaskBody>(
            ikw<"task"> + ikw<"body"> + assign<&ast::TaskBody::name>(Id) +
            AspectSpecOpt<&ast::TaskBody::aspects> + ikw<"is"> +
            many(append<&ast::TaskBody::decls>(ref<DeclItem>)) +
            ikw<"begin"> + assign<&ast::TaskBody::body>(ref<StmtSeqRule>) +
            ikw<"end"> + option(assign<&ast::TaskBody::endName>(Id)) +
            lit<";">)> {};

  struct ProtectedOpDeclRule
      : rule<ref<SubprogramDeclRule> | ref<EntryDeclRule> |
             ref<AspectClauseRule> | ref<PragmaDeclRule>> {};

  struct ProtectedElemDeclRule
      : rule<ref<ProtectedOpDeclRule> | ref<ComponentDeclRule>> {};

  struct ProtectedSpecRule
      : rule<create<ast::ProtectedSpec>(
            ikw<"protected"> +
            option(flag<&ast::ProtectedSpec::isType>(ikw<"type">)) +
            assign<&ast::ProtectedSpec::name>(Id) +
            option(assign<&ast::ProtectedSpec::discriminants>(
                ref<KnownDiscriminantsRule>)) +
            AspectSpecOpt<&ast::ProtectedSpec::aspects> + ikw<"is"> +
            option(ikw<"new"> +
                   some(append<&ast::ProtectedSpec::interfaces>(create<ast::Ident>(
                            assign<&ast::Ident::text>(QualName))),
                        ikw<"and">) +
                   ikw<"with">) +
            many(append<&ast::ProtectedSpec::items>(ref<ProtectedOpDeclRule>)) +
            option(ikw<"private"> +
                   many(append<&ast::ProtectedSpec::privateItems>(
                       ref<ProtectedElemDeclRule>))) +
            ikw<"end"> + option(DirectNameText) + lit<";">)> {};

  struct ProtectedOperationItemRule
      : rule<ref<SubprogramDeclRule> | ref<SubprogramBodyRule> |
             ref<EntryBodyRule> | ref<AspectClauseRule> |
             ref<PragmaDeclRule>> {};

  struct ProtectedBodyRule
      : rule<create<ast::ProtectedBody>(
            ikw<"protected"> + ikw<"body"> +
            assign<&ast::ProtectedBody::name>(Id) + AspectSpecOpt<&ast::ProtectedBody::aspects> +
            ikw<"is"> +
            many(append<&ast::ProtectedBody::items>(
                ref<ProtectedOperationItemRule>)) +
            ikw<"end"> + option(DirectNameText) + lit<";">)> {};

  // -- Generics -----------------------------------------------------

  struct GenericFormalObjectRule
      : rule<create<ast::ObjectDecl>(
            some(append<&ast::ObjectDecl::names>(Id), lit<",">) + lit<":"> +
            assign<&ast::ObjectDecl::mode>(ModeText) +
            option(ikw<"not"> + ikw<"null">) +
            assign<&ast::ObjectDecl::type>(ref<AccessDefRule> |
                                           ref<SubtypeMarkExprRule>) +
            option(lit<":="> +
                   assign<&ast::ObjectDecl::init>(ref<Expression>)) +
            AspectSpecOpt<&ast::ObjectDecl::aspects> + lit<";">)> {};

  struct FormalDerivedDefRule
      : rule<create<ast::FormalDerivedDef>(
            option(flag<&ast::FormalDerivedDef::abstract_>(
                ikw<"abstract">)) +
            option(flag<&ast::FormalDerivedDef::limited_>(
                ikw<"limited">)) +
            option(ikw<"synchronized">) + ikw<"new"> +
            assign<&ast::FormalDerivedDef::base>(
                create<ast::SubtypeIndication>(
                    assign<&ast::SubtypeIndication::mark>(SubtypeMarkText))) +
            option(
                option(ikw<"and"> +
                       some(append<&ast::FormalDerivedDef::interfaces>(
                                create<ast::Ident>(
                                    assign<&ast::Ident::text>(QualName))),
                            ikw<"and">)) +
                flag<&ast::FormalDerivedDef::withPrivate>(
                    ikw<"with"> + ikw<"private">)))> {};

  struct FormalTypeDefRule
      : rule<create<ast::FormalTypeDef>(lit<"("> + lit<"<>"> + lit<")">)  //
             | create<ast::FormalTypeDef>(
                   assign<&ast::FormalTypeDef::kind>(ikw<"range">) +
                   lit<"<>">)  //
             | create<ast::FormalTypeDef>(
                   assign<&ast::FormalTypeDef::kind>(ikw<"mod">) +
                   lit<"<>">)  //
             | create<ast::FormalTypeDef>(
                   assign<&ast::FormalTypeDef::kind>(ikw<"digits">) +
                   lit<"<>">)  //
             | create<ast::FormalTypeDef>(
                   assign<&ast::FormalTypeDef::kind>(ikw<"delta">) +
                   lit<"<>"> + ikw<"digits"> + lit<"<>">)  //
             | create<ast::FormalTypeDef>(
                   assign<&ast::FormalTypeDef::kind>(ikw<"delta">) +
                   lit<"<>">)> {};

  struct FormalTypeDefDispatchRule
      : rule<ref<FormalDerivedDefRule> | ref<FormalTypeDefRule> |
             ref<ArrayTypeDefRule> | ref<AccessDefRule> |
             ref<InterfaceTypeDefRule> | ref<PrivateTypeDefRule>> {};

  struct GenericFormalTypeRule
      : rule<create<ast::TypeDecl>(
                 ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                 option(DiscriminantPartOpt) + ikw<"is"> +
                 assign<&ast::TypeDecl::def>(ref<FormalTypeDefDispatchRule>) +
                 AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)  //
             | ref<IncompleteTypeDeclRule>> {};

  struct SubprogramDefaultRule
      : rule<ref<NameExpr> | ref<BoxRule> | ref<NullLitRule>> {};

  struct GenericFormalSubprogramRule
      : rule<create<ast::GenericFormalSubprogram>(
                 ikw<"with"> +
                 assign<&ast::GenericFormalSubprogram::spec>(
                     ref<SubprogramSpecRule>) +
                 ikw<"is"> +
                 flag<&ast::GenericFormalSubprogram::abstract_>(
                     ikw<"abstract">) +
                 option(assign<&ast::GenericFormalSubprogram::default_>(
                     ref<SubprogramDefaultRule>)) +
                 AspectSpecOpt<&ast::GenericFormalSubprogram::aspects> + lit<";">)  //
             | create<ast::GenericFormalSubprogram>(
                   ikw<"with"> +
                   assign<&ast::GenericFormalSubprogram::spec>(
                       ref<SubprogramSpecRule>) +
                   option(ikw<"is"> +
                          assign<&ast::GenericFormalSubprogram::default_>(
                              ref<SubprogramDefaultRule>)) +
                   AspectSpecOpt<&ast::GenericFormalSubprogram::aspects> + lit<";">)> {};

  struct GenericFormalPackageRule
      : rule<create<ast::GenericFormalPackage>(
            ikw<"with"> + ikw<"package"> +
            assign<&ast::GenericFormalPackage::name>(Id) + ikw<"is"> +
            ikw<"new"> +
            assign<&ast::GenericFormalPackage::instantiated>(QualName) +
            (flag<&ast::GenericFormalPackage::isBox>(
                 lit<"("> + option(ikw<"others"> + lit<"=>">) + lit<"<>"> +
                 lit<")">)  //
             | option(lit<"("> +
                      some(append<&ast::GenericFormalPackage::actuals>(
                               ref<ArgRule>),
                           lit<",">) +
                      option(lit<","> + ikw<"others"> + lit<"=>"> +
                             lit<"<>">) +
                      lit<")">)) +
            AspectSpecOpt<&ast::GenericFormalPackage::aspects> + lit<";">)> {};

  struct GenericFormalDeclRule
      : rule<ref<GenericFormalObjectRule> | ref<GenericFormalTypeRule> |
             ref<GenericFormalSubprogramRule> |
             ref<GenericFormalPackageRule>> {};

  static constexpr auto GenericFormalPart =
      ikw<"generic"> +
      many(append<&ast::GenericDecl::formals>(ref<GenericFormalDeclRule> |
                                              ref<UseClauseRule>));

  struct GenericDeclRule
      : rule<create<ast::GenericDecl>(
                 GenericFormalPart +
                 assign<&ast::GenericDecl::subprogSpec>(
                     ref<SubprogramSpecRule>) +
                 AspectSpecOpt<&ast::GenericDecl::aspects> + lit<";">)  //
             | create<ast::GenericDecl>(
                   GenericFormalPart +
                   assign<&ast::GenericDecl::packageSpec>(
                       ref<PackageSpecRule>) +
                   lit<";">)> {};

  struct GenericInstantiationRule
      : rule<create<ast::GenericInstantiation>(
                 assign<&ast::GenericInstantiation::kind>(ikw<"package">) +
                 assign<&ast::GenericInstantiation::name>(QualName) +
                 ikw<"is"> + ikw<"new"> +
                 assign<&ast::GenericInstantiation::generic>(QualName) +
                 option(lit<"("> +
                        some(append<&ast::GenericInstantiation::actuals>(
                                 ref<ArgRule>),
                             lit<",">) +
                        lit<")">) +
                 AspectSpecOpt<&ast::GenericInstantiation::aspects> + lit<";">)  //
             | create<ast::GenericInstantiation>(
                   OverridingOpt<&ast::GenericInstantiation::notOverriding,
                                 &ast::GenericInstantiation::overriding_> +
                   assign<&ast::GenericInstantiation::kind>(
                       ikw<"procedure">) +
                   assign<&ast::GenericInstantiation::name>(QualName) +
                   ikw<"is"> + ikw<"new"> +
                   assign<&ast::GenericInstantiation::generic>(QualName) +
                   option(lit<"("> +
                          some(append<&ast::GenericInstantiation::actuals>(
                                   ref<ArgRule>),
                               lit<",">) +
                          lit<")">) +
                   AspectSpecOpt<&ast::GenericInstantiation::aspects> + lit<";">)  //
             | create<ast::GenericInstantiation>(
                   OverridingOpt<&ast::GenericInstantiation::notOverriding,
                                 &ast::GenericInstantiation::overriding_> +
                   assign<&ast::GenericInstantiation::kind>(
                       ikw<"function">) +
                   assign<&ast::GenericInstantiation::name>(
                       QualName | ref<STRING_LITERAL>) +
                   ikw<"is"> + ikw<"new"> +
                   assign<&ast::GenericInstantiation::generic>(QualName) +
                   option(lit<"("> +
                          some(append<&ast::GenericInstantiation::actuals>(
                                   ref<ArgRule>),
                               lit<",">) +
                          lit<")">) +
                   AspectSpecOpt<&ast::GenericInstantiation::aspects> + lit<";">)> {};

  // -- Use clauses, representation clauses -------------------------------

  struct UseClauseRule
      : rule<create<ast::UseClause>(
                 ikw<"use"> +
                 some(append<&ast::UseClause::names>(QualName), lit<",">) +
                 lit<";">)  //
             | create<ast::UseClause>(
                   ikw<"use"> +
                   option(flag<&ast::UseClause::all_>(ikw<"all">)) +
                   flag<&ast::UseClause::isType>(ikw<"type">) +
                   some(append<&ast::UseClause::names>(QualName), lit<",">) +
                   lit<";">)> {};

  struct RecordCompClauseRule
      : rule<create<ast::RecordCompClause>(
            assign<&ast::RecordCompClause::name>(QualName) + ikw<"at"> +
            assign<&ast::RecordCompClause::position>(ref<Expression>) +
            ikw<"range"> +
            assign<&ast::RecordCompClause::first>(ref<Simple>) + lit<".."> +
            assign<&ast::RecordCompClause::last>(ref<Simple>) + lit<";">)> {
  };

  struct AspectClauseRule
      : rule<create<ast::AspectClause>(
                 ikw<"for"> + assign<&ast::AspectClause::name>(QualName) +
                 lit<"'"> +
                 assign<&ast::AspectClause::attr>(
                     ref<ID> | ikw<"access"> | ikw<"delta"> | ikw<"digits"> |
                     ikw<"mod"> | ikw<"range">) +
                 ikw<"use"> +
                 assign<&ast::AspectClause::value>(ref<Expression>) +
                 lit<";">)  //
             | create<ast::AspectClause>(
                   ikw<"for"> + assign<&ast::AspectClause::name>(QualName) +
                   flag<&ast::AspectClause::isEnumRep>(ikw<"use">) +
                   assign<&ast::AspectClause::value>(ref<Aggregate>) +
                   lit<";">)  //
             | create<ast::AspectClause>(
                   ikw<"for"> + assign<&ast::AspectClause::name>(QualName) +
                   ikw<"use"> +
                   flag<&ast::AspectClause::isRecordRep>(ikw<"record">) +
                   option(ikw<"at"> + ikw<"mod"> +
                          assign<&ast::AspectClause::modClause>(
                              ref<Expression>) +
                          lit<";">) +
                   many(append<&ast::AspectClause::components>(
                       ref<RecordCompClauseRule>)) +
                   ikw<"end"> + ikw<"record"> + lit<";">)  //
             | create<ast::AspectClause>(
                   ikw<"for"> + assign<&ast::AspectClause::name>(Id) +
                   ikw<"use"> + flag<&ast::AspectClause::isAt>(ikw<"at">) +
                   assign<&ast::AspectClause::value>(ref<Expression>) +
                   lit<";">)> {};

  // -- Body stubs, subunits, compilation -----------------------------

  struct BodyStubRule
      : rule<create<ast::BodyStub>(
                 assign<&ast::BodyStub::spec>(ref<SubprogramSpecRule>) +
                 ikw<"is"> + ikw<"separate"> + AspectSpecOpt<&ast::BodyStub::aspects> + lit<";">)  //
             | create<ast::BodyStub>(
                   ikw<"package"> + ikw<"body"> +
                   assign<&ast::BodyStub::name>(Id) + ikw<"is"> +
                   ikw<"separate"> + AspectSpecOpt<&ast::BodyStub::aspects> + lit<";">)  //
             | create<ast::BodyStub>(
                   ikw<"task"> + ikw<"body"> +
                   assign<&ast::BodyStub::name>(Id) + ikw<"is"> +
                   ikw<"separate"> + AspectSpecOpt<&ast::BodyStub::aspects> + lit<";">)  //
             | create<ast::BodyStub>(
                   ikw<"protected"> + ikw<"body"> +
                   assign<&ast::BodyStub::name>(Id) + ikw<"is"> +
                   ikw<"separate"> + AspectSpecOpt<&ast::BodyStub::aspects> + lit<";">)> {};

  struct ProperBodyRule
      : rule<ref<SubprogramBodyRule> | ref<PackageBodyRule> |
             ref<TaskBodyRule> | ref<ProtectedBodyRule>> {};

  struct SubunitRule
      : rule<create<ast::Subunit>(
            ikw<"separate"> + lit<"("> +
            assign<&ast::Subunit::parent>(QualName) + lit<")"> +
            assign<&ast::Subunit::body>(ref<ProperBodyRule>))> {};

  struct WithClauseRule
      : rule<create<ast::WithClause>(
                 flag<&ast::WithClause::limited_>(ikw<"limited">) +
                 option(flag<&ast::WithClause::private_>(
                     ikw<"private">)) +
                 ikw<"with"> +
                 some(append<&ast::WithClause::names>(QualName), lit<",">) +
                 lit<";">)  //
             | create<ast::WithClause>(
                   option(flag<&ast::WithClause::private_>(
                       ikw<"private">)) +
                   ikw<"with"> +
                   some(append<&ast::WithClause::names>(QualName), lit<",">) +
                   lit<";">)> {};

  struct DeclItem
      : rule<ref<PragmaDeclRule>              //
            | ref<TaskSpecRule>               //
            | ref<ProtectedSpecRule>          //
            | ref<TypeDeclRule>               //
            | ref<SubtypeDeclRule>            //
            | ref<ObjectDeclRule>             //
            | ref<NumberDeclRule>             //
            | ref<AbstractSubprogramDeclRule> //
            | ref<NullProcedureDeclRule>      //
            | ref<ExpressionFuncDeclRule>     //
            | ref<SubprogramDeclRule>         //
            | ref<PackageDeclRule>            //
            | ref<RenameDeclRule>             //
            | ref<ExceptionDeclRule>          //
            | ref<GenericDeclRule>            //
            | ref<GenericInstantiationRule>   //
            | ref<UseClauseRule>              //
            | ref<AspectClauseRule>           //
            | ref<SubprogramBodyRule>         //
            | ref<PackageBodyRule>            //
            | ref<TaskBodyRule>               //
            | ref<ProtectedBodyRule>          //
            | ref<BodyStubRule>> {};

  struct UnitRule
      : rule<create<ast::Unit>(
            many(append<&ast::Unit::withClauses>(ref<WithClauseRule>) |
                 append<&ast::Unit::useClauses>(ref<UseClauseRule>) |
                 append<&ast::Unit::pragmas>(ref<PragmaDeclRule>)) +
            ((option(flag<&ast::Unit::private_>(ikw<"private">)) +
              assign<&ast::Unit::body>(
                  ref<SubprogramDeclRule> | ref<PackageDeclRule> |
                  ref<GenericDeclRule> | ref<GenericInstantiationRule> |
                  ref<RenameDeclRule>))  //
             | assign<&ast::Unit::body>(ref<SubprogramBodyRule> |
                                        ref<PackageBodyRule>)  //
             | assign<&ast::Unit::body>(ref<SubunitRule>)))> {};

  // A compilation of configuration pragmas alone (no library_item) is
  // legal -- e.g. a lone `pragma Restrictions (...);` file.
  struct CompilationRule
      : rule<create<ast::Compilation>(
            many(append<&ast::Compilation::units>(ref<UnitRule>) |
                 append<&ast::Compilation::units>(create<ast::Unit>(
                     append<&ast::Unit::pragmas>(ref<PragmaDeclRule>)))))> {};

  using entry = CompilationRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT>;
};

}  // namespace ada
