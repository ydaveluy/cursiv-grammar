#include "cursiv/bench/LanguageBench.hpp"
#include "csharp/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      csharp::CSharpSharedServices, csharp::register_lsp_services>(argc, argv);
}
