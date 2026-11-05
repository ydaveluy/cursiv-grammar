#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "lrc/ast.hpp"

namespace lrc {

class LrcValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkTimestamp(const ast::Timestamp &,
                      cursiv::ValidationAcceptor &) const;
  void checkMetadata(const ast::Metadata &, cursiv::ValidationAcceptor &) const;
};

struct LrcCoreServices;
void register_validation_checks(LrcCoreServices &services);

} // namespace lrc
