#pragma once

#include <cstdio>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "typescript/core/ast.hpp"

inline bool typescript_lexical_cases(const cursiv::Parser& parser) {
  namespace ast = typescript::ast;
  auto fail = [](const char* what) {
    std::fprintf(stderr, "typescript: %s\n", what);
    return false;
  };
  auto program = [](const cursiv::ParseResult& result) {
    return result.accepted && !result.cst().recovered() &&
                   result.diagnostics.empty()
               ? cursiv::ast_cast<ast::Program>(result.value)
               : nullptr;
  };
  for (const std::string space : {"\u00a0", "\u1680", "\u2000", "\u202f",
                                  "\u3000", "\ufeff", "\u2028", "\u2029"}) {
    const std::string source = "const" + space + "value=1;";
    const auto result = parser.parse(source);
    const auto* root = program(result);
    const auto* decl = root && root->body.size() == 1
                           ? cursiv::ast_cast<ast::VarDecl>(root->body[0])
                           : nullptr;
    if (!decl || decl->declarations.size() != 1)
      return fail("Unicode whitespace between keyword and binding");
  }
  for (const std::string line : {"\r", "\n", "\r\n", "\u2028", "\u2029"}) {
    const std::string source = "let a=1;// comment" + line + "let b=2;";
    const auto result = parser.parse(source);
    const auto* root = program(result);
    if (!root || root->body.size() != 2)
      return fail("code after single-line comment retained");
    const std::string prologueSource = "#!/usr/bin/env node" + line +
                                       "const value=1;";
    const auto prologue = parser.parse(prologueSource);
    const auto* prologueRoot = program(prologue);
    if (!prologueRoot || prologueRoot->body.size() != 1)
      return fail("code after hashbang retained");
    const std::string returnSource = "function f(){return// comment" + line +
                                     "1;}";
    const auto returned = parser.parse(returnSource);
    const auto* functionRoot = program(returned);
    const auto* function = functionRoot && functionRoot->body.size() == 1
                               ? cursiv::ast_cast<ast::FunctionDecl>(
                                     functionRoot->body[0])
                               : nullptr;
    const auto* returnedNode = function && function->body &&
                                      function->body->body.size() == 2
                                  ? cursiv::ast_cast<ast::ReturnStmt>(
                                        function->body->body[0])
                                  : nullptr;
    if (!returnedNode || returnedNode->argument)
      return fail("return line break after comment");
  }
  const auto continued = parser.parse("const value=\"a\\\r\nb\";");
  if (!program(continued)) return fail("CRLF string continuation");
  const auto objects = parser.parse(
      "const o={ [Symbol.iterator](){return this;},"
      "*[\"items\"](){yield 1;}, value:2};");
  const auto* root = program(objects);
  const auto* decl = root && root->body.size() == 1
                         ? cursiv::ast_cast<ast::VarDecl>(root->body[0])
                         : nullptr;
  const auto* object = decl && decl->declarations.size() == 1
                           ? cursiv::ast_cast<ast::ObjectLit>(
                                 decl->declarations[0]->init)
                           : nullptr;
  if (!object || object->properties.size() != 3 ||
      !object->properties[0]->isMethod ||
      !object->properties[1]->isGenerator)
    return fail("computed methods share ordinary object-literal grammar");
  return true;
}
