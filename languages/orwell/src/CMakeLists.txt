# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `orwellGenerated` (the compiled library) and `orwell_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID orwell
    GRAMMAR    orwell/core/OrwellGrammar.hpp
    NAMESPACE  orwell::Orwell
    EXTENSIONS .orwell
    OUT        orwell/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/orwell/core/*.cpp)
add_library(orwellCore ${_core})
target_link_libraries(orwellCore PUBLIC orwellGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/orwell/cli/*.cpp)
add_executable(orwell_cli ${_cli})
target_link_libraries(orwell_cli
                      PRIVATE orwellCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/orwell/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/orwell/lsp/main.cpp)
add_library(orwellLsp ${_lsp})
target_link_libraries(orwellLsp PUBLIC orwellCore cursiv::lsp)
add_executable(orwell_lsp orwell/lsp/main.cpp)
target_link_libraries(orwell_lsp PRIVATE orwellLsp)

cursiv_add_benchmark(orwell SOURCES orwell/bench/main.cpp)

cursiv_package_language(orwell)
