#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "webidl/core/ast.hpp"

inline bool native_regressions(const auto &services) {
  namespace ast = webidl::ast;
  bool ok = true;
  const auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "webidl: %s\n", message);
      ok = false;
    }
  };
  const auto parsed = services.parser->parse(R"idl(
    [A=*, B=123, C=-1.5, D=(1, 2), E=(1.0, 2e1), F="", G=("x", ""),
     H=(_A,_B), I=Make(optional long n = 0), J()]
    interface long-name {
      attribute SharedArrayBuffer _attribute;
      attribute long-long -will-change;
      async_iterable<DOMString>(optional long n = 3);
      async iterable<long, DOMString>;
      void f(async_sequence<long> items);
      Promise<void> g();
    };
    callback _callback = undefined (BigInt64Array _interface);
  )idl");
  check(parsed.accepted && !parsed.cst().recovered() &&
            parsed.diagnostics.empty(),
        "native syntax parses without recovery");
  const auto *root = cursiv::ast_cast<ast::Document>(parsed.value);
  check(root && root->definitions.size() == 2, "both definitions retained");
  if (root && root->definitions.size() == 2) {
    const auto *iface = cursiv::ast_cast<ast::Interface>(root->definitions[0]);
    check(iface && iface->name == "long-name" && iface->attrs.size() == 10 &&
              iface->members.size() == 6,
          "interface fields");
    if (iface && iface->attrs.size() == 10 && iface->members.size() == 6) {
      const auto &attrs = iface->attrs;
      const auto *integer = cursiv::ast_cast<ast::IntValue>(attrs[1]->rhs);
      const auto *decimal = cursiv::ast_cast<ast::FloatValue>(attrs[2]->rhs);
      const auto *empty = cursiv::ast_cast<ast::StringValue>(attrs[5]->rhs);
      check(attrs[0]->wildcard && integer && integer->text == "123" &&
                decimal && decimal->text == "-1.5" && empty &&
                empty->text.empty(),
            "wildcard, exact numbers and empty string remain distinct");
      check(attrs[3]->hasList && attrs[3]->values.size() == 2 &&
                attrs[4]->values.size() == 2 && attrs[6]->values.size() == 2 &&
                attrs[7]->values.size() == 2,
            "typed extended attribute lists");
      const auto *escaped =
          attrs[7]->values.empty()
              ? nullptr
              : cursiv::ast_cast<ast::IdentifierValue>(attrs[7]->values[0]);
      check(escaped && escaped->text == "A" && attrs[8]->hasArguments &&
                attrs[8]->args.size() == 1 && attrs[9]->hasArguments &&
                attrs[9]->args.empty(),
            "escaped values and empty argument list");
      const auto *attribute =
          cursiv::ast_cast<ast::Attribute>(iface->members[0]);
      check(attribute && attribute->name == "attribute" &&
                cursiv::ast_cast<ast::BufferType>(attribute->type),
            "escaped attribute name");
      const auto *modern =
          cursiv::ast_cast<ast::AsyncIterable>(iface->members[2]);
      const auto *legacy =
          cursiv::ast_cast<ast::AsyncIterable>(iface->members[3]);
      check(modern && !modern->legacySyntax && modern->params.size() == 1 &&
                legacy && legacy->legacySyntax && legacy->valueType,
            "modern and legacy asynchronous iterable spelling");
      const auto *operation =
          cursiv::ast_cast<ast::Operation>(iface->members[4]);
      const auto *returnType =
          operation
              ? cursiv::ast_cast<ast::PrimitiveType>(operation->returnType)
              : nullptr;
      const auto *sequence =
          operation && operation->params.size() == 1
              ? cursiv::ast_cast<ast::SequenceType>(operation->params[0]->type)
              : nullptr;
      check(returnType && returnType->kind == ast::Primitive::void_ &&
                sequence && sequence->async_,
            "void and async_sequence are preserved");
    }
    const auto *callback =
        cursiv::ast_cast<ast::Callback>(root->definitions[1]);
    const auto *returned =
        callback ? cursiv::ast_cast<ast::PrimitiveType>(callback->returnType)
                 : nullptr;
    check(callback && callback->name == "callback" && returned &&
              returned->kind == ast::Primitive::undefined_,
          "undefined differs from void");
  }
  struct Case {
    const char *text;
    const char *message;
  };
  for (const auto &c :
       {Case{"dictionary X { required long x = 2; };", "required dictionary"},
        {"interface X { undefined f(optional long... values); };",
         "variadic argument"},
        {"interface X { undefined f(long value = 1); };", "optional argument"},
        {"partial interface X : Base {};", "partial interface"},
        {"partial dictionary X : Base {};", "partial dictionary"},
        {"namespace X { attribute long x; };", "namespace accepts"},
        {"interface mixin X { getter long f(); };", "interface mixin"},
        {"callback interface X { attribute long x; };", "callback interface"},
        {"typedef any? X;", "cannot be nullable"},
        {"typedef Promise<long>? X;", "cannot be nullable"},
        {"typedef (Promise<long> or long) X;", "cannot contain"},
        {"typedef (any or long) X;", "cannot contain"},
        {"typedef (long) X;", "at least two"},
        {"interface X { inherit readonly attribute long x; };",
         "inherited attribute"},
        {"typedef void X;", "only allowed as a return"},
        {"interface _toString {};", "identifier is reserved"},
        {"interface long {};", "Escape this keyword"},
        {"interface __Name {};", "identifier needs a letter"},
        {"[A=()] interface X {};", "list cannot be empty"},
        {"[A=(1, 1.5)] interface X {};", "one kind"},
        {"typedef Promise<[A] long> X;", "Promise subtype"},
        {"typedef record<[A] DOMString, long> X;", "record key"}}) {
    const auto p = services.parser->parse(c.text);
    check(p.accepted && !p.cst().recovered() && p.diagnostics.empty(),
          "invalid form reaches runtime validation");
    const auto result = cursiv::test::validationHelper(services, c.text);
    check(result.diagnostics.size() == 1 && result.diagnostics[0].message.find(
                                                c.message) != std::string::npos,
          c.message);
  }
  for (std::string text : {"interface _interface { attribute long required; "
                           "undefined includes(long async); };",
                           "[A=123(long x), B=*, C=\"\"] interface X {};",
                           "interface X { void f(); Promise<void> g(); };",
                           "interface X { async_iterable<DOMString>; };",
                           "interface X { const long N = "
                           "999999999999999999999999999999999999999; };"})
    check(cursiv::test::validationHelper(services, text).diagnostics.empty(),
          "valid native syntax has no structural validation error");
  const std::string bad =
      "interface X { undefined f(optional long... values); };";
  const auto precise = cursiv::test::validationHelper(services, bad);
  check(precise.diagnostics.size() == 1 &&
            precise.diagnostics[0].range.begin == bad.find("...") &&
            precise.diagnostics[0].range.end == bad.find("...") + 3,
        "variadic error selects the ellipsis");
  return ok;
}
