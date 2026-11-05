#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "vmf/ast.hpp"

namespace vmf {

class VmfValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkKey(const ast::KeyValue&, cursiv::ValidationAcceptor&) const;
};

struct VmfCoreServices;
void register_validation_checks(VmfCoreServices& services);

}  // namespace vmf
