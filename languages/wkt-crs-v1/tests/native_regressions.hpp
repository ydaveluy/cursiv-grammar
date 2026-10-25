#pragma once

#include <cstdio>
#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "wktcrsv1/core/CoreServices.hpp"
#include "wktcrsv1/core/ast.hpp"

namespace native_regressions {
inline bool run() {
  auto language = cursiv::create_language<wktcrsv1::WktCrsV1CoreServices,
      wktcrsv1::WktCrsV1SharedCoreServices, wktcrsv1::install_core_module>();
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "WKT regression: %s\n", message); ok = false; }
  };
  namespace ast = wktcrsv1::ast;
  const std::string source = "local_cs[\"L\",LOCAL_DATUM[\"D\",999999999999999999999999999999],UNIT[\"m\",.5],AXIS[\"X\",EAST]]";
  const auto result = language.language.parser->parse(source);
  auto* document = cursiv::ast_cast<ast::PropsFile>(result.value);
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(), "standalone case-insensitive definition");
  auto* definition = document && document->rows.size() == 1 ? cursiv::ast_cast<ast::Definition>(document->rows[0]) : nullptr;
  check(definition && definition->wkt && definition->wkt->keyword == "local_cs", "source keyword retained");
  if (definition && definition->wkt && definition->wkt->arguments.size() == 4) {
    auto* datum = cursiv::ast_cast<ast::Clause>(definition->wkt->arguments[1]);
    auto* number = datum && datum->arguments.size() == 2 ? cursiv::ast_cast<ast::Number>(datum->arguments[1]) : nullptr;
    check(number && number->literal == "999999999999999999999999999999", "unbounded integer literal retained");
  } else check(false, "ordered arguments retained");
  const std::string properties = "CUSTOM:WGS.84=LOCAL_CS[\"first\"]\n0004326=LOCAL_CS[\"second\"]\n";
  const auto mapped = language.language.parser->parse(properties);
  auto* rows = cursiv::ast_cast<ast::PropsFile>(mapped.value);
  check(mapped.accepted && !mapped.cst().recovered() && rows && rows->rows.size() == 2, "mapped definitions");
  if (rows && rows->rows.size() == 2) {
    auto* first = cursiv::ast_cast<ast::EpsgDefLine>(rows->rows[0]);
    auto* second = cursiv::ast_cast<ast::EpsgDefLine>(rows->rows[1]);
    check(first && second && first->code == "CUSTOM:WGS.84" && second->code == "0004326", "property code text retained");
  }
  const std::string esri = "GEOGCS[\"G\",DATUM[\"D\",SPHEROID[\"S\",6378137,298.257223563]],PRIMEM[\"G\",0],UNIT[\"degree\",1]],VERTCS[\"H\",VDATUM[\"V\"],UNIT[\"m\",1]]";
  const auto esriParsed = language.language.parser->parse(esri);
  auto* esriRoot = cursiv::ast_cast<ast::PropsFile>(esriParsed.value);
  auto* esriDefinition = esriRoot && esriRoot->rows.size() == 1 ? cursiv::ast_cast<ast::Definition>(esriRoot->rows[0]) : nullptr;
  check(esriParsed.accepted && !esriParsed.cst().recovered() && esriDefinition &&
        esriDefinition->vertical && esriDefinition->vertical->keyword == "VERTCS", "ESRI implicit compound retains both systems");
  for (const char* text : {
      "LOCAL_CS[\"L\"]",
      "VERT_CS[\"height\",VERT_DATUM[\"D\",2005],UNIT[\"m\",1]]",
      "GEOCCS[\"G\",DATUM[\"D\",SPHEROID[\"S\",6378137,298.257223563]],PRIMEM[\"G\",0],UNIT[\"m\",1]]",
      "FITTED_CS[\"F\",INVERSE_MT[PARAM_MT[\"Affine\"]],LOCAL_CS[\"L\"]]"}) {
    const auto parsed = language.language.parser->parse(text);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(), "additional WKT1 shapes");
  }
  for (const auto& [text, code] : {
      std::pair{"LOCAL_CS[\"L\",LOCAL_DATUM[\"D\",0],UNIT[\"m\",1e99999],AXIS[\"X\",EAST]]", "wkt.number"},
      std::pair{"LOCAL_CS[\"L\",LOCAL_DATUM[\"D\",0],UNIT[\"m\",1],AXIS[\"X\",SIDEWAYS]]", "wkt.axis"},
      std::pair{"LOCAL_CS[]", "wkt.arguments"},
      std::pair{"LOCAL_CS[\"L\",]", "wkt.arguments"},
      std::pair{"GEOGCS[\"G\",DATUM[\"D\",SPHEROID[\"S\",1,0],TOWGS84[1,2,3,4,5,6]],PRIMEM[\"G\",0],UNIT[\"degree\",1]]", "wkt.arguments"}}) {
    const auto parsed = language.language.parser->parse(text);
    check(parsed.accepted && !parsed.cst().recovered(), "invalid values remain parseable");
    const auto errors = cursiv::test::validationHelper(language.language, text);
    bool found = false;
    for (const auto& d : errors.diagnostics) if (d.code && d.code->isString() && d.code->string() == code) found = true;
    check(found, code);
  }
  return ok;
}
}  // namespace native_regressions
