// The plantuml command line: parse one file, link it, validate it, and
// answer on the exit code -- 0 clean, 1 diagnostics, 2 misuse, 3 fatal.

#include "cursiv/cli.hpp"
#include "cursiv/services/CoreLanguage.hpp"
#include "plantuml/CoreServices.hpp"

// A language is an installer: the three type arguments are all that
// separates one from another, so `run_cli` itself is compiled once for
// every language there is and no grammar reaches it.
constexpr auto kRegister =
    &cursiv::register_language<plantuml::PlantUMLCoreServices,
                           plantuml::PlantUMLSharedCoreServices,
                           plantuml::install_core_module>;

int main(int argc, char** argv) {
  return cursiv::run_cli<plantuml::PlantUMLSharedCoreServices>(
      argc, argv, kRegister);
}
