#pragma once

#include <cstdio>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "java/core/ast.hpp"

inline bool java_literal_dimension_cases(const cursiv::Parser& parser) {
  namespace ast = java::ast;
  auto clean = [](const cursiv::ParseResult& r) {
    return r.accepted && r.value && !r.cst().recovered() && r.diagnostics.empty();
  };
  auto fail = [](const char* what) {
    std::fprintf(stderr, "java: %s\n", what);
    return false;
  };
  for (unsigned slashes = 0; slashes < 6; ++slashes) {
    const std::string literal = "\"\"\"\ntext " + std::string(slashes, '\\') +
                                "\"\"\"" +
                                (slashes % 2 ? " rest\n\"\"\"" : "");
    const std::string source = "class T {String s=" + literal +
                               "; int after=1;}";
    const auto result = parser.parse(source);
    if (!clean(result)) return fail("escaped text-block delimiter");
    unsigned blocks = 0, declarations = 0;
    for (const auto* node : result.nodes) {
      if (const auto* block = cursiv::ast_cast<ast::TextBlockLit>(node)) {
        ++blocks;
        if (block->text != literal) return fail("text-block spelling retained");
      }
      declarations += cursiv::ast_cast<ast::VarDeclarator>(node) != nullptr;
    }
    if (blocks != 1 || declarations != 2)
      return fail("declaration after text block retained");
  }
  const std::string literal = "\"\"\" \f\t\r\nbody\\\r\nmore\r\n\"\"\"";
  const std::string source = "class T {String s=" + literal + ";}";
  const auto text = parser.parse(source);
  if (!clean(text)) return fail("text-block whitespace and CRLF continuation");
  for (const auto* node : text.nodes)
    if (const auto* block = cursiv::ast_cast<ast::TextBlockLit>(node))
      if (block->text != literal) return fail("text-block CRLF spelling");

  const auto result = parser.parse(
      "@java.lang.annotation.Target(java.lang.annotation.ElementType.TYPE_USE) "
      "@interface A {} class T {"
      "int a @A [], b @A [/*[*/] @A [];"
      "int f() @A [] {return null;}"
      "void m(int p @A [], String @A ... args){}"
      "Object n=new int @A [1] @A [];"
      "void loop(int[][] values){for(int item @A []:values){}}"
      "} record R(String @A ... args){}");
  if (!clean(result)) return fail("annotated array dimensions");
  unsigned declarations = 0, methods = 0, parameters = 0, arrays = 0;
  unsigned iterations = 0, components = 0;
  for (const auto* node : result.nodes) {
    if (const auto* decl = cursiv::ast_cast<ast::VarDeclarator>(node)) {
      if (decl->name == "a" || decl->name == "b") {
        ++declarations;
        if (decl->extraDims.size() != (decl->name == "a" ? 1u : 2u))
          return fail("trailing dimension count");
        for (const auto* dim : decl->extraDims)
          if (!dim || dim->size || dim->annotations.size() != 1 ||
              dim->parent != decl)
            return fail("dimension annotations and containment");
      }
    }
    if (const auto* method = cursiv::ast_cast<ast::MethodDecl>(node)) {
      if (method->name == "f") {
        ++methods;
        if (method->returnExtraDims.size() != 1 ||
            method->returnExtraDims[0]->annotations.size() != 1)
          return fail("annotated method return dimensions");
      }
    }
    if (const auto* param = cursiv::ast_cast<ast::Param>(node)) {
      if (param->name == "p") {
        ++parameters;
        if (param->extraDims.size() != 1 ||
            param->extraDims[0]->annotations.size() != 1)
          return fail("annotated parameter dimensions");
      }
      if (param->name == "args") {
        ++parameters;
        if (!param->varargs || param->varargsAnnotations.size() != 1)
          return fail("varargs annotations retained");
      }
    }
    if (const auto* array = cursiv::ast_cast<ast::NewArrayExpr>(node)) {
      ++arrays;
      if (array->dimensions.size() != 2 || !array->dimensions[0]->size ||
          array->dimensions[1]->size ||
          array->dimensions[0]->annotations.size() != 1 ||
          array->dimensions[1]->annotations.size() != 1)
        return fail("annotated sized and unsized creation dimensions");
    }
    if (const auto* loop = cursiv::ast_cast<ast::ForEachStmt>(node)) {
      ++iterations;
      if (loop->extraDims.size() != 1 ||
          loop->extraDims[0]->annotations.size() != 1)
        return fail("annotated foreach dimensions");
    }
    if (const auto* component = cursiv::ast_cast<ast::RecordComponent>(node)) {
      ++components;
      if (!component->varargs || component->varargsAnnotations.size() != 1)
        return fail("record varargs annotations retained");
    }
  }
  return (declarations == 2 && methods == 1 && parameters == 2 && arrays == 1 &&
          iterations == 1 && components == 1) || fail("dimension AST coverage");
}
