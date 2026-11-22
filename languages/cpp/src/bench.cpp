#include "cursiv/LanguageBench.hpp"
#include "cpp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      cpp::CppSharedServices, cpp::register_lsp_services>(argc, argv);
}
