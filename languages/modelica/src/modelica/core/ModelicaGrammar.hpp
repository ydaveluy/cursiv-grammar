// The modelica language: its grammar. A Pascal-shaped declaration syntax
// (class/model/record/connector/package/function) around an equation and
// algorithm section language, with a rich name-based modification system.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "modelica/core/ast.hpp"

namespace modelica {

struct Modelica : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto QChar =
      cr<"0-9A-Za-z_!#$%&()*+,./:;<>=?@[]{}|~^-">;
  static constexpr auto SEscape =
      lit<"\\"> + (cr<R"(abfnrtv\\"'?)"> | lit<"’">);
  struct IDENT
      : terminal<(cr<"a-zA-Z_"> + many(w)) |
                 (lit<"'"> + some(QChar | SEscape) + lit<"'">)> {};

  static constexpr auto unescape = [](std::string_view t,
                                      std::pmr::memory_resource* mem) {
    const std::string_view body = t.substr(1, t.size() - 2);
    if (body.find('\\') == std::string_view::npos) return body;
    const std::span<char> room = cursiv::text_room(mem, body.size());
    std::size_t n = 0;
    for (std::size_t i = 0; i < body.size(); ++i) {
      char c = body[i];
      if (c == '\\' && i + 1 < body.size()) {
        char e = body[++i];
        switch (e) {
          case 'a': c = '\a'; break;
          case 'b': c = '\b'; break;
          case 'f': c = '\f'; break;
          case 'n': c = '\n'; break;
          case 'r': c = '\r'; break;
          case 't': c = '\t'; break;
          case 'v': c = '\v'; break;
          default: c = e; break;
        }
      }
      room[n++] = c;
    }
    return std::string_view(room.data(), n);
  };
  using Unescaped = cursiv::text_conv<unescape>;
  struct STRING
      : terminal<lit<"\""> + many(SEscape | cr<R"(^"\\)">) + lit<"\"">,
                 Unescaped> {};

  static constexpr auto UInt = some(d);
  struct UNSIGNED_NUMBER
      : terminal<UInt + option(lit<"."> + option(UInt)) +
                 option(cr<"eE"> + option(cr<"+-">) + UInt)> {};

  static constexpr auto MulOp = lit<"*"> | lit<"/"> | lit<".*"> | lit<"./">;
  static constexpr auto AddOp = lit<"+"> | lit<"-"> | lit<".+"> | lit<".-">;
  static constexpr auto RelOp =
      lit<"<="> | lit<"<>"> | lit<"<"> | lit<">="> | lit<">"> | lit<"==">;
  static constexpr auto TypePrefixFlow = kw<"flow"> | kw<"stream">;
  static constexpr auto TypePrefixVar =
      kw<"discrete"> | kw<"parameter"> | kw<"constant">;
  static constexpr auto TypePrefixCaus = kw<"input"> | kw<"output">;

  // Class prefixes always land in an ast::ClassDef, whether reached through
  // a full class_definition or a short_class_definition -- one splice, two
  // call sites.
  static constexpr auto ClassPrefixes =
      option(flag<&ast::ClassDef::partial_>(kw<"partial">)) +
      (assign<&ast::ClassDef::kind>(kw<"class">) |
       assign<&ast::ClassDef::kind>(kw<"model">) |
       (option(flag<&ast::ClassDef::isOperator>(kw<"operator">)) +
        assign<&ast::ClassDef::kind>(kw<"record">)) |
       assign<&ast::ClassDef::kind>(kw<"block">) |
       (option(flag<&ast::ClassDef::isExpandable>(kw<"expandable">)) +
        assign<&ast::ClassDef::kind>(kw<"connector">)) |
       assign<&ast::ClassDef::kind>(kw<"type">) |
       assign<&ast::ClassDef::kind>(kw<"package">) |
       (option(assign<&ast::ClassDef::purity>(kw<"pure"> | kw<"impure">)) +
        option(flag<&ast::ClassDef::isOperator>(kw<"operator">)) +
        assign<&ast::ClassDef::kind>(kw<"function">)) |
       assign<&ast::ClassDef::kind>(kw<"operator">));

  struct SubscriptRule;
  struct PrimaryRule;
  struct FactorRule;
  struct TermRule;
  struct SignedTermRule;
  struct ArithExprRule;
  struct RelationRule;
  struct LogicalFactorRule;
  struct LogicalExprRule;
  struct SimpleExpressionRule;
  struct ExpressionRule;
  struct ExprRowRule;
  struct NameRule;
  struct DerOrInitialNameRule;
  struct CallCalleeRule;
  struct ComponentReferenceRule;
  struct RefPartRule;
  struct FunctionArgumentsRule;
  struct FunctionArgumentRule;
  struct NamedArgumentRule;
  struct ForIndexRule;
  struct AnnotationRule;
  struct ClassModRule;
  struct ArgumentRule;
  struct ElementModWithFlagsRule;
  struct ElementReplaceableWithFlagsRule;
  struct ElementReplaceableRule;
  struct ElementRedeclareRule;
  struct ModificationRule;
  struct ComponentDeclRule;
  struct ComponentDecl1Rule;
  struct ComponentClauseRule;
  struct ComponentClause1Rule;
  struct ShortClassDefinitionRule;
  struct ClassDefinitionRule;
  struct ClassSpecifierRule;
  struct LongClassSpecRule;
  struct ShortClassSpecRule;
  struct EnumLiteralRule;
  struct DerClassSpecRule;
  struct ImportClauseRule;
  struct ExtendsClauseRule;
  struct ElementRule;
  struct ElementDeclRule;
  struct CompositionRule;
  struct DefaultSectionRule;
  struct PublicSectionRule;
  struct ProtectedSectionRule;
  struct EquationSectionRule;
  struct AlgorithmSectionRule;
  struct ExternalCallRule;
  struct EquationRule;
  struct IfEquationRule;
  struct ForEquationRule;
  struct ConnectEquationRule;
  struct WhenEquationRule;
  struct EqBlockRule;
  struct StatementRule;
  struct StmtBlockRule;
  struct IfStmtRule;
  struct ForStmtRule;
  struct WhileStmtRule;
  struct WhenStmtRule;
  struct WithinClauseRule;
  struct TopClassDefRule;
  struct StoredDefinitionRule;

  // -- names and references ----

  struct NameRule
      : rule<option(flag<&ast::Name::global>(lit<".">)) +
             append<&ast::Name::parts>(ref<IDENT>) +
             many(lit<"."> + append<&ast::Name::parts>(ref<IDENT>))> {};

  struct DerOrInitialNameRule
      : rule<create<ast::Name> +
             append<&ast::Name::parts>(kw<"der"> | kw<"initial">)> {};

  struct CallCalleeRule
      : rule<ref<NameRule> | ref<DerOrInitialNameRule>> {};

  struct RefPartRule
      : rule<assign<&ast::RefPart::name>(ref<IDENT>) +
             option(lit<"["> +
                    some(append<&ast::RefPart::subscripts>(ref<SubscriptRule>),
                         lit<",">) +
                    lit<"]">)> {};

  struct ComponentReferenceRule
      : rule<option(flag<&ast::ComponentRef::global>(lit<".">)) +
             append<&ast::ComponentRef::parts>(ref<RefPartRule>) +
             many(lit<"."> +
                  append<&ast::ComponentRef::parts>(ref<RefPartRule>))> {};

  // -- expressions ----

  struct SubscriptRule
      : rule<(create<ast::ColonExpr> + lit<":">) | ref<ExpressionRule>> {};

  struct ExprRowRule
      : rule<some(append<&ast::ExprRow::items>(ref<ExpressionRule>),
                  lit<",">)> {};

  struct NamedArgumentRule
      : rule<assign<&ast::NamedArg::name>(ref<IDENT>) + lit<"="> +
             assign<&ast::NamedArg::value>(ref<FunctionArgumentRule>)> {};

  struct FunctionArgumentRule
      : rule<(kw<"function"> + assign<&ast::FunctionRefArg::func>(ref<NameRule>) +
              lit<"("> +
              option(some(append<&ast::FunctionRefArg::named>(
                              ref<NamedArgumentRule>),
                          lit<",">)) +
              lit<")">)  //
             | ref<ExpressionRule>> {};

  struct ForIndexRule
      : rule<assign<&ast::ForIndex::name>(ref<IDENT>) +
             option(kw<"in"> + assign<&ast::ForIndex::range>(ref<ExpressionRule>))> {
  };

  // After the positional run, Modelica's function-call argument list may
  // either keep adding positional arguments, switch permanently to named
  // ones past a comma, or close with a `for` comprehension -- never resume
  // positional once named starts.
  struct FunctionArgumentsRule
      : rule<(some(append<&ast::CallArgs::positional>(ref<FunctionArgumentRule>),
                   lit<",">) +
              ((lit<","> +
                some(append<&ast::CallArgs::named>(ref<NamedArgumentRule>),
                     lit<",">))  //
               | option(kw<"for"> +
                        some(append<&ast::CallArgs::forIndices>(
                                 ref<ForIndexRule>),
                             lit<",">))))  //
             | some(append<&ast::CallArgs::named>(ref<NamedArgumentRule>),
                    lit<",">)> {};

  struct PrimaryRule
      : rule<create<ast::NumberLit>(
                 assign<&ast::NumberLit::value>(ref<UNSIGNED_NUMBER>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))  //
             | (create<ast::BoolLit> +
                (flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">))  //
             | create<ast::CallExpr>(
                   assign<&ast::CallExpr::callee>(ref<CallCalleeRule>) +
                   lit<"("> +
                   option(assign<&ast::CallExpr::args>(
                       ref<FunctionArgumentsRule>)) +
                   lit<")">)                //
             | ref<ComponentReferenceRule>  //
             | (lit<"("> +
                append<&ast::TupleExpr::items>(ref<ExpressionRule> |
                                               create<ast::OmittedOutput>) +
                many(lit<","> +
                     append<&ast::TupleExpr::items>(
                         ref<ExpressionRule> | create<ast::OmittedOutput>)) +
                lit<")">)  //
             |
             (lit<"["> + append<&ast::MatrixLit::rows>(ref<ExprRowRule>) +
              many(lit<";"> + append<&ast::MatrixLit::rows>(ref<ExprRowRule>)) +
              lit<"]">)  //
             | create<ast::ArrayLit>(
                   lit<"{"> +
                   assign<&ast::ArrayLit::args>(ref<FunctionArgumentsRule>) +
                   lit<"}">)  //
             | (create<ast::EndExpr> + kw<"end">)> {};

  struct FactorRule
      : rule<(assign<&ast::BinaryExpr::left>(ref<PrimaryRule>) +
              assign<&ast::BinaryExpr::op>(lit<"^"> | lit<".^">) +
              assign<&ast::BinaryExpr::right>(ref<PrimaryRule>))  //
             | ref<PrimaryRule>> {};

  struct TermRule
      : infix<FactorRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(MulOp))> {};

  // A leading sign belongs only ahead of the whole arithmetic expression in
  // Modelica; folding it onto every term of the `+`/`-` chain over-accepts
  // `a + -b`, a harmless widening.
  struct SignedTermRule
      : rule<(assign<&ast::UnaryExpr::op>(AddOp) +
              assign<&ast::UnaryExpr::operand>(ref<TermRule>))  //
             | ref<TermRule>> {};

  struct ArithExprRule
      : infix<SignedTermRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(AddOp))> {};

  struct RelationRule
      : rule<(assign<&ast::BinaryExpr::left>(ref<ArithExprRule>) +
              assign<&ast::BinaryExpr::op>(RelOp) +
              assign<&ast::BinaryExpr::right>(ref<ArithExprRule>))  //
             | ref<ArithExprRule>> {};

  struct LogicalFactorRule
      : rule<(assign<&ast::UnaryExpr::op>(kw<"not">) +
              assign<&ast::UnaryExpr::operand>(ref<RelationRule>))  //
             | ref<RelationRule>> {};

  struct LogicalExprRule
      : infix<LogicalFactorRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(kw<"and">) >> left(kw<"or">))> {};

  struct SimpleExpressionRule
      : rule<(assign<&ast::RangeExpr::start>(ref<LogicalExprRule>) +
              lit<":"> + assign<&ast::RangeExpr::step>(ref<LogicalExprRule>) +
              lit<":"> +
              assign<&ast::RangeExpr::stop>(ref<LogicalExprRule>))  //
             | (assign<&ast::RangeExpr::start>(ref<LogicalExprRule>) +
                lit<":"> +
                assign<&ast::RangeExpr::stop>(ref<LogicalExprRule>))  //
             | ref<LogicalExprRule>> {};

  struct ExpressionRule
      : rule<(kw<"if"> + append<&ast::IfExpr::conditions>(ref<ExpressionRule>) +
              kw<"then"> + append<&ast::IfExpr::branches>(ref<ExpressionRule>) +
              many(kw<"elseif"> +
                   append<&ast::IfExpr::conditions>(ref<ExpressionRule>) +
                   kw<"then"> +
                   append<&ast::IfExpr::branches>(ref<ExpressionRule>)) +
              kw<"else"> +
              assign<&ast::IfExpr::elseBranch>(ref<ExpressionRule>))  //
             | ref<SimpleExpressionRule>> {};

  // -- modification system ----

  struct AnnotationRule
      : rule<kw<"annotation"> +
             assign<&ast::Annotation::mod>(ref<ClassModRule>)> {};

  struct ClassModRule
      : rule<lit<"("> +
             option(some(append<&ast::ClassMod::args>(ref<ArgumentRule>),
                         lit<",">)) +
             lit<")">> {};

  struct ModificationRule
      : rule<(assign<&ast::Modification::mod>(ref<ClassModRule>) +
              option(assign<&ast::Modification::assignOp>(lit<"=">) +
                     assign<&ast::Modification::value>(ref<ExpressionRule>)))  //
             | (assign<&ast::Modification::assignOp>(lit<"=">) +
                assign<&ast::Modification::value>(ref<ExpressionRule>))  //
             | (assign<&ast::Modification::assignOp>(lit<":=">) +
                assign<&ast::Modification::value>(ref<ExpressionRule>))> {};

  struct ElementModWithFlagsRule
      : rule<option(flag<&ast::ElementMod::each_>(kw<"each">)) +
             option(flag<&ast::ElementMod::final_>(kw<"final">)) +
             assign<&ast::ElementMod::name>(ref<NameRule>) +
             option(assign<&ast::ElementMod::mod>(ref<ModificationRule>)) +
             option(some(append<&ast::ElementMod::docStrings>(ref<STRING>),
                         lit<"+">))> {};

  struct ElementReplaceableRule
      : rule<kw<"replaceable"> +
             (assign<&ast::ElementReplaceableArg::classDef>(
                  ref<ShortClassDefinitionRule>)  //
              | assign<&ast::ElementReplaceableArg::component>(
                    ref<ComponentClause1Rule>)) +
             option(kw<"constrainedby"> +
                    assign<&ast::ElementReplaceableArg::constrainingClause>(
                        ref<NameRule>) +
                    option(assign<&ast::ElementReplaceableArg::constrainingMod>(
                        ref<ClassModRule>)))> {};

  struct ElementReplaceableWithFlagsRule
      : rule<option(flag<&ast::ElementReplaceableArg::each_>(kw<"each">)) +
             option(flag<&ast::ElementReplaceableArg::final_>(kw<"final">)) +
             kw<"replaceable"> +
             (assign<&ast::ElementReplaceableArg::classDef>(
                  ref<ShortClassDefinitionRule>)  //
              | assign<&ast::ElementReplaceableArg::component>(
                    ref<ComponentClause1Rule>)) +
             option(kw<"constrainedby"> +
                    assign<&ast::ElementReplaceableArg::constrainingClause>(
                        ref<NameRule>) +
                    option(assign<&ast::ElementReplaceableArg::constrainingMod>(
                        ref<ClassModRule>)))> {};

  struct ElementRedeclareRule
      : rule<kw<"redeclare"> +
             option(flag<&ast::ElementRedeclare::each_>(kw<"each">)) +
             option(flag<&ast::ElementRedeclare::final_>(kw<"final">)) +
             (assign<&ast::ElementRedeclare::classDef>(
                  ref<ShortClassDefinitionRule>)  //
              | assign<&ast::ElementRedeclare::component>(
                    ref<ComponentClause1Rule>)  //
              | assign<&ast::ElementRedeclare::replaceable_>(
                    ref<ElementReplaceableRule>))> {};

  struct ArgumentRule
      : rule<ref<ElementModWithFlagsRule> |
             ref<ElementReplaceableWithFlagsRule> |
             ref<ElementRedeclareRule>> {};

  // -- component declarations ----

  struct ComponentDeclRule
      : rule<assign<&ast::ComponentDecl::name>(ref<IDENT>) +
             option(lit<"["> +
                    some(append<&ast::ComponentDecl::subscripts>(
                             ref<SubscriptRule>),
                         lit<",">) +
                    lit<"]">) +
             option(assign<&ast::ComponentDecl::mod>(ref<ModificationRule>)) +
             option(kw<"if"> +
                    assign<&ast::ComponentDecl::condition>(ref<ExpressionRule>)) +
             option(some(append<&ast::ComponentDecl::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::ComponentDecl::annotation>(ref<AnnotationRule>))> {
  };

  struct ComponentDecl1Rule
      : rule<assign<&ast::ComponentDecl::name>(ref<IDENT>) +
             option(lit<"["> +
                    some(append<&ast::ComponentDecl::subscripts>(
                             ref<SubscriptRule>),
                         lit<",">) +
                    lit<"]">) +
             option(assign<&ast::ComponentDecl::mod>(ref<ModificationRule>)) +
             option(some(append<&ast::ComponentDecl::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::ComponentDecl::annotation>(ref<AnnotationRule>))> {
  };

  struct ComponentClauseRule
      : rule<option(assign<&ast::ComponentClause::flowStream>(TypePrefixFlow)) +
             option(assign<&ast::ComponentClause::variability>(TypePrefixVar)) +
             option(assign<&ast::ComponentClause::causality>(TypePrefixCaus)) +
             assign<&ast::ComponentClause::type>(ref<NameRule>) +
             option(lit<"["> +
                    some(append<&ast::ComponentClause::subscripts>(
                             ref<SubscriptRule>),
                         lit<",">) +
                    lit<"]">) +
             some(append<&ast::ComponentClause::components>(ref<ComponentDeclRule>),
                  lit<",">)> {};

  struct ComponentClause1Rule
      : rule<option(assign<&ast::ComponentClause::flowStream>(TypePrefixFlow)) +
             option(assign<&ast::ComponentClause::variability>(TypePrefixVar)) +
             option(assign<&ast::ComponentClause::causality>(TypePrefixCaus)) +
             assign<&ast::ComponentClause::type>(ref<NameRule>) +
             append<&ast::ComponentClause::components>(ref<ComponentDecl1Rule>)> {
  };

  // -- classes ----

  struct ShortClassDefinitionRule
      : rule<ClassPrefixes +
             assign<&ast::ClassDef::specifier>(ref<ShortClassSpecRule>)> {};

  struct ClassDefinitionRule
      : rule<option(flag<&ast::ClassDef::encapsulated_>(kw<"encapsulated">)) +
             ClassPrefixes +
             assign<&ast::ClassDef::specifier>(ref<ClassSpecifierRule>)> {};

  struct ClassSpecifierRule
      : rule<ref<LongClassSpecRule> | ref<ShortClassSpecRule> |
             ref<DerClassSpecRule>> {};

  struct LongClassSpecRule
      : rule<(assign<&ast::LongClassSpec::name>(ref<IDENT>) +
              option(some(append<&ast::LongClassSpec::docStrings>(ref<STRING>),
                          lit<"+">)) +
              assign<&ast::LongClassSpec::composition>(ref<CompositionRule>) +
              kw<"end"> +
              assign<&ast::LongClassSpec::endName>(ref<IDENT>))  //
             | (flag<&ast::LongClassSpec::isExtends>(kw<"extends">) +
                assign<&ast::LongClassSpec::name>(ref<IDENT>) +
                option(assign<&ast::LongClassSpec::extendsMod>(ref<ClassModRule>)) +
                option(some(append<&ast::LongClassSpec::docStrings>(ref<STRING>),
                            lit<"+">)) +
                assign<&ast::LongClassSpec::composition>(ref<CompositionRule>) +
                kw<"end"> +
                assign<&ast::LongClassSpec::endName>(ref<IDENT>))> {};

  struct EnumLiteralRule
      : rule<assign<&ast::EnumLiteral::name>(ref<IDENT>) +
             option(some(append<&ast::EnumLiteral::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::EnumLiteral::annotation>(ref<AnnotationRule>))> {
  };

  struct ShortClassSpecRule
      : rule<(assign<&ast::ShortClassSpec::name>(ref<IDENT>) + lit<"="> +
              option(assign<&ast::ShortClassSpec::flowStream>(TypePrefixFlow)) +
              option(
                  assign<&ast::ShortClassSpec::variability>(TypePrefixVar)) +
              option(assign<&ast::ShortClassSpec::causality>(TypePrefixCaus)) +
              assign<&ast::ShortClassSpec::baseType>(ref<NameRule>) +
              option(lit<"["> +
                     some(append<&ast::ShortClassSpec::subscripts>(
                              ref<SubscriptRule>),
                          lit<",">) +
                     lit<"]">) +
              option(assign<&ast::ShortClassSpec::mod>(ref<ClassModRule>)) +
              option(some(append<&ast::ShortClassSpec::docStrings>(ref<STRING>),
                          lit<"+">)) +
              option(
                  assign<&ast::ShortClassSpec::annotation>(ref<AnnotationRule>)))  //
             | (assign<&ast::ShortClassSpec::name>(ref<IDENT>) + lit<"="> +
                flag<&ast::ShortClassSpec::isEnumeration>(
                    kw<"enumeration">) +
                lit<"("> +
                (option(some(append<&ast::ShortClassSpec::literals>(
                                 ref<EnumLiteralRule>),
                             lit<",">))  //
                 | flag<&ast::ShortClassSpec::enumUnspecified>(lit<":">)) +
                lit<")"> +
                option(some(append<&ast::ShortClassSpec::docStrings>(ref<STRING>),
                            lit<"+">)) +
                option(assign<&ast::ShortClassSpec::annotation>(
                    ref<AnnotationRule>)))> {};

  struct DerClassSpecRule
      : rule<assign<&ast::DerClassSpec::name>(ref<IDENT>) + lit<"="> +
             kw<"der"> + lit<"("> +
             assign<&ast::DerClassSpec::baseType>(ref<NameRule>) + lit<","> +
             some(append<&ast::DerClassSpec::args>(ref<IDENT>), lit<",">) +
             lit<")"> +
             option(some(append<&ast::DerClassSpec::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::DerClassSpec::annotation>(ref<AnnotationRule>))> {
  };

  // -- elements and composition ----

  struct ImportClauseRule
      : rule<kw<"import"> +
             ((assign<&ast::ImportClause::alias>(ref<IDENT>) + lit<"="> +
               assign<&ast::ImportClause::name>(ref<NameRule>))  //
              | (assign<&ast::ImportClause::name>(ref<NameRule>) +
                 flag<&ast::ImportClause::wildcard>(lit<".*">))  //
              | (assign<&ast::ImportClause::name>(ref<NameRule>) + lit<".{"> +
                 some(append<&ast::ImportClause::selective>(ref<IDENT>),
                      lit<",">) +
                 lit<"}">)  //
              | assign<&ast::ImportClause::name>(ref<NameRule>)) +
             option(some(append<&ast::ImportClause::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::ImportClause::annotation>(ref<AnnotationRule>))> {
  };

  struct ExtendsClauseRule
      : rule<kw<"extends"> + assign<&ast::ExtendsClause::name>(ref<NameRule>) +
             option(assign<&ast::ExtendsClause::mod>(ref<ClassModRule>)) +
             option(assign<&ast::ExtendsClause::annotation>(ref<AnnotationRule>))> {
  };

  struct ElementDeclRule
      : rule<option(flag<&ast::ElementDecl::redeclare_>(kw<"redeclare">)) +
             option(flag<&ast::ElementDecl::final_>(kw<"final">)) +
             option(flag<&ast::ElementDecl::inner_>(kw<"inner">)) +
             option(flag<&ast::ElementDecl::outer_>(kw<"outer">)) +
             ((assign<&ast::ElementDecl::classDef>(ref<ClassDefinitionRule>)  //
               | assign<&ast::ElementDecl::component>(ref<ComponentClauseRule>))  //
              | (flag<&ast::ElementDecl::replaceable_>(kw<"replaceable">) +
                 (assign<&ast::ElementDecl::classDef>(ref<ClassDefinitionRule>)  //
                  | assign<&ast::ElementDecl::component>(
                        ref<ComponentClauseRule>)) +
                 option(kw<"constrainedby"> +
                        assign<&ast::ElementDecl::constrainingClause>(
                            ref<NameRule>) +
                        option(assign<&ast::ElementDecl::constrainingMod>(
                            ref<ClassModRule>)) +
                        option(some(append<&ast::ElementDecl::docStrings>(
                                        ref<STRING>),
                                    lit<"+">)) +
                        option(assign<&ast::ElementDecl::annotation>(
                            ref<AnnotationRule>)))))> {};

  struct ElementRule
      : rule<ref<ImportClauseRule> | ref<ExtendsClauseRule> |
             ref<ElementDeclRule>> {};

  struct DefaultSectionRule
      : rule<many(append<&ast::CompositionSection::elements>(ref<ElementRule>) +
                  lit<";">)> {};

  struct PublicSectionRule
      : rule<assign<&ast::CompositionSection::kind>(kw<"public">) +
             many(append<&ast::CompositionSection::elements>(ref<ElementRule>) +
                  lit<";">)> {};

  struct ProtectedSectionRule
      : rule<assign<&ast::CompositionSection::kind>(kw<"protected">) +
             many(append<&ast::CompositionSection::elements>(ref<ElementRule>) +
                  lit<";">)> {};

  struct EquationSectionRule
      : rule<option(flag<&ast::CompositionSection::initial_>(kw<"initial">)) +
             assign<&ast::CompositionSection::kind>(kw<"equation">) +
             many(append<&ast::CompositionSection::equations>(ref<EquationRule>) +
                  lit<";">)> {};

  struct AlgorithmSectionRule
      : rule<option(flag<&ast::CompositionSection::initial_>(kw<"initial">)) +
             assign<&ast::CompositionSection::kind>(kw<"algorithm">) +
             many(append<&ast::CompositionSection::statements>(
                      ref<StatementRule>) +
                  lit<";">)> {};

  struct ExternalCallRule
      : rule<option(assign<&ast::ExternalCall::target>(
                 ref<ComponentReferenceRule>) +
                 lit<"=">) +
             assign<&ast::ExternalCall::func>(ref<IDENT>) + lit<"("> +
             option(some(append<&ast::ExternalCall::args>(ref<ExpressionRule>),
                         lit<",">)) +
             lit<")">> {};

  struct CompositionRule
      : rule<append<&ast::Composition::sections>(ref<DefaultSectionRule>) +
             many(append<&ast::Composition::sections>(ref<PublicSectionRule>)  //
                  | append<&ast::Composition::sections>(ref<ProtectedSectionRule>)  //
                  | append<&ast::Composition::sections>(ref<EquationSectionRule>)  //
                  | append<&ast::Composition::sections>(
                        ref<AlgorithmSectionRule>)) +
             option(flag<&ast::Composition::hasExternal>(kw<"external">) +
                    option(assign<&ast::Composition::externalLang>(ref<STRING>)) +
                    option(assign<&ast::Composition::externalCall>(
                        ref<ExternalCallRule>)) +
                    option(assign<&ast::Composition::externalAnnotation>(
                        ref<AnnotationRule>)) +
                    lit<";">) +
             option(assign<&ast::Composition::finalAnnotation>(
                        ref<AnnotationRule>) +
                    lit<";">)> {};

  // -- equations ----

  struct EqBlockRule
      : rule<many(append<&ast::EqBlock::equations>(ref<EquationRule>) +
                  lit<";">)> {};

  struct IfEquationRule
      : rule<kw<"if"> + append<&ast::IfEquation::conditions>(ref<ExpressionRule>) +
             kw<"then"> + append<&ast::IfEquation::branches>(ref<EqBlockRule>) +
             many(kw<"elseif"> +
                  append<&ast::IfEquation::conditions>(ref<ExpressionRule>) +
                  kw<"then"> +
                  append<&ast::IfEquation::branches>(ref<EqBlockRule>)) +
             option(kw<"else"> +
                    assign<&ast::IfEquation::elseBranch>(ref<EqBlockRule>)) +
             kw<"end"> + kw<"if"> +
             option(some(append<&ast::IfEquation::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::IfEquation::annotation>(ref<AnnotationRule>))> {
  };

  struct ForEquationRule
      : rule<kw<"for"> +
             some(append<&ast::ForEquation::indices>(ref<ForIndexRule>),
                  lit<",">) +
             kw<"loop"> +
             many(append<&ast::ForEquation::body>(ref<EquationRule>) + lit<";">) +
             kw<"end"> + kw<"for"> +
             option(some(append<&ast::ForEquation::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::ForEquation::annotation>(ref<AnnotationRule>))> {
  };

  struct ConnectEquationRule
      : rule<kw<"connect"> + lit<"("> +
             assign<&ast::ConnectEquation::a>(ref<ComponentReferenceRule>) +
             lit<","> +
             assign<&ast::ConnectEquation::b>(ref<ComponentReferenceRule>) +
             lit<")"> +
             option(some(append<&ast::ConnectEquation::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(
                 assign<&ast::ConnectEquation::annotation>(ref<AnnotationRule>))> {
  };

  struct WhenEquationRule
      : rule<kw<"when"> +
             append<&ast::WhenEquation::conditions>(ref<ExpressionRule>) +
             kw<"then"> + append<&ast::WhenEquation::branches>(ref<EqBlockRule>) +
             many(kw<"elsewhen"> +
                  append<&ast::WhenEquation::conditions>(ref<ExpressionRule>) +
                  kw<"then"> +
                  append<&ast::WhenEquation::branches>(ref<EqBlockRule>)) +
             kw<"end"> + kw<"when"> +
             option(some(append<&ast::WhenEquation::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::WhenEquation::annotation>(ref<AnnotationRule>))> {
  };

  struct EquationRule
      : rule<create<ast::EqEquality>(
                 assign<&ast::EqEquality::lhs>(ref<SimpleExpressionRule>) +
                 lit<"="> + assign<&ast::EqEquality::rhs>(ref<ExpressionRule>) +
                 option(some(append<&ast::EqEquality::docStrings>(ref<STRING>),
                             lit<"+">)) +
                 option(assign<&ast::EqEquality::annotation>(
                     ref<AnnotationRule>)))  //
             | ref<IfEquationRule>  //
             | ref<ForEquationRule>  //
             | ref<ConnectEquationRule>  //
             | ref<WhenEquationRule>  //
             | create<ast::CallEquation>(
                   assign<&ast::CallEquation::callee>(ref<NameRule>) + lit<"("> +
                   option(assign<&ast::CallEquation::args>(
                       ref<FunctionArgumentsRule>)) +
                   lit<")"> +
                   option(some(append<&ast::CallEquation::docStrings>(ref<STRING>),
                               lit<"+">)) +
                   option(assign<&ast::CallEquation::annotation>(
                       ref<AnnotationRule>)))> {};

  // -- statements ----

  struct StmtBlockRule
      : rule<many(append<&ast::StmtBlock::statements>(ref<StatementRule>) +
                  lit<";">)> {};

  struct IfStmtRule
      : rule<kw<"if"> + append<&ast::IfStmt::conditions>(ref<ExpressionRule>) +
             kw<"then"> + append<&ast::IfStmt::branches>(ref<StmtBlockRule>) +
             many(kw<"elseif"> +
                  append<&ast::IfStmt::conditions>(ref<ExpressionRule>) +
                  kw<"then"> +
                  append<&ast::IfStmt::branches>(ref<StmtBlockRule>)) +
             option(kw<"else"> +
                    assign<&ast::IfStmt::elseBranch>(ref<StmtBlockRule>)) +
             kw<"end"> + kw<"if"> +
             option(some(append<&ast::IfStmt::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::IfStmt::annotation>(ref<AnnotationRule>))> {};

  struct ForStmtRule
      : rule<kw<"for"> +
             some(append<&ast::ForStmt::indices>(ref<ForIndexRule>), lit<",">) +
             kw<"loop"> +
             many(append<&ast::ForStmt::body>(ref<StatementRule>) + lit<";">) +
             kw<"end"> + kw<"for"> +
             option(some(append<&ast::ForStmt::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::ForStmt::annotation>(ref<AnnotationRule>))> {};

  struct WhileStmtRule
      : rule<kw<"while"> + assign<&ast::WhileStmt::cond>(ref<ExpressionRule>) +
             kw<"loop"> +
             many(append<&ast::WhileStmt::body>(ref<StatementRule>) + lit<";">) +
             kw<"end"> + kw<"while"> +
             option(some(append<&ast::WhileStmt::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::WhileStmt::annotation>(ref<AnnotationRule>))> {};

  struct WhenStmtRule
      : rule<kw<"when"> + append<&ast::WhenStmt::conditions>(ref<ExpressionRule>) +
             kw<"then"> + append<&ast::WhenStmt::branches>(ref<StmtBlockRule>) +
             many(kw<"elsewhen"> +
                  append<&ast::WhenStmt::conditions>(ref<ExpressionRule>) +
                  kw<"then"> +
                  append<&ast::WhenStmt::branches>(ref<StmtBlockRule>)) +
             kw<"end"> + kw<"when"> +
             option(some(append<&ast::WhenStmt::docStrings>(ref<STRING>),
                         lit<"+">)) +
             option(assign<&ast::WhenStmt::annotation>(ref<AnnotationRule>))> {};

  struct StatementRule
      : rule<create<ast::AssignStmt>(
                 assign<&ast::AssignStmt::target>(ref<ComponentReferenceRule>) +
                 lit<":="> +
                 assign<&ast::AssignStmt::value>(ref<ExpressionRule>) +
                 option(some(append<&ast::AssignStmt::docStrings>(ref<STRING>),
                             lit<"+">)) +
                 option(assign<&ast::AssignStmt::annotation>(
                     ref<AnnotationRule>)))  //
             | create<ast::CallStmt>(
                   assign<&ast::CallStmt::callee>(ref<ComponentReferenceRule>) +
                   lit<"("> +
                   option(assign<&ast::CallStmt::args>(
                       ref<FunctionArgumentsRule>)) +
                   lit<")"> +
                   option(some(append<&ast::CallStmt::docStrings>(ref<STRING>),
                               lit<"+">)) +
                   option(assign<&ast::CallStmt::annotation>(
                       ref<AnnotationRule>)))  //
             | create<ast::MultiAssignStmt>(
                   lit<"("> +
                   append<&ast::MultiAssignStmt::targets>(
                       ref<ExpressionRule> | create<ast::OmittedOutput>) +
                   many(lit<","> +
                        append<&ast::MultiAssignStmt::targets>(
                            ref<ExpressionRule> | create<ast::OmittedOutput>)) +
                   lit<")"> + lit<":="> +
                   assign<&ast::MultiAssignStmt::callee>(
                       ref<ComponentReferenceRule>) +
                   lit<"("> +
                   option(assign<&ast::MultiAssignStmt::args>(
                       ref<FunctionArgumentsRule>)) +
                   lit<")"> +
                   option(some(
                       append<&ast::MultiAssignStmt::docStrings>(ref<STRING>),
                       lit<"+">)) +
                   option(assign<&ast::MultiAssignStmt::annotation>(
                       ref<AnnotationRule>)))  //
             | create<ast::BreakStmt>(
                   kw<"break"> +
                   option(some(append<&ast::BreakStmt::docStrings>(ref<STRING>),
                               lit<"+">)) +
                   option(assign<&ast::BreakStmt::annotation>(
                       ref<AnnotationRule>)))  //
             |
             create<ast::ReturnStmt>(
                 kw<"return"> +
                 option(some(append<&ast::ReturnStmt::docStrings>(ref<STRING>),
                             lit<"+">)) +
                 option(assign<&ast::ReturnStmt::annotation>(
                     ref<AnnotationRule>)))  //
             | ref<IfStmtRule>               //
             | ref<ForStmtRule>              //
             | ref<WhileStmtRule>            //
             | ref<WhenStmtRule>> {};

  // -- top level ----

  struct WithinClauseRule
      : rule<kw<"within"> + option(assign<&ast::WithinClause::name>(ref<NameRule>)) +
             lit<";">> {};

  struct TopClassDefRule
      : rule<option(flag<&ast::TopClassDef::final_>(kw<"final">)) +
             assign<&ast::TopClassDef::def>(ref<ClassDefinitionRule>) +
             lit<";">> {};

  struct StoredDefinitionRule
      : rule<many(append<&ast::StoredDefinition::withins>(ref<WithinClauseRule>)) +
             many(append<&ast::StoredDefinition::classes>(ref<TopClassDefRule>))> {
  };

  using entry = StoredDefinitionRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, COMMENT>;
};

}  // namespace modelica
