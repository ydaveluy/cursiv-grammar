#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "star/core/ast.hpp"
namespace star {
class StarValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkContent(const ast::Content &, cursiv::ValidationAcceptor &) const;
  void checkLoop(const ast::Loop &, cursiv::ValidationAcceptor &) const;
  void checkFrame(const ast::SaveFrame &, cursiv::ValidationAcceptor &) const;
  void checkTableEntry(const ast::TableEntry &,
                       cursiv::ValidationAcceptor &) const;
  void checkBlock(const ast::DataBlock &, cursiv::ValidationAcceptor &) const;
};
struct StarCoreServices;
void register_validation_checks(StarCoreServices &);
} // namespace star
