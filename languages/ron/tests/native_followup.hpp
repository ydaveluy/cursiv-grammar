#pragma once

#include "cursiv/core/parser/Parser.hpp"
#include "ron/core/ast.hpp"
#include <cstdio>

inline bool native_followup(const cursiv::Parser &parser) {
  namespace ast = ron::ast;
  const auto clean = [&](const auto &result) {
    return result.accepted && !result.cst().recovered() && result.diagnostics.empty();
  };
  struct Case { const char *text, *start, *end; bool inclusive; };
  for (const auto &c : {Case{"3..5", "3", "5", false},
                       {"-2.1..=7.3", "-2.1", "7.3", true},
                       {"..-15", nullptr, "-15", false},
                       {"..=13", nullptr, "13", true},
                       {"3..", "3", nullptr, false},
                       {"..", nullptr, nullptr, false},
                       {"1...2", "1", ".2", false},
                       {"b'A'..b'Z'", "b'A'", "b'Z'", false}}) {
    const auto r = parser.parse(c.text);
    const auto *doc = r.value_as<ast::Document>();
    const auto *held = doc && doc->values.size() == 1
                           ? std::get_if<ast::Range *>(&doc->values[0]->payload)
                           : nullptr;
    const auto *range = held ? *held : nullptr;
    if (!clean(r) || !range || range->inclusive != c.inclusive ||
        bool(range->start) != bool(c.start) || bool(range->end) != bool(c.end) ||
        (c.start && (range->start->text != c.start || range->start->parent != range)) ||
        (c.end && (range->end->text != c.end || range->end->parent != range)) ||
        range->parent != doc->values[0]) {
      std::fprintf(stderr, "ron range AST: %s\n", c.text);
      return false;
    }
  }
  for (const char *text : {"1_", "1__2", "1e_2", "1f32", "0xFFu16",
                           "[1..2, ..=3, ..]", "_", "NaNi8",
                           "#![type=\"SomeType\"]\nSomeType(a:1)",
                           "#![schema=\"url\"]\ntrue",
                           "# ! [ enable ( implicit_some ) ]\nSome(1)"})
    if (!clean(parser.parse(text))) {
      std::fprintf(stderr, "ron native form: %s\n", text);
      return false;
    }
  for (const char *text : {"0XFF", "0b_1", "0x_1", "0x___", "1._2", "1e_",
                           "1u123", "1.2u8", "3..=", "#![enable()]\ntrue",
                           "#![unknown(foo)]\ntrue", "#![enable(unknown)]\ntrue"})
    if (clean(parser.parse(text))) {
      std::fprintf(stderr, "ron invalid native form accepted: %s\n", text);
      return false;
    }
  return true;
}
