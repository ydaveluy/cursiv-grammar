#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "sqlite/core/ast.hpp"
namespace sqlite {
class SqliteValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkBlob(const ast::BlobLit&, cursiv::ValidationAcceptor&) const;
  void checkBind(const ast::BindParam&, cursiv::ValidationAcceptor&) const;
  void checkJoin(const ast::JoinStep&, cursiv::ValidationAcceptor&) const;
};
struct SqliteCoreServices;
void register_validation_checks(SqliteCoreServices& services);
}  // namespace sqlite
