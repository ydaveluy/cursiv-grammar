#pragma once
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "smtlibv2/core/CoreServices.hpp"
#include "smtlibv2/generated/Smtlibv2/Module.hpp"
namespace code_audit {
namespace ast = smtlibv2::ast;
inline bool run() {
  auto language = cursiv::create_language<smtlibv2::Smtlibv2CoreServices,
                                          smtlibv2::Smtlibv2SharedCoreServices,
                                          smtlibv2::install_core_module>();
  auto reflection = smtlibv2::generated::Smtlibv2::create_ast_reflection();
  bool ok = true;
  auto check = [&](bool b, const char* s) {
    if (!b) {
      std::fprintf(stderr, "smtlibv2 code audit: %s\n", s);
      ok = false;
    }
  };
  const auto visit = [&](const cursiv::AstNode* root, auto&& callback) {
    std::function<void(const cursiv::AstNode*)> walk = [&](const auto* n) {
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
  const auto validate = [&](std::string_view source) {
    cursiv::Document doc(cursiv::TextDocument::create(
        "memory:smtlibv2", "smtlibv2", 0, std::string(source)));
    doc.parseResult = language.language.parser->parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    visit(doc.parseResult.value, [&](const auto* n) {
      if (const auto* x = cursiv::ast_cast<ast::SetOptionCommand>(n))
        language.language.validator->checkOption(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::AppTerm>(n))
        language.language.validator->checkApplication(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::LetTerm>(n))
        language.language.validator->checkLet(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::QuantTerm>(n))
        language.language.validator->checkQuantifier(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::MatchTerm>(n))
        language.language.validator->checkMatch(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::AnnotTerm>(n))
        language.language.validator->checkAnnotation(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::DefineFunsRecCommand>(n))
        language.language.validator->checkRecursive(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::DeclareDatatypesCommand>(n))
        language.language.validator->checkDatatypes(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::DatatypeDec>(n))
        language.language.validator->checkDatatype(*x, accept);
      if (const auto* x = cursiv::ast_cast<ast::FunctionDef>(n))
        language.language.validator->checkFunction(*x, accept);
    });
    return errors;
  };
  const auto parse = [&](std::string_view source) {
    auto r = language.language.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
    visit(r.value, [](const auto*) {});
    return r;
  };
  const char* const cases[] = {
      R"smt((declare-const a (Array Int (_ BitVec 8)))
(assert (= ((_ extract 3 0) (select (store a 0 #xff) 0)) #b1111))
)smt",
      R"smt((declare-const a Int)
; ignored
(assert (= a 2))
(assert (= a 1))
)smt",
      R"smt((declare-datatype L ((nil) (cons (head Int) (tail L))))
(declare-const x L)
(assert (= (match x ((nil 0) ((cons h t) h))) 2))
)smt",
      R"smt((assert (= 1.000000000000001 2.00000))
)smt",
      R"smt((assert (let () true))
)smt",
      R"smt((declare-const || Int)
(assert (= || 4))
)smt",
      R"smt((declare-const assert Int)
(declare-const check-sat Int)
(assert (= assert check-sat))
)smt",
      R"smt((declare-const x Int)
(assert (let ((x 1) (y x)) (= x y)))
)smt",
      R"smt((define-funs-rec ((even ((n Int)) Bool) (odd ((n Int)) Bool)) ((ite (= n 0) true (odd (- n 1))) (ite (= n 0) false (even (- n 1)))))
(assert (even 4))
)smt",
      R"smt((set-option :produce-models true)
(set-option :random-seed 123)
(set-option :regular-output-channel "stdout")
(assert true)
)smt",
      R"smt((declare-datatypes ((MyList 1)) ((par (T) ((nil) (cons (head T) (tail (MyList T)))))))
(declare-const x (MyList Int))
(assert (= x (cons 1 (as nil (MyList Int)))))
)smt",
      R"smt((declare-fun f (Int) Int)
(assert (forall ((x Int)) (! (exists ((y Int)) (= (f x) y)) :pattern ((f x)) :qid test)))
)smt",
      R"smt((declare-const |odd name
line| Int)
(assert (= |odd name
line| 123456789012345678901234567890))
)smt",
      R"smt((define-fun-rec fac ((n Int)) Int (ite (= n 0) 1 (* n (fac (- n 1)))))
(assert (= (fac 3) 6))
)smt",
      R"smt((assert (= "123" "he said ""hi"""))
(assert (= "" "\u{1f600}"))
)smt",
  };
  for (const char* source : cases) {
    const auto r = parse(source);
    check(r.value_as<ast::Script>(), "script root");
    check(validate(source) == 0, "native valid script validates");
  }
  {
    const auto r = parse("(assert (= \"123\" \"123\")) (assert (= 123 123))");
    std::vector<const ast::SpecConstant*> values;
    visit(r.value, [&](const auto* n) {
      if (const auto* x = cursiv::ast_cast<ast::SpecConstant>(n))
        values.push_back(x);
    });
    check(values.size() == 4, "four constants");
    if (values.size() == 4)
      check(values[0]->isString && values[1]->isString &&
                !values[2]->isString && !values[3]->isString &&
                values[0]->text == values[2]->text,
            "string and numeral remain distinct");
  }
  {
    const auto r = parse(
        "(set-option :produce-models true) (set-option :verbosity 123) "
        "(set-option :regular-output-channel \"stdout\")");
    const auto* d = r.value_as<ast::Script>();
    check(d && d->commands.size() == 3, "three options");
    if (d && d->commands.size() == 3) {
      const auto* a = cursiv::ast_cast<ast::SetOptionCommand>(d->commands[0]);
      const auto* b = cursiv::ast_cast<ast::SetOptionCommand>(d->commands[1]);
      const auto* c = cursiv::ast_cast<ast::SetOptionCommand>(d->commands[2]);
      const auto* flag = a && a->option
                             ? std::get_if<std::string_view>(&a->option->value)
                             : nullptr;
      const auto* number =
          b && b->option ? std::get_if<ast::SpecConstant*>(&b->option->value)
                         : nullptr;
      const auto* text =
          c && c->option ? std::get_if<ast::SpecConstant*>(&c->option->value)
                         : nullptr;
      check(flag && *flag == "true" && number && *number &&
                !(*number)->isString && (*number)->text == "123" && text &&
                *text && (*text)->isString && (*text)->text == "stdout",
            "uniform attribute values");
    }
  }
  {
    const auto r = parse(
        "(declare-const a Int)\n; ignored\r(assert (= a 2))\n(assert (= a "
        "1))\n");
    const auto* d = r.value_as<ast::Script>();
    check(d && d->commands.size() == 2, "carriage return remains in comment");
  }
  {
    const auto r = parse(R"smt(sat
((x 5))
(
  (define-fun x () Int
    5)
)
(:name "Z3")
true
)smt");
    const auto* d = r.value_as<ast::ResponseDoc>();
    check(d && d->responses.size() == 5, "five native solver responses");
    if (d && d->responses.size() == 5) {
      const auto* sat =
          cursiv::ast_cast<ast::CheckSatResponse>(d->responses[0]);
      const auto* value = cursiv::ast_cast<ast::SExprResponse>(d->responses[1]);
      const auto* model = cursiv::ast_cast<ast::SExprResponse>(d->responses[2]);
      const auto* info = cursiv::ast_cast<ast::SExprResponse>(d->responses[3]);
      const auto* option =
          cursiv::ast_cast<ast::SExprResponse>(d->responses[4]);
      check(sat && sat->result == "sat" && value && model && info && option,
            "native response forms");
      const auto* outer =
          value ? std::get_if<ast::SExprList*>(&value->value) : nullptr;
      const auto* pair = outer && *outer && (*outer)->items.size() == 1
                             ? std::get_if<ast::SExprList*>(&(*outer)->items[0])
                             : nullptr;
      check(pair && *pair && (*pair)->items.size() == 2,
            "native get-value pair");
      if (pair && *pair && (*pair)->items.size() == 2) {
        const auto* name = std::get_if<std::string_view>(&(*pair)->items[0]);
        const auto* v = std::get_if<ast::SpecConstant*>(&(*pair)->items[1]);
        check(name && *name == "x" && v && *v && !(*v)->isString &&
                  (*v)->text == "5",
              "native get-value fields");
      }
      const auto* values =
          info ? std::get_if<ast::SExprList*>(&info->value) : nullptr;
      if (values && *values && (*values)->items.size() == 2) {
        const auto* name = std::get_if<std::string_view>(&(*values)->items[0]);
        const auto* v = std::get_if<ast::SpecConstant*>(&(*values)->items[1]);
        check(name && *name == ":name" && v && *v && (*v)->isString &&
                  (*v)->text == "Z3",
              "native info values");
      } else
        check(false, "native info pair");
      const auto* flag =
          option ? std::get_if<std::string_view>(&option->value) : nullptr;
      check(flag && *flag == "true", "native boolean option response");
    }
  }
  {
    const auto r = parse(
        "success\nunsupported\n(error \"bad \"\"thing\"\"\")\n\"hello\"\n()\n");
    const auto* d = r.value_as<ast::ResponseDoc>();
    check(d && d->responses.size() == 5, "general response stream");
    if (d && d->responses.size() == 5) {
      check(cursiv::ast_cast<ast::SuccessResponse>(d->responses[0]) &&
                cursiv::ast_cast<ast::UnsupportedResponse>(d->responses[1]),
            "general response tags");
      const auto* e = cursiv::ast_cast<ast::ErrorResponse>(d->responses[2]);
      const auto* text = cursiv::ast_cast<ast::EchoResponse>(d->responses[3]);
      check(e && e->message == "bad \"thing\"" && text && text->text == "hello",
            "general response strings");
    }
  }
  {
    const auto r = parse("(push) (pop) (push 2) (pop 2)");
    const auto* d = r.value_as<ast::Script>();
    check(d && d->commands.size() == 4, "default stack commands");
    if (d && d->commands.size() == 4) {
      const auto* a = cursiv::ast_cast<ast::PushCommand>(d->commands[0]);
      const auto* b = cursiv::ast_cast<ast::PopCommand>(d->commands[1]);
      const auto* c = cursiv::ast_cast<ast::PushCommand>(d->commands[2]);
      const auto* e = cursiv::ast_cast<ast::PopCommand>(d->commands[3]);
      check(a && b && c && e && a->count == "1" && b->count == "1" &&
                c->count == "2" && e->count == "2",
            "default and explicit stack counts");
    }
  }
  const char* invalid = R"smt(
(set-option :produce-models "true")
(set-option :verbosity 1.5)
(set-option :regular-output-channel stdout)
(set-option :custom)
(assert (forall () true))
(assert (f))
(assert (! true))
(assert (match x ()))
(define-funs-rec ((f () Bool)) ())
(declare-datatype E ())
(declare-datatypes ((F 2)) ((par (T) ((mk (field T))))))
(assert (let ((x 1) (x 2)) (= x 2)))
(define-fun f ((x Int) (x Int)) Int x)
)smt";
  const auto r = parse(invalid);
  const auto errors = validate(invalid);
  if (errors != 13)
    std::fprintf(stderr, "smtlibv2 validation errors: %u\n", errors);
  check(errors == 13, "precise runtime errors after permissive parse");
  return ok;
}
}  // namespace code_audit
