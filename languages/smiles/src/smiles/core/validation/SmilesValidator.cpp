#include "smiles/core/validation/SmilesValidator.hpp"

#include <memory>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "smiles/core/AtomicSymbols.hpp"
#include "smiles/core/CoreServices.hpp"

namespace smiles {

namespace {
std::optional<std::uint32_t> decimal(std::string_view text,
                                    std::uint32_t limit) {
  std::uint32_t value = 0;
  for (char c : text) {
    const auto digit = static_cast<unsigned>(c - '0');
    if (digit > 9 || digit > limit || value > (limit - digit) / 10)
      return std::nullopt;
    value = value * 10 + digit;
  }
  return value;
}

bool knownSymbol(std::string_view symbol) {
  if (symbol.starts_with('#')) return decimal(symbol.substr(1), 118).has_value();
  if (symbol.size() > 2 && symbol.front() == '\'' && symbol.back() == '\'')
    symbol = symbol.substr(1, symbol.size() - 2);
  if (std::find(atomic_symbols.begin(), atomic_symbols.end(), symbol) != atomic_symbols.end())
    return true;
  for (std::string_view alias : {"Uun", "Uuu", "Uub", "Uut", "Uuq", "Uup", "Uuh", "Uus", "Uuo",
                                "b", "c", "n", "o", "p", "s", "si", "as", "se", "te"})
    if (symbol == alias) return true;
  return false;
}
}  // namespace

void SmilesValidator::checkAtom(const ast::Atom& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (!knownSymbol(node.symbol))
    accept.error(node, "Unknown atom symbol.", "smiles.symbol").property<&ast::Atom::symbol>();
  if (node.isotope && !decimal(*node.isotope, 65535))
    accept.error(node, "Isotope number is too large.", "smiles.isotope").property<&ast::Atom::isotope>();
  if (node.hcount && !decimal(node.hcount->substr(1), 255))
    accept.error(node, "Hydrogen count is too large.", "smiles.hcount").property<&ast::Atom::hcount>();
  if (node.atomClass && !decimal(*node.atomClass, 2147483647))
    accept.error(node, "Atom class is too large.", "smiles.atomClass").property<&ast::Atom::atomClass>();
  if (node.charge) {
    const auto charge = *node.charge;
    const auto digits = charge.substr(1);
    if (charge != "++" && charge != "--" &&
        !decimal(digits, charge.front() == '-' ? 128 : 127))
      accept.error(node, "Invalid or out-of-range formal charge.", "smiles.charge").property<&ast::Atom::charge>();
  }
  if (node.chiral && node.chiral->size() > 1 && *node.chiral != "@@") {
    auto label = node.chiral->substr(1);
    while (!label.empty() && (label.front() == ' ' || label.front() == '\''))
      label.remove_prefix(1);
    const auto kind = label.substr(0, 2);
    const auto digits = label.substr(2);
    const unsigned limit = kind == "TH" || kind == "AL" ? 2 :
                           kind == "SP" ? 3 : kind == "TB" ? 20 : 30;
    const auto permutation = decimal(digits, limit);
    if (!digits.empty() && (!permutation || *permutation == 0))
      accept.error(node, "Invalid chiral permutation.", "smiles.chiral").property<&ast::Atom::chiral>();
  }
}

void SmilesValidator::checkRings(const ast::Smiles& node,
                                 cursiv::ValidationAcceptor& accept) const {
  struct Open { const ast::RingBond* ring; const ast::BranchedAtom* atom; };
  std::unordered_map<std::uint32_t, Open> open;
  std::unordered_map<const ast::BranchedAtom*,
                     std::unordered_set<const ast::BranchedAtom*>> neighbors;
  const auto join = [&](const ast::BranchedAtom* first, const ast::BranchedAtom* second) {
    const bool added = neighbors[first].insert(second).second;
    neighbors[second].insert(first);
    return added;
  };
  const auto visit = [&](auto&& self, const ast::Chain* chain,
                         const ast::BranchedAtom* previous,
                         std::optional<ast::Bond> bond) -> void {
    if (!chain) return;
    const auto atom = [&](const ast::BranchedAtom* value) {
      if (!value) return;
      if (previous && bond != ast::Bond::Dot) join(previous, value);
      for (const auto* suffix : value->suffixes) {
        if (const auto* branch = cursiv::ast_cast<ast::Branch>(suffix)) {
          self(self, branch->chain, value, branch->bond);
        } else if (const auto* ring = cursiv::ast_cast<ast::RingBond>(suffix)) {
          auto digits = ring->number;
          if (digits.starts_with("%(")) { digits.remove_prefix(2); digits.remove_suffix(1); }
          else if (digits.starts_with('%')) digits.remove_prefix(1);
          const auto number = decimal(digits, 99999);
          if (!number || digits.size() > 5) {
            accept.error(*ring, "Ring number is too large.", "smiles.ringNumber")
                .property<&ast::RingBond::number>();
            continue;
          }
          const auto opening = open.find(*number);
          if (opening == open.end()) { open.emplace(*number, Open{ring, value}); continue; }
          if (opening->second.atom == value)
            accept.error(*ring, "A ring cannot join an atom to itself.", "smiles.ringSelf")
                .property<&ast::RingBond::number>();
          else if (!join(opening->second.atom, value))
            accept.error(*ring, "Ring closure duplicates an existing bond.", "smiles.ringDuplicate")
                .property<&ast::RingBond::number>();
          open.erase(opening);
        }
      }
      previous = value;
    };
    atom(chain->first);
    for (const auto* link : chain->rest) if (link) {
      bond = link->bond;
      atom(link->atom);
    }
  };
  visit(visit, node.chain, nullptr, std::nullopt);
  for (const auto& [_, value] : open)
    accept.error(*value.ring, "Unclosed ring bond.", "smiles.ringUnclosed")
        .property<&ast::RingBond::number>();
}

void register_validation_checks(SmilesCoreServices& services) {
  services.validator = std::make_unique<const SmilesValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&SmilesValidator::checkAtom>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&SmilesValidator::checkRings>(validator)});
}

}  // namespace smiles
