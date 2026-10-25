# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pikeGenerated` (the compiled library) and `pike_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pike
    GRAMMAR    pike/core/PikeGrammar.hpp
    NAMESPACE  pike::Pike
    EXTENSIONS .pike
    OUT        pike/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pike/core/*.cpp)
add_library(pikeCore ${_core})
target_link_libraries(pikeCore PUBLIC pikeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pike/cli/*.cpp)
add_executable(pike_cli ${_cli})
target_link_libraries(pike_cli
                      PRIVATE pikeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pike/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pike/lsp/main.cpp)
add_library(pikeLsp ${_lsp})
target_link_libraries(pikeLsp PUBLIC pikeCore cursiv::lsp)
add_executable(pike_lsp pike/lsp/main.cpp)
target_link_libraries(pike_lsp PRIVATE pikeLsp)

cursiv_add_benchmark(pike SOURCES pike/bench/main.cpp)

cursiv_package_language(pike)
