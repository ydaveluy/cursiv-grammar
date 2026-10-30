// The grammar end to end: every document under example/ and tests/ parses
// clean (recovered() false, no diagnostics, the right root type), and a few
// real field values come back out of one document's AST.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "caql/core/ast.hpp"
#include "caql/generated/Caql/Module.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "caql: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      caql::generated::Caql::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  // Native CAQL call shapes from the Circonus reference manual. Preserve
  // argument names and source inputs, not just successful tokenization.
  for (const char* source : {
           "pass()", "count()", "stats:sum(){pass(),counter()}",
           "stats:sum{pass(),counter()}",
           "find('foo', start_time_offset=1d 6h, limit = 5)",
           "find('foo') | rolling:max(1hour30minutes)",
           "find('foo') | top(5, method='max')",
           "op:sum(VIEW_PERIOD)", "delay(-1.5h)",
           "!1 + 2", "not 1 or 0", "find('foo') | stats:sum{pass()}",
           "#preview\n#min_period=10\nfind('foo')"}) {
    const auto r = engine().parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        cursiv::ast_cast<caql::ast::Query>(r.value) == nullptr)
      return fail(std::string(source) + ": native call shape refused");
  }
  {
    const auto r = engine().parse("find('foo') | top(5, method=p'ma%78')");
    const auto* query = cursiv::ast_cast<caql::ast::Query>(r.value);
    const auto* pipe = query != nullptr
                           ? cursiv::ast_cast<caql::ast::PipeExpr>(query->expression)
                           : nullptr;
    const auto* value = pipe != nullptr && pipe->args.size() == 2
                            ? cursiv::ast_cast<caql::ast::StringLit>(
                                  pipe->args[1]->value)
                            : nullptr;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        pipe == nullptr || pipe->args.size() != 2 ||
        !pipe->args[0]->name.empty() || pipe->args[1]->name != "method" ||
        value == nullptr || value->value != "max")
      return fail("named pipe argument or percent-decoded value lost");
  }
  {
    const auto r = engine().parse("stats:sum(){pass(),counter()}");
    const auto* query = cursiv::ast_cast<caql::ast::Query>(r.value);
    const auto* call = query != nullptr
                           ? cursiv::ast_cast<caql::ast::CallExpr>(query->expression)
                           : nullptr;
    if (call == nullptr || !call->args.empty() || call->sources.size() != 2 ||
        cursiv::ast_cast<caql::ast::CallExpr>(call->sources[0]) == nullptr ||
        cursiv::ast_cast<caql::ast::CallExpr>(call->sources[1]) == nullptr)
      return fail("explicit call sources lost");
  }
  {
    const auto r = engine().parse("find('foo', start_time_offset=1d 6h, limit=5)");
    const auto* query = cursiv::ast_cast<caql::ast::Query>(r.value);
    const auto* call = query != nullptr
                           ? cursiv::ast_cast<caql::ast::CallExpr>(query->expression)
                           : nullptr;
    const auto* duration = call != nullptr && call->args.size() == 3
                               ? cursiv::ast_cast<caql::ast::DurationLit>(
                                     call->args[1]->value)
                               : nullptr;
    if (duration == nullptr || duration->text != "1d 6h" ||
        call->args[1]->name != "start_time_offset" ||
        call->args[2]->name != "limit" || !call->args[2]->limit)
      return fail("duration argument or keyword argument metadata lost");
  }
  {
    const auto r = engine().parse("find(p'broken%GG')");
    if (r.diagnostics.empty())
      return fail("malformed percent escape silently accepted");
  }

  std::vector<fs::path> docs;
  // CURSIV_TEST_DIR holds documents written for this port, exercising
  // constructs the upstream corpus never reaches; CURSIV_EXAMPLE_DIR is the
  // corpus itself.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".caql")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path readback_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = engine().parse(text);
    if (!result.accepted)
      return fail(doc.string() + ": the parse was refused");
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<caql::ast::Query>)
      return fail(doc.string() + ": the parse built no query");
    if (doc.filename() == "s1.caql") readback_doc = doc;
  }

  if (readback_doc.empty())
    return fail("s1.caql was not found among the examples");

  // Read real field values out of s1.caql's AST: a converter that silently
  // yields the wrong value, or a grouping that misfires, would still leave
  // recovered() false and the root type right.
  //
  //   (find("dur2") | stats:sum()) * (find("dur2") | stats:sum()) + 2
  const std::string text = read_file(readback_doc);
  const cursiv::ParseResult result = engine().parse(text);
  const auto& query = *static_cast<const caql::ast::Query*>(result.value);

  if (query.expression == nullptr ||
      query.expression->type != &cursiv::ast_type_tag<caql::ast::BinaryExpr>)
    return fail("s1: expected a binary expression at the top");
  const auto& top = *static_cast<const caql::ast::BinaryExpr*>(query.expression);
  if (top.op != caql::ast::Operator::add)
    return fail("s1: expected + at the top");
  if (top.right == nullptr ||
      top.right->type != &cursiv::ast_type_tag<caql::ast::NumberLit>)
    return fail("s1: expected a number on the right");
  const auto& two = *static_cast<const caql::ast::NumberLit*>(top.right);
  if (two.value != 2.0) return fail("s1: wrong number on the right");

  if (top.left == nullptr ||
      top.left->type != &cursiv::ast_type_tag<caql::ast::BinaryExpr>)
    return fail("s1: expected a binary expression on the left");
  const auto& product = *static_cast<const caql::ast::BinaryExpr*>(top.left);
  if (product.op != caql::ast::Operator::multiply)
    return fail("s1: expected * on the left");

  for (const caql::ast::Expression* const side : {product.left, product.right}) {
    if (side == nullptr || side->type != &cursiv::ast_type_tag<caql::ast::PipeExpr>)
      return fail("s1: expected a pipe expression on each side of *");
    const auto& pipe = *static_cast<const caql::ast::PipeExpr*>(side);
    if (pipe.name != "stats:sum" || !pipe.args.empty())
      return fail("s1: wrong pipe call");
    if (pipe.operand == nullptr ||
        pipe.operand->type != &cursiv::ast_type_tag<caql::ast::CallExpr>)
      return fail("s1: expected a call as the pipe operand");
    const auto& call = *static_cast<const caql::ast::CallExpr*>(pipe.operand);
    if (call.name != "find" || call.args.size() != 1)
      return fail("s1: wrong find() call");
    if (call.args[0] == nullptr || call.args[0]->limit ||
        call.args[0]->value == nullptr ||
        call.args[0]->value->type != &cursiv::ast_type_tag<caql::ast::StringLit>)
      return fail("s1: wrong find() argument");
    const auto& arg =
        *static_cast<const caql::ast::StringLit*>(call.args[0]->value);
    if (arg.value != "dur2") return fail("s1: wrong find() argument value");
  }

  std::printf("caql: %zu documents parse clean\n", docs.size());
  return 0;
}
