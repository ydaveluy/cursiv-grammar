// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "verilog/core/CoreServices.hpp"
#include "verilog/core/validation/VerilogValidator.hpp"
#include "verilog/generated/Verilog/Module.hpp"

namespace verilog {

void install_core_module(VerilogCoreServices& services) {
  verilog::generated::Verilog::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace verilog
