#include "gff3/core/validation/Gff3Validator.hpp"

#include <charconv>
#include <cmath>
#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "gff3/core/CoreServices.hpp"
#include "gff3/core/Value.hpp"

namespace gff3 {

void Gff3Validator::checkDocument(const ast::Document &doc,
                                  cursiv::ValidationAcceptor &accept) const {
  bool version = false;
  if (!doc.lines.empty())
    if (const auto *line = std::get_if<ast::CommentLine *>(&doc.lines[0]);
        line && *line) {
      auto text = (*line)->text;
      constexpr std::string_view prefix = "##gff-version";
      if (text.starts_with(prefix)) {
        text.remove_prefix(prefix.size());
        const auto begin = text.find_first_not_of(" \t");
        if (begin != text.npos) {
          text.remove_prefix(begin);
          version =
              text.front() == '3' && (text.size() == 1 || text[1] == '.' ||
                                      text[1] == ' ' || text[1] == '\t');
        }
      }
    }
  if (!version)
    accept.warning(
        doc, "A GFF3 document should begin with a gff-version 3 directive.",
        "gff3.version");
}

void Gff3Validator::checkFeature(const ast::Feature &feature,
                                 cursiv::ValidationAcceptor &accept) const {
  if (!feature.hasAttributeColumn || !feature.extra.empty())
    accept.error(feature,
                 "A GFF3 feature needs exactly nine tab-separated columns.",
                 "gff3.columns");
  const auto start = decimal(feature.start), end = decimal(feature.end);
  if (!start || *start == "0")
    accept
        .error(feature, "Feature start must be a positive decimal coordinate.",
               "gff3.coordinate")
        .property<&ast::Feature::start>();
  if (!end || *end == "0")
    accept
        .error(feature, "Feature end must be a positive decimal coordinate.",
               "gff3.coordinate")
        .property<&ast::Feature::end>();
  if (start && end &&
      (start->size() > end->size() ||
       (start->size() == end->size() && *start > *end)))
    accept.error(feature, "Feature start exceeds its end.", "gff3.range")
        .property<&ast::Feature::start>();
  if (feature.score != ".") {
    auto text = feature.score;
    if (text.starts_with('+'))
      text.remove_prefix(1);
    double score = 0;
    const auto parsed =
        text.empty()
            ? std::from_chars_result{text.data(), std::errc::invalid_argument}
            : std::from_chars(text.data(), text.data() + text.size(), score);
    if (parsed.ec != std::errc{} ||
        (!text.empty() && parsed.ptr != text.data() + text.size()) ||
        !std::isfinite(score))
      accept
          .error(feature, "Feature score must be a finite number or '.'.",
                 "gff3.score")
          .property<&ast::Feature::score>();
  }
  if (feature.strand.size() != 1 ||
      std::string_view("+-.?").find(feature.strand[0]) ==
          std::string_view::npos)
    accept
        .error(feature, "Feature strand must be '+', '-', '.', or '?'.",
               "gff3.strand")
        .property<&ast::Feature::strand>();
  if (feature.phase.size() != 1 ||
      std::string_view("012.").find(feature.phase[0]) == std::string_view::npos)
    accept
        .error(feature, "Feature phase must be 0, 1, 2, or '.'.", "gff3.phase")
        .property<&ast::Feature::phase>();
}

void Gff3Validator::checkAttribute(const ast::Attribute &attribute,
                                   cursiv::ValidationAcceptor &accept) const {
  if (!attribute.hasEquals)
    accept.error(attribute, "A GFF3 attribute needs '=' and a value.",
                 "gff3.attribute");
  if (attribute.values.size() == 1 && attribute.values[0].empty())
    accept.error(attribute, "A GFF3 attribute value is empty.",
                 "gff3.attribute");
  for (auto value : attribute.values) {
    if (!attribute_value(value))
      accept
          .error(attribute, "A percent escape needs two hexadecimal digits.",
                 "gff3.escape")
          .property<&ast::Attribute::values>();
    if (value.find('=') != value.npos)
      accept
          .error(attribute,
                 "An '=' inside an attribute value must be percent-encoded.",
                 "gff3.attribute-value")
          .property<&ast::Attribute::values>();
  }
}

void register_validation_checks(Gff3CoreServices &services) {
  services.validator = std::make_unique<const Gff3Validator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &Gff3Validator::checkDocument>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &Gff3Validator::checkFeature>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &Gff3Validator::checkAttribute>(*services.validator)});
}

} // namespace gff3
