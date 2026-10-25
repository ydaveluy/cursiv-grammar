#include "pbm/core/validation/PbmValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pbm/core/Bitmap.hpp"
#include "pbm/core/CoreServices.hpp"
namespace pbm {
void PbmValidator::checkImage(const ast::Image &image,
                              cursiv::ValidationAcceptor &accept) const {
  const auto bitmap = read_bitmap(image);
  if (!bitmap) {
    auto diagnostic = accept.error(image, bitmap.message, bitmap.code);
    if (bitmap.code == "pbm.magic")
      diagnostic.property<&ast::Image::magic>();
    else if (bitmap.code == "pbm.dimension") {
      if (!dimension(image.width))
        diagnostic.property<&ast::Image::width>();
      else
        diagnostic.property<&ast::Image::height>();
    } else if (bitmap.code == "pbm.header")
      diagnostic.property<&ast::Image::separator>();
    else
      diagnostic.property<&ast::Image::raster>();
  }
}
void PbmValidator::checkDocument(const ast::Document &doc,
                                 cursiv::ValidationAcceptor &accept) const {
  if (doc.images.empty())
    accept.error(doc, "A PBM stream needs at least one image.", "pbm.empty");
  if (!doc.trailing.empty())
    accept
        .error(
            doc,
            "Unexpected bytes after the raster; expected the next PBM image.",
            "pbm.trailing")
        .property<&ast::Document::trailing>();
}
void register_validation_checks(PbmCoreServices &services) {
  using V = PbmValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkImage>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(
           *services.validator)});
}
} // namespace pbm
