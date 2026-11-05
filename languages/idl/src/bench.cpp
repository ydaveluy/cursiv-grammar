#include "cursiv/LanguageBench.hpp"
#include "idl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      idl::IdlSharedServices, idl::register_lsp_services>(argc, argv);
}
