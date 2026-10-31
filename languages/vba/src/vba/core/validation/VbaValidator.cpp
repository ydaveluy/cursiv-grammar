#include "vba/core/validation/VbaValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "vba/core/CoreServices.hpp"

namespace vba {
namespace {
std::size_t line_end(std::string_view text, std::size_t at) {
  if (at >= text.size()) return 0;
  if (text[at] == '\r') return at + 1 < text.size() && text[at + 1] == '\n' ? 2 : 1;
  if (text[at] == '\n') return 1;
  if (text.substr(at).starts_with("\xe2\x80\xa8") ||
      text.substr(at).starts_with("\xe2\x80\xa9")) return 3;
  return 0;
}
std::size_t horizontal_space(std::string_view text, std::size_t at) {
  if (at >= text.size()) return 0;
  if (text[at] == ' ' || text[at] == '\t' || text[at] == '\x19') return 1;
  return text.substr(at).starts_with("\xe3\x80\x80") ? 3 : 0;
}
}  // namespace

void VbaValidator::checkCompilationConst(const ast::CompilationConst& node,
                                        cursiv::ValidationAcceptor& accept) const {
  const auto* document = node.getDocument();
  if (!document) return;
  const auto source = document->textDocument().getText();
  const auto span = accept.spanOf(node);
  if (span.begin > span.end || span.end > source.size()) return;
  auto start = std::size_t(span.begin);
  while (start > 0 && source[start - 1] != '\r' && source[start - 1] != '\n') {
    if (start >= 3 &&
        (source.substr(start - 3, 3) == "\xe2\x80\xa8" ||
         source.substr(start - 3, 3) == "\xe2\x80\xa9")) break;
    --start;
  }
  auto at = start;
  while (at < span.begin) {
    const auto width = horizontal_space(source, at);
    if (!width || width > span.begin - at) break;
    at += width;
  }
  if (at != span.begin)
    accept.error(node, "#Const must start a logical source line, before any statement or label.",
                 "vba.cc-line-start");
  if (!node.value) return;
  at = accept.spanOf(*node.value).end;
  if (at > source.size()) return;
  while (at < source.size()) {
    while (const auto width = horizontal_space(source, at)) at += width;
    if (at == source.size() || line_end(source, at) || source[at] == '\'') return;
    // A continuation is whitespace inside this logical line, including
    // after the last expression token and before its optional apostrophe.
    if (source[at] == '_') {
      auto end = at + 1;
      while (const auto width = horizontal_space(source, end)) end += width;
      if (const auto width = line_end(source, end)) { at = end + width; continue; }
    }
    accept.error(node, "Only a single-quote comment may follow a #Const expression.",
                 "vba.cc-line-end");
    return;
  }
}

void register_validation_checks(VbaCoreServices& services) {
  services.validator = std::make_unique<const VbaValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&VbaValidator::checkCompilationConst>(*services.validator)});
}

}  // namespace vba
