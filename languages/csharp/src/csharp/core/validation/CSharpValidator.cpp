#include "csharp/core/validation/CSharpValidator.hpp"

#include <memory>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

#include "cursiv/core/grammar/unicode/UnicodeRange.hpp"
#include "cursiv/core/grammar/unicode/UnicodeRanges.hpp"
#include "cursiv/core/text/scalar_at.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "csharp/core/CoreServices.hpp"

namespace csharp {
namespace {
bool rawWhitespace(std::string_view text) {
  for (std::size_t at = 0, width = 0; at < text.size(); at += width) {
    const auto scalar = cursiv::scalar_at(text.data() + at, text.size() - at, width);
    if (scalar != '\t' && scalar != '\v' && scalar != '\f' &&
        scalar != 0x1a && scalar != 0xfeff &&
        !cursiv::unicode::within(cursiv::unicode::kZs,
                                std::size(cursiv::unicode::kZs), scalar))
      return false;
  }
  return true;
}

std::pair<std::size_t, std::size_t> rawLineEnd(std::string_view text,
                                             std::size_t begin) {
  for (auto at = begin; at < text.size(); ++at) {
    if (text[at] == '\r')
      return {at, at + 1 < text.size() && text[at + 1] == '\n' ? 2 : 1};
    if (text[at] == '\n') return {at, 1};
    if (text.substr(at, 2) == "\xc2\x85") return {at, 2};
    if (text.substr(at, 3) == "\xe2\x80\xa8" ||
        text.substr(at, 3) == "\xe2\x80\xa9") return {at, 3};
  }
  return {std::string_view::npos, 0};
}

bool rawLayout(std::string_view body) {
  const auto [first, width] = rawLineEnd(body, 0);
  if (first == std::string_view::npos) return true;
  if (!rawWhitespace(body.substr(0, first))) return false;
  auto last = first;
  auto lastWidth = width;
  for (auto next = rawLineEnd(body, first + width);
       next.first != std::string_view::npos;
       next = rawLineEnd(body, next.first + next.second)) {
    last = next.first;
    lastWidth = next.second;
  }
  // The opener's and closer's line endings surround at least one content line.
  if (first == last) return false;
  const auto indentation = body.substr(last + lastWidth);
  if (!rawWhitespace(indentation)) return false;
  for (auto at = first + width; at <= last;) {
    const auto [end, length] = rawLineEnd(body, at);
    const auto line = body.substr(at, end - at);
    if (!line.starts_with(indentation) &&
        !(rawWhitespace(line) && indentation.starts_with(line))) return false;
    if (end == last) break;
    at = end + length;
  }
  return true;
}
}  // namespace

void CSharpValidator::checkRawLiteral(const ast::LiteralExpr& literal,
                                      cursiv::ValidationAcceptor& accept) const {
  auto text = literal.text;
  if (!text.starts_with("\"\"\"")) return;
  if (text.ends_with("u8") || text.ends_with("U8")) text.remove_suffix(2);
  const auto first = text.find_first_not_of('"');
  const auto last = text.find_last_not_of('"');
  if (first == std::string_view::npos || last < first ||
      text.size() - last - 1 != first ||
      text.substr(first, last - first + 1).find(std::string(first, '"')) !=
          std::string_view::npos) {
    accept.error(literal, "Raw string delimiters must be equal maximal quote runs.",
                 "csharp.raw-delimiter").property<&ast::LiteralExpr::text>();
    return;
  }
  if (!rawLayout(text.substr(first, text.size() - 2 * first)))
    accept.error(literal, "Invalid raw string line layout or indentation.",
                 "csharp.raw-layout").property<&ast::LiteralExpr::text>();
}

void register_validation_checks(CSharpCoreServices& services) {
  services.validator = std::make_unique<const CSharpValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&CSharpValidator::checkRawLiteral>(
          *services.validator)});
}

}  // namespace csharp
