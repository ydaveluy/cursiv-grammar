#include "cursiv/LanguageBench.hpp"
#include "istc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      istc::IstcSharedServices, istc::register_lsp_services>(argc, argv);
}
