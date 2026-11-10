// The iso8601 command line: it checks a file and prints its diagnostics
// (`--help` lists the options). It returns 0 without errors, 1 for document
// errors, 2 for invalid arguments or paths, 3 for an internal or output error.

#include "cursiv/cli.hpp"
#include "cursiv/services/CoreLanguage.hpp"
#include "iso8601/CoreServices.hpp"

// Adds the language to the workspace that the command line creates.
constexpr auto kRegister =
    &cursiv::register_language<iso8601::Iso8601CoreServices,
                           iso8601::Iso8601SharedCoreServices,
                           iso8601::install_core_module>;

int main(int argc, char** argv) {
  return cursiv::run_cli<iso8601::Iso8601SharedCoreServices>(
      argc, argv, kRegister);
}
