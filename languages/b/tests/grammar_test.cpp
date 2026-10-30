// The grammar's end to end check: every document under example/ parses
// clean, with no recovery and no diagnostics, and builds a Program root.

#include "native_cases.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "b/core/CoreServices.hpp"
#include "b/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "b: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  namespace fs = std::filesystem;

  const auto language = cursiv::create_language<
      b::BCoreServices, b::BSharedCoreServices, b::install_core_module>();

  if(native_regressions::run(language)) return 1;

  // Thompson's Users' Reference to B, section 2: *(/ *) spell braces
  // inside both string and packed-character constants.
  {
    const auto result = language.language.parser->parse(
        "f(){ putchar('*('); putchar('*)'); printf(\"*(body*)\"); }");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("native brace escapes do not parse clean");
    std::vector<std::string_view> characters;
    const b::ast::StringLit* string = nullptr;
    for (const auto* node : result.nodes) {
      if (const auto* c = cursiv::ast_cast<b::ast::CharLit>(node))
        characters.push_back(c->text);
      if (const auto* s = cursiv::ast_cast<b::ast::StringLit>(node))
        string = s;
    }
    if (characters != std::vector<std::string_view>{"{", "}"} ||
        string == nullptr || string->text != "{body}")
      return fail("native brace escapes lost their character values");
  }

  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".b")
      continue;
    const fs::path path = entry.path();

    std::ifstream file(path, std::ios::binary);
    if (!file) return fail(path.filename().string() + " is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.filename().string() + " does not parse clean");
    if (!result.diagnostics.empty())
      return fail(path.filename().string() + " left diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<b::ast::Program>)
      return fail(path.filename().string() + " built no program");
    ++count;
  }
  if (count == 0) return fail("no example documents found");

  // Every probed text stays alive for the whole run: each `Expr*` below
  // points into the source `expr_of` parsed it from.
  std::vector<std::string> probeTexts;
  std::vector<cursiv::ParseResult> probeResults;
  auto expr_of = [&](const std::string& src) -> const b::ast::Expr* {
    probeTexts.push_back("f(){" + src + ";}");
    probeResults.push_back(
        language.language.parser->parse(probeTexts.back()));
    const cursiv::ParseResult& r = probeResults.back();
    if (r.cst().recovered() || !r.diagnostics.empty()) return nullptr;
    const auto* const prog = static_cast<const b::ast::Program*>(r.value);
    if (prog == nullptr || prog->items.empty()) return nullptr;
    const auto* const fn =
        cursiv::ast_cast<b::ast::FunctionDef>(prog->items[0]);
    if (fn == nullptr) return nullptr;
    const auto* const block = cursiv::ast_cast<b::ast::Block>(fn->body);
    if (block == nullptr || block->stmts.empty()) return nullptr;
    const auto* const es =
        cursiv::ast_cast<b::ast::ExprStmt>(block->stmts[0]);
    return es != nullptr ? es->value : nullptr;
  };

  // B's seven binary levels, tightest first: `* / %`, `+ -`, `<< >>`,
  // relational, equality, `&`, `|` -- one probe per adjacent pair.
  {
    auto check = [&](const std::string& src, std::string_view outerOp,
                     bool innerOnRight) -> bool {
      const auto* const top =
          cursiv::ast_cast<b::ast::BinaryExpr>(expr_of(src));
      if (top == nullptr || top->op != outerOp) return false;
      const b::ast::Expr* const inner =
          innerOnRight ? top->right : top->left;
      return cursiv::ast_cast<b::ast::BinaryExpr>(inner) != nullptr;
    };
    if (!check("a+b*c", "+", true))
      return fail("'a+b*c': '*' should bind tighter than '+'");
    if (!check("a<<b+c", "<<", true))
      return fail("'a<<b+c': '+' should bind tighter than '<<'");
    if (!check("a<b<<c", "<", true))
      return fail("'a<b<<c': '<<' should bind tighter than '<'");
    if (!check("a==b<c", "==", true))
      return fail("'a==b<c': '<' should bind tighter than '=='");
    if (!check("a&b==c", "&", true))
      return fail("'a&b==c': '==' should bind tighter than '&'");
    if (!check("a|b&c", "|", true))
      return fail("'a|b&c': '&' should bind tighter than '|'");
  }

  // `*` (indirection) and `[]` (indexing), Users' Reference to B §3.0,
  // §4.1.7, §4.2.1: `[]` is a primary expression, `*` a unary operator one
  // tier looser, so `*p[i]` reads `*(p[i])`, not `(*p)[i]`.
  {
    const auto* const deref = cursiv::ast_cast<b::ast::PrefixExpr>(expr_of("*p"));
    if (deref == nullptr || deref->op != "*" ||
        cursiv::ast_cast<b::ast::Ident>(deref->operand) == nullptr)
      return fail("'*p' should read as an indirection over a name");

    const auto* const index = cursiv::ast_cast<b::ast::IndexExpr>(expr_of("a[i]"));
    if (index == nullptr ||
        cursiv::ast_cast<b::ast::Ident>(index->array) == nullptr ||
        cursiv::ast_cast<b::ast::Ident>(index->index) == nullptr)
      return fail("'a[i]' should read as an index of a name by a name");

    const auto* const chained =
        cursiv::ast_cast<b::ast::IndexExpr>(expr_of("a[i][j]"));
    if (chained == nullptr ||
        cursiv::ast_cast<b::ast::IndexExpr>(chained->array) == nullptr)
      return fail("'a[i][j]' should chain two indices left to right");

    const auto* const derefIndex =
        cursiv::ast_cast<b::ast::PrefixExpr>(expr_of("*p[i]"));
    if (derefIndex == nullptr || derefIndex->op != "*" ||
        cursiv::ast_cast<b::ast::IndexExpr>(derefIndex->operand) == nullptr)
      return fail("'*p[i]' should read as '*(p[i])': [] tighter than *");

    const auto* const assignDeref =
        cursiv::ast_cast<b::ast::AssignExpr>(expr_of("*p = 5"));
    if (assignDeref == nullptr ||
        cursiv::ast_cast<b::ast::PrefixExpr>(assignDeref->target) == nullptr)
      return fail("'*p = 5' should assign through an indirection target");

    const auto* const assignIndex =
        cursiv::ast_cast<b::ast::AssignExpr>(expr_of("a[i] = 5"));
    if (assignIndex == nullptr ||
        cursiv::ast_cast<b::ast::IndexExpr>(assignIndex->target) == nullptr)
      return fail("'a[i] = 5' should assign through an index target");

    const auto* const incrIndex =
        cursiv::ast_cast<b::ast::PostfixExpr>(expr_of("a[i]++"));
    if (incrIndex == nullptr ||
        cursiv::ast_cast<b::ast::IndexExpr>(incrIndex->operand) == nullptr)
      return fail("'a[i]++' should postfix-increment an index target");

    // Users' Reference to B's own example: `&*x` is identically `x`.
    const auto* const addrDeref =
        cursiv::ast_cast<b::ast::AddrExpr>(expr_of("&*p"));
    if (addrDeref == nullptr ||
        cursiv::ast_cast<b::ast::PrefixExpr>(addrDeref->target) == nullptr)
      return fail("'&*p' should take the address of an indirection");

    // `=op` is one operator, its characters adjacent: `p = &x` is a
    // plain assignment of an address, `x =& y` the compound.
    const auto* const assignAddr =
        cursiv::ast_cast<b::ast::AssignExpr>(expr_of("p = &x"));
    if (assignAddr == nullptr || !assignAddr->op.empty() ||
        cursiv::ast_cast<b::ast::AddrExpr>(assignAddr->value) == nullptr)
      return fail("'p = &x' should assign the address of x");

    const auto* const compound =
        cursiv::ast_cast<b::ast::AssignExpr>(expr_of("x =& y"));
    if (compound == nullptr || compound->op != "&" ||
        cursiv::ast_cast<b::ast::Ident>(compound->value) == nullptr)
      return fail("'x =& y' should be the compound assignment");

    const auto* const assignNeg =
        cursiv::ast_cast<b::ast::AssignExpr>(expr_of("x = -p"));
    if (assignNeg == nullptr || !assignNeg->op.empty() ||
        cursiv::ast_cast<b::ast::PrefixExpr>(assignNeg->value) == nullptr)
      return fail("'x = -p' should assign a negation");
  }

  std::printf("b: %d examples parse\n", count);
  return 0;
}
