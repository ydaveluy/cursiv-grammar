#include "cursiv/LanguageBench.hpp"
#include "csharp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      csharp::CSharpSharedServices, csharp::register_lsp_services>(argc, argv);
}
