// The grammar test: every file of example/ parses clean (no repair, no
// diagnostics, a Program root), and tests/data/full.lol -- built for this
// port, exercising every construct of the language -- checks real field
// values, not only structure.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "lolcode/core/CoreServices.hpp"
#include "lolcode/core/ast.hpp"

namespace {

namespace ast = lolcode::ast;
namespace fs = std::filesystem;

int failures = 0;

void check(bool cond, const char *what) {
  if (!cond) {
    std::fprintf(stderr, "lolcode: FAIL: %s\n", what);
    ++failures;
  }
}

std::string read_file(const fs::path &path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

const cursiv::Parser &parser() {
  static const auto language =
      cursiv::create_language<lolcode::LolcodeCoreServices,
                              lolcode::LolcodeSharedCoreServices,
                              lolcode::install_core_module>();
  return *language.language.parser;
}

ast::Program *parse_clean(const std::string &text, const char *label) {
  const cursiv::ParseResult result = parser().parse(text);
  check(!result.cst().recovered(), label);
  check(result.diagnostics.empty(), label);
  const cursiv::AstNode *const root = result.value;
  check(root != nullptr && root->type == &cursiv::ast_type_tag<ast::Program>,
        label);
  return (root != nullptr && root->type == &cursiv::ast_type_tag<ast::Program>)
             ? const_cast<ast::Program *>(
                   static_cast<const ast::Program *>(root))
             : nullptr;
}

template <class T> const T *as(ast::Expr *e) {
  return e != nullptr ? cursiv::ast_cast<T>(e) : nullptr;
}
template <class T> const T *as(ast::Statement *s) {
  return s != nullptr ? cursiv::ast_cast<T>(s) : nullptr;
}
template <class T> const T *as(ast::ElseClause *e) {
  return e != nullptr ? cursiv::ast_cast<T>(e) : nullptr;
}

void every_example_parses_clean() {
  const fs::path dir = fs::path(CURSIV_EXAMPLE_DIR);
  int count = 0;
  for (const auto &entry : fs::recursive_directory_iterator(dir)) {
    if (!entry.is_regular_file())
      continue;
    ++count;
    const std::string text = read_file(entry.path());
    parse_clean(text, entry.path().string().c_str());
  }
  check(count == 1, "example/ holds the one upstream corpus file");
}

// The corpus's only document: `HAI 1.3` / `VISIBLE "Hai world!"` /
// `KTHXBYE`. HAI's version number is read as an optional
// trailing atom.
void hai_example_values() {
  const std::string text = read_file(fs::path(CURSIV_EXAMPLE_DIR) / "hai.lol");
  ast::Program *prog = parse_clean(text, "example/hai.lol");
  check(prog != nullptr, "hai.lol builds a Program");
  if (prog == nullptr)
    return;

  const ast::NumLit *version = as<ast::NumLit>(prog->version);
  check(version != nullptr && version->value == 1.3, "hai.lol version is 1.3");
  check(prog->hasKthxbye, "hai.lol closes with KTHXBYE");
  check(prog->statements.size() == 1, "hai.lol has one statement");
  if (prog->statements.size() != 1)
    return;

  const ast::PrintBlock *pb = as<ast::PrintBlock>(prog->statements[0]);
  check(pb != nullptr && pb->items.size() == 1,
        "hai.lol VISIBLE takes one item");
  if (pb == nullptr || pb->items.size() != 1)
    return;
  const ast::StrLit *s = as<ast::StrLit>(pb->items[0]);
  check(s != nullptr && s->text == "Hai world!", "hai.lol prints Hai world!");
}

void full_example_values() {
  const std::string text =
      read_file(fs::path(CURSIV_TEST_DIR) / "data" / "full.lol");
  ast::Program *prog = parse_clean(text, "tests/data/full.lol");
  check(prog != nullptr, "full.lol builds a Program");
  if (prog == nullptr)
    return;

  check(prog->hasKthxbye, "full.lol closes with KTHXBYE");
  const ast::NumLit *version = as<ast::NumLit>(prog->version);
  check(version != nullptr && version->value == 1.2, "full.lol version is 1.2");

  check(prog->statements.size() == 34, "full.lol has 34 statements");
  if (prog->statements.size() != 34)
    return;
  auto &st = prog->statements;

  const ast::Declaration *d0 = as<ast::Declaration>(st[0]);
  check(d0 != nullptr && d0->name == "x" && d0->init == nullptr,
        "x is declared with no initializer");

  const ast::Declaration *d1 = as<ast::Declaration>(st[1]);
  const ast::NumLit *d1v = d1 != nullptr ? as<ast::NumLit>(d1->init) : nullptr;
  check(d1 != nullptr && d1->name == "y" && d1v != nullptr && d1v->value == 5.0,
        "y is declared ITZ 5");

  const ast::Comment *c0 = as<ast::Comment>(st[6]);
  check(c0 != nullptr && c0->text.substr(0, 3) == "BTW", "a BTW line comment");
  const ast::Comment *c1 = as<ast::Comment>(st[7]);
  check(c1 != nullptr && c1->text.substr(0, 4) == "OBTW" &&
            c1->text.find("TLDR") != std::string_view::npos,
        "an OBTW/TLDR block comment");

  const ast::PrintBlock *p1 = as<ast::PrintBlock>(st[9]);
  check(p1 != nullptr && p1->items.size() == 3,
        "VISIBLE x y z MKAY? holds 3 items");

  const ast::Assignment *a0 = as<ast::Assignment>(st[12]);
  const ast::BinOp *a0v = a0 != nullptr ? as<ast::BinOp>(a0->value) : nullptr;
  check(a0 != nullptr && a0->name == "x" && a0v != nullptr &&
            a0v->op == ast::Op::add,
        "x R SUM OF 1 AN 2 is an add BinOp");
  if (a0v != nullptr) {
    const ast::NumLit *l = as<ast::NumLit>(a0v->left);
    const ast::NumLit *r = as<ast::NumLit>(a0v->right);
    check(l != nullptr && l->value == 1.0 && r != nullptr && r->value == 2.0,
          "SUM OF 1 AN 2 reads 1 and 2");
  }

  const ast::Assignment *a1 = as<ast::Assignment>(st[26]);
  const ast::CastExpr *a1v =
      a1 != nullptr ? as<ast::CastExpr>(a1->value) : nullptr;
  check(a1 != nullptr && a1->name == "greeting" && a1v != nullptr &&
            a1v->targetType == "YARN",
        "greeting R MAEK x A YARN casts to YARN");

  const ast::FuncDecl *f0 = as<ast::FuncDecl>(st[27]);
  check(f0 != nullptr && f0->name == "add" && f0->params.size() == 2 &&
            f0->params[0] == "aa" && f0->params[1] == "bb" &&
            f0->body.size() == 1,
        "HOW DUZ I add YR aa AN YR bb takes two params");

  const ast::FuncCall *call = as<ast::FuncCall>(st[29]);
  check(call != nullptr && call->callee == "add" && call->args.size() == 2,
        "add x y MKAY? calls add with two arguments");

  const ast::LoopStmt *loop = as<ast::LoopStmt>(st[30]);
  check(loop != nullptr && loop->label == "loopvar" &&
            loop->endLabel == "loopvar" && loop->body.size() == 1,
        "IM IN YR loopvar ... IM OUTTA YR loopvar");

  const ast::IfBlock *if0 = as<ast::IfBlock>(st[31]);
  check(if0 != nullptr && if0->body.size() == 1 && if0->elseChain == nullptr,
        "a plain O RLY?/YA RLY/OIC has no else chain");

  const ast::IfBlock *if1 = as<ast::IfBlock>(st[32]);
  const ast::MebbeClause *mebbe =
      if1 != nullptr ? as<ast::MebbeClause>(if1->elseChain) : nullptr;
  check(if1 != nullptr && mebbe != nullptr && mebbe->body.size() == 1,
        "a MEBBE clause chains off the if");
  const ast::NoWaiClause *nowai =
      mebbe != nullptr ? as<ast::NoWaiClause>(mebbe->next) : nullptr;
  check(nowai != nullptr && nowai->body.size() == 1,
        "the MEBBE chain closes with NO WAI");

  const ast::VarRef *v0 = as<ast::VarRef>(st[33]);
  check(v0 != nullptr && v0->name == "x",
        "a bare LABEL is an expression statement");
}

// A run of bare names ending in MKAY? has two grammatically complete
// readings of the identical span: one call absorbing every name as its own
// argument, or a shorter call preceded by independent bare-name statements.
// The engine consistently takes the one-call, maximal-argument reading;
// pinned here since the two cannot be made disjoint by a first token (both
// start on a plain name) -- confirmed across a plain run, a longer run, and
// a run whose last argument is itself a full expression, not just a name.
void call_absorbs_every_trailing_name() {
  const std::string text1 = "HAI\nX Y Z MKAY?\nKTHXBYE\n";
  ast::Program *p1 = parse_clean(text1, "call/3 names");
  if (p1 != nullptr) {
    check(p1->statements.size() == 1, "call/3 names: one statement");
    const auto *fc = cursiv::ast_cast<ast::FuncCall>(p1->statements[0]);
    check(fc != nullptr && fc->callee == "X" && fc->args.size() == 2,
          "call/3 names: X(Y, Z)");
  }

  const std::string text2 = "HAI\nX Y Z W MKAY?\nKTHXBYE\n";
  ast::Program *p2 = parse_clean(text2, "call/4 names");
  if (p2 != nullptr) {
    check(p2->statements.size() == 1, "call/4 names: one statement");
    const auto *fc = cursiv::ast_cast<ast::FuncCall>(p2->statements[0]);
    check(fc != nullptr && fc->callee == "X" && fc->args.size() == 3,
          "call/4 names: X(Y, Z, W)");
  }

  const std::string text3 = "HAI\nX BOTH SAEM Y AN Z MKAY?\nKTHXBYE\n";
  ast::Program *p3 = parse_clean(text3, "call/expr argument");
  if (p3 != nullptr) {
    check(p3->statements.size() == 1, "call/expr argument: one statement");
    const auto *fc = cursiv::ast_cast<ast::FuncCall>(p3->statements[0]);
    check(fc != nullptr && fc->callee == "X" && fc->args.size() == 1,
          "call/expr argument: X(BOTH SAEM Y AN Z)");
  }
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<lolcode::LolcodeCoreServices,
                                lolcode::LolcodeSharedCoreServices,
                                lolcode::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "HAI\nI HAS A x ITZ I IZ f YR 1 MKAY\nKTHXBYE\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool found = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *call = cursiv::ast_cast<lolcode::ast::FuncCall>(node))
        found = call->callee == "f" && call->args.size() == 1;
    if (!found)
      return 1;
  }

  every_example_parses_clean();
  hai_example_values();
  full_example_values();
  call_absorbs_every_trailing_name();
  if (failures != 0) {
    std::fprintf(stderr, "lolcode: %d failure(s)\n", failures);
    return 1;
  }
  std::puts("lolcode: the grammar test passes");
  return 0;
}
