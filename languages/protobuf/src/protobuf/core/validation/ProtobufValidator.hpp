#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "protobuf/core/ast.hpp"

namespace protobuf {

class ProtobufValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkMessage(const ast::MessageDef&, cursiv::ValidationAcceptor&) const;
  void checkEnumDef(const ast::EnumDef&, cursiv::ValidationAcceptor&) const;
  void checkOneofDef(const ast::Oneof&, cursiv::ValidationAcceptor&) const;
  void checkGroup(const ast::Group&, cursiv::ValidationAcceptor&) const;
  void checkExtensions(const ast::Extensions&, cursiv::ValidationAcceptor&) const;
  void checkField(const ast::Field &, cursiv::ValidationAcceptor &) const;
  void checkOneof(const ast::OneofField &, cursiv::ValidationAcceptor &) const;
  void checkMap(const ast::MapField &, cursiv::ValidationAcceptor &) const;
  void checkEnum(const ast::EnumField &, cursiv::ValidationAcceptor &) const;
  void checkRange(const ast::ReservedRange &,
                  cursiv::ValidationAcceptor &) const;
  void checkDefault(const ast::FieldOption &,
                    cursiv::ValidationAcceptor &) const;
  void checkNumber(const ast::NumberLiteral &,
                   cursiv::ValidationAcceptor &) const;
};

struct ProtobufCoreServices;
void register_validation_checks(ProtobufCoreServices &services);

} // namespace protobuf
