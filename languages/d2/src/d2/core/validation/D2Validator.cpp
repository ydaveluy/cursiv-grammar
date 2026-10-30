#include "d2/core/validation/D2Validator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "d2/core/CoreServices.hpp"

namespace d2 {

namespace {
std::string_view trim_native_space(std::string_view text) {
  // Go's unicode.IsSpace, as used by the native parser's peekNotSpace.
  constexpr std::string_view spaces[] = {
      " ", "\t", "\n", "\v", "\f", "\r", "\xC2\x85", "\u00a0", "\u1680",
      "\u2000", "\u2001", "\u2002", "\u2003", "\u2004", "\u2005", "\u2006",
      "\u2007", "\u2008", "\u2009", "\u200a", "\u2028", "\u2029", "\u202f",
      "\u205f", "\u3000"};
  for (;;) {
    bool removed = false;
    for (const auto space : spaces)
      if (text.starts_with(space)) {
        text.remove_prefix(space.size());
        removed = true;
        break;
      }
    if (!removed) return text;
  }
}
}  // namespace

void D2Validator::checkChunk(const ast::Chunk& node,
                             cursiv::ValidationAcceptor& accept) const {
  const auto* text = cursiv::ast_cast<ast::Text>(node.parent);
  if (!text || cursiv::ast_cast<ast::SingleText>(text) ||
      cursiv::ast_cast<ast::BlockText>(text) ||
      cursiv::ast_cast<ast::KeyPath>(text->parent)) return;
  for (std::size_t i = 0; i < node.raw.size(); ++i) {
    if (node.raw[i] == '\\' && i + 1 < node.raw.size()) {
      ++i;
      continue;
    }
    if (node.raw[i] != '$') continue;
    // Valid substitutions are separate AST parts, never raw chunks. The
    // native unquoted reader also tolerates a final dollar after text when
    // it reaches a line ending/EOF; retain that behavior here.
    if (!trim_native_space(node.raw.substr(i + 1)).empty() ||
        cursiv::ast_cast<ast::QuotedText>(text) || text->parts.back() != &node ||
        (i == 0 && text->parts.size() == 1)) {
      accept.error(node, "A substitution must begin with { after $.",
                   "d2.substitution").property<&ast::Chunk::raw>();
      return;
    }
  }
}

void register_validation_checks(D2CoreServices& services) {
  services.validator = std::make_unique<const D2Validator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&D2Validator::checkChunk>(
          *services.validator)});
}

}  // namespace d2
