#include "bencoding/CoreServices.hpp"
#include "bencoding/generated/Bencoding/Module.hpp"
#include "cursiv/services/CoreLanguage.hpp"
#include <array>
#include <cstdio>
#include <utility>
int main() {
  const auto language =
      cursiv::createLanguage<bencoding::BencodingCoreServices,
                              bencoding::BencodingSharedCoreServices,
                              bencoding::install_core_module>();
  const auto factory = bencoding::generated::Bencoding::createParser();
  for (const cursiv::Parser *parser : std::array<const cursiv::Parser *, 2>{
           language.language.parser.get(), factory.get()}) {
    for (const auto source : {"d1:ai1ee", "0:", "4:abc"}) {
      auto original = parser->parse(source);
      auto result = std::move(original);
      if (result.references) {
        std::fputs("bencoding allocated a reference table for a language without references\n", stderr);
        return 1;
      }
    }
  }
}
