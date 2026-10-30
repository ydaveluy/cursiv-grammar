#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "smalltalk/core/ast.hpp"
#include "smalltalk/generated/Smalltalk/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = smalltalk::generated::Smalltalk::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = smalltalk::ast;
  auto r = parser->parse("'can''t'. $z");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "smalltalk valid syntax regression");
  auto *d = as<ast::Sequence>(r.value);
  check(d && d->statements.size() == 2 &&
            as<ast::StringLiteral>(d->statements[0]) &&
            as<ast::StringLiteral>(d->statements[0])->value == "can't" &&
            as<ast::CharLiteral>(d->statements[1]) &&
            as<ast::CharLiteral>(d->statements[1])->value == "z",
        "decoded string and character AST");

  {
    auto result = parser->parse("|| 1e3. -2.5e-2. 16rFF. #(true nil self super). 1 | 2. {1. 2.}. #{'a' -> 1.}");
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "Amber numeric exponents, pipe selector, empty temps and trailing array periods");
    auto* sequence = as<ast::Sequence>(result.value);
    check(sequence && sequence->temps.empty() && sequence->statements.size() == 7,
          "native forms retain all seven expression nodes");
    if (sequence && sequence->statements.size() == 7) {
      auto* exponent = as<ast::NumberLiteral>(sequence->statements[0]);
      auto* negative = as<ast::NumberLiteral>(sequence->statements[1]);
      auto* hexadecimal = as<ast::NumberLiteral>(sequence->statements[2]);
      auto* literal = as<ast::LiteralArray>(sequence->statements[3]);
      auto* send = as<ast::MessageSend>(sequence->statements[4]);
      auto* array = as<ast::DynamicArray>(sequence->statements[5]);
      auto* dictionary = as<ast::DynamicDictionary>(sequence->statements[6]);
      check(exponent && exponent->value == 1000.0 && negative &&
                negative->value == -0.025 && hexadecimal && hexadecimal->value == 255.0,
            "numeric converters preserve exponent signs and radix values");
      check(literal && literal->items.size() == 4 &&
                as<ast::PseudoVariable>(literal->items[0]) &&
                as<ast::PseudoVariable>(literal->items[1]) &&
                as<ast::SymbolLiteral>(literal->items[2]) &&
                as<ast::SymbolLiteral>(literal->items[2])->name == "self" &&
                as<ast::SymbolLiteral>(literal->items[3]) &&
                as<ast::SymbolLiteral>(literal->items[3])->name == "super",
            "literal array constants and receiver names have different AST kinds");
      check(send && send->selectorParts.size() == 1 && send->selectorParts[0] == "|" &&
                send->args.size() == 1 && array && array->items.size() == 2 &&
                dictionary && dictionary->items.size() == 1,
            "pipe message and array/dictionary contents retained");
    }
  }

  for (const auto source : {"[:x]", "[:x :y]"}) {
    const auto result = parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "Amber rejects unterminated block parameters");
  }
  return ok;
}
} // namespace audit_regressions
