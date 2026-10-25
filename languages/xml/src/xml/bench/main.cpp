#include "cursiv/bench/LanguageBench.hpp"
#include "xml/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xml::XmlSharedServices, xml::register_lsp_services>(argc, argv);
}
