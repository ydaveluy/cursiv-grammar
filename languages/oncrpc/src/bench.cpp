#include "cursiv/LanguageBench.hpp"
#include "oncrpc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      oncrpc::OncrpcSharedServices, oncrpc::register_lsp_services>(argc, argv);
}
