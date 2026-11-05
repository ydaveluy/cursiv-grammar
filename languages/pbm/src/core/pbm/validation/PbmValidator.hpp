#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "pbm/ast.hpp"
namespace pbm {
class PbmValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkImage(const ast::Image &image,
                  cursiv::ValidationAcceptor &accept) const;
  void checkDocument(const ast::Document &document,
                     cursiv::ValidationAcceptor &accept) const;
};
struct PbmCoreServices;
void register_validation_checks(PbmCoreServices &services);
} // namespace pbm
