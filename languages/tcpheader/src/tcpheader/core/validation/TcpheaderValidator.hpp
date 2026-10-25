#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tcpheader/core/ast.hpp"

namespace tcpheader {

class TcpheaderValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkHeader(const ast::SegmentHeader&, cursiv::ValidationAcceptor&) const;
};

struct TcpheaderCoreServices;
void register_validation_checks(TcpheaderCoreServices& services);

}  // namespace tcpheader
