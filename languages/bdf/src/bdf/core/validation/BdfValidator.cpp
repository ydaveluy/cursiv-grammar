#include "bdf/core/validation/BdfValidator.hpp"
#include <charconv>
#include <cstdint>
#include <cmath>
#include <format>
#include <memory>
#include "bdf/core/CoreServices.hpp"
#include "bdf/core/View.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace bdf {
namespace {
bool parse_count(std::string_view text, std::int64_t& out) {
  if (text.starts_with('+')) text.remove_prefix(1);
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), out);
  return error == std::errc() && end == text.data() + text.size();
}
}
void BdfValidator::checkPropertiesCount(const ast::Properties& properties,
                                        cursiv::ValidationAcceptor& accept) const {
  std::int64_t count = 0;
  if (!parse_count(properties.count, count) || count < 0) {
    accept.error(properties, "The property count needs a nonnegative integer.",
                 "bdf.property-count").property<&ast::Properties::count>();
  } else if (static_cast<std::uint64_t>(count) != properties.properties.size()) {
    accept.warning(properties,
                   std::format("STARTPROPERTIES declares {} but {} follow.", count, properties.properties.size()),
                   "bdf.property-count").property<&ast::Properties::count>();
  }
}
void BdfValidator::checkCharsCount(const ast::Font& font,
                                   cursiv::ValidationAcceptor& accept) const {
  for (const std::string_view keyword : {"FONT", "SIZE", "FONTBOUNDINGBOX", "CHARS"})
    if (!find_directive(font.parts, keyword))
      accept.error(font, std::format("The font needs a {} declaration.", keyword),
                   "bdf.required-header").property<&ast::Font::parts>();
  const auto* line = find_directive(font.parts, "CHARS");
  if (!line || line->arguments.size() != 1) return;
  std::int64_t count = 0;
  if (!parse_count(argument(line, 0), count) || count < 0) {
    accept.error(*line, "The character count needs a nonnegative integer.",
                 "bdf.character-count").property<&ast::Directive::arguments>();
    return;
  }
  std::size_t actual = 0;
  for (const auto& part : font.parts)
    actual += std::holds_alternative<ast::Char*>(part);
  if (static_cast<std::uint64_t>(count) != actual)
    accept.warning(*line, std::format("CHARS declares {} but {} follow.", count, actual),
                   "bdf.character-count").property<&ast::Directive::arguments>();
}
void BdfValidator::checkDirective(const ast::Directive& line,
                                  cursiv::ValidationAcceptor& accept) const {
  if (line.keyword == "FONT") return;
  const bool scalable = line.keyword == "SWIDTH" || line.keyword == "SWIDTH1";
  const bool metric = scalable || line.keyword == "DWIDTH" ||
      line.keyword == "DWIDTH1" || line.keyword == "VVECTOR";
  const std::size_t minimum = line.keyword == "SIZE" ? 3 :
      line.keyword == "BBX" || line.keyword == "FONTBOUNDINGBOX" ? 4 : metric ? 2 : 1;
  const std::size_t maximum = line.keyword == "SIZE" ? 4 :
      line.keyword == "ENCODING" ? 2 : minimum;
  if (line.arguments.size() < minimum || line.arguments.size() > maximum) {
    accept.error(line, "The declaration has an invalid number of arguments.",
                 "bdf.directive-arity").property<&ast::Directive::arguments>();
    return;
  }
  for (auto value : line.arguments) {
    bool valid = false;
    if (scalable) {
      if (value.starts_with('+')) value.remove_prefix(1);
      double number = 0;
      const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
      valid = error == std::errc{} && end == value.data() + value.size() &&
              std::isfinite(number);
    } else {
      std::int64_t number = 0;
      valid = parse_count(value, number);
    }
    if (!valid) {
      accept.error(line, scalable ? "A scalable width needs a finite decimal number."
                                 : "A numeric argument needs a representable decimal integer.",
                   "bdf.number").property<&ast::Directive::arguments>();
      break;
    }
  }
  if (line.keyword == "METRICSSET" && line.arguments.size() == 1) {
    std::int64_t mode = 0;
    if (parse_count(argument(&line, 0), mode) && (mode < 0 || mode > 2))
      accept.error(line, "METRICSSET must be 0, 1 or 2.", "bdf.metrics-set")
          .property<&ast::Directive::arguments>();
  }
  if (line.keyword == "SIZE" && line.arguments.size() == 4) {
    const auto bits = argument(&line, 3);
    if (bits != "1" && bits != "2" && bits != "4" && bits != "8")
      accept.error(line, "Bits per pixel must be 1, 2, 4 or 8.",
                   "bdf.bits-per-pixel").property<&ast::Directive::arguments>();
  }
}
void BdfValidator::checkBitmap(const ast::Char& glyph,
                               cursiv::ValidationAcceptor& accept) const {
  const auto* image = bitmap(glyph);
  const auto* box = find_directive(glyph.parts, "BBX");
  const auto* font = cursiv::ast_cast<ast::Font>(glyph.parent);
  for (const std::string_view keyword : {"ENCODING", "BBX"})
    if (!find_directive(glyph.parts, keyword))
      accept.error(glyph, std::format("The glyph needs a {} declaration.", keyword),
                   "bdf.required-metric").property<&ast::Char::parts>();
  if (!image)
    accept.error(glyph, "The glyph needs a BITMAP block.", "bdf.required-metric")
        .property<&ast::Char::parts>();
  std::int64_t mode = 0;
  if (font) {
    const auto text = argument(find_directive(font->parts, "METRICSSET"), 0);
    if (!text.empty()) parse_count(text, mode);
  }
  const auto require = [&](std::string_view keyword) {
    if (!find_directive(glyph.parts, keyword) &&
        !(font && find_directive(font->parts, keyword)))
      accept.error(glyph, std::format("The glyph needs a local or global {} metric.", keyword),
                   "bdf.required-metric").property<&ast::Char::parts>();
  };
  if (mode == 0 || mode == 2) { require("SWIDTH"); require("DWIDTH"); }
  if (mode == 1 || mode == 2) {
    require("SWIDTH1"); require("DWIDTH1"); require("VVECTOR");
  }
  if (!image || !box) return;
  std::int64_t height = 0;
  if (parse_count(argument(box, 1), height) &&
      (height < 0 || static_cast<std::uint64_t>(height) != image->rows.size()))
    accept.error(*box, "The bitmap row count must match the BBX height.",
                 "bdf.bitmap-height").property<&ast::Directive::arguments>();
  std::int64_t width = 0, bits = 1;
  if (font) {
    const auto value = argument(find_directive(font->parts, "SIZE"), 3);
    if (!value.empty() && !parse_count(value, bits)) bits = 0;
  }
  const bool haveWidth = parse_count(argument(box, 0), width) &&
      width >= 0 && width <= 65535 &&
      (bits == 1 || bits == 2 || bits == 4 || bits == 8);
  const auto digits = haveWidth ? static_cast<std::uint64_t>((width * bits + 7) / 8) * 2 : 0;
  for (auto row : image->rows) {
    while (!row.empty() && row.back() == ' ') row.remove_suffix(1);
    if (row.empty() || row.find_first_not_of("0123456789abcdefABCDEF") != row.npos) {
      accept.error(*image, "Bitmap rows must contain hexadecimal digits.",
                   "bdf.bitmap-digits").property<&ast::Bitmap::rows>();
      break;
    }
    if (haveWidth && row.size() != digits) {
      accept.error(*image, "The bitmap row width must match BBX and bits per pixel.",
                   "bdf.bitmap-width").property<&ast::Bitmap::rows>();
      break;
    }
  }
}
void register_validation_checks(BdfCoreServices& services) {
  using V = BdfValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkPropertiesCount>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkCharsCount>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkDirective>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkBitmap>(*services.validator)});
}
}  // namespace bdf
