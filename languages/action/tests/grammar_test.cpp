// The grammar end to end: every document under example/ (the upstream
// corpus) and tests/ (constructs the corpus never reaches) parses clean
// (recovered() false, no diagnostics, the right root type), and a few real
// field values come back out of two documents' ASTs.

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
#include "action/core/ast.hpp"
#include "action/generated/Action/Module.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "action: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      action::generated::Action::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  std::vector<fs::path> docs;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".act")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path primes_doc;
  fs::path hello_doc;
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
        root->type != &cursiv::ast_type_tag<action::ast::File>)
      return fail(doc.string() + ": the parse built no file");
    if (doc.filename() == "primes.act") primes_doc = doc;
    if (doc.filename() == "example1.act") hello_doc = doc;
  }
  if (primes_doc.empty())
    return fail("primes.act was not found among the examples");
  if (hello_doc.empty())
    return fail("example1.act was not found among the examples");

  // Read a few real field values out of primes.act: a converter or a
  // structural mistake would still leave `recovered()` false and the root
  // type right, so only reading the fields back catches it.
  {
    const std::string text = read_file(primes_doc);
    const cursiv::ParseResult result = engine().parse(text);
    const auto& file = *static_cast<const action::ast::File*>(result.value);
    if (file.modules.size() != 1) return fail("primes.act: wrong module count");
    const action::ast::ProgModule& module = *file.modules[0];
    if (module.decls.empty() ||
        module.decls[0]->type != &cursiv::ast_type_tag<action::ast::DefineDecl>)
      return fail("primes.act: the first decl is not a DEFINE");
    const auto& define =
        *static_cast<const action::ast::DefineDecl*>(module.decls[0]);
    if (define.defs.size() != 3 || define.defs[0]->name != "size" ||
        define.defs[0]->value != "8190")
      return fail("primes.act: DEFINE size does not hold 8190");
    if (module.routines.size() != 1 ||
        module.routines[0]->type !=
            &cursiv::ast_type_tag<action::ast::ProcRoutine>)
      return fail("primes.act: no single PROC routine");
    const auto& proc =
        *static_cast<const action::ast::ProcRoutine*>(module.routines[0]);
    if (proc.name != "Primes") return fail("primes.act: wrong PROC name");
    if (proc.body.empty())
      return fail("primes.act: the PROC body is empty");
  }

  // ...and from example1.act, that a routine call and its string argument
  // survive.
  {
    const std::string text = read_file(hello_doc);
    const cursiv::ParseResult result = engine().parse(text);
    const auto& file = *static_cast<const action::ast::File*>(result.value);
    const action::ast::ProgModule& module = *file.modules[0];
    const auto& proc =
        *static_cast<const action::ast::ProcRoutine*>(module.routines[0]);
    if (proc.name != "hello") return fail("example1.act: wrong PROC name");
    if (proc.body.size() != 1 ||
        proc.body[0]->type !=
            &cursiv::ast_type_tag<action::ast::RoutineCallStmt>)
      return fail("example1.act: the body is not one routine call");
    const auto& call =
        *static_cast<const action::ast::RoutineCallStmt*>(proc.body[0]);
    if (call.name != "PrintE" || call.args.size() != 1 ||
        call.args[0]->type != &cursiv::ast_type_tag<action::ast::StrLiteral>)
      return fail("example1.act: wrong PrintE call");
    const auto& arg = *static_cast<const action::ast::StrLiteral*>(call.args[0]);
    if (arg.text != "Hello World")
      return fail("example1.act: the string argument was not read back");
  }

  // Precedence and associativity, checked at AST level:
  // unary minus tighter than `*`, `*` tighter than `+`, XOR/`!` a bitwise
  // arithmetic operator (not relational), a bare condition read as truthy
  // beside a comparison, and AND/OR chained left to right at equal
  // precedence.
  {
    const auto parse_proc = [](const std::string& stmt) {
      static std::vector<std::unique_ptr<std::string>> kept;
      kept.push_back(std::make_unique<std::string>(
          "PROC T()\n   BYTE a, b, c, d\n   " + stmt + "\nRETURN\n"));
      return engine().parse(*kept.back());
    };
    const auto first_stmt =
        [](const cursiv::ParseResult& result) -> const action::ast::Stmt* {
      const auto& file = *static_cast<const action::ast::File*>(result.value);
      const auto& proc = *static_cast<const action::ast::ProcRoutine*>(
          file.modules[0]->routines[0]);
      return proc.body.empty() ? nullptr : proc.body[0];
    };
    const auto as_assign_value =
        [&](const std::string& expr) -> const action::ast::Expr* {
      const cursiv::ParseResult result = parse_proc("a = " + expr);
      if (!result.accepted || result.cst().recovered() ||
          !result.diagnostics.empty())
        return nullptr;
      const auto* const stmt = first_stmt(result);
      if (stmt == nullptr ||
          stmt->type != &cursiv::ast_type_tag<action::ast::AssignStmt>)
        return nullptr;
      return static_cast<const action::ast::AssignStmt*>(stmt)->value;
    };
    const auto is_binary = [](const action::ast::Expr* e, action::ast::ArithOp op)
        -> const action::ast::BinaryExpr* {
      if (e == nullptr || e->type != &cursiv::ast_type_tag<action::ast::BinaryExpr>)
        return nullptr;
      const auto* const b = static_cast<const action::ast::BinaryExpr*>(e);
      return b->op == op ? b : nullptr;
    };

    // `*` binds tighter than `+`: 2 + 3 * 4 is 2 + (3 * 4).
    {
      const auto* const add =
          is_binary(as_assign_value("2 + 3 * 4"), action::ast::ArithOp::add);
      if (add == nullptr ||
          is_binary(add->right, action::ast::ArithOp::mul) == nullptr)
        return fail("precedence: `*` did not bind tighter than `+`");
    }
    // Unary minus binds tighter than `*`: -b * c is (-b) * c.
    {
      const auto* const mul =
          is_binary(as_assign_value("-b * c"), action::ast::ArithOp::mul);
      if (mul == nullptr ||
          mul->left->type != &cursiv::ast_type_tag<action::ast::UnaryMinus>)
        return fail("precedence: unary `-` did not bind tighter than `*`");
    }
    // `!` is bitwise XOR (an arithmetic operator), never "not equal".
    {
      if (is_binary(as_assign_value("b ! c"), action::ast::ArithOp::bxor) ==
          nullptr)
        return fail("precedence: `!` is not bitwise XOR");
    }
    // A comparison takes the specific (Compare) reading, not the bare-value
    // catch-all (ArithCond) -- and vice versa for a condition with no
    // relational operator at all.
    {
      const cursiv::ParseResult cmp = parse_proc("IF a > 0 THEN\n c = 1\nFI");
      if (!cmp.accepted || cmp.cst().recovered() || !cmp.diagnostics.empty())
        return fail("catch-all: `a > 0` did not parse clean");
      const auto* const if_stmt =
          static_cast<const action::ast::IfStmt*>(first_stmt(cmp));
      if (if_stmt->cond->type != &cursiv::ast_type_tag<action::ast::Compare>)
        return fail("catch-all: `a > 0` did not take the Compare reading");

      const cursiv::ParseResult bare = parse_proc("IF a THEN\n c = 1\nFI");
      if (!bare.accepted || bare.cst().recovered() || !bare.diagnostics.empty())
        return fail("catch-all: bare `a` did not parse clean");
      const auto* const bare_if =
          static_cast<const action::ast::IfStmt*>(first_stmt(bare));
      if (bare_if->cond->type != &cursiv::ast_type_tag<action::ast::ArithCond>)
        return fail("catch-all: bare `a` did not take the ArithCond reading");
    }
    // AND/OR chain their comparisons left to right, at equal precedence:
    // `a=b OR c=d AND a=c` is `(a=b OR c=d) AND a=c`, not `a=b OR (c=d AND
    // a=c)`.
    {
      const cursiv::ParseResult chain =
          parse_proc("IF a = b OR c = d AND a = c THEN\n c = 1\nFI");
      if (!chain.accepted || chain.cst().recovered() ||
          !chain.diagnostics.empty())
        return fail("associativity: the AND/OR chain did not parse clean");
      const auto* const if_stmt =
          static_cast<const action::ast::IfStmt*>(first_stmt(chain));
      if (if_stmt->cond->type != &cursiv::ast_type_tag<action::ast::BoolChain>)
        return fail("associativity: no outer BoolChain");
      const auto& outer =
          *static_cast<const action::ast::BoolChain*>(if_stmt->cond);
      if (outer.op != action::ast::BoolOp::and_ ||
          outer.right->type != &cursiv::ast_type_tag<action::ast::Compare> ||
          outer.left->type != &cursiv::ast_type_tag<action::ast::BoolChain>)
        return fail("associativity: AND/OR did not group left to right");
    }
  }

  std::printf("action: %zu documents parse clean\n", docs.size());
  return 0;
}
