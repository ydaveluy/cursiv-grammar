#pragma once
#include "cursiv/core/references/DefaultScopeProvider.hpp"

namespace xsmp {
class XsmpScopeProvider final : public cursiv::DefaultScopeProvider {
 public:
  using cursiv::DefaultScopeProvider::DefaultScopeProvider;
  const cursiv::AstNodeDescription* getScopeEntry(
      const cursiv::ReferenceInfo&, std::string_view) const override;
  bool visitScopeEntries(
      const cursiv::ReferenceInfo&,
      cursiv::function_ref<bool(const cursiv::AstNodeDescription&)>)
      const override;
  bool visitScopeEntries(
      const cursiv::ReferenceInfo&, std::string_view,
      cursiv::function_ref<bool(const cursiv::AstNodeDescription&)>)
      const override;

 private:
  bool visitLocal(
      const cursiv::ReferenceInfo&, std::string_view,
      cursiv::function_ref<bool(const cursiv::AstNodeDescription&)>) const;
};
}  // namespace xsmp
