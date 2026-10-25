#include "cursiv/bench/LanguageBench.hpp"
#include "zig/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      zig::ZigSharedServices, zig::register_lsp_services>(argc, argv);
}
