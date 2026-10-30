#include "native_followup.hpp"
#include "resource_cases.hpp"
#include "assertion_precedence.hpp"
#include "audit_regressions.hpp"
#include "lexical_cases.hpp"
#include "template_converter_cases.hpp"
// The smoke test of the language: parse every file under example/, the way a
// workspace would build one.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "typescript/core/CoreServices.hpp"
#include "typescript/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "typescript: %s\n", what.c_str());
  return 1;
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.path().extension() == ".ts") files.push_back(entry.path());
  if (files.empty()) return fail("no example files found");

  const auto language = cursiv::create_language<
      typescript::TypeScriptCoreServices, typescript::TypeScriptSharedCoreServices,
      typescript::install_core_module>();

  if (!typescript_native_followup::run(*language.language.parser)) return 1;
  if (!typescript_resources::run(language.language)) return 1;
  if (!typescript_lexical_cases(*language.language.parser)) return 1;
  if (!typescript_template_converter_cases::run(*language.language.parser)) return 1;
  if (!assertion_precedence::run(language.language)) return 1;

  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<typescript::ast::Program>)
      return fail(path.string() + ": the parse built no program");
  }

  {
    const auto value_of = [&](const std::string& stmt) -> const typescript::ast::Expr* {
      static std::vector<std::unique_ptr<std::string>> kept;
    static std::vector<cursiv::ParseResult> results;
      kept.push_back(std::make_unique<std::string>(stmt));
      results.push_back(language.language.parser->parse(*kept.back()));
      const auto& result = results.back();
      if (result.cst().recovered() || !result.diagnostics.empty())
        return nullptr;
      const auto* const program = as<typescript::ast::Program>(result.value);
      if (program == nullptr || program->body.empty()) return nullptr;
      const auto* const varDecl =
          as<typescript::ast::VarDecl>(program->body[0]);
      if (varDecl == nullptr || varDecl->declarations.empty()) return nullptr;
      return varDecl->declarations[0]->init;
    };
    const auto binary = [](const typescript::ast::Expr* e, const char* op)
        -> const typescript::ast::BinaryExpr* {
      const auto* const b = as<typescript::ast::BinaryExpr>(e);
      return b != nullptr && b->op == op ? b : nullptr;
    };

    // `a || b ?? c` -- `??` is looser than `||`: `(a || b) ?? c`.
    // (Real TypeScript refuses `a || b ?? c` unparenthesized; parens keep
    // the probe itself valid while still separating the two tiers.)
    {
      const auto* const nc =
          binary(value_of("let x = (a || b) ?? c;"), "??");
      if (nc == nullptr || binary(nc->left, "||") == nullptr)
        return fail("precedence: `??` is not looser than `||`");
    }
    // `a + b as number` is `(a + b) as number`: `as` looser than `+`.
    {
      const cursiv::ParseResult result =
          language.language.parser->parse("let x = a + b as number;");
      if (result.cst().recovered() || !result.diagnostics.empty())
        return fail("precedence: `a + b as number` did not parse clean");
      const auto* const program = as<typescript::ast::Program>(result.value);
      const auto* const varDecl =
          program != nullptr && !program->body.empty()
              ? as<typescript::ast::VarDecl>(program->body[0])
              : nullptr;
      const auto* const asExpr =
          varDecl != nullptr && !varDecl->declarations.empty()
              ? as<typescript::ast::AsExpr>(varDecl->declarations[0]->init)
              : nullptr;
      if (asExpr == nullptr || binary(asExpr->expression, "+") == nullptr)
        return fail("precedence: `as` did not bind looser than `+`");
    }
    // `cond ? a : b as T` is `cond ? a : (b as T)`: `as` tighter than `?:`.
    {
      const cursiv::ParseResult result =
          language.language.parser->parse("let x = cond ? a : b as T;");
      if (result.cst().recovered() || !result.diagnostics.empty())
        return fail("precedence: ternary+`as` did not parse clean");
      const auto* const program = as<typescript::ast::Program>(result.value);
      const auto* const varDecl =
          program != nullptr && !program->body.empty()
              ? as<typescript::ast::VarDecl>(program->body[0])
              : nullptr;
      const auto* const cond =
          varDecl != nullptr && !varDecl->declarations.empty()
              ? as<typescript::ast::ConditionalExpr>(
                    varDecl->declarations[0]->init)
              : nullptr;
      if (cond == nullptr ||
          as<typescript::ast::AsExpr>(cond->alternate) == nullptr)
        return fail("precedence: `as` did not bind tighter than `?:`");
    }
  }

  // Members, modifiers and type forms that once failed or read as something
  // else: each snippet parses clean to the tree its comment states.
  {
    namespace ast = typescript::ast;
    static std::vector<std::unique_ptr<std::string>> kept;
    static std::vector<cursiv::ParseResult> results;
    const auto program_of = [&](const char* text) -> const ast::Program* {
      kept.push_back(std::make_unique<std::string>(text));
      results.push_back(language.language.parser->parse(*kept.back()));
      const auto& result = results.back();
      if (result.cst().recovered() || !result.diagnostics.empty())
        return nullptr;
      return as<ast::Program>(result.value);
    };
    const auto first_member =
        [&](const char* text) -> const ast::ClassMember* {
      const auto* const program = program_of(text);
      const auto* const cls =
          program != nullptr && program->body.size() == 1
              ? as<ast::ClassDecl>(program->body[0])
              : nullptr;
      return cls != nullptr && cls->members.size() == 1 ? cls->members[0]
                                                        : nullptr;
    };
    const auto first_type_member =
        [&](const char* text) -> const ast::TypeMember* {
      const auto* const program = program_of(text);
      const auto* const decl =
          program != nullptr && program->body.size() == 1
              ? as<ast::InterfaceDecl>(program->body[0])
              : nullptr;
      return decl != nullptr && decl->members.size() == 1 ? decl->members[0]
                                                          : nullptr;
    };
    const auto alias_type = [&](const char* text) -> const ast::Type* {
      const auto* const program = program_of(text);
      const auto* const alias =
          program != nullptr && program->body.size() == 1
              ? as<ast::TypeAliasDecl>(program->body[0])
              : nullptr;
      return alias != nullptr ? alias->type : nullptr;
    };

    // A modifier word before a name: one member, not a field named after the
    // modifier followed by another.
    const auto* m = first_member("class A { readonly type: string; }");
    if (m == nullptr || !m->isReadonly)
      return fail("members: `readonly type: string` is not one readonly field");
    m = first_member("class A { static async *g() {} }");
    if (m == nullptr || !m->isStatic || !m->isAsync || !m->isGenerator)
      return fail("members: `static async *g() {}` lost a modifier");
    m = first_member("class A { declare x: number; }");
    if (m == nullptr || !m->isDeclare)
      return fail("members: `declare x: number` is not a declare field");
    m = first_member("class A { x!: number; }");
    if (m == nullptr || !m->definite)
      return fail("members: `x!: number` is not a definite field");
    m = first_member("class A { x?(): void; }");
    if (m == nullptr || !m->isMethod || !m->optional)
      return fail("members: `x?(): void` is not an optional method");
    m = first_member("class A { constructor(public readonly x: number) {} }");
    if (m == nullptr || m->params.size() != 1 || !m->params[0]->isReadonly ||
        m->params[0]->accessibility != "public")
      return fail("members: a parameter property lost `public readonly`");
    m = first_member("abstract class A { abstract if(): void; }");
    if (m == nullptr || m->abstractDecl == nullptr ||
        m->abstractDecl->name != "if")
      return fail("members: `abstract if(): void` is not an abstract method");
    m = first_member("abstract class A { abstract get x(): number; }");
    if (m == nullptr || m->abstractDecl == nullptr ||
        !m->abstractDecl->isGetter)
      return fail("members: `abstract get x()` is not an abstract getter");
    m = first_member("abstract class A { abstract readonly x: number; }");
    if (m == nullptr || m->abstractDecl == nullptr ||
        m->abstractDecl->declaration == nullptr ||
        !m->abstractDecl->declaration->isReadonly)
      return fail("members: `abstract readonly x` lost `readonly`");

    const auto* t = first_type_member("interface I { get x(): number; }");
    if (t == nullptr || !t->isGetter)
      return fail("interface: `get x(): number` is not a getter signature");
    t = first_type_member("interface I { readonly [k: string]: number; }");
    if (t == nullptr || !t->isIndex || !t->readonly_)
      return fail("interface: a readonly index signature lost `readonly`");

    // `[K in T]` is a mapped type whatever `T` is, and keeps its modifiers.
    const auto* mapped = as<ast::MappedType>(
        alias_type("type M = { -readonly [K in Keys]-?: V };"));
    if (mapped == nullptr || mapped->readonlyMod != "-readonly" ||
        mapped->optionalMod != "-?" || mapped->name != "K")
      return fail("types: a mapped type lost its modifiers or name");
    mapped = as<ast::MappedType>(
        alias_type("type M = { [K in keyof T as Foo<K>]: T[K] };"));
    if (mapped == nullptr || mapped->nameType == nullptr)
      return fail("types: a mapped type lost its `as` clause");
    const auto* readonly_type =
        as<ast::ReadonlyType>(alias_type("type R = readonly string[];"));
    if (readonly_type == nullptr ||
        as<ast::IndexedType>(readonly_type->operand) == nullptr)
      return fail("types: `readonly string[]` is not a readonly array type");

    const auto imports = [&](const char* text) -> const ast::ImportStmt* {
      const auto* const program = program_of(text);
      return program != nullptr && program->body.size() == 1
                 ? as<ast::ImportStmt>(program->body[0])
                 : nullptr;
    };
    const auto* imp = imports("import type { x as y } from \"m\";");
    if (imp == nullptr || !imp->isTypeOnly || imp->specifiers.size() != 1)
      return fail("modules: `import type { x as y }` is not type-only");
    imp = imports("import type from \"m\";");
    if (imp == nullptr || imp->isTypeOnly || imp->specifiers.size() != 1 ||
        imp->specifiers[0]->local != "type")
      return fail("modules: `import type from` is not a default named `type`");
    const auto* const program = program_of("export type { T };");
    const auto* const exp = program != nullptr && program->body.size() == 1
                                ? as<ast::ExportStmt>(program->body[0])
                                : nullptr;
    if (exp == nullptr || !exp->isTypeOnly)
      return fail("modules: `export type { T }` is not type-only");
  }

  // Additional constructs beyond the source grammar: each snippet
  // parses clean to the tree its failure message states.
  {
    namespace ast = typescript::ast;
    static std::vector<std::unique_ptr<std::string>> kept;
    static std::vector<cursiv::ParseResult> results;
    const auto program_of = [&](const char* text) -> const ast::Program* {
      kept.push_back(std::make_unique<std::string>(text));
      results.push_back(language.language.parser->parse(*kept.back()));
      const auto& result = results.back();
      if (result.cst().recovered() || !result.diagnostics.empty())
        return nullptr;
      return as<ast::Program>(result.value);
    };
    const auto stmt_of = [&](const char* text) -> const ast::Stmt* {
      const auto* const program = program_of(text);
      return program != nullptr && program->body.size() == 1 ? program->body[0]
                                                             : nullptr;
    };
    const auto init_of = [&](const char* text) -> const ast::Expr* {
      const auto* const decl = as<ast::VarDecl>(stmt_of(text));
      return decl != nullptr && decl->declarations.size() == 1
                 ? decl->declarations[0]->init
                 : nullptr;
    };
    const auto alias_of = [&](const char* text) -> const ast::Type* {
      const auto* const alias = as<ast::TypeAliasDecl>(stmt_of(text));
      return alias != nullptr ? alias->type : nullptr;
    };
    const auto return_of = [&](const char* text) -> const ast::Type* {
      const auto* const fn = as<ast::FunctionDecl>(stmt_of(text));
      return fn != nullptr ? fn->returnType : nullptr;
    };

    // Conditional types nest on the false branch; `infer` takes a constraint.
    const auto* cond = as<ast::ConditionalType>(alias_of(
        "type A<T> = T extends string ? 1 : T extends number ? 2 : 3;"));
    if (cond == nullptr || as<ast::TypeRef>(cond->checkType) == nullptr ||
        as<ast::PredefinedType>(cond->extendsType) == nullptr ||
        as<ast::ConditionalType>(cond->falseType) == nullptr)
      return fail("types: a conditional type lost a branch");
    cond = as<ast::ConditionalType>(alias_of(
        "type A<T> = T extends [infer H extends string] ? H : never;"));
    const auto* const tuple =
        cond != nullptr ? as<ast::TupleType>(cond->extendsType) : nullptr;
    const auto* const infer = tuple != nullptr && tuple->elements.size() == 1
                                  ? as<ast::InferType>(tuple->elements[0])
                                  : nullptr;
    if (infer == nullptr || infer->typeParam == nullptr ||
        infer->typeParam->constraint == nullptr)
      return fail("types: `infer H extends string` lost its constraint");

    const auto* const tpl =
        as<ast::TemplateLiteralType>(alias_of("type A = `a${T}b${U}c`;"));
    if (tpl == nullptr || tpl->quasis.size() != 3 || tpl->types.size() != 2 ||
        tpl->quasis[1] != "b")
      return fail("types: a template literal type lost a span");

    const auto* const labeled = as<ast::TupleType>(
        alias_of("type A = [a: string, b?: number, ...rest: T[]];"));
    const auto* const member =
        labeled != nullptr && labeled->elements.size() == 3
            ? as<ast::NamedTupleMember>(labeled->elements[2])
            : nullptr;
    if (member == nullptr || !member->rest || member->name != "rest" ||
        as<ast::NamedTupleMember>(labeled->elements[1]) == nullptr ||
        !as<ast::NamedTupleMember>(labeled->elements[1])->optional)
      return fail("types: a labeled tuple lost a modifier");
    const auto* const bare =
        as<ast::TupleType>(alias_of("type A = [string?, ...T[]];"));
    if (bare == nullptr || bare->elements.size() != 2 ||
        as<ast::OptionalType>(bare->elements[0]) == nullptr ||
        as<ast::RestType>(bare->elements[1]) == nullptr)
      return fail(
          "types: `[string?, ...T[]]` lost an optional or rest element");

    // A predicate takes a whole type; `asserts` may have no `is`.
    const auto* pred = as<ast::TypePredicateType>(
        return_of("function f(x): x is string | number {}"));
    if (pred == nullptr || pred->asserts ||
        as<ast::TypeBinExpr>(pred->type) == nullptr)
      return fail("types: `x is string | number` is not one predicate");
    pred = as<ast::TypePredicateType>(return_of("function f(x): asserts x {}"));
    if (pred == nullptr || !pred->asserts || pred->type != nullptr)
      return fail("types: `asserts x` is not an assertion without a type");
    pred = as<ast::TypePredicateType>(
        return_of("function f(this: A): asserts this is B {}"));
    if (pred == nullptr || !pred->asserts ||
        as<ast::ThisType>(pred->param) == nullptr)
      return fail("types: `asserts this is B` lost its `this`");
    if (as<ast::ThisType>(return_of("function f(): this {}")) == nullptr)
      return fail("types: a `this` return type is not a ThisType");

    const auto* const ctor =
        as<ast::ConstructorType>(alias_of("type A = abstract new () => T;"));
    if (ctor == nullptr || !ctor->isAbstract)
      return fail("types: `abstract new () => T` is not abstract");
    const auto* const imported =
        as<ast::ImportType>(alias_of("type A = typeof import(\"m\").B.C<D>;"));
    if (imported == nullptr || !imported->isTypeOf ||
        imported->qualifier != "B.C" || imported->typeArgs.size() != 1)
      return fail("types: `typeof import(\"m\").B.C<D>` lost a part");
    const auto* const negative =
        as<ast::PredefinedType>(alias_of("type A = -1;"));
    if (negative == nullptr || negative->name != "-1")
      return fail("types: `-1` is not a literal type");

    // `as` and `satisfies` stand on the relational level.
    const auto* const eq = as<ast::BinaryExpr>(init_of("let x = a == b as T;"));
    if (eq == nullptr || eq->op != "==" ||
        as<ast::AsExpr>(eq->right) == nullptr)
      return fail("expressions: `a == b as T` is not `a == (b as T)`");
    const auto* const land =
        as<ast::BinaryExpr>(init_of("let x = a && b satisfies T;"));
    if (land == nullptr || as<ast::SatisfiesExpr>(land->right) == nullptr)
      return fail(
          "expressions: `a && b satisfies T` is not `a && (b satisfies T)`");
    const auto* const plus =
        as<ast::SatisfiesExpr>(init_of("let x = a + b satisfies T;"));
    if (plus == nullptr || as<ast::BinaryExpr>(plus->expression) == nullptr)
      return fail(
          "expressions: `a + b satisfies T` is not `(a + b) satisfies T`");

    const auto* meta = as<ast::MetaProperty>(init_of("let x = new.target;"));
    if (meta == nullptr || meta->meta != "new" || meta->property != "target")
      return fail("expressions: `new.target` is not a meta property");
    const auto* const url =
        as<ast::MemberExpr>(init_of("let x = import.meta.url;"));
    meta = url != nullptr ? as<ast::MetaProperty>(url->object) : nullptr;
    if (meta == nullptr || meta->meta != "import" || meta->property != "meta")
      return fail("expressions: `import.meta.url` lost its meta property");
    if (as<ast::ImportCallExpr>(init_of("let x = import(\"m\");")) == nullptr)
      return fail("expressions: `import(\"m\")` is not a dynamic import");

    // A generic arrow is not a cast of an arrow; a cast takes a unary operand.
    const auto* const arrow =
        as<ast::ArrowFunctionExpr>(init_of("let f = <T>(x: T) => x;"));
    if (arrow == nullptr || arrow->typeParams.size() != 1)
      return fail("expressions: `<T>(x: T) => x` is not a generic arrow");
    const auto* const sum = as<ast::BinaryExpr>(init_of("let x = <T>y + z;"));
    if (sum == nullptr || as<ast::TypeAssertionExpr>(sum->left) == nullptr)
      return fail("expressions: `<T>y + z` is not `(<T>y) + z`");

    // Ambient and module declarations.
    const auto* mod =
        as<ast::NamespaceDecl>(stmt_of("declare module \"m\" { }"));
    if (mod == nullptr || !mod->isDeclare || mod->kind != "module" ||
        mod->name != "\"m\"")
      return fail("declarations: `declare module \"m\" { }` lost a part");
    mod = as<ast::NamespaceDecl>(stmt_of("declare global { interface W { } }"));
    if (mod == nullptr || mod->kind != "global" || mod->body.size() != 1)
      return fail("declarations: `declare global { }` lost its body");
    mod = as<ast::NamespaceDecl>(stmt_of("module M.N { }"));
    if (mod == nullptr || mod->isDeclare || mod->name != "M.N")
      return fail("declarations: `module M.N { }` is not a module");
    const auto* const fn =
        as<ast::FunctionDecl>(stmt_of("declare function f(): T;"));
    if (fn == nullptr || !fn->isDeclare || fn->body != nullptr)
      return fail("declarations: `declare function f(): T;` lost `declare`");
    const auto* const cls =
        as<ast::ClassDecl>(stmt_of("declare abstract class C { }"));
    if (cls == nullptr || !cls->isDeclare || !cls->isAbstract)
      return fail("declarations: `declare abstract class` lost a modifier");
    const auto* const decl =
        as<ast::VarDecl>(stmt_of("declare const x: number;"));
    if (decl == nullptr || !decl->isDeclare)
      return fail("declarations: `declare const` lost `declare`");

    const auto* req =
        as<ast::ImportEqualsDecl>(stmt_of("import x = require(\"m\");"));
    if (req == nullptr || req->name != "x" || req->source != "\"m\"")
      return fail("modules: `import x = require(\"m\")` lost a part");
    const auto* const alias =
        as<ast::ImportEqualsDecl>(stmt_of("import x = N.M;"));
    if (alias == nullptr || alias->entity != "N.M")
      return fail("modules: `import x = N.M` lost its entity name");
    const auto* const equals = as<ast::ExportStmt>(stmt_of("export = x;"));
    if (equals == nullptr || !equals->isEquals ||
        equals->defaultExpr == nullptr)
      return fail("modules: `export = x` is not an export assignment");
    const auto* const ns =
        as<ast::NamespaceExportDecl>(stmt_of("export as namespace N;"));
    if (ns == nullptr || ns->name != "N")
      return fail("modules: `export as namespace N` lost its name");
    const auto* const decorated =
        as<ast::ExportStmt>(stmt_of("@dec export class A {}"));
    if (decorated == nullptr || decorated->decorators.size() != 1 ||
        as<ast::ClassDecl>(decorated->declaration) == nullptr)
      return fail("modules: `@dec export class` lost its decorator");

    // A `type` before a specifier is a modifier or the name, by what follows.
    const auto specifiers = [&](const char* text) -> const ast::ImportStmt* {
      return as<ast::ImportStmt>(stmt_of(text));
    };
    const auto* imp = specifiers("import { type X, y } from \"m\";");
    if (imp == nullptr || imp->specifiers.size() != 2 ||
        !imp->specifiers[0]->isTypeOnly || imp->specifiers[1]->isTypeOnly)
      return fail("modules: `import { type X, y }` mixed up the modifier");
    imp = specifiers("import { type as as } from \"m\";");
    if (imp == nullptr || imp->specifiers.size() != 1 ||
        imp->specifiers[0]->isTypeOnly ||
        imp->specifiers[0]->imported != "type" ||
        imp->specifiers[0]->local != "as")
      return fail(
          "modules: `import { type as as }` is not `type` renamed `as`");
    imp = specifiers("import { type as } from \"m\";");
    if (imp == nullptr || imp->specifiers.size() != 1 ||
        !imp->specifiers[0]->isTypeOnly || imp->specifiers[0]->imported != "as")
      return fail("modules: `import { type as }` is not a type-only `as`");
    const auto* const out = as<ast::ExportStmt>(stmt_of("export { type X };"));
    if (out == nullptr || out->specifiers.size() != 1 ||
        !out->specifiers[0]->isTypeOnly)
      return fail("modules: `export { type X }` is not a type-only specifier");

    // Members and signatures.
    const auto class_member = [&](const char* text) -> const ast::ClassMember* {
      const auto* const c = as<ast::ClassDecl>(stmt_of(text));
      return c != nullptr && c->members.size() == 1 ? c->members[0] : nullptr;
    };
    const auto* cm = class_member("abstract class A { abstract x?: T; }");
    if (cm == nullptr || cm->abstractDecl == nullptr ||
        cm->abstractDecl->declaration == nullptr ||
        !cm->abstractDecl->declaration->declarations[0]->optional)
      return fail("members: `abstract x?: T` lost its `?`");
    cm = class_member("class A { static declare y: number; }");
    if (cm == nullptr || !cm->isStatic || !cm->isDeclare)
      return fail("members: `static declare y` lost a modifier");
    cm = class_member("class A { protected override m(): void {} }");
    if (cm == nullptr || !cm->isOverride || cm->accessibility != "protected")
      return fail("members: `protected override m()` lost a modifier");
    cm = class_member("class A { [key: string]: number; }");
    if (cm == nullptr || !cm->isIndex || cm->indexName != "key")
      return fail("members: a class index signature lost its name");
    cm = class_member("declare class A { get x(): number; }");
    if (cm == nullptr || !cm->isGetter || cm->body != nullptr)
      return fail("members: an ambient getter needs no body");

    const auto* const iface = as<ast::InterfaceDecl>(
        stmt_of("interface I { [type: string]: T; <U>(x: U): U }"));
    if (iface == nullptr || iface->members.size() != 2 ||
        !iface->members[0]->isIndex || iface->members[0]->indexName != "type" ||
        !iface->members[1]->isCall || iface->members[1]->typeParams.size() != 1)
      return fail("members: an index or generic call signature lost a part");
    const auto* const lines = as<ast::InterfaceDecl>(stmt_of(
        "interface I {\n a: string\n m(): void\n readonly b?: number\n}"));
    if (lines == nullptr || lines->members.size() != 3)
      return fail("members: line breaks do not separate interface members");

    const auto* const obj =
        as<ast::ObjectLit>(init_of("let o = { m<T>(x: T): T { return x; } };"));
    const auto* const prop = obj != nullptr && obj->properties.size() == 1
                                 ? obj->properties[0]
                                 : nullptr;
    const auto* const method =
        prop != nullptr ? as<ast::FunctionExpr>(prop->value) : nullptr;
    if (method == nullptr || !prop->isMethod ||
        method->typeParams.size() != 1 || method->returnType == nullptr)
      return fail("members: a typed object method lost its type or parameter");
    if (results.back().cst().text(method->cstNodeId) !=
        "<T>(x: T): T { return x; }")
      return fail("members: a generic object method starts after its `<`");

    const auto* const plain_obj =
        as<ast::ObjectLit>(init_of("let o = { m(x) { return x; } };"));
    const auto* const plain_method =
        plain_obj != nullptr && plain_obj->properties.size() == 1
            ? as<ast::FunctionExpr>(plain_obj->properties[0]->value)
            : nullptr;
    if (plain_method == nullptr || !plain_obj->properties[0]->isMethod ||
        results.back().cst().text(plain_method->cstNodeId) !=
            "(x) { return x; }")
      return fail("members: an object method starts after its `(`");

    const auto* const lines_of_calls = as<ast::InterfaceDecl>(
        stmt_of("interface I {\n (a: object): Node\n <T>(a: T): Node[]\n}"));
    if (lines_of_calls == nullptr || lines_of_calls->members.size() != 2 ||
        !lines_of_calls->members[0]->isCall ||
        !lines_of_calls->members[1]->isCall)
      return fail("members: call signatures on separate lines lost one");

    // An `extends` that is no type reference is an expression.
    const auto* heritage =
        as<ast::ClassDecl>(stmt_of("class A extends mixin(B, C) {}"));
    if (heritage == nullptr || heritage->superClass != nullptr ||
        as<ast::CallExpr>(heritage->superExpr) == nullptr)
      return fail("classes: `extends mixin(B, C)` is not an expression");
    heritage = as<ast::ClassDecl>(stmt_of("class A extends B.C<D> {}"));
    if (heritage == nullptr || heritage->superExpr != nullptr ||
        as<ast::TypeRef>(heritage->superClass) == nullptr)
      return fail("classes: `extends B.C<D>` is not a type reference");

    const auto* const definite = as<ast::VarDecl>(stmt_of("let x!: number;"));
    if (definite == nullptr || !definite->declarations[0]->definite)
      return fail("declarations: `let x!: number` lost its `!`");
    const auto* const overload =
        as<ast::ExportStmt>(stmt_of("export default function (): T;"));
    if (overload == nullptr || !overload->isDefault ||
        as<ast::FunctionDecl>(overload->declaration) == nullptr)
      return fail(
          "modules: `export default function (): T;` is not a declaration");
    const auto* const label =
        as<ast::LabeledStmt>(stmt_of("type: for (;;) { break type; }"));
    if (label == nullptr || label->label != "type")
      return fail("statements: `type` is not a label");

    const auto* const shebang = program_of("#!/usr/bin/env node\nlet x;");
    if (shebang == nullptr || shebang->prologue != "#!/usr/bin/env node" ||
        shebang->body.size() != 1)
      return fail("text: a `#!` line is not the program's prologue");

    const auto* const two = program_of("function f() {} function g() {}");
    if (two == nullptr || two->body.size() != 2)
      return fail("statements: a function body needs no terminator");

    // Contextual words are names outside their own constructs.
    const auto* const named =
        as<ast::ExprStmt>(stmt_of("declare = readonly + is;"));
    if (named == nullptr || as<ast::AssignExpr>(named->expression) == nullptr)
      return fail("identifiers: `declare`, `readonly`, `is` are not names");
    if (as<ast::ExprStmt>(stmt_of("module.exports = 1;")) == nullptr)
      return fail("identifiers: `module.exports` is not an expression");
  }

  std::printf("typescript: %zu example(s) parse clean\n", files.size());
  return 0;
}
