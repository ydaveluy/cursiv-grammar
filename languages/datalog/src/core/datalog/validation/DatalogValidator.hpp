#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "datalog/ast.hpp"

namespace datalog {

class DatalogValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkClauseHead(const ast::Clause& clause,
                       cursiv::ValidationAcceptor& accept) const;
};

struct DatalogCoreServices;
void register_validation_checks(DatalogCoreServices& services);

}  // namespace datalog
