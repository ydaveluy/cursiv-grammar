#pragma once
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "hexpat/core/CoreServices.hpp"
#include "hexpat/core/ast.hpp"

namespace hexpat_native_followup {
inline bool run(const hexpat::HexpatCoreServices& services) {
  bool ok = true;
  auto check = [&](bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "hexpat native: %s\n", message); ok = false; }
  };
  const std::string source = R"hexpat(
namespace Audit::Inner {
  using Forward<T>;
  using Alias<T, auto N> = T [[sealed]];
  using Ref = ref be u24;
  u48 placed @ (4 + 8) in (1 + 2);
  str name = typenameof(char16);
  u96 count = 1'000u;
  double scale = 12f;
  str escaped = "\u0041\U0001F600\xFF";
  fn defaults(u32, ref be u24 value = 3, auto ...rest) {
    u8 data[4];
    return value;
  };
}
)hexpat";
  const auto result = services.parser->parse(source);
  const auto* document = result.value_as<hexpat::ast::Document>();
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
        document && document->items.size() == 1, "native declaration document parses clean");
  const auto* space = document && document->items.size() == 1
      ? cursiv::ast_cast<hexpat::ast::Namespace>(document->items[0]) : nullptr;
  check(space && space->name == "Audit::Inner", "qualified namespace spelling");
  bool forward = false, alias = false, ref = false, placement = false;
  bool typenameofPrimitive = false, array = false, parameters = false, integer = false, floating = false;
  for (const auto* node : result.nodes) {
    if (const auto* use = cursiv::ast_cast<hexpat::ast::UsingAlias>(node)) {
      forward |= use->name == "Forward" && use->typeParams.size() == 1 && !use->type;
      alias |= use->name == "Alias" && use->typeParams.size() == 2 &&
               use->typeParams[1]->isAuto && use->type && use->type->name == "T" && use->attr;
      ref |= use->name == "Ref" && use->type && use->isRef &&
             use->endian == "be" && use->type->name == "u24";
    }
    if (const auto* at = cursiv::ast_cast<hexpat::ast::At>(node))
      placement |= cursiv::ast_cast<hexpat::ast::BinaryExpr>(at->expr) &&
                   cursiv::ast_cast<hexpat::ast::BinaryExpr>(at->section);
    if (const auto* type = cursiv::ast_cast<hexpat::ast::TypenameofExpr>(node))
      typenameofPrimitive |= type->primType == "char16" && !type->operand;
    if (const auto* value = cursiv::ast_cast<hexpat::ast::IntLit>(node)) integer |= value->text == "1'000u";
    if (const auto* value = cursiv::ast_cast<hexpat::ast::FloatLit>(node)) floating |= value->text == "12f";
    if (const auto* local = cursiv::ast_cast<hexpat::ast::VarDeclarator>(node))
      array |= local->name == "data" && local->isArray && local->size;
    if (const auto* fn = cursiv::ast_cast<hexpat::ast::Fn>(node))
      parameters |= fn->params.size() == 3 && fn->params[0]->name.empty() &&
                    fn->params[1]->isRef && fn->params[1]->endian == "be" &&
                    fn->params[1]->defaultValue && fn->params[2]->isPack &&
                    fn->params[2]->name == "rest";
  }
  check(forward && alias && ref && placement && typenameofPrimitive && array && parameters &&
        integer && floating, "declaration, literal and parameter fields survive in the AST");
  for (const auto& [text, code] : {
      std::pair{"fn f(u32 x = 1, u32 y) {};", "hexpat.default-order"},
      std::pair{"fn f(auto ...rest, u32 x) {};", "hexpat.parameter-pack"},
      std::pair{"fn f(u32 ...rest) {};", "hexpat.parameter-pack"},
      std::pair{"str s = \"\\uD800\";", "hexpat.literal"},
      std::pair{"char c = '\\u00E9';", "hexpat.literal"}}) {
    cursiv::Document checked(cursiv::TextDocument::create("memory:hexpat", "hexpat", 0, text));
    checked.language = &services;
    checked.parseResult = services.parser->parse(text);
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(checked, diagnostics);
    check(checked.parseResult.accepted && !checked.parseResult.cst().recovered() &&
          checked.parseResult.diagnostics.empty(), "native restriction is diagnosed after structural parsing");
    check(diagnostics.size() == 1 && diagnostics[0].code && diagnostics[0].code->isString() &&
          diagnostics[0].code->string() == code, "native restriction diagnostic");
  }
  for (const char* text : {"str s = \"\\q\";", "str s = \"a\nb\";", "char c = '\\xG0';"}) {
    const auto invalid = services.parser->parse(text);
    check(!invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty(),
          "malformed native literal must not parse clean");
  }
  return ok;
}
} // namespace hexpat_native_followup
