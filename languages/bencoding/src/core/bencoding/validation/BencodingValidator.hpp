#pragma once

#include "bencoding/ast.hpp"
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"

namespace bencoding {

class BencodingValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkInteger(const ast::Integer &, cursiv::ValidationAcceptor &) const;
  void checkString(const ast::ByteString &, cursiv::ValidationAcceptor &) const;
  void checkDocument(const ast::Document &, cursiv::ValidationAcceptor &) const;
  void checkUniqueKeys(const ast::Dict &dict,
                       cursiv::ValidationAcceptor &accept) const;
};

struct BencodingCoreServices;
void register_validation_checks(BencodingCoreServices &services);

} // namespace bencoding
