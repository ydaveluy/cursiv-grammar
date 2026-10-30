// The parser preserves a permissive key-file syntax. These checks enforce
// selected Desktop Entry 1.5 structural constraints on the resulting tree.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "desktopentry/core/ast.hpp"

namespace desktopentry {

class DesktopEntryValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDocument(const ast::Document&, cursiv::ValidationAcceptor&) const;
  void checkGroup(const ast::Group&, cursiv::ValidationAcceptor&) const;
  void checkEntry(const ast::Entry&, cursiv::ValidationAcceptor&) const;
};

struct DesktopEntryCoreServices;
void register_validation_checks(DesktopEntryCoreServices& services);

}  // namespace desktopentry
