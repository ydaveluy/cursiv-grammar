#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "recfile/ast.hpp"

namespace recfile {

class RecfileValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkField(const ast::Field &, cursiv::ValidationAcceptor &) const;
  void checkComment(const ast::Comment &, cursiv::ValidationAcceptor &) const;
  void checkRawLine(const ast::RawLine &, cursiv::ValidationAcceptor &) const;
};

struct RecfileCoreServices;
void register_validation_checks(RecfileCoreServices &services);

} // namespace recfile
