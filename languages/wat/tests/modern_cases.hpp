#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "wat/core/ast.hpp"
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>
namespace wat_audit {
template <class Language> bool modern_cases(const Language &language) {
  namespace a = wat::ast;
  const auto &parser = *language.language.parser;
  bool ok = true;
  unsigned checks = 0;
  auto check = [&](bool v, const char *message) {
    ++checks;
    if (!v) {
      std::fprintf(stderr, "wat audit: %s\n", message);
      ok = false;
    }
  };
  struct Case {
    std::string_view name, source;
    bool validated;
  };
  constexpr Case valid[] = {
      {"utf8-name", R"WT((module (func (export "\c3\a9") (export "a\00b")) (data "\ff")))WT", true},
      {"unicode-underscores", R"WT((module (data "\u{1_0000}\u{1__0000}\u{10_ffff}")))WT", true},
      {"empty", R"WT()WT", true},
      {"module", R"WT((module))WT", true},
      {"headerless",
       R"WT((func $f (result i32) i32.const 7) (export "f" (func $f)))WT",
       true},
      {"comments", R"WT((module (; outer (; inner ;) end ;) ;;tail
))WT",
       true},
      {"decoded",
       R"WT((module (memory (export "m\u{1f642}") 1) (data (i32.const 0) "a\00b\n\t\r\\\"\27\u{1f642}")))WT",
       true},
      {"numeric",
       R"WT((module (func (result i64) i64.const 0xffff_ffff_ffff_ffff) (func (result f64) f64.const -0x1.8p+2)))WT",
       true},
      {"flat-end-label",
       R"WT((module (func (result i32) block $b (result i32) i32.const 1 br $b end $b)))WT",
       true},
      {"flat-else-label",
       R"WT((module (func (param i32) (result i32) local.get 0 if $b (result i32) i32.const 1 else $b i32.const 2 end $b)))WT",
       true},
      {"folded-arith",
       R"WT((module (func (result i32) (i32.add (i32.const 1) (i32.mul (i32.const 2) (i32.const 3))))))WT",
       true},
      {"fold-if",
       R"WT((module (func (param i32) (result i32) (if (result i32) (local.get 0) (then (i32.const 1)) (else (i32.const 2))))))WT",
       true},
      {"multivalue",
       R"WT((module (func (result i32 i64) block (result i32 i64) i32.const 1 i64.const 2 end)))WT",
       true},
      {"block-params",
       R"WT((module (func (param i32) (result i32) local.get 0 block (param i32) (result i32) end)))WT",
       true},
      {"block-type",
       R"WT((module (type $t (func (param i32) (result i32))) (func (param i32) (result i32) local.get 0 block (type $t) end)))WT",
       true},
      {"call-indirect",
       R"WT((module (type $t (func (result i32))) (table 1 funcref) (func (result i32) i32.const 0 call_indirect (type $t) i32.const 1 i32.add)))WT",
       true},
      {"call-indirect-table",
       R"WT((module (type $t (func)) (table $tab 1 funcref) (func i32.const 0 call_indirect $tab (type $t))))WT",
       true},
      {"memory-index",
       R"WT((module (memory $m 1) (func (result i32) memory.size $m)))WT",
       true},
      {"memory-load",
       R"WT((module (memory $m 1) (func (result i32) i32.const 0 i32.load $m offset=4 align=2)))WT",
       true},
      {"bulk-memory",
       R"WT((module (memory 1) (data $d "abc") (func i32.const 0 i32.const 0 i32.const 3 memory.init $d data.drop $d i32.const 0 i32.const 0 i32.const 3 memory.copy i32.const 0 i32.const 0 i32.const 3 memory.fill)))WT",
       true},
      {"passive-data", R"WT((module (memory 1) (data $d "ab" "\00cd")))WT",
       true},
      {"active-data-named",
       R"WT((module (memory $m 1) (data $d (memory $m) (offset (i32.const 0)) "a" "b")))WT",
       true},
      {"passive-elem", R"WT((module (func $f) (elem $e func $f)))WT", true},
      {"declared-elem", R"WT((module (func $f) (elem $e declare func $f)))WT",
       true},
      {"active-elem",
       R"WT((module (table $t 1 funcref) (func $f) (elem $e (table $t) (i32.const 0) func $f)))WT",
       true},
      {"element-expressions",
       R"WT((module (table 2 funcref) (func $f) (elem (i32.const 0) funcref (ref.func $f) (ref.null func))))WT",
       true},
      {"references",
       R"WT((module (global $g externref (ref.null extern)) (func (param externref) (result i32) local.get 0 ref.is_null)))WT",
       true},
      {"ref-typed", R"WT((module (type $t (func)) (func (param (ref $t)))))WT",
       true},
      {"select-types",
       R"WT((module (func (param externref externref i32) (result externref) local.get 0 local.get 1 local.get 2 select (result externref))))WT",
       true},
      {"sign-extension",
       R"WT((module (func (param i32) (result i32) local.get 0 i32.extend8_s)))WT",
       true},
      {"saturating",
       R"WT((module (func (param f64) (result i32) local.get 0 i32.trunc_sat_f64_s)))WT",
       true},
      {"simd",
       R"WT((module (func (result v128) v128.const i32x4 1 2 3 4 i8x16.popcnt)))WT",
       true},
      {"simd-shuffle",
       R"WT((module (func (param v128 v128) (result v128) local.get 0 local.get 1 i8x16.shuffle 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15)))WT",
       true},
      {"simd-lane",
       R"WT((module (func (param v128) (result i32) local.get 0 i32x4.extract_lane 3)))WT",
       true},
      {"atomic",
       R"WT((module (memory 1 1 shared) (func (result i32) i32.const 0 i32.atomic.load offset=4 align=4)))WT",
       true},
      {"atomic-fence", R"WT((module (func atomic.fence)))WT", true},
      {"memory64",
       R"WT((module (memory i64 1 2) (func (result i64) memory.size)))WT",
       true},
      {"custom-page", R"WT((module (memory 1 2 (pagesize 1))))WT", true},
      {"memory-inline", R"WT((module (memory (data "a\00b"))))WT", true},
      {"import-typeuse",
       R"WT((module (type $t (func (param i32) (result i32))) (import "a\00b" "f\u{41}" (func $f (type $t) (param i32) (result i32)))))WT",
       true},
      {"tailcall",
       R"WT((module (func $f (result i32) i32.const 1) (func (result i32) return_call $f)))WT",
       true},
      {"gc-struct",
       R"WT((module (type $s (struct (field $x i32) (field (mut i64))))))WT",
       true},
      {"gc-array", R"WT((module (type $a (array (mut i32)))))WT", true},
      {"exception-tag",
       R"WT((module (tag $e (param i32)) (func i32.const 1 throw $e)))WT",
       true},
      {"exception-try",
       R"WT((module (tag $e) (func try $l throw $e catch $e nop end $l)))WT",
       true},
      {"exception-table",
       R"WT((module (tag $e) (func block $b try_table (catch $e $b) throw $e end end)))WT",
       true},
      {"assert-nan",
       R"WT((module (func (export "nan") (result f32) f32.const nan)) (assert_return (invoke "nan") (f32.const nan:canonical)))WT",
       true},
      {"assert-v128",
       R"WT((module (func (export "v") (result v128) v128.const i32x4 1 2 3 4)) (assert_return (invoke "v") (v128.const i32x4 1 2 3 4)))WT",
       true},
      {"assert-exception",
       R"WT((module (tag $e) (func (export "f") throw $e)) (assert_exception (invoke "f")))WT",
       true},
      {"delegate", R"WT((module (func try nop delegate 0)))WT", true},
      {"import-tag",
       R"WT((module (import "m" "e" (tag $e (param i32))) (export "e" (tag $e))))WT",
       true},
      {"table64", R"WT((module (table $t i64 1 2 externref)))WT", true},
      {"inline-table-expressions",
       R"WT((module (func $f) (table funcref (elem (ref.func $f)))))WT", true},
      {"memory64-inline", R"WT((module (memory i64 (data "ab"))))WT", true},
      {"custom-annotation",
       R"WT((module (@custom "a\00b" (after function) "a\00c")))WT", true},
      {"metadata-annotation",
       R"WT((module (func (@metadata.code.branch_hint "\00") nop)))WT", true},
      {"invalid-typecheck", R"WT((module (func (result i32) i64.const 1)))WT",
       false},
      {"invalid-limit", R"WT((module (memory 3 2)))WT", false},
  };
  auto diagnostics = [&](std::string_view source) {
    cursiv::Document doc(cursiv::TextDocument::create(
        "file:///audit.wat", "wat", 0, std::string(source)));
    doc.language = &language.language;
    doc.parseResult = parser.parse(doc.textDocument().getText());
    check(doc.parseSucceeded() && !doc.parseRecovered(),
          "runtime fixture parses without repair");
    if (doc.parseResult.value)
      doc.parseResult.value->parent = &doc;
    std::vector<cursiv::Diagnostic> result;
    language.language.documentValidator->validateDocument(doc, result);
    return result;
  };
  for (const auto &entry : valid) {
    auto parsed = parser.parse(entry.source);
    bool clean = parsed.accepted && !parsed.cst().recovered() &&
                 parsed.diagnostics.empty();
    if (!clean)
      std::fprintf(stderr, "wat native source: %.*s\n", int(entry.name.size()),
                   entry.name.data());
    check(clean, "native parse-positive source parses cleanly");
    std::unordered_set<const cursiv::AstNode *> reachable;
    bool parents = true;
    auto walk = [&](auto &&self, const cursiv::AstNode *node,
                    const cursiv::AstNode *parent) -> void {
      if (!node || !reachable.insert(node).second) {
        parents = false;
        return;
      }
      parents &= node->parent == parent;
      auto child = [&](const cursiv::AstNode *n) {
        if (n)
          self(self, n, node);
      };
      auto variant = [&](const auto &v) {
        std::visit(
            [&](const auto &x) {
              if constexpr (std::is_pointer_v<std::decay_t<decltype(x)>>)
                child(x);
            },
            v);
      };
      if (auto x = cursiv::ast_cast<a::Script>(node)) {
        for (const auto &c : x->cmds)
          variant(c);
        for (const auto &f : x->fields)
          variant(f);
      }
      if (auto x = cursiv::ast_cast<a::Module>(node))
        for (const auto &f : x->fields)
          variant(f);
      if (auto x = cursiv::ast_cast<a::TypeDef>(node)) {
        for (auto n : x->params)
          child(n);
        for (auto n : x->fields)
          child(n);
      }
      if (auto x = cursiv::ast_cast<a::Global>(node)) {
        child(x->import_);
        for (auto n : x->init)
          child(n);
      }
      if (auto x = cursiv::ast_cast<a::Table>(node)) {
        child(x->import_);
        for (auto n : x->inlineExprs)
          child(n);
      }
      if (auto x = cursiv::ast_cast<a::Memory>(node))
        child(x->import_);
      if (auto x = cursiv::ast_cast<a::FunctionDecl>(node)) {
        child(x->import_);
        for (auto n : x->params)
          child(n);
        for (auto n : x->locals)
          child(n);
        for (auto n : x->body)
          child(n);
      }
      if (auto x = cursiv::ast_cast<a::Elem>(node)) {
        for (auto n : x->offset)
          child(n);
        for (auto n : x->expressions)
          child(n);
      }
      if (auto x = cursiv::ast_cast<a::Data>(node))
        for (auto n : x->offset)
          child(n);
      if (auto x = cursiv::ast_cast<a::Import>(node))
        for (auto n : x->params)
          child(n);
      if (auto x = cursiv::ast_cast<a::Instr>(node))
        for (const auto &v : x->args)
          variant(v);
      if (auto x = cursiv::ast_cast<a::Action>(node))
        for (auto n : x->args)
          child(n);
      if (auto x = cursiv::ast_cast<a::Assertion>(node)) {
        variant(x->module_);
        child(x->action);
        for (auto n : x->results)
          child(n);
      }
    };
    walk(walk, parsed.value, nullptr);
    check(parents && reachable.size() == parsed.nodes.size(),
          "every captured node is reachable with its exact field parent");
    if (entry.validated) {
      auto errors = diagnostics(entry.source);
      if (!errors.empty())
        std::fprintf(stderr, "wat false diagnostics: %.*s\n",
                     int(entry.name.size()), entry.name.data());
      check(errors.empty(),
            "natively validated source has no local diagnostic");
    }
  }
  auto parse = [&](std::string_view name) {
    for (const auto &c : valid)
      if (c.name == name)
        return parser.parse(c.source);
    return parser.parse("");
  };
  auto module = [](const cursiv::ParseResult &r) {
    auto s = cursiv::ast_cast<a::Script>(r.value);
    auto m =
        s && !s->cmds.empty() ? std::get_if<a::Module *>(&s->cmds[0]) : nullptr;
    return m ? *m : nullptr;
  };
  auto field = [&](const cursiv::ParseResult &r,
                   std::size_t i) -> const cursiv::AstNode * {
    auto m = module(r);
    return m && m->fields.size() > i
               ? std::visit(
                     [](auto *n) -> const cursiv::AstNode * { return n; },
                     m->fields[i])
               : nullptr;
  };
  auto instr = [](const a::Instr *n, std::size_t i) {
    if (!n || n->args.size() <= i)
      return static_cast<a::Instr *>(nullptr);
    auto v = std::get_if<a::Instr *>(&n->args[i]);
    return v ? *v : nullptr;
  };
  auto text = [](const a::Instr *n, std::size_t i, std::string_view value) {
    if (!n || n->args.size() <= i)
      return false;
    auto v = std::get_if<std::string_view>(&n->args[i]);
    return v && *v == value;
  };
  {
    auto r = parse("unicode-underscores");
    auto data = cursiv::ast_cast<a::Data>(field(r, 0));
    check(data && data->strings.size() == 1 &&
              data->strings[0] == "\xf0\x90\x80\x80\xf0\x90\x80\x80\xf4\x8f\xbf\xbf",
          "Unicode escapes with native internal underscores decode exactly");
  }
  {
    auto r = parse("decoded");
    auto memory = cursiv::ast_cast<a::Memory>(field(r, 0));
    auto data = cursiv::ast_cast<a::Data>(field(r, 1));
    check(memory && memory->exports.size() == 1 &&
              memory->exports[0] == "m\xf0\x9f\x99\x82",
          "Unicode export name is decoded to native UTF-8 bytes");
    check(data && data->strings.size() == 1 &&
              data->strings[0] ==
                  std::string_view("a\0b\n\t\r\\\"'\xf0\x9f\x99\x82", 13),
          "data bytes preserve NUL, escapes and Unicode after decoding");
  }
  {
    auto r = parse("numeric");
    auto f = cursiv::ast_cast<a::Func>(field(r, 0));
    auto g = cursiv::ast_cast<a::Func>(field(r, 1));
    check(f && f->body.size() == 1 && f->body[0]->op == "i64.const" &&
              text(f->body[0], 0, "0xffff_ffff_ffff_ffff"),
          "i64 literal preserves all significant digits and separators");
    check(g && g->body.size() == 1 && g->body[0]->op == "f64.const" &&
              text(g->body[0], 0, "-0x1.8p+2"),
          "hexadecimal float spelling and exponent are retained");
  }
  {
    auto r = parse("flat-end-label");
    auto f = cursiv::ast_cast<a::Func>(field(r, 0));
    auto b = f && !f->body.empty() ? f->body[0] : nullptr;
    check(b && b->op == "block" && b->label == "$b" && b->endLabel == "$b" &&
              !b->folded,
          "opening and closing labels are distinct captured fields");
    auto q = parse("flat-else-label");
    f = cursiv::ast_cast<a::Func>(field(q, 0));
    b = f && f->body.size() == 2 ? f->body[1] : nullptr;
    auto e = instr(b, 2);
    check(b && b->op == "if" && b->label == "$b" && b->endLabel == "$b" && e &&
              e->op == "else" && e->label == "$b" && e->args.size() == 1 &&
              text(instr(e, 0), 0, "2"),
          "flat else has its own label and structured branch body");
  }
  {
    auto r = parse("multivalue");
    auto f = cursiv::ast_cast<a::Func>(field(r, 0));
    auto b = f && !f->body.empty() ? f->body[0] : nullptr;
    auto sig = instr(b, 0);
    check(f && f->results.size() == 2 && f->results[0] == "i32" &&
              f->results[1] == "i64" && sig && sig->op == "result" &&
              sig->args.size() == 2 && text(sig, 0, "i32") &&
              text(sig, 1, "i64"),
          "multi-value block keeps signature clause and result order");
  }
  {
    auto r = parse("call-indirect");
    auto f = cursiv::ast_cast<a::Func>(field(r, 2));
    auto call = f && f->body.size() == 4 ? f->body[1] : nullptr;
    auto type = instr(call, 0);
    check(call && call->op == "call_indirect" && call->args.size() == 1 &&
              type && type->op == "type" && text(type, 0, "$t") &&
              f->body[2]->op == "i32.const" && f->body[3]->op == "i32.add",
          "call_indirect retains type clause and does not swallow the next "
          "instruction");
    auto q = parse("call-indirect-table");
    f = cursiv::ast_cast<a::Func>(field(q, 2));
    call = f && f->body.size() == 2 ? f->body[1] : nullptr;
    check(call && text(call, 0, "$tab") && instr(call, 1) &&
              text(instr(call, 1), 0, "$t"),
          "explicit indirect-call table index remains separate from type use");
  }
  {
    auto r = parse("memory-load");
    auto f = cursiv::ast_cast<a::Func>(field(r, 1));
    auto load = f && f->body.size() == 2 ? f->body[1] : nullptr;
    check(load && load->op == "i32.load" && load->args.size() == 3 &&
              text(load, 0, "$m") && text(load, 1, "offset=4") &&
              text(load, 2, "align=2"),
          "memory index, offset and alignment remain separate immediates");
    auto q = parse("memory64");
    auto m = cursiv::ast_cast<a::Memory>(field(q, 0));
    check(m && m->indexType == "i64" && m->min == "1" && m->max == "2",
          "64-bit memory index type and limits");
    auto v = parse("table64");
    auto t = cursiv::ast_cast<a::Table>(field(v, 0));
    check(t && t->indexType == "i64" && t->elemType == "externref" &&
              t->min == "1" && t->max == "2",
          "64-bit table index type, reference kind and limits");
  }
  {
    auto r = parse("passive-data");
    auto d = cursiv::ast_cast<a::Data>(field(r, 1));
    check(d && d->name == "$d" && !d->memVar && d->offset.empty() &&
              d->strings.size() == 2 && d->strings[0] == "ab" &&
              d->strings[1] == std::string_view("\0cd", 3),
          "passive segment name and unmerged decoded source fragments");
    auto q = parse("active-data-named");
    d = cursiv::ast_cast<a::Data>(field(q, 1));
    check(d && d->name == "$d" && d->memVar == "$m" && d->offset.size() == 1 &&
              text(d->offset[0], 0, "0") && d->strings.size() == 2 &&
              d->strings[0] == "a" && d->strings[1] == "b",
          "active segment keeps segment name, memory index and offset "
          "separately");
  }
  {
    auto r = parse("declared-elem");
    auto e = cursiv::ast_cast<a::Elem>(field(r, 1));
    check(e && e->name == "$e" && e->declared && e->offset.empty() &&
              e->funcVars.size() == 1 && e->funcVars[0] == "$f",
          "declared element segment flag and function list");
    auto q = parse("element-expressions");
    e = cursiv::ast_cast<a::Elem>(field(q, 2));
    check(e && e->elemType == "funcref" && e->expressions.size() == 2 &&
              e->offset.size() == 1 && e->expressions[0]->op == "ref.func" &&
              text(e->expressions[0], 0, "$f") &&
              e->expressions[1]->op == "ref.null" &&
              text(e->expressions[1], 0, "func"),
          "element initializer expressions are separate from active offset");
  }
  {
    auto r = parse("simd");
    auto f = cursiv::ast_cast<a::Func>(field(r, 0));
    auto c = f && f->body.size() == 2 ? f->body[0] : nullptr;
    check(f && f->results.size() == 1 && f->results[0] == "v128" && c &&
              c->op == "v128.const" && c->args.size() == 5 &&
              text(c, 0, "i32x4") && text(c, 1, "1") && text(c, 4, "4") &&
              f->body[1]->op == "i8x16.popcnt",
          "SIMD shape and every lane immediate are captured");
    auto q = parse("simd-shuffle");
    f = cursiv::ast_cast<a::Func>(field(q, 0));
    auto shuffle = f && f->body.size() == 3 ? f->body[2] : nullptr;
    check(shuffle && shuffle->op == "i8x16.shuffle" &&
              shuffle->args.size() == 16 && text(shuffle, 0, "0") &&
              text(shuffle, 15, "15"),
          "SIMD shuffle retains all lane indices in order");
  }
  {
    auto r = parse("import-typeuse");
    auto i = cursiv::ast_cast<a::Import>(field(r, 1));
    check(i && i->module_ == std::string_view("a\0b", 3) && i->name == "fA" &&
              i->typeUse == "$t" && i->params.size() == 1 &&
              i->params[0]->valueType == "i32" && i->results.size() == 1 &&
              i->results[0] == "i32",
          "import type use and inline signature coexist with decoded names");
  }
  {
    auto r = parse("gc-struct");
    auto t = cursiv::ast_cast<a::TypeDef>(field(r, 0));
    check(t && t->kind == "struct" && t->name == "$s" &&
              t->fields.size() == 2 && t->fields[0]->name == "$x" &&
              t->fields[0]->valueType == "i32" && !t->fields[0]->mutable_ &&
              t->fields[1]->valueType == "i64" && t->fields[1]->mutable_,
          "GC field names, storage types and mutability");
    auto q = parse("gc-array");
    t = cursiv::ast_cast<a::TypeDef>(field(q, 0));
    check(t && t->kind == "array" && t->fields.size() == 1 &&
              t->fields[0]->mutable_ && t->fields[0]->valueType == "i32",
          "array element type and mutability");
  }
  {
    auto r = parse("exception-tag");
    auto t = cursiv::ast_cast<a::Tag>(field(r, 0));
    auto f = cursiv::ast_cast<a::Func>(field(r, 1));
    check(t && t->name == "$e" && t->params.size() == 1 &&
              t->params[0]->valueType == "i32" && f && f->body.size() == 2 &&
              f->body[1]->op == "throw" && text(f->body[1], 0, "$e"),
          "tag declaration is distinct from a function and throw retains its "
          "tag index");
    auto q = parse("delegate");
    f = cursiv::ast_cast<a::Func>(field(q, 0));
    auto b = f && !f->body.empty() ? f->body[0] : nullptr;
    auto delegate = instr(b, 1);
    check(b && b->op == "try" && !b->endLabel && delegate &&
              delegate->op == "delegate" && text(delegate, 0, "0"),
          "delegate is retained as the closing try operand");
  }
  {
    auto r = parse("assert-v128");
    auto s = cursiv::ast_cast<a::Script>(r.value);
    auto ret = s && s->cmds.size() == 2
                   ? std::get_if<a::Assertion *>(&s->cmds[1])
                   : nullptr;
    auto c = ret && *ret && (*ret)->results.size() == 1 ? (*ret)->results[0]
                                                        : nullptr;
    check(c && c->valueType == "v128.const" && c->literals.size() == 5 &&
              c->literals[0] == "i32x4" && c->literals[4] == "4",
          "script vector expectation retains shape and all lane values");
    auto q = parse("assert-nan");
    s = cursiv::ast_cast<a::Script>(q.value);
    ret = s && s->cmds.size() == 2 ? std::get_if<a::Assertion *>(&s->cmds[1])
                                   : nullptr;
    c = ret && *ret && (*ret)->results.size() == 1 ? (*ret)->results[0]
                                                   : nullptr;
    check(c && c->literals.size() == 1 && c->literals[0] == "nan:canonical",
          "NaN expectation kind is preserved");
  }
  {
    auto r = parse("custom-annotation");
    auto n = cursiv::ast_cast<a::Instr>(field(r, 0));
    auto placement = instr(n, 1);
    check(n && n->op == "@custom" && n->folded && n->args.size() == 3 &&
              text(n, 0, std::string_view("a\0b", 3)) && placement &&
              placement->op == "after" && text(placement, 0, "function") &&
              text(n, 2, std::string_view("a\0c", 3)),
          "custom section annotation preserves decoded name, placement and "
          "payload");
    auto q = parse("metadata-annotation");
    auto f = cursiv::ast_cast<a::Func>(field(q, 0));
    auto meta = f && f->body.size() == 2 ? f->body[0] : nullptr;
    check(meta && meta->op == "@metadata.code.branch_hint" &&
              meta->args.size() == 1 &&
              text(meta, 0, std::string_view("\0", 1)) &&
              f->body[1]->op == "nop",
          "code metadata is preserved before its instruction");
  }
  {
    auto r = parse("folded-arith");
    auto f = cursiv::ast_cast<a::Func>(field(r, 0));
    auto add = f && f->body.size() == 1 ? f->body[0] : nullptr;
    auto left = instr(add, 0);
    auto multiply = instr(add, 1);
    check(add && add->folded && add->op == "i32.add" && left &&
              left->op == "i32.const" && text(left, 0, "1") && multiply &&
              multiply->op == "i32.mul" && multiply->folded &&
              text(instr(multiply, 0), 0, "2") &&
              text(instr(multiply, 1), 0, "3"),
          "folded operands retain their nesting and evaluation order");
    auto q = parse("ref-typed");
    f = cursiv::ast_cast<a::Func>(field(q, 1));
    check(f && f->params.size() == 1 && f->params[0]->valueType == "(ref$t)",
          "typed function reference is retained in its parameter type");
  }
  for (const unsigned code : {0U, 9U, 13U, 31U, 127U}) {
    const std::string source = "(module (data \"a" + std::string(1, char(code)) + "b\"))";
    const auto errors = diagnostics(source);
    check(errors.size() == 1 && errors[0].code &&
              errors[0].code->isString() && errors[0].code->string() == "wat.string",
          "unescaped string controls receive a native lexical diagnostic");
  }
  for (const std::string source : {std::string(";;\x80\n(module)"),
                                    std::string("(module (; \xc3 ;))"), std::string("(module (data \"\xc3\"))"),
                                    std::string("(module (data \"[\xed\xa0\x80]\"))")}) {
    const auto result = parser.parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "raw malformed UTF-8 cannot become a WAT string character");
  }
  struct Invalid {
    std::string_view source, code, selected;
  };
  constexpr Invalid invalid[] = {
      {R"((module (func (export "\ff"))))", "wat.name-utf8", R"("\ff")"},
      {R"((module (import "\ff" "f" (func))))", "wat.name-utf8", R"("\ff")"},
      {R"((module (func (export "\c0\af"))))", "wat.name-utf8", R"("\c0\af")"},
      {R"((module (data "\u{_10000}")))", "wat.string", R"(\u{_10000})"},
      {R"((module (data "\u{10000_}")))", "wat.string", R"(\u{10000_})"},
      {R"((module (data "\u{d800}")))", "wat.string", R"(\u{d800})"},
      {"(module (func block $a end $b))", "wat.end-label", "$b"},
      {"(module (func if $a else $b end $a))", "wat.else-label", "$b"},
      {"(module (memory 1) (func i32.const 0 i32.load align=3 drop))",
       "wat.alignment", "align=3"},
      {R"((module (memory 1) (data (i32.const 0) "a\q")))", "wat.string",
       R"(\q)"},
      {"(module (memory 3 2))", "wat.limits", "2"},
      {"(module (func i32.invalid))", "wat.opcode", "i32.invalid"},
      {"(module (func i32.const))", "wat.immediate", ""},
      {"(module (memory 1 (pagesize 3)))", "wat.page-size", "3"},
  };
  for (const auto &x : invalid) {
    auto errors = diagnostics(x.source);
    unsigned count = 0;
    for (const auto &e : errors)
      if (e.code && e.code->isString() && e.code->string() == x.code) {
        ++count;
        if (!x.selected.empty())
          check(e.range.begin == x.source.find(x.selected),
                "runtime diagnostic selects the invalid operand");
      }
    if (count != 1)
      std::fprintf(stderr, "wat missing diagnostic: %.*s\n", int(x.code.size()),
                   x.code.data());
    check(count == 1, "invalid form has its precise runtime diagnostic");
  }
  std::printf("wat audit: %zu native parse-positive forms, %u checks\n",
              std::size(valid), checks);
  return ok;
}
} // namespace wat_audit
