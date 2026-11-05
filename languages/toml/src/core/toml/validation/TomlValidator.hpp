#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "toml/ast.hpp"

namespace toml {

class TomlValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkKey(const ast::Key &, cursiv::ValidationAcceptor &) const;
  void checkValue(const ast::KeyValue &, cursiv::ValidationAcceptor &) const;
  void checkArray(const ast::Array &, cursiv::ValidationAcceptor &) const;
  void checkDocument(const ast::Document &, cursiv::ValidationAcceptor &) const;
  void checkInlineTable(const ast::InlineTable &,
                        cursiv::ValidationAcceptor &) const;
};

struct TomlCoreServices;
void register_validation_checks(TomlCoreServices &services);

} // namespace toml
