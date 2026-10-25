#include "wavefront/core/validation/WavefrontValidator.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "wavefront/core/CoreServices.hpp"

namespace wavefront {
namespace {
template <auto Member>
void finite(const auto& n, cursiv::ValidationAcceptor& a) {
  const auto& v = n.*Member;
  bool valid;
  if constexpr (requires { v.has_value(); })
    valid = !v || std::isfinite(*v);
  else
    valid = std::isfinite(v);
  if (!valid)
    a.error(n, "a coordinate must be finite").template property<Member>();
}
template <auto Member>
void index(const ast::ElementRef& n, std::string_view text, std::size_t seen,
           std::size_t total, cursiv::ValidationAcceptor& a) {
  if (text.empty()) return;
  if (text.front() == '+') text.remove_prefix(1);
  std::int64_t value = 0;
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), value);
  const auto distance =
      value < 0 ? std::uint64_t(-(value + 1)) + 1 : std::uint64_t(value);
  if (error != std::errc{} || end != text.data() + text.size() || !value ||
      distance > (value < 0 ? seen : total))
    a.error(n, "an index must reference an existing element; zero is invalid")
        .template property<Member>();
}
}  // namespace

void WavefrontValidator::checkVertex(const ast::Vertex& n,
                                     cursiv::ValidationAcceptor& a) const {
  finite<&ast::Vertex::x>(n, a);
  finite<&ast::Vertex::y>(n, a);
  finite<&ast::Vertex::z>(n, a);
  finite<&ast::Vertex::w>(n, a);
  if (!n.color.empty() && n.color.size() != 3 && n.color.size() != 4)
    a.error(n, "a vertex color has three or four components")
        .property<&ast::Vertex::color>();
  for (double c : n.color)
    if (!std::isfinite(c)) {
      a.error(n, "a color component must be finite")
          .property<&ast::Vertex::color>();
      break;
    }
}

void WavefrontValidator::checkNormal(const ast::VertexNormal& n,
                                     cursiv::ValidationAcceptor& a) const {
  finite<&ast::VertexNormal::x>(n, a);
  finite<&ast::VertexNormal::y>(n, a);
  finite<&ast::VertexNormal::z>(n, a);
}
void WavefrontValidator::checkTexture(const ast::VertexTexture& n,
                                      cursiv::ValidationAcceptor& a) const {
  finite<&ast::VertexTexture::u>(n, a);
  finite<&ast::VertexTexture::v>(n, a);
  finite<&ast::VertexTexture::w>(n, a);
}
void WavefrontValidator::checkParameter(const ast::VertexParameter& n,
                                        cursiv::ValidationAcceptor& a) const {
  finite<&ast::VertexParameter::u>(n, a);
  finite<&ast::VertexParameter::v>(n, a);
  finite<&ast::VertexParameter::w>(n, a);
}

void WavefrontValidator::checkDocument(const ast::Document& n,
                                       cursiv::ValidationAcceptor& a) const {
  std::array<std::size_t, 3> total{}, seen{};
  auto category = [](const ast::Statement* s) -> int {
    if (cursiv::ast_cast<ast::Vertex>(s)) return 0;
    if (cursiv::ast_cast<ast::VertexTexture>(s)) return 1;
    if (cursiv::ast_cast<ast::VertexNormal>(s)) return 2;
    return -1;
  };
  for (const auto* s : n.statements)
    if (const int i = category(s); i >= 0) ++total[i];
  auto refs = [&](const auto& values) {
    for (const auto* v : values) {
      index<&ast::ElementRef::vertex>(*v, v->vertex, seen[0], total[0], a);
      if (v->texture)
        index<&ast::ElementRef::texture>(*v, *v->texture, seen[1], total[1], a);
      if (v->normal)
        index<&ast::ElementRef::normal>(*v, *v->normal, seen[2], total[2], a);
    }
  };
  for (const auto* s : n.statements) {
    if (const int i = category(s); i >= 0) ++seen[i];
    if (const auto* p = cursiv::ast_cast<ast::Faces>(s)) {
      if (p->v.size() < 3)
        a.error(*p, "a face needs at least three vertices")
            .property<&ast::Faces::v>();
      refs(p->v);
    } else if (const auto* p = cursiv::ast_cast<ast::Lines>(s)) {
      if (p->v.size() < 2)
        a.error(*p, "a line needs at least two vertices")
            .property<&ast::Lines::v>();
      refs(p->v);
    } else if (const auto* p = cursiv::ast_cast<ast::Points>(s)) {
      if (p->v.empty())
        a.error(*p, "a point statement needs at least one vertex")
            .property<&ast::Points::v>();
      refs(p->v);
    } else if (const auto* p = cursiv::ast_cast<ast::FreeFormSurface>(s)) {
      if (const auto* h = cursiv::ast_cast<ast::Surface>(p->head)) refs(h->v);
    }
  }
}

void register_validation_checks(WavefrontCoreServices& services) {
  services.validator = std::make_unique<const WavefrontValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &WavefrontValidator::checkDocument>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &WavefrontValidator::checkVertex>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &WavefrontValidator::checkNormal>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &WavefrontValidator::checkTexture>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &WavefrontValidator::checkParameter>(*services.validator)});
}
}  // namespace wavefront
