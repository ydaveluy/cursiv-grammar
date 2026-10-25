#pragma once
#include <cstdio>
#include <functional>
#include <vector>

#include "capnproto/core/CoreServices.hpp"
#include "capnproto/generated/Capnproto/Module.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
namespace code_audit {
namespace ast = capnproto::ast;
inline bool run() {
  const auto parser = capnproto::generated::Capnproto::create_parser();
  const auto reflection =
      capnproto::generated::Capnproto::create_ast_reflection();
  bool ok = true;
  const auto check = [&](bool b, const char* message) {
    if (!b) {
      std::fprintf(stderr, "capnproto code audit: %s\n", message);
      ok = false;
    }
  };
  const auto clean = [&](const cursiv::ParseResult& r) {
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
  };
  const auto visit = [&](const cursiv::AstNode* root, auto&& callback) {
    std::function<void(const cursiv::AstNode*)> walk =
        [&](const cursiv::AstNode* n) {
          if (!n) return;
          callback(n);
          reflection->visitMembers(
              *n, [&](std::string_view, std::uint32_t, cursiv::AstNode& c) {
                check(c.parent == n, "parent ownership");
                walk(&c);
              });
        };
    walk(root);
  };
  const char* const cases[] = {
      R"capnp(@0xb923423423423423;
using Base = import "common.capnp".Base;
struct Outer { struct Inner { value @0 :Text; } }
using Alias = .Outer.Inner;
using Outer.Inner;
struct Holder { a @0 : .Outer.Inner; b @1 :import "common.capnp".Base; c @2 :Base; }
)capnp",
      R"capnp(@0xb923423423423423;
annotation flag @0xf923423423423426 (*) :Void;
annotation meta (file, struct, field, enumerant, enum, const, interface, method, param, annotation, group, union) :Text;
$flag;
$meta("file");
struct S $flag $meta("s") { x @0 :Text = "value" $flag $meta("x"); }
enum E $flag { one @0 $flag $meta("one"); }
const value :UInt32 = 7 $flag $meta("value");
annotation extra (file) :Void $flag;
)capnp",
      R"capnp(﻿@0xb923423423423423;
struct Foo { bar @0 :UInt32; }
)capnp",
      R"capnp(@0xb923423423423431;
struct Base { value @0 :Text; }
)capnp",
      R"capnp(@0xb923423423423423;
const bytes :Data = embed "payload.bin";
const text :Text = embed "payload.txt";
)capnp",
      R"capnp(@0xb923423423423423;
annotation flag (*) :Void;
struct S {
  part :group $flag {
    nested :group { value @0 :Text; }
    union { one @1 :Void; two @2 :Text; }
  }
  choice :union $flag { one @3 :Void; two @4 :UInt32; }
}
)capnp",
      R"capnp(@0xb923423423423423;
annotation flag (*) :Void;
struct Box(T) { value @0 :T; }
interface Base {}
interface Other {}
interface I @0xf923423423423427 (T,) extends(Base, Other,) $flag {
  call @0 [U,] (x :Box(U) $flag,) -> (y :Text,) $flag;
  streamCall @1 () -> stream;
  using Alias = Box(Text);
  const value :UInt32 = 0x42;
  annotation extra (method) :Void;
}
)capnp",
      R"capnp(@0xb923423423423423;
const lines :Text = `first
 `second
 ;
)capnp",
      R"capnp(@0xb923423423423423;
const text :Text = "a\"b\\c\n\t\x41\141" "tail";
const bytes :Data = 0x"00 41 ff";
const octal :UInt64 = 0123;
const max :UInt64 = 18446744073709551615;
const min :Int64 = - 9223372036854775808;
const hex :UInt64 = 0xffffffffffffffff;
const float :Float64 = 1.E+2;
const exponent :Float64 = 1E+2;
const neg :Float64 = - 1.25e-2;
const infinity :Float64 = -inf;
const notANumber :Float64 = nan;
)capnp",
      R"capnp(@0xb923423423423423;
const value @0xf923423423423428 :UInt32 = 7;
enum Choice @0xf923423423423429 { first @0; second @1; }
struct Nested @0xf923423423423430 { using C = Choice; enum E { x @0; } }
)capnp",
      R"capnp(@0xb923423423423423;
struct Box @0xf923423423423424 (T,) { value @0 :T; }
struct Holder @0xf923423423423425 { value @0 :Box(Text); }
)capnp",
      R"capnp(@0xb923423423423423;
struct Data { name @0 :Text; values @1 :List(Int32); }
const value :Data = (name = "a\"b", values = [1, -2, 3,],);
const list :List(Data) = [(name = "x"), (name = "y", values = [],),];
const nested :List(List(UInt32)) = [[1,2,],[],];
)capnp",
  };
  for (const auto* source : cases) {
    const auto r = parser->parse(source);
    clean(r);
    check(r.value_as<ast::Document>() != nullptr, "document root");
    visit(r.value, [](const auto*) {});
  }
  {
    const auto r = parser->parse(
        "@0xb923423423423423;\n# ignored\rstruct Ignored {}\nstruct Visible "
        "{}\n");
    clean(r);
    const auto* d = r.value_as<ast::Document>();
    const auto* visible = d && d->content.size() == 2
                              ? cursiv::ast_cast<ast::StructDef>(d->content[1])
                              : nullptr;
    check(visible && visible->name == "Visible",
          "carriage return stays inside a line comment");
  }
  {
    const auto r = parser->parse(R"capnp(@0xb923423423423423;
const text :Text = "a\"b\\c\n\t\x41\141" "tail";
const bytes :Data = 0x"00 41 ff";
const max :UInt64 = 18446744073709551615;
const min :Int64 = - 9223372036854775808;
const float :Float64 = 1.E+2;
)capnp");
    clean(r);
    const auto* d = r.value_as<ast::Document>();
    check(d && d->content.size() == 6, "literal declarations");
    if (d && d->content.size() == 6) {
      const auto* t = cursiv::ast_cast<ast::ConstDef>(d->content[1]);
      const auto* text =
          t ? cursiv::ast_cast<ast::TextLiteral>(t->value) : nullptr;
      const auto* b = cursiv::ast_cast<ast::ConstDef>(d->content[2]);
      const auto* bytes =
          b ? cursiv::ast_cast<ast::BytesLiteral>(b->value) : nullptr;
      const auto* m = cursiv::ast_cast<ast::ConstDef>(d->content[3]);
      const auto* max =
          m ? cursiv::ast_cast<ast::IntegerLiteral>(m->value) : nullptr;
      const auto* n = cursiv::ast_cast<ast::ConstDef>(d->content[4]);
      const auto* neg =
          n ? cursiv::ast_cast<ast::NegativeExpr>(n->value) : nullptr;
      const auto* min =
          neg ? cursiv::ast_cast<ast::IntegerLiteral>(neg->operand) : nullptr;
      const auto* f = cursiv::ast_cast<ast::ConstDef>(d->content[5]);
      const auto* number =
          f ? cursiv::ast_cast<ast::FloatLiteral>(f->value) : nullptr;
      check(text && text->value == "a\"b\\c\n\tAatail",
            "decoded and concatenated text");
      const char expected[] = {0, 65, static_cast<char>(255)};
      check(bytes && bytes->value == std::string_view(expected, 3),
            "decoded binary distinct from text");
      check(max && max->text == "18446744073709551615" && min &&
                min->text == "9223372036854775808" && min->parent == neg,
            "exact unsigned magnitudes and spaced minus");
      check(number && number->text == "1.E+2", "exact floating literal");
    }
  }
  {
    const auto r = parser->parse(
        "@0xb923423423423423; annotation flag (*) :Void; struct Box "
        "@0xf923423423423424 (T,) { value @0 :T; } interface I(T) "
        "extends(Base, Other,) $flag { call @0 [U,] (x :Box(U) $flag,) -> (y "
        ":Text,) $flag; push @1 () -> stream; }");
    clean(r);
    const ast::StructDef* box = nullptr;
    const ast::InterfaceDef* iface = nullptr;
    std::vector<const ast::FunctionDef*> methods;
    visit(r.value, [&](const auto* n) {
      if (const auto* x = cursiv::ast_cast<ast::StructDef>(n)) box = x;
      if (const auto* x = cursiv::ast_cast<ast::InterfaceDef>(n)) iface = x;
      if (const auto* x = cursiv::ast_cast<ast::FunctionDef>(n))
        methods.push_back(x);
    });
    check(box && box->id && box->id->text == "0xf923423423423424" &&
              box->typeParams.size() == 1 && box->typeParams[0] == "T",
          "declaration UID before generic parameters");
    check(iface && iface->typeParams.size() == 1 &&
              iface->extends.size() == 2 && iface->annotations.size() == 1,
          "multiple superclasses and interface annotation");
    check(methods.size() == 2, "two method nodes");
    if (methods.size() == 2) {
      const auto* m = methods[0];
      check(m->typeParams.size() == 1 && m->typeParams[0] == "U" && m->params &&
                m->params->params.size() == 1 &&
                m->params->params[0]->name == "x" &&
                m->params->params[0]->annotations.size() == 1 && m->results &&
                m->results->params.size() == 1 && m->annotations.size() == 1,
            "method parameters, result, generics and annotations");
      check(methods[1]->results && methods[1]->results->stream,
            "stream result");
    }
  }
  {
    const auto r = parser->parse(
        "@0xb923423423423423; using Alias = import "
        "\"common.capnp\".Outer(Text).Inner; annotation ann "
        "(file,struct,field) :List(UInt32); $ann([1,2,]); struct S {} using S2 "
        "= .S;");
    clean(r);
    const ast::UsingAlias* alias = nullptr;
    const ast::AnnotationStmt* statement = nullptr;
    visit(r.value, [&](const auto* n) {
      if (!alias) alias = cursiv::ast_cast<ast::UsingAlias>(n);
      if (const auto* x = cursiv::ast_cast<ast::AnnotationStmt>(n))
        statement = x;
    });
    const auto* member =
        alias ? cursiv::ast_cast<ast::MemberExpr>(alias->target) : nullptr;
    const auto* app =
        member ? cursiv::ast_cast<ast::ApplicationExpr>(member->parentExpr)
               : nullptr;
    const auto* outer =
        app ? cursiv::ast_cast<ast::MemberExpr>(app->function) : nullptr;
    const auto* import =
        outer ? cursiv::ast_cast<ast::ImportExpr>(outer->parentExpr) : nullptr;
    check(member && member->name == "Inner" && app && app->params.size() == 1 &&
              outer && outer->name == "Outer" && import &&
              import->path == "common.capnp",
          "import, application and member precedence");
    const auto* ann = statement && statement->annotation
                          ? cursiv::ast_cast<ast::ApplicationExpr>(
                                statement->annotation->expression)
                          : nullptr;
    const auto* list =
        ann && ann->params.size() == 1
            ? cursiv::ast_cast<ast::ConstList>(ann->params[0]->value)
            : nullptr;
    check(list && list->items.size() == 2, "generic annotation value");
  }
  {
    auto language =
        cursiv::create_language<capnproto::CapnprotoCoreServices,
                                capnproto::CapnprotoSharedCoreServices,
                                capnproto::install_core_module>();
    const std::string source =
        "@1; struct bad_name { x :Text; y @70000 :Text; } annotation "
        "mark(field,field,unknown,*) :Void; const n :UInt64 = "
        "18446744073709551616; const neg :Int32 = -foo; using Missing; using "
        "lower = Text; const empty :Data = 0x\"\"; struct Outer { struct Inner "
        "{} } using Outer.Inner; struct Inner {}";
    cursiv::Document doc(cursiv::TextDocument::create("memory:capnproto",
                                                      "capnproto", 0, source));
    doc.parseResult = parser->parse(source);
    clean(doc.parseResult);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    visit(doc.parseResult.value, [&](const auto* n) {
      if (const auto* x = cursiv::ast_cast<ast::Document>(n))
        language.language.validator->checkDocument(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::Declaration>(n))
        language.language.validator->checkDeclaration(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::Id>(n))
        language.language.validator->checkId(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::IntegerLiteral>(n))
        language.language.validator->checkInteger(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::BytesLiteral>(n))
        language.language.validator->checkBytes(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::NegativeExpr>(n))
        language.language.validator->checkNegative(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::AnnotationDef>(n))
        language.language.validator->checkTargets(*x, accept);
    });
    check(errors == 14, "precise semantic errors after permissive parse");
  }
  return ok;
}
}  // namespace code_audit
