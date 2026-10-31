#include "protobuf/core/validation/ProtobufValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "protobuf/core/CoreServices.hpp"
#include "protobuf/core/Numbers.hpp"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <unordered_set>
namespace protobuf {
namespace {
std::string_view syntaxOf(const cursiv::AstNode& node) {
  const cursiv::AstNode* current=&node;
  while(current) {
    if(const auto* document=cursiv::ast_cast<ast::Document>(current))return document->syntax;
    current=current->parent;
  }
  return {};
}
bool messageSet(const ast::MessageDef& node) {
  for(const auto* option:node.options)
    if(option->name && !option->name->isExtension &&
       option->name->name=="message_set_wire_format")
      if(const auto* value=std::get_if<bool>(&option->value))return *value;
  return false;
}
std::int64_t rangeMax(const ast::ReservedRange& node) {
  const auto* owner=node.parent?cursiv::ast_cast<ast::MessageDef>(node.parent->parent):nullptr;
  // protoc's MessageSet `to max` sentinel is the exclusive INT32_MAX end.
  return owner && messageSet(*owner)?INT32_MAX-1:536870911;
}
std::string groupFieldName(const ast::Group& node) {
  std::string name(node.name);
  for(auto& c:name)if(c>='A' && c<='Z')c=static_cast<char>(c-'A'+'a');
  return name;
}
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
  if(syntaxOf(node)=="proto3"&&node.label==ast::FieldLabel::required_)
    accept.error(node,"Required fields are not allowed in proto3.","protobuf.field-label")
        .property<&ast::Field::label>();
  if(syntaxOf(node)=="proto2"&&!node.label)
    accept.error(node,"A proto2 field needs optional, required or repeated.","protobuf.field-label");
  if(cursiv::ast_cast<ast::ExtendDef>(node.parent) && node.label==ast::FieldLabel::required_)
    accept.error(node,"An extension cannot be required.","protobuf.extension-label")
        .property<&ast::Field::label>();
}
void ProtobufValidator::checkGroup(const ast::Group& node,
                                    cursiv::ValidationAcceptor& accept) const {
  field(node,node.number,accept);
  if(node.name.empty() || node.name.front()<'A' || node.name.front()>'Z')
    accept.error(node,"A group name must start with an ASCII capital letter.","protobuf.group-name")
        .property<&ast::MessageDef::name>();
  if(syntaxOf(node)=="proto3")
    accept.error(node,"Groups are not allowed in proto3.","protobuf.proto3-group");
  if(!cursiv::ast_cast<ast::Oneof>(node.parent) && !node.label)
    accept.error(node,"A group needs optional, required or repeated.","protobuf.field-label");
  if(cursiv::ast_cast<ast::ExtendDef>(node.parent) && node.label==ast::FieldLabel::required_)
    accept.error(node,"An extension cannot be required.","protobuf.extension-label")
        .property<&ast::Group::label>();
}
void ProtobufValidator::checkExtensions(const ast::Extensions& node,
                                         cursiv::ValidationAcceptor& accept) const {
  if(syntaxOf(node)=="proto3")
    accept.error(node,"Extension ranges are not allowed in proto3.","protobuf.proto3-extensions");
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
                     hi = enumeration ? INT32_MAX : rangeMax(node);
  const auto valid = [&](auto v) { return v && *v >= lo && *v <= hi; };
  if (!valid(from))
    accept
        .error(
            node,
            "Range bound is outside the range allowed in this declaration.",
            "protobuf.reserved-range")
        .property<&ast::ReservedRange::from>();
  if (node.to && !valid(to))
    accept
        .error(
            node,
            "Range bound is outside the range allowed in this declaration.",
            "protobuf.reserved-range")
        .property<&ast::ReservedRange::to>();
  else if (from && to && *from > *to)
    accept
        .error(node, "A range ends before its first value.",
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
  if(syntaxOf(node)=="proto3") {
    accept.error(node,"Explicit default values are not allowed in proto3.","protobuf.proto3-default")
        .property<&ast::FieldOption::value>();
    return;
  }
  if(cursiv::ast_cast<ast::Group>(node.parent)) {
    accept.error(node,"A group cannot have an explicit default.","protobuf.default-type");
    return;
  }
  const auto* field=cursiv::ast_cast<ast::Field>(node.parent);
  const auto* oneof=cursiv::ast_cast<ast::OneofField>(node.parent);
  if(cursiv::ast_cast<ast::MapField>(node.parent)||(field&&field->label==ast::FieldLabel::repeated_)) {
    accept.error(node,"Repeated or map fields cannot have defaults.","protobuf.default-type");return;
  }
  const auto* literal=std::get_if<ast::NumberLiteral*>(&node.value);
  if(!literal||!*literal||(!field&&!oneof))return;
  const auto type=field?field->type:oneof->type;
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
namespace {
bool reservedName(const auto& declarations,std::string_view name) {
  for(const auto* declaration:declarations)
    for(const auto value:declaration->fieldNames)if(value==name)return true;
  return false;
}
bool reservedNumber(const auto& declarations,std::int64_t value,std::int64_t max) {
  for(const auto* declaration:declarations)for(const auto* range:declaration->ranges) {
    const auto from=integer_value(range->from);
    const auto to=range->toMax?std::optional<std::int64_t>(max):range->to?integer_value(*range->to):from;
    if(from&&to&&value>=*from&&value<=*to)return true;
  }
  return false;
}
}
void ProtobufValidator::checkDocument(const ast::Document& node,cursiv::ValidationAcceptor& accept) const {
  if(node.syntax!="proto3"&&node.syntax!="proto2")
    accept.error(node,"Unrecognized Protobuf syntax identifier.","protobuf.syntax").property<&ast::Document::syntax>();
  for(std::size_t i=1;i<node.packages.size();++i)
    accept.error(*node.packages[i],"A file can have only one package declaration.","protobuf.package");
}
void ProtobufValidator::checkMessage(const ast::MessageDef& node,cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string> names;
  std::unordered_set<std::int64_t> numbers;
  std::vector<const ast::ReservedRange*> extensionRanges;
  for(const auto* declaration:node.extensions)
    for(const auto* range:declaration->ranges)extensionRanges.push_back(range);
  const auto bounds=[](const ast::ReservedRange& range) {
    const auto from=integer_value(range.from);
    const auto to=range.toMax?std::optional<std::int64_t>(rangeMax(range)):
                  range.to?integer_value(*range.to):from;
    return std::pair(from,to);
  };
  const auto add=[&](const cursiv::AstNode& field,std::string_view name,std::string_view text) {
    if(!names.insert(std::string(name)).second)
      accept.error(field,"Duplicate field name.","protobuf.duplicate-field");
    if(reservedName(node.reserved,name))
      accept.error(field,"Field name is reserved.","protobuf.reserved-field");
    if(const auto number=integer_value(text)) {
      if(!numbers.insert(*number).second)
        accept.error(field,"Duplicate field number.","protobuf.duplicate-field");
      if(reservedNumber(node.reserved,*number,536870911))
        accept.error(field,"Field number is reserved.","protobuf.reserved-field");
      for(const auto* range:extensionRanges) {
        const auto [from,to]=bounds(*range);
        if(from && to && *from<=*number && *number<=*to)
          accept.error(field,"A regular field cannot use an extension range.","protobuf.extension-field");
      }
    }
  };
  const auto addGroup=[&](const ast::Group* group) {
    add(*group,groupFieldName(*group),group->number);
  };
  for(const auto* field:node.fields)add(*field,field->name,field->number);
  for(const auto* field:node.mapFields)add(*field,field->name,field->number);
  for(const auto* group:node.groups)addGroup(group);
  for(const auto* oneof:node.oneofs) {
    for(const auto* field:oneof->fields)add(*field,field->name,field->number);
    for(const auto* group:oneof->groups)addGroup(group);
  }
  const auto overlaps=[&](const ast::ReservedRange& a,const ast::ReservedRange& b) {
    const auto [af,at]=bounds(a);const auto [bf,bt]=bounds(b);
    return af && at && bf && bt && *af<=*bt && *bf<=*at;
  };
  for(std::size_t i=0;i<extensionRanges.size();++i) {
    const auto& range=*extensionRanges[i];
    for(std::size_t j=0;j<i;++j)
      if(overlaps(range,*extensionRanges[j]))
        accept.error(range,"Extension ranges overlap.","protobuf.extension-overlap");
    for(const auto* reserved:node.reserved)for(const auto* other:reserved->ranges)
      if(overlaps(range,*other))
        accept.error(range,"An extension range overlaps a reserved range.","protobuf.extension-overlap");
  }
}
void ProtobufValidator::checkEnumDef(const ast::EnumDef& node,cursiv::ValidationAcceptor& accept) const {
  if(node.fields.empty()) {
    accept.error(node,"An enum must contain a value.","protobuf.empty-enum");
    return;
  }
  if(syntaxOf(node)=="proto3"&&integer_value(node.fields[0]->value)!=0)
    accept.error(*node.fields[0],"The first proto3 enum value must be zero.","protobuf.enum-first");
  bool alias=false;
  for(const auto* option:node.options)
    if(option->name&&!option->name->isExtension&&option->name->name=="allow_alias")
      if(const auto* value=std::get_if<bool>(&option->value))alias=*value;
  std::unordered_set<std::string_view> names;
  std::unordered_set<std::int64_t> numbers;
  for(const auto* field:node.fields) {
    if(!names.insert(field->name).second)
      accept.error(*field,"Duplicate enum name.","protobuf.duplicate-enum");
    if(reservedName(node.reserved,field->name))
      accept.error(*field,"Enum name is reserved.","protobuf.reserved-enum");
    if(const auto number=integer_value(field->value)) {
      if(!numbers.insert(*number).second&&!alias)
        accept.error(*field,"Duplicate enum number needs allow_alias=true.","protobuf.duplicate-enum");
      if(reservedNumber(node.reserved,*number,INT32_MAX))
        accept.error(*field,"Enum number is reserved.","protobuf.reserved-enum");
    }
  }
}
void ProtobufValidator::checkOneofDef(const ast::Oneof& node,cursiv::ValidationAcceptor& accept) const {
  if(node.fields.empty() && node.groups.empty())accept.error(node,"A oneof must contain a field.","protobuf.empty-oneof");
}
void register_validation_checks(ProtobufCoreServices &services) {
  services.validator = std::make_unique<const ProtobufValidator>(services);
  using V = ProtobufValidator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMessage>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkEnumDef>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkOneofDef>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkGroup>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkExtensions>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkField>(
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
