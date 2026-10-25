#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "qif/core/ast.hpp"

namespace qif {

class QifValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkRecords(const ast::Document &, cursiv::ValidationAcceptor &) const;
};

struct QifCoreServices;
void register_validation_checks(QifCoreServices &services);

} // namespace qif
