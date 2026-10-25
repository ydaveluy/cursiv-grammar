// The Z notation language (Unicode/`.zed` markup, ISO/IEC 13568). A document
// is prose text interspersed with Z paragraphs, each opened by one of three
// box-drawing characters and closed by a fourth.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "z/core/ast.hpp"

namespace z {

struct Z : cursiv::GrammarSpec {
  // A name's own alphabet: any Unicode letter or digit. The handful of
  // letters and words the standard toolkit binds as fixed operators are
  // spelled `kw<>`, so a name still wins where they are not expected.
  static constexpr auto word_boundary = uc::L | uc::Nd | cr<"_">;

  struct WS : terminal<some(uc::Zs)> {};
  struct NL_TRIVIA : terminal<some(cr<R"(\r\n)">)> {};
  // Prose between Z paragraphs: anything but the three openers.
  struct TEXT_TOK : terminal<some(cr<"^─┌╷">)> {};

  struct STROKE : terminal<cr<"!?′"> | (lit<"↘"> + uc::Nd + lit<"↖">)> {};
  struct NUMERAL : terminal<some(uc::Nd)> {};
  // `_` and the arrows glue a further run onto a name (`MAX_NAT`, a
  // subscript `x↘1↖`); a name's strokes are lexically part of it too
  // (`file′` is one token, so a schema hidden by name keeps its prime).
  // `expression STROKE` in a rule body decorates what a name's own
  // trailing run could not reach.
  static constexpr auto NameGlue = cr<"_↘↙↗↖"> + some(uc::L | uc::Nd);
  struct NAME
      : terminal<((uc::L + many(uc::L | uc::Nd) + many(NameGlue)) |
                  some(NameGlue)) +
                 many(cr<"!?′"> | (lit<"↘"> + uc::Nd + lit<"↖">))> {};

  // The standard toolkit (Annex B), fixed: a closed set of names the
  // language always binds as operators, independent of any document's own
  // operator-template paragraphs. Those declarations do not make new
  // operator names usable outside their defining paragraph.
  static constexpr auto PrefixOp =
      kw<"dom"> | kw<"ran"> | kw<"first"> | kw<"second"> | kw<"id"> |
      kw<"succ"> | kw<"iter"> | kw<"min"> | kw<"max"> | kw<"seq"> |
      kw<"iseq"> | kw<"rev"> | kw<"head"> | kw<"last"> | kw<"tail"> |
      kw<"front"> | kw<"squash"> | kw<"disjoint"> | kw<"pre"> | lit<"ℙ"> |
      lit<"⋃"> | lit<"⋂"> | lit<"𝔽"> | lit<"⁀/"> | lit<"#"> |
      lit<"-", !cr<"0-9">>;
  static constexpr auto PostfixOp = lit<"∼"> | lit<"↗+↙"> | lit<"↗*↙">;

  static constexpr auto ToolkitInner =
      left(lit<"×">) >>                                                //
      left(lit<"◁"> | lit<"⩤">) >> left(lit<"▷"> | lit<"⩥">) >>        //
      left(lit<"⊕">) >> right(lit<"↿">) >>                             //
      left(lit<"∩"> | lit<"⨾"> | lit<"∘"> | lit<"*"> | kw<"div"> |      //
           kw<"mod"> | lit<"↾">) >>                                    //
      left(lit<"+"> | lit<"∪"> | lit<"∖"> | lit<"−"> | lit<"⁀">) >>    //
      left(lit<"⊖">) >> left(lit<"..">) >> left(lit<"↦">) >>           //
      right(lit<"↔"> | lit<"→"> | lit<"⇸"> | lit<"⤔"> | lit<"↣"> |      //
            lit<"⤀"> | lit<"↠"> | lit<"⤖"> | lit<"⇻"> | lit<"⤕">) >>   //
      left(lit<"≠"> | lit<"∉"> | lit<"⊆"> | lit<"⊂"> | lit<"≤"> |       //
           lit<"<"> | lit<"≥"> | lit<">"> | kw<"partition"> |          //
           kw<"prefix"> | kw<"suffix"> | kw<"infix"> | lit<"∈"> |      //
           lit<"=">) >>                                                //
      left(lit<"⨟">) >> left(lit<"⨠">) >> left(lit<"⨡">);

  static constexpr auto ToolkitOuter =
      left(lit<"∧">) >> left(lit<"∨">) >> right(lit<"⇒">) >>
      left(lit<"⇔">) >> left(lit<";">);

  // -- expressions -------------------------------------------------------
  struct Expr;
  struct SchemaText;
  struct ThetaRule;

  struct DeclEqRule
      : rule<assign<&ast::DeclEq::name>(ref<NAME>) + lit<"=="> +
             assign<&ast::DeclEq::value>(ref<Expr>)> {};

  // Not a named rule: each alternative must keep its own exact class, so the
  // heterogeneous list below can tell which of the three it just built.
  static constexpr auto DeclItem =
      create<ast::DeclNames>(
          append<&ast::DeclNames::names>(ref<NAME>) +
          many(lit<","> + append<&ast::DeclNames::names>(ref<NAME>)) +
          lit<":"> + assign<&ast::DeclNames::type>(ref<Expr>))  //
      | ref<DeclEqRule> | ref<Expr>;

  struct SchemaText
      : rule<create<ast::SchemaText> +
             option(append<&ast::SchemaText::decls>(DeclItem) +
                    many(implied(lit<";">, gap::line) +
                         append<&ast::SchemaText::decls>(DeclItem))) +
             option(lit<"|"> +
                    option(append<&ast::SchemaText::where>(ref<Expr>) +
                           many(implied(lit<";">, gap::line) +
                                append<&ast::SchemaText::where>(ref<Expr>))))> {
  };

  struct GenericInstArgs
      : rule<assign<&ast::GenericInstantiation::name>(ref<NAME>) + lit<"["> +
             option(append<&ast::GenericInstantiation::args>(ref<Expr>) +
                    many(lit<","> + append<&ast::GenericInstantiation::args>(
                                        ref<Expr>)))> {};

  struct Atom
      : rule<create<ast::Number>(assign<&ast::Number::text>(ref<NUMERAL>))  //
             | create<ast::BoolLit>(flag<&ast::BoolLit::value>(
                                        kw<"true">) |
                                    kw<"false">)  //
             | create<ast::SetExtension>(
                   lit<"{"> +
                   option(append<&ast::SetExtension::items>(ref<Expr>) +
                          many(lit<","> + append<&ast::SetExtension::items>(
                                              ref<Expr>))) +
                   lit<"}">)  //
             | create<ast::SetComprehension>(
                   lit<"{"> +
                   assign<&ast::SetComprehension::text>(ref<SchemaText>) +
                   option(lit<"⦁"> +
                          assign<&ast::SetComprehension::body>(ref<Expr>)) +
                   lit<"}">)  //
             | create<ast::SchemaConstruction>(
                   lit<"["> + assign<&ast::SchemaConstruction::text>(
                                  ref<SchemaText>) +
                   lit<"]">)  //
             | create<ast::BindingExtension>(
                   lit<"⦉"> +
                   option(append<&ast::BindingExtension::items>(
                              ref<DeclEqRule>) +
                          many(lit<","> +
                               append<&ast::BindingExtension::items>(
                                   ref<DeclEqRule>))) +
                   lit<"⦊">)  //
             | create<ast::SeqExtension>(
                   lit<"⟨"> +
                   option(append<&ast::SeqExtension::items>(ref<Expr>) +
                          many(lit<","> + append<&ast::SeqExtension::items>(
                                              ref<Expr>))) +
                   lit<"⟩">)  //
             | create<ast::CharacteristicMu>(
                   lit<"("> + lit<"μ"> +
                   assign<&ast::CharacteristicMu::text>(ref<SchemaText>) +
                   lit<")">)  //
             | create<ast::TupleExtension>(
                   lit<"("> + append<&ast::TupleExtension::items>(ref<Expr>) +
                   some(lit<","> +
                        append<&ast::TupleExtension::items>(ref<Expr>)) +
                   lit<")">)                                    //
             | (lit<"("> + ref<Expr> + lit<")">)                //
             | ref<ThetaRule>                                   //
             | (ref<GenericInstArgs> + lit<"]">)                //
             // A toolkit prefix word (`dom`, `last`, ...) is `kw<>`, not
             // `NAME`: offered here too, so the reading that is not
             // applied stays available -- one spelling, two actions of a
             // state on one token, never a second terminal for it.
             | create<ast::NameRef>(
                   assign<&ast::NameRef::name>(ref<NAME> | lit<"∅"> |
                                               PrefixOp))> {};

  struct RenameRule
      : rule<assign<&ast::Rename::newName>(ref<NAME>) + lit<"/"> +
             assign<&ast::Rename::oldName>(ref<NAME>)> {};

  // Decoration (strokes), binding selection, tuple selection, renaming:
  // postfix, and tightest after application.
  struct Decorated
      : rule<ref<Atom> +
             many(nest<ast::Decoration, &ast::Decoration::operand>(
                      append<&ast::Decoration::strokes>(ref<STROKE>))  //
                  | nest<ast::Selection, &ast::Selection::operand>(
                        lit<"."> +
                        assign<&ast::Selection::field>(ref<NAME>))  //
                  | nest<ast::TupleSelection, &ast::TupleSelection::operand>(
                        lit<"."> + assign<&ast::TupleSelection::index>(
                                       ref<NUMERAL>))  //
                  | nest<ast::Renaming, &ast::Renaming::operand>(
                        lit<"["> +
                        append<&ast::Renaming::renames>(ref<RenameRule>) +
                        many(lit<","> + append<&ast::Renaming::renames>(
                                             ref<RenameRule>)) +
                        lit<"]">))> {};

  // Juxtaposition: function application, left-associative. A line break
  // before the argument closes the chain instead of extending it -- without
  // this, a bare schema reference on its own line reads as applied to the
  // declaration that follows it, and the implied `;` between them never
  // gets a chance: `implied()` only fires where the construct in hand
  // truly cannot take the token, and application always could.
  struct Applied
      : rule<ref<Decorated> +
             many(!gap::line + nest<ast::Application, &ast::Application::func>(
                                    assign<&ast::Application::arg>(
                                        ref<Decorated>)))> {};

  // Postfix toolkit operators and relational image.
  struct PostfixApplied
      : rule<ref<Applied> +
             many(nest<ast::Postfix, &ast::Postfix::operand>(
                      assign<&ast::Postfix::op>(PostfixOp))  //
                  | nest<ast::Image, &ast::Image::fn>(
                        lit<"⦇"> + assign<&ast::Image::arg>(ref<Expr>) +
                        lit<"⦈">))> {};

  // Prefix: every toolkit prefix name and `pre`, one tier (self-recursive,
  // so `dom ran f` chains).
  struct PrefixApplied
      : rule<ref<PostfixApplied>  //
             | create<ast::Prefix>(
                   assign<&ast::Prefix::op>(PrefixOp) +
                   assign<&ast::Prefix::operand>(ref<PrefixApplied>))> {};

  // Schema hiding: postfix bracket, repeatable. Spelled `⧹`, the character
  // Z documents actually carry here.
  struct Hidden
      : rule<ref<PrefixApplied> +
             many(nest<ast::Hide, &ast::Hide::operand>(
                 lit<"⧹"> + lit<"("> + append<&ast::Hide::names>(ref<NAME>) +
                 many(lit<","> + append<&ast::Hide::names>(ref<NAME>)) +
                 lit<")">))> {};

  static constexpr auto KnownInfix =
      lit<"×"> | lit<"◁"> | lit<"⩤"> | lit<"▷"> | lit<"⩥"> | lit<"⊕"> | lit<"↿"> |
      lit<"∩"> | lit<"⨾"> | lit<"∘"> | lit<"*"> | kw<"div"> | kw<"mod"> | lit<"↾"> |
      lit<"+"> | lit<"∪"> | lit<"∖"> | lit<"−"> | lit<"⁀"> | lit<"⊖"> | lit<".."> |
      lit<"↦"> | lit<"↔"> | lit<"→"> | lit<"⇸"> | lit<"⤔"> | lit<"↣"> | lit<"⤀"> |
      lit<"↠"> | lit<"⤖"> | lit<"⇻"> | lit<"⤕"> | lit<"≠"> | lit<"∉"> | lit<"⊆"> |
      lit<"⊂"> | lit<"≤"> | lit<"<"> | lit<"≥"> | lit<">"> | kw<"partition"> |
      kw<"prefix"> | kw<"suffix"> | kw<"infix"> | lit<"∈"> | lit<"="> | lit<"⨟"> |
      lit<"⨠"> | lit<"⨡"> | lit<"∧"> | lit<"∨"> | lit<"⇒"> | lit<"⇔"> | lit<";">;
  struct CustomOperator : terminal<!KnownInfix + !PrefixOp + !PostfixOp +
                                   some(!cr<"─-╿|¬⦁⦉⦊⦇⦈⟨⟩⦅⦆⟪⟫"> + (uc::Sm | uc::So))> {
  };
  struct KnownStep : rule<assign<&ast::OperatorStep::op>(KnownInfix) +
                          assign<&ast::OperatorStep::operand>(ref<Hidden>)> {};
  struct CustomStep : rule<assign<&ast::OperatorStep::op>(ref<CustomOperator>) +
                           assign<&ast::OperatorStep::operand>(ref<Hidden>)> {};
  struct AnyStep : rule<ref<KnownStep> | ref<CustomStep>> {};
  struct CustomSequence
      : rule<create<ast::OperatorSequence>(
            assign<&ast::OperatorSequence::first>(ref<Hidden>) +
            many(append<&ast::OperatorSequence::steps>(ref<KnownStep>)) +
            append<&ast::OperatorSequence::steps>(ref<CustomStep>) +
            many(append<&ast::OperatorSequence::steps>(ref<AnyStep>)))> {};

  struct InnerOps
      : infix<Hidden, &ast::Infix::left, &ast::Infix::op, &ast::Infix::right,
              ToolkitInner> {};

  // Negation: looser than every relation and toolkit operator (it negates a
  // whole fact, `¬ x ∈ A` is `¬(x ∈ A)`), tighter than the schema calculus.
  struct Negated
      : rule<create<ast::Prefix>(assign<&ast::Prefix::op>(lit<"¬">) +
                                 assign<&ast::Prefix::operand>(
                                     ref<Negated>))  //
             | ref<InnerOps>> {};

  struct OuterOps
      : infix<Negated, &ast::Infix::left, &ast::Infix::op, &ast::Infix::right,
              ToolkitOuter> {};

  struct IfThenElseRule
      : rule<create<ast::IfThenElse>(
            kw<"if"> + assign<&ast::IfThenElse::cond>(ref<Expr>) +
            kw<"then"> + assign<&ast::IfThenElse::thenValue>(ref<Expr>) +
            kw<"else"> + assign<&ast::IfThenElse::elseValue>(ref<Expr>))> {};

  struct LetRule
      : rule<create<ast::Let>(
            kw<"let"> + append<&ast::Let::bindings>(ref<DeclEqRule>) +
            many(lit<";"> + append<&ast::Let::bindings>(ref<DeclEqRule>)) +
            lit<"⦁"> + assign<&ast::Let::body>(ref<Expr>))> {};

  struct ThetaRule
      : rule<create<ast::BindingConstructionTheta>(
            lit<"θ"> + assign<&ast::BindingConstructionTheta::operand>(
                           ref<Decorated>) +
            many(append<&ast::BindingConstructionTheta::strokes>(
                ref<STROKE>)))> {};

  struct QuantifierRule
      : rule<create<ast::Quantifier>(
                 assign<&ast::Quantifier::kind>(lit<"∀">) +
                 assign<&ast::Quantifier::text>(ref<SchemaText>) + lit<"⦁"> +
                 assign<&ast::Quantifier::body>(ref<Expr>))  //
             | create<ast::Quantifier>(
                   assign<&ast::Quantifier::kind>(lit<"∃">) +
                   option(flag<&ast::Quantifier::uniqueExists>(
                       lit<"↘1↖">)) +
                   assign<&ast::Quantifier::text>(ref<SchemaText>) +
                   lit<"⦁"> + assign<&ast::Quantifier::body>(ref<Expr>))  //
             | create<ast::Quantifier>(
                   assign<&ast::Quantifier::kind>(lit<"λ">) +
                   assign<&ast::Quantifier::text>(ref<SchemaText>) +
                   lit<"⦁"> + assign<&ast::Quantifier::body>(ref<Expr>))  //
             | create<ast::Quantifier>(
                   assign<&ast::Quantifier::kind>(lit<"μ">) +
                   assign<&ast::Quantifier::text>(ref<SchemaText>) +
                   lit<"⦁"> + assign<&ast::Quantifier::body>(ref<Expr>))> {};

  struct Expr : rule<ref<CustomSequence> | ref<QuantifierRule>  //
                     | ref<LetRule>                             //
                     | ref<IfThenElseRule>                      //
                     | ref<OuterOps>> {};

  // -- paragraphs & sections ---------------------------------------------

  struct OperatorTemplateBody : terminal<some(cr<"^)">)> {};

  struct OperatorTemplateRule
      : rule<create<ast::OperatorTemplate>(
            assign<&ast::OperatorTemplate::category>(
                kw<"relation"> | kw<"function"> | kw<"generic">) +
            option(flag<&ast::OperatorTemplate::hasPrec>(ref<NUMERAL>) +
                   (flag<&ast::OperatorTemplate::rightAssoc>(
                        kw<"rightassoc">) |
                    kw<"leftassoc">)) +
            lit<"("> +
            option(assign<&ast::OperatorTemplate::text>(ref<OperatorTemplateBody>)) +
            lit<")">)> {};

  struct FreeTypeBranchRule
      : rule<assign<&ast::FreeTypeBranch::name>(ref<NAME>) +
             option(lit<"⟪"> +
                    assign<&ast::FreeTypeBranch::payload>(ref<Expr>) +
                    lit<"⟫">)> {};

  struct FreeTypeRule
      : rule<assign<&ast::FreeType::name>(ref<NAME>) + lit<"::="> +
             append<&ast::FreeType::branches>(ref<FreeTypeBranchRule>) +
             many(lit<"|"> +
                  append<&ast::FreeType::branches>(ref<FreeTypeBranchRule>))> {
  };

  struct Paragraph
      : rule<create<ast::GivenTypes>(
                 lit<"─"> + lit<"["> +
                 option(append<&ast::GivenTypes::names>(ref<NAME>) +
                        many(lit<","> + append<&ast::GivenTypes::names>(
                                            ref<NAME>))) +
                 lit<"]">)  //
             | create<ast::SchemaDef>(
                   flag<&ast::SchemaDef::isAxiomatic>(lit<"╷">) +
                   option(lit<"═"> + lit<"["> +
                          option(append<&ast::SchemaDef::formals>(ref<NAME>) +
                                 many(lit<","> +
                                      append<&ast::SchemaDef::formals>(
                                          ref<NAME>))) +
                          lit<"]">) +
                   assign<&ast::SchemaDef::text>(ref<SchemaText>))  //
             | create<ast::SchemaDef>(
                   lit<"┌"> +
                   ((lit<"═"> + assign<&ast::SchemaDef::name>(ref<NAME>) +
                     lit<"["> +
                     option(append<&ast::SchemaDef::formals>(ref<NAME>) +
                            many(lit<","> + append<&ast::SchemaDef::formals>(
                                                ref<NAME>))) +
                     lit<"]">)  //
                    | assign<&ast::SchemaDef::name>(ref<NAME>)) +
                   assign<&ast::SchemaDef::text>(ref<SchemaText>))  //
             | (lit<"─"> + ref<OperatorTemplateRule>)               //
             | create<ast::HorizontalDef>(
                   lit<"─"> + assign<&ast::HorizontalDef::name>(ref<NAME>) +
                   option(lit<"["> +
                          option(append<&ast::HorizontalDef::formals>(
                                     ref<NAME>) +
                                 many(lit<","> +
                                      append<&ast::HorizontalDef::formals>(
                                          ref<NAME>))) +
                          lit<"]">) +
                   lit<"=="> +
                   assign<&ast::HorizontalDef::value>(ref<Expr>))  //
             | create<ast::FreeTypes>(
                   lit<"─"> +
                   append<&ast::FreeTypes::types>(ref<FreeTypeRule>) +
                   many(lit<"&"> +
                        append<&ast::FreeTypes::types>(ref<FreeTypeRule>)))  //
             | create<ast::Conjecture>(
                   lit<"─"> +
                   option(lit<"["> +
                          option(append<&ast::Conjecture::formals>(
                                     ref<NAME>) +
                                 many(lit<","> +
                                      append<&ast::Conjecture::formals>(
                                          ref<NAME>))) +
                          lit<"]">) +
                   lit<"⊢?"> + assign<&ast::Conjecture::value>(ref<Expr>))> {
  };

  struct ParagraphItem : rule<ref<Paragraph> + lit<"└">> {};

  static constexpr auto SectionHeader =
      lit<"─"> + kw<"section"> + assign<&ast::Section::name>(ref<NAME>) +
      option(kw<"parents"> + append<&ast::Section::parents>(ref<NAME>) +
             many(lit<","> + append<&ast::Section::parents>(ref<NAME>))) +
      lit<"└">;

  // Prose commonly separates a section's paragraphs; without this, a
  // paragraph after a comment would read as a fresh top-level item instead
  // of the section's own.
  struct Section
      : rule<create<ast::Section>(
            SectionHeader +
            many(ref<TEXT_TOK> |
                 append<&ast::Section::body>(ref<ParagraphItem>)))> {};

  // A section keeps all that follows it up to the next section, so only a
  // section can follow one: its end is never a second reading of its text.
  struct Specification
      : rule<create<ast::Specification> +
             many(ref<TEXT_TOK> |
                  append<&ast::Specification::items>(ref<ParagraphItem>)) +
             many(append<&ast::Specification::items>(ref<Section>))> {};

  using entry = Specification;
  using ignore = list<WS, NL_TRIVIA>;
};

}  // namespace z
