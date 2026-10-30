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
  struct CurveKind
      : rule<as<to_curve_kind>(kw<"bmatrix"> | kw<"bezier"> | kw<"bspline"> |
                               kw<"cardinal"> | kw<"taylor">)> {};

  static constexpr auto axis_uv = as<ast::Axis::u>(kw<"u">) | as<ast::Axis::v>(kw<"v">);

  static constexpr auto nl_body = lit<"\r\n"> | lit<"\r"> | lit<"\n">;
  struct NL : terminal<nl_body> {};
  struct BOM : terminal<lit<"\xEF\xBB\xBF">> {};

  struct WS : terminal<some(cr<R"( \t)"> | (lit<"\\"> + nl_body))> {};

  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  static constexpr auto int_digits = option(cr<"+-">) + some(d);
  struct INTEGER : terminal<as<std::int64_t>(int_digits)> {};
  struct NUMBER
      : terminal<as<number_value>(
            option(cr<"+-">) +
            ((((some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d))) +
              option(cr<"eE"> + option(cr<"+-">) + some(d))) |
             ikw<"nan"> | ikw<"inf"> | ikw<"infinity">))> {};
  struct INDEX : terminal<int_digits> {};
  struct SlashIndex
      : terminal<as<[](std::string_view t) { return t.substr(1); }>(
            lit<"/"> + option(int_digits))> {};
  struct Element
      : rule<create<ast::ElementRef> +
             assign<&ast::ElementRef::vertex>(ref<INDEX>) +
             option(
                 assign<&ast::ElementRef::texture>(ref<SlashIndex>) +
                 option(assign<&ast::ElementRef::normal>(ref<SlashIndex>)))> {};
  static constexpr auto element_ref = ref<Element>;
  struct NAME : terminal<some(cr<R"(^ \t\r\n)">)> {};
  static constexpr auto filename_body =
      some((lit<"\\"> + cr<R"( \t)">) | cr<R"(^ \t\r\n)">);
  struct FILENAME : terminal<filename_body> {};
  struct LIBRARY_FILENAME : terminal<as<filename_value>(filename_body)> {};
  struct DIRECTIVE : terminal<!lit<"\xEF\xBB\xBF"> + cr<R"(^ \t\r\n#)"> +
                              many(cr<R"(^ \t\r\n)">)> {};
  using NON_WS = NAME;
  struct REST : terminal<as<trimmed>(some(cr<R"(^\r\n)">))> {};
  struct OptionalRest : rule<option(ref<REST>)> {};

  struct Vertex
      : rule<kw<"v"> + assign<&ast::Vertex::x>(ref<NUMBER>) +
             assign<&ast::Vertex::y>(ref<NUMBER>) +
             assign<&ast::Vertex::z>(ref<NUMBER>) +
             option((append<&ast::Vertex::color>(ref<NUMBER>) +
                     some(append<&ast::Vertex::color>(ref<NUMBER>))) |
                    assign<&ast::Vertex::w>(ref<NUMBER>))> {};

  struct VertexParameter
      : rule<kw<"vp"> + assign<&ast::VertexParameter::u>(ref<NUMBER>) +
             option(assign<&ast::VertexParameter::v>(ref<NUMBER>) +
                    option(assign<&ast::VertexParameter::w>(ref<NUMBER>)))> {};

  struct VertexNormal
      : rule<kw<"vn"> + assign<&ast::VertexNormal::x>(ref<NUMBER>) +
             assign<&ast::VertexNormal::y>(ref<NUMBER>) +
             assign<&ast::VertexNormal::z>(ref<NUMBER>)> {};

  struct VertexTexture
      : rule<kw<"vt"> + assign<&ast::VertexTexture::u>(ref<NUMBER>) +
             option(assign<&ast::VertexTexture::v>(ref<NUMBER>) +
                    option(assign<&ast::VertexTexture::w>(ref<NUMBER>)))> {};

  struct CurveSurfaceType
      : rule<kw<"cstype"> +
             option(flag<&ast::CurveSurfaceType::rational>(kw<"rat">)) +
             assign<&ast::CurveSurfaceType::kind>(ref<CurveKind>) +
             option(flag<&ast::CurveSurfaceType::rational>(kw<"rat">))> {};

  struct Degree : rule<kw<"deg"> + assign<&ast::Degree::u>(ref<INTEGER>) +
                           option(assign<&ast::Degree::v>(ref<INTEGER>))> {};

  struct BasisMatrix
      : rule<kw<"bmat"> + assign<&ast::BasisMatrix::axis>(axis_uv) +
             some(append<&ast::BasisMatrix::values>(ref<NUMBER>))> {};

  struct Step : rule<kw<"step"> + assign<&ast::Step::u>(ref<INTEGER>) +
                         option(assign<&ast::Step::v>(ref<INTEGER>))> {};

  struct Points
      : rule<kw<"p"> + many(append<&ast::Points::v>(element_ref))> {};

  struct Lines : rule<kw<"l"> + many(append<&ast::Lines::v>(element_ref))> {
  };

  struct Faces : rule<kw<"f"> + many(append<&ast::Faces::v>(element_ref))> {
  };

  struct Curve : rule<kw<"curv"> + assign<&ast::Curve::u0>(ref<NUMBER>) +
                          assign<&ast::Curve::u1>(ref<NUMBER>) +
                          some(append<&ast::Curve::v>(ref<INTEGER>))> {};

  struct Curve2D
      : rule<kw<"curv2"> + many(append<&ast::Curve2D::vp>(ref<INTEGER>))> {};

  struct Surface
      : rule<kw<"surf"> + assign<&ast::Surface::s0>(ref<NUMBER>) +
             assign<&ast::Surface::s1>(ref<NUMBER>) +
             assign<&ast::Surface::t0>(ref<NUMBER>) +
             assign<&ast::Surface::t1>(ref<NUMBER>) +
             some(append<&ast::Surface::v>(element_ref))> {};

  struct Parameter
      : rule<kw<"parm"> + assign<&ast::Parameter::axis>(axis_uv) +
             some(append<&ast::Parameter::p>(ref<NUMBER>))> {};

  struct TrimSegment
      : rule<assign<&ast::TrimSegment::u0>(ref<NUMBER>) +
             assign<&ast::TrimSegment::u1>(ref<NUMBER>) +
             assign<&ast::TrimSegment::curv2d>(ref<INTEGER>)> {};

  struct OuterTrimmingLoop
      : rule<kw<"trim"> + some(append<&ast::OuterTrimmingLoop::segments>(
                              ref<TrimSegment>))> {};

  struct InnerTrimmingLoop
      : rule<kw<"hole"> + some(append<&ast::InnerTrimmingLoop::segments>(
                              ref<TrimSegment>))> {};

  struct SpecialCurve
      : rule<kw<"scrv"> +
             some(append<&ast::SpecialCurve::segments>(ref<TrimSegment>))> {
  };

  struct SpecialPoint
      : rule<kw<"sp"> + some(append<&ast::SpecialPoint::vp>(ref<INTEGER>))> {};

  struct FreeFormBodyItem
      : rule<ref<Parameter> | ref<OuterTrimmingLoop> |
             ref<InnerTrimmingLoop> | ref<SpecialCurve> |
             ref<SpecialPoint>> {};

  static constexpr auto blank = ref<NL> | ref<COMMENT>;
  static constexpr auto line_end =
      option(ref<COMMENT>) + (some(ref<NL>) | eof) + many(blank);

  struct FreeFormSurface
      : rule<assign<&ast::FreeFormSurface::head>(
                 ref<Curve> | ref<Curve2D> | ref<Surface>) +
             line_end +
             many(append<&ast::FreeFormSurface::body>(
                      ref<FreeFormBodyItem>) +
                  line_end) +
             (kw<"end"> | eof)> {};

  struct Connectivity
      : rule<kw<"con"> + assign<&ast::Connectivity::surf1>(ref<INTEGER>) +
             assign<&ast::Connectivity::q0_1>(ref<NUMBER>) +
             assign<&ast::Connectivity::q1_1>(ref<NUMBER>) +
             assign<&ast::Connectivity::curv2d1>(ref<INTEGER>) +
             assign<&ast::Connectivity::surf2>(ref<INTEGER>) +
             assign<&ast::Connectivity::q0_2>(ref<NUMBER>) +
             assign<&ast::Connectivity::q1_2>(ref<NUMBER>) +
             assign<&ast::Connectivity::curv2d2>(ref<INTEGER>)> {};

  struct Group
      : rule<kw<"g"> + many(append<&ast::Group::names>(ref<NAME>))> {};

  struct SmoothingGroup
      : rule<kw<"s"> +
             (flag<&ast::SmoothingGroup::off>(kw<"off">) |
              assign<&ast::SmoothingGroup::groupNumber>(ref<INTEGER>))> {};

  struct MergingGroup
      : rule<kw<"mg"> +
             (flag<&ast::MergingGroup::off>(kw<"off">) |
              (assign<&ast::MergingGroup::groupNumber>(ref<INTEGER>) +
               assign<&ast::MergingGroup::res>(ref<NUMBER>)))> {};

  struct ObjectName
      : rule<kw<"o"> + assign<&ast::ObjectName::name>(ref<OptionalRest>)> {};

  struct Bevel
      : rule<kw<"bevel"> + (flag<&ast::Bevel::on>(kw<"on">) | kw<"off">)> {
  };

  struct ColorInterpolation
      : rule<kw<"c_interp"> +
             (flag<&ast::ColorInterpolation::on>(kw<"on">) | kw<"off">)> {
  };

  struct DissolveInterpolation
      : rule<kw<"d_interp"> +
             (flag<&ast::DissolveInterpolation::on>(kw<"on">) |
              kw<"off">)> {};

  struct LevelOfDetail
      : rule<kw<"lod"> + assign<&ast::LevelOfDetail::level>(ref<INTEGER>)> {};

  struct MapLibrary
      : rule<kw<"maplib"> +
             many(append<&ast::MapLibrary::filenames>(ref<FILENAME>))> {};

  struct UseMap
      : rule<kw<"usemap"> + (flag<&ast::UseMap::off>(kw<"off">) |
                             assign<&ast::UseMap::name>(ref<NAME>))> {};

  struct MaterialLibrary
      : rule<kw<"mtllib"> + many(append<&ast::MaterialLibrary::filenames>(
                                ref<LIBRARY_FILENAME>))> {};

  struct UseMaterial : rule<kw<"usemtl"> + assign<&ast::UseMaterial::name>(
                                                   ref<OptionalRest>)> {};

  struct ShadowObject
      : rule<kw<"shadow_obj"> +
             assign<&ast::ShadowObject::filename>(ref<FILENAME>)> {};

  struct TraceObject
      : rule<kw<"trace_obj"> +
             assign<&ast::TraceObject::filename>(ref<FILENAME>)> {};

  struct CtechCparm : rule<kw<"ctech"> + kw<"cparm"> +
                               assign<&ast::CtechCparm::res>(ref<NUMBER>)> {};

  struct CtechCspace
      : rule<kw<"ctech"> + kw<"cspace"> +
             assign<&ast::CtechCspace::maxlength>(ref<NUMBER>)> {};

  struct CtechCurv : rule<kw<"ctech"> + kw<"curv"> +
                              assign<&ast::CtechCurv::maxdist>(ref<NUMBER>) +
                              assign<&ast::CtechCurv::maxangle>(ref<NUMBER>)> {
  };

  struct StechCparma : rule<kw<"stech"> + kw<"cparma"> +
                                assign<&ast::StechCparma::ures>(ref<NUMBER>) +
                                assign<&ast::StechCparma::vres>(ref<NUMBER>)> {
  };

  struct StechCparmb : rule<kw<"stech"> + kw<"cparmb"> +
                                assign<&ast::StechCparmb::uvres>(ref<NUMBER>)> {
  };

  struct StechCspace
      : rule<kw<"stech"> + kw<"cspace"> +
             assign<&ast::StechCspace::maxlength>(ref<NUMBER>)> {};

  struct StechCurv : rule<kw<"stech"> + kw<"curv"> +
                              assign<&ast::StechCurv::maxdist>(ref<NUMBER>) +
                              assign<&ast::StechCurv::maxangle>(ref<NUMBER>)> {
  };

  struct Call
      : rule<kw<"call"> + assign<&ast::Call::filename>(ref<FILENAME>) +
             many(append<&ast::Call::args>(ref<NON_WS>))> {};

  struct Csh : rule<kw<"csh"> + assign<&ast::Csh::command>(ref<FILENAME>) +
                        option(flag<&ast::Csh::dash>(lit<"-">)) +
                        many(append<&ast::Csh::args>(ref<NON_WS>))> {};

  struct Statement
      : rule<ref<Vertex> | ref<VertexNormal> | ref<VertexTexture> |
             ref<VertexParameter> | ref<Points> | ref<Lines> |
             ref<Faces> | ref<CurveSurfaceType> | ref<Degree> |
             ref<BasisMatrix> | ref<Step> | ref<FreeFormSurface> |
             ref<Connectivity> | ref<Group> | ref<SmoothingGroup> |
             ref<MergingGroup> | ref<ObjectName> | ref<Bevel> |
             ref<ColorInterpolation> | ref<DissolveInterpolation> |
             ref<LevelOfDetail> | ref<MapLibrary> | ref<UseMap> |
             ref<MaterialLibrary> | ref<UseMaterial> |
             ref<ShadowObject> | ref<TraceObject> |
             ref<CtechCparm> | ref<CtechCspace> | ref<CtechCurv> |
             ref<StechCparma> | ref<StechCparmb> |
             ref<StechCspace> | ref<StechCurv> | ref<Call> |
             ref<Csh>> {};

  static constexpr auto Known =
      kw<"v"> | kw<"vn"> | kw<"vt"> | kw<"vp"> | kw<"p"> | kw<"l"> | kw<"f"> |
      kw<"cstype"> | kw<"deg"> | kw<"bmat"> | kw<"step"> | kw<"curv"> |
      kw<"curv2"> | kw<"surf"> | kw<"con"> | kw<"g"> | kw<"s"> | kw<"mg"> |
      kw<"o"> | kw<"bevel"> | kw<"c_interp"> | kw<"d_interp"> | kw<"lod"> |
      kw<"maplib"> | kw<"usemap"> | kw<"mtllib"> | kw<"usemtl"> |
      kw<"shadow_obj"> | kw<"trace_obj"> | kw<"ctech"> | kw<"stech"> |
      kw<"call"> | kw<"csh">;
  struct OtherStatement
      : rule<!Known + create<ast::OtherStatement> +
             assign<&ast::OtherStatement::keyword>(ref<DIRECTIVE>) +
             assign<&ast::OtherStatement::arguments>(ref<OptionalRest>)> {};
  struct Doc : rule<create<ast::Document> + option(ref<BOM>) + many(blank) +
                        many(append<&ast::Document::statements>(
                                 ref<Statement> | ref<OtherStatement>) +
                             line_end)> {};

  using entry = Doc;
  using ignored = WS;
};

}  // namespace wavefront