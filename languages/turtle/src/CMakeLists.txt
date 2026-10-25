# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `turtleGenerated` (the compiled library) and `turtle_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID turtle
    GRAMMAR    turtle/core/TurtleGrammar.hpp
    NAMESPACE  turtle::Turtle
    EXTENSIONS .ttl
    OUT        turtle/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtle/core/*.cpp)
add_library(turtleCore ${_core})
target_link_libraries(turtleCore PUBLIC turtleGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtle/cli/*.cpp)
add_executable(turtle_cli ${_cli})
target_link_libraries(turtle_cli
                      PRIVATE turtleCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtle/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/turtle/lsp/main.cpp)
add_library(turtleLsp ${_lsp})
target_link_libraries(turtleLsp PUBLIC turtleCore cursiv::lsp)
add_executable(turtle_lsp turtle/lsp/main.cpp)
target_link_libraries(turtle_lsp PRIVATE turtleLsp)

cursiv_add_benchmark(turtle SOURCES turtle/bench/main.cpp)

cursiv_package_language(turtle)
