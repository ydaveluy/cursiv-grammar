// The ini command line: one language, one file, the verdict on the exit code.

#include "cursiv/cli/cli.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "ini/core/CoreServices.hpp"

int main(int argc, char** argv) {
  return cursiv::run_cli<ini::IniSharedCoreServices>(
      argc, argv,
      &cursiv::register_language<ini::IniCoreServices,
                                 ini::IniSharedCoreServices,
                                 ini::install_core_module>);
}
