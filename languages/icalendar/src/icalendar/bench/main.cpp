#include "cursiv/bench/LanguageBench.hpp"
#include "icalendar/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      icalendar::IcalendarSharedServices, icalendar::register_lsp_services>(argc, argv);
}
