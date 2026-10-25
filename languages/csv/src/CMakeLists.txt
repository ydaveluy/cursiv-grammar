# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `csvGenerated` (the compiled library) and `csv_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID csv
    GRAMMAR    csv/core/CsvGrammar.hpp
    NAMESPACE  csv::Csv
    EXTENSIONS .csv
    OUT        csv/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csv/core/*.cpp)
add_library(csvCore ${_core})
target_link_libraries(csvCore PUBLIC csvGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csv/cli/*.cpp)
add_executable(csv_cli ${_cli})
target_link_libraries(csv_cli
                      PRIVATE csvCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/csv/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/csv/lsp/main.cpp)
add_library(csvLsp ${_lsp})
target_link_libraries(csvLsp PUBLIC csvCore cursiv::lsp)
add_executable(csv_lsp csv/lsp/main.cpp)
target_link_libraries(csv_lsp PRIVATE csvLsp)

cursiv_add_benchmark(csv SOURCES csv/bench/main.cpp)

cursiv_package_language(csv)
