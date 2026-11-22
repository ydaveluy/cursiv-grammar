#include "cursiv/LanguageBench.hpp"
#include "csharp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      csharp::CSharpSharedServices, csharp::register_lsp_services>(argc, argv);
}
