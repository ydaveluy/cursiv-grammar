#pragma once
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "native_projection.hpp"
#include <fstream>
#include <sstream>
#include <unordered_set>
namespace native_regressions {
inline const ast::Expr *expression(const ast::Program *root) {
  if (root && !root->items.empty())
    if (auto *fn = cursiv::ast_cast<ast::FunctionDef>(root->items[0]))
      if (auto *block = cursiv::ast_cast<ast::Block>(fn->body))
        if (!block->stmts.empty())
          if (auto *stmt = cursiv::ast_cast<ast::ExprStmt>(block->stmts[0]))
            return stmt->value;
  return nullptr;
}
inline int run(const auto &language) {
  int failures = 0;
  const auto read = [](const char *name) {
    std::ifstream f(std::string(CURSIV_TEST_DIR) + "/native/" + name);
    return std::string((std::istreambuf_iterator<char>(f)), {});
  };
  struct Case {
    const char *file;
    const char *tree;
  };
  const Case cases[] = {
      {"e00.b", R"B(["+",["name","a"],["*",["name","b"],["name","c"]]])B"},
      {"e01.b", R"B(["<<",["name","a"],["+",["name","b"],["name","c"]]])B"},
      {"e02.b", R"B(["<",["name","a"],["<<",["name","b"],["name","c"]]])B"},
      {"e03.b", R"B(["==",["name","a"],["<",["name","b"],["name","c"]]])B"},
      {"e04.b", R"B(["&",["name","a"],["==",["name","b"],["name","c"]]])B"},
      {"e05.b", R"B(["|",["name","a"],["&",["name","b"],["name","c"]]])B"},
      {"e06.b", R"B(["*u",["name","p"]])B"},
      {"e07.b", R"B(["*u",["+",["name","a"],["name","i"]]])B"},
      {"e08.b",
       R"B(["*u",["+",["*u",["+",["name","a"],["name","i"]]],["name","j"]]])B"},
      {"e09.b", R"B(["*u",["*u",["+",["name","p"],["name","i"]]]])B"},
      {"e10.b", R"B(["=",["*u",["name","p"]],["int","5"]])B"},
      {"e11.b",
       R"B(["=",["*u",["+",["name","a"],["name","i"]]],["int","5"]])B"},
      {"e12.b", R"B(["u++",["*u",["+",["name","a"],["name","i"]]]])B"},
      {"e13.b", R"B(["name","p"])B"},
      {"e14.b", R"B(["=",["name","p"],["&u",["name","x"]]])B"},
      {"e15.b", R"B(["=&",["name","x"],["name","y"]])B"},
      {"e16.b", R"B(["=",["name","x"],["-u",["name","p"]]])B"},
      {"e17.b", R"B(["+",["++u",["*u",["name","p"]]],["name","q"]])B"},
      {"e18.b", R"B(["+",["name","p"],["name","q"]])B"},
      {"e19.b", R"B(["u++",["*u",["name","p"]]])B"},
      {"e20.b", R"B(["call",["name","f"],[["name","x"],["name","y"]]])B"},
      {"e21.b",
       R"B(["call",["*u",["name","p"]],[["name","x"],["name","y"]]])B"},
      {"e22.b", R"B(["call",["call",["name","f"],[]],[["name","x"]]])B"},
      {"e23.b",
       R"B(["call",["?",["name","a"],[":",["name","f"],["name","g"]]],[["name","x"]]])B"},
      {"e24.b", R"B([",",["name","a"],[",",["name","b"],["name","c"]]])B"},
      {"e25.b", R"B([",",["=",["name","x"],["name","a"]],["name","b"]])B"},
      {"e26.b",
       R"B(["call",["name","f"],[["name","a"],["name","b"],["name","c"]]])B"},
      {"e27.b", R"B(["===",["name","x"],["name","y"]])B"},
      {"e28.b", R"B(["=!=",["name","x"],["name","y"]])B"},
      {"e29.b", R"B(["=<<",["name","x"],["int","1"]])B"},
      {"e30.b", R"B(["=",["name","x"],["name","y"]])B"},
      {"e31.b", R"B(["int","8"])B"},
      {"e32.b", R"B(["int","26984"])B"},
      {"e33.b", R"B(["int","10"])B"},
      {"e34.b", R"B(["int","39"])B"},
      {"e35.b", R"B(["string","a\"b"])B"},
      {"e36.b", R"B(["string","a\n\t\u0000\u0000*z"])B"},
      {"e37.b", R"B(["name","_x"])B"},
      {"e38.b", R"B(["name",".x"])B"},
      {"e39.b", R"B(["int","0"])B"},
      {"e44.b", R"B(["string","\r"])B"},
      {"e45.b", R"B(["int","17"])B"},
  };
  for (const auto &c : cases) {
    const std::string source = read(c.file);
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<ast::Program>(result.value);
    std::ostringstream text;
    Projection(text).expr(expression(root));
    bool parents = root && root->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto visit = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        parents = false;
        return;
      }
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            parents &= child.parent == &n &&
                       result.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (root)
      visit(visit, *root);
    parents &= seen.size() == result.nodes.size();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || text.str() != c.tree || !parents ||
        !cursiv::test::validationHelper(language.language, source)
             .diagnostics.empty()) {
      std::fprintf(stderr, "b: native expression fields differ %s\n", c.file);
      ++failures;
    }
  }
  for (const auto &c :
       {std::pair{"e31.b", "010"}, std::pair{"e39.b", "18446744073709551616"},
        std::pair{"e45.b", "019"}}) {
    const std::string source = read(c.first);
    const auto result = language.language.parser->parse(source);
    const auto *literal = cursiv::ast_cast<ast::IntLit>(
        expression(cursiv::ast_cast<ast::Program>(result.value)));
    if (!literal || literal->value != c.second)
      ++failures;
  }
  for (const auto &c : {std::pair{"p00.b", 3}, std::pair{"p01.b", 2},
                        std::pair{"p02.b", 0}, std::pair{"p03.b", 1}}) {
    const std::string source = read(c.first);
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<ast::Program>(result.value);
    const auto *var = root && !root->items.empty()
                          ? cursiv::ast_cast<ast::VarDef>(root->items[0])
                          : nullptr;
    const bool scalar = std::string_view(c.first) == "p03.b";
    bool ok = var && var->name && var->name->name == (scalar ? "x" : "v") &&
              var->bracketed != scalar &&
              var->inits.size() == std::size_t(c.second);
    if (var) {
      if (std::string_view(c.first) == "p00.b" ||
          std::string_view(c.first) == "p02.b") {
        const auto *size = cursiv::ast_cast<ast::IntLit>(var->size);
        ok &= size && size->value == "3";
      } else
        ok &= var->size == nullptr;
      if (scalar) {
        const auto *value = var->inits.empty()
                                ? nullptr
                                : cursiv::ast_cast<ast::IntLit>(var->inits[0]);
        ok &= value && value->value == "010";
      } else
        for (std::size_t i = 0; i < var->inits.size(); ++i) {
          const auto *value = cursiv::ast_cast<ast::IntLit>(var->inits[i]);
          ok &= value && value->value == std::to_string(i + 1);
        }
    }
    if (!ok || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() ||
        !cursiv::test::validationHelper(language.language, source)
             .diagnostics.empty()) {
      std::fprintf(stderr, "b: vector fields differ %s\n", c.first);
      ++failures;
    }
  }
  {
    const std::string source = read("p04.b");
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<ast::Program>(result.value);
    const auto *fn = root && !root->items.empty()
                         ? cursiv::ast_cast<ast::FunctionDef>(root->items[0])
                         : nullptr;
    const auto *block = fn ? cursiv::ast_cast<ast::Block>(fn->body) : nullptr;
    const auto *vars = block && !block->stmts.empty()
                           ? cursiv::ast_cast<ast::AutoStmt>(block->stmts[0])
                           : nullptr;
    bool ok = vars && vars->vars.size() == 2;
    if (ok) {
      const auto *v = vars->vars[0];
      const auto *w = vars->vars[1];
      const auto *size = cursiv::ast_cast<ast::IntLit>(v->size);
      ok = v->bracketed && w->bracketed && v->name->name == "v" &&
           w->name->name == "w" && size && size->value == "3" && !w->size;
    }
    if (!ok || !cursiv::test::validationHelper(language.language, source)
                    .diagnostics.empty())
      ++failures;
  }
  {
    const std::string source =
        "x 1,2; f(){auto break;break=1;default:return(break);}";
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<ast::Program>(result.value);
    const auto *var = root && root->items.size() == 2
                          ? cursiv::ast_cast<ast::VarDef>(root->items[0])
                          : nullptr;
    const auto *fn = root && root->items.size() == 2
                         ? cursiv::ast_cast<ast::FunctionDef>(root->items[1])
                         : nullptr;
    const auto *block = fn ? cursiv::ast_cast<ast::Block>(fn->body) : nullptr;
    const auto *label =
        block && block->stmts.size() == 3
            ? cursiv::ast_cast<ast::LabeledStmt>(block->stmts[2])
            : nullptr;
    if (!var || var->bracketed || var->inits.size() != 2 || !label ||
        !label->name || label->name->name != "default" || !result.accepted ||
        result.cst().recovered() || !result.diagnostics.empty() ||
        !cursiv::test::validationHelper(language.language, source)
             .diagnostics.empty())
      ++failures;
  }
  for (const auto &c : {std::pair{"f(){1=2;}", "b.lvalue"},
                        std::pair{"f(a,b){++(a+b);}", "b.lvalue"},
                        std::pair{"f(){&1;}", "b.lvalue"},
                        std::pair{"f(){case 1:;}", "b.case-context"},
                        std::pair{"x[2] 1 2;", "b.initializer-separator"},
                        std::pair{"f(){auto a \"x\";}", "b.vector-size"},
                        std::pair{"f(){return(if);}", "b.keyword"}}) {
    const auto result = language.language.parser->parse(c.first);
    const auto validation =
        cursiv::test::validationHelper(language.language, c.first);
    bool found = false;
    for (const auto &d : validation.diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == c.second;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !found) {
      std::fprintf(stderr, "b: missing runtime diagnostic %s\n", c.second);
      ++failures;
    }
  }
  return failures;
}
} // namespace native_regressions
