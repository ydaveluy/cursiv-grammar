#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "vmf/core/ast.hpp"

namespace vmf {

class VmfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkKey(const ast::KeyValue&, cursiv::ValidationAcceptor&) const;
};

struct VmfCoreServices;
void register_validation_checks(VmfCoreServices& services);

}  // namespace vmf
