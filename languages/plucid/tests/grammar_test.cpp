// The smoke test of the language: no upstream corpus ships with plucid,
// so the documents under tests/ exercise represented syntax and AST values.
// Native comparison evidence and coverage limits live in the audit report.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <variant>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "plucid/core/CoreServices.hpp"
#include "plucid/core/ast.hpp"

namespace {

namespace ast = plucid::ast;

int fail(const char* what) {
  std::fprintf(stderr, "plucid: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

}  // namespace

int main() {
  const std::string dir = CURSIV_TEST_DIR;
  const char* const names[] = {"arithmetic.plu", "text.plu", "flow.plu",
                               "precedence.plu"};

  // Every string_view in a tree points into the text that produced it: keep
  // all four alive for the whole function.
  std::vector<std::string> texts;
  for (const char* const name : names)
    texts.push_back(read_file(dir + "/" + name));
  for (const std::string& t : texts)
    if (t.empty()) return fail("a test document is not readable");

  const auto language = cursiv::create_language<
      plucid::PlucidCoreServices, plucid::PlucidSharedCoreServices,
      plucid::install_core_module>();
  for(const auto source:{"~5", "a ^ b", "'plain string'", "filter(a,b,c)", "substr(x,y,z)",
      "if p then a elseif q then b elseif r then c else d fi", "case x of 1:2; default:3; end",
      "x where // comment\n x=1; end"}) {
    const auto r=language.language.parser->parse(source);
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty())return fail("source-backed native form rejected");
  }
  for(const auto source:{"filter a,b,c", "substr x,y,z", "x where if=1; end", "if p then a elseif q then b endif"}) {
    const auto r=language.language.parser->parse(source);
    if(r.accepted&&!r.cst().recovered()&&r.diagnostics.empty())return fail("native-invalid keyword/transcription form accepted");
  }

  {
    const auto r=language.language.parser->parse(
        "[% ~5, 'text', filter(a,b,c), substr(x,y,z), "
        "if p then a elseif q then b elseif r then c else d fi %]");
    const auto* p=r.value_as<ast::Program>();
    const auto* list=p?as<ast::ListExpr>(p->expression):nullptr;
    if(!r.accepted||r.cst().recovered()||!r.diagnostics.empty()||!list||list->items.size()!=5)
      return fail("native forms built no complete list");
    const auto* number=as<ast::IntegerLit>(list->items[0]);
    const auto* text=as<ast::StringLit>(list->items[1]);
    const auto* filter=as<ast::FilterExpr>(list->items[2]);
    const auto* substr=as<ast::SubstrExpr>(list->items[3]);
    const auto* conditional=as<ast::IfExpr>(list->items[4]);
    const auto* first=conditional?as<ast::ElseifTail>(conditional->tail):nullptr;
    const auto* second=first?as<ast::ElseifTail>(first->tail):nullptr;
    const auto* last=second?as<ast::ElseTail>(second->tail):nullptr;
    const auto* result=last?as<ast::Identifier>(last->value):nullptr;
    if(!number||number->value!=-5||!text||text->value!="text"||
       !filter||filter->args.size()!=3||!substr||substr->args.size()!=3||
       !result||result->name!="d")
      return fail("native literals, argument lists or recursive elseif AST was lost");
  }

  {
    for (const std::string source : {std::string("9223372036854775808"), std::string("˜9223372036854775809"),
                                     std::string(400, '9') + ".0"}) {
      const auto result = language.language.parser->parse(source);
      if (!result.accepted || result.cst().recovered() ||
          result.diagnostics.size() != 1 ||
          result.diagnostics[0].kind != cursiv::DiagnosticKind::conversion_error)
        return fail("out-of-range literal must report a conversion error");
    }
    const std::string source = "˜9223372036854775808";
    const auto result = language.language.parser->parse(source);
    const auto *program = result.value_as<ast::Program>();
    const auto *integer = program ? cursiv::ast_cast<ast::IntegerLit>(program->expression) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !integer || integer->value != std::numeric_limits<std::int64_t>::min())
      return fail("minimum signed integer must remain representable");
  }


  std::vector<cursiv::ParseResult> results;
  for (const std::string& t : texts)
    results.push_back(language.language.parser->parse(t));

  for (const cursiv::ParseResult& r : results) {
    if (r.cst().recovered()) return fail("an example does not parse cleanly");
    if (!r.diagnostics.empty())
      return fail("an example carries a diagnostic");
    if (as<ast::Program>(r.value) == nullptr)
      return fail("an example built no program");
  }

  // arithmetic.plu: every numeric infix operator and prefix operator, plus
  // the sign-parity fold of a leading `˜`.
  {
    ast::Program* const program = as<ast::Program>(results[0].value);
    ast::ListExpr* const root = as<ast::ListExpr>(program->expression);
    if (root == nullptr || root->items.size() != 9)
      return fail("arithmetic.plu: wrong item count");

    ast::Expression* leaf = root->items[0];
    while (ast::BinaryExpr* const bin = as<ast::BinaryExpr>(leaf))
      leaf = bin->left;
    ast::IntegerLit* const five = as<ast::IntegerLit>(leaf);
    if (five == nullptr || five->value != -5)
      return fail("arithmetic.plu: a leading tilde did not negate");

    ast::PrefixExpr* const log10Call = as<ast::PrefixExpr>(root->items[6]);
    ast::IntegerLit* const log10Arg =
        log10Call != nullptr ? as<ast::IntegerLit>(log10Call->operand)
                             : nullptr;
    if (log10Call == nullptr || log10Call->op != ast::PrefixOp::log10 ||
        log10Arg == nullptr || log10Arg->value != 100)
      return fail("arithmetic.plu: log10 misread");

    ast::PrefixExpr* const logCall = as<ast::PrefixExpr>(root->items[7]);
    ast::IntegerLit* const logArg =
        logCall != nullptr ? as<ast::IntegerLit>(logCall->operand) : nullptr;
    if (logCall == nullptr || logCall->op != ast::PrefixOp::log ||
        logArg == nullptr || logArg->value != 10)
      return fail("arithmetic.plu: log ran into log10's word boundary");
  }

  // text.plu: quoted word content, a bare string, nil, [], and a nested
  // populated list.
  {
    ast::Program* const program = as<ast::Program>(results[1].value);
    ast::ListExpr* const root = as<ast::ListExpr>(program->expression);
    if (root == nullptr || root->items.size() != 10)
      return fail("text.plu: wrong item count");

    const char* const words[] = {"abc123", "+-*", "(", ".", ","};
    for (std::size_t i = 0; i < 5; ++i) {
      ast::WordLit* const w = as<ast::WordLit>(root->items[i]);
      if (w == nullptr || w->value != words[i])
        return fail("text.plu: a quoted word lost its content");
    }
    ast::WordLit* const quoteChar = as<ast::WordLit>(root->items[5]);
    if (quoteChar == nullptr || quoteChar->value != "\"")
      return fail("text.plu: the embedded quote character was not kept");

    ast::StringLit* const str = as<ast::StringLit>(root->items[6]);
    if (str == nullptr || str->value != "a string with spaces!")
      return fail("text.plu: the string constant lost its content");

    if (as<ast::NilLit>(root->items[7]) == nullptr)
      return fail("text.plu: nil");
    ast::ListLit* const empty = as<ast::ListLit>(root->items[8]);
    if (empty == nullptr || !empty->elements.empty())
      return fail("text.plu: [] is not empty");

    ast::ListLit* const nested = as<ast::ListLit>(root->items[9]);
    if (nested == nullptr || nested->elements.size() != 10)
      return fail("text.plu: the nested list lost an element");
    ast::Expression* const* const firstElem =
        std::get_if<ast::Expression*>(&nested->elements[0]);
    ast::IntegerLit* const firstInt =
        firstElem != nullptr ? as<ast::IntegerLit>(*firstElem) : nullptr;
    if (firstInt == nullptr || firstInt->value != 1)
      return fail("text.plu: the nested list's first element is not 1");
  }

  // flow.plu: if/case/cond, filter/substr/length/arg, every remaining
  // prefix and infix operator, function_call, error/eod, a bare identifier.
  {
    ast::Program* const program = as<ast::Program>(results[2].value);
    ast::ListExpr* const root = as<ast::ListExpr>(program->expression);
    if (root == nullptr || root->items.size() != 37)
      return fail("flow.plu: wrong item count");

    ast::IfExpr* const ifElse = as<ast::IfExpr>(root->items[0]);
    ast::ElseTail* const elseTail =
        ifElse != nullptr ? as<ast::ElseTail>(ifElse->tail)
                          : nullptr;
    if (ifElse == nullptr || elseTail == nullptr)
      return fail("flow.plu: if/then/else/fi");

    ast::IfExpr* const ifElseif = as<ast::IfExpr>(root->items[1]);
    ast::ElseifTail* const elseifTail =
        ifElseif != nullptr ? as<ast::ElseifTail>(ifElseif->tail)
                            : nullptr;
    if (ifElseif == nullptr || elseifTail == nullptr)
      return fail("flow.plu: if/then/elseif/then/else/fi");

    ast::CaseExpr* const caseExpr = as<ast::CaseExpr>(root->items[2]);
    if (caseExpr == nullptr || caseExpr->cases.size() != 2 ||
        as<ast::IntegerLit>(caseExpr->default_) == nullptr)
      return fail("flow.plu: case/of/end");

    ast::CondExpr* const condExpr = as<ast::CondExpr>(root->items[3]);
    if (condExpr == nullptr || condExpr->cases.size() != 1 ||
        as<ast::IntegerLit>(condExpr->default_) == nullptr)
      return fail("flow.plu: cond/end");

    ast::FunctionCall* const call = as<ast::FunctionCall>(root->items[8]);
    if (call == nullptr || call->name != "foo" || call->args.size() != 3)
      return fail("flow.plu: function_call");

    if (as<ast::ErrorLit>(root->items[34]) == nullptr)
      return fail("flow.plu: error");
    if (as<ast::EodLit>(root->items[35]) == nullptr)
      return fail("flow.plu: eod");
    ast::Identifier* const bare = as<ast::Identifier>(root->items[36]);
    if (bare == nullptr || bare->name != "myvar")
      return fail("flow.plu: a bare identifier");
  }

  // precedence.plu: every adjacent pair of the pLucid precedence table
  // (Wadge & Ashcroft 1985, Appendix F.2) and its associativity, plus
  // grouping parens and a where-clause with both binding forms.
  {
    ast::Program* const program = as<ast::Program>(results[3].value);
    ast::ListExpr* const root = as<ast::ListExpr>(program->expression);
    if (root == nullptr || root->items.size() != 10)
      return fail("precedence.plu: wrong item count");

    ast::BinaryExpr* const p0 = as<ast::BinaryExpr>(root->items[0]);
    ast::BinaryExpr* const p0Right =
        p0 != nullptr ? as<ast::BinaryExpr>(p0->right) : nullptr;
    if (p0 == nullptr || p0->op != ast::InfixOp::add ||
        as<ast::IntegerLit>(p0->left) == nullptr || p0Right == nullptr ||
        p0Right->op != ast::InfixOp::multiply)
      return fail("precedence.plu: * does not bind tighter than +");

    ast::BinaryExpr* const p1 = as<ast::BinaryExpr>(root->items[1]);
    ast::BinaryExpr* const p1Left =
        p1 != nullptr ? as<ast::BinaryExpr>(p1->left) : nullptr;
    if (p1 == nullptr || p1->op != ast::InfixOp::power || p1Left == nullptr ||
        p1Left->op != ast::InfixOp::power ||
        as<ast::IntegerLit>(p1->right) == nullptr)
      return fail("precedence.plu: ** is not left-associative");

    ast::BinaryExpr* const p2 = as<ast::BinaryExpr>(root->items[2]);
    ast::BinaryExpr* const p2Right =
        p2 != nullptr ? as<ast::BinaryExpr>(p2->right) : nullptr;
    if (p2 == nullptr || p2->op != ast::InfixOp::concat ||
        as<ast::Identifier>(p2->left) == nullptr || p2Right == nullptr ||
        p2Right->op != ast::InfixOp::concat)
      return fail("precedence.plu: ^ is not right-associative");

    ast::BinaryExpr* const p3 = as<ast::BinaryExpr>(root->items[3]);
    ast::BinaryExpr* const p3Right =
        p3 != nullptr ? as<ast::BinaryExpr>(p3->right) : nullptr;
    if (p3 == nullptr || p3->op != ast::InfixOp::list_cons ||
        p3Right == nullptr || p3Right->op != ast::InfixOp::list_cons)
      return fail("precedence.plu: :: is not right-associative");

    ast::BinaryExpr* const p4 = as<ast::BinaryExpr>(root->items[4]);
    ast::PrefixExpr* const p4Left =
        p4 != nullptr ? as<ast::PrefixExpr>(p4->left) : nullptr;
    if (p4 == nullptr || p4->op != ast::InfixOp::logical_and ||
        p4Left == nullptr || p4Left->op != ast::PrefixOp::lognot)
      return fail("precedence.plu: not does not bind tighter than and");

    ast::BinaryExpr* const p5 = as<ast::BinaryExpr>(root->items[5]);
    ast::BinaryExpr* const p5Left =
        p5 != nullptr ? as<ast::BinaryExpr>(p5->left) : nullptr;
    if (p5 == nullptr || p5->op != ast::InfixOp::logical_or ||
        p5Left == nullptr || p5Left->op != ast::InfixOp::logical_and)
      return fail("precedence.plu: and does not bind tighter than or");

    ast::BinaryExpr* const p6 = as<ast::BinaryExpr>(root->items[6]);
    ast::BinaryExpr* const p6Left =
        p6 != nullptr ? as<ast::BinaryExpr>(p6->left) : nullptr;
    ast::BinaryExpr* const p6LeftLeft =
        p6Left != nullptr ? as<ast::BinaryExpr>(p6Left->left) : nullptr;
    if (p6 == nullptr || p6->op != ast::InfixOp::lt || p6Left == nullptr ||
        p6Left->op != ast::InfixOp::eq || p6LeftLeft == nullptr ||
        p6LeftLeft->op != ast::InfixOp::add)
      return fail("precedence.plu: relation does not chain outside +");

    ast::BinaryExpr* const p7 = as<ast::BinaryExpr>(root->items[7]);
    ast::GroupExpr* const p7Left =
        p7 != nullptr ? as<ast::GroupExpr>(p7->left) : nullptr;
    ast::BinaryExpr* const p7Inner =
        p7Left != nullptr ? as<ast::BinaryExpr>(p7Left->inner) : nullptr;
    if (p7 == nullptr || p7->op != ast::InfixOp::multiply ||
        p7Left == nullptr || p7Inner == nullptr ||
        p7Inner->op != ast::InfixOp::add)
      return fail("precedence.plu: (2 + 3) * 4 lost its grouping");

    ast::WhereExpr* const p8 = as<ast::WhereExpr>(root->items[8]);
    if (p8 == nullptr || as<ast::Identifier>(p8->subject) == nullptr ||
        !p8->decls.empty() || p8->definitions.size() != 2 ||
        p8->definitions[0]->name != "f" || p8->definitions[1]->name != "g")
      return fail("precedence.plu: the fby where-clause misparsed");
    ast::BinaryExpr* const fby0 = as<ast::BinaryExpr>(p8->definitions[0]->value);
    ast::BinaryExpr* const fby0Right =
        fby0 != nullptr ? as<ast::BinaryExpr>(fby0->right) : nullptr;
    if (fby0 == nullptr || fby0->op != ast::InfixOp::fby ||
        fby0Right == nullptr || fby0Right->op != ast::InfixOp::add)
      return fail("precedence.plu: fby does not bind looser than +");

    ast::WhereExpr* const p9 = as<ast::WhereExpr>(root->items[9]);
    if (p9 == nullptr || p9->decls.size() != 1 || p9->definitions.size() != 2)
      return fail("precedence.plu: the current/formals where-clause misparsed");
    if (p9->decls[0]->name != "w" ||
        as<ast::IntegerLit>(p9->decls[0]->value) == nullptr)
      return fail("precedence.plu: 'is current' misparsed");
    if (p9->definitions[0]->name != "scale" ||
        p9->definitions[0]->params.size() != 1 ||
        p9->definitions[0]->params[0] != "x")
      return fail("precedence.plu: a function definition's formals misparsed");
    ast::FunctionCall* const call =
        as<ast::FunctionCall>(p9->definitions[1]->value);
    if (call == nullptr || call->name != "scale" || call->args.size() != 1)
      return fail("precedence.plu: a where-bound function call misparsed");
  }

  std::puts("plucid: the examples parse");
  return 0;
}
