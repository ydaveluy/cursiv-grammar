// The lua grammar end to end: every example parses clean, and the shapes
// that are easy to get wrong are pinned -- long brackets at any level, `^`
// against unary, and the no-newline-significance call chain.

#include <deque>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/CstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "lua/core/ast.hpp"
#include "lua/core/CoreServices.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "lua/generated/Lua/Module.hpp"

namespace {

namespace ast = lua::ast;

const auto& services() {
  static const auto language = cursiv::create_language<lua::LuaCoreServices,
      lua::LuaSharedCoreServices, lua::install_core_module>();
  return language.language;
}
const cursiv::Parser& engine() { return *services().parser; }

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

// The document outlives the tree: every AST member is a `string_view` into
// the source, so a composed document must live past the `ParseResult` that
// points into it.
std::string_view kept(std::string text) {
  static std::deque<std::string> documents;
  documents.push_back(std::move(text));
  return documents.back();
}

cursiv::ParseResult parse(std::string_view text) { return engine().parse(text); }

bool clean(std::string_view text) {
  const cursiv::ParseResult r = parse(text);
  return r.accepted && !r.cst().recovered();
}

const char* const kExamples[] = {
    "3885-example1.lua",       "bitwise.lua", "closure.lua",
    "comments-galore.lua",     "factorial.lua", "goto.lua",
    "helloworld.lua",          "pz.lua",
    "stats.lua",               "tabWidth.lua",        "shebang.lua",
};

void every_example_parses_clean() {
  for (const char* const name : kExamples) {
    const std::string text = cursiv::test::read_file(
        std::string(CURSIV_EXAMPLE_DIR) + "/" + name);
    CHECK(!text.empty());
    const cursiv::ParseResult r = parse(kept(text));
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    ast::Block* const root = as<ast::Block>(r.value);
    CHECK(root != nullptr);
  }
}

// Any level, and the two shapes real Lua code leans on: an unmatched
// bracket inside a lower level, and a `[` that opens nothing.
void long_brackets_every_level() {
  CHECK(clean("--[=oops\nreturn 1\n"));
  CHECK(clean("--[==\nreturn 1\n"));
  CHECK(clean("x = [[plain]]\n"));
  CHECK(clean("x = [=[ has ]] inside ]=]\n"));
  CHECK(clean("x = [==[ has ]=] inside ]==]\n"));
  CHECK(clean("x = [===[ has ]==] inside ]===]\n"));
  CHECK(clean("x = [====[ has ]===] inside ]====]\n"));
  CHECK(clean("x = [=====[ has ]====] inside ]=====]\n"));
  CHECK(clean("x = [==========[ has ]=========] inside ]==========]\n"));
  CHECK(!clean("x = [==[ never closed ]=]\n"));
  CHECK(clean("--[[ a long comment ]]\nx = 1\n"));
  CHECK(clean("--[==[ a long comment ]==]\nx = 1\n"));
  CHECK(clean("-- [ not a long bracket\nx = 1\n"));

  // A level-six comment and a level-eight string, the string holding false
  // closers up to level six that must not end it early.
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_TEST_DIR) + "/long-bracket-level6.lua");
  CHECK(!text.empty());
  CHECK(clean(kept(text)));
}

// `^` binds tighter than a unary operator, but a unary operand may itself
// hold a `^` chain: `-2^2` is `-(2^2)`, not `(-2)^2`.
void pow_against_unary() {
  const cursiv::ParseResult r = parse("x = -2^2\n");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Block* const block = as<ast::Block>(r.value);
  CHECK(block != nullptr && block->statements.size() == 1);
  if (block == nullptr || block->statements.empty()) return;
  ast::AssignStat* const assign = as<ast::AssignStat>(block->statements[0]);
  CHECK(assign != nullptr && assign->values.size() == 1);
  if (assign == nullptr || assign->values.empty()) return;
  ast::UnaryExpr* const neg = as<ast::UnaryExpr>(assign->values[0]);
  CHECK(neg != nullptr && neg->op == ast::UnOp::neg);
  if (neg == nullptr) return;
  ast::BinaryExpr* const pow = as<ast::BinaryExpr>(neg->operand);
  CHECK(pow != nullptr && pow->op == ast::BinOp::pow);
}

// Lua has no automatic semicolon insertion: a line starting with `(` reads
// on from what came before, chaining a call onto it rather than starting a
// new statement -- the trap `stats.lua` itself exercises.
void no_newline_significance() {
  const cursiv::ParseResult r = parse("a = b + c\n(print or io.write)('done')\n");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Block* const block = as<ast::Block>(r.value);
  CHECK(block != nullptr && block->statements.size() == 1);
  if (block == nullptr || block->statements.empty()) return;
  ast::AssignStat* const assign = as<ast::AssignStat>(block->statements[0]);
  CHECK(assign != nullptr && assign->values.size() == 1);
  if (assign == nullptr || assign->values.empty()) return;
  ast::BinaryExpr* const sum = as<ast::BinaryExpr>(assign->values[0]);
  CHECK(sum != nullptr && sum->op == ast::BinOp::add);
  if (sum == nullptr) return;
  CHECK(as<ast::CallExpr>(sum->right) != nullptr);
}

// Lua 5.2 and later read a `(` after a call on the next line as one more
// call: `f()\n(g)()` is a single statement.
void paren_on_next_line_calls() {
  const cursiv::ParseResult r = parse("f()\n(g)()\n");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Block* const block = as<ast::Block>(r.value);
  CHECK(block != nullptr && block->statements.size() == 1);
  if (block == nullptr || block->statements.empty()) return;
  ast::CallStat* const stat = as<ast::CallStat>(block->statements[0]);
  CHECK(stat != nullptr);
  if (stat == nullptr) return;
  ast::CallExpr* const outer = as<ast::CallExpr>(stat->call);
  CHECK(outer != nullptr && outer->args.empty());
  if (outer == nullptr) return;
  ast::CallExpr* const middle = as<ast::CallExpr>(outer->callee);
  CHECK(middle != nullptr && middle->args.size() == 1);
  if (middle == nullptr) return;
  ast::CallExpr* const inner = as<ast::CallExpr>(middle->callee);
  CHECK(inner != nullptr && inner->args.empty() &&
        as<ast::NameExpr>(inner->callee) != nullptr);
}

// `a.b[c]:d(e)` is a method call wrapping an index wrapping a field wrapping
// a name.
void suffix_chain_nests_left() {
  const cursiv::ParseResult r = parse("a.b[c]:d(e)\n");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Block* const block = as<ast::Block>(r.value);
  CHECK(block != nullptr && block->statements.size() == 1);
  if (block == nullptr || block->statements.empty()) return;
  ast::CallStat* const stat = as<ast::CallStat>(block->statements[0]);
  CHECK(stat != nullptr);
  if (stat == nullptr) return;
  ast::MethodCallExpr* const call = as<ast::MethodCallExpr>(stat->call);
  CHECK(call != nullptr && call->method == "d" && call->args.size() == 1);
  if (call == nullptr) return;
  ast::IndexExpr* const index = as<ast::IndexExpr>(call->object);
  CHECK(index != nullptr);
  if (index == nullptr) return;
  ast::FieldExpr* const field = as<ast::FieldExpr>(index->object);
  CHECK(field != nullptr && field->field == "b");
  if (field == nullptr) return;
  CHECK(as<ast::NameExpr>(field->object) != nullptr);
}

void keywords_may_name() {
  CHECK(!clean("local end = 1\n"));
  CHECK(!clean("t.end = 1\n"));
  CHECK(clean("local ending = 1\n"));
}

void whitespace_and_line_endings() {
  CHECK(clean("local\vx = 1\n"));
  for (const std::string_view newline : {"\n", "\r", "\r\n", "\n\r"}) {
    const std::string_view escaped = kept("x = \"a\\" + std::string(newline) + "b\"");
    CHECK(clean(escaped));
    const std::string_view zapped =
        kept("x = \"a\\z\v" + std::string(newline) + "b\"");
    CHECK(clean(zapped));
    const std::string_view longString =
        kept("x = [[" + std::string(newline) + "hello]]");
    const auto result = parse(longString);
    CHECK(result.accepted && !result.cst().recovered());
    const auto* root = as<ast::Block>(result.value);
    const auto* assignment = root && root->statements.size() == 1
                                 ? as<ast::AssignStat>(root->statements[0])
                                 : nullptr;
    const auto* value = assignment && assignment->values.size() == 1
                            ? as<ast::StringExpr>(assignment->values[0])
                            : nullptr;
    CHECK(value != nullptr && value->value == "hello");
  }
}

void numeral_forms() {
  for (const std::string_view numeral :
       {"1", "1.", ".5", "1.25", "1e2", "1.e-2", ".5E+3", "0x1",
        "0X1.", "0x.8", "0x1.fp+2", "0x1p-2", "0X.8P2"}) {
    const std::string_view text = kept("x = " + std::string(numeral));
    const auto result = parse(text);
    CHECK(result.accepted && !result.cst().recovered());
    const auto* root = as<ast::Block>(result.value);
    const auto* assignment = root && root->statements.size() == 1
                                 ? as<ast::AssignStat>(root->statements[0])
                                 : nullptr;
    const auto* value = assignment && assignment->values.size() == 1
                            ? as<ast::NumberExpr>(assignment->values[0])
                            : nullptr;
    CHECK(value != nullptr && value->text == numeral);
  }
}

void native_reader_constraints() {
  for (const auto source : {"x=1foo()", "x=0x1g()", "x=1..2", "local x<close>=nil"})
    CHECK(!clean(source));
  for (const auto source : {R"(x="\q")", R"(x="\256")", R"(x="\xF")",
                            R"(x="\u{80000000}")", R"(x="\u{}")",
                            "local x<invalid> = 1", "local a<close>,b<close> = nil,nil",
                            "break", "while true do local f=function() break end end",
                            "function f() return ... end"}) {
    const auto result = parse(source);
    CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    CHECK(!cursiv::test::validationHelper(services(), source).diagnostics.empty());
  }
  for (const auto source : {R"(x="\xFF\2559\u{7FFFFFFF}\u{D800}")",
                            R"(x=[[\q\256\u{FFFFFFFF}]])", "x=1 .. 2",
                            "local a<close>,b<const> = nil,1", "while true do break end",
                            "function f(...) return ... end", "return ...", "\ufeffreturn 1"})
    CHECK(cursiv::test::validationHelper(services(), source).diagnostics.empty());
  const auto result = parse(R"(x=[[\q]])");
  const auto* root = result.value_as<ast::Block>();
  const auto* assignment = root && root->statements.size() == 1
      ? as<ast::AssignStat>(root->statements[0]) : nullptr;
  const auto* value = assignment && assignment->values.size() == 1
      ? as<ast::StringExpr>(assignment->values[0]) : nullptr;
  CHECK(value && value->longForm && value->value == R"(\q)");
}

}  // namespace

// A shebang is a token of the entry itself: the block behind it is still the
// root, alone, and its record is the whole document.
void shebang_keeps_the_root() {
  const std::string_view text = kept("#!/usr/bin/lua\nx = 1\n");
  const cursiv::ParseResult r = parse(text);
  CHECK(r.accepted && !r.cst().recovered());
  ast::Block* const root = as<ast::Block>(r.value);
  CHECK(root != nullptr && root->statements.size() == 1);
  if (root != nullptr) {
    const cursiv::CstNode& record = r.cst()[root->cstNodeId];
    // ...up to the last token: the final newline is trivia behind the root.
    CHECK(record.begin == 0 && record.end == text.size() - 1);
  }
}

int main() {
  every_example_parses_clean();
  shebang_keeps_the_root();
  long_brackets_every_level();
  pow_against_unary();
  no_newline_significance();
  paren_on_next_line_calls();
  suffix_chain_nests_left();
  keywords_may_name();
  whitespace_and_line_endings();
  numeral_forms();
  native_reader_constraints();
  return cursiv::test::report("lua_grammar_test");
}
