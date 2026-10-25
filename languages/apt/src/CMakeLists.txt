# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `aptGenerated` (the compiled library) and `apt_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID apt
    GRAMMAR    apt/core/AptGrammar.hpp
    NAMESPACE  apt::Apt
    EXTENSIONS .list
    OUT        apt/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apt/core/*.cpp)
add_library(aptCore ${_core})
target_link_libraries(aptCore PUBLIC aptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apt/cli/*.cpp)
add_executable(apt_cli ${_cli})
target_link_libraries(apt_cli
                      PRIVATE aptCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apt/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/apt/lsp/main.cpp)
add_library(aptLsp ${_lsp})
target_link_libraries(aptLsp PUBLIC aptCore cursiv::lsp)
add_executable(apt_lsp apt/lsp/main.cpp)
target_link_libraries(apt_lsp PRIVATE aptLsp)

cursiv_add_benchmark(apt SOURCES apt/bench/main.cpp)

cursiv_package_language(apt)
