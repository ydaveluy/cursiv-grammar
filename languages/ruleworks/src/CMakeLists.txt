# Generate the parser and its language module.
cursiv_generate_language(ID ruleworks
    GRAMMAR    ruleworks/core/RuleworksGrammar.hpp
    NAMESPACE  ruleworks::Ruleworks
    EXTENSIONS .rul
    OUT        ruleworks/generated)

# The grammar's write-action table is large enough (~150 terminals, ~150
# rules) to need more than the compiler's default template recursion depth.
if(CMAKE_CROSSCOMPILING)
  cursiv_host_manifest("target_compile_options(ruleworks_gen PRIVATE -ftemplate-depth=4000)")
else()
  target_compile_options(ruleworks_gen PRIVATE -ftemplate-depth=4000)
endif()
target_compile_options(ruleworksGenerated PRIVATE -ftemplate-depth=4000)
# The generated reflection and grammar metadata also exceed GCC's default
# constexpr budget; match the budget used by Cursiv's grammar generators.
target_compile_options(ruleworksGenerated PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=2147483647>)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/core/*.cpp)
add_library(ruleworksCore ${_core})
target_link_libraries(ruleworksCore PUBLIC ruleworksGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/cli/*.cpp)
if(_cli)
  add_executable(ruleworks_cli ${_cli})
  target_link_libraries(ruleworks_cli
                        PRIVATE ruleworksCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/lsp/main.cpp)
  add_library(ruleworksLsp ${_lsp})
  target_link_libraries(ruleworksLsp PUBLIC ruleworksCore cursiv::lsp)
  add_executable(ruleworks_lsp ruleworks/lsp/main.cpp)
  target_link_libraries(ruleworks_lsp PRIVATE ruleworksLsp)
endif()

if(TARGET ruleworksLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/bench/main.cpp")
  cursiv_add_benchmark(ruleworks
      SOURCES ruleworks/bench/main.cpp)
endif()

cursiv_package_language(ruleworks)
