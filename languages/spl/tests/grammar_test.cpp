#include "audit_regressions.hpp"
// SPL end to end: search-pipeline shape (commands, operations, subqueries),
// the arithmetic/value expression family, and the upstream corpus -- every
// document in example/ parses clean and yields the right root type.

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/Check.hpp"
#include "spl/core/ast.hpp"
#include "spl/generated/Spl/AstReflection.hpp"
#include "spl/generated/Spl/Module.hpp"

namespace {

namespace ast = spl::ast;
namespace fs = std::filesystem;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      spl::generated::Spl::create_parser();
  return *p;
}

const cursiv::AstReflection& reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      spl::generated::Spl::create_ast_reflection();
  return *r;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && reflection().isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T*>(n)
             : nullptr;
}

bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

cursiv::ParseResult build(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  return r;
}

void accepted_texts() {
  CHECK(accepts("index=web sourcetype=test"));
  CHECK(accepts("search index=web sourcetype=test"));
  CHECK(accepts("index=web status=404"));
  CHECK(accepts("index=web (error OR warning)"));
  CHECK(accepts("index=web NOT status=500"));
  CHECK(accepts("index=web AND status=200"));
  CHECK(accepts("field_name IN (\"a\", \"b\", \"c\")"));
  CHECK(accepts("field_name LIKE \"%pattern%\""));
  CHECK(accepts("search like(field_name, \"%pattern%\")"));
  CHECK(accepts("index=web | timechart count by status"));
  CHECK(accepts("index=web | stats count by status host"));
  CHECK(accepts("index=web | lookup t user OUTPUTNEW user_info"));
  CHECK(accepts("index=web | lookup t a b OUTPUT x, y"));
  CHECK(accepts("index=web | lookup t a b c OUTPUTNEW x, y, z"));
  CHECK(accepts("index=web AS w"));
  CHECK(accepts("index=web earliest=-24h@h latest=@h"));
  CHECK(accepts("index=web | eval x=1+2*3"));
  CHECK(accepts("index=web | eval x=(1+2)*3"));
  CHECK(accepts("index=web | eval x=2^3"));
  CHECK(accepts("index=web | eval x=now()"));
  CHECK(accepts("index=web | eval x=if(isnum(a), \"y\", \"n\")"));
  CHECK(accepts("index=*"));
  CHECK(accepts("index=web productID=*.html"));
  CHECK(accepts("index=web productID=id*"));            // trailing star, no leading one
  CHECK(accepts("index=web productID=*(1+2)*"));         // a star wraps a full expression
  CHECK(accepts("index=web sourcetype=/var/log"));
  CHECK(accepts("index=web sourcetype=/var/log/messages"));
  // `in`/`like` also read as eval functions where a call is expected, beside
  // their keyword reading elsewhere.
  CHECK(accepts("index=web | eval x=if(in(a,1,2,3), \"y\", \"n\")"));
  CHECK(accepts("index=web | eval x=if(like(a,\"%x%\"), \"y\", \"n\")"));
  CHECK(accepts("index=web | join type=inner user [ search index=users ]"));
  CHECK(accepts("index=web productID=* [ search index=users | fields user ]"));
  // Command and function words are also plain names.
  CHECK(accepts("index=web | eval count=1"));
  CHECK(accepts("index=web stats=1"));
}

void refused_texts() {
  CHECK(!accepts(""));
  CHECK(!accepts("|"));
  CHECK(!accepts("index=web AND"));
  CHECK(!accepts("index=web |"));
  CHECK(!accepts("index='unterminated"));
}

// Field values read back out of the AST: a converter that silently produces
// the wrong value passes every structural check alone.
void field_values() {
  cursiv::ParseResult r = build("source=\"error.log\" status=404");
  ast::Query* const q = r.value_as<ast::Query>();
  CHECK(q != nullptr && q->init != nullptr);
  if (q == nullptr || q->init == nullptr) return;
  CHECK(q->init->commandName.empty());
  CHECK(q->init->operations.size() == 2);
  if (q->init->operations.size() != 2) return;

  ast::Comparison* const c0 = as<ast::Comparison>(q->init->operations[0]);
  CHECK(c0 != nullptr && c0->field == "source" && c0->op == ast::CompareOp::eq);
  ast::StringLit* const v0 = as<ast::StringLit>(c0 != nullptr ? c0->value : nullptr);
  CHECK(v0 != nullptr && v0->value == "error.log");

  ast::Comparison* const c1 = as<ast::Comparison>(q->init->operations[1]);
  CHECK(c1 != nullptr && c1->field == "status" && c1->op == ast::CompareOp::eq);
  ast::NumberLit* const v1 = as<ast::NumberLit>(c1 != nullptr ? c1->value : nullptr);
  CHECK(v1 != nullptr && !v1->negative && v1->value == 404.0);

  cursiv::ParseResult p = build("index=web | timechart count by status");
  ast::Query* const pq = p.value_as<ast::Query>();
  CHECK(pq != nullptr && pq->pipeline.size() == 1);
  if (pq == nullptr || pq->pipeline.empty()) return;
  ast::NextCommand* const stage = pq->pipeline[0];
  CHECK(stage->commandName == "timechart");
  CHECK(stage->operations.size() == 2);
  ast::ByClause* const by = as<ast::ByClause>(
      stage->operations.size() == 2 ? stage->operations[1] : nullptr);
  CHECK(by != nullptr && by->fields.size() == 1 && by->fields[0] == "status");

  cursiv::ParseResult g = build("index=web productID=*.html");
  ast::Query* const gq = g.value_as<ast::Query>();
  CHECK(gq != nullptr && gq->init != nullptr && gq->init->operations.size() == 2);
  ast::Comparison* const gc = as<ast::Comparison>(
      gq != nullptr && gq->init->operations.size() == 2 ? gq->init->operations[1]
                                                        : nullptr);
  ast::Glob* const glob = as<ast::Glob>(gc != nullptr ? gc->value : nullptr);
  CHECK(glob != nullptr && glob->leadingStar && !glob->trailingStar);
  // A name may start with `.`: the segment after the star keeps its leading
  // dot.
  ast::NameExpr* const globName = as<ast::NameExpr>(glob != nullptr ? glob->inner : nullptr);
  CHECK(globName != nullptr && globName->name == ".html");

  cursiv::ParseResult j = build("index=web | join user [ search index=users ]");
  ast::Query* const jq = j.value_as<ast::Query>();
  CHECK(jq != nullptr && jq->pipeline.size() == 1);
  ast::NextCommand* const jstage = jq != nullptr ? jq->pipeline[0] : nullptr;
  CHECK(jstage != nullptr && jstage->subquery != nullptr);
  CHECK(jstage != nullptr && jstage->subquery != nullptr &&
        jstage->subquery->init != nullptr &&
        jstage->subquery->init->commandName == "search");
}

// `*`, `/` and `%` are one precedence tier, left-associative: `2/3*4` is
// `(2/3)*4`, not `2/(3*4)`.
void arithmetic_precedence() {
  cursiv::ParseResult r = build("search x=2/3*4");
  ast::Query* const q = r.value_as<ast::Query>();
  CHECK(q != nullptr && q->init != nullptr && q->init->operations.size() == 1);
  if (q == nullptr || q->init == nullptr || q->init->operations.size() != 1)
    return;
  ast::Comparison* const c = as<ast::Comparison>(q->init->operations[0]);
  ast::ArithExpr* const mul = as<ast::ArithExpr>(c != nullptr ? c->value : nullptr);
  CHECK(mul != nullptr && mul->op == ast::ArithOp::multiply);
  ast::ArithExpr* const div =
      as<ast::ArithExpr>(mul != nullptr ? mul->left : nullptr);
  CHECK(div != nullptr && div->op == ast::ArithOp::divide);
}

std::vector<fs::path> corpus_files() {
  std::vector<fs::path> files;
  for (const fs::directory_entry& e :
       fs::recursive_directory_iterator(fs::path(CURSIV_EXAMPLE_DIR) / "example")) {
    if (e.is_regular_file()) files.push_back(e.path());
  }
  std::sort(files.begin(), files.end());
  return files;
}

// Every document of the upstream corpus: a clean parse, no diagnostics, the
// right root type.
void corpus() {
  const std::vector<fs::path> files = corpus_files();
  CHECK(!files.empty());
  for (const fs::path& file : files) {
    const std::string src = cursiv::test::read_file(file.string());
    const cursiv::ParseResult r = engine().parse(src);
    const bool clean = r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    if (!clean) {
      std::printf("FAIL %s: not clean (accepted=%d recovered=%d diagnostics=%zu)\n",
                  file.string().c_str(), r.accepted, r.cst().recovered(),
                  r.diagnostics.size());
      ++cursiv::test::failures;
      continue;
    }
    CHECK(r.value_as<ast::Query>() != nullptr);
  }
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  accepted_texts();
  refused_texts();
  field_values();
  arithmetic_precedence();
  corpus();
  return cursiv::test::report("spl_grammar_test");
}
