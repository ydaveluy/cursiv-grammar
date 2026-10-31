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

  // Modelica reserved words remain unavailable in IDENT-only contexts.
  static constexpr auto Reserved =
      kw<"algorithm"> | kw<"discrete"> | kw<"false"> | kw<"loop"> | kw<"pure"> |
      kw<"and"> | kw<"each"> | kw<"final"> | kw<"model"> | kw<"record"> |
      kw<"annotation"> | kw<"else"> | kw<"flow"> | kw<"not"> | kw<"redeclare"> |
      kw<"elseif"> | kw<"for"> | kw<"operator"> | kw<"replaceable"> | kw<"block"> |
      kw<"elsewhen"> | kw<"function"> | kw<"or"> | kw<"return"> | kw<"break"> |
      kw<"encapsulated"> | kw<"if"> | kw<"outer"> | kw<"stream"> | kw<"class"> |
      kw<"end"> | kw<"import"> | kw<"output"> | kw<"then"> | kw<"connect"> |
      kw<"enumeration"> | kw<"impure"> | kw<"package"> | kw<"true"> | kw<"connector"> |
      kw<"equation"> | kw<"in"> | kw<"parameter"> | kw<"type"> | kw<"constant"> |
      kw<"expandable"> | kw<"initial"> | kw<"partial"> | kw<"when"> | kw<"constrainedby"> |
      kw<"extends"> | kw<"inner"> | kw<"protected"> | kw<"while"> | kw<"der"> |
      kw<"external"> | kw<"input"> | kw<"public"> | kw<"within">;

  static constexpr auto QChar =
      cr<R"( 0-9A-Za-z_!#$%&()*+,./:;<>=?@[]{}|~^"-)">;
  static constexpr auto SEscape =
      lit<"\\"> + cr<R"(abfnrtv\\"'?)">;
  struct IDENT
      : terminal<!Reserved + ((cr<"a-zA-Z_"> + many(w)) |
                             (lit<"'"> + many(QChar | SEscape) + lit<"'">))> {};

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
  struct STRING : terminal<as<unescape>(
                      lit<"\""> + many(SEscape | cr<R"(^"\\)">) + lit<"\"">)> {
  };

  static constexpr auto UInt = some(d);
  struct UNSIGNED_NUMBER
      : terminal<((UInt + option(lit<"."> + option(UInt))) |
                  (lit<"."> + UInt)) +
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

  struct Subscript;
  struct Primary;
  struct Factor;
  struct Term;
  struct SignedTerm;
  struct ArithExpr;
  struct Relation;
  struct LogicalFactor;
  struct LogicalExpr;
  struct SimpleExpression;
  struct Expression;
  struct ExprRow;
  struct Name;
  struct SpecialCallCallee;
  struct CallCallee;
  struct ComponentReference;
  struct RefPart;
  struct FunctionArguments;
  struct FunctionArgument;
  struct FunctionPartialApplication;
  struct NamedArgument;
  struct ForIndex;
  struct Annotation;
  struct ClassMod;
  struct Argument;
  struct ElementModWithFlags;
  struct ElementReplaceableWithFlags;
  struct ElementReplaceable;
  struct ElementRedeclare;
  struct Modification;
  struct ComponentDecl;
  struct ComponentDecl1;
  struct ComponentClause;
  struct ComponentClause1;
  struct ShortClassDefinition;
  struct ClassDefinition;
  struct ClassSpecifier;
  struct LongClassSpec;
  struct ShortClassSpec;
  struct EnumLiteral;
  struct DerClassSpec;
  struct ImportClause;
  struct ExtendsClause;
  struct Element;
  struct ElementDecl;
  struct Composition;
  struct DefaultSection;
  struct PublicSection;
  struct ProtectedSection;
  struct EquationSection;
  struct AlgorithmSection;
  struct ExternalCall;
  struct Equation;
  struct IfEquation;
  struct ForEquation;
  struct ConnectEquation;
  struct WhenEquation;
  struct EqBlock;
  struct Statement;
  struct StmtBlock;
  struct IfStmt;
  struct ForStmt;
  struct WhileStmt;
  struct WhenStmt;
  struct WithinClause;
  struct TopClassDef;
  struct StoredDefinition;

  // -- names and references ----

  struct Name
      : rule<option(flag<&ast::Name::global>(lit<".">)) +
             append<&ast::Name::parts>(ref<IDENT>) +
             many(lit<"."> + append<&ast::Name::parts>(ref<IDENT>))> {};

  struct SpecialCallCallee
      : rule<create<ast::ComponentRef> +
             append<&ast::ComponentRef::parts>(create<ast::RefPart>(
                 assign<&ast::RefPart::name>(kw<"der"> | kw<"initial"> | kw<"pure">)))> {};

  struct CallCallee
      : rule<ref<ComponentReference> | ref<SpecialCallCallee>> {};

  struct RefPart
      : rule<assign<&ast::RefPart::name>(ref<IDENT>) +
             option(lit<"["> +
                    some(append<&ast::RefPart::subscripts>(ref<Subscript>),
                         lit<",">) +
                    lit<"]">)> {};

  struct ComponentReference
      : rule<option(flag<&ast::ComponentRef::global>(lit<".">)) +
             append<&ast::ComponentRef::parts>(ref<RefPart>) +
             many(lit<"."> +
                  append<&ast::ComponentRef::parts>(ref<RefPart>))> {};

  // -- expressions ----

  struct Subscript
      : rule<(create<ast::ColonExpr> + lit<":">) | ref<Expression>> {};

  struct ExprRow
      : rule<some(append<&ast::ExprRow::items>(ref<Expression>),
                  lit<",">)> {};

  struct NamedArgument
      : rule<assign<&ast::NamedArg::name>(ref<IDENT>) + lit<"="> +
             assign<&ast::NamedArg::value>(ref<FunctionArgument>)> {};

  struct FunctionPartialApplication
      : rule<kw<"function"> + assign<&ast::FunctionRefArg::func>(ref<Name>) +
             lit<"("> +
             many(append<&ast::FunctionRefArg::named>(ref<NamedArgument>), lit<",">) +
             lit<")">> {};
  struct FunctionArgument : rule<ref<FunctionPartialApplication> | ref<Expression>> {};

  struct ForIndex
      : rule<assign<&ast::ForIndex::name>(ref<IDENT>) +
             option(kw<"in"> + assign<&ast::ForIndex::range>(ref<Expression>))> {
  };

  // The first expression may head a reduction/comprehension. After a
  // comma only positional/named arguments continue; partial applications
  // and named arguments do not open a for-clause.
  static constexpr auto NonFirstArguments =
      (some(append<&ast::CallArgs::positional>(ref<FunctionArgument>), lit<",">) +
       option(lit<","> + some(append<&ast::CallArgs::named>(ref<NamedArgument>), lit<",">))) |
      some(append<&ast::CallArgs::named>(ref<NamedArgument>), lit<",">);
  static constexpr auto ArgumentIndices =
      kw<"for"> + some(append<&ast::CallArgs::forIndices>(ref<ForIndex>), lit<",">);
  struct FunctionArguments
      : rule<(append<&ast::CallArgs::positional>(ref<Expression>) +
              option((lit<","> + NonFirstArguments) | ArgumentIndices)) |
             (append<&ast::CallArgs::positional>(ref<FunctionPartialApplication>) +
              option(lit<","> + NonFirstArguments)) |
             some(append<&ast::CallArgs::named>(ref<NamedArgument>), lit<",">)> {};
  struct ArrayArguments
      : rule<append<&ast::CallArgs::positional>(ref<Expression>) +
             (ArgumentIndices |
              many(lit<","> + append<&ast::CallArgs::positional>(ref<Expression>)))> {};

  struct Primary
      : rule<create<ast::NumberLit>(
                 assign<&ast::NumberLit::value>(ref<UNSIGNED_NUMBER>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))  //
             | (create<ast::BoolLit> +
                (flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">))  //
             | create<ast::CallExpr>(
                   assign<&ast::CallExpr::callee>(ref<CallCallee>) +
                   lit<"("> +
                   option(assign<&ast::CallExpr::args>(
                       ref<FunctionArguments>)) +
                   lit<")">)                //
             | ref<ComponentReference>  //
             | (lit<"("> +
                append<&ast::TupleExpr::items>(ref<Expression> |
                                               create<ast::OmittedOutput>) +
                many(lit<","> +
                     append<&ast::TupleExpr::items>(
                         ref<Expression> | create<ast::OmittedOutput>)) +
                lit<")">)  //
             |
             (lit<"["> + append<&ast::MatrixLit::rows>(ref<ExprRow>) +
              many(lit<";"> + append<&ast::MatrixLit::rows>(ref<ExprRow>)) +
              lit<"]">)  //
             | create<ast::ArrayLit>(
                   lit<"{"> +
                   assign<&ast::ArrayLit::args>(ref<ArrayArguments>) +
                   lit<"}">)  //
             | (create<ast::EndExpr> + kw<"end">)> {};

  struct Factor
      : rule<(assign<&ast::BinaryExpr::left>(ref<Primary>) +
              assign<&ast::BinaryExpr::op>(lit<"^"> | lit<".^">) +
              assign<&ast::BinaryExpr::right>(ref<Primary>))  //
             | ref<Primary>> {};

  struct Term
      : infix<Factor, ast::BinaryExpr, (left(MulOp))> {};

  // A leading sign precedes the first whole term. Subsequent additive
  // operands are unsigned terms, as in the native arithmetic-expression.
  struct SignedTerm
      : rule<(assign<&ast::UnaryExpr::op>(AddOp) +
              assign<&ast::UnaryExpr::operand>(ref<Term>))  //
             | ref<Term>> {};

  struct ArithExpr
      : rule<ref<SignedTerm> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(AddOp) +
                 assign<&ast::BinaryExpr::right>(ref<Term>)))> {};

  struct Relation
      : rule<(assign<&ast::BinaryExpr::left>(ref<ArithExpr>) +
              assign<&ast::BinaryExpr::op>(RelOp) +
              assign<&ast::BinaryExpr::right>(ref<ArithExpr>))  //
             | ref<ArithExpr>> {};

  struct LogicalFactor
      : rule<(assign<&ast::UnaryExpr::op>(kw<"not">) +
              assign<&ast::UnaryExpr::operand>(ref<Relation>))  //
             | ref<Relation>> {};

  struct LogicalExpr
      : infix<LogicalFactor, ast::BinaryExpr,
              levels(left(kw<"and">), left(kw<"or">))> {};

  struct SimpleExpression
      : rule<(assign<&ast::RangeExpr::start>(ref<LogicalExpr>) +
              lit<":"> + assign<&ast::RangeExpr::step>(ref<LogicalExpr>) +
              lit<":"> +
              assign<&ast::RangeExpr::stop>(ref<LogicalExpr>))  //
             | (assign<&ast::RangeExpr::start>(ref<LogicalExpr>) +
                lit<":"> +
                assign<&ast::RangeExpr::stop>(ref<LogicalExpr>))  //
             | ref<LogicalExpr>> {};

  struct Expression
      : rule<(kw<"if"> + append<&ast::IfExpr::conditions>(ref<Expression>) +
              kw<"then"> + append<&ast::IfExpr::branches>(ref<Expression>) +
              many(kw<"elseif"> +
                   append<&ast::IfExpr::conditions>(ref<Expression>) +
                   kw<"then"> +
                   append<&ast::IfExpr::branches>(ref<Expression>)) +
              kw<"else"> +
              assign<&ast::IfExpr::elseBranch>(ref<Expression>))  //
             | ref<SimpleExpression>> {};

  // -- modification system ----

  struct Annotation
      : rule<kw<"annotation"> +
             assign<&ast::Annotation::mod>(ref<ClassMod>)> {};

  static constexpr auto ConnectBody = kw<"connect"> + lit<"("> +
      assign<&ast::ConnectEquation::a>(ref<ComponentReference>) + lit<","> +
      assign<&ast::ConnectEquation::b>(ref<ComponentReference>) + lit<")">;
  struct BrokenConnection : rule<ConnectBody> {};
  struct InheritanceBreak : rule<kw<"break"> +
      (assign<&ast::InheritanceBreak::name>(ref<IDENT>) |
       assign<&ast::InheritanceBreak::connection>(ref<BrokenConnection>))> {};
  template<auto Item> static constexpr auto class_modification =
      lit<"("> + many(append<&ast::ClassMod::args>(Item), lit<",">) + lit<")">;
  struct ClassMod : rule<class_modification<ref<Argument>>> {};
  struct InheritanceClassMod
      : rule<class_modification<ref<Argument> | ref<InheritanceBreak>>> {};
  static constexpr auto ModificationValue =
      ref<Expression> | create<ast::BreakExpr>(kw<"break">);

  struct Modification
      : rule<(assign<&ast::Modification::mod>(ref<ClassMod>) +
              option(assign<&ast::Modification::assignOp>(lit<"=">) +
                     assign<&ast::Modification::value>(ModificationValue)))  //
             | (assign<&ast::Modification::assignOp>(lit<"=">) +
                assign<&ast::Modification::value>(ModificationValue))  //
             | (assign<&ast::Modification::assignOp>(lit<":=">) +
                assign<&ast::Modification::value>(ModificationValue))> {};

  struct ElementModWithFlags
      : rule<option(flag<&ast::ElementMod::each_>(kw<"each">)) +
             option(flag<&ast::ElementMod::final_>(kw<"final">)) +
             assign<&ast::ElementMod::name>(ref<Name>) +
             option(assign<&ast::ElementMod::mod>(ref<Modification>)) +
             many(append<&ast::ElementMod::docStrings>(ref<STRING>),
                         lit<"+">)> {};

  struct ElementReplaceable
      : rule<kw<"replaceable"> +
             (assign<&ast::ElementReplaceableArg::classDef>(
                  ref<ShortClassDefinition>)  //
              | assign<&ast::ElementReplaceableArg::component>(
                    ref<ComponentClause1>)) +
             option(kw<"constrainedby"> +
                    assign<&ast::ElementReplaceableArg::constrainingClause>(
                        ref<Name>) +
                    option(assign<&ast::ElementReplaceableArg::constrainingMod>(
                        ref<ClassMod>)))> {};

  struct ElementReplaceableWithFlags
      : rule<option(flag<&ast::ElementReplaceableArg::each_>(kw<"each">)) +
             option(flag<&ast::ElementReplaceableArg::final_>(kw<"final">)) +
             kw<"replaceable"> +
             (assign<&ast::ElementReplaceableArg::classDef>(
                  ref<ShortClassDefinition>)  //
              | assign<&ast::ElementReplaceableArg::component>(
                    ref<ComponentClause1>)) +
             option(kw<"constrainedby"> +
                    assign<&ast::ElementReplaceableArg::constrainingClause>(
                        ref<Name>) +
                    option(assign<&ast::ElementReplaceableArg::constrainingMod>(
                        ref<ClassMod>)))> {};

  struct ElementRedeclare
      : rule<kw<"redeclare"> +
             option(flag<&ast::ElementRedeclare::each_>(kw<"each">)) +
             option(flag<&ast::ElementRedeclare::final_>(kw<"final">)) +
             (assign<&ast::ElementRedeclare::classDef>(
                  ref<ShortClassDefinition>)  //
              | assign<&ast::ElementRedeclare::component>(
                    ref<ComponentClause1>)  //
              | assign<&ast::ElementRedeclare::replaceable_>(
                    ref<ElementReplaceable>))> {};

  struct Argument
      : rule<ref<ElementModWithFlags> |
             ref<ElementReplaceableWithFlags> |
             ref<ElementRedeclare>> {};

  // -- component declarations ----

  struct ComponentDecl
      : rule<assign<&ast::ComponentDecl::name>(ref<IDENT>) +
             option(lit<"["> +
                    some(append<&ast::ComponentDecl::subscripts>(
                             ref<Subscript>),
                         lit<",">) +
                    lit<"]">) +
             option(assign<&ast::ComponentDecl::mod>(ref<Modification>)) +
             option(kw<"if"> +
                    assign<&ast::ComponentDecl::condition>(ref<Expression>)) +
             many(append<&ast::ComponentDecl::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::ComponentDecl::annotation>(ref<Annotation>))> {
  };

  struct ComponentDecl1
      : rule<assign<&ast::ComponentDecl::name>(ref<IDENT>) +
             option(lit<"["> +
                    some(append<&ast::ComponentDecl::subscripts>(
                             ref<Subscript>),
                         lit<",">) +
                    lit<"]">) +
             option(assign<&ast::ComponentDecl::mod>(ref<Modification>)) +
             many(append<&ast::ComponentDecl::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::ComponentDecl::annotation>(ref<Annotation>))> {
  };

  struct ComponentClause
      : rule<option(assign<&ast::ComponentClause::flowStream>(TypePrefixFlow)) +
             option(assign<&ast::ComponentClause::variability>(TypePrefixVar)) +
             option(assign<&ast::ComponentClause::causality>(TypePrefixCaus)) +
             assign<&ast::ComponentClause::type>(ref<Name>) +
             option(lit<"["> +
                    some(append<&ast::ComponentClause::subscripts>(
                             ref<Subscript>),
                         lit<",">) +
                    lit<"]">) +
             some(append<&ast::ComponentClause::components>(ref<ComponentDecl>),
                  lit<",">)> {};

  struct ComponentClause1
      : rule<option(assign<&ast::ComponentClause::flowStream>(TypePrefixFlow)) +
             option(assign<&ast::ComponentClause::variability>(TypePrefixVar)) +
             option(assign<&ast::ComponentClause::causality>(TypePrefixCaus)) +
             assign<&ast::ComponentClause::type>(ref<Name>) +
             append<&ast::ComponentClause::components>(ref<ComponentDecl1>)> {
  };

  // -- classes ----

  struct ShortClassDefinition
      : rule<ClassPrefixes +
             assign<&ast::ClassDef::specifier>(ref<ShortClassSpec>)> {};

  struct ClassDefinition
      : rule<option(flag<&ast::ClassDef::encapsulated_>(kw<"encapsulated">)) +
             ClassPrefixes +
             assign<&ast::ClassDef::specifier>(ref<ClassSpecifier>)> {};

  struct ClassSpecifier
      : rule<ref<LongClassSpec> | ref<ShortClassSpec> |
             ref<DerClassSpec>> {};

  struct LongClassSpec
      : rule<(assign<&ast::LongClassSpec::name>(ref<IDENT>) +
              many(append<&ast::LongClassSpec::docStrings>(ref<STRING>),
                          lit<"+">) +
              assign<&ast::LongClassSpec::composition>(ref<Composition>) +
              kw<"end"> +
              assign<&ast::LongClassSpec::endName>(ref<IDENT>))  //
             | (flag<&ast::LongClassSpec::isExtends>(kw<"extends">) +
                assign<&ast::LongClassSpec::name>(ref<IDENT>) +
                option(assign<&ast::LongClassSpec::extendsMod>(ref<ClassMod>)) +
                many(append<&ast::LongClassSpec::docStrings>(ref<STRING>),
                            lit<"+">) +
                assign<&ast::LongClassSpec::composition>(ref<Composition>) +
                kw<"end"> +
                assign<&ast::LongClassSpec::endName>(ref<IDENT>))> {};

  struct EnumLiteral
      : rule<assign<&ast::EnumLiteral::name>(ref<IDENT>) +
             many(append<&ast::EnumLiteral::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::EnumLiteral::annotation>(ref<Annotation>))> {
  };

  struct ShortClassSpec
      : rule<(assign<&ast::ShortClassSpec::name>(ref<IDENT>) + lit<"="> +
              option(assign<&ast::ShortClassSpec::flowStream>(TypePrefixFlow)) +
              option(
                  assign<&ast::ShortClassSpec::variability>(TypePrefixVar)) +
              option(assign<&ast::ShortClassSpec::causality>(TypePrefixCaus)) +
              assign<&ast::ShortClassSpec::baseType>(ref<Name>) +
              option(lit<"["> +
                     some(append<&ast::ShortClassSpec::subscripts>(
                              ref<Subscript>),
                          lit<",">) +
                     lit<"]">) +
              option(assign<&ast::ShortClassSpec::mod>(ref<ClassMod>)) +
              many(append<&ast::ShortClassSpec::docStrings>(ref<STRING>),
                          lit<"+">) +
              option(
                  assign<&ast::ShortClassSpec::annotation>(ref<Annotation>)))  //
             | (assign<&ast::ShortClassSpec::name>(ref<IDENT>) + lit<"="> +
                flag<&ast::ShortClassSpec::isEnumeration>(
                    kw<"enumeration">) +
                lit<"("> +
                (many(append<&ast::ShortClassSpec::literals>(
                                 ref<EnumLiteral>),
                             lit<",">)  //
                 | flag<&ast::ShortClassSpec::enumUnspecified>(lit<":">)) +
                lit<")"> +
                many(append<&ast::ShortClassSpec::docStrings>(ref<STRING>),
                            lit<"+">) +
                option(assign<&ast::ShortClassSpec::annotation>(
                    ref<Annotation>)))> {};

  struct DerClassSpec
      : rule<assign<&ast::DerClassSpec::name>(ref<IDENT>) + lit<"="> +
             kw<"der"> + lit<"("> +
             assign<&ast::DerClassSpec::baseType>(ref<Name>) + lit<","> +
             some(append<&ast::DerClassSpec::args>(ref<IDENT>), lit<",">) +
             lit<")"> +
             many(append<&ast::DerClassSpec::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::DerClassSpec::annotation>(ref<Annotation>))> {
  };

  // -- elements and composition ----

  struct ImportClause
      : rule<kw<"import"> +
             ((assign<&ast::ImportClause::alias>(ref<IDENT>) + lit<"="> +
               assign<&ast::ImportClause::name>(ref<Name>))  //
              | (assign<&ast::ImportClause::name>(ref<Name>) +
                 flag<&ast::ImportClause::wildcard>(lit<".*"> | (lit<"."> + lit<"*">)))  //
              | (assign<&ast::ImportClause::name>(ref<Name>) + lit<"."> + lit<"{"> +
                 some(append<&ast::ImportClause::selective>(ref<IDENT>),
                      lit<",">) +
                 lit<"}">)  //
              | assign<&ast::ImportClause::name>(ref<Name>)) +
             many(append<&ast::ImportClause::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::ImportClause::annotation>(ref<Annotation>))> {
  };

  struct ExtendsClause
      : rule<kw<"extends"> + assign<&ast::ExtendsClause::name>(ref<Name>) +
             option(assign<&ast::ExtendsClause::mod>(ref<InheritanceClassMod>)) +
             option(assign<&ast::ExtendsClause::annotation>(ref<Annotation>))> {
  };

  struct ElementDecl
      : rule<option(flag<&ast::ElementDecl::redeclare_>(kw<"redeclare">)) +
             option(flag<&ast::ElementDecl::final_>(kw<"final">)) +
             option(flag<&ast::ElementDecl::inner_>(kw<"inner">)) +
             option(flag<&ast::ElementDecl::outer_>(kw<"outer">)) +
             ((assign<&ast::ElementDecl::classDef>(ref<ClassDefinition>)  //
               | assign<&ast::ElementDecl::component>(ref<ComponentClause>))  //
              | (flag<&ast::ElementDecl::replaceable_>(kw<"replaceable">) +
                 (assign<&ast::ElementDecl::classDef>(ref<ClassDefinition>)  //
                  | assign<&ast::ElementDecl::component>(
                        ref<ComponentClause>)) +
                 option(kw<"constrainedby"> +
                        assign<&ast::ElementDecl::constrainingClause>(
                            ref<Name>) +
                        option(assign<&ast::ElementDecl::constrainingMod>(
                            ref<ClassMod>)) +
                        many(append<&ast::ElementDecl::docStrings>(
                                        ref<STRING>),
                                    lit<"+">) +
                        option(assign<&ast::ElementDecl::annotation>(
                            ref<Annotation>)))))> {};

  struct Element
      : rule<ref<ImportClause> | ref<ExtendsClause> |
             ref<ElementDecl>> {};

  struct DefaultSection
      : rule<many(append<&ast::CompositionSection::elements>(ref<Element>) +
                  lit<";">)> {};

  struct PublicSection
      : rule<assign<&ast::CompositionSection::kind>(kw<"public">) +
             many(append<&ast::CompositionSection::elements>(ref<Element>) +
                  lit<";">)> {};

  struct ProtectedSection
      : rule<assign<&ast::CompositionSection::kind>(kw<"protected">) +
             many(append<&ast::CompositionSection::elements>(ref<Element>) +
                  lit<";">)> {};

  struct EquationSection
      : rule<option(flag<&ast::CompositionSection::initial_>(kw<"initial">)) +
             assign<&ast::CompositionSection::kind>(kw<"equation">) +
             many(append<&ast::CompositionSection::equations>(ref<Equation>) +
                  lit<";">)> {};

  struct AlgorithmSection
      : rule<option(flag<&ast::CompositionSection::initial_>(kw<"initial">)) +
             assign<&ast::CompositionSection::kind>(kw<"algorithm">) +
             many(append<&ast::CompositionSection::statements>(
                      ref<Statement>) +
                  lit<";">)> {};

  struct ExternalCall
      : rule<option(assign<&ast::ExternalCall::target>(
                 ref<ComponentReference>) +
                 lit<"=">) +
             assign<&ast::ExternalCall::func>(ref<IDENT>) + lit<"("> +
             many(append<&ast::ExternalCall::args>(ref<Expression>),
                         lit<",">) +
             lit<")">> {};

  struct Composition
      : rule<append<&ast::Composition::sections>(ref<DefaultSection>) +
             many(append<&ast::Composition::sections>(ref<PublicSection>)  //
                  | append<&ast::Composition::sections>(ref<ProtectedSection>)  //
                  | append<&ast::Composition::sections>(ref<EquationSection>)  //
                  | append<&ast::Composition::sections>(
                        ref<AlgorithmSection>)) +
             option(flag<&ast::Composition::hasExternal>(kw<"external">) +
                    option(assign<&ast::Composition::externalLang>(ref<STRING>)) +
                    option(assign<&ast::Composition::externalCall>(
                        ref<ExternalCall>)) +
                    option(assign<&ast::Composition::externalAnnotation>(
                        ref<Annotation>)) +
                    lit<";">) +
             option(assign<&ast::Composition::finalAnnotation>(
                        ref<Annotation>) +
                    lit<";">)> {};

  // -- equations ----

  struct EqBlock
      : rule<many(append<&ast::EqBlock::equations>(ref<Equation>) +
                  lit<";">)> {};

  struct IfEquation
      : rule<kw<"if"> + append<&ast::IfEquation::conditions>(ref<Expression>) +
             kw<"then"> + append<&ast::IfEquation::branches>(ref<EqBlock>) +
             many(kw<"elseif"> +
                  append<&ast::IfEquation::conditions>(ref<Expression>) +
                  kw<"then"> +
                  append<&ast::IfEquation::branches>(ref<EqBlock>)) +
             option(kw<"else"> +
                    assign<&ast::IfEquation::elseBranch>(ref<EqBlock>)) +
             kw<"end"> + kw<"if"> +
             many(append<&ast::IfEquation::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::IfEquation::annotation>(ref<Annotation>))> {
  };

  struct ForEquation
      : rule<kw<"for"> +
             some(append<&ast::ForEquation::indices>(ref<ForIndex>),
                  lit<",">) +
             kw<"loop"> +
             many(append<&ast::ForEquation::body>(ref<Equation>) + lit<";">) +
             kw<"end"> + kw<"for"> +
             many(append<&ast::ForEquation::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::ForEquation::annotation>(ref<Annotation>))> {
  };

  struct ConnectEquation
      : rule<ConnectBody +
             many(append<&ast::ConnectEquation::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(
                 assign<&ast::ConnectEquation::annotation>(ref<Annotation>))> {
  };

  struct WhenEquation
      : rule<kw<"when"> +
             append<&ast::WhenEquation::conditions>(ref<Expression>) +
             kw<"then"> + append<&ast::WhenEquation::branches>(ref<EqBlock>) +
             many(kw<"elsewhen"> +
                  append<&ast::WhenEquation::conditions>(ref<Expression>) +
                  kw<"then"> +
                  append<&ast::WhenEquation::branches>(ref<EqBlock>)) +
             kw<"end"> + kw<"when"> +
             many(append<&ast::WhenEquation::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::WhenEquation::annotation>(ref<Annotation>))> {
  };

  struct Equation
      : rule<create<ast::EqEquality>(
                 assign<&ast::EqEquality::lhs>(ref<SimpleExpression>) +
                 lit<"="> + assign<&ast::EqEquality::rhs>(ref<Expression>) +
                 many(append<&ast::EqEquality::docStrings>(ref<STRING>),
                             lit<"+">) +
                 option(assign<&ast::EqEquality::annotation>(
                     ref<Annotation>)))  //
             | ref<IfEquation>  //
             | ref<ForEquation>  //
             | ref<ConnectEquation>  //
             | ref<WhenEquation>  //
             | create<ast::CallEquation>(
                   assign<&ast::CallEquation::callee>(ref<ComponentReference>) + lit<"("> +
                   option(assign<&ast::CallEquation::args>(
                       ref<FunctionArguments>)) +
                   lit<")"> +
                   many(append<&ast::CallEquation::docStrings>(ref<STRING>),
                               lit<"+">) +
                   option(assign<&ast::CallEquation::annotation>(
                       ref<Annotation>)))> {};

  // -- statements ----

  struct StmtBlock
      : rule<many(append<&ast::StmtBlock::statements>(ref<Statement>) +
                  lit<";">)> {};

  struct IfStmt
      : rule<kw<"if"> + append<&ast::IfStmt::conditions>(ref<Expression>) +
             kw<"then"> + append<&ast::IfStmt::branches>(ref<StmtBlock>) +
             many(kw<"elseif"> +
                  append<&ast::IfStmt::conditions>(ref<Expression>) +
                  kw<"then"> +
                  append<&ast::IfStmt::branches>(ref<StmtBlock>)) +
             option(kw<"else"> +
                    assign<&ast::IfStmt::elseBranch>(ref<StmtBlock>)) +
             kw<"end"> + kw<"if"> +
             many(append<&ast::IfStmt::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::IfStmt::annotation>(ref<Annotation>))> {};

  struct ForStmt
      : rule<kw<"for"> +
             some(append<&ast::ForStmt::indices>(ref<ForIndex>), lit<",">) +
             kw<"loop"> +
             many(append<&ast::ForStmt::body>(ref<Statement>) + lit<";">) +
             kw<"end"> + kw<"for"> +
             many(append<&ast::ForStmt::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::ForStmt::annotation>(ref<Annotation>))> {};

  struct WhileStmt
      : rule<kw<"while"> + assign<&ast::WhileStmt::cond>(ref<Expression>) +
             kw<"loop"> +
             many(append<&ast::WhileStmt::body>(ref<Statement>) + lit<";">) +
             kw<"end"> + kw<"while"> +
             many(append<&ast::WhileStmt::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::WhileStmt::annotation>(ref<Annotation>))> {};

  struct WhenStmt
      : rule<kw<"when"> + append<&ast::WhenStmt::conditions>(ref<Expression>) +
             kw<"then"> + append<&ast::WhenStmt::branches>(ref<StmtBlock>) +
             many(kw<"elsewhen"> +
                  append<&ast::WhenStmt::conditions>(ref<Expression>) +
                  kw<"then"> +
                  append<&ast::WhenStmt::branches>(ref<StmtBlock>)) +
             kw<"end"> + kw<"when"> +
             many(append<&ast::WhenStmt::docStrings>(ref<STRING>),
                         lit<"+">) +
             option(assign<&ast::WhenStmt::annotation>(ref<Annotation>))> {};

  struct Statement
      : rule<create<ast::AssignStmt>(
                 assign<&ast::AssignStmt::target>(ref<ComponentReference>) +
                 lit<":="> +
                 assign<&ast::AssignStmt::value>(ref<Expression>) +
                 many(append<&ast::AssignStmt::docStrings>(ref<STRING>),
                             lit<"+">) +
                 option(assign<&ast::AssignStmt::annotation>(
                     ref<Annotation>)))  //
             | create<ast::CallStmt>(
                   assign<&ast::CallStmt::callee>(ref<ComponentReference>) +
                   lit<"("> +
                   option(assign<&ast::CallStmt::args>(
                       ref<FunctionArguments>)) +
                   lit<")"> +
                   many(append<&ast::CallStmt::docStrings>(ref<STRING>),
                               lit<"+">) +
                   option(assign<&ast::CallStmt::annotation>(
                       ref<Annotation>)))  //
             | create<ast::MultiAssignStmt>(
                   lit<"("> +
                   append<&ast::MultiAssignStmt::targets>(
                       ref<Expression> | create<ast::OmittedOutput>) +
                   many(lit<","> +
                        append<&ast::MultiAssignStmt::targets>(
                            ref<Expression> | create<ast::OmittedOutput>)) +
                   lit<")"> + lit<":="> +
                   assign<&ast::MultiAssignStmt::callee>(
                       ref<ComponentReference>) +
                   lit<"("> +
                   option(assign<&ast::MultiAssignStmt::args>(
                       ref<FunctionArguments>)) +
                   lit<")"> +
                   many(
                       append<&ast::MultiAssignStmt::docStrings>(ref<STRING>),
                       lit<"+">) +
                   option(assign<&ast::MultiAssignStmt::annotation>(
                       ref<Annotation>)))  //
             | create<ast::BreakStmt>(
                   kw<"break"> +
                   many(append<&ast::BreakStmt::docStrings>(ref<STRING>),
                               lit<"+">) +
                   option(assign<&ast::BreakStmt::annotation>(
                       ref<Annotation>)))  //
             |
             create<ast::ReturnStmt>(
                 kw<"return"> +
                 many(append<&ast::ReturnStmt::docStrings>(ref<STRING>),
                             lit<"+">) +
                 option(assign<&ast::ReturnStmt::annotation>(
                     ref<Annotation>)))  //
             | ref<IfStmt>               //
             | ref<ForStmt>              //
             | ref<WhileStmt>            //
             | ref<WhenStmt>> {};

  // -- top level ----

  struct WithinClause
      : rule<kw<"within"> + option(assign<&ast::WithinClause::name>(ref<Name>)) +
             lit<";">> {};

  struct TopClassDef
      : rule<option(flag<&ast::TopClassDef::final_>(kw<"final">)) +
             assign<&ast::TopClassDef::def>(ref<ClassDefinition>) +
             lit<";">> {};

  struct StoredDefinition
      : rule<option(append<&ast::StoredDefinition::withins>(ref<WithinClause>)) +
             many(append<&ast::StoredDefinition::classes>(ref<TopClassDef>))> {
  };

  using entry = StoredDefinition;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, COMMENT>;
};

}  // namespace modelica
