# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `icalendarGenerated` (the compiled library) and `icalendar_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID icalendar
    GRAMMAR    icalendar/core/IcalendarGrammar.hpp
    NAMESPACE  icalendar::Icalendar
    EXTENSIONS .ics
    OUT        icalendar/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icalendar/core/*.cpp)
add_library(icalendarCore ${_core})
target_link_libraries(icalendarCore PUBLIC icalendarGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icalendar/cli/*.cpp)
add_executable(icalendar_cli ${_cli})
target_link_libraries(icalendar_cli
                      PRIVATE icalendarCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/icalendar/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/icalendar/lsp/main.cpp)
add_library(icalendarLsp ${_lsp})
target_link_libraries(icalendarLsp PUBLIC icalendarCore cursiv::lsp)
add_executable(icalendar_lsp icalendar/lsp/main.cpp)
target_link_libraries(icalendar_lsp PRIVATE icalendarLsp)

cursiv_add_benchmark(icalendar SOURCES icalendar/bench/main.cpp)

cursiv_package_language(icalendar)
