#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "zig/core/CoreServices.hpp"
#include "zig/core/ast.hpp"
namespace zig_audit {
template<class T> const T* as(const cursiv::AstNode* n) {
  return n && n->type == &cursiv::ast_type_tag<T> ? static_cast<const T*>(n) : nullptr;
}
inline bool modern_cases(zig::ZigCoreServices& services) {
  namespace ast = zig::ast;
  const auto& parser = *services.parser;
  bool ok = true;
  unsigned checks = 0;
  auto check = [&](bool value, const char* message) {
    ++checks;
    if (!value) { std::fprintf(stderr, "zig audit: %s\n", message); ok = false; }
  };
  constexpr std::string_view valid[] = {
    R"ZIG(/// first
/// second
pub const x = 1;
)ZIG",
    R"ZIG(//! first
//! second
const x = 1;
)ZIG",
    R"ZIG(const Color = enum { red, blue = 3, };
)ZIG",
    R"ZIG(const Value = union(enum) { empty, number: i32, };
)ZIG",
    R"ZIG(const Pair = struct { u32, []const u8, };
)ZIG",
    R"ZIG(const x = 0xff.p10;
)ZIG",
    R"ZIG(const x = 1.25e+2;
)ZIG",
    R"ZIG(const x = 0x1.fp-2;
)ZIG",
    R"ZIG(const x = foo() catch |err| fallback(err);
)ZIG",
    R"ZIG(const T = *align(4:0:32) addrspace(.generic) const volatile allowzero u32;
)ZIG",
    R"ZIG(const T = **u32;
)ZIG",
    R"ZIG(const A = [:0]const u8; const B = [*:0]const u8; const C = [4:0]u8;
)ZIG",
    R"ZIG(const T = *if (a) B else C;
)ZIG",
    R"ZIG(const s = \\one
    \\two
;
)ZIG",
    R"ZIG(const @"a\"b" = 1;
)ZIG",
    R"ZIG(const x = '😀';
)ZIG",
    R"ZIG(fn f() void { asm volatile ("" : [out] "=r" (-> usize) : [in] "r" (x) : .{ .memory = true }); }
)ZIG",
    R"ZIG(fn f() void { for (xs, 0..) |*x, i| { x.* = i; } }
)ZIG",
    R"ZIG(fn f() void { state: switch (x) { 1 => continue :state 2, else => {}, } }
)ZIG",
    R"ZIG(extern "c" fn f(comptime T: type, noalias p: *T, ...) callconv(.c) void;
)ZIG",
    R"ZIG(const x = @as(fn () void, f)();
)ZIG",
    R"ZIG(const T = error{ A, B }![]const u8;
)ZIG",
    R"ZIG(const x = [_]u8{ 1, 2, 3 };
)ZIG",
    R"ZIG(fn f() void { const a, var b: u32 = .{ 1, 2 }; a, b = .{ 3, 4 }; }
)ZIG",
    R"ZIG(const x = switch (v) { inline 0...3, 5 => |a| a, else => 0, };
)ZIG",
    R"ZIG(const a = true; const b = false; const c = null; const d = undefined;
)ZIG",
    R"ZIG(const s = \\one
// gap
    \\two
;
)ZIG",
    R"ZIG(const x = 1e+2.foo;
)ZIG",
  };
  for (const auto source : valid) {
    auto result = parser.parse(source);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "native-confirmed syntax");
  }
  auto value = [](const cursiv::ParseResult& r) -> const ast::Expr* {
    auto root = as<ast::ContainerMembers>(r.value);
    auto d = root && !root->members.empty() ? as<ast::Decl>(root->members[0]) : nullptr;
    return d && d->varDecl ? d->varDecl->value : nullptr;
  };
  {
    auto r = parser.parse("//! one\n//! two\n/// first\n/// second\npub const x = 1;");
    auto root = as<ast::ContainerMembers>(r.value);
    auto d = root && root->members.size() == 1 ? as<ast::Decl>(root->members[0]) : nullptr;
    check(root && root->docComment && *root->docComment == "//! one\n//! two", "container doc lines");
    check(d && d->docComment && *d->docComment == "/// first\n/// second" && d->pub, "declaration doc lines");
    check(d && d->parent == root && d->varDecl && d->varDecl->parent == d, "declaration parents");
  }
  {
    auto r = parser.parse("const T = enum { red, blue = 3, };");
    auto c = as<ast::ContainerDecl>(value(r));
    auto a = c && c->body && c->body->members.size() == 2 ? as<ast::ContainerField>(c->body->members[0]) : nullptr;
    auto b = a ? as<ast::ContainerField>(c->body->members[1]) : nullptr;
    check(c && c->kind == "enum" && c->body->parent == c, "enum container");
    check(a && a->name && *a->name == "red" && !a->type && a->parent == c->body, "bare enum name and parent");
    check(b && b->name && *b->name == "blue" && !b->type && as<ast::NumberLiteral>(b->value) &&
          as<ast::NumberLiteral>(b->value)->text == "3" && b->value->parent == b, "enum value");
  }
  {
    auto r = parser.parse("const T = union(enum) { empty, number: i32, };");
    auto c = as<ast::ContainerDecl>(value(r));
    auto a = c && c->body && c->body->members.size() == 2 ? as<ast::ContainerField>(c->body->members[0]) : nullptr;
    auto b = a ? as<ast::ContainerField>(c->body->members[1]) : nullptr;
    check(c && c->unionEnumTag && c->kind == "union", "tagged union");
    check(a && a->name && *a->name == "empty" && !a->type, "bare union name");
    check(b && b->name && *b->name == "number" && as<ast::Identifier>(b->type) &&
          as<ast::Identifier>(b->type)->name == "i32" && b->type->parent == b, "typed union field");
  }
  {
    auto r = parser.parse("const T = struct { u32, []const u8, };");
    auto c = as<ast::ContainerDecl>(value(r));
    auto a = c && c->body && c->body->members.size() == 2 ? as<ast::ContainerField>(c->body->members[0]) : nullptr;
    check(a && !a->name && as<ast::Identifier>(a->type) && as<ast::Identifier>(a->type)->name == "u32", "tuple field stays a type");
  }
  {
    auto r = parser.parse("const T = **u32;");
    auto outer = as<ast::PtrType>(value(r));
    auto inner = outer ? as<ast::PtrType>(outer->base) : nullptr;
    check(inner && inner->parent == outer && as<ast::Identifier>(inner->base) &&
          as<ast::Identifier>(inner->base)->name == "u32" && inner->base->parent == inner, "two pointer levels");
  }
  {
    auto r = parser.parse("const x = 0xff.p10;");
    auto n = as<ast::NumberLiteral>(value(r));
    check(n && n->text == "0xff.p10", "hex float is one number");
  }
  {
    auto r = parser.parse("const x = 1e+2.foo;");
    auto m = as<ast::MemberExpr>(value(r));
    auto n = m ? as<ast::NumberLiteral>(m->object) : nullptr;
    check(m && m->field == "foo" && n && n->text == "1e+2" && n->parent == m, "number token boundary");
  }
  {
    auto r = parser.parse("const s = \\\\one\n// gap\n  \\\\two\n;");
    auto s = as<ast::StringLiteral>(value(r));
    check(s && s->text == "\\\\one\n// gap\n  \\\\two", "multiline string source");
  }
  {
    auto r = parser.parse("const x = foo() catch |err| fallback(err);");
    auto b = as<ast::BinaryExpr>(value(r));
    auto call = b ? as<ast::CallExpr>(b->right) : nullptr;
    auto arg = call && call->args.size() == 1 ? as<ast::Identifier>(call->args[0]) : nullptr;
    check(b && b->op == "catch |err|" && as<ast::CallExpr>(b->left), "catch operator source");
    check(arg && arg->name == "err" && arg->parent == call && call->parent == b, "catch expression parents");
  }
  auto validate = [&](std::string source, std::string_view code, unsigned expected) {
    cursiv::Document document(cursiv::TextDocument::create("file:///audit.zig", "zig", 0, std::move(source)));
    document.language = &services;
    document.parseResult = parser.parse(document.textDocument().getText());
    check(document.parseSucceeded() && !document.parseRecovered(), "semantic error parses cleanly");
    if (document.parseResult.value) document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    unsigned count = 0;
    for (const auto& diagnostic : diagnostics)
      if (diagnostic.code && diagnostic.code->isString() && diagnostic.code->string() == code) ++count;
    check(count == expected, "runtime diagnostic code and count");
  };
  validate("const T = struct { a: u8, const A = 1; b: u8, };", "zig.field-order", 1);
  validate("const T = enum { x, x };", "zig.duplicate-field", 2);
  for (auto number : {"0b102", "1__2", "0x_1", "1e+", "01", "0X1"})
    validate("const x = " + std::string(number) + ";", "zig.number", 1);
  for (auto number : {"0", "0xff.p10", "0x1.fp-2", "1.25e+2", "0b10_10"})
    validate("const x = " + std::string(number) + ";", "zig.number", 0);
  std::printf("zig audit: %zu native forms, %u checks\n", std::size(valid), checks);
  return ok;
}
}  // namespace zig_audit
