// The checks of this language: consistency no single node can see on its own.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "csv/core/ast.hpp"

namespace csv {

class CsvValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkRowFieldCount(const ast::Row& row,
                          cursiv::ValidationAcceptor& accept) const;
};

struct CsvCoreServices;
void register_validation_checks(CsvCoreServices& services);

}  // namespace csv
