# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `yiniGenerated` (the compiled library) and `yini_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID yini
    GRAMMAR    yini/core/YiniGrammar.hpp
    NAMESPACE  yini::Yini
    EXTENSIONS .yini
    OUT        yini/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yini/core/*.cpp)
add_library(yiniCore ${_core})
target_link_libraries(yiniCore PUBLIC yiniGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yini/cli/*.cpp)
add_executable(yini_cli ${_cli})
target_link_libraries(yini_cli
                      PRIVATE yiniCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yini/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/yini/lsp/main.cpp)
add_library(yiniLsp ${_lsp})
target_link_libraries(yiniLsp PUBLIC yiniCore cursiv::lsp)
add_executable(yini_lsp yini/lsp/main.cpp)
target_link_libraries(yini_lsp PRIVATE yiniLsp)

cursiv_add_benchmark(yini SOURCES yini/bench/main.cpp)

cursiv_package_language(yini)
