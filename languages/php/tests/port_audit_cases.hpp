#pragma once

#include <cstdio>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "php/core/ast.hpp"

namespace port_audit_cases {

template <class T>
const T* as(const cursiv::AstNode* node) { return cursiv::ast_cast<T>(node); }

inline bool run(const cursiv::Parser& parser) {
  using namespace php;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "php port audit: %s\n", message); ok = false; }
  };
  const auto inspect = [&](const char* text, auto inspect_block) {
    const std::string source = std::string("<?php ") + text;
    const auto r = parser.parse(source);
    const auto* doc = as<ast::Document>(r.value);
    const auto* block = doc && doc->items.size() == 1 ? as<ast::PhpBlock>(doc->items[0]) : nullptr;
    const bool clean = r.accepted && !r.cst().recovered() && r.diagnostics.empty() && block;
    if (!clean) std::fprintf(stderr, "php port audit input: %s\n", text);
    check(clean, "expected a clean PHP block");
    if (block) inspect_block(*block);
  };
  const auto value = [](const ast::Item* item) -> const ast::Expr* {
    const auto* stmt = as<ast::ExprStmt>(item);
    const auto* assign = stmt ? as<ast::AssignExpr>(stmt->value) : nullptr;
    return assign ? assign->value : stmt ? stmt->value : nullptr;
  };
  const auto sole_value = [&](const ast::PhpBlock& b) {
    check(b.items.size() == 1, "one statement expected");
    return b.items.size() == 1 ? value(b.items[0]) : nullptr;
  };
  const auto dnf = [](const ast::TypeRef* t) {
    const auto* u = as<ast::UnionTypeRef>(t);
    const auto* i = u && u->options.size() == 2 ? as<ast::IntersectionTypeRef>(u->options[0]) : nullptr;
    return i && i->options.size() == 2 && as<ast::NamedTypeRef>(u->options[1]) &&
           i->parent == u && i->options[0]->parent == i;
  };
  inspect("namespace École; class Élève {const 名=1; function été(){}}", [&](const auto& b) {
    const auto* ns = b.items.size() == 2 ? as<ast::NamespaceDecl>(b.items[0]) : nullptr;
    const auto* c = b.items.size() == 2 ? as<ast::ClassDecl>(b.items[1]) : nullptr;
    const auto* k = c && c->members.size() == 2 ? as<ast::ConstMember>(c->members[0]) : nullptr;
    const auto* m = c && c->members.size() == 2 ? as<ast::MethodMember>(c->members[1]) : nullptr;
    check(ns && ns->name && ns->name->parts.size() == 1 && ns->name->parts[0] == "École" &&
          c && c->name == "Élève" && k && k->consts.size() == 1 && k->consts[0]->name == "名" &&
          m && m->function && m->function->name == "été", "non-ASCII names retain every declaration");
  });
  inspect("function f((A&B)|C $x, A&B & /*gap*/ $y): (A&B)|C {return $x;}", [&](const auto& b) {
    const auto* f = b.items.size() == 1 ? as<ast::FunctionDecl>(b.items[0]) : nullptr;
    const auto* i = f && f->params.size() == 2 ? as<ast::IntersectionTypeRef>(f->params[1]->type) : nullptr;
    check(f && f->params.size() == 2 && dnf(f->params[0]->type) && dnf(f->returnType) &&
          i && i->options.size() == 2 && f->params[1]->byRef,
          "DNF types retain grouping and distinguish the parameter reference marker");
  });
  inspect("class C {public ?string $x=null;}", [&](const auto& b) {
    const auto* c = b.items.size() == 1 ? as<ast::ClassDecl>(b.items[0]) : nullptr;
    const auto* p = c && c->members.size() == 1 ? as<ast::PropertyMember>(c->members[0]) : nullptr;
    const auto* n = p ? as<ast::NullableTypeRef>(p->type) : nullptr;
    check(n && as<ast::PrimitiveTypeRef>(n->type) && n->type->parent == n,
          "nullable property types retain their underlying type");
  });
  inspect("$f = static fn &(int &$x): ?int => $x;", [&](const auto& b) {
    const auto* f = as<ast::Closure>(sole_value(b));
    const auto* n = f ? as<ast::NullableTypeRef>(f->returnType) : nullptr;
    check(f && f->isStatic && f->byRef && f->params.size() == 1 && f->params[0]->byRef &&
          f->arrowBody && !f->body && n && n->parent == f,
          "arrow functions preserve static, reference, result and expression body");
  });
  inspect("$f = static function &(#[A] int &$x) use (&$v, $u,): ?int {return $x;};", [&](const auto& b) {
    const auto* f = as<ast::Closure>(sole_value(b));
    check(f && f->isStatic && f->byRef && f->params.size() == 1 &&
          f->params[0]->attributes.size() == 1 && f->uses.size() == 2 &&
          f->uses[0]->name == "$v" && f->uses[0]->byRef && !f->uses[1]->byRef &&
          f->uses[0]->parent == f && f->body && as<ast::NullableTypeRef>(f->returnType),
          "closures preserve parameter attributes and capture references");
  });
  inspect("function f(){yield 'key'=>42; $x = yield $v; yield from $it;}", [&](const auto& b) {
    const auto* f = b.items.size() == 1 ? as<ast::FunctionDecl>(b.items[0]) : nullptr;
    const auto* body = f ? as<ast::BlockStmt>(f->body) : nullptr;
    const auto* pair = body && body->items.size() == 3 ? as<ast::YieldExpr>(value(body->items[0])) : nullptr;
    const auto* key = pair ? as<ast::StringLit>(pair->key) : nullptr;
    const auto* val = pair ? as<ast::IntLit>(pair->value) : nullptr;
    const auto* assigned = body && body->items.size() == 3 ? as<ast::YieldExpr>(value(body->items[1])) : nullptr;
    const auto* from = body && body->items.size() == 3 ? as<ast::YieldExpr>(value(body->items[2])) : nullptr;
    check(key && key->text == "'key'" && val && val->text == "42" && pair->key->parent == pair &&
          assigned && as<ast::Var>(assigned->value) && from && from->from && as<ast::Var>(from->value),
          "yield keeps key/value order, assignment operands and delegation");
  });
  for (const char* text : {"$x = new C(1, name: 2);", "$x = f(1, name: 2);"}) {
    inspect(text, [&](const auto& b) {
      const auto* e = sole_value(b);
      const auto* n = as<ast::NewExpr>(e);
      const auto* c = as<ast::CallExpr>(e);
      const auto* args = n ? &n->args : c ? &c->args : nullptr;
      const auto* callee = n ? n->target : c ? c->callee : nullptr;
      const auto* name = as<ast::NameExpr>(callee);
      check(name && name->name && args && args->size() == 2 &&
            (*args)[0]->name.empty() && (*args)[1]->name == "name" &&
            as<ast::IntLit>((*args)[1]->value) && (*args)[0]->parent == e &&
            (*args)[0]->value->parent == (*args)[0],
            "calls and constructors retain argument nodes in their own scope");
    });
  }
  inspect("$x = strlen(...);", [&](const auto& b) {
    const auto* c = as<ast::CallExpr>(sole_value(b));
    check(c && c->args.size() == 1 && c->args[0]->spread && !c->args[0]->value,
          "first-class callable syntax retains its placeholder");
  });
  inspect("$x = \"hello {$user->name}\";", [&](const auto& b) {
    const auto* s = as<ast::InterpString>(sole_value(b));
    const auto* m = s && s->pieces.size() == 2 ? as<ast::MemberExpr>(s->pieces[1]) : nullptr;
    const auto* v = m ? as<ast::Var>(m->object) : nullptr;
    check(v && v->name == "$user" && v->parent == m && m->parent == s,
          "curly interpolation preserves the variable sigil and expression");
  });
  inspect(R"($x = "hello $user->name $user[1] $user?->name";)", [&](const auto& b) {
    const auto* s = as<ast::InterpString>(sole_value(b));
    const auto* m = s && s->pieces.size() == 6 ? as<ast::MemberExpr>(s->pieces[1]) : nullptr;
    const auto* i = s && s->pieces.size() == 6 ? as<ast::IndexExpr>(s->pieces[3]) : nullptr;
    const auto* n = s && s->pieces.size() == 6 ? as<ast::MemberExpr>(s->pieces[5]) : nullptr;
    const auto* v = m ? as<ast::Var>(m->object) : nullptr;
    const auto* key = i ? as<ast::IntLit>(i->index) : nullptr;
    check(v && v->name == "$user" && !m->nullsafe && m->parent == s &&
          key && key->text == "1" && as<ast::Var>(i->base) &&
          n && n->nullsafe && as<ast::Var>(n->object),
          "simple interpolation retains member, index and nullsafe accesses");
  });
  inspect(R"($x = "$user-> $user-> name $user?->";)", [&](const auto& b) {
    const auto* s = as<ast::InterpString>(sole_value(b));
    const auto* a = s && s->pieces.size() == 6 ? as<ast::StrText>(s->pieces[1]) : nullptr;
    const auto* c = s && s->pieces.size() == 6 ? as<ast::StrText>(s->pieces[3]) : nullptr;
    const auto* d = s && s->pieces.size() == 6 ? as<ast::StrText>(s->pieces[5]) : nullptr;
    check(a && c && d && as<ast::Var>(s->pieces[0]) &&
          as<ast::Var>(s->pieces[2]) && as<ast::Var>(s->pieces[4]) &&
          a->text == "-> " && c->text == "-> name " && d->text == "?->",
          "an interpolation arrow needs an adjacent property name");
  });
  inspect("$x=$a ? $b : $c; $y=$a ?: $b;", [&](const auto& b) {
    const auto* full = b.items.size() == 2 ? as<ast::ConditionalExpr>(value(b.items[0])) : nullptr;
    const auto* brief = b.items.size() == 2 ? as<ast::ConditionalExpr>(value(b.items[1])) : nullptr;
    const auto* cond = full ? as<ast::Var>(full->cond) : nullptr;
    const auto* yes = full ? as<ast::Var>(full->thenValue) : nullptr;
    const auto* no = full ? as<ast::Var>(full->elseValue) : nullptr;
    check(cond && cond->name == "$a" && cond->parent == full &&
          yes && yes->name == "$b" && no && no->name == "$c" &&
          brief && !brief->thenValue && as<ast::Var>(brief->cond) &&
          as<ast::Var>(brief->elseValue), "conditional operands retain their roles");
  });
  inspect("$x = [1, ...$a, ...[2]];", [&](const auto& b) {
    const auto* a = as<ast::ArrayExpr>(sole_value(b));
    check(a && a->items.size() == 3 && !a->items[0]->spread && a->items[1]->spread &&
          a->items[2]->spread && as<ast::ArrayExpr>(a->items[2]->value),
          "array unpacking retains spread operands");
  });
  inspect("$x = [1,2][0];", [&](const auto& b) {
    const auto* i = as<ast::IndexExpr>(sole_value(b));
    const auto* a = i ? as<ast::ArrayExpr>(i->base) : nullptr;
    check(a && a->items.size() == 2 && as<ast::IntLit>(i->index), "array literals support postfix indexing");
  });
  inspect("$x = 'a'.'b' |> strlen(...) == 2;", [&](const auto& b) {
    const auto* eq = as<ast::BinaryExpr>(sole_value(b));
    const auto* pipe = eq ? as<ast::BinaryExpr>(eq->left) : nullptr;
    const auto* concat = pipe ? as<ast::BinaryExpr>(pipe->left) : nullptr;
    check(eq && eq->op == "==" && pipe && pipe->op == "|>" && concat && concat->op == ".",
          "pipe precedence follows concatenation and precedes comparison");
  });
  inspect("class C { public string $name { get => $this->name; set(string $value) {$this->name=$value;} } }", [&](const auto& b) {
    const auto* c = b.items.size() == 1 ? as<ast::ClassDecl>(b.items[0]) : nullptr;
    const auto* p = c && c->members.size() == 1 ? as<ast::PropertyMember>(c->members[0]) : nullptr;
    const auto* d = p && p->props.size() == 1 ? p->props[0] : nullptr;
    check(d && d->hooks.size() == 2 && d->hooks[0]->name == "get" && d->hooks[0]->expression &&
          d->hooks[1]->name == "set" && d->hooks[1]->params.size() == 1 && d->hooks[1]->body &&
          d->hooks[0]->parent == d && d->hooks[1]->params[0]->parent == d->hooks[1],
          "property hooks retain bodies, parameters and ownership");
  });
  inspect("abstract readonly class C { public private(set) string $name; }", [&](const auto& b) {
    const auto* c = b.items.size() == 1 ? as<ast::ClassDecl>(b.items[0]) : nullptr;
    const auto* p = c && c->members.size() == 1 ? as<ast::PropertyMember>(c->members[0]) : nullptr;
    check(c && c->modifiers.size() == 2 && c->modifiers[0] == "abstract" && c->modifiers[1] == "readonly" &&
          p && p->modifiers.size() == 2 && p->modifiers[1] == "private(set)",
          "class and asymmetric property modifiers retain source order");
  });
  inspect("#[A] enum E { #[B] case C; }", [&](const auto& b) {
    const auto* e = b.items.size() == 1 ? as<ast::EnumDecl>(b.items[0]) : nullptr;
    check(e && e->attributes.size() == 1 && e->cases.size() == 1 && e->cases[0]->attributes.size() == 1,
          "attributes do not swallow enum declarations as comments");
  });
  inspect("use A\\{B, function f, const X};", [&](const auto& b) {
    const auto* u = b.items.size() == 1 ? as<ast::UseDecl>(b.items[0]) : nullptr;
    const auto* g = u && u->items.size() == 1 ? u->items[0] : nullptr;
    check(g && g->group.size() == 3 && g->group[0]->kind.empty() &&
          g->group[1]->kind == "function" && g->group[2]->kind == "const",
          "mixed group imports retain each entry kind");
  });
  inspect("$x = match($a) {}; $y = empty(1+2);", [&](const auto& b) {
    const auto* m = b.items.size() == 2 ? as<ast::MatchExpr>(value(b.items[0])) : nullptr;
    const auto* e = b.items.size() == 2 ? as<ast::EmptyExpr>(value(b.items[1])) : nullptr;
    check(m && m->arms.empty() && e && as<ast::BinaryExpr>(e->operand),
          "empty matches and arbitrary empty() operands retain their AST");
  });
  inspect("$x = namespace\\Foo(); $y = \\Foo();", [&](const auto& b) {
    for (unsigned i = 0; i < 2; ++i) {
      const auto* c = b.items.size() == 2 ? as<ast::CallExpr>(value(b.items[i])) : nullptr;
      const auto* n = c ? as<ast::NameExpr>(c->callee) : nullptr;
      check(n && n->name && n->name->nsRelative == (i == 0) && n->name->rooted == (i == 1) &&
            n->name->parts.size() == 1 && n->name->parts[0] == "Foo",
            "relative namespace names are distinct from rooted names");
    }
  });
  inspect("$x = b\"hello\"; $y = B'world';", [&](const auto& b) {
    const auto* d = b.items.size() == 2 ? as<ast::InterpString>(value(b.items[0])) : nullptr;
    const auto* s = b.items.size() == 2 ? as<ast::StringLit>(value(b.items[1])) : nullptr;
    const auto* t = d && d->pieces.size() == 1 ? as<ast::StrText>(d->pieces[0]) : nullptr;
    check(t && t->text == "hello" && s && s->text == "B'world'", "binary string prefixes preserve contents");
  });
  {
    const std::string source = "<?= 1 ?>\n";
    const auto r = parser.parse(source);
    const auto* d = as<ast::Document>(r.value);
    const auto* b = d && d->items.size() == 1 ? as<ast::PhpBlock>(d->items[0]) : nullptr;
    const auto* e = b && b->items.size() == 1 ? as<ast::EchoStmt>(b->items[0]) : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && e && e->values.size() == 1 &&
          as<ast::IntLit>(e->values[0]), "short echo tags build echo statements");
  }
  inspect("$f = #[A,] static fn(int $x): int => $x;", [&](const auto& b) {
    const auto* f = as<ast::Closure>(sole_value(b));
    check(f && f->attributes.size() == 1 && f->isStatic, "closure attributes retain trailing-comma groups");
  });
  inspect("class C {function __construct(public string $name {set => $value;}){}}", [&](const auto& b) {
    const auto* c = b.items.size() == 1 ? as<ast::ClassDecl>(b.items[0]) : nullptr;
    const auto* m = c && c->members.size() == 1 ? as<ast::MethodMember>(c->members[0]) : nullptr;
    const auto* f = m ? m->function : nullptr;
    check(f && f->params.size() == 1 && f->params[0]->hooks.size() == 1 &&
          f->params[0]->hooks[0]->name == "set" && f->params[0]->hooks[0]->expression,
          "promoted parameters retain property hooks");
  });
  inspect("print 'x' and false; function f(){yield 1 and 2;} include 'x' or true;", [&](const auto& b) {
    const auto* printed = b.items.size() == 3 ? as<ast::BinaryExpr>(value(b.items[0])) : nullptr;
    const auto* fn = b.items.size() == 3 ? as<ast::FunctionDecl>(b.items[1]) : nullptr;
    const auto* body = fn ? as<ast::BlockStmt>(fn->body) : nullptr;
    const auto* yielded = body && body->items.size() == 1 ? as<ast::BinaryExpr>(value(body->items[0])) : nullptr;
    const auto* y = yielded ? as<ast::YieldExpr>(yielded->left) : nullptr;
    const auto* include = b.items.size() == 3 ? as<ast::IncludeExpr>(value(b.items[2])) : nullptr;
    const auto* logical = include ? as<ast::BinaryExpr>(include->operand) : nullptr;
    check(printed && printed->op == "and" && as<ast::PrintExpr>(printed->left) &&
          yielded && yielded->op == "and" && y && as<ast::IntLit>(y->value) &&
          logical && logical->op == "or", "print, yield and include follow Zend precedence");
  });
  inspect("$x = $value instanceof $class; exit(status: 0);", [&](const auto& b) {
    const auto* i = b.items.size() == 2 ? as<ast::InstanceofExpr>(value(b.items[0])) : nullptr;
    const auto* t = i ? as<ast::Var>(i->type) : nullptr;
    const auto* e = b.items.size() == 2 ? as<ast::ExitExpr>(value(b.items[1])) : nullptr;
    check(t && t->name == "$class" && e && e->args.size() == 1 && e->args[0]->name == "status" &&
          as<ast::IntLit>(e->args[0]->value), "dynamic instanceof and named exit arguments retain operands");
  });
  inspect("function f(?string $x): ?string {} function g(resource $x){}", [&](const auto& b) {
    const auto* f = b.items.size() == 2 ? as<ast::FunctionDecl>(b.items[0]) : nullptr;
    const auto* g = b.items.size() == 2 ? as<ast::FunctionDecl>(b.items[1]) : nullptr;
    const auto* arg = f && f->params.size() == 1 ? as<ast::NullableTypeRef>(f->params[0]->type) : nullptr;
    const auto* ret = f ? as<ast::NullableTypeRef>(f->returnType) : nullptr;
    const auto* resource = g && g->params.size() == 1 ? as<ast::NamedTypeRef>(g->params[0]->type) : nullptr;
    check(arg && ret && as<ast::PrimitiveTypeRef>(arg->type) && resource && resource->name &&
          resource->name->parts.size() == 1 && resource->name->parts[0] == "resource",
          "nullable hints share one type model and resource remains a class name");
  });
  for (const char* text : {"   ", "//text", "/*text*/", "{plain}"}) {
    const std::string code = std::string("$x=\"") + text + "\";";
    inspect(code.c_str(), [&](const auto& b) {
      const auto* string = as<ast::InterpString>(sole_value(b));
      std::string joined;
      if (string) for (const auto* p : string->pieces) {
        const auto* t = as<ast::StrText>(p);
        check(t, "literal string contents do not become expressions");
        if (t) joined += t->text;
      }
      check(string && joined == text, "string contents retain whitespace, comments and plain braces");
    });
  }
  inspect("$x=\"{$user /*comment*/ -> name} ${foo} ${foo[1]}\";", [&](const auto& b) {
    const auto* s = as<ast::InterpString>(sole_value(b));
    const auto* member = s && s->pieces.size() == 5 ? as<ast::MemberExpr>(s->pieces[0]) : nullptr;
    const auto* named = s && s->pieces.size() == 5 ? as<ast::Var>(s->pieces[2]) : nullptr;
    const auto* index = s && s->pieces.size() == 5 ? as<ast::IndexExpr>(s->pieces[4]) : nullptr;
    const auto* base = index ? as<ast::Var>(index->base) : nullptr;
    check(member && as<ast::Var>(member->object) && named && named->name == "foo" &&
          base && base->name == "foo" && as<ast::IntLit>(index->index),
          "interpolations retain PHP trivia and legacy variable names");
  });
  {
    const std::string source = " \n// html<?php echo 1;?> \t ";
    const auto r = parser.parse(source);
    const auto* d = as<ast::Document>(r.value);
    const auto* left = d && d->items.size() == 3 ? as<ast::InlineHtmlStmt>(d->items[0]) : nullptr;
    const auto* right = d && d->items.size() == 3 ? as<ast::InlineHtmlStmt>(d->items[2]) : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && left && right &&
          left->text == " \n// html" && right->text == " \t ",
          "markup retains whitespace and PHP-like comments outside islands");
  }
  for (const char* text : {"<?php $x=1; \t\n", "<?php $x=1; /*comment*/ \n", "<?php $x=1; //comment\n \t"}) {
    const std::string source = text;
    const auto r = parser.parse(source);
    const auto* d = as<ast::Document>(r.value);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && d && d->items.size() == 1 &&
          as<ast::PhpBlock>(d->items[0]), "EOF owns trailing PHP whitespace and comments");
  }
  {
    const std::string source = "  <?= 1 ?>\n";
    const auto r = parser.parse(source);
    const auto* d = as<ast::Document>(r.value);
    const auto* left = d && d->items.size() == 2 ? as<ast::InlineHtmlStmt>(d->items[0]) : nullptr;
    const auto* block = d && d->items.size() == 2 ? as<ast::PhpBlock>(d->items[1]) : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && left && left->text == "  " &&
          block && block->items.size() == 1 && as<ast::EchoStmt>(block->items[0]),
          "short echoes preserve surrounding markup and consume their closing newline");
  }
  {
    const std::string source = R"(<script language="php"> $x = 1; </script>
<% $x = 2; %><%= 3 %>)";
    const auto r = parser.parse(source);
    const auto* d = as<ast::Document>(r.value);
    const auto* h = d && d->items.size() == 1 ? as<ast::InlineHtmlStmt>(d->items[0]) : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && h && h->text == source,
          "obsolete ASP and script tags remain literal HTML");
  }
  for (const char* text : {"<?PHP echo 1;", "<?pHp echo 1;"}) {
    const std::string source = text;
    const auto r = parser.parse(source);
    const auto* d = as<ast::Document>(r.value);
    const auto* b = d && d->items.size() == 1 ? as<ast::PhpBlock>(d->items[0]) : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && b &&
          b->items.size() == 1 && as<ast::EchoStmt>(b->items[0]),
          "PHP opening tags ignore letter case");
  }
  inspect("$$x; $$$x; ${$x};", [&](const auto& b) {
    const auto* two = b.items.size() == 3 ? as<ast::Var>(value(b.items[0])) : nullptr;
    const auto* three = b.items.size() == 3 ? as<ast::Var>(value(b.items[1])) : nullptr;
    const auto* dynamic = b.items.size() == 3 ? as<ast::Var>(value(b.items[2])) : nullptr;
    const auto* innerTwo = two ? as<ast::Var>(two->nameExpr) : nullptr;
    const auto* innerThree = three ? as<ast::Var>(three->nameExpr) : nullptr;
    check(innerTwo && innerTwo->name == "$x" && innerThree &&
          as<ast::Var>(innerThree->nameExpr) &&
          as<ast::Var>(innerThree->nameExpr)->name == "$x" &&
          dynamic && as<ast::Var>(dynamic->nameExpr),
          "each variable indirection retains its own AST node");
  });
  inspect("if ($ok) { ?>hello <?= $value ?><?php }", [&](const auto& b) {
    const auto* conditional = b.items.size() == 1 ? as<ast::IfStmt>(b.items[0]) : nullptr;
    const auto* body = conditional ? as<ast::BlockStmt>(conditional->thenBranch) : nullptr;
    const auto* bridge = body && !body->items.empty() ? as<ast::BlockStmt>(body->items[0]) : nullptr;
    const auto* html = bridge && bridge->items.size() == 2 ? as<ast::InlineHtmlStmt>(bridge->items[0]) : nullptr;
    const auto* echo = bridge && bridge->items.size() == 2 ? as<ast::EchoStmt>(bridge->items[1]) : nullptr;
    check(html && html->text == "hello " && echo && echo->values.size() == 1 &&
          as<ast::Var>(echo->values[0]), "short echoes inside control-flow markup retain echo semantics");
  });
  inspect("$x=\"${foo} ${foo } ${ foo}\";", [&](const auto& b) {
    const auto* str = as<ast::InterpString>(sole_value(b));
    const auto* bare = str && str->pieces.size() == 5 ? as<ast::Var>(str->pieces[0]) : nullptr;
    const auto* trailing = str && str->pieces.size() == 5 ? as<ast::Var>(str->pieces[2]) : nullptr;
    const auto* leading = str && str->pieces.size() == 5 ? as<ast::Var>(str->pieces[4]) : nullptr;
    check(bare && bare->name == "foo" && !bare->nameExpr &&
          trailing && as<ast::NameExpr>(trailing->nameExpr) &&
          leading && as<ast::NameExpr>(leading->nameExpr),
          "legacy variable names require immediate native lexer delimiters");
  });
  return ok;
}

}
