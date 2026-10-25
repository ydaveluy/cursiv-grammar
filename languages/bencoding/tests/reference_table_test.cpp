#include "bencoding/core/CoreServices.hpp"
#include "bencoding/generated/Bencoding/Module.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include <array>
#include <cstdio>
#include <utility>
int main() {
  const auto language =
      cursiv::create_language<bencoding::BencodingCoreServices,
                              bencoding::BencodingSharedCoreServices,
                              bencoding::install_core_module>();
  const auto factory = bencoding::generated::Bencoding::create_parser();
  for (const cursiv::Parser *parser : std::array<const cursiv::Parser *, 2>{
           language.language.parser.get(), factory.get()}) {
    for (const auto source : {"d1:ai1ee", "0:", "4:abc"}) {
      auto original = parser->parse(source);
      auto result = std::move(original);
      if (!result.references || !result.references->handles.empty() ||
          result.references->anyUnresolved()) {
        std::fputs("bencoding: missing empty reference table\n", stderr);
        return 1;
      }
      bool visited = false;
      result.references->forceAll(cursiv::kNoCancel);
      result.references->visitResolved(
          result.cst(), [&](cursiv::TextOffset, cursiv::TextOffset,
                            const cursiv::AstNode &, bool) { visited = true; });
      result.references->reset();
      if (visited)
        return 1;
    }
  }
}
