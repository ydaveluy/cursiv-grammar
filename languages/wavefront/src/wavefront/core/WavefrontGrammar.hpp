// The wavefront language: its grammar.
#pragma once

#include <cstdlib>
#include <string>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "wavefront/core/ast.hpp"

namespace wavefront {

inline constexpr auto to_curve_kind =
    [](std::string_view t) -> ast::CurveSurfaceKind {
  if (t == "bmatrix") return ast::CurveSurfaceKind::bmatrix;
  if (t == "bezier") return ast::CurveSurfaceKind::bezier;
  if (t == "bspline") return ast::CurveSurfaceKind::bspline;
  if (t == "cardinal") return ast::CurveSurfaceKind::cardinal;
  return ast::CurveSurfaceKind::taylor;
};
using CurveKindConv = cursiv::text_conv<to_curve_kind>;

inline constexpr auto number_value = [](std::string_view t,
                                        std::pmr::memory_resource*) {
  return std::strtod(std::string(t).c_str(), nullptr);
};
inline constexpr auto trimmed = [](std::string_view t,
                                   std::pmr::memory_resource*) {
  while (!t.empty() && (t.front() == ' ' || t.front() == '\t'))
    t.remove_prefix(1);
  while (!t.empty() && (t.back() == ' ' || t.back() == '\t'))
    t.remove_suffix(1);
  return t;
};

inline constexpr auto filename_value = [](std::string_view t,
                                          std::pmr::memory_resource* memory) {
  if (t.find('\\') == t.npos) return t;
  char* data = static_cast<char*>(memory->allocate(t.size(), alignof(char)));
  std::size_t n = 0;
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] == '\\' && ++i == t.size()) break;
    data[n++] = t[i];
  }
  return std::string_view(data, n);
};

struct Wavefront : cursiv::GrammarSpec {
  struct CurveKindRule : rule<kw<"bmatrix"> | kw<"bezier"> | kw<"bspline"> |
                                  kw<"cardinal"> | kw<"taylor">,
                              CurveKindConv> {};

  struct AxisURule : rule<kw<"u">, value<ast::Axis::u>> {};
  struct AxisVRule : rule<kw<"v">, value<ast::Axis::v>> {};
  static constexpr auto axis_uv = ref<AxisURule> | ref<AxisVRule>;

  static constexpr auto nl_body = lit<"\r\n"> | lit<"\r"> | lit<"\n">;
  struct NL : terminal<nl_body> {};
  struct BOM : terminal<lit<"\xEF\xBB\xBF">> {};

  struct WS : terminal<some(cr<R"( \t)"> | (lit<"\\"> + nl_body))> {};

  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto int_digits = option(cr<"+-">) + some(d);
  struct INTEGER : terminal<int_digits, std::int64_t> {};
  struct NUMBER
      : terminal<option(cr<"+-">) +
                     (((some(d) + option(lit<"."> + many(d))) |
                       (lit<"."> + some(d))) +
                          option(cr<"eE"> + option(cr<"+-">) + some(d)) |
                      ikw<"nan"> | ikw<"inf"> | ikw<"infinity">),
                 cursiv::text_conv<number_value>> {};
  struct INDEX : terminal<int_digits> {};
  struct SlashIndex
      : terminal<
            lit<"/"> + option(int_digits),
            cursiv::text_conv<[](std::string_view t) { return t.substr(1); }>> {
  };
  struct ElementRule
      : rule<create<ast::ElementRef> +
             assign<&ast::ElementRef::vertex>(ref<INDEX>) +
             option(
                 assign<&ast::ElementRef::texture>(ref<SlashIndex>) +
                 option(assign<&ast::ElementRef::normal>(ref<SlashIndex>)))> {};
  static constexpr auto element_ref = ref<ElementRule>;
  struct NAME : terminal<some(cr<R"(^ \t\r\n)">)> {};
  static constexpr auto filename_body =
      some((lit<"\\"> + cr<R"( \t)">) | cr<R"(^ \t\r\n)">);
  struct FILENAME : terminal<filename_body> {};
  struct LIBRARY_FILENAME
      : terminal<filename_body, cursiv::text_conv<filename_value>> {};
  struct DIRECTIVE : terminal<!lit<"\xEF\xBB\xBF"> + cr<R"(^ \t\r\n#)"> +
                              many(cr<R"(^ \t\r\n)">)> {};
  using NON_WS = NAME;
  struct REST : terminal<some(cr<R"(^\r\n)">), cursiv::text_conv<trimmed>> {};
  struct OptionalRest : rule<option(ref<REST>)> {};

  struct VertexRule
      : rule<kw<"v"> + assign<&ast::Vertex::x>(ref<NUMBER>) +
             assign<&ast::Vertex::y>(ref<NUMBER>) +
             assign<&ast::Vertex::z>(ref<NUMBER>) +
             option((append<&ast::Vertex::color>(ref<NUMBER>) +
                     some(append<&ast::Vertex::color>(ref<NUMBER>))) |
                    assign<&ast::Vertex::w>(ref<NUMBER>))> {};

  struct VertexParameterRule
      : rule<kw<"vp"> + assign<&ast::VertexParameter::u>(ref<NUMBER>) +
             option(assign<&ast::VertexParameter::v>(ref<NUMBER>) +
                    option(assign<&ast::VertexParameter::w>(ref<NUMBER>)))> {};

  struct VertexNormalRule
      : rule<kw<"vn"> + assign<&ast::VertexNormal::x>(ref<NUMBER>) +
             assign<&ast::VertexNormal::y>(ref<NUMBER>) +
             assign<&ast::VertexNormal::z>(ref<NUMBER>)> {};

  struct VertexTextureRule
      : rule<kw<"vt"> + assign<&ast::VertexTexture::u>(ref<NUMBER>) +
             option(assign<&ast::VertexTexture::v>(ref<NUMBER>) +
                    option(assign<&ast::VertexTexture::w>(ref<NUMBER>)))> {};

  struct CurveSurfaceTypeRule
      : rule<kw<"cstype"> +
             option(flag<&ast::CurveSurfaceType::rational>(kw<"rat">)) +
             assign<&ast::CurveSurfaceType::kind>(ref<CurveKindRule>) +
             option(flag<&ast::CurveSurfaceType::rational>(kw<"rat">))> {};

  struct DegreeRule : rule<kw<"deg"> + assign<&ast::Degree::u>(ref<INTEGER>) +
                           option(assign<&ast::Degree::v>(ref<INTEGER>))> {};

  struct BasisMatrixRule
      : rule<kw<"bmat"> + assign<&ast::BasisMatrix::axis>(axis_uv) +
             some(append<&ast::BasisMatrix::values>(ref<NUMBER>))> {};

  struct StepRule : rule<kw<"step"> + assign<&ast::Step::u>(ref<INTEGER>) +
                         option(assign<&ast::Step::v>(ref<INTEGER>))> {};

  struct PointsRule
      : rule<kw<"p"> + many(append<&ast::Points::v>(element_ref))> {};

  struct LinesRule : rule<kw<"l"> + many(append<&ast::Lines::v>(element_ref))> {
  };

  struct FacesRule : rule<kw<"f"> + many(append<&ast::Faces::v>(element_ref))> {
  };

  struct CurveRule : rule<kw<"curv"> + assign<&ast::Curve::u0>(ref<NUMBER>) +
                          assign<&ast::Curve::u1>(ref<NUMBER>) +
                          some(append<&ast::Curve::v>(ref<INTEGER>))> {};

  struct Curve2DRule
      : rule<kw<"curv2"> + many(append<&ast::Curve2D::vp>(ref<INTEGER>))> {};

  struct SurfaceRule
      : rule<kw<"surf"> + assign<&ast::Surface::s0>(ref<NUMBER>) +
             assign<&ast::Surface::s1>(ref<NUMBER>) +
             assign<&ast::Surface::t0>(ref<NUMBER>) +
             assign<&ast::Surface::t1>(ref<NUMBER>) +
             some(append<&ast::Surface::v>(element_ref))> {};

  struct ParameterRule
      : rule<kw<"parm"> + assign<&ast::Parameter::axis>(axis_uv) +
             some(append<&ast::Parameter::p>(ref<NUMBER>))> {};

  struct TrimSegmentRule
      : rule<assign<&ast::TrimSegment::u0>(ref<NUMBER>) +
             assign<&ast::TrimSegment::u1>(ref<NUMBER>) +
             assign<&ast::TrimSegment::curv2d>(ref<INTEGER>)> {};

  struct OuterTrimmingLoopRule
      : rule<kw<"trim"> + some(append<&ast::OuterTrimmingLoop::segments>(
                              ref<TrimSegmentRule>))> {};

  struct InnerTrimmingLoopRule
      : rule<kw<"hole"> + some(append<&ast::InnerTrimmingLoop::segments>(
                              ref<TrimSegmentRule>))> {};

  struct SpecialCurveRule
      : rule<kw<"scrv"> +
             some(append<&ast::SpecialCurve::segments>(ref<TrimSegmentRule>))> {
  };

  struct SpecialPointRule
      : rule<kw<"sp"> + some(append<&ast::SpecialPoint::vp>(ref<INTEGER>))> {};

  struct FreeFormBodyItemRule
      : rule<ref<ParameterRule> | ref<OuterTrimmingLoopRule> |
             ref<InnerTrimmingLoopRule> | ref<SpecialCurveRule> |
             ref<SpecialPointRule>> {};

  static constexpr auto blank = ref<NL> | ref<COMMENT>;
  static constexpr auto line_end =
      option(ref<COMMENT>) + (some(ref<NL>) | eof) + many(blank);

  struct FreeFormSurfaceRule
      : rule<assign<&ast::FreeFormSurface::head>(
                 ref<CurveRule> | ref<Curve2DRule> | ref<SurfaceRule>) +
             line_end +
             many(append<&ast::FreeFormSurface::body>(
                      ref<FreeFormBodyItemRule>) +
                  line_end) +
             (kw<"end"> | eof)> {};

  struct ConnectivityRule
      : rule<kw<"con"> + assign<&ast::Connectivity::surf1>(ref<INTEGER>) +
             assign<&ast::Connectivity::q0_1>(ref<NUMBER>) +
             assign<&ast::Connectivity::q1_1>(ref<NUMBER>) +
             assign<&ast::Connectivity::curv2d1>(ref<INTEGER>) +
             assign<&ast::Connectivity::surf2>(ref<INTEGER>) +
             assign<&ast::Connectivity::q0_2>(ref<NUMBER>) +
             assign<&ast::Connectivity::q1_2>(ref<NUMBER>) +
             assign<&ast::Connectivity::curv2d2>(ref<INTEGER>)> {};

  struct GroupRule
      : rule<kw<"g"> + many(append<&ast::Group::names>(ref<NAME>))> {};

  struct SmoothingGroupRule
      : rule<kw<"s"> +
             (flag<&ast::SmoothingGroup::off>(kw<"off">) |
              assign<&ast::SmoothingGroup::groupNumber>(ref<INTEGER>))> {};

  struct MergingGroupRule
      : rule<kw<"mg"> +
             (flag<&ast::MergingGroup::off>(kw<"off">) |
              (assign<&ast::MergingGroup::groupNumber>(ref<INTEGER>) +
               assign<&ast::MergingGroup::res>(ref<NUMBER>)))> {};

  struct ObjectNameRule
      : rule<kw<"o"> + assign<&ast::ObjectName::name>(ref<OptionalRest>)> {};

  struct BevelRule
      : rule<kw<"bevel"> + (flag<&ast::Bevel::on>(kw<"on">) | kw<"off">)> {
  };

  struct ColorInterpolationRule
      : rule<kw<"c_interp"> +
             (flag<&ast::ColorInterpolation::on>(kw<"on">) | kw<"off">)> {
  };

  struct DissolveInterpolationRule
      : rule<kw<"d_interp"> +
             (flag<&ast::DissolveInterpolation::on>(kw<"on">) |
              kw<"off">)> {};

  struct LevelOfDetailRule
      : rule<kw<"lod"> + assign<&ast::LevelOfDetail::level>(ref<INTEGER>)> {};

  struct MapLibraryRule
      : rule<kw<"maplib"> +
             many(append<&ast::MapLibrary::filenames>(ref<FILENAME>))> {};

  struct UseMapRule
      : rule<kw<"usemap"> + (flag<&ast::UseMap::off>(kw<"off">) |
                             assign<&ast::UseMap::name>(ref<NAME>))> {};

  struct MaterialLibraryRule
      : rule<kw<"mtllib"> + many(append<&ast::MaterialLibrary::filenames>(
                                ref<LIBRARY_FILENAME>))> {};

  struct UseMaterialRule : rule<kw<"usemtl"> + assign<&ast::UseMaterial::name>(
                                                   ref<OptionalRest>)> {};

  struct ShadowObjectRule
      : rule<kw<"shadow_obj"> +
             assign<&ast::ShadowObject::filename>(ref<FILENAME>)> {};

  struct TraceObjectRule
      : rule<kw<"trace_obj"> +
             assign<&ast::TraceObject::filename>(ref<FILENAME>)> {};

  struct CtechCparmRule : rule<kw<"ctech"> + kw<"cparm"> +
                               assign<&ast::CtechCparm::res>(ref<NUMBER>)> {};

  struct CtechCspaceRule
      : rule<kw<"ctech"> + kw<"cspace"> +
             assign<&ast::CtechCspace::maxlength>(ref<NUMBER>)> {};

  struct CtechCurvRule : rule<kw<"ctech"> + kw<"curv"> +
                              assign<&ast::CtechCurv::maxdist>(ref<NUMBER>) +
                              assign<&ast::CtechCurv::maxangle>(ref<NUMBER>)> {
  };

  struct StechCparmaRule : rule<kw<"stech"> + kw<"cparma"> +
                                assign<&ast::StechCparma::ures>(ref<NUMBER>) +
                                assign<&ast::StechCparma::vres>(ref<NUMBER>)> {
  };

  struct StechCparmbRule : rule<kw<"stech"> + kw<"cparmb"> +
                                assign<&ast::StechCparmb::uvres>(ref<NUMBER>)> {
  };

  struct StechCspaceRule
      : rule<kw<"stech"> + kw<"cspace"> +
             assign<&ast::StechCspace::maxlength>(ref<NUMBER>)> {};

  struct StechCurvRule : rule<kw<"stech"> + kw<"curv"> +
                              assign<&ast::StechCurv::maxdist>(ref<NUMBER>) +
                              assign<&ast::StechCurv::maxangle>(ref<NUMBER>)> {
  };

  struct CallRule
      : rule<kw<"call"> + assign<&ast::Call::filename>(ref<FILENAME>) +
             many(append<&ast::Call::args>(ref<NON_WS>))> {};

  struct CshRule : rule<kw<"csh"> + assign<&ast::Csh::command>(ref<FILENAME>) +
                        option(flag<&ast::Csh::dash>(lit<"-">)) +
                        many(append<&ast::Csh::args>(ref<NON_WS>))> {};

  struct StatementRule
      : rule<ref<VertexRule> | ref<VertexNormalRule> | ref<VertexTextureRule> |
             ref<VertexParameterRule> | ref<PointsRule> | ref<LinesRule> |
             ref<FacesRule> | ref<CurveSurfaceTypeRule> | ref<DegreeRule> |
             ref<BasisMatrixRule> | ref<StepRule> | ref<FreeFormSurfaceRule> |
             ref<ConnectivityRule> | ref<GroupRule> | ref<SmoothingGroupRule> |
             ref<MergingGroupRule> | ref<ObjectNameRule> | ref<BevelRule> |
             ref<ColorInterpolationRule> | ref<DissolveInterpolationRule> |
             ref<LevelOfDetailRule> | ref<MapLibraryRule> | ref<UseMapRule> |
             ref<MaterialLibraryRule> | ref<UseMaterialRule> |
             ref<ShadowObjectRule> | ref<TraceObjectRule> |
             ref<CtechCparmRule> | ref<CtechCspaceRule> | ref<CtechCurvRule> |
             ref<StechCparmaRule> | ref<StechCparmbRule> |
             ref<StechCspaceRule> | ref<StechCurvRule> | ref<CallRule> |
             ref<CshRule>> {};

  static constexpr auto Known =
      kw<"v"> | kw<"vn"> | kw<"vt"> | kw<"vp"> | kw<"p"> | kw<"l"> | kw<"f"> |
      kw<"cstype"> | kw<"deg"> | kw<"bmat"> | kw<"step"> | kw<"curv"> |
      kw<"curv2"> | kw<"surf"> | kw<"con"> | kw<"g"> | kw<"s"> | kw<"mg"> |
      kw<"o"> | kw<"bevel"> | kw<"c_interp"> | kw<"d_interp"> | kw<"lod"> |
      kw<"maplib"> | kw<"usemap"> | kw<"mtllib"> | kw<"usemtl"> |
      kw<"shadow_obj"> | kw<"trace_obj"> | kw<"ctech"> | kw<"stech"> |
      kw<"call"> | kw<"csh">;
  struct OtherStatementRule
      : rule<!Known + create<ast::OtherStatement> +
             assign<&ast::OtherStatement::keyword>(ref<DIRECTIVE>) +
             assign<&ast::OtherStatement::arguments>(ref<OptionalRest>)> {};
  struct DocRule : rule<create<ast::Document> + option(ref<BOM>) + many(blank) +
                        many(append<&ast::Document::statements>(
                                 ref<StatementRule> | ref<OtherStatementRule>) +
                             line_end)> {};

  using entry = DocRule;
  using ignore = WS;
};

}  // namespace wavefront