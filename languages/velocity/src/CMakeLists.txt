# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `velocityGenerated` (the compiled library) and `velocity_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID velocity
    GRAMMAR    velocity/core/VelocityGrammar.hpp
    NAMESPACE  velocity::Velocity
    EXTENSIONS .vm
    OUT        velocity/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/velocity/core/*.cpp)
add_library(velocityCore ${_core})
target_link_libraries(velocityCore PUBLIC velocityGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/velocity/cli/*.cpp)
add_executable(velocity_cli ${_cli})
target_link_libraries(velocity_cli
                      PRIVATE velocityCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/velocity/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/velocity/lsp/main.cpp)
add_library(velocityLsp ${_lsp})
target_link_libraries(velocityLsp PUBLIC velocityCore cursiv::lsp)
add_executable(velocity_lsp velocity/lsp/main.cpp)
target_link_libraries(velocity_lsp PRIVATE velocityLsp)

cursiv_add_benchmark(velocity SOURCES velocity/bench/main.cpp)

cursiv_package_language(velocity)
