#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "swift/core/ast.hpp"
#include "swift/generated/Swift/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool trailing_closure_boundaries(const cursiv::Parser &parser) {
  namespace ast = swift::ast;
  bool ok = true;
  const auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  struct Statements {
    const char *text, *nextName;
  };
  for (const Statements c : {
           Statements{"let sortedNumbers = numbers.sorted { $0 > $1 }\n"
                      "print(sortedNumbers)\n", "print"},
           Statements{"numbers.sorted { $0 > $1 }\nprint(sortedNumbers)\n", "print"},
           Statements{"foo {}\nbar()\n", "bar"},
           Statements{"foo { foo() }\nbar()\n", "bar"},
           Statements{"printTwoStrings { (_ x: String, _ y: String) in print(x, y) }\n"
                      "print(1)\n", "print"},
           Statements{"numbers.map({ (number: Int) -> Int in return number })\n"
                      "print(1)\n", "print"},
           Statements{"numbers.map({ number in 3 * number })\nprint(1)\n", "print"},
       }) {
    const auto r = parser.parse(c.text);
    auto *file = as<ast::File>(r.value);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
              file && file->statements.size() == 2,
          "a statement after a trailing closure stays separate");
    if (!file || file->statements.size() != 2) continue;
    auto *second = as<ast::ExprStmt>(file->statements[1]);
    auto *call = second ? as<ast::CallExpr>(second->value) : nullptr;
    auto *name = call ? as<ast::Ident>(call->callee) : nullptr;
    check(name && name->name == c.nextName &&
              call->labeledTrailingClosures.empty(),
          "the following call retains its callee and statement AST");
  }
  struct Label {
    const char *text, *name;
  };
  for (const Label c : {
           Label{"foo {} label: {}\n", "label"},
           Label{"foo {} label : {}\n", "label"},
           Label{"foo {} label\n: {}\n", "label"},
           Label{"foo {}\nlabel: {}\n", "label"},
           Label{"foo {} label /* reason */ : {}\n", "label"},
           Label{"foo {} label :\n{}\n", "label"},
           Label{"foo {} label : /* reason */ {}\n", "label"},
           Label{"foo {} `class` : {}\n", "class"},
       }) {
    const auto r = parser.parse(c.text);
    auto *file = as<ast::File>(r.value);
    auto *statement = file && file->statements.size() == 1
                          ? as<ast::ExprStmt>(file->statements[0]) : nullptr;
    auto *call = statement ? as<ast::CallExpr>(statement->value) : nullptr;
    auto *label = call && call->labeledTrailingClosures.size() == 1
                      ? call->labeledTrailingClosures[0] : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
              call && call->trailingClosure && label && label->label == c.name &&
              label->parent == call && label->closure &&
              label->closure->parent == label,
          "labeled trailing closures preserve trivia and label text");
  }
  struct LabeledLoop {
    const char *text;
    bool isWhile;
  };
  for (const LabeledLoop c : {
           LabeledLoop{"f {}\nloop: while true {}\n", true},
           LabeledLoop{"f() {}\nloop: while true {}\n", true},
           LabeledLoop{"f {}\nloop: for x in items {}\n", false},
           LabeledLoop{"f() {}\nloop: for x in items {}\n", false},
       }) {
    const auto r = parser.parse(c.text);
    auto *file = as<ast::File>(r.value);
    auto *first = file && file->statements.size() == 2
                      ? as<ast::ExprStmt>(file->statements[0]) : nullptr;
    auto *call = first ? as<ast::CallExpr>(first->value) : nullptr;
    auto *label = file && file->statements.size() == 2
                      ? as<ast::LabeledStmt>(file->statements[1]) : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
              call && call->trailingClosure &&
              call->labeledTrailingClosures.empty() && label &&
              label->label == "loop" && label->parent == file &&
              label->body && label->body->parent == label,
          "a labeled loop after a trailing closure stays a separate statement");
    auto *whileLoop = label ? as<ast::WhileStmt>(label->body) : nullptr;
    auto *forLoop = label ? as<ast::ForInStmt>(label->body) : nullptr;
    if (c.isWhile) {
      auto *condition = whileLoop && whileLoop->conditions.size() == 1
                            ? whileLoop->conditions[0] : nullptr;
      auto *value = condition ? as<ast::BoolLit>(condition->expr) : nullptr;
      check(value && value->value && whileLoop->body &&
                whileLoop->body->statements.empty(),
            "labeled while retains its condition and body");
    } else {
      auto *pattern = forLoop ? as<ast::IdentifierPattern>(forLoop->pattern) : nullptr;
      auto *collection = forLoop ? as<ast::Ident>(forLoop->collection) : nullptr;
      check(pattern && pattern->name == "x" && collection &&
                collection->name == "items" && forLoop->body &&
                forLoop->body->statements.empty(),
            "labeled for retains its pattern, collection and body");
    }
  }
  {
    const auto r = parser.parse("foo {} struct: {} in: {}\n");
    auto *file = as<ast::File>(r.value);
    auto *statement = file && file->statements.size() == 1
                          ? as<ast::ExprStmt>(file->statements[0]) : nullptr;
    auto *call = statement ? as<ast::CallExpr>(statement->value) : nullptr;
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() &&
              call && call->labeledTrailingClosures.size() == 2 &&
              call->labeledTrailingClosures[0]->label == "struct" &&
              call->labeledTrailingClosures[1]->label == "in",
          "reserved labels remain available on trailing closures");
  }
  return ok;
}

inline bool run() {
  auto parser = swift::generated::Swift::create_parser();
  bool ok = trailing_closure_boundaries(*parser);
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = swift::ast;

  {
    auto r = parser->parse(
        "let f = { [weak self, unowned(unsafe) owner] in self?.foo() }\n");
    check(r.accepted && !r.cst().recovered(), "closure capture list");
    auto *d = as<ast::File>(r.value);
    auto *v = d && !d->statements.empty()
                  ? as<ast::VariableDecl>(d->statements[0])
                  : nullptr;
    auto *c = v && !v->bindings.empty()
                  ? as<ast::ClosureExpr>(v->bindings[0]->initializer)
                  : nullptr;
    check(c && c->captures.size() == 2 && c->captures[0]->specifier == "weak" &&
              c->captures[1]->specifier == "unowned(unsafe)",
          "capture AST");
  }
  {
    auto r = parser->parse("let a = try? f()\nlet b = try! f()\nlet s = "
                           "#\"hello\"#\nfunc f(_ x: inout Int) {}\n");
    check(r.accepted && !r.cst().recovered(), "try/raw/inout syntax");
    auto *d = as<ast::File>(r.value);
    if (d && d->statements.size() == 4) {
      auto *va = as<ast::VariableDecl>(d->statements[0]);
      auto *vb = as<ast::VariableDecl>(d->statements[1]);
      auto *vs = as<ast::VariableDecl>(d->statements[2]);
      auto *fn = as<ast::FunctionDecl>(d->statements[3]);
      check(va && as<ast::TryExpr>(va->bindings[0]->initializer) &&
                as<ast::TryExpr>(va->bindings[0]->initializer)->kind == "try?",
            "optional try marker");
      check(vb && as<ast::TryExpr>(vb->bindings[0]->initializer) &&
                as<ast::TryExpr>(vb->bindings[0]->initializer)->kind == "try!",
            "forced try marker");
      check(vs && as<ast::RawStringLit>(vs->bindings[0]->initializer) &&
                as<ast::RawStringLit>(vs->bindings[0]->initializer)->text ==
                    "#\"hello\"#",
            "whole raw string");
      check(fn && fn->params.size() == 1 && fn->params[0]->inoutFlag,
            "inout flag");
    } else
      check(false, "four declarations");
  }
  {
    auto r = parser->parse(
        "#if swift(>=5.3)\nlet x = 1\n#endif\n#sourceLocation(file: "
        "\"x.swift\", line: 1)\n");
    check(r.accepted && !r.cst().recovered(),
          "compiler conditions and sourceLocation");
    auto *d = as<ast::File>(r.value);
    auto *c = d && !d->statements.empty()
                  ? as<ast::IfConfigStmt>(d->statements[0])
                  : nullptr;
    auto *cond = c && !c->branches.empty()
                     ? as<ast::PlatformCondition>(c->branches[0]->condition)
                     : nullptr;
    check(cond && cond->argument == ">=5.3" && c->branches[0]->body.size() == 1,
          "whole platform condition stays outside body");
    auto *line = d && d->statements.size() == 2
                     ? as<ast::LineControlStmt>(d->statements[1])
                     : nullptr;
    check(line && line->fileName == "x.swift" && line->line == "1",
          "sourceLocation operands retained");
  }
  {
    auto r = parser->parse("struct S { var x: Int { mutating get { return 1 } "
                           "nonmutating set {} } }\n");
    check(r.accepted && !r.cst().recovered(), "accessor mutability syntax");
    auto *d = as<ast::File>(r.value);
    auto *s = d && !d->statements.empty()
                  ? as<ast::StructDecl>(d->statements[0])
                  : nullptr;
    auto *v = s && !s->members.empty() ? as<ast::VariableDecl>(s->members[0])
                                       : nullptr;
    check(v && v->accessors.size() == 2 &&
              v->accessors[0]->mutability == "mutating" &&
              v->accessors[1]->mutability == "nonmutating",
          "accessor mutability retained");
  }

  {
    auto r = parser->parse(
        "let literal = 42\nlet plain = f()\nlet chain = try? f() + try! g()\n"
        "let grouped = (1 + 2) * 3\nlet cast = x as? Int\n"
        "let test = x is Int\nlet choice = flag ? 1 : try? f()\n");
    check(r.accepted && !r.cst().recovered(), "factored expression forms");
    auto *d = as<ast::File>(r.value);
    auto init = [&](std::size_t i) -> ast::Expr * {
      auto *v = d && i < d->statements.size()
                    ? as<ast::VariableDecl>(d->statements[i]) : nullptr;
      return v && v->bindings.size() == 1 ? v->bindings[0]->initializer : nullptr;
    };
    check(d && d->statements.size() == 7 && as<ast::IntLit>(init(0)) &&
              as<ast::CallExpr>(init(1)), "single expressions stay unwrapped");
    auto *chain = as<ast::BinaryChainExpr>(init(2));
    auto *first = chain ? as<ast::TryExpr>(chain->first) : nullptr;
    auto *plus = chain && chain->items.size() == 1
                     ? as<ast::BinaryOp>(chain->items[0]) : nullptr;
    check(first && first->kind == "try?" && plus && plus->op == "+" &&
              as<ast::TryExpr>(plus->operand) &&
              as<ast::TryExpr>(plus->operand)->kind == "try!" &&
              as<ast::CallExpr>(as<ast::TryExpr>(plus->operand)->operand),
          "chain preserves first and subsequent try markers");
    auto *grouped = as<ast::BinaryChainExpr>(init(3));
    auto *inner = grouped ? as<ast::BinaryChainExpr>(grouped->first) : nullptr;
    auto *times = grouped && grouped->items.size() == 1
                     ? as<ast::BinaryOp>(grouped->items[0]) : nullptr;
    check(inner && inner->items.size() == 1 && times && times->op == "*",
          "parenthesized expression keeps nested chain");
    auto *cast = as<ast::BinaryChainExpr>(init(4));
    auto *asOp = cast && cast->items.size() == 1
                     ? as<ast::AsCastOp>(cast->items[0]) : nullptr;
    check(asOp && asOp->marker && *asOp->marker == "?" && asOp->type,
          "conditional cast AST");
    auto *test = as<ast::BinaryChainExpr>(init(5));
    auto *isOp = test && test->items.size() == 1
                     ? as<ast::IsCastOp>(test->items[0]) : nullptr;
    check(isOp && isOp->type, "type test AST");
    auto *choice = as<ast::BinaryChainExpr>(init(6));
    auto *ternary = choice && choice->items.size() == 1
                     ? as<ast::TernaryOp>(choice->items[0]) : nullptr;
    check(ternary && ternary->thenExpr && as<ast::TryExpr>(ternary->operand) &&
              as<ast::TryExpr>(ternary->operand)->kind == "try?", "ternary branches and try marker");
  }
  {
    auto r = parser->parse(
        "struct S { var x: Int { @foo mutating get { return 1 } "
        "@bar nonmutating set(value) {} }\n"
        "var y: Int { willSet(next) {} didSet(old) {} } }\n"
        "protocol P { var value: Int { get set } }\n");
    check(r.accepted && !r.cst().recovered(), "all factored accessor forms");
    auto *d = as<ast::File>(r.value);
    auto *s = d && !d->statements.empty() ? as<ast::StructDecl>(d->statements[0])
                                         : nullptr;
    auto *x = s && s->members.size() == 2 ? as<ast::VariableDecl>(s->members[0])
                                         : nullptr;
    auto *y = s && s->members.size() == 2 ? as<ast::VariableDecl>(s->members[1])
                                         : nullptr;
    auto *get = x && x->accessors.size() == 2 ? x->accessors[0] : nullptr;
    auto *set = x && x->accessors.size() == 2 ? x->accessors[1] : nullptr;
    check(get && get->kind == "get" && get->mutability == "mutating" &&
              get->attributes.size() == 1 && get->hasBody &&
              get->body.size() == 1 && set && set->kind == "set" &&
              set->mutability == "nonmutating" && set->attributes.size() == 1 &&
              set->parameterName && *set->parameterName == "value" &&
              set->hasBody && set->body.empty(), "getter and setter fields");
    auto *will = y && y->accessors.size() == 2 ? y->accessors[0] : nullptr;
    auto *did = y && y->accessors.size() == 2 ? y->accessors[1] : nullptr;
    check(will && will->kind == "willSet" && will->parameterName &&
              *will->parameterName == "next" && will->hasBody &&
              did && did->kind == "didSet" && did->parameterName &&
              *did->parameterName == "old" && did->hasBody,
          "observer names and bodies");
    auto *p = d && d->statements.size() == 2
                  ? as<ast::ProtocolDecl>(d->statements[1]) : nullptr;
    auto *v = p && p->members.size() == 1 ? as<ast::VariableDecl>(p->members[0])
                                         : nullptr;
    check(v && v->accessors.size() == 2 && !v->accessors[0]->hasBody &&
              !v->accessors[1]->hasBody, "bodyless accessors remain distinct");
  }

  {
    auto r = parser->parse(
        "let normal = \"a\\(1)b\"\n"
        "let multi = \"\"\"\na\\(2)b\n\"\"\"\n"
        "let raw = #\"\"\"\nhello\n\"\"\"#\n");
    check(r.accepted && !r.cst().recovered(), "single/multiline/raw strings");
    auto *d = as<ast::File>(r.value);
    auto init = [&](std::size_t i) -> ast::Expr * {
      auto *v = d && i < d->statements.size()
                    ? as<ast::VariableDecl>(d->statements[i]) : nullptr;
      return v && v->bindings.size() == 1 ? v->bindings[0]->initializer : nullptr;
    };
    auto *normal = as<ast::StringLit>(init(0));
    auto *multi = as<ast::StringLit>(init(1));
    auto hasInterpolation = [](ast::StringLit *str) {
      if (!str) return false;
      for (auto *part : str->parts)
        if (part->interpolation) return true;
      return false;
    };
    check(normal && !normal->isMultiline && hasInterpolation(normal) &&
              multi && multi->isMultiline && hasInterpolation(multi),
          "string flags and interpolation parts");
    auto *raw = as<ast::RawStringLit>(init(2));
    check(raw && raw->text == "#\"\"\"\nhello\n\"\"\"#",
          "whole raw multiline string");
  }
  {
    auto r = parser->parse(
        "#if os(Linux) && swift(>=5.3)\nlet a = 1\n"
        "#else\nlet a = 2\n#endif\n");
    check(r.accepted && !r.cst().recovered(), "compound compilation condition");
    auto *d = as<ast::File>(r.value);
    auto *config = d && d->statements.size() == 1
                     ? as<ast::IfConfigStmt>(d->statements[0]) : nullptr;
    auto *chain = config && config->branches.size() == 2
                      ? as<ast::BinaryChainExpr>(config->branches[0]->condition)
                      : nullptr;
    auto *first = chain ? as<ast::PlatformCondition>(chain->first) : nullptr;
    auto *op = chain && chain->items.size() == 1
                   ? as<ast::BinaryOp>(chain->items[0]) : nullptr;
    auto *second = op ? as<ast::PlatformCondition>(op->operand) : nullptr;
    check(first && first->kind == "os" && first->argument == "Linux" && op &&
              op->op == "&&" && second && second->kind == "swift" &&
              second->argument == ">=5.3" &&
              config->branches[0]->body.size() == 1 &&
              config->branches[1]->body.size() == 1,
          "condition operands remain separate from branch bodies");
  }

  {
    auto r = parser->parse(
        "func f<T, U>() where T == U, T: P {}\n"
        "typealias F = (@foo T, @foo label: T, @foo inout T, "
        "@foo label: inout T) -> U\n");
    check(r.accepted && !r.cst().recovered(), "requirements/function type params");
    auto *d = as<ast::File>(r.value);
    auto *f = d && d->statements.size() == 2
                  ? as<ast::FunctionDecl>(d->statements[0]) : nullptr;
    auto *same = f && f->whereClause.size() == 2
                     ? as<ast::SameTypeReq>(f->whereClause[0]) : nullptr;
    auto *conform = f && f->whereClause.size() == 2
                        ? as<ast::ConformanceReq>(f->whereClause[1]) : nullptr;
    auto named = [](ast::TypeRef *type, const char *name) {
      auto *n = as<ast::NamedType>(type);
      return n && n->name == name;
    };
    check(same && named(same->left, "T") && named(same->right, "U") &&
              conform && named(conform->type, "T") &&
              named(conform->constraint, "P") &&
              same->left->parent == same && same->right->parent == same &&
              conform->type->parent == conform && conform->constraint->parent == conform,
          "requirement kinds, operands and parents");
    auto *alias = d && d->statements.size() == 2
                      ? as<ast::TypealiasDecl>(d->statements[1]) : nullptr;
    auto *type = alias ? as<ast::FunctionType>(alias->target) : nullptr;
    check(type && type->params.size() == 4 && named(type->result, "U"),
          "four function type parameter forms");
    if (type && type->params.size() == 4) {
      for (std::size_t i = 0; i < 4; ++i) {
        auto *param = type->params[i];
        check(param && param->attributes.size() == 1 && named(param->type, "T") &&
                  param->attributes[0]->parent == param && param->type->parent == param &&
                  param->inoutFlag == (i >= 2) &&
                  bool(param->label) == (i % 2 == 1) &&
                  (!(i % 2) || *param->label == "label"),
              "function type labels, attributes and inout flags");
      }
    }
  }

  {
    auto r = parser->parse("#sourceLocation(file: \"\", line: 1)\n"
                           "#sourceLocation()\n");
    check(r.accepted && !r.cst().recovered(), "empty sourceLocation file/reset");
    auto *d = as<ast::File>(r.value);
    auto *empty = d && d->statements.size() == 2
                      ? as<ast::LineControlStmt>(d->statements[0]) : nullptr;
    auto *reset = d && d->statements.size() == 2
                      ? as<ast::LineControlStmt>(d->statements[1]) : nullptr;
    check(empty && empty->fileName && empty->fileName->empty() &&
              empty->line == "1" && reset && !reset->fileName && !reset->line,
          "empty file name remains present, reset remains absent");
  }

  {
    // A keyword guard belongs only at the start of an expression statement:
    // contextual words, escaped names, keyword members and labels stay usable.
    for (auto text : {"self.foo()\n", "Self.foo()\n", "open()\n", "final()\n",
                      "mutating()\n", "classify()\n", "privateThing()\n",
                      "`class`()\n", "value.class()\n", ".class\n",
                      "f(class: value)\n"}) {
      auto r = parser->parse(text);
      auto *d = as<ast::File>(r.value);
      auto *expr = d && d->statements.size() == 1
                       ? as<ast::ExprStmt>(d->statements[0]) : nullptr;
      check(r.accepted && !r.cst().recovered() && expr && expr->value,
            "valid expression heads/member names/argument labels");
    }
  }

  {
    const std::string cases[] = {
        "#\"a\"b\"#", "##\"a\"#b\"##", "#\"\"#", "##\"\"##",
        "#\"\"literal\"#", "#\"\"\"\na\"b\n\"\"\"#",
        "##\"\"\"\na\"\"\"#b\n\"\"\"##",
        "#\"" + std::string(8192, '#') + "\"#",
        "#\"" + std::string(100000, 'x') + "\"#"};
    for (const auto &raw : cases) {
      const auto source = "let s = " + raw + "\n";
      auto r = parser->parse(source);
      auto *d = as<ast::File>(r.value);
      auto *v = d && d->statements.size() == 1
                    ? as<ast::VariableDecl>(d->statements[0]) : nullptr;
      auto *s = v && v->bindings.size() == 1
                    ? as<ast::RawStringLit>(v->bindings[0]->initializer) : nullptr;
      check(r.accepted && !r.cst().recovered() && s && s->text == raw,
            "raw delimiter count/internal quotes/long bodies");
    }
    auto r = parser->parse("let a = #\"first\"#\nlet b = #\"second\"#\n");
    auto *d = as<ast::File>(r.value);
    auto *a = d && d->statements.size() == 2
                  ? as<ast::VariableDecl>(d->statements[0]) : nullptr;
    auto *b = d && d->statements.size() == 2
                  ? as<ast::VariableDecl>(d->statements[1]) : nullptr;
    auto *first = a && a->bindings.size() == 1
                      ? as<ast::RawStringLit>(a->bindings[0]->initializer) : nullptr;
    auto *second = b && b->bindings.size() == 1
                       ? as<ast::RawStringLit>(b->bindings[0]->initializer) : nullptr;
    check(r.accepted && !r.cst().recovered() && first && second &&
              first->text == "#\"first\"#" && second->text == "#\"second\"#",
          "separate raw literals keep separate values");
  }

  {
    for (auto raw : {"#\"a\"b\"#", "##\"a\"#b\"##"}) {
      const auto source = "@attr(" + std::string(raw) + ") func f() {}\n";
      auto r = parser->parse(source);
      auto *d = as<ast::File>(r.value);
      auto *f = d && d->statements.size() == 1
                    ? as<ast::FunctionDecl>(d->statements[0]) : nullptr;
      auto *args = f && f->attributes.size() == 1 ? f->attributes[0]->rawArgs
                                                 : nullptr;
      check(r.accepted && !r.cst().recovered() && args &&
                args->children.size() == 1 &&
                r.cst().text(args->children[0]->cstNodeId) == raw,
            "raw attribute token keeps its complete CST range");
    }
  }

  {
    auto r = parser->parse("do {} catch {}\n"
                           "f(catch: 1, in: 2, where: 3, default: 4, as: 5, operator: 6)\n");
    auto *d = as<ast::File>(r.value);
    auto *stmt = d && d->statements.size() == 2
                     ? as<ast::DoStmt>(d->statements[0]) : nullptr;
    auto *expr = d && d->statements.size() == 2
                     ? as<ast::ExprStmt>(d->statements[1]) : nullptr;
    auto *call = expr ? as<ast::CallExpr>(expr->value) : nullptr;
    check(r.accepted && !r.cst().recovered() && stmt && stmt->catches.size() == 1 &&
              call && call->args.size() == 6,
          "catch belongs to do, reserved argument labels stay usable");
    const char *labels[] = {"catch", "in", "where", "default", "as", "operator"};
    if (call && call->args.size() == 6)
      for (std::size_t i = 0; i < 6; ++i)
        check(call->args[i]->label == labels[i] &&
                  as<ast::IntLit>(call->args[i]->value),
              "reserved label and argument value retained");
  }


  {
    const char *inputs[] = {
        "@first @second let x = 1\n", "@first @second func f() {}\n",
        "@first @second enum E {}\n", "@first @second struct S {}\n",
        "@first @second class C {}\n", "@first @second protocol P {}\n",
        "@first @second extension C {}\n", "@first @second typealias T = Int\n",
        "@first @second import Foundation\n"};
    const cursiv::AstTypeTag *types[] = {
        &cursiv::ast_type_tag<ast::VariableDecl>, &cursiv::ast_type_tag<ast::FunctionDecl>,
        &cursiv::ast_type_tag<ast::EnumDecl>, &cursiv::ast_type_tag<ast::StructDecl>,
        &cursiv::ast_type_tag<ast::ClassDecl>, &cursiv::ast_type_tag<ast::ProtocolDecl>,
        &cursiv::ast_type_tag<ast::ExtensionDecl>, &cursiv::ast_type_tag<ast::TypealiasDecl>,
        &cursiv::ast_type_tag<ast::ImportDecl>};
    for (std::size_t i=0;i<9;++i) {
      auto r=parser->parse(inputs[i]); auto *f=as<ast::File>(r.value);
      auto *stmt=f && f->statements.size()==1?f->statements[0]:nullptr;
      auto *d=stmt && stmt->type==types[i]?static_cast<ast::Decl*>(stmt):nullptr;
      check(r.accepted && !r.cst().recovered() && d && d->attributes.size()==2,
            "common declaration attributes retain derived node and sequence");
      if(d && d->attributes.size()==2) {
        check(d->attributes[0]->nameParts.size()==1 &&
                  d->attributes[0]->nameParts[0]=="first" &&
                  d->attributes[1]->nameParts.size()==1 &&
                  d->attributes[1]->nameParts[0]=="second" &&
                  d->attributes[0]->parent==d && d->attributes[1]->parent==d,
              "inherited attributes retain values, order, and parent");
      }
    }
    auto r=parser->parse("@outer enum E { @item case a }\n");
    auto *f=as<ast::File>(r.value);
    auto *e=f && f->statements.size()==1?as<ast::EnumDecl>(f->statements[0]):nullptr;
    auto *c=e && e->members.size()==1?as<ast::EnumCaseDecl>(e->members[0]):nullptr;
    check(r.accepted && !r.cst().recovered() && e && c &&
              e->attributes.size()==1 && c->attributes.size()==1 &&
              e->attributes[0]->nameParts[0]=="outer" &&
              c->attributes[0]->nameParts[0]=="item" &&
              c->attributes[0]->parent==c && c->parent==e,
          "nested declaration attributes have independent owners");
  }


  {
    const char *conditions[][2] = {
        {"swift(>= 5.3)", ">= 5.3"},
        {"canImport(Café.Module, _version: 1.2.3)", "Café.Module, _version: 1.2.3"},
        {"compiler(<= -1..2)", "<= -1..2"},
        {"canImport(Foundation\n, _version: \n1.2.3)", "Foundation\n, _version: \n1.2.3"},
        {"canImport(Foo /* reason */ Bar)", "Foo /* reason */ Bar"},
        {"canImport(Foo Bar)", "Foo Bar"}};
    for (const auto &row : conditions) {
      const auto source="#if "+std::string(row[0])+"\n#endif\n";
      auto r=parser->parse(source);auto *f=as<ast::File>(r.value);
      auto *stmt=f && f->statements.size()==1?as<ast::IfConfigStmt>(f->statements[0]):nullptr;
      auto *c=stmt && stmt->branches.size()==1?as<ast::PlatformCondition>(stmt->branches[0]->condition):nullptr;
      check(r.accepted && !r.cst().recovered() && c && c->argument==row[1],
            "platform arguments retain exact operators, Unicode and internal trivia");
    }
  }


  {
    const char *inputs[] = {
        "@tag private(set) final var x: Int = 1\n",
        "@tag public final func f() {}\n",
        "@tag public required init() {}\n",
        "@tag public final subscript(i: Int) -> Int { get { return 1 } }\n"};
    const cursiv::AstTypeTag *types[] = {
        &cursiv::ast_type_tag<ast::VariableDecl>, &cursiv::ast_type_tag<ast::FunctionDecl>,
        &cursiv::ast_type_tag<ast::InitializerDecl>, &cursiv::ast_type_tag<ast::SubscriptDecl>};
    const char *first[] = {"private", "public", "public", "public"};
    const char *second[] = {"final", "final", "required", "final"};
    for(std::size_t i=0;i<4;++i) {
      auto r=parser->parse(inputs[i]); auto *f=as<ast::File>(r.value);
      auto *stmt=f && f->statements.size()==1?f->statements[0]:nullptr;
      auto *d=stmt && stmt->type==types[i]?static_cast<ast::Decl*>(stmt):nullptr;
      check(r.accepted && !r.cst().recovered() && d && d->attributes.size()==1 &&
                d->attributes[0]->nameParts.size()==1 &&
                d->attributes[0]->nameParts[0]=="tag" &&
                d->attributes[0]->parent==d && d->modifiers.size()==2,
            "common modifier list retains derived node and attributes");
      if(d && d->modifiers.size()==2) {
        check(d->modifiers[0]->keyword==first[i] && d->modifiers[1]->keyword==second[i] &&
                  d->modifiers[0]->parent==d && d->modifiers[1]->parent==d,
              "inherited modifiers retain values, order, and parent");
        check(i!=0 || d->modifiers[0]->qualifier=="set",
              "inherited modifier qualifier retained");
      }
    }
  }


  {
    const char *inputs[]={"final class C {}\n", "public final class C {}\n", "final public class C {}\n"};
    for(std::size_t i=0;i<3;++i){
      auto r=parser->parse(inputs[i]);auto *f=as<ast::File>(r.value);
      auto *c=f && f->statements.size()==1?as<ast::ClassDecl>(f->statements[0]):nullptr;
      check(r.accepted && !r.cst().recovered() && c && c->modifiers.size()==(i==0?1u:2u) &&
                c->modifiers[i==1?1:0]->keyword=="final" &&
                (i==0 || c->modifiers[i==1?0:1]->keyword=="public"),
            "final class flag is independent from access-level modifier");
    }
  }

  return ok;
}
} // namespace audit_regressions
