#include "audit_regressions.hpp"
#include "native_regressions.hpp"
// The grammar itself: every corpus and coverage document parses clean, and
// a sample of real field values comes back through the converters that
// built them.

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "wkt/core/ast.hpp"
#include "wkt/core/Numbers.hpp"
#include "wkt/generated/Wkt/Module.hpp"

namespace {

namespace ast = wkt::ast;
namespace fs = std::filesystem;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      wkt::generated::Wkt::create_parser();
  return *p;
}

cursiv::ParseResult parse(std::string_view text) {
  return engine().parse(text);
}

ast::Document* document(cursiv::ParseResult& r) {
  if (r.value == nullptr ||
      r.value->type != &cursiv::ast_type_tag<ast::Document>)
    return nullptr;
  return static_cast<ast::Document*>(r.value);
}

void check_clean(const fs::path& path) {
  const std::string text = cursiv::test::read_file(path.string());
  cursiv::ParseResult r = parse(text);
  CHECK(r.accepted);
  CHECK(!r.cst().recovered());
  if (!r.diagnostics.empty())
    std::printf("FAIL %s: unexpected diagnostic '%s'\n",
                path.filename().string().c_str(),
                r.diagnostics[0].message.c_str());
  CHECK(r.diagnostics.empty());
  CHECK(document(r) != nullptr);
}

// example/ is the upstream corpus: every document in it is meant to come
// out clean.
void corpus_parses_clean() {
  const fs::path exampleDir = fs::path(CURSIV_EXAMPLE_DIR) / "example";
  int seen = 0;
  for (const fs::directory_entry& e : fs::directory_iterator(exampleDir)) {
    if (!e.is_regular_file() || e.path().extension() != ".wkt") continue;
    ++seen;
    check_clean(e.path());
  }
  CHECK(seen == 5);
}

// This port's own coverage documents: every geometry kind in the Z/M/ZM/
// EMPTY forms the corpus itself leaves untried.
void coverage_parses_clean() {
  const fs::path coverageDir = fs::path(CURSIV_TEST_DIR) / "coverage";
  int seen = 0;
  for (const fs::directory_entry& e : fs::directory_iterator(coverageDir)) {
    if (!e.is_regular_file() || e.path().extension() != ".wkt") continue;
    ++seen;
    check_clean(e.path());
  }
  CHECK(seen == 4);
}

// A converter that silently returns the wrong value passes every
// structural check, so ordinates are read back rather than only counted --
// including the two ways EMPTY reads (a null Point*, and a zero-length
// PointList*) and the two special-value ordinates.
void point_ordinates_read_back() {
  {
    cursiv::ParseResult r = parse("POINT (1 2)");
    ast::Document* d = document(r);
    CHECK(d != nullptr && d->geometries.size() == 1);
    if (d == nullptr || d->geometries.size() != 1) return;
    auto* g = static_cast<ast::PointGeom*>(d->geometries[0]);
    CHECK(g->dim == ast::Dim::none);
    CHECK(g->value != nullptr && g->value->ordinates.size() == 2 && g->value->ordinates[0] == "1" && g->value->ordinates[1] == "2");
  }
  {
    cursiv::ParseResult r = parse("POINT ZM (1 2 3 4)");
    ast::Document* d = document(r);
    CHECK(d != nullptr && d->geometries.size() == 1);
    if (d == nullptr || d->geometries.size() != 1) return;
    auto* g = static_cast<ast::PointGeom*>(d->geometries[0]);
    CHECK(g->dim == ast::Dim::zm);
    CHECK(g->value != nullptr && g->value->ordinates.size() == 4 &&
          g->value->ordinates[2] == "3" && g->value->ordinates[3] == "4");
  }
  {
    cursiv::ParseResult r = parse("POINT Z EMPTY");
    ast::Document* d = document(r);
    CHECK(d != nullptr && d->geometries.size() == 1);
    if (d == nullptr || d->geometries.size() != 1) return;
    auto* g = static_cast<ast::PointGeom*>(d->geometries[0]);
    CHECK(g->dim == ast::Dim::z && g->value == nullptr);
  }
  {
    cursiv::ParseResult r = parse("LINESTRING EMPTY");
    ast::Document* d = document(r);
    CHECK(d != nullptr && d->geometries.size() == 1);
    if (d == nullptr || d->geometries.size() != 1) return;
    auto* g = static_cast<ast::LineStringGeom*>(d->geometries[0]);
    CHECK(g->points.empty());
  }
  {
    cursiv::ParseResult r = parse("POINT (Inf -Inf)");
    ast::Document* d = document(r);
    CHECK(d != nullptr && d->geometries.size() == 1);
    if (d == nullptr || d->geometries.size() != 1) return;
    auto* g = static_cast<ast::PointGeom*>(d->geometries[0]);
    CHECK(g->value != nullptr && *wkt::ordinate_value(g->value->ordinates[0]) == INFINITY &&
          *wkt::ordinate_value(g->value->ordinates[1]) == -INFINITY);
  }
  {
    cursiv::ParseResult r = parse("LINESTRING (1 NaN, NaN 2)");
    ast::Document* d = document(r);
    CHECK(d != nullptr && d->geometries.size() == 1);
    if (d == nullptr || d->geometries.size() != 1) return;
    auto* g = static_cast<ast::LineStringGeom*>(d->geometries[0]);
    CHECK(g->points.size() == 2 && std::isnan(*wkt::ordinate_value(g->points[0]->ordinates[1])) &&
          std::isnan(*wkt::ordinate_value(g->points[1]->ordinates[0])));
  }
}

// The ISO 13249-3 placement form and the JTS (startKappa, endKappa, length)
// extension share the CLOTHOID keyword and the ClothoidGeom class; told
// apart here by which group of fields came back non-empty.
void clothoid_two_forms_read_back() {
  {
    cursiv::ParseResult r = parse(
        "CLOTHOID (REFERENCELOCATION AFFINEPLACEMENT (LOCATION (0 0), "
        "REFERENCEDIRECTIONS (VECTOR (1 0), VECTOR (0 1))), SCALEFACTOR "
        "100, STARTDISTANCE 0, ENDDISTANCE 50)");
    ast::Document* d = document(r);
    CHECK(d != nullptr && d->geometries.size() == 1);
    if (d == nullptr || d->geometries.size() != 1) return;
    auto* g = static_cast<ast::ClothoidGeom*>(d->geometries[0]);
    CHECK(g->referenceLocation != nullptr);
    CHECK(g->scaleFactor.has_value() && *g->scaleFactor == 100);
    CHECK(g->endDistance.has_value() && *g->endDistance == 50);
    CHECK(!g->startKappa.has_value() && !g->length.has_value());
  }
  {
    cursiv::ParseResult r = parse("CLOTHOID (0, 0.005, 80)");
    ast::Document* d = document(r);
    CHECK(d != nullptr && d->geometries.size() == 1);
    if (d == nullptr || d->geometries.size() != 1) return;
    auto* g = static_cast<ast::ClothoidGeom*>(d->geometries[0]);
    CHECK(g->referenceLocation == nullptr && !g->scaleFactor.has_value());
    CHECK(g->startKappa.has_value() && *g->startKappa == 0);
    CHECK(g->endKappa.has_value() && *g->endKappa == 0.005);
    CHECK(g->length.has_value() && *g->length == 80);
  }
}

// SPIRAL_TYPE_NAME is legal only where SPIRALTYPE's value is expected, so a
// spelling that is a geometry keyword everywhere else reads as plain text
// here -- per-state lexing, not a lexer mode.
void spiral_type_text_read_back() {
  cursiv::ParseResult r = parse(
      "SPIRALCURVE (REFERENCELOCATION AFFINEPLACEMENT EMPTY, LENGTH 1, "
      "STARTCURVATURE 0, ENDCURVATURE 0, SPIRALTYPE POINT)");
  ast::Document* d = document(r);
  CHECK(d != nullptr && d->geometries.size() == 1);
  if (d == nullptr || d->geometries.size() != 1) return;
  auto* g = static_cast<ast::SpiralCurveGeom*>(d->geometries[0]);
  CHECK(g->spiralType == "POINT");

  cursiv::ParseResult r2 = parse(
      "SPIRALCURVE (REFERENCELOCATION AFFINEPLACEMENT EMPTY, LENGTH 1, "
      "STARTCURVATURE 0, ENDCURVATURE 0, SPIRALTYPE trailing-space   )");
  ast::Document* d2 = document(r2);
  CHECK(d2 != nullptr && d2->geometries.size() == 1);
  if (d2 == nullptr || d2->geometries.size() != 1) return;
  auto* g2 = static_cast<ast::SpiralCurveGeom*>(d2->geometries[0]);
  CHECK(g2->spiralType == "trailing-space");
}

// The optional ID/TAG pair every TIN element carries, and which of the
// three payload shapes its kind carries.
void tin_element_read_back() {
  cursiv::ParseResult r = parse(
      "TIN (PATCHES ((0 0, 1 0, 0 1)) ELEMENTS (BREAKLINE ID 7 TAG "
      "\"ridge line\" LINESTRING (0 0, 1 1)))");
  ast::Document* d = document(r);
  CHECK(d != nullptr && d->geometries.size() == 1);
  if (d == nullptr || d->geometries.size() != 1) return;
  auto* tin = static_cast<ast::TinGeom*>(d->geometries[0]);
  CHECK(tin->patchTriangles.size() == 1);
  CHECK(tin->elements.size() == 1);
  if (tin->elements.size() != 1) return;
  const ast::TinElement& el = *tin->elements[0];
  CHECK(el.kind == ast::TinElementKind::breakline);
  CHECK(el.id.has_value() && *el.id == 7);
  CHECK(el.tag == "ridge line");
  auto* line = std::get_if<ast::LineStringGeom*>(&el.payload);
  CHECK(line != nullptr && *line != nullptr && (*line)->points.size() == 2);
}

// A real corpus document, read end to end rather than only counted: the
// index of each checked line is fixed by example2.wkt's own line order.
void real_corpus_document_read_back() {
  const std::string text = cursiv::test::read_file(
      (fs::path(CURSIV_EXAMPLE_DIR) / "example" / "example2.wkt").string());
  cursiv::ParseResult r = parse(text);
  CHECK(r.accepted && !r.cst().recovered() && r.diagnostics.empty());
  ast::Document* d = document(r);
  CHECK(d != nullptr);
  if (d == nullptr) return;
  CHECK(d->geometries.size() == 39);
  if (d->geometries.size() != 39) return;

  auto* first = static_cast<ast::PointGeom*>(d->geometries[0]);  // POINT (1 2)
  CHECK(first->value != nullptr && first->value->ordinates.size() == 2 && first->value->ordinates[0] == "1" &&
        first->value->ordinates[1] == "2");

  // COMPOUNDCURVE ((0 0, 100 0), CLOTHOID (0, 0.005, 48),
  //   CIRCULARSTRING (147.93 1.92, 200 50, 150 100), CLOTHOID (0.005, 0, 42))
  auto* last = static_cast<ast::CompoundCurveGeom*>(d->geometries[38]);
  CHECK(last->members.size() == 4);
  if (last->members.size() != 4) return;
  auto* seg0 = std::get_if<ast::PointList*>(&last->members[0]);
  CHECK(seg0 != nullptr && *seg0 != nullptr && (*seg0)->points.size() == 2);
  auto* seg1 = std::get_if<ast::Geometry*>(&last->members[1]);
  CHECK(seg1 != nullptr && *seg1 != nullptr &&
        (*seg1)->type == &cursiv::ast_type_tag<ast::ClothoidGeom>);
  if (seg1 != nullptr && *seg1 != nullptr) {
    auto* clothoid = static_cast<ast::ClothoidGeom*>(*seg1);
    CHECK(clothoid->startKappa.has_value() && *clothoid->startKappa == 0);
    CHECK(clothoid->length.has_value() && *clothoid->length == 48);
  }
}

}  // namespace

int main() {
  CHECK(native_regressions());
  if (!audit_regressions::run()) return 1;
  corpus_parses_clean();
  coverage_parses_clean();
  point_ordinates_read_back();
  clothoid_two_forms_read_back();
  spiral_type_text_read_back();
  tin_element_read_back();
  real_corpus_document_read_back();
  return cursiv::test::report("wkt_grammar_test");
}
