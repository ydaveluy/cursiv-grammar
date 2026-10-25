#include "protobuf/core/validation/ProtobufValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "protobuf/core/CoreServices.hpp"
#include "protobuf/core/Numbers.hpp"
#include <memory>
namespace protobuf {
namespace {
void field(const cursiv::AstNode &node, std::string_view text,
           cursiv::ValidationAcceptor &accept) {
  const auto value = integer_value(text);
  if (!value || *value < 1 || *value > 536870911 ||
      (*value >= 19000 && *value <= 19999))
    accept
        .error(
            node,
            "A field number must be in 1..536870911, excluding 19000..19999.",
            "protobuf.field-number")
        .range(accept.featureSpan(node, "number", 0)
                   .value_or(accept.spanOf(node)));
}
} // namespace
void ProtobufValidator::checkField(const ast::Field &node,
                                   cursiv::ValidationAcceptor &accept) const {
  field(node, node.number, accept);
}
void ProtobufValidator::checkOneof(const ast::OneofField &node,
                                   cursiv::ValidationAcceptor &accept) const {
  field(node, node.number, accept);
}
void ProtobufValidator::checkMap(const ast::MapField &node,
                                 cursiv::ValidationAcceptor &accept) const {
  field(node, node.number, accept);
}
void ProtobufValidator::checkEnum(const ast::EnumField &node,
                                  cursiv::ValidationAcceptor &accept) const {
  const auto value = integer_value(node.value);
  if (!value || *value < INT32_MIN || *value > INT32_MAX)
    accept
        .error(node, "An enum value must fit in a signed 32-bit integer.",
               "protobuf.enum-number")
        .property<&ast::EnumField::value>();
}
void ProtobufValidator::checkRange(const ast::ReservedRange &node,
                                   cursiv::ValidationAcceptor &accept) const {
  const bool enumeration =
      node.parent && cursiv::ast_cast<ast::EnumDef>(node.parent->parent);
  const auto from = integer_value(node.from),
             to = node.to ? integer_value(*node.to) : from;
  const std::int64_t lo = enumeration ? INT32_MIN : 1,
                     hi = enumeration ? INT32_MAX : 536870911;
  const auto valid = [&](auto v) { return v && *v >= lo && *v <= hi; };
  if (!valid(from))
    accept
        .error(
            node,
            "Reserved bound is outside the range allowed in this declaration.",
            "protobuf.reserved-range")
        .property<&ast::ReservedRange::from>();
  if (node.to && !valid(to))
    accept
        .error(
            node,
            "Reserved bound is outside the range allowed in this declaration.",
            "protobuf.reserved-range")
        .property<&ast::ReservedRange::to>();
  else if (from && to && *from > *to)
    accept
        .error(node, "A reserved range ends before its first value.",
               "protobuf.reserved-range")
        .property<&ast::ReservedRange::to>();
}
void ProtobufValidator::checkNumber(const ast::NumberLiteral &node,
                                    cursiv::ValidationAcceptor &accept) const {
  if (!number_value(node.text, node.floating))
    accept
        .error(
            node,
            "Integer literal exceeds the unsigned 64-bit range in this base.",
            "protobuf.number-range")
        .property<&ast::NumberLiteral::text>();
}
void ProtobufValidator::checkDefault(const ast::FieldOption &node,
                                     cursiv::ValidationAcceptor &accept) const {
  if (!node.name || node.name->isExtension || node.name->name != "default")
    return;
  const auto *literal = std::get_if<ast::NumberLiteral *>(&node.value);
  if (!literal || !*literal)
    return;
  const auto *field = cursiv::ast_cast<ast::Field>(node.parent);
  if (!field)
    return;
  const auto type = field->type;
  const bool signed32 =
                 type == "int32" || type == "sint32" || type == "sfixed32",
             signed64 =
                 type == "int64" || type == "sint64" || type == "sfixed64",
             unsigned32 = type == "uint32" || type == "fixed32",
             unsigned64 = type == "uint64" || type == "fixed64";
  if (!signed32 && !signed64 && !unsigned32 && !unsigned64)
    return;
  const auto value = number_value((*literal)->text, (*literal)->floating);
  bool valid = false;
  if (value)
    std::visit(
        [&](auto n) {
          using T = decltype(n);
          if constexpr (!std::is_same_v<T, double>) {
            if (signed32) {
              if constexpr (std::is_signed_v<T>)
                valid = n >= INT32_MIN && n <= INT32_MAX;
              else
                valid = n <= INT32_MAX;
            } else if (signed64) {
              if constexpr (std::is_signed_v<T>)
                valid = true;
              else
                valid = n <= INT64_MAX;
            } else if constexpr (std::is_signed_v<T>)
              valid = n >= 0 && (!unsigned32 || n <= UINT32_MAX);
            else
              valid = !unsigned32 || n <= UINT32_MAX;
          }
        },
        *value);
  if ((unsigned32 || unsigned64) && (*literal)->text.starts_with('-'))
    valid = false;
  if (!valid)
    accept
        .error(node, "Numeric default is outside the range of its field type.",
               "protobuf.default-number")
        .property<&ast::FieldOption::value>();
}
void register_validation_checks(ProtobufCoreServices &services) {
  services.validator = std::make_unique<const ProtobufValidator>(services);
  using V = ProtobufValidator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkField>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkOneof>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMap>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkEnum>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRange>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDefault>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(
           *services.validator)});
}
} // namespace protobuf
