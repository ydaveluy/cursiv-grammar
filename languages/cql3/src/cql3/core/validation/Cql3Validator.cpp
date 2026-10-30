#include "cql3/core/validation/Cql3Validator.hpp"

#include <charconv>
#include <memory>
#include <string>

#include "cql3/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace cql3 {

void Cql3Validator::checkColumnType(const ast::ColumnType& node,
                                    cursiv::ValidationAcceptor& accept) const {
  if (node.quoted) {
    if (!node.params.empty())
      accept.error(node, "A user type cannot have type parameters.");
    return;
  }
  std::string name(node.name);
  for (auto& c : name)
    if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
  std::size_t arity = 0;
  if (name == "list" || name == "set" || name == "frozen")
    arity = 1;
  else if (name == "map" || name == "vector")
    arity = 2;
  if ((arity && node.params.size() != arity) ||
      (!arity && name != "tuple" && !node.params.empty()))
    accept.error(node, "Incorrect number of type parameters.");
  if (name == "tuple" && node.params.empty())
    accept.error(node, "A tuple needs at least one component type.");
  if (name == "vector" && node.params.size() == 2 && node.params[1]) {
    const auto t = node.params[1]->name;
    std::uint64_t dim = 0;
    auto result = std::from_chars(t.data(), t.data() + t.size(), dim);
    if (result.ec != std::errc{} || result.ptr != t.data() + t.size() ||
        dim == 0)
      accept.error(*node.params[1],
                   "A vector dimension must be a positive integer.");
  }
}

void register_validation_checks(Cql3CoreServices& services) {
  services.validator = std::make_unique<const Cql3Validator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &Cql3Validator::checkColumnType>(*services.validator)});
}

}  // namespace cql3
