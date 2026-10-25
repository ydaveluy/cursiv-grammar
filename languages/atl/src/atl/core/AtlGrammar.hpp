// The atl language: its grammar. ATL is a model-transformation language over
// an embedded OCL expression sub-language: a document is a module, a library
// or a query.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

#include "atl/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace atl {

// STRING covers both quote styles: the escape set (\b\t\n\f\r\"\'\\) is the
// same either way, only the excluded quote character differs.
inline constexpr auto unescape = [](std::string_view t,
                                     std::pmr::memory_resource* mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('\\') == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') {
      room[n++] = body[i];
      continue;
    }
    const char e = body[++i];
    switch (e) {
      case 'b':
        room[n++] = '\b';
        break;
      case 't':
        room[n++] = '\t';
        break;
      case 'n':
        room[n++] = '\n';
        break;
      case 'f':
        room[n++] = '\f';
        break;
      case 'r':
        room[n++] = '\r';
        break;
      default:
        room[n++] = e;  // Preserve quote and backslash escapes.
        break;
    }
  }
  return std::string_view(room.data(), n);
};
using Unescaped = cursiv::text_conv<unescape>;

// FLOAT carries an optional f/F/d/D suffix the value itself does not need.
inline constexpr auto parse_float = [](std::string_view t) {
  if (!t.empty()) {
    const char last = t.back();
    if (last == 'f' || last == 'F' || last == 'd' || last == 'D')
      t.remove_suffix(1);
  }
  double v = 0.0;
  std::from_chars(t.data(), t.data() + t.size(), v);
  return v;
};
using FloatConv = cursiv::text_conv<parse_float>;

struct Atl : cursiv::GrammarSpec {
  // Past the default word class so a keyword's boundary guard also refuses
  // `$`/`_`: ATL identifiers admit both.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(s)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"--"> + many(cr<R"(^\r\n)">)> {};

  // A name opens on a letter, `_` or `$`, never a digit: a digit head would
  // tie with INTEGER/FLOAT at the same length.
  struct ID : terminal<cr<"a-zA-Z_$"> + many(cr<"a-zA-Z0-9_$">)> {};

  static constexpr auto Escape = lit<"\\"> + cr<R"(btnfr"'\\)">;
  struct STRING
      : terminal<(lit<"\""> + many(Escape | cr<R"(^"\\)">) + lit<"\"">)  //
                     | (lit<"'"> + many(Escape | cr<R"(^'\\)">) + lit<"'">),
                 Unescaped> {};
  static constexpr auto NameTok = ref<STRING> | ref<ID>;

  // '0', or a nonzero digit followed by more digits -- a leading zero
  // belongs only to FLOAT.
  struct INTEGER
      : terminal<lit<"0"> | (cr<"1-9"> + many(d)), std::int64_t> {};

  static constexpr auto FDigits = some(d);
  static constexpr auto FExp = cr<"eE"> + option(cr<"+-">) + FDigits;
  static constexpr auto FSuffix = cr<"fFdD">;
  struct FLOAT
      : terminal<(FDigits + lit<"."> + option(FDigits) + option(FExp) +
                  option(FSuffix))  //
                     | (lit<"."> + FDigits + option(FExp) +
                        option(FSuffix))                             //
                     | (FDigits + FExp + option(FSuffix))            //
                     | (FDigits + FSuffix),
                 FloatConv> {};

  struct Expression;
  struct BinaryExprRule;
  struct Unary;
  struct Navigation;
  struct Primary;
  struct IfExprRule;
  struct LetExprRule;
  struct OclTypeRule;
  struct VarDeclRule;
  struct TuplePartRule;
  struct MapEntryRule;
  struct TupleTypeAttrRule;
  struct ParameterRule;
  struct InPatternElemRule;
  struct InPatternRule;
  struct BindingRule;
  struct OutPatternElemRule;
  struct OutPatternRule;
  struct StatementRule;
  struct ActionBlockRule;
  struct HelperRule;
  struct MatchedRuleRule;
  struct CalledRuleRule;
  struct ModuleElementRule;

  // --- types (oclType) ---

  struct TupleTypeAttrRule
      : rule<create<ast::TupleAttr> + assign<&ast::TupleAttr::name>(ref<ID>) +
             lit<":"> +
             assign<&ast::TupleAttr::type>(ref<OclTypeRule>)> {};

  struct OclTypeRule
      : rule<create<ast::ModelType>(
                 assign<&ast::ModelType::modelName>(ref<ID>) + lit<"!"> +
                 assign<&ast::ModelType::className>(NameTok))  //
             | create<ast::AnyType>(kw<"OclAny">)              //
             | create<ast::MetaType>(kw<"OclType">)            //
             | create<ast::TupleTypeRef>(
                   kw<"TupleType"> + lit<"("> +
                   many(append<&ast::TupleTypeRef::attrs>(
                            ref<TupleTypeAttrRule>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::MapTypeRef>(
                   kw<"Map"> + lit<"("> +
                   assign<&ast::MapTypeRef::keyType>(ref<OclTypeRule>) +
                   lit<","> +
                   assign<&ast::MapTypeRef::valueType>(ref<OclTypeRule>) +
                   lit<")">)  //
             | create<ast::PrimType>(assign<&ast::PrimType::keyword>(
                   kw<"Integer"> | kw<"Real"> | kw<"Boolean"> |
                   kw<"String">))  //
             | create<ast::CollTypeRef>(
                   assign<&ast::CollTypeRef::keyword>(
                       kw<"Bag"> | kw<"Set"> | kw<"OrderedSet"> |
                       kw<"Sequence"> | kw<"Collection">) +
                   lit<"("> +
                   assign<&ast::CollTypeRef::elementType>(ref<OclTypeRule>) +
                   lit<")">)> {};

  // --- expressions ---

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            kw<"if"> + assign<&ast::IfExpr::cond>(ref<Expression>) +
            kw<"then"> + assign<&ast::IfExpr::thenExpr>(ref<Expression>) +
            kw<"else"> + assign<&ast::IfExpr::elseExpr>(ref<Expression>) +
            kw<"endif">)> {};

  // Shared by `let`, a rule's `using` block and a tuple part; only the type
  // annotation's optionality differs (VarDeclRule requires it).
  struct VarDeclRule
      : rule<create<ast::VarDecl> + assign<&ast::VarDecl::name>(ref<ID>) +
             lit<":"> + assign<&ast::VarDecl::type>(ref<OclTypeRule>) +
             lit<"="> + assign<&ast::VarDecl::value>(ref<Expression>)> {};

  struct TuplePartRule
      : rule<create<ast::VarDecl> + assign<&ast::VarDecl::name>(ref<ID>) +
             option(lit<":"> +
                    assign<&ast::VarDecl::type>(ref<OclTypeRule>)) +
             lit<"="> + assign<&ast::VarDecl::value>(ref<Expression>)> {};

  struct LetExprRule
      : rule<create<ast::LetExpr>(
            kw<"let"> + assign<&ast::LetExpr::binding>(ref<VarDeclRule>) +
            kw<"in"> + assign<&ast::LetExpr::body>(ref<Expression>))> {};

  struct MapEntryRule
      : rule<create<ast::MapEntry> + lit<"("> +
             assign<&ast::MapEntry::key>(ref<Expression>) + lit<","> +
             assign<&ast::MapEntry::value>(ref<Expression>) + lit<")">> {};

  struct Primary
      : rule<create<ast::UndefinedLit>(kw<"OclUndefined">)  //
             | create<ast::BoolLit>(
                   assign<&ast::BoolLit::value>(kw<"true"> | kw<"false">))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::value>(ref<FLOAT>))  //
             | create<ast::IntLit>(
                   assign<&ast::IntLit::value>(ref<INTEGER>))  //
             | create<ast::StringLit>(
                   assign<&ast::StringLit::value>(ref<STRING>))  //
             | ref<IfExprRule>                                   //
             | create<ast::SuperExpr>(kw<"super">)                //
             | create<ast::EnumLit>(lit<"#"> +
                                    assign<&ast::EnumLit::name>(ref<ID>))  //
             | create<ast::SetLit>(
                   assign<&ast::SetLit::kind>(kw<"Bag"> | kw<"Set"> |
                                              kw<"OrderedSet"> |
                                              kw<"Sequence">) +
                   lit<"{"> +
                   many(append<&ast::SetLit::items>(ref<Expression>),
                        lit<",">) +
                   lit<"}">)  //
             | create<ast::MapLit>(
                   kw<"Map"> + lit<"{"> +
                   many(append<&ast::MapLit::entries>(ref<MapEntryRule>),
                        lit<",">) +
                   lit<"}">)  //
             | create<ast::TupleLit>(
                   kw<"Tuple"> + lit<"{"> +
                   many(append<&ast::TupleLit::parts>(ref<TuplePartRule>),
                        lit<",">) +
                   lit<"}">)             //
             | ref<OclTypeRule>          //
             | create<ast::NameRef>(
                   assign<&ast::NameRef::name>(ref<ID>))  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  // One postfix chain for '.'/'->': a bare call is one shape whichever
  // punctuation opens it, told apart by `arrow`.
  struct Navigation
      : rule<ref<Primary> +
             many(nest<ast::MethodCall, &ast::MethodCall::object>(
                      (lit<"."> | flag<&ast::MethodCall::arrow>(
                                      lit<"->">)) +
                      assign<&ast::MethodCall::name>(ref<ID>) + lit<"("> +
                      many(append<&ast::MethodCall::args>(ref<Expression>),
                           lit<",">) +
                      lit<")">)  //
                  | nest<ast::IterateCall, &ast::IterateCall::object>(
                        lit<"->"> + kw<"iterate"> + lit<"("> +
                        some(append<&ast::IterateCall::iterators>(ref<ID>),
                             lit<",">) +
                        lit<";"> +
                        assign<&ast::IterateCall::accum>(ref<VarDeclRule>) +
                        lit<"|"> +
                        assign<&ast::IterateCall::body>(ref<Expression>) +
                        lit<")">)  //
                  | nest<ast::IteratorCall, &ast::IteratorCall::object>(
                        lit<"->"> + assign<&ast::IteratorCall::name>(ref<ID>) +
                        lit<"("> +
                        some(append<&ast::IteratorCall::iterators>(ref<ID>),
                             lit<",">) +
                        lit<"|"> +
                        assign<&ast::IteratorCall::body>(ref<Expression>) +
                        lit<")">)  //
                  | nest<ast::MemberAccess, &ast::MemberAccess::object>(
                        lit<"."> +
                        assign<&ast::MemberAccess::name>(ref<ID>)))> {};

  // 'not'/'-' bind to a navigation chain alone, not to another unary:
  // `not not x` is refused.
  struct Unary
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(kw<"not"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Navigation>))  //
             | ref<Navigation>> {};

  // Eight precedence levels, tightest to loosest: relational binds tighter
  // than equality, and "and" binds tighter than "or" tighter than "xor"
  // tighter than "implies" (OCL 2.4 Sec. 7.4.8).
  struct BinaryExprRule
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"*"> | lit<"/"> | kw<"div"> | kw<"mod">) >>
               left(lit<"+"> | lit<"-">) >>
               left(lit<"<="> | lit<">="> | lit<"<"> | lit<">">) >>
               left(lit<"<>"> | lit<"=">) >> left(kw<"and">) >>
               left(kw<"or">) >> left(kw<"xor">) >> left(kw<"implies">))> {};

  struct Expression : rule<ref<LetExprRule> | ref<BinaryExprRule>> {};

  // --- module elements ---

  struct ParameterRule
      : rule<create<ast::Parameter> +
             assign<&ast::Parameter::name>(ref<ID>) + lit<":"> +
             assign<&ast::Parameter::type>(ref<OclTypeRule>)> {};

  struct OperationRule
      : rule<create<ast::Operation> + assign<&ast::Operation::name>(ref<ID>) +
             lit<"("> +
             many(append<&ast::Operation::params>(ref<ParameterRule>),
                  lit<",">) +
             lit<")"> + lit<":"> +
             assign<&ast::Operation::returnType>(ref<OclTypeRule>) +
             lit<"="> +
             assign<&ast::Operation::body>(ref<Expression>)> {};

  struct HelperRule
      : rule<create<ast::Helper> + kw<"helper"> +
             option(kw<"context"> +
                    assign<&ast::Helper::context>(ref<OclTypeRule>)) +
             kw<"def"> + lit<":"> +
             assign<&ast::Helper::feature>(ref<OperationRule> |
                                           ref<VarDeclRule>) +
             lit<";">> {};

  struct InPatternElemRule
      : rule<create<ast::InPatternElem> +
             assign<&ast::InPatternElem::name>(ref<ID>) + lit<":"> +
             assign<&ast::InPatternElem::type>(ref<OclTypeRule>) +
             option(kw<"in"> +
                    some(append<&ast::InPatternElem::inModels>(ref<ID>),
                         lit<",">))> {};

  struct InPatternRule
      : rule<create<ast::InPattern> + kw<"from"> +
             some(append<&ast::InPattern::elements>(ref<InPatternElemRule>),
                  lit<",">) +
             option(lit<"("> +
                    assign<&ast::InPattern::filter>(ref<Expression>) +
                    lit<")">)> {};

  struct BindingRule
      : rule<create<ast::Binding> + assign<&ast::Binding::name>(ref<ID>) +
             lit<"<-"> + assign<&ast::Binding::value>(ref<Expression>)> {};

  struct OutPatternElemRule
      : rule<create<ast::OutPatternElem> +
             assign<&ast::OutPatternElem::name>(ref<ID>) + lit<":"> +
             ((flag<&ast::OutPatternElem::isForEach>(kw<"distinct">) +
               assign<&ast::OutPatternElem::type>(ref<OclTypeRule>) +
               kw<"foreach"> + lit<"("> +
               assign<&ast::OutPatternElem::iterVar>(ref<ID>) + kw<"in"> +
               assign<&ast::OutPatternElem::source>(ref<Expression>) +
               lit<")">)  //
              | (assign<&ast::OutPatternElem::type>(ref<OclTypeRule>) +
                 option(kw<"in"> + assign<&ast::OutPatternElem::inModel>(
                                       ref<ID>)))) +
             option(kw<"mapsTo"> +
                    assign<&ast::OutPatternElem::mapsTo>(ref<ID>)) +
             option(lit<"("> +
                    many(append<&ast::OutPatternElem::bindings>(
                             ref<BindingRule>),
                         lit<",">) +
                    lit<")">)> {};

  struct OutPatternRule
      : rule<create<ast::OutPattern> + kw<"to"> +
             some(append<&ast::OutPattern::elements>(ref<OutPatternElemRule>),
                  lit<",">)> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt> + kw<"if"> + lit<"("> +
             assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
             ((lit<"{"> +
               many(append<&ast::IfStmt::thenStmts>(ref<StatementRule>)) +
               lit<"}">)  //
              | append<&ast::IfStmt::thenStmts>(ref<StatementRule>)) +
             option(
                 kw<"else"> +
                 ((lit<"{"> +
                   many(
                       append<&ast::IfStmt::elseStmts>(ref<StatementRule>)) +
                   lit<"}">)  //
                  | append<&ast::IfStmt::elseStmts>(ref<StatementRule>)))> {};

  struct ForStmtRule
      : rule<create<ast::ForStmt> + kw<"for"> + lit<"("> +
             assign<&ast::ForStmt::iterVar>(ref<ID>) + kw<"in"> +
             assign<&ast::ForStmt::source>(ref<Expression>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::ForStmt::body>(ref<StatementRule>)) +
             lit<"}">> {};

  struct BindStmtRule
      : rule<create<ast::BindStmt>(
            assign<&ast::BindStmt::target>(ref<Expression>) + lit<"<-"> +
            assign<&ast::BindStmt::value>(ref<Expression>) + lit<";">)> {};

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">)> {};

  struct StatementRule
      : rule<ref<IfStmtRule> | ref<ForStmtRule> | ref<BindStmtRule> |
             ref<ExprStmtRule>> {};

  struct ActionBlockRule
      : rule<create<ast::ActionBlock> + kw<"do"> + lit<"{"> +
             many(append<&ast::ActionBlock::statements>(ref<StatementRule>)) +
             lit<"}">> {};

  struct MatchedRuleRule
      : rule<create<ast::MatchedRule> +
            many(
                   flag<&ast::MatchedRule::isUnique>(
                     append<&ast::MatchedRule::modifiers>(kw<"unique">)) |
                 flag<&ast::MatchedRule::isLazy>(
                     append<&ast::MatchedRule::modifiers>(kw<"lazy">)) |
                 flag<&ast::MatchedRule::isAbstract>(
                     append<&ast::MatchedRule::modifiers>(kw<"abstract">)) |
                 flag<&ast::MatchedRule::isRefining>(
                     append<&ast::MatchedRule::modifiers>(kw<"refining">)) |
                 flag<&ast::MatchedRule::isNodefault>(
                     append<&ast::MatchedRule::modifiers>(
                     kw<"nodefault">))) +
             kw<"rule"> + assign<&ast::MatchedRule::name>(ref<ID>) +
             option(kw<"extends"> +
                    assign<&ast::MatchedRule::extendsRule>(ref<ID>)) +
             lit<"{"> +
             assign<&ast::MatchedRule::inPattern>(ref<InPatternRule>) +
             option(kw<"using"> + lit<"{"> +
                    many(append<&ast::MatchedRule::variables>(
                             ref<VarDeclRule>) +
                         lit<";">) +
                    lit<"}">) +
             option(
                 assign<&ast::MatchedRule::outPattern>(ref<OutPatternRule>)) +
             option(
                 assign<&ast::MatchedRule::action>(ref<ActionBlockRule>)) +
             lit<"}">> {};

  struct CalledRuleRule
      : rule<create<ast::CalledRule> +
             option(flag<&ast::CalledRule::isEntrypoint>(
                 kw<"entrypoint">)) +
             option(
                 flag<&ast::CalledRule::isEndpoint>(kw<"endpoint">)) +
             kw<"rule"> + assign<&ast::CalledRule::name>(ref<ID>) +
             lit<"("> +
             many(append<&ast::CalledRule::params>(ref<ParameterRule>),
                  lit<",">) +
             lit<")"> + lit<"{"> +
             option(kw<"using"> + lit<"{"> +
                    many(append<&ast::CalledRule::variables>(
                             ref<VarDeclRule>) +
                         lit<";">) +
                    lit<"}">) +
             option(
                 assign<&ast::CalledRule::outPattern>(ref<OutPatternRule>)) +
             option(
                 assign<&ast::CalledRule::action>(ref<ActionBlockRule>)) +
             lit<"}">> {};

  struct ModuleElementRule
      : rule<ref<HelperRule> | ref<MatchedRuleRule> | ref<CalledRuleRule>> {};

  struct OclModelRule
      : rule<create<ast::OclModel> + assign<&ast::OclModel::alias>(ref<ID>) +
             lit<":"> +
             assign<&ast::OclModel::modelName>(ref<ID>)> {};

  struct ModuleRule
      : rule<create<ast::Module> + kw<"module"> +
             assign<&ast::Module::name>(NameTok) + lit<";"> + kw<"create"> +
             some(append<&ast::Module::targetModels>(ref<OclModelRule>),
                  lit<",">) +
             assign<&ast::Module::mode>(kw<"refining"> | kw<"from">) +
             some(append<&ast::Module::sourceModels>(ref<OclModelRule>),
                  lit<",">) +
             lit<";"> +
             many(kw<"uses"> +
                  append<&ast::Module::uses>(ref<STRING>) + lit<";">) +
             many(append<&ast::Module::elements>(ref<ModuleElementRule>))> {
  };

  struct LibraryRule
      : rule<create<ast::Library> + kw<"library"> +
             assign<&ast::Library::name>(NameTok) + lit<";"> +
             many(kw<"uses"> +
                  append<&ast::Library::uses>(ref<STRING>) + lit<";">) +
             many(append<&ast::Library::helpers>(ref<HelperRule>))> {};

  struct QueryRule
      : rule<create<ast::Query> + kw<"query"> +
             assign<&ast::Query::name>(NameTok) + lit<"="> +
             assign<&ast::Query::body>(ref<Expression>) + lit<";"> +
             many(kw<"uses"> +
                  append<&ast::Query::uses>(ref<STRING>) + lit<";">) +
             many(append<&ast::Query::helpers>(ref<HelperRule>))> {};

  struct Unit : rule<ref<ModuleRule> | ref<LibraryRule> | ref<QueryRule>> {};

  using entry = Unit;
  using ignore = WS;
  using hidden = list<ML_COMMENT, LINE_COMMENT>;
};

}  // namespace atl
