# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `lambdaGenerated` (the compiled library) and `lambda_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lambda
    GRAMMAR    lambda/core/LambdaGrammar.hpp
    NAMESPACE  lambda::Lambda
    EXTENSIONS .lambda
    OUT        lambda/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lambda/core/*.cpp)
add_library(lambdaCore ${_core})
target_link_libraries(lambdaCore PUBLIC lambdaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lambda/cli/*.cpp)
add_executable(lambda_cli ${_cli})
target_link_libraries(lambda_cli
                      PRIVATE lambdaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lambda/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lambda/lsp/main.cpp)
add_library(lambdaLsp ${_lsp})
target_link_libraries(lambdaLsp PUBLIC lambdaCore cursiv::lsp)
add_executable(lambda_lsp lambda/lsp/main.cpp)
target_link_libraries(lambda_lsp PRIVATE lambdaLsp)

cursiv_add_benchmark(lambda SOURCES lambda/bench/main.cpp)

cursiv_package_language(lambda)
