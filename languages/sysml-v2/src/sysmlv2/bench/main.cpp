#include "cursiv/bench/LanguageBench.hpp"
#include "sysmlv2/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sysmlv2::SysmlV2SharedServices, sysmlv2::register_lsp_services>(argc, argv);
}
