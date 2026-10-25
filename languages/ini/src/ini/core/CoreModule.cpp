
#include "ini/core/CoreServices.hpp"
#include "ini/generated/INI/Module.hpp"

namespace ini {

void install_core_module(IniCoreServices& services) {
  ini::generated::INI::install_generated_module(services);
  install_reader(services);
}

}  // namespace ini
