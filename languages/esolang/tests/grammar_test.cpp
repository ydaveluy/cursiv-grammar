// The grammar test: every file of example/ parses clean (no repair, no
// diagnostics, a Program root), and tests/data/full.lol -- built for this
// port, exercising every construct of the language -- checks real field
// values, not only structure.

#include <cstdio>
#include <deque>
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
#include "switch_cases.hpp"

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
  // AST allocations belong to ParseResult; retain it for the callers' reads.
  const auto& p = parser();
  static std::deque<std::string> sources;
  static std::deque<cursiv::ParseResult> results;
  sources.push_back(text);
  results.push_back(p.parse(sources.back()));
  const auto& result = results.back();
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
  check(version != nullptr && version->text == "1.3" && version->numbar,
        "hai.lol version is NUMBAR 1.3");
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
  check(version != nullptr && version->text == "1.2" && version->numbar,
        "full.lol version is NUMBAR 1.2");

  check(prog->statements.size() == 34, "full.lol has 34 statements");
  if (prog->statements.size() != 34)
    return;
  auto &st = prog->statements;

  const ast::Declaration *d0 = as<ast::Declaration>(st[0]);
  check(d0 != nullptr && d0->name == "x" && d0->init == nullptr,
        "x is declared with no initializer");

  const ast::Declaration *d1 = as<ast::Declaration>(st[1]);
  const ast::NumLit *d1v = d1 != nullptr ? as<ast::NumLit>(d1->init) : nullptr;
  check(d1 != nullptr && d1->name == "y" && d1v != nullptr &&
            d1v->text == "5" && !d1v->numbar,
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
    check(l != nullptr && l->text == "1" && !l->numbar &&
              r != nullptr && r->text == "2" && !r->numbar,
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

void native_spec_regressions() {
  for (const std::string source : {
      "HAI 1.2\nKTHXBYE\n",
      "HAI 1.2, I\tHAS   A x ITZ -2, VISIBLE x, x R SUM\tOF x 1, KTHXBYE",
      "HAI\nHOW IZ I f YR p\nFOUND YR p\nIF U SAY SO\nKTHXBYE\n",
      "HAI\nHOW IZ I empty\nIF U SAY SO\nKTHXBYE\n",
      "HAI\nIM IN YR infinite\nGTFO\nIM OUTTA YR infinite\nKTHXBYE\n",
      "HAI\nI HAS A x ITZ ALL OF WIN AN FAIL BTW inline comment\nKTHXBYE\n",
      "HAI\nI HAS A x ITZ WON OF WIN FAIL\nx IS NOW A NUMBAR\nKTHXBYE\n",
      "HAI\nI HAS A x ITZ MAEK 1 YARN\nI HAS A y ITZ SMOOSH x AN 2 MKAY\nKTHXBYE\n"}) {
    parse_clean(source, "native LOLCODE 1.2 syntax");
  }
  {
    const std::string source = "HAI\nVISIBLE 1\nx\nKTHXBYE\n";
    const auto* p = parse_clean(source, "newline ends VISIBLE arguments");
    const auto* print = p && p->statements.size() == 2 ?
        cursiv::ast_cast<ast::PrintBlock>(p->statements[0]) : nullptr;
    const auto* name = p && p->statements.size() == 2 ?
        cursiv::ast_cast<ast::VarRef>(p->statements[1]) : nullptr;
    check(print && print->items.size() == 1 && name && name->name == "x",
          "VISIBLE must not consume the following expression statement");
  }
  {
    const std::string source = "HAI\nVISIBLE \"a\" ...\n \"b\"\nx\nKTHXBYE\n";
    const auto* p = parse_clean(source, "continued print");
    const auto* print = p && p->statements.size() == 2 ?
        cursiv::ast_cast<ast::PrintBlock>(p->statements[0]) : nullptr;
    check(print && print->items.size() == 2, "continuation joins one print statement");
  }
  {
    const std::string source = R"(HAI
VISIBLE "a:"b"!
IM IN YR counted UPPIN YR i TIL BOTH SAEM i AN 3
GTFO
IM OUTTA YR counted
KTHXBYE
)";
    const auto* p = parse_clean(source, "native string and counted loop");
    const auto* print = p && p->statements.size() == 2 ?
        cursiv::ast_cast<ast::PrintBlock>(p->statements[0]) : nullptr;
    const auto* loop = p && p->statements.size() == 2 ?
        cursiv::ast_cast<ast::LoopStmt>(p->statements[1]) : nullptr;
    const auto* string = print && print->items.size() == 1 ?
        cursiv::ast_cast<ast::StrLit>(print->items[0]) : nullptr;
    check(print && print->suppressNewline && string && string->text == "a:\"b" &&
          loop && loop->operation == "UPPIN" && loop->iterator == "i" && loop->until &&
          loop->condition && loop->body.size() == 1 &&
          cursiv::ast_cast<ast::BreakStmt>(loop->body[0]),
          "colon escape spelling, print flag and loop controls are retained");
  }
  for (const std::string source : {"HAI\nVISIBLE 1\n", "HAI\nVISIBLE .\nKTHXBYE\n"}) {
    const auto parsed = parser().parse(source);
    check(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty(),
          "missing program close or lone numeric dot rejected");
  }
}

void numeric_literal_regressions() {
  struct Case { std::string text; bool numbar; };
  const std::vector<Case> cases = {
      {"0", false}, {"-0", false}, {"1", false}, {"-1", false},
      {"9007199254740992", false}, {"9007199254740993", false},
      {"9223372036854775807", false}, {"-9223372036854775808", false},
      {"00042", false}, {std::string(320, '9'), false},
      {"1.0", true}, {"-0.0", true}, {"1.", true}, {"-1.", true},
      {".5", true}, {"-.5", true}, {"1.0000000000000001", true},
      {"12345678901234567890.123456789", true},
      {"0." + std::string(320, '0') + "1", true},
  };
  for (const auto& test : cases) {
    const std::string source = "HAI 1.2\nI HAS A n ITZ " + test.text +
        "\nVISIBLE SUM OF n AN " + test.text + "\nKTHXBYE\n";
    const auto parsed = parser().parse(source);
    const auto* program = parsed.value_as<ast::Program>();
    check(parsed.accepted && !parsed.cst().recovered() &&
              parsed.diagnostics.empty() && parsed.cst().text() == source &&
              program && program->statements.size() == 2,
          "numeric literal document parses without precision conversion");
    if (!program || program->statements.size() != 2) continue;
    const auto* declaration = cursiv::ast_cast<ast::Declaration>(program->statements[0]);
    const auto* literal = declaration ? cursiv::ast_cast<ast::NumLit>(declaration->init) : nullptr;
    const auto* print = cursiv::ast_cast<ast::PrintBlock>(program->statements[1]);
    const auto* sum = print && print->items.size() == 1 ?
        cursiv::ast_cast<ast::BinOp>(print->items[0]) : nullptr;
    const auto* right = sum ? cursiv::ast_cast<ast::NumLit>(sum->right) : nullptr;
    check(literal && right && literal->text == test.text && right->text == test.text &&
              literal->numbar == test.numbar && right->numbar == test.numbar,
          "NUMBR/NUMBAR category and complete spelling survive both expression contexts");
    check(declaration && declaration->parent == program && literal && literal->parent == declaration &&
              print && print->parent == program && sum && sum->parent == print &&
              right && right->parent == sum,
          "numeric literal expressions belong to their immediate owners");
  }
  for (const std::string_view invalid : {".", "--1", "1.2.3", "1e3"}) {
    const auto parsed = parser().parse("HAI 1.2\nI HAS A n ITZ " + std::string(invalid) + "\nKTHXBYE\n");
    check(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty(),
          "neighboring malformed numeric initializer is rejected");
  }
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<lolcode::LolcodeCoreServices,
                                lolcode::LolcodeSharedCoreServices,
                                lolcode::install_core_module>();
    if (!lolcode_switch_cases::run(audit_language.language))
      return 1;
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

  native_spec_regressions();
  numeric_literal_regressions();
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
