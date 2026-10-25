#include "mps/core/validation/MpsValidator.hpp"

#include <memory>
#include <cstdlib>
#include <string>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "mps/core/CoreServices.hpp"

namespace mps {
void MpsValidator::checkModel(const ast::Model& node,
                             cursiv::ValidationAcceptor& accept) const {
  const auto numeric = [](std::string_view text) {
    std::string normalized(text);
    for (char& c : normalized) if (c == 'D' || c == 'd') c = 'E';
    char* end = nullptr;
    std::strtod(normalized.c_str(), &end);
    return end != normalized.c_str() && end == normalized.c_str() + normalized.size();
  };
  const auto checkCard = [&](const ast::ColumnCard& card) {
    if (card.entries.size() > 2)
      accept.error(card, "A column card has at most two entries.", "mps.card-entries");
    for (const auto* entry : card.entries)
      if (!numeric(entry->value))
        accept.error(*entry, "A numeric coefficient is required.", "mps.coefficient")
            .property<&ast::Entry::value>();
  };
  for (const auto* row : node.rows)
    if (row->type != "E" && row->type != "L" && row->type != "G" && row->type != "N")
      accept.error(*row, "Unknown row type.", "mps.row-type")
          .property<&ast::RowDecl::type>();
  bool integral = false;
  for (const auto* item : node.columns) {
    if (const auto* marker = cursiv::ast_cast<ast::Marker>(item)) {
      const bool open = marker->kind == "INTORG";
      if ((marker->kind != "INTORG" && marker->kind != "INTEND") || integral == open)
        accept.error(*marker, "Expected the other integer marker.", "mps.marker");
      integral = open;
    } else if (const auto* card = cursiv::ast_cast<ast::ColumnCard>(item)) {
      checkCard(*card);
    }
  }
  if (integral) accept.error(node, "The integer marker is not closed.", "mps.marker");
  for (const auto* section : node.quadratic) {
    for (const auto* card : section->cards) checkCard(*card);
    if ((section->kind == "QSECTION" || section->kind == "QCMATRIX") && !section->row)
      accept.error(*section, "A quadratic row name is required.", "mps.quadratic-row");
  }
  const auto checkData = [&](const ast::DataCard& card) {
    if (card.fields.size() < 2 || card.fields.size() > 5)
      accept.error(card, "An RHS or range card needs one or two entries.", "mps.card-entries");
    for (std::size_t i = card.fields.size() % 2 + 1; i < card.fields.size(); i += 2)
      if (!numeric(card.fields[i]))
        accept.error(card, "A numeric value is required.", "mps.coefficient");
  };
  for (const auto* card : node.rhs) checkData(*card);
  for (const auto* card : node.ranges) checkData(*card);
  for (const auto* card : node.bounds) {
    const bool valued = card->key == "UP" || card->key == "LO" || card->key == "FX" ||
        card->key == "LI" || card->key == "UI" || card->key == "SC" || card->key == "SI";
    const bool unvalued = card->key == "FR" || card->key == "BV" || card->key == "MI" || card->key == "PL";
    if (!valued && !unvalued)
      accept.error(*card, "Unknown bound type.", "mps.bound-type");
    else if (card->fields.size() < (valued ? 2u : 1u) || card->fields.size() > (valued ? 3u : 2u))
      accept.error(*card, "This bound has missing or extra fields.", "mps.bound-fields");
    else if (valued && !numeric(card->fields.back()))
      accept.error(*card, "A numeric bound value is required.", "mps.bound-value");
  }
}
void register_validation_checks(MpsCoreServices& services) {
  services.validator = std::make_unique<const MpsValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&MpsValidator::checkModel>(*services.validator)});
}
}  // namespace mps
