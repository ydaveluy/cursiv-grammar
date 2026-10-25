// The eiffel language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "eiffel/core/ast.hpp"

namespace eiffel {

struct Eiffel : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_À-ÿ">;

  struct WS : terminal<some(cr<R"( \t\n\r﻿)">)> {};
  struct COMMENT : terminal<lit<"--"> + many(cr<R"(^\n)">)> {};

  struct ID : terminal<cr<"a-zA-ZÀ-ÿ"> + many(cr<"a-zA-ZÀ-ÿ0-9_">)> {};
  // "tuple" is a soft keyword: spelled once so it stays usable as a name
  // everywhere else (docs/reference/grammar.md, soft keywords).
  struct NAME : rule<ref<ID> | ikw<"tuple">> {};

  static constexpr auto STR_ESCAPE =
      lit<"%\""> | lit<"%%"> |
      (lit<"%"> + many(cr<R"( \t)">) + lit<"\n"> + many(cr<R"( \t)">) + lit<"%">);
  struct BASIC_STRING
      : terminal<lit<"\""> + many(STR_ESCAPE | (!cr<R"("\n)"> + any)) + lit<"\"">> {};
  // Over-accepts a verbatim string with no interior newline: a rare
  // multi-line-only spelling, widened with no semantic loss.
  struct VERBATIM_STRING
      : terminal<through(lit<"\"[">, lit<"]\"">) | through(lit<"\"{">, lit<"}\"">)> {};

  struct CHAR_CONST
      : terminal<lit<"'"> +
                 ((lit<"%"> + lit<"/"> + some(cr<"0-9a-fA-F_">) + lit<"/">) |
                  (lit<"%"> + any) | (!cr<"%'"> + any)) +
                 lit<"'">> {};

  static constexpr auto INT_BODY =
      (lit<"0"> + cr<"bBcCxX"> + some(cr<"0-9a-fA-F_">)) |
      (cr<"0-9"> + many(cr<"0-9a-fA-F_">));
  struct INTEGER : terminal<INT_BODY> {};
  struct REAL
      : terminal<(many(cr<"0-9">) + lit<"."> + some(cr<"0-9">) +
                  option(cr<"eE"> + option(cr<"+-">) + some(cr<"0-9">))) |
                 (some(cr<"0-9">) + lit<".", !lit<".">> + many(cr<"0-9">) +
                  option(cr<"eE"> + option(cr<"+-">) + some(cr<"0-9">)))> {};

  // Admits {":=.", ":=+", ":=-", "<<-", "<<+", "<<>", "<<>>"} as free
  // operators too (over-acceptance of a handful of unusual spellings, not
  // exercised by any corpus file).
  // Never spelled as a standard operator: those are literals of their own.
  static constexpr auto FREE_CH = cr<R"(-!#$%&*+\\/:.<=>?@^|~--¡-¬°-¶)"
                                     "¹-¿×÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿"
                                     "■-◿←-⇿⟰-⟿⤀-⥿⬀-⯿">;
  struct FREE_OP
      : terminal<cr<R"(#%&\\@|--¡-¥§-«°-¶¹-¿×÷)"
                    "∁-∂∄-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿■-"
                    "◿←-⇿⟰-⟱⟴-⟿⤀-⥿⬀-⯿">
                 | (cr<"!#$%&*+=?@^|~--¡-¬°-¶¹-¿×"
                       "÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿■-◿"
                       "←-⇿⟰-⟿⤀-⥿⬀-⯿"> + FREE_CH)
                 | (lit<"-"> + cr<R"(-!#$%&*+\\/:.<=?@^|~--¡-¬°-¶)"
                                  "¹-¿×÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿"
                                  "■-◿←-⇿⟰-⟿⤀-⥿⬀-⯿">)
                 | (lit<"."> + cr<R"(-!#$%&*+\\/:<=>?@^|~--¡-¬°-¶)"
                                  "¹-¿×÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿"
                                  "■-◿←-⇿⟰-⟿⤀-⥿⬀-⯿">)
                 | (lit<"/"> + cr<R"(-!#$%&*+\\:.<>?@^|--¡-¬°-¶¹-)"
                                  "¿×÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿■-"
                                  "◿←-⇿⟰-⟿⤀-⥿⬀-⯿">)
                 | (lit<":"> + cr<R"(-!#$%&*+\\/:.<>?@^|~--¡-¬°-¶)"
                                  "¹-¿×÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿"
                                  "■-◿←-⇿⟰-⟿⤀-⥿⬀-⯿">)
                 | (lit<"<"> + cr<R"(-!#$%&*+\\/:.>?@^|~--¡-¬°-¶¹-)"
                                  "¿×÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿■-"
                                  "◿←-⇿⟰-⟿⤀-⥿⬀-⯿">)
                 | (lit<">"> + cr<R"(-!#$%&*+\\/:.<?@^|~--¡-¬°-¶¹-)"
                                  "¿×÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿■-"
                                  "◿←-⇿⟰-⟿⤀-⥿⬀-⯿">)
                 | (lit<"\\"> + cr<"-!#$%&*+/:.<=>?@^|~--¡-¬°-¶¹-"
                                   "¿×÷∀-⋿⨀-⫿⟀-⟯⦀-⧿⌀-⏿■-"
                                   "◿←-⇿⟰-⟿⤀-⥿⬀-⯿">)
                 | (FREE_CH + FREE_CH + some(FREE_CH))> {};

  // ---- Types ----

  struct Expression;
  struct Type;
  struct Postfix;
  struct InstructionRule;

  struct EntityDeclGroupRule
      : rule<create<ast::EntityDeclGroup>(
                 append<&ast::EntityDeclGroup::names>(ref<NAME>) +
                 many(lit<","> + append<&ast::EntityDeclGroup::names>(ref<NAME>)) +
                 lit<":"> + assign<&ast::EntityDeclGroup::type>(ref<Type>))> {};

  struct Type
      : rule<many(append<&ast::Type::modifiers>(ikw<"attached"> | ikw<"detachable"> |
                                                 ikw<"separate"> | ikw<"expanded">)) +
             (create<ast::TupleType>(
                  ikw<"tuple"> + lit<"["> +
                  option(append<&ast::TupleType::params>(ref<EntityDeclGroupRule>) +
                         many(many(lit<";">) +
                              append<&ast::TupleType::params>(ref<EntityDeclGroupRule>))) +
                  lit<"]">)  //
              | create<ast::AnchoredType>(
                    option(assign<&ast::AnchoredType::attachment>(lit<"?"> | lit<"!">)) +
                    ikw<"like"> + assign<&ast::AnchoredType::anchor>(ref<Expression>))  //
              | create<ast::ClassType>(
                    option(assign<&ast::ClassType::attachment>(lit<"?"> | lit<"!">)) +
                    assign<&ast::ClassType::name>(ref<NAME>) +
                    option(lit<"["> +
                           append<&ast::ClassType::generics>(ref<Type>) +
                           many(lit<","> + append<&ast::ClassType::generics>(ref<Type>)) +
                           lit<"]">)))> {};

  struct ManifestValueRule
      : rule<ikw<"true"> | ikw<"false"> | ref<CHAR_CONST> |
             (option(lit<"+"> | lit<"-">) + ref<REAL>) |
             (option(lit<"+"> | lit<"-">) + ref<INTEGER>) | ref<BASIC_STRING> |
             ref<VERBATIM_STRING>> {};

  struct ManifestStringRule : rule<ref<BASIC_STRING> | ref<VERBATIM_STRING>> {};

  struct ManifestConstantRule
      : rule<create<ast::ManifestConstant>(
                 option(lit<"{"> + assign<&ast::ManifestConstant::type>(ref<Type>) +
                        lit<"}">) +
                 assign<&ast::ManifestConstant::text>(ref<ManifestValueRule>))> {};

  struct ManifestTypeOnlyRule
      : rule<create<ast::ManifestConstant>(
                 lit<"{"> + assign<&ast::ManifestConstant::type>(ref<Type>) + lit<"}">)> {
  };

  // ---- Expressions ----

  struct Compound;
  struct RoutineBodyRule;

  struct NonObjectCallRule
      : rule<create<ast::NonObjectCallExpr>(
                 lit<"{"> + assign<&ast::NonObjectCallExpr::type>(ref<Type>) + lit<"}"> +
                 lit<"."> + assign<&ast::NonObjectCallExpr::member>(ref<NAME>) +
                 option(flag<&ast::NonObjectCallExpr::called>(lit<"(">) +
                        option(append<&ast::NonObjectCallExpr::args>(ref<Expression>) +
                               many(lit<","> +
                                    append<&ast::NonObjectCallExpr::args>(ref<Expression>))) +
                        lit<")">))> {};

  struct PrecursorRule
      : rule<create<ast::Precursor>(
                 ikw<"precursor"> +
                 option(lit<"{"> + assign<&ast::Precursor::parentClass>(ref<NAME>) +
                        lit<"}">) +
                 option(flag<&ast::Precursor::called>(lit<"(">) +
                        option(append<&ast::Precursor::args>(ref<Expression>) +
                               many(lit<","> + append<&ast::Precursor::args>(ref<Expression>))) +
                        lit<")">))> {};

  struct AgentActualRule
      : rule<create<ast::AgentActual>(assign<&ast::AgentActual::value>(ref<Expression>))  //
             | create<ast::AgentActual>(
                   option(lit<"{"> +
                          assign<&ast::AgentActual::placeholderType>(ref<Type>) + lit<"}">) +
                   flag<&ast::AgentActual::placeholder>(lit<"?">))> {};

  struct AgentRule
      : rule<create<ast::Agent>(
                 ikw<"agent"> +
                 option(assign<&ast::Agent::target>(ref<Expression>) + lit<".">) +
                 option(assign<&ast::Agent::member>(ref<NAME>)) +
                 option(lit<"("> +
                        option(append<&ast::Agent::inlineParams>(ref<EntityDeclGroupRule>) +
                               many(many(lit<";">) +
                                    append<&ast::Agent::inlineParams>(ref<EntityDeclGroupRule>))) +
                        lit<")">) +
                 option(lit<":"> + assign<&ast::Agent::inlineType>(ref<Type>)) +
                 option(assign<&ast::Agent::inlineBody>(ref<RoutineBodyRule>)) +
                 option(flag<&ast::Agent::called>(lit<"(">) +
                        option(append<&ast::Agent::args>(ref<AgentActualRule>) +
                               many(lit<","> + append<&ast::Agent::args>(ref<AgentActualRule>))) +
                        lit<")">))> {};

  struct ObjectTestRule
      : rule<create<ast::ObjectTest>(
                 lit<"{"> + assign<&ast::ObjectTest::name>(ref<NAME>) + lit<":"> +
                 assign<&ast::ObjectTest::type>(ref<Type>) + lit<"}"> +
                 assign<&ast::ObjectTest::value>(ref<Expression>))> {};

  struct CondBranchRule
      : rule<create<ast::CondBranch>(assign<&ast::CondBranch::cond>(ref<Expression>) +
                                     ikw<"then"> +
                                     assign<&ast::CondBranch::value>(ref<Expression>))> {};

  struct Atom
      : rule<create<ast::ParenExpr>(lit<"("> +
                 assign<&ast::ParenExpr::inner>(ref<Expression>) + lit<")">)  //
             | create<ast::ParenTargetExpr>(
                   lit<"(|"> + assign<&ast::ParenTargetExpr::inner>(ref<Expression>) +
                   lit<"|)">)  //
             | create<ast::QuantifierExpr>(
                   assign<&ast::QuantifierExpr::kind>(lit<"∀">) +
                   assign<&ast::QuantifierExpr::name>(ref<NAME>) + lit<":"> +
                   assign<&ast::QuantifierExpr::domain>(ref<Expression>) + lit<"¦"> +
                   assign<&ast::QuantifierExpr::body>(ref<Expression>))  //
             | create<ast::QuantifierExpr>(
                   assign<&ast::QuantifierExpr::kind>(lit<"∃">) +
                   assign<&ast::QuantifierExpr::name>(ref<NAME>) + lit<":"> +
                   assign<&ast::QuantifierExpr::domain>(ref<Expression>) + lit<"¦"> +
                   assign<&ast::QuantifierExpr::body>(ref<Expression>))  //
             | create<ast::CurrentExpr>(ikw<"current">)  //
             | create<ast::ResultExpr>(ikw<"result">)    //
             | create<ast::VoidExpr>(ikw<"void">)        //
             | create<ast::AttachedExpr>(
                   ikw<"attached"> +
                   option(lit<"{"> + assign<&ast::AttachedExpr::type>(ref<Type>) + lit<"}">) +
                   assign<&ast::AttachedExpr::value>(ref<Expression>) +
                   option(ikw<"as"> + assign<&ast::AttachedExpr::asName>(ref<NAME>)))  //
             | create<ast::AcrossExpr>(
                   ikw<"across"> + assign<&ast::AcrossExpr::source>(ref<Expression>) +
                   ikw<"as"> + assign<&ast::AcrossExpr::varName>(ref<NAME>) +
                   (flag<&ast::AcrossExpr::isAll>(ikw<"all">) | ikw<"some">) +
                   assign<&ast::AcrossExpr::body>(ref<Expression>) + ikw<"end">)  //
             | create<ast::OnceExpr>(
                   ikw<"once"> +
                   option(lit<"{"> + assign<&ast::OnceExpr::type>(ref<Type>) + lit<"}">) +
                   assign<&ast::OnceExpr::value>(ref<Expression>))  //
             | create<ast::PrecursorExpr>(assign<&ast::PrecursorExpr::value>(ref<PrecursorRule>))  //
             | create<ast::CreationExpr>(
                   ikw<"create"> + lit<"{"> + assign<&ast::CreationExpr::type>(ref<Type>) +
                   lit<"}"> +
                   option(lit<"."> + assign<&ast::CreationExpr::member>(ref<NAME>) +
                          option(flag<&ast::CreationExpr::called>(lit<"(">) +
                                 option(append<&ast::CreationExpr::args>(ref<Expression>) +
                                        many(lit<","> +
                                             append<&ast::CreationExpr::args>(ref<Expression>))) +
                                 lit<")">)))  //
             | create<ast::ConditionalExpr>(
                   ikw<"if"> + append<&ast::ConditionalExpr::branches>(ref<CondBranchRule>) +
                   many(ikw<"elseif"> +
                        append<&ast::ConditionalExpr::branches>(ref<CondBranchRule>)) +
                   ikw<"else"> + assign<&ast::ConditionalExpr::elseValue>(ref<Expression>) +
                   ikw<"end">)  //
             | ref<NonObjectCallRule>  //
             | ref<ManifestConstantRule>  //
             | create<ast::ManifestArray>(
                   option(lit<"{"> + assign<&ast::ManifestArray::type>(ref<Type>) + lit<"}">) +
                   lit<"<<"> +
                   option(append<&ast::ManifestArray::items>(ref<Expression>) +
                          many(lit<","> + append<&ast::ManifestArray::items>(ref<Expression>))) +
                   lit<">>">)  //
             | create<ast::ManifestTuple>(
                   lit<"["> +
                   option(append<&ast::ManifestTuple::items>(ref<Expression>) +
                          many(lit<","> + append<&ast::ManifestTuple::items>(ref<Expression>))) +
                   lit<"]">)  //
             | ref<AgentRule>  //
             | ref<ObjectTestRule>  //
             | create<ast::OnceStringExpr>(
                   ikw<"once"> + assign<&ast::OnceStringExpr::text>(ref<ManifestStringRule>))  //
             | create<ast::AddressExpr>(
                   lit<"$"> + assign<&ast::AddressExpr::operand>(ref<Postfix>))  //
             | create<ast::NameExpr>(
                   assign<&ast::NameExpr::name>(ref<NAME>) +
                   option(flag<&ast::NameExpr::called>(lit<"(">) +
                          option(append<&ast::NameExpr::args>(ref<Expression>) +
                                 many(lit<","> + append<&ast::NameExpr::args>(ref<Expression>))) +
                          lit<")">))> {};

  struct Postfix
      : rule<ref<Atom> +
             many(nest<ast::QualifiedCall, &ast::QualifiedCall::target>(
                      lit<"."> + assign<&ast::QualifiedCall::member>(ref<NAME>) +
                      option(flag<&ast::QualifiedCall::called>(lit<"(">) +
                             option(append<&ast::QualifiedCall::args>(ref<Expression>) +
                                    many(lit<","> +
                                         append<&ast::QualifiedCall::args>(ref<Expression>))) +
                             lit<")">))  //
                  | nest<ast::IndexExpr, &ast::IndexExpr::target>(
                        lit<"["> +
                        option(append<&ast::IndexExpr::indices>(ref<Expression>) +
                               many(lit<","> +
                                    append<&ast::IndexExpr::indices>(ref<Expression>))) +
                        lit<"]">))> {};

  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"old"> | ikw<"not"> | lit<"¬"> |
                                             lit<"+"> | lit<"-"> | ref<FREE_OP>) +
                 assign<&ast::UnaryExpr::operand>(ref<Unary>))  //
             | ref<Postfix>> {};

  struct AndThenOp : rule<ikw<"and"> + ikw<"then">> {};
  struct OrElseOp : rule<ikw<"or"> + ikw<"else">> {};

  struct AddSubLevel
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (left(ref<FREE_OP>) >>                                        //
               right(lit<"^">) >>                                          //
               left(lit<"*"> | lit<"/"> | lit<"//"> | lit<"\\\\">) >>      //
               left(lit<"+"> | lit<"-">))> {};

  struct IntervalLevel
      : rule<create<ast::IntervalExpr>(
                 assign<&ast::IntervalExpr::first>(ref<AddSubLevel>) + lit<".."> +
                 assign<&ast::IntervalExpr::last>(ref<AddSubLevel>))  //
             | ref<AddSubLevel>> {};

  struct RelLevel
      : infix<IntervalLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(lit<"="> | lit<"/="> | lit<"~"> | lit<"/~"> | lit<"<="> | lit<">="> |
                   lit<"<"> | lit<">">)> {};

  struct AndLevel
      : infix<RelLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              left(ikw<"and"> | ref<AndThenOp>)> {};

  struct OrLevel
      : infix<AndLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              left(ikw<"or"> | ref<OrElseOp> | ikw<"xor">)> {};

  struct ImpliesLevel
      : infix<OrLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              left(ikw<"implies">)> {};

  struct Expression : rule<ref<ImpliesLevel>> {};

  // ---- Instructions ----

  struct Compound
      : rule<create<ast::Compound> + many(lit<";">) +
             many(append<&ast::Compound::items>(ref<InstructionRule>) +
                  many(lit<";">))> {};

  struct AssertionClauseRule
      : rule<(option(assign<&ast::AssertionClause::tag>(ref<NAME>) + lit<":">) +
              (assign<&ast::AssertionClause::condition>(ref<Expression>) |
               flag<&ast::AssertionClause::isClassMark>(ikw<"class">)))  //
             | (assign<&ast::AssertionClause::tag>(ref<NAME>) + lit<":">)> {};

  struct AssertionRule
      : rule<create<ast::Assertion> + many(lit<";">) +
             many(append<&ast::Assertion::clauses>(ref<AssertionClauseRule>) +
                  many(lit<";">))> {};

  struct AssignmentRule
      : rule<create<ast::Assignment>(assign<&ast::Assignment::target>(ref<Expression>) +
                                     lit<":="> +
                                     assign<&ast::Assignment::value>(ref<Expression>))> {};

  struct CreationInstructionRule
      : rule<create<ast::CreationInstruction>(
                 ikw<"create"> +
                 option(lit<"<"> + assign<&ast::CreationInstruction::passiveRegionClass>(ref<NAME>) +
                        lit<">">) +
                 option(lit<"{"> + assign<&ast::CreationInstruction::type>(ref<Type>) +
                        lit<"}">) +
                 assign<&ast::CreationInstruction::target>(ref<Expression>) +
                 option(lit<"."> + assign<&ast::CreationInstruction::member>(ref<NAME>) +
                        option(flag<&ast::CreationInstruction::called>(lit<"(">) +
                               option(append<&ast::CreationInstruction::args>(ref<Expression>) +
                                      many(lit<","> + append<&ast::CreationInstruction::args>(
                                                          ref<Expression>))) +
                               lit<")">)))> {};

  struct IfBranchRule
      : rule<create<ast::IfBranch>(assign<&ast::IfBranch::cond>(ref<Expression>) +
                                   ikw<"then"> +
                                   assign<&ast::IfBranch::body>(ref<Compound>))> {};

  struct ConditionalRule
      : rule<create<ast::IfInstruction>(
                 ikw<"if"> + append<&ast::IfInstruction::branches>(ref<IfBranchRule>) +
                 many(ikw<"elseif"> + append<&ast::IfInstruction::branches>(ref<IfBranchRule>)) +
                 option(ikw<"else"> + assign<&ast::IfInstruction::elseBody>(ref<Compound>)) +
                 ikw<"end">)> {};

  struct ChoiceAtomRule
      : rule<ref<ManifestConstantRule> | ref<ManifestTypeOnlyRule> | ref<NonObjectCallRule> |
             create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<NAME>))> {};

  struct ChoiceRule
      : rule<create<ast::Choice>(
                 assign<&ast::Choice::low>(ref<ChoiceAtomRule>) +
                 option(lit<".."> + assign<&ast::Choice::high>(ref<ChoiceAtomRule>)))> {};

  struct WhenBranchRule
      : rule<create<ast::WhenBranch>(
                 ikw<"when"> + append<&ast::WhenBranch::choices>(ref<ChoiceRule>) +
                 many(lit<","> + append<&ast::WhenBranch::choices>(ref<ChoiceRule>)) +
                 ikw<"then"> + assign<&ast::WhenBranch::body>(ref<Compound>))> {};

  struct MultiBranchRule
      : rule<create<ast::InspectInstruction>(
                 ikw<"inspect"> + assign<&ast::InspectInstruction::subject>(ref<Expression>) +
                 many(append<&ast::InspectInstruction::branches>(ref<WhenBranchRule>)) +
                 option(ikw<"else"> +
                        assign<&ast::InspectInstruction::elseBody>(ref<Compound>)) +
                 ikw<"end">)> {};

  struct LoopRule
      : rule<create<ast::LoopInstruction>(
                 option(ikw<"across"> + assign<&ast::LoopInstruction::acrossSource>(ref<Expression>) +
                        ikw<"as"> + assign<&ast::LoopInstruction::acrossVar>(ref<NAME>)) +
                 option(ikw<"from"> + assign<&ast::LoopInstruction::initBody>(ref<Compound>)) +
                 option(ikw<"invariant"> +
                        assign<&ast::LoopInstruction::invariant>(ref<AssertionRule>)) +
                 option(ikw<"variant"> + option(ref<NAME> + lit<":">) +
                        assign<&ast::LoopInstruction::variantBefore>(ref<Expression>)) +
                 option(ikw<"until"> + assign<&ast::LoopInstruction::untilCond>(ref<Expression>)) +
                 ((ikw<"loop"> + assign<&ast::LoopInstruction::loopBody>(ref<Compound>))  //
                  | (ikw<"all"> + assign<&ast::LoopInstruction::allCond>(ref<Expression>))  //
                  | (ikw<"some"> + assign<&ast::LoopInstruction::someCond>(ref<Expression>))) +
                 option(ikw<"variant"> + option(ref<NAME> + lit<":">) +
                        assign<&ast::LoopInstruction::variantAfter>(ref<Expression>)) +
                 ikw<"end">)> {};

  struct DebugRule
      : rule<create<ast::DebugInstruction>(
                 ikw<"debug"> +
                 option(lit<"("> +
                        append<&ast::DebugInstruction::keys>(ref<ManifestStringRule>) +
                        many(lit<","> +
                             append<&ast::DebugInstruction::keys>(ref<ManifestStringRule>)) +
                        lit<")">) +
                 assign<&ast::DebugInstruction::body>(ref<Compound>) + ikw<"end">)> {};

  struct NotesRule;

  struct CheckRule
      : rule<create<ast::CheckInstruction>(
                 ikw<"check"> + assign<&ast::CheckInstruction::assertion>(ref<AssertionRule>) +
                 option(assign<&ast::CheckInstruction::notes>(ref<NotesRule>)) + ikw<"end">)> {};

  struct ConditionalCheckRule
      : rule<create<ast::ConditionalCheckInstruction>(
                 ikw<"check"> +
                 assign<&ast::ConditionalCheckInstruction::assertion>(ref<AssertionRule>) +
                 ikw<"then"> +
                 assign<&ast::ConditionalCheckInstruction::body>(ref<Compound>) +
                 ikw<"end">)> {};

  struct RetryRule : rule<create<ast::RetryInstruction>(ikw<"retry">)> {};

  struct InlineSeparateArgRule
      : rule<create<ast::InlineSeparateArg>(
                 assign<&ast::InlineSeparateArg::value>(ref<Expression>) + ikw<"as"> +
                 assign<&ast::InlineSeparateArg::name>(ref<NAME>))> {};

  struct InlineSeparateRule
      : rule<create<ast::InlineSeparateInstruction>(
                 ikw<"separate"> +
                 append<&ast::InlineSeparateInstruction::args>(ref<InlineSeparateArgRule>) +
                 many(lit<","> +
                      append<&ast::InlineSeparateInstruction::args>(ref<InlineSeparateArgRule>)) +
                 ikw<"do"> + assign<&ast::InlineSeparateInstruction::body>(ref<Compound>) +
                 ikw<"end">)> {};

  struct IteratorRule
      : rule<create<ast::IteratorInstruction>(
                 lit<"⟳"> + assign<&ast::IteratorInstruction::name>(ref<NAME>) + lit<":"> +
                 assign<&ast::IteratorInstruction::source>(ref<Expression>) + lit<"¦"> +
                 assign<&ast::IteratorInstruction::body>(ref<Compound>) + lit<"⟲">)> {};

  struct InstructionRule
      : rule<ref<CreationInstructionRule>       //
             | ref<ConditionalRule>             //
             | ref<MultiBranchRule>              //
             | ref<LoopRule>                     //
             | ref<DebugRule>                    //
             | ref<PrecursorRule>                //
             | ref<ConditionalCheckRule>         //
             | ref<CheckRule>                    //
             | ref<RetryRule>                    //
             | ref<InlineSeparateRule>           //
             | ref<IteratorRule>                 //
             | ref<AssignmentRule>               //
             | create<ast::ExprInstruction>(assign<&ast::ExprInstruction::value>(ref<Expression>))> {
  };

  // ---- Notes ----

  struct NoteItemRule
      : rule<ref<NAME> | ref<BASIC_STRING> | ref<VERBATIM_STRING> | ref<CHAR_CONST> |
             ref<INTEGER> | ref<REAL> | ikw<"true"> | ikw<"false">> {};

  struct NoteEntryRule
      : rule<create<ast::NoteEntry>(
                 assign<&ast::NoteEntry::name>(ref<NAME>) + lit<":"> +
                 append<&ast::NoteEntry::values>(ref<NoteItemRule>) +
                 many(lit<","> + append<&ast::NoteEntry::values>(ref<NoteItemRule>)) +
                 option(lit<",">))> {};

  struct NotesRule
      : rule<create<ast::Notes> + ikw<"note"> +
             many(append<&ast::Notes::entries>(ref<NoteEntryRule>) + many(lit<";">))> {};

  // ---- Feature declarations ----

  struct AliasRule
      : rule<create<ast::Alias>(
                 ikw<"alias"> + assign<&ast::Alias::text>(ref<ManifestStringRule>) +
                 option(flag<&ast::Alias::convert>(ikw<"convert">)))> {};

  struct NewFeatureNameRule
      : rule<create<ast::NewFeatureName>(
                 option(flag<&ast::NewFeatureName::frozen>(ikw<"frozen">)) +
                 assign<&ast::NewFeatureName::name>(ref<NAME>) +
                 many(append<&ast::NewFeatureName::aliases>(ref<AliasRule>)))> {};

  struct RoutineBodyRule
      : rule<create<ast::RoutineBody>(
                 option(ikw<"require"> + option(flag<&ast::RoutineBody::requireElse>(ikw<"else">)) +
                        assign<&ast::RoutineBody::precondition>(ref<AssertionRule>)) +
                 option(ikw<"local"> +
                        option(append<&ast::RoutineBody::locals>(ref<EntityDeclGroupRule>) +
                               many(many(lit<";">) +
                                    append<&ast::RoutineBody::locals>(ref<EntityDeclGroupRule>)))) +
                 ((assign<&ast::RoutineBody::kind>(ikw<"deferred">))  //
                  | (assign<&ast::RoutineBody::kind>(ikw<"do">) +
                     assign<&ast::RoutineBody::body>(ref<Compound>))  //
                  | (assign<&ast::RoutineBody::kind>(ikw<"once">) +
                     option(lit<"("> +
                            append<&ast::RoutineBody::onceKeys>(ref<ManifestStringRule>) +
                            many(lit<","> +
                                 append<&ast::RoutineBody::onceKeys>(ref<ManifestStringRule>)) +
                            lit<")">) +
                     assign<&ast::RoutineBody::body>(ref<Compound>))  //
                  | (assign<&ast::RoutineBody::kind>(ikw<"external">) +
                     assign<&ast::RoutineBody::externalLanguage>(ref<ManifestStringRule>) +
                     option(ikw<"alias"> + assign<&ast::RoutineBody::externalAliasName>(
                                               ref<ManifestStringRule>)))  //
                  | (assign<&ast::RoutineBody::kind>(ikw<"attribute">) +
                     assign<&ast::RoutineBody::body>(ref<Compound>))) +
                 option(ikw<"ensure"> + option(flag<&ast::RoutineBody::ensureThen>(ikw<"then">)) +
                        assign<&ast::RoutineBody::postcondition>(ref<AssertionRule>) +
                        option(ikw<"only"> +
                               append<&ast::RoutineBody::postOnly>(ref<NAME>) +
                               many(lit<","> + append<&ast::RoutineBody::postOnly>(ref<NAME>)))) +
                 option(ikw<"rescue"> + assign<&ast::RoutineBody::rescueBody>(ref<Compound>)) +
                 ikw<"end">)> {};

  struct FeatureValueRule
      : rule<create<ast::FeatureValue>(
                 option(lit<"="> +
                        assign<&ast::FeatureValue::explicitValue>(ref<ManifestValueRule>)) +
                 option(ikw<"obsolete"> +
                        assign<&ast::FeatureValue::obsoleteMessage>(ref<ManifestStringRule>)) +
                 option(assign<&ast::FeatureValue::body>(ref<RoutineBodyRule>)) +
                 option(lit<"="> + flag<&ast::FeatureValue::unique>(ikw<"unique">)))> {};

  struct FeatureDeclarationRule
      : rule<create<ast::FeatureDeclaration>(
                 append<&ast::FeatureDeclaration::names>(ref<NewFeatureNameRule>) +
                 many(lit<","> + append<&ast::FeatureDeclaration::names>(ref<NewFeatureNameRule>)) +
                 option(lit<"("> +
                        option(append<&ast::FeatureDeclaration::args>(ref<EntityDeclGroupRule>) +
                               many(many(lit<";">) + append<&ast::FeatureDeclaration::args>(
                                                          ref<EntityDeclGroupRule>))) +
                        lit<")">) +
                 option(lit<":"> + assign<&ast::FeatureDeclaration::type>(ref<Type>) +
                        option(ikw<"assign"> +
                               assign<&ast::FeatureDeclaration::assignerName>(ref<NAME>))) +
                 option(assign<&ast::FeatureDeclaration::notes>(ref<NotesRule>)) +
                 assign<&ast::FeatureDeclaration::value>(ref<FeatureValueRule>))> {};

  struct FeatureClauseRule
      : rule<create<ast::FeatureClause>(
                 ikw<"feature"> +
                 option(lit<"{"> +
                        option(append<&ast::FeatureClause::clients>(ref<NAME>) +
                               many(lit<","> + append<&ast::FeatureClause::clients>(ref<NAME>))) +
                        lit<"}">) +
                 many(lit<";">) +
                 many(append<&ast::FeatureClause::features>(ref<FeatureDeclarationRule>) +
                      many(lit<";">)))> {};

  // ---- Inheritance ----

  struct RenamePairRule
      : rule<create<ast::RenamePair>(
                 assign<&ast::RenamePair::from>(ref<NAME>) + ikw<"as"> +
                 assign<&ast::RenamePair::to>(ref<NAME>) +
                 many(append<&ast::RenamePair::toAliases>(ref<AliasRule>)))> {};

  struct ExportItemRule
      : rule<create<ast::ExportItem>(
                 lit<"{"> +
                 option(append<&ast::ExportItem::clients>(ref<NAME>) +
                        many(lit<","> + append<&ast::ExportItem::clients>(ref<NAME>))) +
                 lit<"}"> +
                 (flag<&ast::ExportItem::all>(ikw<"all">)  //
                  | option(append<&ast::ExportItem::features>(ref<NAME>) +
                           many(lit<","> +
                                append<&ast::ExportItem::features>(ref<NAME>)))))> {};

  struct ParentRule
      : rule<create<ast::Parent>(
                 assign<&ast::Parent::type>(ref<Type>) +
                 option(many(
                            (ikw<"undefine"> +
                             append<&ast::Parent::undefineNames>(ref<NAME>) +
                             many(lit<","> + append<&ast::Parent::undefineNames>(ref<NAME>)))  //
                            | (ikw<"redefine"> +
                               append<&ast::Parent::redefineNames>(ref<NAME>) +
                               many(lit<","> +
                                    append<&ast::Parent::redefineNames>(ref<NAME>)))  //
                            | (ikw<"rename"> +
                               append<&ast::Parent::renames>(ref<RenamePairRule>) +
                               many(lit<","> +
                                    append<&ast::Parent::renames>(ref<RenamePairRule>)))  //
                            | (ikw<"export"> +
                               append<&ast::Parent::exports>(ref<ExportItemRule>) +
                               many(lit<";"> + append<&ast::Parent::exports>(ref<ExportItemRule>)) +
                               many(lit<";">))  //
                            | (ikw<"select"> +
                               append<&ast::Parent::selectNames>(ref<NAME>) +
                               many(lit<","> +
                                    append<&ast::Parent::selectNames>(ref<NAME>)))) +
                        ikw<"end">))> {};

  struct InheritClauseRule
      : rule<create<ast::InheritClause>(
                 ikw<"inherit"> +
                 option(lit<"{"> + assign<&ast::InheritClause::nonConformanceClass>(ref<NAME>) +
                        lit<"}">) +
                 append<&ast::InheritClause::parents>(ref<ParentRule>) +
                 many(many(lit<";">) + append<&ast::InheritClause::parents>(ref<ParentRule>)) +
                 many(lit<";">))> {};

  struct CreationClauseRule
      : rule<create<ast::CreationClause>(
                 ikw<"create"> +
                 option(lit<"{"> +
                        option(append<&ast::CreationClause::clients>(ref<NAME>) +
                               many(lit<","> +
                                    append<&ast::CreationClause::clients>(ref<NAME>))) +
                        lit<"}">) +
                 option(append<&ast::CreationClause::procedures>(ref<NAME>) +
                        many(lit<","> + append<&ast::CreationClause::procedures>(ref<NAME>))))> {
  };

  struct ConverterItemRule
      : rule<create<ast::Converter>(
                 assign<&ast::Converter::name>(ref<NAME>) +
                 ((lit<"("> + lit<"{"> +
                   option(append<&ast::Converter::types>(ref<Type>) +
                          many(lit<","> + append<&ast::Converter::types>(ref<Type>))) +
                   lit<"}"> + lit<")">)  //
                  | (flag<&ast::Converter::isQuery>(lit<":">) + lit<"{"> +
                     option(append<&ast::Converter::types>(ref<Type>) +
                            many(lit<","> + append<&ast::Converter::types>(ref<Type>))) +
                     lit<"}">)))> {};

  struct FormalGenericRule
      : rule<create<ast::FormalGeneric>(
                 many(append<&ast::FormalGeneric::modifiers>(ikw<"frozen"> | ikw<"reference">)) +
                 option(flag<&ast::FormalGeneric::question>(lit<"?">)) +
                 assign<&ast::FormalGeneric::name>(ref<NAME>) +
                 option(lit<"->"> +
                        ((append<&ast::FormalGeneric::constraints>(ref<Type>) +
                          option(append<&ast::FormalGeneric::constraintRenames>(ref<RenamePairRule>) +
                                 many(lit<","> +
                                      append<&ast::FormalGeneric::constraintRenames>(
                                          ref<RenamePairRule>)) +
                                 ikw<"end">))  //
                         | (lit<"{"> +
                            append<&ast::FormalGeneric::constraints>(ref<Type>) +
                            many(lit<","> + append<&ast::FormalGeneric::constraints>(ref<Type>)) +
                            lit<"}">)) +
                        option(ikw<"create"> +
                               append<&ast::FormalGeneric::creators>(ref<NAME>) +
                               many(lit<","> + append<&ast::FormalGeneric::creators>(ref<NAME>)) +
                               ikw<"end">)))> {};

  static constexpr auto PlainHeaderMark =
      ikw<"deferred"> | ikw<"expanded"> | ikw<"frozen"> | ikw<"external">;

  // ---- Class ----

  struct ClassDeclRule
      : rule<create<ast::ClassDecl> +
             option(assign<&ast::ClassDecl::notesTop>(ref<NotesRule>)) +
             many(append<&ast::ClassDecl::headerMarks>(PlainHeaderMark) |
                  (append<&ast::ClassDecl::headerMarks>(ikw<"once">) +
                   option(lit<"("> +
                          append<&ast::ClassDecl::onceKeys>(ref<ManifestStringRule>) +
                          many(lit<","> +
                               append<&ast::ClassDecl::onceKeys>(ref<ManifestStringRule>)) +
                          lit<")">))) +
             ikw<"class"> + assign<&ast::ClassDecl::name>(ref<NAME>) +
             option(lit<"["> +
                    append<&ast::ClassDecl::generics>(ref<FormalGenericRule>) +
                    many(lit<","> + append<&ast::ClassDecl::generics>(ref<FormalGenericRule>)) +
                    lit<"]">) +
             option(ikw<"obsolete"> +
                    assign<&ast::ClassDecl::obsoleteMessage>(ref<ManifestStringRule>)) +
             option(some(append<&ast::ClassDecl::inheritance>(ref<InheritClauseRule>))) +
             option(some(append<&ast::ClassDecl::creators>(ref<CreationClauseRule>))) +
             option(ikw<"convert"> +
                    append<&ast::ClassDecl::converters>(ref<ConverterItemRule>) +
                    many(lit<","> + append<&ast::ClassDecl::converters>(ref<ConverterItemRule>))) +
             option(some(append<&ast::ClassDecl::features>(ref<FeatureClauseRule>))) +
             option(assign<&ast::ClassDecl::notesMid>(ref<NotesRule>)) +
             option(ikw<"invariant"> + assign<&ast::ClassDecl::invariant>(ref<AssertionRule>) +
                    option(assign<&ast::ClassDecl::notesBottom>(ref<NotesRule>))) +
             ikw<"end">> {};

  using entry = ClassDeclRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace eiffel
