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

  struct WS
      : terminal<some(uc::Zs | uc::Cf | cr<R"(\t\n\v\f\r\u0085\u2028\u2029)">)> {};
  struct LINE_COMMENT
      : terminal<lit<"--"> + many(cr<R"(^\r\n\v\f\u0085\u2028\u2029)">)> {};

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
  struct Allocator;
  struct ParenExpr;
  struct CondExpr;
  struct QuantifiedExpr;
  struct SubtypeIndication;
  struct AccessDef;
  struct Constraint;
  struct Range;
  struct ArrayTypeDef;
  struct DiscreteRange;
  struct DiscreteSubtypeDef;
  struct FormalPart;
  struct DeclItem;
  struct Stmt;
  struct StmtSeq;
  struct SubprogramSpec;
  struct TypeDef;
  struct AspectClause;
  struct PackageSpec;
  struct EntryDecl;
  struct EntryBody;
  struct TaskItem;
  struct ProtectedOpDecl;
  struct ProtectedElemDecl;
  struct ProtectedOperationItem;
  struct GenericFormalDecl;
  struct UseClause;

  // -- Operators: each token or fixed word pair hands `op` a constant. -


  static constexpr auto LvlMul =
      as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">) | as<ast::Operator::mod_>(ikw<"mod">) | as<ast::Operator::rem_>(ikw<"rem">);
  static constexpr auto LvlAdd = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">) | as<ast::Operator::concat>(lit<"&">);
  static constexpr auto LvlRel = as<ast::Operator::ne>(lit<"/=">) | as<ast::Operator::le>(lit<"<=">) | as<ast::Operator::ge>(lit<">=">) |
                                 as<ast::Operator::eq>(lit<"=">) | as<ast::Operator::lt>(lit<"<">) | as<ast::Operator::gt>(lit<">">);

  // A document routinely names units and entities outside itself: no
  // `reference<>` here. `with` clauses are handled at Unit level; aspect
  // associations remain attached to their declaration.
  struct AspectName : rule<QualName + option(lit<"'"> + ikw<"class">)> {};
  struct Aspect
      : rule<assign<&ast::Aspect::name>(ref<AspectName>) +
      option(lit<"=>"> + assign<&ast::Aspect::value>(ref<Expression>))> {
  };
  template <auto Field>
  static constexpr auto AspectSpecOpt =
      option(ikw<"with"> + some(append<Field>(ref<Aspect>), lit<",">));

  // -- Leaves ------------------------------------------------------------

  struct Ident : rule<assign<&ast::Ident::text>(Id)> {};
  struct OpSymbol : rule<assign<&ast::Ident::text>(ref<STRING_LITERAL>)> {
  };
  struct Selector : rule<assign<&ast::Ident::text>(SelectorText)> {};
  struct CharLit
      : rule<assign<&ast::CharLit::text>(ref<CHARACTER_LITERAL>)> {};
  struct StringLit
      : rule<assign<&ast::StringLit::text>(ref<STRING_LITERAL>)> {};
  struct NumericLit
      : rule<assign<&ast::NumericLit::text>(ref<NUMERIC_LITERAL>)> {};
  struct NullLit : rule<create<ast::NullLit>(ikw<"null">)> {};
  struct Others : rule<create<ast::OthersExpr>(ikw<"others">)> {};
  struct Box : rule<create<ast::BoxExpr>(lit<"<>">)> {};
  struct TerminateStmt
      : rule<create<ast::TerminateStmt>(ikw<"terminate"> + lit<";">)> {};

  struct DirectName : rule<ref<Ident> | ref<OpSymbol>> {};
  static constexpr auto DirectNameText = Id | ref<STRING_LITERAL>;
  // A subtype mark that denotes a class-wide subtype (`T'Class`) is common
  // in parameter/return positions; folded in here rather than left
  // unparsed.
  struct SubtypeMarkTextRule
      : rule<QualName + option(lit<"'"> + ikw<"class">)> {};
  static constexpr auto SubtypeMarkText = ref<SubtypeMarkTextRule>;
  struct SubtypeMarkExpr : rule<assign<&ast::Ident::text>(SubtypeMarkText)> {
  };

  // -- Name/expression chain --------------------------------------------

  // Indexing, slicing, a call, a generalized indexing or a type
  // conversion: `name '(' ... ')'` in every case, told apart only by
  // whether the prefix names a type; unified here into one Call. Gap
  // `semantic-predicate`, nil price.
  // A slice's bound (`S(I..J)`) is a range, not an expression: `Expression`
  // has no `..`. One argument shape covers calls, indexing and slices.
  struct Arg
      : rule<create<ast::Choice>(
            option(append<&ast::Choice::tests>(ref<Selector>) + lit<"=>">) +
            assign<&ast::Choice::value>(ref<Box> | ref<Range> |
                                        ref<CondExpr> | ref<Expression>))> {};

  struct NameExpr
      : rule<(ref<DirectName> | ref<CharLit>) +
             many(
                 nest<ast::AllExpr, &ast::AllExpr::prefix>(lit<"."> +
                                                           ikw<"all">)  //
                 | nest<ast::SelectExpr, &ast::SelectExpr::prefix>(
                       lit<"."> +
                       assign<&ast::SelectExpr::selector>(SelectorText))  //
                 | nest<ast::CallExpr, &ast::CallExpr::callee>(
                       lit<"("> +
                       some(append<&ast::CallExpr::args>(ref<Arg>),
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
                              assign<&ast::AttrExpr::arg>(ref<CondExpr> |
                                                    ref<Expression>) +
                              lit<")">)))> {};

  struct Subpool : rule<lit<"("> + ref<NameExpr> + lit<")">> {};

  struct AllocatorTarget
      : rule<(ref<SubtypeMarkExpr> +
              nest<ast::QualifiedExpr, &ast::QualifiedExpr::target>(
                  lit<"'"> +
                  assign<&ast::QualifiedExpr::value>(
                      ref<Aggregate> |
                      (lit<"("> + ref<Expression> + lit<")">))))  //
             | ref<SubtypeIndication>> {};

  struct Allocator
      : rule<create<ast::Allocator>(
            ikw<"new"> +
            option(assign<&ast::Allocator::subpool>(ref<Subpool>)) +
            assign<&ast::Allocator::target>(ref<AllocatorTarget>))> {};

  static constexpr auto Quantifier =
      flag<&ast::QuantifiedExpr::forAll>(ikw<"all">) | ikw<"some">;

  struct QuantifiedExpr
      : rule<create<ast::QuantifiedExpr>(
                 ikw<"for"> + Quantifier +
                 assign<&ast::QuantifiedExpr::var>(create<ast::Param>(
                     append<&ast::Param::names>(Id) + ikw<"in"> +
                     option(flag<&ast::Param::reverse_>(ikw<"reverse">)) +
                     assign<&ast::Param::type>(
                         ref<DiscreteSubtypeDef>))) +
                 lit<"=>"> +
                 assign<&ast::QuantifiedExpr::predicate>(ref<Expression>))  //
             | create<ast::QuantifiedExpr>(
                   ikw<"for"> + Quantifier +
                   assign<&ast::QuantifiedExpr::var>(create<ast::Param>(
                       append<&ast::Param::names>(Id) +
                       option(lit<":"> +
                              assign<&ast::Param::type>(
                                  ref<SubtypeIndication>)) +
                       flag<&ast::Param::isOf>(ikw<"of">) + option(flag<&ast::Param::reverse_>(ikw<"reverse">)) +
                       assign<&ast::Param::def>(ref<NameExpr>))) +
                   lit<"=>"> +
                   assign<&ast::QuantifiedExpr::predicate>(ref<Expression>))  //
             | create<ast::QuantifiedExpr>(
                   ikw<"for"> + Quantifier +
                   assign<&ast::QuantifiedExpr::var>(create<ast::Param>(
                       append<&ast::Param::names>(Id) + ikw<"in"> +
                       option(flag<&ast::Param::reverse_>(ikw<"reverse">)) +
                       assign<&ast::Param::def>(ref<NameExpr>))) +
                   lit<"=>"> +
                   assign<&ast::QuantifiedExpr::predicate>(ref<Expression>))> {
  };

  struct ParenExpr
      : rule<create<ast::GroupedExpr>(
            lit<"("> +
            assign<&ast::GroupedExpr::value>(ref<QuantifiedExpr> | ref<CondExpr> | ref<Expression>) +
             lit<")">)> {};

  struct Primary : rule<ref<NumericLit>    //
                        | ref<NullLit>     //
                        | ref<StringLit>   //
                        | ref<Aggregate>       //
                        | ref<NameExpr>        //
                        | ref<Allocator>   //
                        | ref<ParenExpr>> {};

  struct Factor
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<Primary>) +
                 assign<&ast::BinaryExpr::op>(as<ast::Operator::power>(lit<"**">)) +
                 assign<&ast::BinaryExpr::right>(ref<Primary>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(ikw<"abs">) +
                   assign<&ast::UnaryExpr::operand>(ref<Primary>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(ikw<"not">) +
                   assign<&ast::UnaryExpr::operand>(ref<Primary>))  //
             | ref<Primary>> {};

  struct Term : infix<Factor, ast::BinaryExpr, left(LvlMul)> {};

  struct SignedTerm
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Term>))  //
             | ref<Term>> {};

  struct Simple : infix<SignedTerm, ast::BinaryExpr,
                        left(LvlAdd)> {};

  struct DiscreteRange : rule<ref<Range> | ref<SubtypeIndication>> {};

  // A discrete choice is a discrete range OR a bare choice expression
  // (`when 1 =>`) -- easy to lose sight of since both are Expr-typed and
  // `DiscreteRange` alone still compiles, just under-accepts. A bare name
  // reads as an expression: only its declaration could make it a subtype.
  static constexpr auto DiscreteChoice =
      ref<Range> | ref<Expression> | ref<SubtypeIndication>;

  static constexpr auto MembershipList =
      some(append<&ast::MembershipExpr::choices>(ref<DiscreteRange> |
                                                  ref<Simple>),
           lit<"|">);

  struct RaiseExpr
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
             | ref<RaiseExpr>  //
             | ref<Simple>> {};

  // Read a logical chain permissively. Runtime validation checks that
  // unparenthesized links use one operator kind (Ada RM 4.4).
  struct Expression
      : infix<Relation, ast::BinaryExpr,
                            left(as<ast::Operator::and_then>(ikw<"and"> + ikw<"then">) | as<ast::Operator::or_else>(ikw<"or"> + ikw<"else">) | as<ast::Operator::and_>(ikw<"and">) |
                                 as<ast::Operator::or_>(ikw<"or">) | as<ast::Operator::xor_>(ikw<"xor">))> {};

  struct IfExprElsif
      : rule<create<ast::Choice>(
            ikw<"elsif"> + append<&ast::Choice::tests>(ref<Expression>) +
            ikw<"then"> + assign<&ast::Choice::value>(ref<Expression>))> {};

  struct IfExpr
      : rule<create<ast::IfExpr>(
            ikw<"if"> + assign<&ast::IfExpr::cond>(ref<Expression>) +
            ikw<"then"> + assign<&ast::IfExpr::thenValue>(ref<Expression>) +
            many(append<&ast::IfExpr::elsifs>(ref<IfExprElsif>)) +
            option(ikw<"else"> +
                   assign<&ast::IfExpr::elseValue>(ref<Expression>)))> {};

  struct CaseExprAlt
      : rule<create<ast::Choice>(
            ikw<"when"> +
            some(append<&ast::Choice::tests>(DiscreteChoice |
                                             ref<Others>),
                 lit<"|">) +
            lit<"=>"> + assign<&ast::Choice::value>(ref<Expression>))> {};

  struct CaseExpr
      : rule<create<ast::CaseExpr>(
            ikw<"case"> + assign<&ast::CaseExpr::selector>(ref<Expression>) +
            ikw<"is"> +
            some(append<&ast::CaseExpr::alts>(ref<CaseExprAlt>),
                 lit<",">))> {};

  struct CondExpr : rule<ref<IfExpr> | ref<CaseExpr>> {};

  // -- Constraints, subtype indications, access definitions --------------

  struct Range
      : rule<create<ast::RangeExpr>(
                 assign<&ast::RangeExpr::low>(ref<Simple>) + lit<".."> +
                 assign<&ast::RangeExpr::high>(ref<Simple>))  //
             | (ref<NameExpr> + lit<"'"> +
                nest<ast::AttrExpr, &ast::AttrExpr::prefix>(
                    assign<&ast::AttrExpr::attr>(ikw<"range">) +
                    option(lit<"("> +
                           assign<&ast::AttrExpr::arg>(ref<CondExpr> | ref<Expression>) +
                           lit<")">)))> {};

  struct IndexConstraint
      : rule<create<ast::IndexConstraint>(
            lit<"("> +
            some(append<&ast::IndexConstraint::ranges>(ref<DiscreteRange>),
                 lit<",">) +
            lit<")">)> {};

  struct DiscAssoc
      : rule<create<ast::Choice>(
            option(some(append<&ast::Choice::tests>(ref<Selector>),
                        lit<"|">) +
                   lit<"=>">) +
            assign<&ast::Choice::value>(ref<Expression>))> {};

  struct DiscriminantConstraint
      : rule<create<ast::DiscriminantConstraint>(
            lit<"("> +
            some(append<&ast::DiscriminantConstraint::associations>(
                     ref<DiscAssoc>),
                 lit<",">) +
            lit<")">)> {};

  struct DigitsConstraint
      : rule<create<ast::DigitsConstraint>(
            ikw<"digits"> + assign<&ast::DigitsConstraint::digits_>(ref<Simple>) +
            option(ikw<"range"> +
                   assign<&ast::DigitsConstraint::range>(ref<Range>)))> {
  };

  struct DeltaConstraint
      : rule<create<ast::DeltaConstraint>(
            ikw<"delta"> + assign<&ast::DeltaConstraint::delta_>(ref<Simple>) +
            option(ikw<"range"> +
                   assign<&ast::DeltaConstraint::range>(ref<Range>)))> {};

  struct RangeConstraint : rule<ikw<"range"> + ref<Range>> {};

  // A loop's subtype is discrete (RM 5.5): a range constraint at most
  // (RM 3.2.2), so `F (X)` there is an iterator name, never a subtype.
  struct DiscreteSubtypeDef
      : rule<ref<Range> |
             create<ast::SubtypeIndication>(
                 assign<&ast::SubtypeIndication::mark>(SubtypeMarkText) +
                 option(assign<&ast::SubtypeIndication::constraint>(
                     ref<RangeConstraint>)))> {};

  struct Constraint
      : rule<ref<RangeConstraint> | ref<DigitsConstraint> |
             ref<DeltaConstraint> | ref<IndexConstraint> |
             ref<DiscriminantConstraint>> {};

  struct SubtypeIndication
      : rule<create<ast::SubtypeIndication>(
            option(flag<&ast::SubtypeIndication::nullExclusion>(
                ikw<"not"> + ikw<"null">)) +
            assign<&ast::SubtypeIndication::mark>(SubtypeMarkText) +
            option(assign<&ast::SubtypeIndication::constraint>(
                ref<Constraint>)))> {};

  struct AccessDef
      : rule<create<ast::AccessDef>(
                 option(flag<&ast::AccessDef::nullExclusion>(
                     ikw<"not"> + ikw<"null">)) +
                 ikw<"access"> +
                 option(flag<&ast::AccessDef::constant_>(ikw<"constant">) |
                        flag<&ast::AccessDef::all_>(ikw<"all">)) +
                 assign<&ast::AccessDef::mark>(SubtypeMarkText) +
                 option(assign<&ast::AccessDef::constraint>(
                     ref<Constraint>)))  //
             | create<ast::AccessDef>(
                   option(flag<&ast::AccessDef::nullExclusion>(
                       ikw<"not"> + ikw<"null">)) +
                   ikw<"access"> +
                   option(flag<&ast::AccessDef::protected_>(
                       ikw<"protected">)) +
                   flag<&ast::AccessDef::isSubprogram>(ikw<"procedure">) +
                   option(assign<&ast::AccessDef::params>(
                       ref<FormalPart>)))  //
             | create<ast::AccessDef>(
                   option(flag<&ast::AccessDef::nullExclusion>(
                       ikw<"not"> + ikw<"null">)) +
                   ikw<"access"> +
                   option(flag<&ast::AccessDef::protected_>(
                       ikw<"protected">)) +
                   flag<&ast::AccessDef::isFunction>(ikw<"function">) +
                   option(assign<&ast::AccessDef::params>(
                       ref<FormalPart>)) +
                   ikw<"return"> + option(flag<&ast::AccessDef::returnNullExclusion>(
                       ikw<"not"> + ikw<"null">)) +
                   assign<&ast::AccessDef::returnType>(
                       ref<AccessDef> | ref<SubtypeMarkExpr>))> {
  };

  // -- Formal parts / parameters -----------------------------------------

  struct ParamSpec
      : rule<create<ast::Param>(
                 some(append<&ast::Param::names>(Id), lit<",">) + lit<":"> +
                 option(flag<&ast::Param::aliased_>(ikw<"aliased">)) +
                 assign<&ast::Param::mode>(ModeText) +
                 option(flag<&ast::Param::nullExclusion>(
                     ikw<"not"> + ikw<"null">)) +
                 assign<&ast::Param::type>(ref<SubtypeMarkExpr>) +
                 option(lit<":="> +
                        assign<&ast::Param::def>(ref<Expression>)))  //
             | create<ast::Param>(
                   some(append<&ast::Param::names>(Id), lit<",">) + lit<":"> +
                   assign<&ast::Param::type>(ref<AccessDef>) +
                   option(lit<":="> +
                          assign<&ast::Param::def>(ref<Expression>)))> {};

  struct FormalPart
      : rule<lit<"("> +
             some(append<&ast::Params::items>(ref<ParamSpec>), lit<";">) +
             lit<")">> {};

  struct DiscSpec
      : rule<create<ast::Param>(
                 some(append<&ast::Param::names>(Id), lit<",">) + lit<":"> +
                 option(flag<&ast::Param::nullExclusion>(
                     ikw<"not"> + ikw<"null">)) +
                 assign<&ast::Param::type>(ref<SubtypeMarkExpr>) +
                 option(lit<":="> +
                        assign<&ast::Param::def>(ref<Expression>)))  //
             | create<ast::Param>(
                   some(append<&ast::Param::names>(Id), lit<",">) + lit<":"> +
                   assign<&ast::Param::type>(ref<AccessDef>) +
                   option(lit<":="> +
                          assign<&ast::Param::def>(ref<Expression>)))> {};

  struct KnownDiscriminants
      : rule<lit<"("> +
             some(append<&ast::Params::items>(ref<DiscSpec>), lit<";">) +
             lit<")">> {};

  static constexpr auto DiscriminantPartOpt =
      assign<&ast::TypeDecl::discriminants>(ref<KnownDiscriminants>) |
      flag<&ast::TypeDecl::unknownDiscriminants>(
          lit<"("> + lit<"<>"> + lit<")">);

  // -- Aggregates ----------------------------------------------------

  struct AggItem
      : rule<create<ast::Choice>(
                 some(append<&ast::Choice::tests>(ref<Selector> |
                                                  ref<Others>),
                      lit<"|">) +
                 lit<"=>"> +
                 assign<&ast::Choice::value>(ref<Box> | ref<Expression>))  //
             | create<ast::Choice>(
                   assign<&ast::Choice::value>(ref<Expression>))> {};

  struct ArrayAggItem
      : rule<create<ast::Choice>(
            some(append<&ast::Choice::tests>(DiscreteChoice |
                                             ref<Others>),
                 lit<"|">) +
            lit<"=>"> +
            assign<&ast::Choice::value>(ref<Box> | ref<Expression>))> {};

  struct Aggregate
      : rule<lit<"("> +
             create<ast::Aggregate>(
                 (ikw<"null"> +
                  flag<&ast::Aggregate::isNullRecord>(ikw<"record">))  //
                 | (assign<&ast::Aggregate::ancestorOrTag>(ref<Expression>) +
                    flag<&ast::Aggregate::isExtension>(ikw<"with">) +
                    ((ikw<"null"> + ikw<"record">)  //
                     | some(append<&ast::Aggregate::items>(
                                ref<AggItem>),
                            lit<",">)))  //
                 | some(append<&ast::Aggregate::items>(ref<ArrayAggItem>),
                        lit<",">)  //
                 // A lone association is named, read above: `(E)` is a
                 // parenthesized expression (RM 4.3.1(7), 4.3.3(3)).
                 | (append<&ast::Aggregate::items>(ref<AggItem>) +
                    lit<","> +
                    some(append<&ast::Aggregate::items>(ref<AggItem>),
                         lit<",">))) +
             lit<")">> {};

  // -- Statements ------------------------------------------------------

  struct NullStmt : rule<create<ast::NullStmt>(ikw<"null">) + lit<";">> {};

  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::target>(ref<NameExpr>) + lit<":="> +
            assign<&ast::AssignStmt::value>(ref<Expression>) + lit<";">)> {};

  static constexpr auto CallArgs =
      lit<"("> +
      some(append<&ast::ProcCallStmt::args>(ref<Arg>), lit<",">) +
      lit<")">;

  struct ProcCallStmt
      : rule<create<ast::ProcCallStmt>(
            assign<&ast::ProcCallStmt::target>(ref<NameExpr>) +
            option(CallArgs) + lit<";">)> {};

  struct ExitStmt
      : rule<create<ast::ExitStmt>(
            ikw<"exit"> +
            option(assign<&ast::ExitStmt::target>(ref<NameExpr>)) +
            option(ikw<"when"> +
                   assign<&ast::ExitStmt::cond>(ref<Expression>)) +
            lit<";">)> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
            ikw<"goto"> + assign<&ast::GotoStmt::target>(ref<NameExpr>) +
            lit<";">)> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            ikw<"return"> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
            lit<";">)> {};

  struct ExtReturnObj
      : rule<create<ast::ObjectDecl>(
            some(append<&ast::ObjectDecl::names>(Id), lit<",">) + lit<":"> +
            option(flag<&ast::ObjectDecl::aliased_>(ikw<"aliased">)) +
            option(flag<&ast::ObjectDecl::constant_>(ikw<"constant">)) +
            assign<&ast::ObjectDecl::type>(ref<AccessDef> |
                                           ref<SubtypeIndication>) +
            option(lit<":="> +
                   assign<&ast::ObjectDecl::init>(ref<Expression>)))> {};

  struct ExtReturnStmt
      : rule<create<ast::ExtendedReturnStmt>(
            ikw<"return"> +
            assign<&ast::ExtendedReturnStmt::obj>(ref<ExtReturnObj>) +
            option(ikw<"do"> +
                   assign<&ast::ExtendedReturnStmt::body>(ref<StmtSeq>) +
                   ikw<"end"> + ikw<"return">) +
            lit<";">)> {};

  struct RequeueStmt
      : rule<create<ast::RequeueStmt>(
            ikw<"requeue"> + assign<&ast::RequeueStmt::target>(ref<NameExpr>) +
            option(ikw<"with"> +
                   flag<&ast::RequeueStmt::abort_>(ikw<"abort">)) +
            lit<";">)> {};

  struct DelayStmt
      : rule<create<ast::DelayStmt>(
            ikw<"delay"> +
            option(flag<&ast::DelayStmt::until_>(ikw<"until">)) +
            assign<&ast::DelayStmt::value>(ref<Expression>) + lit<";">)> {};

  struct AbortStmt
      : rule<create<ast::AbortStmt>(
            ikw<"abort"> +
            some(append<&ast::AbortStmt::targets>(ref<NameExpr>), lit<",">) +
            lit<";">)> {};

  struct RaiseStmt
      : rule<create<ast::RaiseStmt>(
            ikw<"raise"> +
            option(assign<&ast::RaiseStmt::target>(ref<NameExpr>) +
                   option(ikw<"with"> +
                          assign<&ast::RaiseStmt::message>(ref<Expression>))) +
            lit<";">)> {};

  struct PragmaArg
      : rule<create<ast::Choice>(
            option(append<&ast::Choice::tests>(ref<Selector>) + lit<"=>">) +
            assign<&ast::Choice::value>(ref<Expression>))> {};

  struct PragmaStmt
      : rule<create<ast::PragmaStmt>(
            ikw<"pragma"> + assign<&ast::PragmaStmt::name>(Id) +
            option(lit<"("> +
                   some(append<&ast::PragmaStmt::args>(ref<PragmaArg>),
                        lit<",">) +
                   lit<")">) +
            lit<";">)> {};

  struct PragmaDecl
      : rule<create<ast::PragmaDecl>(
            ikw<"pragma"> + assign<&ast::PragmaDecl::name>(Id) +
            option(lit<"("> +
                   some(append<&ast::PragmaDecl::args>(ref<PragmaArg>),
                        lit<",">) +
                   lit<")">) +
            lit<";">)> {};

  struct SimpleStmt
      : rule<ref<NullStmt>       //
            | ref<AssignStmt>    //
            | ref<ExitStmt>      //
            | ref<GotoStmt>      //
            | ref<ReturnStmt>    //
            | ref<RequeueStmt>   //
            | ref<DelayStmt>     //
            | ref<AbortStmt>     //
            | ref<RaiseStmt>     //
            | ref<PragmaStmt>    //
            | ref<ProcCallStmt>> {};

  struct CondBlock
      : rule<create<ast::CondBlock>(
            ikw<"elsif"> + assign<&ast::CondBlock::cond>(ref<Expression>) +
            ikw<"then"> + some(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            ikw<"if"> + assign<&ast::IfStmt::cond>(ref<Expression>) +
            ikw<"then"> + some(append<&ast::IfStmt::thenBody>(ref<Stmt>)) +
            many(append<&ast::IfStmt::elsifs>(ref<CondBlock>)) +
            option(ikw<"else"> +
                   some(append<&ast::IfStmt::elseBody>(ref<Stmt>))) +
            ikw<"end"> + ikw<"if"> + lit<";">)> {};

  struct CaseAlt
      : rule<create<ast::CaseAlt>(
            ikw<"when"> +
            some(append<&ast::CaseAlt::choices>(DiscreteChoice |
                                                ref<Others>),
                 lit<"|">) +
            lit<"=>"> + some(append<&ast::CaseAlt::body>(ref<Stmt>)))> {};

  struct CaseStmt
      : rule<create<ast::CaseStmt>(
            ikw<"case"> + assign<&ast::CaseStmt::selector>(ref<Expression>) +
            ikw<"is"> + some(append<&ast::CaseStmt::alts>(ref<CaseAlt>)) +
            ikw<"end"> + ikw<"case"> + lit<";">)> {};

  struct ForSpec
      : rule<create<ast::ForSpec>(
                 assign<&ast::ForSpec::var>(Id) +
                 option(lit<":"> + assign<&ast::ForSpec::subtypeIndication>(
                                       ref<SubtypeIndication>)) +
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
                       ref<DiscreteSubtypeDef>))  //
             | create<ast::ForSpec>(
                   assign<&ast::ForSpec::var>(Id) +
                   flag<&ast::ForSpec::isIterator>(ikw<"in">) +
                   option(flag<&ast::ForSpec::reverse_>(
                       ikw<"reverse">)) +
                   assign<&ast::ForSpec::range>(ref<NameExpr>))> {};

  static constexpr auto IterScheme =
      (ikw<"while"> + assign<&ast::LoopStmt::whileCond>(ref<Expression>))  //
      | (ikw<"for"> + assign<&ast::LoopStmt::forSpec>(ref<ForSpec>));

  struct LoopStmt
      : rule<create<ast::LoopStmt>(
            option(assign<&ast::LoopStmt::label>(Id) + lit<":">) +
            option(IterScheme) + ikw<"loop"> +
            some(append<&ast::LoopStmt::body>(ref<Stmt>)) + ikw<"end"> +
            ikw<"loop"> + option(assign<&ast::LoopStmt::endLabel>(Id)) +
            lit<";">)> {};

  struct BlockStmt
      : rule<create<ast::BlockStmt>(
            option(assign<&ast::BlockStmt::label>(Id) + lit<":">) +
            option(ikw<"declare"> +
                   many(append<&ast::BlockStmt::decls>(ref<DeclItem>))) +
            ikw<"begin"> + assign<&ast::BlockStmt::body>(ref<StmtSeq>) +
            ikw<"end"> + option(assign<&ast::BlockStmt::endName>(DirectNameText)) + lit<";">)> {};

  struct AcceptStmt
      : rule<create<ast::AcceptStmt>(
            ikw<"accept"> + assign<&ast::AcceptStmt::name>(Id) +
            option(lit<"("> +
                   assign<&ast::AcceptStmt::index>(ref<Expression>) +
                   lit<")">) +
            option(assign<&ast::AcceptStmt::params>(ref<FormalPart>)) +
            option(ikw<"do"> +
                   assign<&ast::AcceptStmt::body>(ref<StmtSeq>) +
                   ikw<"end"> + option(assign<&ast::AcceptStmt::endName>(DirectNameText))) +
            lit<";">)> {};

  struct SelectAlt
      : rule<create<ast::CondBlock>(
            option(ikw<"when"> + assign<&ast::CondBlock::cond>(ref<Expression>) +
                   lit<"=>">) +
            append<&ast::CondBlock::body>(ref<AcceptStmt> |
                                          ref<DelayStmt> |
                                          ref<TerminateStmt>) +
            many(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct EntryCallAlt
      : rule<create<ast::CondBlock>(
            append<&ast::CondBlock::body>(ref<ProcCallStmt>) +
            many(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct DelayAlt
      : rule<create<ast::CondBlock>(
            append<&ast::CondBlock::body>(ref<DelayStmt>) +
            many(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct TriggerAlt
      : rule<create<ast::CondBlock>(
            append<&ast::CondBlock::body>(ref<ProcCallStmt> |
                                          ref<DelayStmt>) +
            many(append<&ast::CondBlock::body>(ref<Stmt>)))> {};

  struct SelectStmt
      : rule<create<ast::SelectStmt>(
                 ikw<"select"> +
                 append<&ast::SelectStmt::alts>(ref<SelectAlt>) +
                 many(ikw<"or"> +
                      append<&ast::SelectStmt::alts>(ref<SelectAlt>)) +
                 option(ikw<"else"> +
                        some(append<&ast::SelectStmt::elseBody>(
                            ref<Stmt>))) +
                 ikw<"end"> + ikw<"select"> + lit<";">)  //
             | create<ast::SelectStmt>(
                   ikw<"select"> +
                   append<&ast::SelectStmt::alts>(ref<EntryCallAlt>) +
                   flag<&ast::SelectStmt::isTimed>(ikw<"or">) +
                   append<&ast::SelectStmt::alts>(ref<DelayAlt>) +
                   ikw<"end"> + ikw<"select"> + lit<";">)  //
             | create<ast::SelectStmt>(
                   ikw<"select"> +
                   append<&ast::SelectStmt::alts>(ref<EntryCallAlt>) +
                   flag<&ast::SelectStmt::isConditional>(ikw<"else">) +
                   some(append<&ast::SelectStmt::elseBody>(ref<Stmt>)) +
                   ikw<"end"> + ikw<"select"> + lit<";">)  //
             | create<ast::SelectStmt>(
                   ikw<"select"> +
                   append<&ast::SelectStmt::alts>(ref<TriggerAlt>) +
                   flag<&ast::SelectStmt::isAsynchronous>(ikw<"then">) +
                   ikw<"abort"> +
                   some(append<&ast::SelectStmt::elseBody>(ref<Stmt>)) +
                   ikw<"end"> + ikw<"select"> + lit<";">)> {};

  struct CompoundStmt
      : rule<ref<IfStmt>          //
            | ref<CaseStmt>       //
            | ref<LoopStmt>       //
            | ref<BlockStmt>      //
            | ref<ExtReturnStmt>  //
            | ref<AcceptStmt>     //
            | ref<SelectStmt>> {};

  struct LabeledStmt
      : rule<create<ast::LabeledStmt>(
                 some(lit<"<<"> + append<&ast::LabeledStmt::labels>(Id) +
                      lit<">>">) +
                 option(assign<&ast::LabeledStmt::inner>(ref<SimpleStmt> |
                                                         ref<CompoundStmt>)))  //
             | ref<SimpleStmt>  //
             | ref<CompoundStmt>> {};

  struct Stmt : rule<ref<LabeledStmt>> {};

  struct ExcHandler
      : rule<create<ast::ExcHandler>(
            ikw<"when"> +
            option(assign<&ast::ExcHandler::name>(Id) + lit<":">) +
            some(append<&ast::ExcHandler::choices>(ref<NameExpr> |
                                                    ref<Others>),
                 lit<"|">) +
            lit<"=>"> + some(append<&ast::ExcHandler::body>(ref<Stmt>)))> {};

  struct StmtSeq
      : rule<create<ast::StmtSeq>(
            some(append<&ast::StmtSeq::body>(ref<Stmt>)) +
            option(ikw<"exception"> +
                   some(append<&ast::StmtSeq::handlers>(
                       ref<ExcHandler>))))> {};

  // -- Objects, renamings, exceptions ------------------------------------

  struct ObjectDecl
      : rule<create<ast::ObjectDecl>(
            some(append<&ast::ObjectDecl::names>(Id), lit<",">) + lit<":"> +
            option(flag<&ast::ObjectDecl::aliased_>(ikw<"aliased">)) +
            option(flag<&ast::ObjectDecl::constant_>(ikw<"constant">)) +
            assign<&ast::ObjectDecl::type>(ref<AccessDef> |
                                           ref<ArrayTypeDef> |
                                           ref<SubtypeIndication>) +
            option(lit<":="> +
                   assign<&ast::ObjectDecl::init>(ref<Expression>)) +
            AspectSpecOpt<&ast::ObjectDecl::aspects> + lit<";">)> {};

  struct NumberDecl
      : rule<create<ast::ObjectDecl>(
            some(append<&ast::ObjectDecl::names>(Id), lit<",">) + lit<":"> +
            flag<&ast::ObjectDecl::constant_>(ikw<"constant">) +
            lit<":="> + assign<&ast::ObjectDecl::init>(ref<Expression>) +
            lit<";">)> {};

  struct ExceptionDecl
      : rule<create<ast::ExceptionDecl>(
            some(append<&ast::ExceptionDecl::names>(Id), lit<",">) +
            lit<":"> + ikw<"exception"> + AspectSpecOpt<&ast::ExceptionDecl::aspects> + lit<";">)> {};

  template <auto Negative, auto Present>
  static constexpr auto OverridingOpt =
      option(option(flag<Negative>(ikw<"not">)) +
             flag<Present>(ikw<"overriding">));

  struct SubprogramSpec
      : rule<create<ast::SubprogramSpec>(
                 OverridingOpt<&ast::SubprogramSpec::notOverriding,
                               &ast::SubprogramSpec::overriding_> +
                 ikw<"procedure"> +
                 assign<&ast::SubprogramSpec::name>(QualName) +
                 option(assign<&ast::SubprogramSpec::params>(
                     ref<FormalPart>)))  //
             | create<ast::SubprogramSpec>(
                   OverridingOpt<&ast::SubprogramSpec::notOverriding,
                                 &ast::SubprogramSpec::overriding_> +
                   flag<&ast::SubprogramSpec::isFunction>(ikw<"function">) +
                   assign<&ast::SubprogramSpec::name>(
                       QualName | ref<STRING_LITERAL>) +
                   option(assign<&ast::SubprogramSpec::params>(
                       ref<FormalPart>)) +
                   ikw<"return"> + option(flag<&ast::SubprogramSpec::returnNullExclusion>(
                       ikw<"not"> + ikw<"null">)) +
                   assign<&ast::SubprogramSpec::returnType>(
                       ref<AccessDef> | ref<SubtypeMarkExpr>))> {
  };

  struct RenameDecl
      : rule<create<ast::RenameDecl>(
                 assign<&ast::RenameDecl::name>(Id) + lit<":"> +
                 option(flag<&ast::RenameDecl::nullExclusion>(
                     ikw<"not"> + ikw<"null">)) +
                 assign<&ast::RenameDecl::type>(
                     ref<AccessDef> | ref<SubtypeMarkExpr>) +
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
                   assign<&ast::RenameDecl::spec>(ref<SubprogramSpec>) +
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

  struct EnumLiteral
      : rule<assign<&ast::Ident::text>(Id) | ref<CharLit>> {};

  struct EnumTypeDef
      : rule<create<ast::EnumTypeDef>(
            lit<"("> +
            some(append<&ast::EnumTypeDef::literals>(ref<EnumLiteral>),
                 lit<",">) +
            lit<")">)> {};

  struct ModularTypeDef
      : rule<create<ast::ModularTypeDef>(
            ikw<"mod"> +
            assign<&ast::ModularTypeDef::modulus>(ref<Expression>))> {};

  struct IntegerTypeDef
      : rule<create<ast::IntegerTypeDef>(
            ikw<"range"> +
            assign<&ast::IntegerTypeDef::range>(create<ast::RangeExpr>(
                assign<&ast::RangeExpr::low>(ref<Expression>) + lit<".."> +
                assign<&ast::RangeExpr::high>(ref<Expression>))))> {};

  struct FloatTypeDef
      : rule<create<ast::FloatTypeDef>(
            ikw<"digits"> +
            assign<&ast::FloatTypeDef::digits_>(ref<Expression>) +
            option(ikw<"range"> +
                   assign<&ast::FloatTypeDef::range>(create<ast::RangeExpr>(
                       assign<&ast::RangeExpr::low>(ref<Expression>) +
                       lit<".."> +
                       assign<&ast::RangeExpr::high>(ref<Expression>)))))> {};

  struct OrdinaryFixedDef
      : rule<create<ast::OrdinaryFixedDef>(
            ikw<"delta"> +
            assign<&ast::OrdinaryFixedDef::delta_>(ref<Expression>) +
            ikw<"range"> +
            assign<&ast::OrdinaryFixedDef::range>(create<ast::RangeExpr>(
                assign<&ast::RangeExpr::low>(ref<Expression>) + lit<".."> +
                assign<&ast::RangeExpr::high>(ref<Expression>))))> {};

  struct DecimalFixedDef
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

  struct IndexSubtypeDef
      : rule<assign<&ast::Ident::text>(QualName) + ikw<"range"> +
             lit<"<>">> {};

  struct ArrayTypeDef
      : rule<create<ast::ArrayTypeDef>(
                 ikw<"array"> + lit<"("> +
                 some(append<&ast::ArrayTypeDef::indexes>(
                          ref<IndexSubtypeDef>),
                      lit<",">) +
                 lit<")"> + ikw<"of"> +
                 option(flag<&ast::ArrayTypeDef::aliased_>(
                     ikw<"aliased">)) +
                 assign<&ast::ArrayTypeDef::componentType>(
                     ref<AccessDef> | ref<SubtypeIndication>))  //
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
                       ref<AccessDef> | ref<SubtypeIndication>))> {};

  struct VariantPart;

  struct ComponentDecl
      : rule<create<ast::ComponentDecl>(
            some(append<&ast::ComponentDecl::names>(Id), lit<",">) +
            lit<":"> +
            option(flag<&ast::ComponentDecl::aliased_>(
                ikw<"aliased">)) +
            assign<&ast::ComponentDecl::type>(ref<AccessDef> |
                                              ref<SubtypeIndication>) +
            option(lit<":="> +
                   assign<&ast::ComponentDecl::def>(ref<Expression>)) +
            AspectSpecOpt<&ast::ComponentDecl::aspects> + lit<";">)> {};

  struct ComponentItem
      : rule<ref<ComponentDecl> | ref<AspectClause> |
             ref<PragmaDecl>> {};

  struct Variant
      : rule<create<ast::Variant>(
            ikw<"when"> +
            some(append<&ast::Variant::choices>(DiscreteChoice |
                                                ref<Others>),
                 lit<"|">) +
            lit<"=>"> +
            (some(append<&ast::Variant::components>(ref<ComponentItem>))  //
             | (many(append<&ast::Variant::components>(ref<ComponentItem>)) +
                assign<&ast::Variant::nested>(ref<VariantPart>))  //
             | (ikw<"null"> + lit<";">)))> {};

  struct VariantPart
      : rule<create<ast::VariantPart>(
            ikw<"case"> +
            assign<&ast::VariantPart::discriminant>(Id) + ikw<"is"> +
            some(append<&ast::VariantPart::variants>(ref<Variant>)) +
            ikw<"end"> + ikw<"case"> + lit<";">)> {};

  struct RecordTypeDef
      : rule<create<ast::RecordTypeDef>(
                 option(option(flag<&ast::RecordTypeDef::abstract_>(ikw<"abstract">)) +
                        flag<&ast::RecordTypeDef::tagged_>(
                            ikw<"tagged">)) +
                 option(flag<&ast::RecordTypeDef::limited_>(
                     ikw<"limited">)) +
                 ikw<"record"> +
                 (some(append<&ast::RecordTypeDef::components>(
                          ref<ComponentItem>))  //
                  | (many(append<&ast::RecordTypeDef::components>(
                              ref<ComponentItem>)) +
                     assign<&ast::RecordTypeDef::variant>(
                         ref<VariantPart>))  //
                  | (ikw<"null"> + lit<";">)) +
                 ikw<"end"> + ikw<"record">)  //
             | create<ast::RecordTypeDef>(
                   option(option(flag<&ast::RecordTypeDef::abstract_>(ikw<"abstract">)) +
                          flag<&ast::RecordTypeDef::tagged_>(
                              ikw<"tagged">)) +
                   option(flag<&ast::RecordTypeDef::limited_>(
                       ikw<"limited">)) +
                   ikw<"null"> +
                   flag<&ast::RecordTypeDef::isNullRecord>(
                       ikw<"record">))> {};

  struct InterfaceList
      : rule<some(append<&ast::DerivedTypeDef::interfaces>(
                      create<ast::Ident>(assign<&ast::Ident::text>(QualName))),
                  ikw<"and">)> {};

  struct DerivedTypeDef
      : rule<create<ast::DerivedTypeDef>(
            option(flag<&ast::DerivedTypeDef::abstract_>(
                ikw<"abstract">)) +
            option(flag<&ast::DerivedTypeDef::limited_>(
                ikw<"limited">)) +
            ikw<"new"> +
            assign<&ast::DerivedTypeDef::base>(ref<SubtypeIndication>) +
            option(
                option(ikw<"and"> +
                       some(append<&ast::DerivedTypeDef::interfaces>(
                                create<ast::Ident>(
                                    assign<&ast::Ident::text>(QualName))),
                            ikw<"and">)) +
                ikw<"with"> +
                assign<&ast::DerivedTypeDef::extension>(
                    ref<RecordTypeDef>)))> {};

  struct InterfaceTypeDef
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

  struct PrivateTypeDef
      : rule<create<ast::PrivateTypeDef>(
            option(option(flag<&ast::PrivateTypeDef::abstract_>(ikw<"abstract">)) +
                   flag<&ast::PrivateTypeDef::tagged_>(ikw<"tagged">)) +
            option(flag<&ast::PrivateTypeDef::limited_>(
                ikw<"limited">)) +
            ikw<"private">)> {};

  struct PrivateExtensionDef
      : rule<create<ast::PrivateExtensionDef>(
            option(flag<&ast::PrivateExtensionDef::abstract_>(
                ikw<"abstract">)) +
            option(assign<&ast::PrivateExtensionDef::kind>(
                ikw<"limited"> | ikw<"synchronized">)) +
            ikw<"new"> +
            assign<&ast::PrivateExtensionDef::base>(
                ref<SubtypeIndication>) +
            option(ikw<"and"> +
                   some(append<&ast::PrivateExtensionDef::interfaces>(
                            create<ast::Ident>(
                                assign<&ast::Ident::text>(QualName))),
                        ikw<"and">)) +
            ikw<"with"> + ikw<"private">)> {};

  struct TypeDef
      : rule<ref<EnumTypeDef>       //
            | ref<ModularTypeDef>   //
            | ref<IntegerTypeDef>   //
            | ref<FloatTypeDef>     //
            | ref<OrdinaryFixedDef> //
            | ref<DecimalFixedDef>  //
            | ref<ArrayTypeDef>     //
            | ref<RecordTypeDef>    //
            | ref<AccessDef>        //
            | ref<DerivedTypeDef>   //
            | ref<InterfaceTypeDef>> {};

  struct FullTypeDecl
      : rule<create<ast::TypeDecl>(
                 ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                 option(assign<&ast::TypeDecl::discriminants>(
                     ref<KnownDiscriminants>)) +
                 ikw<"is"> + assign<&ast::TypeDecl::def>(ref<TypeDef>) +
                 AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)  //
             | create<ast::TypeDecl>(
                   ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                   option(DiscriminantPartOpt) +
                   ikw<"is"> +
                   assign<&ast::TypeDecl::def>(ref<PrivateTypeDef>) +
                   AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)  //
             | create<ast::TypeDecl>(
                   ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                   option(DiscriminantPartOpt) +
                   flag<&ast::TypeDecl::privateExtension>(ikw<"is">) +
                   assign<&ast::TypeDecl::def>(
                       ref<PrivateExtensionDef>) +
                   AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)> {};

  struct IncompleteTypeDecl
      : rule<create<ast::TypeDecl>(
            ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
            option(DiscriminantPartOpt) +
            option(ikw<"is"> +
                   flag<&ast::TypeDecl::isTagged>(ikw<"tagged">)) +
            flag<&ast::TypeDecl::incomplete>(lit<";">))> {};

  struct TypeDecl
      : rule<ref<FullTypeDecl> | ref<IncompleteTypeDecl>> {};

  struct SubtypeDecl
      : rule<create<ast::TypeDecl>(
            ikw<"subtype"> + assign<&ast::TypeDecl::name>(Id) + ikw<"is"> +
            assign<&ast::TypeDecl::def>(ref<SubtypeIndication>) +
            AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)> {};

  // -- Subprograms -----------------------------------------------------

  struct AbstractSubprogramDecl
      : rule<create<ast::SubprogramDecl>(
            assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpec>) +
            ikw<"is"> +
            flag<&ast::SubprogramDecl::abstract_>(ikw<"abstract">) +
            AspectSpecOpt<&ast::SubprogramDecl::aspects> + lit<";">)> {};

  struct NullProcedureDecl
      : rule<create<ast::SubprogramDecl>(
            assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpec>) +
            ikw<"is"> +
            flag<&ast::SubprogramDecl::isNullProc>(ikw<"null">) +
            AspectSpecOpt<&ast::SubprogramDecl::aspects> + lit<";">)> {};

  struct ExpressionFuncDecl
      : rule<create<ast::SubprogramDecl>(
            assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpec>) +
            ikw<"is"> +
            assign<&ast::SubprogramDecl::exprBody>(
                (lit<"("> + ref<Expression> + lit<")">) | ref<Aggregate>) +
            AspectSpecOpt<&ast::SubprogramDecl::aspects> + lit<";">)> {};

  struct SubprogramDecl
      : rule<create<ast::SubprogramDecl>(
            assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpec>) +
            AspectSpecOpt<&ast::SubprogramDecl::aspects> + lit<";">)> {};

  struct SubprogramBody
      : rule<create<ast::SubprogramBody>(
            assign<&ast::SubprogramBody::spec>(ref<SubprogramSpec>) +
            AspectSpecOpt<&ast::SubprogramBody::aspects> + ikw<"is"> +
            many(append<&ast::SubprogramBody::decls>(ref<DeclItem>)) +
            ikw<"begin"> +
            assign<&ast::SubprogramBody::body>(ref<StmtSeq>) +
            ikw<"end"> +
            option(assign<&ast::SubprogramBody::endName>(
                QualName | ref<STRING_LITERAL>)) +
            lit<";">)> {};

  // -- Packages --------------------------------------------------------

  struct PackageSpec
      : rule<create<ast::PackageDecl>(
            ikw<"package"> + assign<&ast::PackageDecl::name>(QualName) +
            AspectSpecOpt<&ast::PackageDecl::aspects> + ikw<"is"> +
            many(append<&ast::PackageDecl::visible>(ref<DeclItem>)) +
            option(ikw<"private"> +
                   many(append<&ast::PackageDecl::private_>(ref<DeclItem>))) +
            ikw<"end"> +
            option(assign<&ast::PackageDecl::endName>(QualName)))> {};

  struct PackageDecl : rule<ref<PackageSpec> + lit<";">> {};

  struct PackageBody
      : rule<create<ast::PackageBody>(
            ikw<"package"> + ikw<"body"> +
            assign<&ast::PackageBody::name>(QualName) + AspectSpecOpt<&ast::PackageBody::aspects> +
            ikw<"is"> +
            many(append<&ast::PackageBody::decls>(ref<DeclItem>)) +
            option(ikw<"begin"> +
                   assign<&ast::PackageBody::body>(ref<StmtSeq>)) +
            ikw<"end"> +
            option(assign<&ast::PackageBody::endName>(QualName)) +
            lit<";">)> {};

  // -- Entries, tasks, protected objects ---------------------------------

  struct EntryDecl
      : rule<create<ast::EntryDecl>(
            OverridingOpt<&ast::EntryDecl::notOverriding,
                          &ast::EntryDecl::overriding_> + ikw<"entry"> +
            assign<&ast::EntryDecl::name>(Id) +
            option(lit<"("> +
                   assign<&ast::EntryDecl::familyRange>(ref<DiscreteRange>) +
                   lit<")">) +
            option(assign<&ast::EntryDecl::params>(ref<FormalPart>)) +
            AspectSpecOpt<&ast::EntryDecl::aspects> + lit<";">)> {};

  struct EntryIndexSpec
      : rule<create<ast::Param>(
            ikw<"for"> + append<&ast::Param::names>(Id) + ikw<"in"> +
            assign<&ast::Param::type>(ref<DiscreteRange>))> {};

  struct EntryBody
      : rule<create<ast::EntryBody>(
            ikw<"entry"> + assign<&ast::EntryBody::name>(Id) +
            option(lit<"("> +
                   assign<&ast::EntryBody::index>(ref<EntryIndexSpec>) +
                   lit<")">) +
            option(assign<&ast::EntryBody::params>(ref<FormalPart>)) +
            ikw<"when"> + assign<&ast::EntryBody::barrier>(ref<Expression>) +
            ikw<"is"> + many(append<&ast::EntryBody::decls>(ref<DeclItem>)) +
            ikw<"begin"> + assign<&ast::EntryBody::body>(ref<StmtSeq>) +
            ikw<"end"> + option(assign<&ast::EntryBody::endName>(DirectNameText)) + lit<";">)> {};

  struct TaskItem
      : rule<ref<EntryDecl> | ref<AspectClause> |
             ref<PragmaDecl>> {};

  struct InterfacesAnd
      : rule<some(append<&ast::TaskSpec::interfaces>(
                      create<ast::Ident>(assign<&ast::Ident::text>(QualName))),
                  ikw<"and">)> {};

  struct TaskSpec
      : rule<create<ast::TaskSpec>(
            ikw<"task"> +
            option(flag<&ast::TaskSpec::isType>(ikw<"type">)) +
            assign<&ast::TaskSpec::name>(Id) +
            option(assign<&ast::TaskSpec::discriminants>(
                ref<KnownDiscriminants>)) +
            AspectSpecOpt<&ast::TaskSpec::aspects> +
            option(flag<&ast::TaskSpec::hasDef>(ikw<"is">) +
                   option(ikw<"new"> +
                          some(append<&ast::TaskSpec::interfaces>(create<ast::Ident>(
                                   assign<&ast::Ident::text>(QualName))),
                               ikw<"and">) +
                          ikw<"with">) +
                   many(append<&ast::TaskSpec::items>(ref<TaskItem>)) +
                   option(ikw<"private"> +
                          many(append<&ast::TaskSpec::privateItems>(
                              ref<TaskItem>))) +
                   ikw<"end"> + option(assign<&ast::TaskSpec::endName>(DirectNameText))) +
            lit<";">)> {};

  struct TaskBody
      : rule<create<ast::TaskBody>(
            ikw<"task"> + ikw<"body"> + assign<&ast::TaskBody::name>(Id) +
            AspectSpecOpt<&ast::TaskBody::aspects> + ikw<"is"> +
            many(append<&ast::TaskBody::decls>(ref<DeclItem>)) +
            ikw<"begin"> + assign<&ast::TaskBody::body>(ref<StmtSeq>) +
            ikw<"end"> + option(assign<&ast::TaskBody::endName>(Id)) +
            lit<";">)> {};

  struct ProtectedOpDecl
      : rule<ref<SubprogramDecl> | ref<EntryDecl> |
             ref<AspectClause> | ref<PragmaDecl>> {};

  struct ProtectedElemDecl
      : rule<ref<ProtectedOpDecl> | ref<ComponentDecl>> {};

  struct ProtectedSpec
      : rule<create<ast::ProtectedSpec>(
            ikw<"protected"> +
            option(flag<&ast::ProtectedSpec::isType>(ikw<"type">)) +
            assign<&ast::ProtectedSpec::name>(Id) +
            option(assign<&ast::ProtectedSpec::discriminants>(
                ref<KnownDiscriminants>)) +
            AspectSpecOpt<&ast::ProtectedSpec::aspects> + ikw<"is"> +
            option(ikw<"new"> +
                   some(append<&ast::ProtectedSpec::interfaces>(create<ast::Ident>(
                            assign<&ast::Ident::text>(QualName))),
                        ikw<"and">) +
                   ikw<"with">) +
            many(append<&ast::ProtectedSpec::items>(ref<ProtectedOpDecl>)) +
            option(ikw<"private"> +
                   many(append<&ast::ProtectedSpec::privateItems>(
                       ref<ProtectedElemDecl>))) +
            ikw<"end"> + option(assign<&ast::ProtectedSpec::endName>(DirectNameText)) + lit<";">)> {};

  struct ProtectedOperationItem
      : rule<ref<SubprogramDecl> | ref<SubprogramBody> |
             ref<NullProcedureDecl> | ref<ExpressionFuncDecl> |
             ref<EntryBody> | ref<AspectClause> |
             ref<PragmaDecl>> {};

  struct ProtectedBody
      : rule<create<ast::ProtectedBody>(
            ikw<"protected"> + ikw<"body"> +
            assign<&ast::ProtectedBody::name>(Id) + AspectSpecOpt<&ast::ProtectedBody::aspects> +
            ikw<"is"> +
            many(append<&ast::ProtectedBody::items>(
                ref<ProtectedOperationItem>)) +
            ikw<"end"> + option(assign<&ast::ProtectedBody::endName>(DirectNameText)) + lit<";">)> {};

  // -- Generics -----------------------------------------------------

  struct GenericFormalObject
      : rule<create<ast::ObjectDecl>(
            some(append<&ast::ObjectDecl::names>(Id), lit<",">) + lit<":"> +
            assign<&ast::ObjectDecl::mode>(ModeText) +
            option(flag<&ast::ObjectDecl::nullExclusion>(
                ikw<"not"> + ikw<"null">)) +
            assign<&ast::ObjectDecl::type>(ref<AccessDef> |
                                           ref<SubtypeMarkExpr>) +
            option(lit<":="> +
                   assign<&ast::ObjectDecl::init>(ref<Expression>)) +
            AspectSpecOpt<&ast::ObjectDecl::aspects> + lit<";">)> {};

  struct FormalDerivedDef
      : rule<create<ast::FormalDerivedDef>(
            option(flag<&ast::FormalDerivedDef::abstract_>(
                ikw<"abstract">)) +
            option(flag<&ast::FormalDerivedDef::limited_>(
                ikw<"limited">)) +
            option(flag<&ast::FormalDerivedDef::synchronized_>(ikw<"synchronized">)) + ikw<"new"> +
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

  struct FormalTypeDef
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
                   lit<"<>"> + flag<&ast::FormalTypeDef::decimalFixed>(ikw<"digits">) + lit<"<>">)  //
             | create<ast::FormalTypeDef>(
                   assign<&ast::FormalTypeDef::kind>(ikw<"delta">) +
                   lit<"<>">)> {};

  struct FormalTypeDefDispatch
      : rule<ref<FormalDerivedDef> | ref<FormalTypeDef> |
             ref<ArrayTypeDef> | ref<AccessDef> |
             ref<InterfaceTypeDef> | ref<PrivateTypeDef>> {};

  struct GenericFormalType
      : rule<create<ast::TypeDecl>(
                 ikw<"type"> + assign<&ast::TypeDecl::name>(Id) +
                 option(DiscriminantPartOpt) + ikw<"is"> +
                 assign<&ast::TypeDecl::def>(ref<FormalTypeDefDispatch>) +
                 AspectSpecOpt<&ast::TypeDecl::aspects> + lit<";">)  //
             | ref<IncompleteTypeDecl>> {};

  struct SubprogramDefault
      : rule<ref<NameExpr> | ref<Box> | ref<NullLit>> {};

  struct GenericFormalSubprogram
      : rule<create<ast::GenericFormalSubprogram>(
                 ikw<"with"> +
                 assign<&ast::GenericFormalSubprogram::spec>(
                     ref<SubprogramSpec>) +
                 ikw<"is"> +
                 flag<&ast::GenericFormalSubprogram::abstract_>(
                     ikw<"abstract">) +
                 option(assign<&ast::GenericFormalSubprogram::default_>(
                     ref<SubprogramDefault>)) +
                 AspectSpecOpt<&ast::GenericFormalSubprogram::aspects> + lit<";">)  //
             | create<ast::GenericFormalSubprogram>(
                   ikw<"with"> +
                   assign<&ast::GenericFormalSubprogram::spec>(
                       ref<SubprogramSpec>) +
                   option(ikw<"is"> +
                          assign<&ast::GenericFormalSubprogram::default_>(
                              ref<SubprogramDefault>)) +
                   AspectSpecOpt<&ast::GenericFormalSubprogram::aspects> + lit<";">)> {};

  struct GenericFormalPackage
      : rule<create<ast::GenericFormalPackage>(
            ikw<"with"> + ikw<"package"> +
            assign<&ast::GenericFormalPackage::name>(Id) + ikw<"is"> +
            ikw<"new"> +
            assign<&ast::GenericFormalPackage::instantiated>(QualName) +
            (flag<&ast::GenericFormalPackage::isBox>(
                 lit<"("> + option(flag<&ast::GenericFormalPackage::othersBox>(ikw<"others">) + lit<"=>">) + lit<"<>"> +
                 lit<")">)  //
             | option(lit<"("> +
                      some(append<&ast::GenericFormalPackage::actuals>(
                               ref<Arg>),
                           lit<",">) +
                      option(lit<","> + flag<&ast::GenericFormalPackage::othersBox>(ikw<"others">) + lit<"=>"> +
                             lit<"<>">) +
                      lit<")">)) +
            AspectSpecOpt<&ast::GenericFormalPackage::aspects> + lit<";">)> {};

  struct GenericFormalDecl
      : rule<ref<GenericFormalObject> | ref<GenericFormalType> |
             ref<GenericFormalSubprogram> |
             ref<GenericFormalPackage>> {};

  static constexpr auto GenericFormalPart =
      ikw<"generic"> +
      many(append<&ast::GenericDecl::formals>(ref<GenericFormalDecl> |
                                              ref<UseClause>));

  struct GenericDecl
      : rule<create<ast::GenericDecl>(
                 GenericFormalPart +
                 assign<&ast::GenericDecl::subprogSpec>(
                     ref<SubprogramSpec>) +
                 AspectSpecOpt<&ast::GenericDecl::aspects> + lit<";">)  //
             | create<ast::GenericDecl>(
                   GenericFormalPart +
                   assign<&ast::GenericDecl::packageSpec>(
                       ref<PackageSpec>) +
                   lit<";">)> {};

  struct GenericInstantiation
      : rule<create<ast::GenericInstantiation>(
                 assign<&ast::GenericInstantiation::kind>(ikw<"package">) +
                 assign<&ast::GenericInstantiation::name>(QualName) +
                 ikw<"is"> + ikw<"new"> +
                 assign<&ast::GenericInstantiation::generic>(QualName) +
                 option(lit<"("> +
                        some(append<&ast::GenericInstantiation::actuals>(
                                 ref<Arg>),
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
                                   ref<Arg>),
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
                                   ref<Arg>),
                               lit<",">) +
                          lit<")">) +
                   AspectSpecOpt<&ast::GenericInstantiation::aspects> + lit<";">)> {};

  // -- Use clauses, representation clauses -------------------------------

  struct UseClause
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

  struct RecordCompClause
      : rule<create<ast::RecordCompClause>(
            assign<&ast::RecordCompClause::name>(QualName) + ikw<"at"> +
            assign<&ast::RecordCompClause::position>(ref<Expression>) +
            ikw<"range"> +
            assign<&ast::RecordCompClause::first>(ref<Simple>) + lit<".."> +
            assign<&ast::RecordCompClause::last>(ref<Simple>) + lit<";">)> {
  };

  struct AspectClause
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
                       ref<RecordCompClause>)) +
                   ikw<"end"> + ikw<"record"> + lit<";">)  //
             | create<ast::AspectClause>(
                   ikw<"for"> + assign<&ast::AspectClause::name>(Id) +
                   ikw<"use"> + flag<&ast::AspectClause::isAt>(ikw<"at">) +
                   assign<&ast::AspectClause::value>(ref<Expression>) +
                   lit<";">)> {};

  // -- Body stubs, subunits, compilation -----------------------------

  struct BodyStub
      : rule<create<ast::BodyStub>(
                 assign<&ast::BodyStub::spec>(ref<SubprogramSpec>) +
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

  struct ProperBody
      : rule<ref<SubprogramBody> | ref<PackageBody> |
             ref<TaskBody> | ref<ProtectedBody>> {};

  struct Subunit
      : rule<create<ast::Subunit>(
            ikw<"separate"> + lit<"("> +
            assign<&ast::Subunit::parent>(QualName) + lit<")"> +
            assign<&ast::Subunit::body>(ref<ProperBody>))> {};

  struct WithClause
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
      : rule<ref<PragmaDecl>              //
            | ref<TaskSpec>               //
            | ref<ProtectedSpec>          //
            | ref<TypeDecl>               //
            | ref<SubtypeDecl>            //
            | ref<ObjectDecl>             //
            | ref<NumberDecl>             //
            | ref<AbstractSubprogramDecl> //
            | ref<NullProcedureDecl>      //
            | ref<ExpressionFuncDecl>     //
            | ref<SubprogramDecl>         //
            | ref<PackageDecl>            //
            | ref<RenameDecl>             //
            | ref<ExceptionDecl>          //
            | ref<GenericDecl>            //
            | ref<GenericInstantiation>   //
            | ref<UseClause>              //
            | ref<AspectClause>           //
            | ref<SubprogramBody>         //
            | ref<PackageBody>            //
            | ref<TaskBody>               //
            | ref<ProtectedBody>          //
            | ref<BodyStub>> {};

  struct UnitRule
      : rule<create<ast::Unit>(
            many(append<&ast::Unit::withClauses>(ref<WithClause>) |
                 append<&ast::Unit::useClauses>(ref<UseClause>) |
                 append<&ast::Unit::pragmas>(ref<PragmaDecl>)) +
            ((option(flag<&ast::Unit::private_>(ikw<"private">)) +
              assign<&ast::Unit::body>(
                  ref<SubprogramDecl> | ref<PackageDecl> |
                  ref<GenericDecl> | ref<GenericInstantiation> |
                  ref<RenameDecl>))  //
             | assign<&ast::Unit::body>(ref<SubprogramBody> |
                                        ref<PackageBody>)  //
             | assign<&ast::Unit::body>(ref<Subunit>)))> {};

  // A compilation of configuration pragmas alone (no library_item) is
  // legal -- e.g. a lone `pragma Restrictions (...);` file.
  struct Compilation
      : rule<create<ast::Compilation>(
            many(append<&ast::Compilation::units>(ref<UnitRule>) |
                 append<&ast::Compilation::units>(create<ast::Unit>(
                     append<&ast::Unit::pragmas>(ref<PragmaDecl>)))))> {};

  using entry = Compilation;
  using ignored = WS;
  using hidden = list<LINE_COMMENT>;
};

}  // namespace ada
