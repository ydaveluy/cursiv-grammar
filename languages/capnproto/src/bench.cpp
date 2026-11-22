#include "cursiv/LanguageBench.hpp"
#include "capnproto/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      capnproto::CapnprotoSharedServices, capnproto::register_lsp_services>(argc, argv);
}
