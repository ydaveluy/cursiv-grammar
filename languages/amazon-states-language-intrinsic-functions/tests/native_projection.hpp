#pragma once
#include "amazonstateslanguageintrinsicfunctions/core/ast.hpp"
#include <ostream>
#include <string>
namespace native_regressions {
namespace ns = amazonstateslanguageintrinsicfunctions;
struct Projection {
  std::ostream &out;
  explicit Projection(std::ostream &s) : out(s) {}
  void q(std::string_view s) {
    out << '"';
    for (unsigned char c : s) {
      if (c == '"' || c == '\\')
        out << '\\' << char(c);
      else if (c < 32) {
        const char *h = "0123456789abcdef";
        out << "\\u00" << h[c >> 4] << h[c & 15];
      } else
        out << char(c);
    }
    out << '"';
  }

  void arg(const ns::ast::Arg *p) {
    using namespace ns::ast;
    if (!p) {
      out << "null";
      return;
    }
    if (cursiv::ast_cast<NullArg>(p)) {
      out << "{\"kind\":\"null\"}";
      return;
    }
    if (auto *c = cursiv::ast_cast<FuncCall>(p)) {
      constexpr std::string_view names[] = {
          "Format",         "StringToJson",  "JsonToString", "Array",
          "ArrayPartition", "ArrayContains", "ArrayRange",   "ArrayGetItem",
          "ArrayLength",    "ArrayUnique",   "Base64Encode", "Base64Decode",
          "Hash",           "JsonMerge",     "MathRandom",   "MathAdd",
          "StringSplit",    "UUID"};
      out << "{\"kind\":\"call\",\"function\":";
      q(names[std::size_t(c->function)]);
      out << ",\"args\":[";
      bool f = true;
      for (auto *a : c->args) {
        if (!f)
          out << ',';
        f = false;
        arg(a);
      }
      out << "]}";
      return;
    }
    std::string_view kind, text;
    std::string numeric;
    if (auto *c = cursiv::ast_cast<StringArg>(p)) {
      kind = "string";
      text = c->value;
    } else if (auto *c = cursiv::ast_cast<JsonPathArg>(p)) {
      kind = "path";
      text = c->path;
    } else if (auto *c = cursiv::ast_cast<ContextPathArg>(p)) {
      kind = "context";
      text = c->path;
    } else if (auto *c = cursiv::ast_cast<BoolArg>(p)) {
      kind = "bool";
      text = c->value ? "true" : "false";
    } else if (auto *c = cursiv::ast_cast<VariableArg>(p)) {
      kind = "variable";
      text = c->expression;
    } else if (auto *c = cursiv::ast_cast<IntArg>(p)) {
      kind = "int";
      text = c->value;
    } else if (auto *c = cursiv::ast_cast<FloatArg>(p)) {
      kind = "float";
      text = c->value;
    }
    out << "{\"kind\":";
    q(kind);
    out << ",\"value\":";
    q(text);
    out << '}';
  }
};
} // namespace native_regressions
