#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "ply/core/CoreServices.hpp"
#include "ply/core/Data.hpp"
int main() {
  const auto language =
      cursiv::create_language<ply::PlyCoreServices, ply::PlySharedCoreServices,
                              ply::install_core_module>();
  unsigned seen = 0;
  for (const char* path : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".ply")
        continue;
      std::ifstream f(entry.path(), std::ios::binary);
      const std::string source((std::istreambuf_iterator<char>(f)), {});
      const auto r = language.language.parser->parse(source);
      const auto* d = r.value_as<ply::ast::Document>();
      if (!d || !r.accepted || r.cst().recovered() || !r.diagnostics.empty()) {
        std::fprintf(stderr, "PLY parse: %s\n", entry.path().c_str());
        return 1;
      }
      const auto data = ply::read_data(*d);
      if (entry.path().filename() == "types.ply") {
        if (data || data.elements.size() != 2 ||
            data.elements[0].properties.size() != 11)
          return 1;
      } else if (!data) {
        std::fprintf(stderr, "PLY data: %s: %s\n", entry.path().c_str(),
                     data.issues[0].message);
        return 1;
      }
      ++seen;
    }
  std::printf("PLY: %u included inputs\n", seen);
  return seen == 5 ? 0 : 1;
}
