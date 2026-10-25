# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `robotwarsGenerated` (the compiled library) and `robotwars_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID robotwars
    GRAMMAR    robotwars/core/RobotwarsGrammar.hpp
    NAMESPACE  robotwars::Robotwars
    EXTENSIONS .robotwars
    OUT        robotwars/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/robotwars/core/*.cpp)
add_library(robotwarsCore ${_core})
target_link_libraries(robotwarsCore PUBLIC robotwarsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/robotwars/cli/*.cpp)
add_executable(robotwars_cli ${_cli})
target_link_libraries(robotwars_cli
                      PRIVATE robotwarsCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/robotwars/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/robotwars/lsp/main.cpp)
add_library(robotwarsLsp ${_lsp})
target_link_libraries(robotwarsLsp PUBLIC robotwarsCore cursiv::lsp)
add_executable(robotwars_lsp robotwars/lsp/main.cpp)
target_link_libraries(robotwars_lsp PRIVATE robotwarsLsp)

cursiv_add_benchmark(robotwars SOURCES robotwars/bench/main.cpp)

cursiv_package_language(robotwars)
