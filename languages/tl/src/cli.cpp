// The tl command line: it checks a file and prints its diagnostics
// (`--help` lists the options). It returns 0 without errors, 1 for document
// errors, 2 for invalid arguments or paths, 3 for an internal or output error.

#include "cursiv/cli.hpp"
#include "cursiv/services/CoreLanguage.hpp"
#include "tl/CoreServices.hpp"

// Adds the language to the workspace that the command line creates.
constexpr auto kRegister =
    &cursiv::register_language<tl::TlCoreServices,
                           tl::TlSharedCoreServices,
                           tl::install_core_module>;

int main(int argc, char** argv) {
  return cursiv::run_cli<tl::TlSharedCoreServices>(
      argc, argv, kRegister);
}
