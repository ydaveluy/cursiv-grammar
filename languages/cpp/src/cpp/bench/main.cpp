#include "cursiv/bench/LanguageBench.hpp"
#include "cpp/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      cpp::CppSharedServices, cpp::register_lsp_services>(argc, argv);
}
