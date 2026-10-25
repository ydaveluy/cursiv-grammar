#include "audit_regressions.hpp"
#include "code_audit.hpp"
// The smoke test of the language: every corpus document parses clean, and a
// few are read back field by field to catch a converter that silently
// mismaps its value.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "wavefront/core/CoreServices.hpp"
#include "wavefront/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "wavefront: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "b1_14_bezier_in_u_direction_with_b_spline_in_v_direction_with_basis_"
    "matrix.obj",
    "b1_14_cubic_bezier_surface_with_basis_matrix.obj",
    "b1_14_hermite_curve_with_basis_matrix.obj",
    "b1_20_cube.obj",
    "b1_20_square.obj",
    "b1_21_cube_with_negative_reference_numbers.obj",
    "b1_27_taylor_curve.obj",
    "b1_28_bezier_curve.obj",
    "b1_29_b_spline_surface.obj",
    "b1_30_cardinal_surface.obj",
    "b1_31_rational_b_spline_surface.obj",
    "b1_32_trimmed_nurb_surface.obj",
    "b1_33_two_trimming_regions_with_hole.obj",
    "b1_35_trimming_with_special_curve.obj",
    "b1_36_trimming_with_special_points.obj",
    "b1_39_connectivity_between_two_surfaces.obj",
    "b1_43_cube_with_group_names.obj",
    "b1_44_two_adjoining_squares_with_smoothing_group.obj",
    "b1_45_two_adjoining_squares_with_vertex_normals.obj",
    "b1_46_merging_group.obj",
    "b1_55_cube_with_materials.obj",
    "b1_56_cube_casting_shadow.obj",
    "b1_57_cube_casting_reflection.obj",
    "b1_58_texture_mapped_square.obj",
    "b1_59_approximation_technique_for_surface.obj",
    "b1_60_approximation_technique_for_curve.obj",
    "b1_76_cardinal_curve.obj",
    "b1_77_bezier_patch.obj",
    "b1_76_cardinal_curve_old.obj",
    "b1_77_bezier_patch_old.obj",
};

using Language =
    decltype(cursiv::create_language<wavefront::WavefrontCoreServices,
                                     wavefront::WavefrontSharedCoreServices,
                                     wavefront::install_core_module>());

cursiv::ParseResult parse_clean(const Language& language,
                                const std::string& path,
                                const std::string& text) {
  cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "wavefront: %s: does not parse clean\n", path.c_str());
    return result;
  }
  if (!result.diagnostics.empty()) {
    std::fprintf(stderr, "wavefront: %s: has diagnostics\n", path.c_str());
    return result;
  }
  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<wavefront::ast::Document>) {
    std::fprintf(stderr, "wavefront: %s: built no Document\n", path.c_str());
    return result;
  }
  return result;
}

}  // namespace

int main() {
  if (!audit_regressions::run() || !code_audit::run()) return 1;
  const auto language =
      cursiv::create_language<wavefront::WavefrontCoreServices,
                              wavefront::WavefrontSharedCoreServices,
                              wavefront::install_core_module>();

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail((std::string(name) + ": empty or unreadable").c_str());
    const auto parsed = parse_clean(language, name, text);
    if (!parsed.accepted || parsed.cst().recovered() ||
        !parsed.diagnostics.empty() || !parsed.value)
      return 1;
  }

  // features.obj covers what the corpus never uses: points, lines, object
  // name, the display toggles, level of detail, maplib/usemap, "off"
  // smoothing/merging groups, call/csh, and the ctech/stech forms the
  // corpus leaves untried.
  {
    const std::string path = std::string(CURSIV_TEST_DIR) + "/features.obj";
    const std::string text = read_file(path);
    if (text.empty()) return fail("features.obj: empty or unreadable");
    const auto parsed = parse_clean(language, "features.obj", text);
    const auto* doc = parsed.value_as<wavefront::ast::Document>();
    if (doc == nullptr) return 1;

    const wavefront::ast::Statement* objectName = nullptr;
    const wavefront::ast::Statement* points = nullptr;
    const wavefront::ast::Statement* call = nullptr;
    const wavefront::ast::Statement* csh = nullptr;
    const wavefront::ast::Statement* useMapName = nullptr;
    const wavefront::ast::Statement* useMapOff = nullptr;
    for (const wavefront::ast::Statement* const stmt : doc->statements) {
      if (stmt->type == &cursiv::ast_type_tag<wavefront::ast::ObjectName>)
        objectName = stmt;
      else if (stmt->type == &cursiv::ast_type_tag<wavefront::ast::Points>)
        points = stmt;
      else if (stmt->type == &cursiv::ast_type_tag<wavefront::ast::Call>)
        call = stmt;
      else if (stmt->type == &cursiv::ast_type_tag<wavefront::ast::Csh>)
        csh = stmt;
      else if (stmt->type == &cursiv::ast_type_tag<wavefront::ast::UseMap>) {
        const auto& um = *static_cast<const wavefront::ast::UseMap*>(stmt);
        (um.off ? useMapOff : useMapName) = stmt;
      }
    }

    if (objectName == nullptr ||
        static_cast<const wavefront::ast::ObjectName*>(objectName)->name !=
            "widget")
      return fail("features.obj: wrong object name");

    if (points == nullptr) return fail("features.obj: no points statement");
    const auto& p = *static_cast<const wavefront::ast::Points*>(points);
    if (p.v.size() != 3 || p.v[0]->vertex != "1" || p.v[1]->vertex != "2" ||
        p.v[2]->vertex != "3")
      return fail("features.obj: wrong points list");

    if (call == nullptr) return fail("features.obj: no call statement");
    const auto& c = *static_cast<const wavefront::ast::Call*>(call);
    if (c.filename != "other.obj" || c.args.size() != 2 ||
        c.args[0] != "arg1" || c.args[1] != "arg2")
      return fail("features.obj: wrong call fields");

    if (csh == nullptr) return fail("features.obj: no csh statement");
    const auto& sh = *static_cast<const wavefront::ast::Csh*>(csh);
    if (sh.command != "backup" || !sh.dash || sh.args.size() != 1 ||
        sh.args[0] != "nightly")
      return fail("features.obj: wrong csh fields");

    if (useMapName == nullptr || useMapOff == nullptr)
      return fail("features.obj: expected two usemap statements");
    const auto& umName =
        *static_cast<const wavefront::ast::UseMap*>(useMapName);
    if (!umName.name.has_value() || *umName.name != "tiles")
      return fail("features.obj: wrong usemap name");
  }

  // b1_20_cube.obj, read back field by field: 8 vertices then 6 faces, no
  // texture/normal indices.
  {
    const std::string path =
        std::string(CURSIV_EXAMPLE_DIR) + "/b1_20_cube.obj";
    const std::string text = read_file(path);
    const auto parsed = parse_clean(language, "b1_20_cube.obj", text);
    const auto* doc = parsed.value_as<wavefront::ast::Document>();
    if (doc == nullptr) return 1;
    if (doc->statements.size() != 14)
      return fail("b1_20_cube.obj: expected 14 statements");

    if (doc->statements[0]->type !=
        &cursiv::ast_type_tag<wavefront::ast::Vertex>)
      return fail("b1_20_cube.obj: statement 0 is not a Vertex");
    const auto& v0 =
        *static_cast<const wavefront::ast::Vertex*>(doc->statements[0]);
    if (v0.x != 0.0 || v0.y != 2.0 || v0.z != 2.0 || v0.w.has_value())
      return fail("b1_20_cube.obj: wrong first vertex");

    if (doc->statements[8]->type !=
        &cursiv::ast_type_tag<wavefront::ast::Faces>)
      return fail("b1_20_cube.obj: statement 8 is not Faces");
    const auto& f0 =
        *static_cast<const wavefront::ast::Faces*>(doc->statements[8]);
    if (f0.v.size() != 4) return fail("b1_20_cube.obj: wrong face arity");
    const auto* const first = f0.v[0];
    if (first == nullptr || first->vertex != "1")
      return fail("b1_20_cube.obj: wrong first face index");
  }

  // b1_43_cube_with_group_names.obj: group names, read back as text.
  {
    const std::string path =
        std::string(CURSIV_EXAMPLE_DIR) + "/b1_43_cube_with_group_names.obj";
    const std::string text = read_file(path);
    const auto parsed =
        parse_clean(language, "b1_43_cube_with_group_names.obj", text);
    const auto* doc = parsed.value_as<wavefront::ast::Document>();
    if (doc == nullptr) return 1;
    if (doc->statements.size() != 20)
      return fail("b1_43_cube_with_group_names.obj: expected 20 statements");
    if (doc->statements[8]->type !=
        &cursiv::ast_type_tag<wavefront::ast::Group>)
      return fail(
          "b1_43_cube_with_group_names.obj: statement 8 is not a Group");
    const auto& g =
        *static_cast<const wavefront::ast::Group*>(doc->statements[8]);
    if (g.names.size() != 2 || g.names[0] != "front" || g.names[1] != "cube")
      return fail("b1_43_cube_with_group_names.obj: wrong group names");
  }

  // b1_31_rational_b_spline_surface.obj: `cstype rat bspline`, and a
  // vertex with an explicit `w` beside one that defaults it.
  {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) +
                             "/b1_31_rational_b_spline_surface.obj";
    const std::string text = read_file(path);
    const auto parsed =
        parse_clean(language, "b1_31_rational_b_spline_surface.obj", text);
    const auto* doc = parsed.value_as<wavefront::ast::Document>();
    if (doc == nullptr) return 1;

    const auto& v1 =
        *static_cast<const wavefront::ast::Vertex*>(doc->statements[1]);
    if (!v1.w.has_value() || *v1.w != 7.6)
      return fail("b1_31_rational_b_spline_surface.obj: wrong vertex w");

    const wavefront::ast::CurveSurfaceType* cstype = nullptr;
    for (const wavefront::ast::Statement* const stmt : doc->statements)
      if (stmt->type == &cursiv::ast_type_tag<wavefront::ast::CurveSurfaceType>)
        cstype = static_cast<const wavefront::ast::CurveSurfaceType*>(stmt);
    if (cstype == nullptr || !cstype->rational ||
        cstype->kind != wavefront::ast::CurveSurfaceKind::bspline)
      return fail("b1_31_rational_b_spline_surface.obj: wrong cstype");
  }

  std::puts("wavefront: the corpus parses");
  return 0;
}
