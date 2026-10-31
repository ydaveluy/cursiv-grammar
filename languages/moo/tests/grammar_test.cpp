
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "moo/core/CoreServices.hpp"
#include "moo/core/ast.hpp"
#include "conditional_fork_cases.hpp"
#include "native_regressions.hpp"
// The smoke test of the language: every corpus document parses clean, and
// one is read back field by field to catch a converter that silently
// mismaps its value.

#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "moo: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "cupcake.moo", "dest.moo",   "drone.moo", "hello.moo",
    "strike.moo",  "target.moo", "toy.moo",
};

// The document text and its ParseResult must outlive every read of the
// tree: both stay local to this function, never returned from a helper.
// Null when the document did not parse clean.
const moo::ast::Expr* if_cond(const cursiv::ParseResult& result) {
  if (result.cst().recovered() || result.value == nullptr) return nullptr;
  const auto& prog = *static_cast<const moo::ast::Prog*>(result.value);
  if (prog.declarations.empty()) return nullptr;
  const auto& decl =
      *static_cast<const moo::ast::ProgramDecl*>(prog.declarations[0]);
  if (decl.body.empty()) return nullptr;
  const auto& ifStmt = *static_cast<const moo::ast::IfStmt*>(decl.body[0]);
  return ifStmt.cond;
}

// LambdaMOO's parser.y gives && and || the same left-associative level:
// `1==2 || 3==4 && 5==6` is `((1==2)||(3==4))&&(5==6)`.
int check_condition_precedence(moo::MooCoreServices& language) {
  {
    const std::string text =
        "@program prog:run\nif (1==2 && 3==4)\n  return 1;\nendif\n.";
    const cursiv::ParseResult result = language.parser->parse(text);
    const auto* cond = if_cond(result);
    if (cond == nullptr) return fail("'1==2 && 3==4' probe does not parse clean");
    const auto* top = static_cast<const moo::ast::BinaryExpr*>(cond);
    if (top->op != moo::ast::Operator::logical_and)
      return fail("'1==2 && 3==4' is not top-level &&");
    if (top->left->type != &cursiv::ast_type_tag<moo::ast::BinaryExpr>)
      return fail("'1==2 && 3==4' left operand is not a comparison");
    const auto* left = static_cast<const moo::ast::BinaryExpr*>(top->left);
    if (left->op != moo::ast::Operator::equal)
      return fail("'1==2 && 3==4' left operand is not a comparison");
  }
  {
    const std::string text =
        "@program prog:run\nif (1==2 || 3==4 && 5==6)\n  return 1;\nendif\n.";
    const cursiv::ParseResult result = language.parser->parse(text);
    const auto* cond = if_cond(result);
    if (cond == nullptr)
      return fail("'1==2 || 3==4 && 5==6' probe does not parse clean");
    const auto* top = static_cast<const moo::ast::BinaryExpr*>(cond);
    if (top->op != moo::ast::Operator::logical_and)
      return fail("'1==2 || 3==4 && 5==6' is not top-level &&");
    const auto* left = cursiv::ast_cast<moo::ast::BinaryExpr>(top->left);
    if (!left || left->op != moo::ast::Operator::logical_or)
      return fail("'1==2 || 3==4 && 5==6' left operand is not ||");
  }
  for (const auto *expression : {"2^3^4", "!1+2"}) {
    const std::string text = "@program prog:run\nif (" +
        std::string(expression) + ")\nreturn 1;\nendif\n.";
    const auto result = language.parser->parse(text);
    const auto *top = cursiv::ast_cast<moo::ast::BinaryExpr>(if_cond(result));
    if (!top || !result.diagnostics.empty())
      return fail("native expression precedence probe did not build a binary expression");
    if (expression == std::string_view("2^3^4")) {
      const auto *right = cursiv::ast_cast<moo::ast::BinaryExpr>(top->right);
      if (top->op != moo::ast::Operator::power || !right ||
          right->op != moo::ast::Operator::power)
        return fail("exponentiation must associate right");
    } else if (top->op != moo::ast::Operator::add ||
               !cursiv::ast_cast<moo::ast::NotExpr>(top->left)) {
      return fail("logical negation must bind before arithmetic");
    }
  }
  return 0;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<moo::MooCoreServices, moo::MooSharedCoreServices,
                              moo::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "@program o:v\nwhile (1)\nx = 1;\nendwhile\n.");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r =
        language.language.parser->parse("@program o:v\nreturn $x;\n.");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
  }
  {
    const auto r = language.language.parser->parse(
        ";disabled client command\n@program o:v\nx=1; y=2;\n.");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* doc = r.value_as<moo::ast::Prog>();
    const auto* program =
        doc && doc->declarations.size() == 1
            ? cursiv::ast_cast<moo::ast::ProgramDecl>(doc->declarations[0])
            : nullptr;
    if (!program || program->body.size() != 2) return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "@program o:v\nif (1) endif\nwhile (0) endwhile\nreturn time();\n.");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      moo::MooCoreServices, moo::MooSharedCoreServices,
      moo::install_core_module>();

  if (!moo_conditional_fork_cases::run(language.language)) return 1;

  if (!moo_native::run(*language.language.parser)) return 1;

  {
    const std::string source =
        R"MOO(@program hello:run
x = {325.0, 325., 3.25e2, 0.325E3, .0325e+4, 32500e-2};
s = "His name was \"Leroy\" and \\.";
.)MOO";
    const auto result = language.language.parser->parse(source);
    const auto *root = result.value_as<moo::ast::Prog>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !root || root->declarations.size() != 1)
      return fail("native real literals and escaped quotes parse cleanly");
    const auto *program = cursiv::ast_cast<moo::ast::ProgramDecl>(root->declarations[0]);
    if (!program || program->body.size() != 2)
      return fail("native literal test preserves both assignments");
    const auto *numbers = cursiv::ast_cast<moo::ast::AssignStmt>(program->body[0]);
    const auto *list = numbers ? cursiv::ast_cast<moo::ast::ListExpr>(numbers->value) : nullptr;
    if (!list || list->items.size() != 6)
      return fail("native real literals are six separate values");
    for (const auto *item : list->items) {
      const auto *number = cursiv::ast_cast<moo::ast::RealLit>(item);
      if (!number || number->value != 325.0)
        return fail("native real literal retains its numeric value");
    }
    const auto *assignment = cursiv::ast_cast<moo::ast::AssignStmt>(program->body[1]);
    const auto *string = assignment ? cursiv::ast_cast<moo::ast::StringLit>(assignment->value) : nullptr;
    if (!string || string->value != "His name was \"Leroy\" and \\.")
      return fail("MOO quote and backslash escapes decode once");
  }

  for (const auto *source : {"@programhello:run\nreturn 1;\n.", "@kidsfoo"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("a directive must not consume the prefix of a longer word");
  }

  if (const int rc = check_condition_precedence(language.language); rc != 0)
    return rc;

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty()) return fail((std::string(name) + ": empty or unreadable").c_str());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail((std::string(name) + ": does not parse clean").c_str());
    if (!result.diagnostics.empty())
      return fail((std::string(name) + ": has diagnostics").c_str());

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<moo::ast::Prog>)
      return fail((std::string(name) + ": built no Prog").c_str());

    const auto& prog = *static_cast<const moo::ast::Prog*>(root);
    if (prog.declarations.empty())
      return fail((std::string(name) + ": Prog holds no declaration").c_str());
  }

  // features.moo covers the constructs the corpus never uses: try/while/do,
  // the relop chain, the declarations with no corpus example.
  {
    const std::string text = read_file(CURSIV_FEATURES_SAMPLE);
    if (text.empty()) return fail("features.moo: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail("features.moo: does not parse clean");
    if (!result.diagnostics.empty()) return fail("features.moo: has diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<moo::ast::Prog>)
      return fail("features.moo: built no Prog");
  }

  // hello.moo, read back field by field.
  const std::string helloText =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/hello.moo");
  const cursiv::ParseResult result = language.language.parser->parse(helloText);
  const auto& prog = *static_cast<const moo::ast::Prog*>(result.value);
  if (prog.declarations.size() != 1) return fail("hello.moo: expected one declaration");

  if (prog.declarations[0]->type != &cursiv::ast_type_tag<moo::ast::ProgramDecl>)
    return fail("hello.moo: declaration is not a ProgramDecl");
  const auto& decl =
      *static_cast<const moo::ast::ProgramDecl*>(prog.declarations[0]);
  if (decl.object != "hello") return fail("hello.moo: wrong object name");
  if (decl.verb != "run") return fail("hello.moo: wrong verb name");
  if (decl.body.size() != 1) return fail("hello.moo: expected one statement");

  const auto& cmd = *static_cast<const moo::ast::CommandStmt*>(decl.body[0]);
  const auto* const* const callPtr = std::get_if<moo::ast::VerbCall*>(&cmd.command);
  if (callPtr == nullptr || *callPtr == nullptr)
    return fail("hello.moo: statement is not a verb call");
  const moo::ast::VerbCall& call = **callPtr;
  if (call.target == nullptr || call.target->base != "player")
    return fail("hello.moo: wrong call target");
  if (call.verb != "tell") return fail("hello.moo: wrong verb");
  if (call.args.size() != 1) return fail("hello.moo: expected one argument");

  if (call.args[0]->type != &cursiv::ast_type_tag<moo::ast::StringLit>)
    return fail("hello.moo: wrong argument node type");
  const auto& arg = *static_cast<const moo::ast::StringLit*>(call.args[0]);
  if (arg.value != "Hello to the world of MOO!")
    return fail("hello.moo: wrong argument value");

  std::puts("moo: the corpus parses");
  return 0;
}
