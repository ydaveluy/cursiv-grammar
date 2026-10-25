#include "stl/core/validation/StlValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "stl/core/Coordinates.hpp"
#include "stl/core/CoreServices.hpp"
namespace stl {
void StlValidator::checkDocument(const ast::Document &doc,
                                 cursiv::ValidationAcceptor &accept) const {
  if (doc.binary) {
    if (doc.header.size() != 80 || doc.countBytes.size() != 4) {
      accept.error(doc,
                   "A binary STL header needs 80 bytes followed by a four-byte "
                   "triangle count.",
                   "stl.header");
      return;
    }
    std::uint64_t facets = 0;
    for (const auto *solid : doc.solids)
      facets += solid->facets.size();
    if (facets != little_u32(doc.countBytes) || !doc.trailing.empty())
      accept
          .error(doc,
                 "The binary raster needs 50 bytes for every declared facet.",
                 "stl.length")
          .property<&ast::Document::countBytes>();
  } else if (doc.solids.empty())
    accept.error(doc, "An ASCII STL needs a solid.", "stl.empty");
}
void StlValidator::checkSolid(const ast::Solid &solid,
                              cursiv::ValidationAcceptor &accept) const {
  const auto *doc = cursiv::ast_cast<ast::Document>(solid.parent);
  if (doc && !doc->binary && !solid.hasEnd)
    accept.error(solid, "The solid needs an endsolid marker.", "stl.end");
}
void StlValidator::checkFacet(const ast::Facet &facet,
                              cursiv::ValidationAcceptor &accept) const {
  if (!facet.normal)
    accept.error(facet, "A facet needs a normal vector.", "stl.normal");
  if (facet.vertices.size() != 3)
    accept
        .error(facet, "A triangular facet needs exactly three vertices.",
               "stl.vertices")
        .property<&ast::Facet::vertices>();
}
void StlValidator::checkTriple(const ast::Triple &triple,
                               cursiv::ValidationAcceptor &accept) const {
  if (!coordinate(triple.i, triple.binary))
    accept.error(triple, "Invalid coordinate number.", "stl.number")
        .property<&ast::Triple::i>();
  if (!coordinate(triple.j, triple.binary))
    accept.error(triple, "Invalid coordinate number.", "stl.number")
        .property<&ast::Triple::j>();
  if (!coordinate(triple.k, triple.binary))
    accept.error(triple, "Invalid coordinate number.", "stl.number")
        .property<&ast::Triple::k>();
}
void register_validation_checks(StlCoreServices &services) {
  using V = StlValidator;
  services.validator = std::make_unique<const V>(services);
  const auto &v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSolid>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFacet>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTriple>(v)});
}
} // namespace stl
