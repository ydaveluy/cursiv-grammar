#include "audit_regressions.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "alloy/core/CoreServices.hpp"
#include "alloy/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/Check.hpp"

namespace {

namespace ast = alloy::ast;

int fail(const std::string &what) {
  std::fprintf(stderr, "alloy: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser &engine() {
  static const auto language =
      cursiv::create_language<alloy::AlloyCoreServices,
                              alloy::AlloySharedCoreServices,
                              alloy::install_core_module>();
  return *language.language.parser;
}

template <class T> T *as(cursiv::AstNode *n) {
  return n != nullptr ? const_cast<T *>(cursiv::ast_cast<T>(n)) : nullptr;
}

struct ExpressionProbes {
  struct Owned {
    std::string source;
    cursiv::ParseResult result;

    explicit Owned(std::string_view probe)
        : source("sig A {}\nfact f { " + std::string(probe) + " }\n"),
          result(engine().parse(source)) {}
  };
  std::vector<std::unique_ptr<Owned>> owned;

  ast::Expr *parse(std::string_view probe) {
    owned.push_back(std::make_unique<Owned>(probe));
    const auto &result = owned.back()->result;
    CHECK(result.accepted && !result.cst().recovered());
    CHECK(result.diagnostics.empty());
    ast::Module *mod = as<ast::Module>(result.value);
    CHECK(mod != nullptr && mod->paragraphs.size() == 2);
    if (mod == nullptr || mod->paragraphs.size() != 2)
      return nullptr;
    ast::FactDecl *f = as<ast::FactDecl>(mod->paragraphs[1]);
    CHECK(f != nullptr && f->body != nullptr && f->body->exprs.size() == 1);
    if (f == nullptr || f->body == nullptr || f->body->exprs.size() != 1)
      return nullptr;
    return f->body->exprs[0];
  }
};

bool name_is(ast::Expr *e, std::string_view n) {
  ast::NameExpr *name = as<ast::NameExpr>(e);
  return name != nullptr && name->name == n;
}

ast::BinaryExpr *bin(ast::Expr *e, ast::BinOp op) {
  ast::BinaryExpr *b = as<ast::BinaryExpr>(e);
  return (b != nullptr && b->op == op) ? b : nullptr;
}

ast::UnaryExpr *un(ast::Expr *e, ast::UnaryOp op) {
  ast::UnaryExpr *u = as<ast::UnaryExpr>(e);
  return (u != nullptr && u->op == op) ? u : nullptr;
}
void dot_binds_tighter_than_box_join() {
  ExpressionProbes probes;
  ast::BoxJoinExpr *box = as<ast::BoxJoinExpr>(probes.parse("a.b[c]"));
  CHECK(box != nullptr && box->args.size() == 1);
  if (box == nullptr || box->args.size() != 1)
    return;
  CHECK(name_is(box->args[0], "c"));
  ast::BinaryExpr *join = bin(box->callee, ast::BinOp::join);
  CHECK(join != nullptr && name_is(join->left, "a") &&
        name_is(join->right, "b"));
}
void long_chain() {
  ExpressionProbes probes;
  ast::BinaryExpr *ov =
      bin(probes.parse("~a.b[c]<:d->e&f++g"), ast::BinOp::override_);
  CHECK(ov != nullptr);
  if (ov == nullptr)
    return;
  CHECK(name_is(ov->right, "g"));
  ast::BinaryExpr *isect = bin(ov->left, ast::BinOp::intersect);
  CHECK(isect != nullptr && name_is(isect->right, "f"));
  if (isect == nullptr)
    return;
  ast::ArrowExpr *arrow = as<ast::ArrowExpr>(isect->left);
  CHECK(arrow != nullptr && name_is(arrow->right, "e"));
  if (arrow == nullptr)
    return;
  ast::BinaryExpr *restrict_ = bin(arrow->left, ast::BinOp::domain_restrict);
  CHECK(restrict_ != nullptr && name_is(restrict_->right, "d"));
  if (restrict_ == nullptr)
    return;
  ast::BoxJoinExpr *box = as<ast::BoxJoinExpr>(restrict_->left);
  CHECK(box != nullptr && box->args.size() == 1 && name_is(box->args[0], "c"));
  if (box == nullptr)
    return;
  ast::BinaryExpr *join = bin(box->callee, ast::BinOp::join);
  CHECK(join != nullptr && name_is(join->right, "b"));
  if (join == nullptr)
    return;
  ast::UnaryExpr *transpose = un(join->left, ast::UnaryOp::transpose);
  CHECK(transpose != nullptr && name_is(transpose->operand, "a"));
}
void historical_identifiers() {
  ExpressionProbes probes;
  for (std::string_view name :
       {"\u1885x", "\u1886x", "\u30FBx", "\uFF65x", "x\u1369x", "x\u136Ax",
        "x\u136Bx", "x\u136Cx", "x\u136Dx", "x\u136Ex", "x\u136Fx", "x\u1370x",
        "x\u1371x"})
    CHECK(name_is(probes.parse(name), name));
}

void comprehension_and_block() {
  ExpressionProbes probes;
  const auto *block =
      as<ast::Block>(probes.parse("{p = FirstProc => a else b}"));
  CHECK(block && block->exprs.size() == 1);
  const auto *conditional = block && block->exprs.size() == 1
                                ? as<ast::IfExpr>(block->exprs[0])
                                : nullptr;
  const auto *condition =
      conditional ? as<ast::CompareExpr>(conditional->cond) : nullptr;
  CHECK(condition && condition->op == ast::CompareOp::eq &&
        name_is(condition->left, "p") &&
        name_is(condition->right, "FirstProc"));
  const auto *comprehension = as<ast::SetComprExpr>(probes.parse("{p:A}"));
  CHECK(comprehension && comprehension->decls.size() == 1 &&
        !comprehension->body);
}

void cardinality_tier() {
  ExpressionProbes probes;
  ast::UnaryExpr *card1 = un(probes.parse("#a++b"), ast::UnaryOp::card);
  CHECK(card1 != nullptr);
  if (card1 != nullptr) {
    ast::BinaryExpr *ov = bin(card1->operand, ast::BinOp::override_);
    CHECK(ov != nullptr && name_is(ov->left, "a") && name_is(ov->right, "b"));
  }

  ast::BinaryExpr *add = bin(probes.parse("#a+b"), ast::BinOp::add);
  CHECK(add != nullptr);
  if (add != nullptr) {
    CHECK(un(add->left, ast::UnaryOp::card) != nullptr);
    CHECK(name_is(add->right, "b"));
  }
}
void quantifier_prefix_tier() {
  ExpressionProbes probes;
  ast::UnaryExpr *some1 = un(probes.parse("some a+b"), ast::UnaryOp::some);
  CHECK(some1 != nullptr);
  if (some1 != nullptr) {
    ast::BinaryExpr *add = bin(some1->operand, ast::BinOp::add);
    CHECK(add != nullptr && name_is(add->left, "a") &&
          name_is(add->right, "b"));
  }

  ast::CompareExpr *cmp = as<ast::CompareExpr>(probes.parse("some a=b"));
  CHECK(cmp != nullptr && cmp->op == ast::CompareOp::eq);
  if (cmp != nullptr) {
    CHECK(un(cmp->left, ast::UnaryOp::some) != nullptr);
    CHECK(name_is(cmp->right, "b"));
  }
}
void general_negation_tier() {
  ExpressionProbes probes;
  ast::UnaryExpr *not1 = un(probes.parse("not a in b"), ast::UnaryOp::not_);
  CHECK(not1 != nullptr);
  if (not1 != nullptr) {
    ast::CompareExpr *cmp = as<ast::CompareExpr>(not1->operand);
    CHECK(cmp != nullptr && cmp->op == ast::CompareOp::in &&
          name_is(cmp->left, "a") && name_is(cmp->right, "b"));
  }

  ast::BinaryExpr *and_ = bin(probes.parse("not a and b"), ast::BinOp::and_);
  CHECK(and_ != nullptr);
  if (and_ != nullptr)
    CHECK(un(and_->left, ast::UnaryOp::not_) != nullptr);
}
void implication_tier() {
  ExpressionProbes probes;
  const auto *and_implies = as<ast::IfExpr>(probes.parse("p and q => r"));
  CHECK(and_implies && !and_implies->else_ && name_is(and_implies->then, "r") &&
        bin(and_implies->cond, ast::BinOp::and_));
  const auto *spelled = as<ast::IfExpr>(probes.parse("p implies q"));
  CHECK(spelled && !spelled->else_ && name_is(spelled->cond, "p") &&
        name_is(spelled->then, "q"));
  const auto *chain = as<ast::IfExpr>(probes.parse("p => q => r"));
  const auto *inner = chain ? as<ast::IfExpr>(chain->then) : nullptr;
  CHECK(chain && !chain->else_ && name_is(chain->cond, "p"));
  CHECK(inner && !inner->else_ && name_is(inner->cond, "q") &&
        name_is(inner->then, "r"));
  const auto *outer = as<ast::IfExpr>(probes.parse("p => q => r else s"));
  inner = outer ? as<ast::IfExpr>(outer->then) : nullptr;
  CHECK(outer && !outer->else_ && name_is(outer->cond, "p"));
  CHECK(inner && name_is(inner->cond, "q") && name_is(inner->then, "r") &&
        name_is(inner->else_, "s"));
  const auto *iff = bin(probes.parse("p => q <=> r"), ast::BinOp::iff);
  CHECK(iff && name_is(iff->right, "r") && as<ast::IfExpr>(iff->left));
}
void iff_and_or_tiers() {
  ExpressionProbes probes;
  ast::BinaryExpr *or_ = bin(probes.parse("p <=> q || r"), ast::BinOp::or_);
  CHECK(or_ != nullptr && name_is(or_->right, "r"));
  if (or_ != nullptr)
    CHECK(bin(or_->left, ast::BinOp::iff) != nullptr);

  ast::BinaryExpr *or_and = bin(probes.parse("a || b && c"), ast::BinOp::or_);
  CHECK(or_and != nullptr && name_is(or_and->left, "a"));
  if (or_and != nullptr) {
    ast::BinaryExpr *and_ = bin(or_and->right, ast::BinOp::and_);
    CHECK(and_ != nullptr && name_is(and_->left, "b") &&
          name_is(and_->right, "c"));
  }
}
void reserved_words_are_not_names() {
  ExpressionProbes probes;
  ast::BinaryExpr *and_ =
      bin(probes.parse("android and order"), ast::BinOp::and_);
  CHECK(and_ != nullptr && name_is(and_->left, "android") &&
        name_is(and_->right, "order"));

  const auto *implies = as<ast::IfExpr>(probes.parse("a.f and b.f => c.f"));
  CHECK(implies && !implies->else_ && bin(implies->cond, ast::BinOp::and_) &&
        bin(implies->then, ast::BinOp::join));
}

} // namespace

int main() {
  audit_regressions::run();
  const std::filesystem::path dir =
      std::filesystem::path(CURSIV_EXAMPLE_DIR) / "example";
  std::vector<std::filesystem::path> files;
  for (const std::filesystem::path &d :
       {dir, std::filesystem::path(CURSIV_TEST_DIR)})
    for (const auto &entry : std::filesystem::directory_iterator(d))
      if (entry.is_regular_file() && entry.path().extension() == ".als")
        files.push_back(entry.path());
  if (files.empty())
    return fail("no example document found");

  const auto language = cursiv::create_language<alloy::AlloyCoreServices,
                                                alloy::AlloySharedCoreServices,
                                                alloy::install_core_module>();

  int parsed = 0;
  for (const std::filesystem::path &file : files) {
    std::ifstream in(file);
    if (!in)
      return fail(file.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail(file.string() + ": does not parse clean");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<alloy::ast::Module>)
      return fail(file.string() + ": built no Module");

    ++parsed;
  }
  std::printf("alloy: %d example document(s) parse\n", parsed);

  dot_binds_tighter_than_box_join();
  long_chain();
  cardinality_tier();
  comprehension_and_block();
  historical_identifiers();
  quantifier_prefix_tier();
  general_negation_tier();
  implication_tier();
  iff_and_or_tiers();
  reserved_words_are_not_names();

  return cursiv::test::report("alloy_grammar_test");
}
