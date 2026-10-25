#pragma once
#include <cstdio>
#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include "haskell/core/ast.hpp"
namespace port_audit_cases {
using namespace haskell;
template<class T> const T* as(const cursiv::AstNode* n) { return cursiv::ast_cast<T>(n); }
inline bool run(const cursiv::Parser& parser) {
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "haskell audit: %s\n", message); ok = false; }
  };
  for (const auto text : {"0b1010", "1_000", "0x_1f", "0o7_7"}) {
    const std::string source = std::string("x = ") + text + "\n";
    const auto result = parser.parse(source);
    const auto* module = as<ast::Module>(result.value);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(), text);
    const auto* decl = module && module->decls.size() == 1 ? as<ast::ValueDecl>(module->decls[0]) : nullptr;
    const auto* rhs = decl && decl->rhs.size() == 1 ? decl->rhs[0] : nullptr;
    const auto* literal = rhs ? as<ast::IntLit>(rhs->value) : nullptr;
    check(literal && literal->text == text, "integer spelling and node");
    check(literal && literal->parent == rhs && rhs->parent == decl && decl->parent == module,
          "integer ancestry");
  }
  for (const auto text : {"0x1.fp3", "1_2.3_4e+2"}) {
    const std::string source = std::string("x = ") + text + "\n";
    const auto result = parser.parse(source);
    bool found = false;
    for (const auto* node : result.nodes)
      if (const auto* literal = as<ast::FloatLit>(node)) found |= literal->text == text;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && found, text);
  }
  for (const auto text : {"'\\x41'", "'\\65'", "'\\NUL'", "'\\^A'"}) {
    const std::string source = std::string("x = ") + text + "\n";
    const auto result = parser.parse(source);
    bool found = false;
    for (const auto* node : result.nodes)
      if (const auto* literal = as<ast::CharLit>(node)) found |= literal->text == text;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && found, text);
  }
  for (const auto op : {"-->", "---+", "⊕", "Data.List.++"}) {
    const std::string source = std::string("x = 1 ") + op + " 2\n";
    const auto result = parser.parse(source);
    bool found = false;
    for (const auto* node : result.nodes)
      if (const auto* chain = as<ast::OpChain>(node))
        found |= chain->ops.size() == 1 && chain->ops[0] == op &&
                 chain->operands.size() == 1 && as<ast::IntLit>(chain->first) &&
                 as<ast::IntLit>(chain->operands[0]) && chain->first->parent == chain &&
                 chain->operands[0]->parent == chain;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && found, op);
  }
  {
    const std::string source = "变量 = 1\nx́ = 变量\n";
    const auto result = parser.parse(source);
    const auto* module = as<ast::Module>(result.value);
    const auto* decl = module && module->decls.size() == 2 ? as<ast::ValueDecl>(module->decls[1]) : nullptr;
    const auto* name = decl ? as<ast::Var>(decl->lhs) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          name && name->name == "x́", "Unicode identifier categories");
  }
  {
    const std::string source = "module Audit where { import {-# SOURCE #-} safe qualified \"base\" Data.List as L; import Data.Map qualified; x = 1; y = 2 }\n";
    const auto result = parser.parse(source);
    const auto* module = as<ast::Module>(result.value);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && module &&
          module->name == "Audit" && module->imports.size() == 2 && module->decls.size() == 2,
          "explicit module body and import separators");
    if (module && module->imports.size() == 2) {
      const auto* first = module->imports[0];
      check(first->qualified && first->source && first->safe && first->packageLiteral == "\"base\"" &&
            first->moduleName == "Data.List" && first->asName == "L" && first->parent == module,
            "import attributes and parent");
      check(module->imports[1]->qualified && module->imports[1]->moduleName == "Data.Map",
            "postpositive qualified import");
    }
  }
  {
    const std::string source = "class C a b | a -> b where { f :: a -> b }\n";
    const auto result = parser.parse(source);
    const auto* module = as<ast::Module>(result.value);
    const auto* decl = module && module->decls.size() == 1 ? as<ast::ClassDecl>(module->decls[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && decl &&
          decl->dependencies.size() == 1, "functional dependency");
    if (decl && decl->dependencies.size() == 1) {
      const auto* dep = decl->dependencies[0];
      check(dep->from.size() == 1 && dep->from[0] == "a" && dep->to.size() == 1 && dep->to[0] == "b" &&
            dep->parent == decl, "functional dependency values and parent");
    }
  }
  return ok;
}
}
