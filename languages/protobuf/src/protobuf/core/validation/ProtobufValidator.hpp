#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "protobuf/core/ast.hpp"

namespace protobuf {

class ProtobufValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
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
