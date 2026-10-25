// peoplecode end to end: every document under example/ parses clean, and a
// handful of real field values are read back out of the tree -- a converter
// that silently mis-reads a value passes every structural check alone.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "peoplecode/core/ast.hpp"
#include "peoplecode/generated/Peoplecode/Module.hpp"

namespace {

namespace ast = peoplecode::ast;
namespace fs = std::filesystem;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      peoplecode::generated::Peoplecode::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  return cursiv::test::read_file(path.string());
}

// Every example, sorted so a failure is reported at a stable line.
void corpus() {
  std::vector<fs::path> files;
  const fs::path dirs[] = {fs::path(CURSIV_EXAMPLE_DIR) / "example",
                           fs::path(CURSIV_TEST_DIR)};
  for (const fs::path& dir : dirs)
    for (const auto& entry : fs::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".pc")
        files.push_back(entry.path());
  std::sort(files.begin(), files.end());
  CHECK(!files.empty());

  for (const fs::path& file : files) {
    const std::string src = read_file(file);
    const cursiv::ParseResult r = engine().parse(src);
    const bool clean =
        r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    if (!clean)
      std::printf("FAIL %s: accepted=%d recovered=%d diagnostics=%zu\n",
                  file.filename().string().c_str(), r.accepted,
                  r.cst().recovered(), r.diagnostics.size());
    CHECK(clean);
    CHECK(r.value != nullptr &&
          r.value->type == &cursiv::ast_type_tag<ast::Program>);
  }
}

// Real values read back from one document: a Constant's numeric value, a
// method's name, an app-class import's joined path, and the shape of a
// nested if/for/while chain -- every one a place a converter could silently
// mis-read while every structural check above still passes.
void field_values() {
  const std::string src = read_file(
      fs::path(CURSIV_EXAMPLE_DIR) /
      "example/AppClassPC.SSF_SS_PMT.SSF_Student.Student.OnExecute.pc");
  const cursiv::ParseResult r = engine().parse(src);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Program* const prog = r.value_as<ast::Program>();
  CHECK(prog != nullptr);
  if (prog == nullptr) return;

  // The five leading imports, the first one a plain package wildcard.
  int imports = 0;
  for (ast::Stmt* const s : prog->statements) {
    const ast::ImportStmt* const imp = cursiv::ast_cast<ast::ImportStmt>(s);
    if (imp == nullptr) continue;
    ++imports;
    if (imports == 1) {
      CHECK(imp->target == "SSF_SS_PMT:EXCEPTION");
      CHECK(imp->wildcard);
    }
  }
  CHECK(imports == 5);

  // The class declaration: its name, a public property, a Constant's value,
  // and the private block that follows the public one.
  const ast::ClassDecl* classDecl = nullptr;
  for (ast::Stmt* const s : prog->statements) {
    classDecl = cursiv::ast_cast<ast::ClassDecl>(s);
    if (classDecl != nullptr) break;
  }
  CHECK(classDecl != nullptr && classDecl->name == "Student");
  CHECK(classDecl != nullptr && classDecl->blocks.size() == 2);
  if (classDecl == nullptr || classDecl->blocks.size() != 2) return;
  CHECK(!classDecl->blocks[0]->isPrivate);
  CHECK(classDecl->blocks[1]->isPrivate);

  const ast::PropertyDecl* prop = nullptr;
  for (ast::ClassMember* const m : classDecl->blocks[0]->members) {
    prop = cursiv::ast_cast<ast::PropertyDecl>(m);
    if (prop != nullptr) break;
  }
  CHECK(prop != nullptr && prop->name == "StdntISet" &&
        prop->varType == "string");

  const ast::ConstantDecl* constant = nullptr;
  for (ast::ClassMember* const m : classDecl->blocks[1]->members) {
    constant = cursiv::ast_cast<ast::ConstantDecl>(m);
    if (constant != nullptr) break;
  }
  CHECK(constant != nullptr && constant->name == "&msgNoInstSet");
  const ast::IntLit* const constVal =
      constant != nullptr ? cursiv::ast_cast<ast::IntLit>(constant->value)
                          : nullptr;
  CHECK(constVal != nullptr && constVal->value == 17);

  // A method implementation: an if/else with a nested throw create() call.
  const ast::MethodImplStmt* method = nullptr;
  for (ast::Stmt* const s : prog->statements) {
    const ast::MethodImplStmt* const m = cursiv::ast_cast<ast::MethodImplStmt>(s);
    if (m != nullptr && m->name == "GetStudentIset") {
      method = m;
      break;
    }
  }
  CHECK(method != nullptr);
  if (method == nullptr) return;
  bool sawThrow = false;
  for (ast::Stmt* const s : method->body) {
    const ast::IfStmt* const iff = cursiv::ast_cast<ast::IfStmt>(s);
    if (iff == nullptr) continue;
    for (ast::Stmt* const inner : iff->thenBranch) {
      if (cursiv::ast_cast<ast::ThrowStmt>(inner) != nullptr) sawThrow = true;
    }
    for (ast::Stmt* const inner : iff->elseBranch) {
      if (cursiv::ast_cast<ast::ThrowStmt>(inner) != nullptr) sawThrow = true;
    }
  }
  CHECK(sawThrow);
}

// The document written for this port, in tests/: a block comment, a decimal
// literal, a property with set/readonly, a For loop with no Step, and the
// ">" branch of Evaluate together with When-Other.
void coverage_document() {
  const std::string src = read_file(fs::path(CURSIV_TEST_DIR) / "coverage.pc");
  const cursiv::ParseResult r = engine().parse(src);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Program* const prog = r.value_as<ast::Program>();
  CHECK(prog != nullptr);
  if (prog == nullptr) return;

  const ast::ClassDecl* classDecl = nullptr;
  for (ast::Stmt* const s : prog->statements) {
    classDecl = cursiv::ast_cast<ast::ClassDecl>(s);
    if (classDecl != nullptr) break;
  }
  CHECK(classDecl != nullptr && classDecl->blocks.size() == 1);
  if (classDecl == nullptr || classDecl->blocks.empty()) return;
  const ast::PropertyDecl* rate = nullptr;
  for (ast::ClassMember* const m : classDecl->blocks[0]->members) {
    const ast::PropertyDecl* const p = cursiv::ast_cast<ast::PropertyDecl>(m);
    if (p != nullptr && p->name == "Rate") rate = p;
  }
  CHECK(rate != nullptr && rate->hasGet && rate->hasSet && !rate->readonly);

  const ast::MethodImplStmt* compute = nullptr;
  for (ast::Stmt* const s : prog->statements) {
    const ast::MethodImplStmt* const m = cursiv::ast_cast<ast::MethodImplStmt>(s);
    if (m != nullptr && m->name == "Compute") compute = m;
  }
  CHECK(compute != nullptr);
  if (compute == nullptr) return;
  const ast::ForStmt* forStmt = nullptr;
  const ast::EvaluateStmt* evalStmt = nullptr;
  for (ast::Stmt* const s : compute->body) {
    if (const ast::ForStmt* const f = cursiv::ast_cast<ast::ForStmt>(s))
      forStmt = f;
    if (const ast::EvaluateStmt* const e = cursiv::ast_cast<ast::EvaluateStmt>(s))
      evalStmt = e;
  }
  CHECK(forStmt != nullptr && forStmt->step == nullptr);
  CHECK(evalStmt != nullptr && evalStmt->hasOtherBranch);
  CHECK(evalStmt != nullptr && evalStmt->branches.size() == 1 &&
        evalStmt->branches[0]->op == ">");

  const ast::DecimalLit* pi = nullptr;
  for (ast::Stmt* const s : compute->body) {
    const ast::VarDeclStmt* const decl = cursiv::ast_cast<ast::VarDeclStmt>(s);
    if (decl == nullptr) continue;
    for (ast::VarDeclarator* const v : decl->declarators)
      if (const ast::DecimalLit* const d = cursiv::ast_cast<ast::DecimalLit>(v->init))
        pi = d;
  }
  CHECK(pi != nullptr && pi->value == 3.14);
}

// `And` binds tighter than `Or` (Oracle PeopleCode Language Reference,
// "Boolean Operators": "Not takes the highest precedence, And is next, and
// Or is lowest"), so `&a Or &b And &c` reads `&a Or (&b And &c)`.
void precedence() {
  const std::string src =
      "If &a Or &b And &c Then\n"
      "   &x = 1;\n"
      "End-If;\n";
  const cursiv::ParseResult r = engine().parse(src);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Program* const prog = r.value_as<ast::Program>();
  CHECK(prog != nullptr && !prog->statements.empty());
  if (prog == nullptr || prog->statements.empty()) return;

  const ast::IfStmt* const ifStmt =
      cursiv::ast_cast<ast::IfStmt>(prog->statements[0]);
  CHECK(ifStmt != nullptr);
  if (ifStmt == nullptr) return;
  const auto* const root = cursiv::ast_cast<ast::BinaryExpr>(ifStmt->cond);
  CHECK(root != nullptr && root->op == ast::BinOp::logical_or);
  if (root == nullptr) return;
  const auto* const right = cursiv::ast_cast<ast::BinaryExpr>(root->right);
  CHECK(right != nullptr && right->op == ast::BinOp::logical_and);
}

}  // namespace

int main() {
  corpus();
  field_values();
  coverage_document();
  precedence();
  return cursiv::test::report("peoplecode_grammar_test");
}
