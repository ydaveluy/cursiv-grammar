#include "cursiv/LanguageBench.hpp"
#include "aspectj/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      aspectj::AspectjSharedServices, aspectj::register_lsp_services>(argc, argv);
}
