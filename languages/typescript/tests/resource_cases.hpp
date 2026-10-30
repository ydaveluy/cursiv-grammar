#pragma once
#include <cstdio>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "typescript/core/CoreServices.hpp"
#include "typescript/core/ast.hpp"
namespace typescript_resources {
namespace ast=typescript::ast;
struct Binding {const char* name; const char* kind; bool definite, type, init;};
// Expectations from root-executed TypeScript5.9.3 native99+21+13 receipts.
// Semantic diagnostics do not change these parse/AST expectations.
inline constexpr Binding bindings[] = {
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"a", "ident", false, false, true},
  {"b", "ident", false, false, true},
  {"r", "ident", false, true, true},
  {"r", "ident", false, false, false},
  {"r", "ident", false, true, false},
  {"{x}", "object", false, false, true},
  {"{x}", "object", false, false, true},
  {"a", "ident", false, false, true},
  {"[x]", "array", false, false, true},
  {"a", "ident", false, false, true},
  {"{x}", "object", false, false, true},
  {"using", "ident", false, false, true},
  {"of", "ident", false, false, true},
  {"await", "ident", false, false, true},
  {"let", "ident", false, false, true},
  {"r", "ident", true, true, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"s", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"await$foo", "ident", false, false, true},
  {"as", "ident", false, false, false},
  {"T", "ident", false, false, false},
  {"satisfies", "ident", false, false, false},
  {"T", "ident", false, false, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"using", "ident", false, false, false},
  {"of", "ident", false, false, false},
  {"of", "ident", false, false, true},
  {"of", "ident", false, false, false},
  {"of", "ident", false, true, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, true},
  {"a", "ident", false, false, false},
  {"b", "ident", false, false, false},
  {"r", "ident", false, true, false},
  {"{x}", "object", false, false, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"a", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"s", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, true},
  {"r", "ident", false, true, false},
  {"r", "ident", false, true, false},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, true, false},
  {"r", "ident", false, false, true},
  {"r\\u0065s", "ident", false, false, true},
  {"r", "ident", false, false, false},
  {"of", "ident", false, false, false},
  {"xs", "ident", false, false, false},
  {"{}", "object", false, false, false},
  {"r", "ident", false, false, false},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"await", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"r", "ident", false, false, true},
  {"of", "ident", false, false, true},
  {"of", "ident", false, false, false},
  {"of", "ident", false, true, false},
  {"of", "ident", false, false, true},
  {"\\u006cet", "ident", false, false, true},
  {"\\u{006c}et", "ident", false, false, true},
  {"a\\u0077ait", "ident", false, false, true},
  {"\\u0079ield", "ident", false, false, true},
  {"r\\u0065s", "ident", false, false, true},
};
struct Case {
  const char* name; const char* source; bool clean; unsigned resources;
  bool awaitUsing; unsigned bindingAt, bindingCount;
  const char* loop; bool forAwait;
};
inline constexpr Case cases[] = {
  {"using-basic", "using r=null;", true, 1, false, 0, 1, "none", false},
  {"await-using-module", "export {}; await using r=null;", true, 1, true, 1, 1, "none", false},
  {"await-using-function", "async function f(){await using r=null;}", true, 1, true, 2, 1, "none", false},
  {"using-multiple", "using a=null,b=null;", true, 1, false, 3, 2, "none", false},
  {"using-annotation", "using r: Disposable=null;", true, 1, false, 5, 1, "none", false},
  {"using-uninitialized", "using r;", true, 1, false, 6, 1, "none", false},
  {"using-uninitialized-type", "using r: Disposable;", true, 1, false, 7, 1, "none", false},
  {"using-object-binding", "using {x}=obj;", true, 1, false, 8, 1, "none", false},
  {"using-object-binding-adjacent", "using{x}=obj;", true, 1, false, 9, 1, "none", false},
  {"using-array-expression", "using[x]=obj;", true, 0, false, 10, 0, "none", false},
  {"using-array-expression-spaced", "using [x]=obj;", true, 0, false, 10, 0, "none", false},
  {"using-later-array-binding", "using a=null,[x]=obj;", true, 1, false, 10, 2, "none", false},
  {"using-later-object-binding", "using a=null,{x}=obj;", true, 1, false, 12, 2, "none", false},
  {"using-self-name", "using using=null;", true, 1, false, 14, 1, "none", false},
  {"using-of-name", "using of=null;", true, 1, false, 15, 1, "none", false},
  {"using-await-name", "using await=null;", true, 1, false, 16, 1, "none", false},
  {"using-let-name", "using let=null;", true, 1, false, 17, 1, "none", false},
  {"using-definite", "using r!: Disposable=null;", true, 1, false, 18, 1, "none", false},
  {"using-trailing-comma", "using r=null,;", true, 1, false, 19, 1, "none", false},
  {"newline-using-binding", "using\nr=null;", true, 0, false, 20, 0, "none", false},
  {"comment-newline-using-binding", "using/*\n*/r=null;", true, 0, false, 20, 0, "none", false},
  {"comment-using-binding", "using/*c*/r=null;", true, 1, false, 20, 1, "none", false},
  {"newline-before-equals", "using r\n=null;", true, 1, false, 21, 1, "none", false},
  {"newline-after-comma", "using r=null,\ns=null;", true, 1, false, 22, 2, "none", false},
  {"newline-await-using", "export {}; await\nusing r=null;", true, 1, true, 24, 1, "none", false},
  {"comment-newline-await-using", "export {}; await/*\n*/using r=null;", true, 1, true, 25, 1, "none", false},
  {"newline-await-using-binding", "export {}; await using\nr=null;", true, 0, false, 26, 0, "none", false},
  {"comment-newline-await-using-binding", "export {}; await using/*\n*/r=null;", true, 0, false, 26, 0, "none", false},
  {"line-separator-using-binding", "using\u2028r=null;", true, 0, false, 26, 0, "none", false},
  {"paragraph-separator-using-binding", "using\u2029r=null;", true, 0, false, 26, 0, "none", false},
  {"line-separator-await-using", "export {}; await\u2028using r=null;", true, 1, true, 26, 1, "none", false},
  {"ordinary-binding-call", "let using=(x:any)=>x; using(1);", true, 0, false, 27, 0, "none", false},
  {"ordinary-call", "using();", true, 0, false, 27, 0, "none", false},
  {"ordinary-property", "using.x;", true, 0, false, 27, 0, "none", false},
  {"ordinary-element", "using[x];", true, 0, false, 27, 0, "none", false},
  {"ordinary-optional-call", "using?.();", true, 0, false, 27, 0, "none", false},
  {"ordinary-assignment", "using=acquire();", true, 0, false, 27, 0, "none", false},
  {"ordinary-label", "using: while(false){break using;}", true, 0, false, 27, 0, "none", false},
  {"ordinary-object-method", "const o={using:1, m(){return this.using}}; o.using;", true, 0, false, 27, 0, "none", false},
  {"ordinary-as-in-expression", "const x=using as T;", true, 0, false, 27, 0, "none", false},
  {"ordinary-satisfies-in-expression", "const x=using satisfies T;", true, 0, false, 27, 0, "none", false},
  {"ordinary-instanceof", "using instanceof C;", true, 0, false, 27, 0, "none", false},
  {"ordinary-in", "using in obj;", true, 0, false, 27, 0, "none", false},
  {"ordinary-plus", "using+1;", true, 0, false, 27, 0, "none", false},
  {"ordinary-await-call", "export {}; await using();", true, 0, false, 27, 0, "none", false},
  {"ordinary-await-property", "export {}; await using.x;", true, 0, false, 27, 0, "none", false},
  {"identifier-dollar", "using$foo=null;", true, 0, false, 27, 0, "none", false},
  {"identifier-unicode", "using\u00e9=null;", true, 0, false, 27, 0, "none", false},
  {"identifier-combining", "using\u0301=null;", true, 0, false, 27, 0, "none", false},
  {"using-binding-dollar", "using await$foo=null;", true, 1, false, 27, 1, "none", false},
  {"eager-using-as", "using as T;", false, 1, false, 28, 2, "none", false},
  {"eager-using-satisfies", "using satisfies T;", false, 1, false, 30, 2, "none", false},
  {"for-of-using", "async function f(){for(using r of xs){}}", true, 1, false, 32, 1, "of", false},
  {"for-await-of-using", "async function f(){for await(using r of xs){}}", true, 1, false, 33, 1, "of", true},
  {"for-of-await-using", "async function f(){for(await using r of xs){}}", true, 1, true, 34, 1, "of", false},
  {"for-await-of-await-using", "async function f(){for await(await using r of xs){}}", true, 1, true, 35, 1, "of", true},
  {"for-classic-using", "async function f(){for(using r=null; false; ){}}", true, 1, false, 36, 1, "classic", false},
  {"for-classic-await-using", "async function f(){for(await using r=null; false; ){}}", true, 1, true, 37, 1, "classic", false},
  {"for-of-using-self-name", "async function f(){for(using using of xs){}}", true, 1, false, 38, 1, "of", false},
  {"for-of-await-using-of-name", "async function f(){for(await using of of xs){}}", true, 1, true, 39, 1, "of", false},
  {"for-classic-using-of-name", "async function f(){for(using of=null; false; ){}}", true, 1, false, 40, 1, "classic", false},
  {"for-classic-of-noinit", "async function f(){for(using of; false; ){}}", true, 1, false, 41, 1, "classic", false},
  {"for-of-using-of-typed-name", "async function f(){for(using of: T of xs){}}", true, 1, false, 42, 1, "of", false},
  {"for-in-using", "async function f(){for(using r in obj){}}", true, 1, false, 43, 1, "in", false},
  {"for-in-await-using", "async function f(){for(await using r in obj){}}", true, 1, true, 44, 1, "in", false},
  {"for-of-initializer", "async function f(){for(using r=null of xs){}}", true, 1, false, 45, 1, "of", false},
  {"for-of-multiple", "async function f(){for(using a,b of xs){}}", true, 1, false, 46, 2, "of", false},
  {"for-of-annotation", "async function f(){for(using r:T of xs){}}", true, 1, false, 48, 1, "of", false},
  {"for-of-object-binding", "async function f(){for(using {x} of xs){}}", true, 1, false, 49, 1, "of", false},
  {"for-await-newline-using", "async function f(){for(await\nusing r of xs){}}", true, 1, true, 50, 1, "of", false},
  {"for-using-type-in", "async function f(){for(using r=key in obj){}}", true, 1, false, 51, 1, "in", false},
  {"for-using-parenthesized-in", "async function f(){for(using r=(key in obj); false; ){}}", true, 1, false, 52, 1, "classic", false},
  {"for-ordinary-using-of", "async function f(){for(using of xs){}}", true, 0, false, 53, 0, "of", false},
  {"for-await-ordinary-using-of", "async function f(){for await(using of xs){}}", true, 0, false, 53, 0, "of", true},
  {"for-ordinary-using-in", "async function f(){for(using in obj){}}", true, 0, false, 53, 0, "in", false},
  {"missing-initializer-expression", "using r=;", false, 1, false, 53, 1, "none", false},
  {"hard-reserved-binding", "using null=null;", false, 0, false, 54, 0, "none", false},
  {"hard-reserved-later-binding", "using a=null,true=null;", false, 1, false, 54, 1, "none", false},
  {"double-comma", "using r=null,,s=null;", false, 1, false, 55, 2, "none", false},
  {"for-using-newline-binding", "for(using\nr of xs){}", false, 0, false, 57, 0, "classic", false},
  {"for-await-using-newline-binding", "async function f(){for(await using\nr of xs){}}", false, 0, false, 57, 0, "classic", false},
  {"for-using-of-of", "for(using of of xs){}", false, 0, false, 57, 0, "of", false},
  {"for-await-classic", "async function f(){for await(using r=null; false; ){}}", false, 1, false, 57, 1, "classic", false},
  {"for-missing-of-expression", "for(using r of ){}", false, 1, false, 58, 1, "of", false},
  {"for-definite", "for(using r! of xs){}", false, 1, false, 59, 1, "classic", false},
  {"export-using", "export using r=null;", true, 1, false, 60, 1, "none", false},
  {"declare-using", "declare using r: Disposable;", true, 1, false, 61, 1, "none", false},
  {"ambient-using", "declare namespace N {using r: Disposable;}", true, 1, false, 62, 1, "none", false},
  {"await-using-script", "await using r=null;", true, 1, true, 63, 1, "none", false},
  {"await-using-sync-function", "function f(){await using r=null;}", true, 1, true, 64, 1, "none", false},
  {"await-using-static-block", "class C{static{await using r=null;}}", true, 1, true, 65, 1, "none", false},
  {"single-if-body", "if(ok) using r=null;", true, 1, false, 66, 1, "none", false},
  {"single-loop-body", "while(ok) using r=null;", true, 1, false, 67, 1, "none", false},
  {"using-nondisposable-type", "using r=1;", true, 1, false, 68, 1, "none", false},
  {"export-default-using", "export default using r=null;", false, 0, false, 69, 0, "none", false},
  {"declare-await-using", "declare await using r: Disposable;", true, 1, true, 69, 1, "none", false},
  {"escaped-using-keyword", "\\u0075sing r=null;", false, 1, false, 70, 1, "none", false},
  {"escaped-using-binding", "using r\\u0065s=null;", true, 1, false, 71, 1, "none", false},
  {"for-of-array-expression", "for(using[x] of xs){}", true, 0, false, 72, 0, "of", false},
  {"for-await-using-empty", "async function f(){for(await using of xs){}}", true, 1, true, 72, 0, "of", false},
  {"for-await-await-using-empty", "async function f(){for await(await using of xs){}}", true, 1, true, 72, 0, "of", true},
  {"for-resource-trailing-before-of", "for(using r, of xs){}", false, 1, false, 72, 4, "classic", false},
  {"for-resource-trailing-before-in", "for(using r, in obj){}", true, 1, false, 76, 1, "in", false},
  {"for-resource-trailing-semicolon", "for(using r=null,; false; ){}", true, 1, false, 77, 1, "classic", false},
  {"for-resource-noin-arrow", "for(using r=()=>key in obj; false; ){}", false, 1, false, 78, 1, "in", false},
  {"for-resource-noin-arrow-forin", "for(using r=()=>key in obj){}", true, 1, false, 79, 1, "in", false},
  {"for-resource-noin-paren-arrow", "for(using r=(()=>key in obj); false; ){}", true, 1, false, 80, 1, "classic", false},
  {"for-resource-noin-conditional", "for(using r=ok?key in obj:null; false; ){}", true, 1, false, 81, 1, "classic", false},
  {"for-resource-noin-call", "for(using r=f(key in obj); false; ){}", true, 1, false, 82, 1, "classic", false},
  {"for-resource-noin-function", "for(using r=function(){return key in obj}; false; ){}", true, 1, false, 83, 1, "classic", false},
  {"async-object-resource", "const o={async m(){await using r=null;}};", true, 1, true, 84, 1, "none", false},
  {"async-class-resource", "class C{async m(){await using r=null;}}", true, 1, true, 85, 1, "none", false},
  {"namespace-await-resource", "namespace N{await using r=null;}", true, 1, true, 86, 1, "none", false},
  {"async-await-binding", "async function f(){using await=null;}", true, 1, false, 87, 1, "none", false},
  {"internal-import-await-resource", "import q=N.M; await using r=null;", true, 1, true, 88, 1, "none", false},
  {"external-import-await-resource", "import q=require(\"m\"); await using r=null;", true, 1, true, 89, 1, "none", false},
  {"namespace-export-await-resource", "export as namespace N; await using r=null;", true, 1, true, 90, 1, "none", false},
  {"import-meta-await-resource", "import.meta; await using r=null;", true, 1, true, 91, 1, "none", false},
  {"declare-linebreak-using", "declare\nusing r=null;", true, 1, false, 92, 1, "none", false},
  {"comment-trailing-comma", "using r=null,/*end*/;", true, 1, false, 93, 1, "none", false},
  {"for-using-lf-of-equals", "for(using\nof=null; false;){}", true, 1, false, 94, 1, "classic", false},
  {"for-using-lf-of-semicolon", "for(using\nof; false;){}", true, 1, false, 95, 1, "classic", false},
  {"for-using-lf-of-colon", "for(using\nof:T of xs){}", true, 1, false, 96, 1, "of", false},
  {"for-using-comment-lf-of", "for(using/*\n*/of=null; false;){}", true, 1, false, 97, 1, "classic", false},
  {"for-using-lf-of-expression", "for(using\nof xs){}", true, 0, false, 98, 0, "of", false},
  {"for-await-using-lf-of-binding", "async function f(){for(await using\nof=null; false;){}}", false, 0, false, 98, 0, "of", false},
  {"statement-using-lf-of", "using\nof=null;", true, 0, false, 98, 0, "none", false},
  {"for-using-lf-hard-binding", "for(using\nr of xs){}", false, 0, false, 98, 0, "classic", false},
  {"escaped-let-binding", "using \\u006cet=null;", true, 1, false, 98, 1, "none", false},
  {"escaped-braced-let-binding", "using \\u{006c}et=null;", true, 1, false, 99, 1, "none", false},
  {"escaped-async-await-binding", "async function f(){using a\\u0077ait=null;}", true, 1, false, 100, 1, "none", false},
  {"escaped-generator-yield-binding", "function* f(){using \\u0079ield=null;}", true, 1, false, 101, 1, "none", false},
  {"escaped-ordinary-binding", "using r\\u0065s=null;", true, 1, false, 102, 1, "none", false},
};
inline bool run(const typescript::TypeScriptCoreServices& services) {
  bool ok=true;
  const auto check=[&](bool valid,const char* label,const char* source) {
    if(!valid){ok=false;std::fprintf(stderr,"typescript resources: %s source=[%s]\n",label,source);}
  };
  for(const auto& test:cases) {
    const auto result=services.parser->parse(test.source);
    const bool clean=result.accepted&&!result.cst().recovered()&&result.diagnostics.empty();
    check(clean==test.clean,test.name,test.source);
    if(clean!=test.clean)for(const auto& d:result.diagnostics)
      std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
    if(!clean||!test.clean)continue;
    check(result.cst().text()==test.source,"CST source retained",test.source);
    std::vector<const ast::VarDecl*> lists;
    unsigned loops=0;
    for(const auto* node:result.nodes) {
      if(const auto* declaration=cursiv::ast_cast<ast::VarDecl>(node); declaration&&declaration->kind=="using")lists.push_back(declaration);
      const ast::VarDecl* initializer=nullptr;
      const ast::Expr* expression=nullptr;
      const ast::Stmt* body=nullptr;
      const char* loopKind=nullptr;
      bool forAwait=false;
      if(const auto* loop=cursiv::ast_cast<ast::ForOfStmt>(node)) {
        loopKind="of";forAwait=loop->isAwait;initializer=loop->leftDecl;expression=loop->leftExpr;body=loop->body;
      } else if(const auto* loop=cursiv::ast_cast<ast::ForInStmt>(node)) {
        loopKind="in";initializer=loop->leftDecl;expression=loop->leftExpr;body=loop->body;
      } else if(const auto* loop=cursiv::ast_cast<ast::ForStmt>(node)) {
        loopKind="classic";initializer=loop->initDecl;expression=loop->initExpr;body=loop->body;
      }
      if(loopKind) {
        ++loops;
        check(std::string_view(loopKind)==test.loop&&forAwait==test.forAwait&&
                  (test.resources?initializer&&initializer->kind=="using"&&initializer->parent==node
                                 :!initializer&&expression&&expression->parent==node)&&
                  body&&body->parent==node,"loop kind/await axes/initializer ownership",test.source);
      }
    }
    check(lists.size()==test.resources,"contextual using resource-list count",test.source);
    check(loops==(std::string_view(test.loop)=="none"?0u:1u),"loop count",test.source);
    if(lists.size()!=1||test.resources!=1)continue;
    const auto* list=lists[0];
    const std::string_view name=test.name;
    const bool trailing=name=="using-trailing-comma"||name=="for-resource-trailing-before-in"||
        name=="for-resource-trailing-semicolon"||name=="comment-trailing-comma";
    const bool declared=name=="declare-using"||name=="declare-await-using";
    check(list->hasTrailingComma==trailing&&list->isDeclare==declared,
          "resource trailing-comma/declare syntax retained",test.source);
    check(list->isAwaitUsing==test.awaitUsing&&list->parent&&list->declarations.size()==test.bindingCount,
          "resource kind/await/binding count",test.source);
    if(list->declarations.size()!=test.bindingCount)continue;
    for(unsigned i=0;i<test.bindingCount;++i) {
      const auto* binding=list->declarations[i];const auto& expected=bindings[test.bindingAt+i];
      const auto* ident=binding?cursiv::ast_cast<ast::Ident>(binding->id):nullptr;
      bool name=std::string_view(expected.kind)=="ident"?ident&&ident->name==expected.name:
          std::string_view(expected.kind)=="object"?binding&&cursiv::ast_cast<ast::ObjectLit>(binding->id):
          binding&&cursiv::ast_cast<ast::ArrayLit>(binding->id);
      check(binding&&binding->parent==list&&binding->id&&binding->id->parent==binding&&name&&
                binding->definite==expected.definite&&bool(binding->typeAnnotation)==expected.type&&
                bool(binding->init)==expected.init&&
                (!binding->typeAnnotation||binding->typeAnnotation->parent==binding)&&
                (!binding->init||binding->init->parent==binding),
            "declarator spelling/kind/type/init/ownership",test.source);
    }
    const auto* first=list->declarations.empty()?nullptr:list->declarations[0];
    const auto* init=first?first->init:nullptr;
    const auto isIn=[](const ast::Expr* expression) {
      const auto* binary=cursiv::ast_cast<ast::BinaryExpr>(expression);
      const auto* left=binary?cursiv::ast_cast<ast::Ident>(binary->left):nullptr;
      const auto* right=binary?cursiv::ast_cast<ast::Ident>(binary->right):nullptr;
      return binary&&binary->op=="in"&&left&&left->name=="key"&&right&&right->name=="obj"&&
          left->parent==binary&&right->parent==binary;
    };
    if(name=="for-resource-noin-arrow-forin"||name=="for-resource-noin-paren-arrow") {
      const auto* arrow=cursiv::ast_cast<ast::ArrowFunctionExpr>(init);
      const auto* ident=arrow?cursiv::ast_cast<ast::Ident>(arrow->exprBody):nullptr;
      check(arrow&&arrow->exprBody&&arrow->exprBody->parent==arrow&&
                (name=="for-resource-noin-paren-arrow"?isIn(arrow->exprBody):ident&&ident->name=="key"),
            "NoIn reaches concise arrow; parentheses restore in-expression AST",test.source);
    } else if(name=="for-resource-noin-conditional") {
      const auto* conditional=cursiv::ast_cast<ast::ConditionalExpr>(init);
      check(conditional&&isIn(conditional->consequent)&&conditional->consequent->parent==conditional,
            "conditional consequent restores in-expression AST",test.source);
    } else if(name=="for-resource-noin-call") {
      const auto* call=cursiv::ast_cast<ast::CallExpr>(init);
      check(call&&call->args.size()==1&&isIn(call->args[0])&&call->args[0]->parent==call,
            "call argument restores in-expression AST",test.source);
    } else if(name=="for-resource-noin-function") {
      const auto* fn=cursiv::ast_cast<ast::FunctionExpr>(init);
      const auto* ret=fn&&fn->body&&fn->body->body.size()==1?
          cursiv::ast_cast<ast::ReturnStmt>(fn->body->body[0]):nullptr;
      check(ret&&isIn(ret->argument)&&ret->argument->parent==ret,
            "function block restores in-expression AST",test.source);
    }
  }
  {
    const char* source="using();using.x;using[x];using?.();using=acquire();";
    const auto parsed=services.parser->parse(source);const auto* root=parsed.value_as<ast::Program>();
    const auto expression=[&](std::size_t index)->const ast::Expr* {
      const auto* statement=root&&root->body.size()==5?cursiv::ast_cast<ast::ExprStmt>(root->body[index]):nullptr;
      return statement?statement->expression:nullptr;
    };
    const auto* member=cursiv::ast_cast<ast::MemberExpr>(expression(1));
    const auto* element=cursiv::ast_cast<ast::MemberExpr>(expression(2));
    const auto* optional=cursiv::ast_cast<ast::CallExpr>(expression(3));
    check(parsed.accepted&&!parsed.cst().recovered()&&parsed.diagnostics.empty()&&
              cursiv::ast_cast<ast::CallExpr>(expression(0))&&member&&!member->computed&&
              element&&element->computed&&optional&&optional->optional&&
              cursiv::ast_cast<ast::AssignExpr>(expression(4)),
          "ordinary using calls/properties/indexing/assignment keep expression AST",source);
  }
  struct Validation {const char* source; const char* code;};
  for(const auto test:{
      Validation{"using r;","typescript.using.initializer"},
      Validation{"using {x}=obj;","typescript.using.binding-pattern"},
      Validation{"using a=null,[x]=obj;","typescript.using.binding-pattern"},
      Validation{"for(using r in obj){}","typescript.using.for-in"},
      Validation{"for(using r=null of xs){}","typescript.using.for-of-initializer"},
      Validation{"for(using r:T of xs){}","typescript.using.for-of-type"},
      Validation{"for(using r,s of xs){}","typescript.using.for-of-count"},
      Validation{"export using r=null;","typescript.using.modifier"},
      Validation{"declare await using r:Disposable;","typescript.using.modifier"},
      Validation{"declare namespace N{using r:Disposable;}","typescript.using.ambient"},
      Validation{"using r=null,;","typescript.using.trailing-comma"},
      Validation{"using let=null;","typescript.using.binding-name"},
      Validation{"async function f(){using await=null;}","typescript.using.binding-name"},
      Validation{"using \\u006cet=null;","typescript.using.binding-name"},
      Validation{"using \\u{006c}et=null;","typescript.using.binding-name"},
      Validation{"async function f(){using a\\u0077ait=null;}","typescript.using.binding-name"},
      Validation{"function* f(){using \\u0079ield=null;}","typescript.using.binding-name"},
      Validation{"using r!:Disposable=null;","typescript.using.definite"},
      Validation{"if(ok) using r=null;","typescript.using.block"},
      Validation{"await using r=null;","typescript.using.await-context"},
      Validation{"namespace N{await using r=null;}","typescript.using.await-context"},
      Validation{"import q=N.M; await using r=null;","typescript.using.await-context"},
      Validation{"export as namespace N; await using r=null;","typescript.using.await-context"},
      Validation{"function f(){await using r=null;}","typescript.using.await-context"},
      Validation{"class C{static{await using r=null;}}","typescript.using.await-static"},
      Validation{"async function f(){for(await using of xs){}}","typescript.using.empty"},
      Validation{"using r=null;",""},
      Validation{"using await=null;",""},
      Validation{"using r\\u0065s=null;",""},
      Validation{"import.meta; await using r=null;",""},
      Validation{"import q=require(\"m\"); await using r=null;",""},
      Validation{"export {}; await using r=null;",""},
      Validation{"async function f(){for await(await using r of xs){}}",""},
      Validation{"const o={async m(){await using r=null;}};",""},
      Validation{"using[x]=obj; using(); using.x;",""}}) {
    const auto parsed=services.parser->parse(test.source);
    check(parsed.accepted&&!parsed.cst().recovered()&&parsed.diagnostics.empty(),
          "checker-only restriction must parse clean",test.source);
    const auto validation=cursiv::test::validationHelper(services,test.source);
    bool found=false,any=false,range=true;
    for(const auto& d:validation.diagnostics)if(d.code&&d.code->isString()&&d.code->string().starts_with("typescript.using.")) {
      any=true;found|=*d.code==test.code;
      range&=d.range.begin<=d.range.end&&d.range.end<=std::string_view(test.source).size();
    }
    check((*test.code?found:!any)&&range,"local resource validation/code/range",test.source);
  }
  return ok;
}
} // namespace typescript_resources
