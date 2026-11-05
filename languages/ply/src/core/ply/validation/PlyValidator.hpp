#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "ply/ast.hpp"
namespace ply {
class PlyValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document &, cursiv::ValidationAcceptor &) const;
};
struct PlyCoreServices;
void register_validation_checks(PlyCoreServices &services);
} // namespace ply
