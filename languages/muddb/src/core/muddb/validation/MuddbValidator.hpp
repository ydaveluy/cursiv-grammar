#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "muddb/ast.hpp"
namespace muddb {
class MuddbValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkRoom(const ast::Room &, cursiv::ValidationAcceptor &) const;
  void checkDb(const ast::Db &, cursiv::ValidationAcceptor &) const;
  void checkKey(const ast::KeyReference &, cursiv::ValidationAcceptor &) const;
};
struct MuddbCoreServices;
void register_validation_checks(MuddbCoreServices &);
} // namespace muddb
