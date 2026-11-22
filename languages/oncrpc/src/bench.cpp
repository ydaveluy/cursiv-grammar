#include "cursiv/LanguageBench.hpp"
#include "oncrpc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      oncrpc::OncrpcSharedServices, oncrpc::register_lsp_services>(argc, argv);
}
