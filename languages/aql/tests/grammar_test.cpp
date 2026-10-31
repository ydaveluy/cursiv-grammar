// AQL end to end: every corpus document parses clean, a handful of
// deliberately unexercised constructs get documents of their own, and a
// few field values are read back out of the tree.

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "aql/core/ast.hpp"
#include "aql/generated/Aql/AstReflection.hpp"
#include "aql/generated/Aql/Module.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "upsert_filter.hpp"

namespace {
template <class T> std::vector<T *> operations(const aql::ast::ForOp *op) {
  std::vector<T *> result;
  for (const auto &item : op->operations)
    if (const auto *node = std::get_if<T *>(&item))
      result.push_back(*node);
  return result;
}

namespace ast = aql::ast;
namespace fs = std::filesystem;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      aql::generated::Aql::create_parser();
  return *p;
}

const cursiv::AstReflection& reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      aql::generated::Aql::create_ast_reflection();
  return *r;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && reflection().isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T*>(n)
             : nullptr;
}

// The text lives in the caller: every string_view the tree holds is a slice
// of it, so it must outlive every read of the tree, not just the parse.
cursiv::ParseResult build(const std::string& text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  return r;
}

std::string load(const char* name) {
  return cursiv::test::read_file(std::string(CURSIV_EXAMPLE_DIR) +
                                  "/example/" + name);
}

std::string load_test(const char* name) {
  return cursiv::test::read_file(std::string(CURSIV_TEST_DIR) + "/" + name);
}

// Every document under example/ and this port's own tests/*.aql: a clean
// parse, no diagnostics, a root that is one of the three query shapes.
void corpus() {
  std::vector<fs::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR "/example", CURSIV_TEST_DIR})
    for (const auto& entry : fs::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".aql")
        files.push_back(entry.path());
  CHECK(!files.empty());

  for (const fs::path& file : files) {
    const std::string text = cursiv::test::read_file(file.string());
    CHECK(!text.empty());
    const cursiv::ParseResult r = engine().parse(text);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) {
      std::printf("FAIL %s: accepted=%d recovered=%d diagnostics=%zu\n",
                  file.filename().string().c_str(),
                  static_cast<int>(r.accepted),
                  static_cast<int>(r.cst().recovered()), r.diagnostics.size());
      ++cursiv::test::failures;
      continue;
    }
    cursiv::AstNode* const root = r.value;
    const bool isQuery =
        root != nullptr &&
        reflection().isInstance(*root, &cursiv::ast_type_tag<ast::Query>);
    if (!isQuery) {
      std::printf("FAIL %s: root is not a Query\n",
                  file.filename().string().c_str());
      ++cursiv::test::failures;
    }
  }
}

// COLLECT INTO an object literal (collect_1.aql): the group key is a comma
// chain of assignments, INTO's expression is itself an assignment.
void collect_into_object() {
  const std::string src = load("collect_1.aql");
  cursiv::ParseResult r = build(src);
  ast::SimpleQuery* const q = as<ast::SimpleQuery>(r.value);
  CHECK(q != nullptr && q->forOps.size() == 1);
  if (q == nullptr || q->forOps.empty()) return;
  ast::ForOp* const op = q->forOps[0];
  CHECK(operations<ast::Collect>(op).size() == 1);
  if (operations<ast::Collect>(op).empty()) return;
  ast::Collect* const c = operations<ast::Collect>(op)[0];
  ast::BinaryExpr* const key = as<ast::BinaryExpr>(c->expr_);
  CHECK(key != nullptr && key->op == ast::BinOp::comma);
  ast::BinaryExpr* const into = as<ast::BinaryExpr>(c->into);
  CHECK(into != nullptr && into->op == ast::BinOp::assign);
  ast::IdRef* const intoName = as<ast::IdRef>(into->left);
  CHECK(intoName != nullptr && intoName->name == "groups");
}

// WINDOW/AGGREGATE (window_1.aql): two WINDOW clauses on one for_op, each
// with its own AGGREGATE.
void window_clauses() {
  const std::string src = load("window_1.aql");
  cursiv::ParseResult r = build(src);
  ast::SimpleQuery* const q = as<ast::SimpleQuery>(r.value);
  CHECK(q != nullptr && q->forOps.size() == 1);
  if (q == nullptr || q->forOps.empty()) return;
  ast::ForOp* const op = q->forOps[0];
  CHECK(operations<ast::Window>(op).size() == 2);
  if (operations<ast::Window>(op).size() != 2) return;
  CHECK(operations<ast::Window>(op)[0]->aggregate != nullptr &&
        operations<ast::Window>(op)[0]->aggregate->name == "rollingAverage");
  CHECK(operations<ast::Window>(op)[1]->aggregate != nullptr &&
        operations<ast::Window>(op)[1]->aggregate->name == "cumulativeSum");
}

// remove_1.aql: OLD read back as a NewOldRef, through the REMOVE clause and
// the RETURN clause's own leading LET.
void new_old_reference() {
  const std::string src = load("remove_1.aql");
  cursiv::ParseResult r = build(src);
  ast::DataModificationQuery* const q = as<ast::DataModificationQuery>(r.value);
  CHECK(q != nullptr && q->forOps.size() == 1 && q->removes.size() == 1);
  if (q == nullptr) return;
  ast::ReturnExpr* const ret = q->return_;
  CHECK(ret != nullptr && ret->letBefore != nullptr);
  if (ret == nullptr || ret->letBefore == nullptr) return;
  // `LET removed = OLD`: one binding, an assignment whose right side is OLD.
  CHECK(ret->letBefore->bindings.size() == 1);
  ast::BinaryExpr* const bind = as<ast::BinaryExpr>(ret->letBefore->bindings[0]);
  CHECK(bind != nullptr && bind->op == ast::BinOp::assign);
  ast::NewOldRef* const old =
      bind != nullptr ? as<ast::NewOldRef>(bind->right) : nullptr;
  CHECK(old != nullptr && !old->isNew);
  // `RETURN removed._key`: a DotExpr over the LET-bound name.
  ast::DotExpr* const dot = ret != nullptr ? as<ast::DotExpr>(ret->value) : nullptr;
  CHECK(dot != nullptr && dot->field == "_key");
}

// The graph traversal shape: no corpus document exercises it, so
// tests/own_traversal_graph.aql does.
void graph_traversal() {
  const std::string src = load_test("own_traversal_graph.aql");
  cursiv::ParseResult r = build(src);
  ast::TraversalQuery* const q = as<ast::TraversalQuery>(r.value);
  CHECK(q != nullptr);
  if (q == nullptr) return;
  CHECK(q->v == "v" && q->e == "e" && q->p == "p");
  CHECK(q->rangeFrom.has_value() && *q->rangeFrom == 1.0);
  CHECK(q->rangeTo.has_value() && *q->rangeTo == 3.0);
  CHECK(q->direction == "OUTBOUND");
  CHECK(q->graph != nullptr);
  CHECK(q->prune != nullptr);
  CHECK(q->pruneVar == "stop");
  CHECK(q->filters.size() == 1);
  CHECK(q->options != nullptr);
}

// The quantified array comparison, ternary and NOT run: own_operators.aql.
void quantified_comparison() {
  const std::string src = load_test("own_operators.aql");
  cursiv::ParseResult r = build(src);
  CHECK(static_cast<bool>(r) && r.diagnostics.empty());
  ast::SimpleQuery* const q = as<ast::SimpleQuery>(r.value);
  CHECK(q != nullptr && q->forOps.empty());
  if (q == nullptr) return;
  CHECK(q->with_.empty());
}

// `@@name` as a FOR source: own_bindparam_collection.aql.
void bind_param_collection() {
  const std::string src = load_test("own_bindparam_collection.aql");
  cursiv::ParseResult r = build(src);
  ast::SimpleQuery* const q = as<ast::SimpleQuery>(r.value);
  CHECK(q != nullptr && q->forOps.size() == 1);
  if (q == nullptr || q->forOps.empty()) return;
  ast::CollectionRef* const src2 =
      as<ast::CollectionRef>(q->forOps[0]->forIn->source);
  CHECK(src2 != nullptr && src2->name == "@@bindcoll");
}

ast::Expr* returned(cursiv::ParseResult& r) {
  ast::SimpleQuery* const q = as<ast::SimpleQuery>(r.value);
  return (q != nullptr && q->return_ != nullptr) ? q->return_->value : nullptr;
}

// AQL's own operator table: range above relational, relational above
// equality/IN/LIKE, AND above OR above assignment above the ternary
// (loosest), and NOT/`!` a tight unary.
void operator_precedence() {
  {
    cursiv::ParseResult r = build("RETURN 1 .. 5 < 10");
    ast::BinaryExpr* const lt = as<ast::BinaryExpr>(returned(r));
    CHECK(lt != nullptr && lt->op == ast::BinOp::lt);
    ast::BinaryExpr* const range = lt != nullptr ? as<ast::BinaryExpr>(lt->left) : nullptr;
    CHECK(range != nullptr && range->op == ast::BinOp::range);
  }
  {
    cursiv::ParseResult r = build("RETURN a < b == c");
    ast::BinaryExpr* const eq = as<ast::BinaryExpr>(returned(r));
    CHECK(eq != nullptr && eq->op == ast::BinOp::eq);
    ast::BinaryExpr* const lt = eq != nullptr ? as<ast::BinaryExpr>(eq->left) : nullptr;
    CHECK(lt != nullptr && lt->op == ast::BinOp::lt);
  }
  {
    cursiv::ParseResult r = build("RETURN a && b || c");
    ast::BinaryExpr* const or_ = as<ast::BinaryExpr>(returned(r));
    CHECK(or_ != nullptr && or_->op == ast::BinOp::or_);
    ast::BinaryExpr* const and_ = or_ != nullptr ? as<ast::BinaryExpr>(or_->left) : nullptr;
    CHECK(and_ != nullptr && and_->op == ast::BinOp::and_);
  }
  {
    // The condition is the full AND/OR level; NOT/`!` outbinds it (tight
    // unary), so this is `(a || b) ? (!c) : d`, not `a || (b ? !c : d)`.
    cursiv::ParseResult r = build("RETURN a || b ? !c : d");
    ast::TernaryExpr* const t = as<ast::TernaryExpr>(returned(r));
    CHECK(t != nullptr);
    if (t == nullptr) return;
    ast::BinaryExpr* const cond = as<ast::BinaryExpr>(t->cond);
    CHECK(cond != nullptr && cond->op == ast::BinOp::or_);
    CHECK(as<ast::NotExpr>(t->then_) != nullptr);
  }
  {
    // Commas and assignment form LET lists, not scalar expressions.
    const std::string text = "LET a = 1, b = c || d RETURN b";
    cursiv::ParseResult r = build(text);
    auto* const query = as<ast::SimpleQuery>(r.value);
    CHECK(query && query->return_ && query->return_->letBefore);
    if (!query || !query->return_ || !query->return_->letBefore) return;
    const auto& bindings = query->return_->letBefore->bindings;
    CHECK(bindings.size() == 1);
    ast::BinaryExpr* const comma = bindings.empty() ? nullptr :
                                  as<ast::BinaryExpr>(bindings[0]);
    CHECK(comma != nullptr && comma->op == ast::BinOp::comma);
    ast::BinaryExpr* const assign = comma != nullptr ? as<ast::BinaryExpr>(comma->right) : nullptr;
    CHECK(assign != nullptr && assign->op == ast::BinOp::assign);
    ast::BinaryExpr* const or_ = assign != nullptr ? as<ast::BinaryExpr>(assign->right) : nullptr;
    CHECK(or_ != nullptr && or_->op == ast::BinOp::or_);
  }
  {
    // `a ? b : c ? d : e` reads as `a ? b : (c ? d : e)`.
    cursiv::ParseResult r = build("RETURN a ? b : c ? d : e");
    ast::TernaryExpr* const outer = as<ast::TernaryExpr>(returned(r));
    CHECK(outer != nullptr && as<ast::TernaryExpr>(outer->else_) != nullptr);
  }
  {
    // NOT is a tight unary: `NOT a == b` is `(NOT a) == b`, and it still
    // stacks (`!!true`, exercised by own_operators.aql).
    cursiv::ParseResult r = build("RETURN NOT a == b");
    ast::BinaryExpr* const eq = as<ast::BinaryExpr>(returned(r));
    CHECK(eq != nullptr && eq->op == ast::BinOp::eq);
    CHECK(eq != nullptr && as<ast::NotExpr>(eq->left) != nullptr);
  }
}

void list_values(ast::Expr* value, std::vector<ast::Expr*>& result) {
  auto* binary = as<ast::BinaryExpr>(value);
  if (binary && binary->op == ast::BinOp::comma) {
    list_values(binary->left, result);
    list_values(binary->right, result);
  } else if (value) {
    result.push_back(value);
  }
}

void native_text_and_numbers() {
  const std::string source = R"AQL(
    /* outer /* is ordinary comment content */
    RETURN ["", '', 'a\'b', "a\"b", "\b\f\n\r\t\/\\", "\u0061\uD83D\uDE00", "\q", "\u0000", 0b101, 0xAf,]
  )AQL";
  auto result = build(source);
  CHECK(!result.cst().recovered() && result.diagnostics.empty());
  auto* const array = as<ast::ArrayLit>(returned(result));
  CHECK(array);
  std::vector<ast::Expr*> values;
  if (array) list_values(array->value, values);
  CHECK(values.size() == 10);
  if (values.size() != 10) return;
  const std::vector<std::string> expected{
      "", "", "a'b", "a\"b", "\b\f\n\r\t/\\", "a\xf0\x9f\x98\x80", "q",
      std::string(1, '\0')};
  for (std::size_t i = 0; i < expected.size(); ++i) {
    auto* const text = as<ast::StringLit>(values[i]);
    CHECK(text && text->value == expected[i]);
  }
  auto* const binary = as<ast::NumberLit>(values[8]);
  auto* const hex = as<ast::NumberLit>(values[9]);
  CHECK(binary && binary->value == 5);
  CHECK(hex && hex->value == 175);
  const std::string lineComment = "RETURN 1 // comment\r still comment\n";
  const auto commented = build(lineComment);
  CHECK(!commented.cst().recovered() && commented.diagnostics.empty());
  const auto overflow = engine().parse("RETURN 0x100000000");
  CHECK(!overflow.diagnostics.empty());
}

void native_objects_and_names() {
  const std::string source = R"AQL(
    LET $value = 1, short = 2
    RETURN {first: $value, short, [@_key]: 3, @_name: 4, last: 5,}
  )AQL";
  auto result = build(source);
  CHECK(!result.cst().recovered() && result.diagnostics.empty());
  auto* const object = as<ast::ObjectLit>(returned(result));
  CHECK(object && object->pairs.size() == 5);
  if (object && object->pairs.size() == 5) {
    CHECK(object->pairs[0]->key == "first" && !object->pairs[0]->shorthand);
    CHECK(object->pairs[1]->key == "short" && object->pairs[1]->shorthand &&
          !object->pairs[1]->value);
    CHECK(as<ast::BindParamRef>(object->pairs[2]->keyExpr));
    CHECK(as<ast::BindParamRef>(object->pairs[3]->keyExpr));
    CHECK(object->pairs[4]->key == "last");
  }
  const std::string names = R"AQL(FOR d IN `my-coll` RETURN d.´sort´)AQL";
  auto quoted = build(names);
  CHECK(!quoted.cst().recovered() && quoted.diagnostics.empty());
  auto* const dot = as<ast::DotExpr>(returned(quoted));
  CHECK(dot && dot->field == "sort");
  const std::string bound = "FOR d IN @@_collection RETURN d.@_field";
  auto boundResult = build(bound);
  auto* const boundDot = as<ast::DotExpr>(returned(boundResult));
  CHECK(boundDot && boundDot->fieldParam &&
        boundDot->fieldParam->name == "@_field");
  const std::string call = "RETURN my :: namespace :: fn()";
  auto function = build(call);
  auto* const functionNode = as<ast::FuncCall>(returned(function));
  CHECK(functionNode && functionNode->name == "my::namespace::fn" &&
        !functionNode->args);
}

void native_scalar_expressions() {
  const std::string source = "RETURN [true ? 1 : 2, false ?: 3, 4]";
  auto result = build(source);
  CHECK(!result.cst().recovered() && result.diagnostics.empty());
  auto* const array = as<ast::ArrayLit>(returned(result));
  std::vector<ast::Expr*> values;
  if (array) list_values(array->value, values);
  CHECK(values.size() == 3);
  if (values.size() == 3) {
    CHECK(as<ast::TernaryExpr>(values[0]));
    auto* const elvis = as<ast::TernaryExpr>(values[1]);
    CHECK(elvis && !elvis->then_ && elvis->else_);
    CHECK(as<ast::NumberLit>(values[2]));
  }
  const std::string membership = "RETURN 1 == 2 IN [2]";
  auto compare = build(membership);
  auto* const eq = as<ast::BinaryExpr>(returned(compare));
  CHECK(eq && eq->op == ast::BinOp::eq);
  auto* const in = eq ? as<ast::BinaryExpr>(eq->right) : nullptr;
  CHECK(in && in->op == ast::BinOp::in);
  for (const std::string invalid : {"RETURN 1, 2", "RETURN a = b",
                                    "RETURN 'a''b'", "RETURN 01"}) {
    const auto rejected = engine().parse(invalid);
    CHECK(!rejected.accepted || rejected.cst().recovered() ||
          !rejected.diagnostics.empty());
  }
  for (const std::string source : {"RETURN [1,2] ALL NOT IN [3]",
                                   "RETURN [1,2] ANY !IN [3]",
                                   "RETURN [1,2] AT LEAST(@minimum) == 1"}) {
    auto comparison = build(source);
    CHECK(!comparison.cst().recovered() && comparison.diagnostics.empty());
    auto* const quantified = as<ast::QuantifiedExpr>(returned(comparison));
    CHECK(quantified);
    if (quantified && quantified->quant == ast::Quantifier::atLeast)
      CHECK(as<ast::BindParamRef>(quantified->minimum));
    else if (quantified)
      CHECK(quantified->cmp == ast::CmpOp::notIn);
  }
}

void native_pipeline_and_writes() {
  const std::string source =
      "LET x = 1 FILTER x > 0 SORT x, x + 1 DESC LIMIT @offset, @count RETURN x";
  auto result = build(source);
  CHECK(!result.cst().recovered() && result.diagnostics.empty());
  auto* const query = as<ast::SimpleQuery>(result.value);
  CHECK(query && query->forOps.size() == 1);
  if (query && query->forOps.size() == 1) {
    auto* const pipeline = query->forOps[0];
    CHECK(!pipeline->forIn);
    const auto sorts = operations<ast::Sort>(pipeline);
    const auto limits = operations<ast::Limit>(pipeline);
    CHECK(sorts.size() == 1 && sorts[0]->terms.size() == 2);
    CHECK(limits.size() == 1 && as<ast::BindParamRef>(limits[0]->count) &&
          as<ast::BindParamRef>(limits[0]->second));
  }
  const std::string aggregate =
      "FOR x IN [1,2] COLLECT AGGREGATE a = SUM(x), b = MAX(x) RETURN [a,b]";
  auto grouped = build(aggregate);
  CHECK(!grouped.cst().recovered() && grouped.diagnostics.empty());
  auto* const groupQuery = as<ast::SimpleQuery>(grouped.value);
  if (groupQuery && groupQuery->forOps.size() == 1) {
    const auto collects = operations<ast::Collect>(groupQuery->forOps[0]);
    CHECK(collects.size() == 1 && collects[0]->aggregate &&
          collects[0]->moreAggregates.size() == 1 &&
          collects[0]->moreAggregates[0]->name == "b");
  } else {
    CHECK(false);
  }
  const std::string insert =
      "LET doc = {a: 1} INSERT doc INTO 'my-coll' RETURN NEW";
  auto inserted = build(insert);
  CHECK(!inserted.cst().recovered() && inserted.diagnostics.empty());
  auto* const write = as<ast::DataModificationQuery>(inserted.value);
  CHECK(write && write->letBefore && write->insert_ &&
        as<ast::IdRef>(write->insert_->doc) &&
        write->insert_->target->name == "my-coll");
  const std::string update = "UPDATE 'key' WITH @patch INTO @@_collection";
  auto updated = build(update);
  CHECK(!updated.cst().recovered() && updated.diagnostics.empty());
  auto* const updateQuery = as<ast::DataModificationQuery>(updated.value);
  CHECK(updateQuery && updateQuery->update_ &&
        as<ast::BindParamRef>(updateQuery->update_->doc));
  const std::string replace = "REPLACE @doc INTO `my-coll`";
  auto replaced = build(replace);
  CHECK(!replaced.cst().recovered() && replaced.diagnostics.empty());
  auto* const replaceQuery = as<ast::DataModificationQuery>(replaced.value);
  CHECK(replaceQuery && replaceQuery->replace_ &&
        as<ast::BindParamRef>(replaceQuery->replace_->doc));
}

}  // namespace

int main() {
  if (!aql_upsert::run(engine(), reflection())) return 1;
  {
    const std::string source = "FOR x IN xs SORT x FILTER x > 0 LIMIT 2 FILTER x < 9 RETURN F()";
    auto result = build(source);
    CHECK(!result.cst().recovered() && result.diagnostics.empty());
    const auto *query = as<ast::SimpleQuery>(result.value);
    CHECK(query && query->forOps.size() == 1);
    if (query && query->forOps.size() == 1) {
      const auto &ops = query->forOps[0]->operations;
      CHECK(ops.size() == 4);
      if (ops.size() == 4)
        CHECK(std::holds_alternative<ast::Sort *>(ops[0]) &&
              std::holds_alternative<ast::Filter *>(ops[1]) &&
              std::holds_alternative<ast::Limit *>(ops[2]) &&
              std::holds_alternative<ast::Filter *>(ops[3]));
    }
    const auto *call = as<ast::FuncCall>(returned(result));
    CHECK(call && !call->args);
  }
  corpus();
  collect_into_object();
  window_clauses();
  new_old_reference();
  graph_traversal();
  quantified_comparison();
  bind_param_collection();
  operator_precedence();
  native_text_and_numbers();
  native_objects_and_names();
  native_scalar_expressions();
  native_pipeline_and_writes();
  return cursiv::test::report("aql_grammar_test");
}
