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
      if (text.starts_with(prefix) && text.size() > prefix.size() &&
          (text[prefix.size()] == ' ' || text[prefix.size()] == '\t')) {
        text.remove_prefix(prefix.size());
        const auto begin = text.find_first_not_of(" \t");
        if (begin != text.npos) {
          text.remove_prefix(begin);
          text = text.substr(0, text.find_last_not_of(" \t") + 1);
          if (text.starts_with('3')) {
            text.remove_prefix(1);
            bool complete = true;
            for (int part = 0; part < 2 && text.starts_with('.'); ++part) {
              text.remove_prefix(1);
              const auto end = text.find_first_not_of("0123456789");
              if (text.empty() || end == 0) { complete = false; break; }
              if (end == text.npos) { text = {}; break; }
              text.remove_prefix(end);
            }
            version = complete && text.empty();
          }
        }
      }
    }
  if (!version)
    accept.error(
        doc, "A GFF3 document must begin with a gff-version 3 directive.",
        "gff3.version");
}

void Gff3Validator::checkFeature(const ast::Feature &feature,
                                 cursiv::ValidationAcceptor &accept) const {
  if (!feature.hasAttributeColumn || !feature.extra.empty())
    accept.error(feature,
                 "A GFF3 feature needs exactly nine tab-separated columns.",
                 "gff3.columns");
  if (!encoded_field(feature.seqid, true))
    accept.error(feature, "Sequence ID characters outside the GFF3 set must be percent-encoded.",
                 "gff3.seqid").property<&ast::Feature::seqid>();
  if (feature.source.empty() || feature.type.empty())
    accept.error(feature, "An undefined source or type must be written as '.'.",
                 "gff3.field");
  if (!encoded_field(feature.source) || !encoded_field(feature.type))
    accept.error(feature, "Literal controls and percent signs in fields must be percent-encoded.",
                 "gff3.field-escape");
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
  else if ((feature.type == "CDS" || feature.type == "SO:0000316") &&
           feature.phase == ".")
    accept.error(feature, "A CDS feature requires phase 0, 1 or 2.",
                 "gff3.cds-phase").property<&ast::Feature::phase>();
}

void Gff3Validator::checkAttribute(const ast::Attribute &attribute,
                                   cursiv::ValidationAcceptor &accept) const {
  if (!encoded_field(attribute.key))
    accept.error(attribute, "Control characters must be encoded and percent escapes need two hexadecimal digits.",
                 "gff3.escape").property<&ast::Attribute::key>();
  if (attribute.key.find_first_of(",&") != attribute.key.npos)
    accept.error(attribute, "Reserved punctuation in attribute names must be percent-encoded.",
                 "gff3.attribute-key").property<&ast::Attribute::key>();
  if (!attribute.hasEquals)
    accept.error(attribute, "A GFF3 attribute needs '=' and a value.",
                 "gff3.attribute");
  if (attribute.values.size() == 1 && attribute.values[0].empty())
    accept.error(attribute, "A GFF3 attribute value is empty.",
                 "gff3.attribute");
  for (auto value : attribute.values) {
    if (!encoded_field(value))
      accept
          .error(attribute, "Control characters must be encoded and percent escapes need two hexadecimal digits.",
                 "gff3.escape")
          .property<&ast::Attribute::values>();
    if (value.find_first_of("=&") != value.npos)
      accept
          .error(attribute,
                 "An '=' or '&' inside an attribute value must be percent-encoded.",
                 "gff3.attribute-value")
          .property<&ast::Attribute::values>();
  }
  // Reserved attribute names are case-sensitive, including when escaped.
  const auto key = attribute_value(attribute.key);
  if (!key || (*key != "Target" && *key != "Gap")) return;
  bool valid = attribute.values.size() == 1;
  if (valid) {
    const auto parts = words(attribute.values[0]);
    if (*key == "Target") {
      valid = (parts.size() == 3 || parts.size() == 4) &&
              ordered_coordinates(parts[1], parts[2]) &&
              (parts.size() == 3 || parts[3] == "+" || parts[3] == "-");
    } else {
      valid = !parts.empty();
      for (auto part : parts) {
        valid &= part.size() >= 2 &&
                 std::string_view("MIDFR").find(part.front()) != std::string_view::npos &&
                 part.substr(1).find_first_not_of("0123456789") == part.npos;
      }
    }
  }
  if (!valid)
    accept.error(attribute, *key == "Target"
                    ? "Target needs an ID, positive ordered coordinates and an optional '+' or '-' strand."
                    : "Gap needs space-separated M, I, D, F or R operations with decimal lengths.",
                 *key == "Target" ? "gff3.target" : "gff3.gap")
        .property<&ast::Attribute::values>();
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
