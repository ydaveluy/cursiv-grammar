#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "tcpheader/ast.hpp"

namespace tcpheader {

class TcpheaderValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkHeader(const ast::SegmentHeader&, cursiv::ValidationAcceptor&) const;
};

struct TcpheaderCoreServices;
void register_validation_checks(TcpheaderCoreServices& services);

}  // namespace tcpheader
