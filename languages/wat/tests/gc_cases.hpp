#pragma once
#include <cstdio>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "wat/core/ast.hpp"
namespace wat_gc_audit {
struct Case {
  std::string_view name, source;
  bool validated;
};
inline constexpr Case cases[] = {
    {"gc-op-0",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable struct.new $s drop)))GC",
     true},
    {"gc-op-1",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable struct.new_default $s drop)))GC",
     true},
    {"gc-op-2",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable struct.get $s $x drop)))GC",
     true},
    {"gc-op-3",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable struct.get_s $p 0 drop)))GC",
     true},
    {"gc-op-4",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable struct.get_u $p 0 drop)))GC",
     true},
    {"gc-op-5",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable struct.set $s $x drop)))GC",
     true},
    {"gc-op-6",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.new $a drop)))GC",
     true},
    {"gc-op-7",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.new_default $a drop)))GC",
     true},
    {"gc-op-8",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.new_fixed $a 3 drop)))GC",
     true},
    {"gc-op-9",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.new_data $pa $d drop)))GC",
     true},
    {"gc-op-10",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.new_elem $fa $e drop)))GC",
     true},
    {"gc-op-11",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.get $a drop)))GC",
     true},
    {"gc-op-12",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.get_s $pa drop)))GC",
     true},
    {"gc-op-13",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.get_u $pa drop)))GC",
     true},
    {"gc-op-14",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.set $a drop)))GC",
     true},
    {"gc-op-15",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.len drop)))GC",
     true},
    {"gc-op-16",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.fill $a drop)))GC",
     true},
    {"gc-op-17",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.copy $a $a drop)))GC",
     true},
    {"gc-op-18",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.init_data $pa $d drop)))GC",
     true},
    {"gc-op-19",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable array.init_elem $fa $e drop)))GC",
     true},
    {"gc-op-20",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable ref.i31 drop)))GC",
     true},
    {"gc-op-21",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable i31.get_s drop)))GC",
     true},
    {"gc-op-22",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable i31.get_u drop)))GC",
     true},
    {"gc-op-23",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable ref.eq drop)))GC",
     true},
    {"gc-op-24",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable ref.test (ref $s) drop)))GC",
     true},
    {"gc-op-25",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable ref.test (ref null $s) drop)))GC",
     true},
    {"gc-op-26",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable ref.cast (ref $s) drop)))GC",
     true},
    {"gc-op-27",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable ref.cast (ref null $s) drop)))GC",
     true},
    {"gc-op-28",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable any.convert_extern drop)))GC",
     true},
    {"gc-op-29",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable extern.convert_any drop)))GC",
     true},
    {"gc-op-30",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable br_on_null 0 drop)))GC",
     true},
    {"gc-op-31",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func unreachable br_on_non_null 0 drop)))GC",
     false},
    {"gc-folded-0",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func (ref.cast (ref null $s) (ref.null any)) drop)))GC",
     true},
    {"gc-folded-1",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func (ref.test (ref $s) (ref.null any)) drop)))GC",
     true},
    {"gc-folded-2",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func (struct.new $s (i32.const 7)) drop)))GC", true},
    {"gc-folded-3",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func (struct.get $s $x (struct.new $s (i32.const 9))) drop)))GC",
     true},
    {"gc-folded-4",
     R"GC((module (type $s (struct (field $x (mut i32)))) (type $p (struct (field (mut i8)))) (type $a (array (mut i32))) (type $pa (array (mut i16))) (type $fa (array (mut funcref))) (data $d "abcd") (elem $e func) (func (array.new_fixed $a 2 (i32.const 1) (i32.const 2)) drop)))GC",
     true},
    {"gc-branch-0",
     R"GC((module (type $s (struct)) (func block $b (result (ref null any)) unreachable br_on_cast $b (ref null any) (ref $s) drop end drop)))GC",
     true},
    {"gc-branch-1",
     R"GC((module (type $s (struct)) (func block $b (result (ref null any)) unreachable br_on_cast_fail $b (ref null any) (ref $s) drop end drop)))GC",
     true},
    {"alias-anyref", R"GC((module (func (param anyref))))GC", true},
    {"alias-eqref", R"GC((module (func (param eqref))))GC", true},
    {"alias-i31ref", R"GC((module (func (param i31ref))))GC", true},
    {"alias-structref", R"GC((module (func (param structref))))GC", true},
    {"alias-arrayref", R"GC((module (func (param arrayref))))GC", true},
    {"alias-nullref", R"GC((module (func (param nullref))))GC", true},
    {"alias-nullfuncref", R"GC((module (func (param nullfuncref))))GC", true},
    {"alias-nullexternref", R"GC((module (func (param nullexternref))))GC",
     true},
    {"alias-funcref", R"GC((module (func (param funcref))))GC", true},
    {"alias-externref", R"GC((module (func (param externref))))GC", true},
    {"alias-exnref", R"GC((module (func (param exnref))))GC", true},
    {"alias-nullexnref", R"GC((module (func (param nullexnref))))GC", true},
    {"heap-any", R"GC((module (func (result (ref null any)) ref.null any)))GC",
     true},
    {"heap-eq", R"GC((module (func (result (ref null eq)) ref.null eq)))GC",
     true},
    {"heap-i31", R"GC((module (func (result (ref null i31)) ref.null i31)))GC",
     true},
    {"heap-struct",
     R"GC((module (func (result (ref null struct)) ref.null struct)))GC", true},
    {"heap-array",
     R"GC((module (func (result (ref null array)) ref.null array)))GC", true},
    {"heap-none",
     R"GC((module (func (result (ref null none)) ref.null none)))GC", true},
    {"heap-nofunc",
     R"GC((module (func (result (ref null nofunc)) ref.null nofunc)))GC", true},
    {"heap-noextern",
     R"GC((module (func (result (ref null noextern)) ref.null noextern)))GC",
     true},
    {"heap-func",
     R"GC((module (func (result (ref null func)) ref.null func)))GC", true},
    {"heap-extern",
     R"GC((module (func (result (ref null extern)) ref.null extern)))GC", true},
    {"heap-exn", R"GC((module (func (result (ref null exn)) ref.null exn)))GC",
     true},
    {"heap-noexn",
     R"GC((module (func (result (ref null noexn)) ref.null noexn)))GC", true},
    {"gc-packed",
     R"GC((module (type $s (struct (field i8 i16 (mut i8) (mut i16) i32))) (type $a (array (mut i8)))))GC",
     true},
    {"gc-rec",
     R"GC((module (rec (type $s (struct (field (ref null $a)))) (type $a (array (mut (ref null $s)))))))GC",
     true},
    {"gc-sub",
     R"GC((module (type $s (sub (struct (field i32)))) (type $t (sub final $s (struct (field i32 i32))))))GC",
     true},
    {"gc-sub-function",
     R"GC((module (type $f (sub (func (param eqref) (result anyref)))) (type $g (sub final $f (func (param anyref) (result eqref))))))GC",
     true},
    {"gc-array-sub",
     R"GC((module (type $a (sub (array i32))) (type $b (sub final $a (array i32)))))GC",
     true},
    {"gc-shared", R"GC((module (type $s (shared (struct (field i32))))))GC",
     true},
    {"gc-shared-ref", R"GC((module (func (param (ref null (shared any))))))GC",
     true},
    {"gc-empty-rec", R"GC((module (rec)))GC", true},
    {"gc-invalid-final-parent",
     R"GC((module (type $a (struct)) (type (sub $a (struct)))))GC", false},
    {"gc-br-on-non-null-typed",
     R"GC((module (func block $b (result (ref any)) unreachable br_on_non_null $b end drop)))GC",
     true},
    {"gc-cast-shorthand",
     R"GC((module (func unreachable ref.cast anyref drop)))GC", true},
    {"gc-test-shorthand",
     R"GC((module (func unreachable ref.test eqref drop)))GC", true},
    {"gc-null-concrete",
     R"GC((module (type $s (struct)) (func (result (ref null $s)) ref.null $s)))GC",
     true},
    {"gc-null-numeric",
     R"GC((module (type (struct)) (func (result (ref null 0)) ref.null 0)))GC",
     true},
};
template <class Language>
bool run(const Language& language) {
  namespace ast = wat::ast;
  bool ok = true;
  auto check = [&](bool v, const char* what) {
    if (!v) {
      std::fprintf(stderr, "wat GC: %s\n", what);
      ok = false;
    }
  };
  const auto& services = language.language;
  for (const auto& c : cases) {
    cursiv::Document document(cursiv::TextDocument::create(
        "memory:gc", "wat", 0, std::string(c.source)));
    document.language = &services;
    document.parseResult = services.parser->parse(c.source);
    const auto& parsed = document.parseResult;
    check(parsed.accepted && !parsed.cst().recovered() &&
              parsed.diagnostics.empty(),
          c.name.data());
    std::vector<cursiv::Diagnostic> errors;
    services.documentValidator->validateDocument(document, errors);
    if (c.validated) check(errors.empty(), c.name.data());
    const auto* script = parsed.template value_as<ast::Script>();
    if (!script || script->cmds.empty()) continue;
    const auto* ptr = std::get_if<ast::Module*>(&script->cmds[0]);
    const auto* module = ptr ? *ptr : nullptr;
    if (!module) continue;
    auto type = [&](std::size_t i) {
      auto p = i < module->fields.size()
                   ? std::get_if<ast::TypeDef*>(&module->fields[i])
                   : nullptr;
      return p ? *p : nullptr;
    };
    if (c.name == "gc-packed") {
      const auto* t = type(0);
      check(t && t->fields.size() == 5, "packed field count");
      if (t && t->fields.size() == 5)
        check(t->fields[0]->valueType == "i8" &&
                  t->fields[1]->valueType == "i16" && t->fields[2]->mutable_ &&
                  t->fields[3]->mutable_ && t->fields[4]->valueType == "i32",
              "packed field roles");
    }
    if (c.name == "gc-rec") {
      const auto* p = std::get_if<ast::RecGroup*>(&module->fields[0]);
      const auto* group = p ? *p : nullptr;
      check(group && group->types.size() == 2 && group->parent == module,
            "recursive group");
      if (group && group->types.size() == 2)
        check(group->types[0]->name == "$s" && group->types[1]->name == "$a" &&
                  group->types[0]->parent == group &&
                  group->types[0]->fields[0]->valueType == "(refnull$a)",
              "recursive type fields and parent");
    }
    if (c.name == "gc-sub") {
      const auto* a = type(0);
      const auto* b = type(1);
      check(a && b && a->final_ == false && b->final_ == true &&
                b->supertypes.size() == 1 && b->supertypes[0] == "$s",
            "subtype final and supertype");
    }
    if (c.name == "gc-folded-0") {
      const auto* p = std::get_if<ast::Func*>(&module->fields[7]);
      const auto* f = p ? *p : nullptr;
      const auto* cast = f && !f->body.empty() ? f->body[0] : nullptr;
      check(cast && cast->op == "ref.cast" && cast->folded &&
                cast->args.size() == 2,
            "folded cast shape");
      if (cast && cast->args.size() == 2) {
        const auto* ref = std::get_if<std::string_view>(&cast->args[0]);
        const auto* operand = std::get_if<ast::Instr*>(&cast->args[1]);
        check(ref && *ref == "(refnull$s)" && operand &&
                  (*operand)->op == "ref.null" && (*operand)->parent == cast,
              "cast immediate versus operand");
        const auto& span = parsed.cst()[cast->cstNodeId];
        check(c.source.substr(span.begin, span.end - span.begin) ==
                  "(ref.cast (ref null $s) (ref.null any))",
              "folded cast source range");
      }
    }
  }
  struct Invalid {
    std::string_view source, code;
  };
  for (const auto c :
       {Invalid{"(module (type (array i32 i64)))", "wat.array-fields"},
        Invalid{"(module (type $s (struct)) (func struct.new))",
                "wat.gc-immediate"},
        Invalid{"(module (func struct.no_such_operation))", "wat.opcode"},
        Invalid{"(module (type $a (struct)) (type (sub $a (struct))))",
                "wat.final-supertype"}}) {
    cursiv::Document document(cursiv::TextDocument::create(
        "memory:invalid-gc", "wat", 0, std::string(c.source)));
    document.language = &services;
    document.parseResult = services.parser->parse(c.source);
    check(document.parseResult.accepted &&
              !document.parseResult.cst().recovered() &&
              document.parseResult.diagnostics.empty(),
          "invalid form remains parseable");
    std::vector<cursiv::Diagnostic> errors;
    services.documentValidator->validateDocument(document, errors);
    bool found = false;
    for (const auto& e : errors)
      if (e.code && e.code->isString() && e.code->string() == c.code)
        found = true;
    check(found, "runtime GC diagnostic");
  }
  return ok;
}
}  // namespace wat_gc_audit
