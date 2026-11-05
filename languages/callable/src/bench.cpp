#include "cursiv/LanguageBench.hpp"
#include "callable/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      callable::CallableSharedServices, callable::register_lsp_services>(argc, argv);
}
